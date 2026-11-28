// =====================================================================================
//  충돌 처리 — XZ 평면 OBB(회전된 직사각형) 판정 + 반발(튕김) 응답
//
//  판정: 차량 = 회전된 직사각형, 장애물 = 축 정렬 상자, 트랙 벽 = 중심선에서 도로 폭/2
//  응답:
//   1) 위치 보정: 겹친 만큼 밖으로 밀어냄 (Game.cpp 의 1/120초 분할과 함께 고속 관통 방지)
//   2) 속도 반발: "부딪히는 방향 속도"가 있을 때만 한 번 충격량을 줌
//      → 벽: 벽 쪽 속도를 반대로 튕기고(반발계수), 스치면 진행 방향을 벽을 따라 꺾음
//      → 차끼리: 질량이 같다고 보고 운동량을 나눔 (뒤에서 박으면 앞차는 밀려나고 뒷차는 느려짐)
//   속도 변화 중 차 진행 방향 성분은 speed 로, 옆 방향 성분은 bump(반동 속도)로 들어감
//   (부딪히는 순간에만 적용되므로 벽에 붙어 있어도 매 프레임 감속되지 않음)
// =====================================================================================
#include "race/Collision.h"
#include "world/Track.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"

namespace
{
const float WALL_RESTITUTION = 0.35f;      // 벽 반발계수 (0 = 안 튕김, 1 = 완전 탄성)
const float CAR_RESTITUTION = 0.45f;       // 차끼리 반발계수
const float OBSTACLE_RESTITUTION = 0.4f;
const float SCRAPE_LOSS = 0.12f;           // 벽에 긁힐 때 추가로 잃는 속도 비율 (정면일수록 큼)
const float MIN_IMPACT = 0.3f;             // 이보다 약하게 닿으면 튕기지 않고 미끄러지기만 함 (m/s)

struct Box { glm::vec3 pos, right, forward; float hx, hz; };
Box CarBox(const Car& c)
{
	const glm::vec3 size = CarSpec_Get(c.type).size;
	return { c.pos, Car_Right(c), Car_Forward(c), size.x * 0.5f, size.z * 0.5f };
}
float Radius(const Box& b, glm::vec3 axis)
{
	return std::fabs(glm::dot(b.right, axis)) * b.hx + std::fabs(glm::dot(b.forward, axis)) * b.hz;
}
// 분리축 판정. normal = a 에서 b 쪽으로 향하는 축, depth = 겹친 깊이
bool Overlap(const Box& a, const Box& b, glm::vec3& normal, float& depth)
{
	glm::vec3 d = b.pos - a.pos; d.y = 0;
	depth = 1e9f;
	glm::vec3 axes[] = { a.right, a.forward, b.right, b.forward };
	for (glm::vec3 axis : axes)
	{
		float projected = glm::dot(d, axis);
		float penetration = Radius(a, axis) + Radius(b, axis) - std::fabs(projected);
		if (penetration <= 0.0f) return false;
		if (penetration < depth)
		{
			depth = penetration;
			normal = axis * (projected >= 0.0f ? 1.0f : -1.0f);
		}
	}
	return true;
}

float WrapAngle(float a)
{
	while (a > PI) a -= 2 * PI;
	while (a < -PI) a += 2 * PI;
	return a;
}

// 속도 변화 dv 를 차에 적용: 진행 방향 성분 → speed, 나머지(옆) → bump
void ApplyVelocityChange(Car& c, glm::vec3 dv)
{
	dv.y = 0.0f;
	glm::vec3 dir = Car_MoveDir(c);
	float along = glm::dot(dv, dir);
	c.speed += along;
	c.bump += dv - dir * along;
}

// 고정된 면(벽·장애물)에 부딪힘. n = 면에서 차 쪽(밀어낼 쪽)을 향하는 방향
void HitStatic(Car& c, glm::vec3 n, float restitution)
{
	glm::vec3 v = Car_Velocity(c);
	float into = -glm::dot(v, n);          // 면 안쪽으로 들어가는 속도
	if (into <= 0.0f) return;              // 이미 멀어지는 중 → 속도는 그대로 (붙지 않음)

	float bounce = into > MIN_IMPACT ? restitution : 0.0f;
	ApplyVelocityChange(c, n * into * (1.0f + bounce));

	// 정면에 가까울수록 긁히는 손실이 큼 (스치면 거의 그대로 미끄러져 지나감)
	float headOn = glm::clamp(into / std::max(1.0f, glm::length(v)), 0.0f, 1.0f);
	c.speed *= 1.0f - SCRAPE_LOSS * headOn;
	c.slipAngle *= 0.5f;

	// 비스듬히 부딪히면 차 머리를 벽과 나란한 쪽으로 돌림 → 벽을 따라 튕겨 나가는 느낌
	glm::vec3 f = Car_Forward(c);
	glm::vec3 along = f - n * glm::dot(f, n);
	if (glm::length(along) > 0.2f)
	{
		along = glm::normalize(along);
		float diff = WrapAngle(std::atan2(along.x, along.z) - c.yaw);
		if (std::fabs(diff) < glm::radians(65.0f)) c.yaw += diff * 0.45f;
	}
}

void Walls(const Track& t, Car& c)
{
	glm::vec3 nearest, tangent;
	float distance = Track_DistanceFromCenter(t, c.pos, &nearest, &tangent);
	if (distance < 0.0001f) return;
	glm::vec3 outward = (c.pos - nearest) / distance; outward.y = 0;
	float limit = std::max(0.1f, t.roadWidth * 0.5f - Radius(CarBox(c), outward));
	if (distance <= limit) return;
	c.pos = nearest + outward * limit;      // 1) 위치 보정
	HitStatic(c, -outward, WALL_RESTITUTION);   // 2) 벽에서 도로 안쪽 방향으로 반발
}

void Obstacles(const Track& t, Car& c)
{
	for (const Obstacle& o : t.obstacles)
	{
		Box b = { o.pos, {1, 0, 0}, {0, 0, 1}, o.size.x * 0.5f, o.size.z * 0.5f };
		glm::vec3 normal; float depth;
		if (!Overlap(CarBox(c), b, normal, depth)) continue;
		c.pos -= normal * (depth + 0.01f);
		HitStatic(c, -normal, OBSTACLE_RESTITUTION);
	}
}

void Cars(Car& a, Car& b)
{
	glm::vec3 normal; float depth;
	if (!Overlap(CarBox(a), CarBox(b), normal, depth)) return;
	// 1) 위치 보정: 반씩 밀어냄
	a.pos -= normal * (depth * 0.5f + 0.005f);
	b.pos += normal * (depth * 0.5f + 0.005f);
	// 2) 다가가는 중일 때만 충격량 (같은 질량): a 는 뒤로, b 는 앞으로
	float closing = glm::dot(Car_Velocity(a) - Car_Velocity(b), normal);
	if (closing <= 0.0f) return;
	float j = closing * (1.0f + CAR_RESTITUTION) * 0.5f;
	ApplyVelocityChange(a, -normal * j);
	ApplyVelocityChange(b, normal * j);
}
}

void Collision_Update(Game& g)
{
	// 두 번 풀어 차량끼리 밀린 결과가 벽/장애물 안에 남지 않도록 한다.
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int i = 0; i < g.carCount; ++i)
		{
			if (!g.cars[i].active) continue;
			for (int j = i + 1; j < g.carCount; ++j)
				if (g.cars[j].active) Cars(g.cars[i], g.cars[j]);
		}
		for (int i = 0; i < g.carCount; ++i)
			if (g.cars[i].active) { Obstacles(g.track, g.cars[i]); Walls(g.track, g.cars[i]); }
	}
}
