// XZ 평면 OBB 충돌. 차량은 회전된 직사각형, 장애물은 축에 정렬된 상자다.
// Game.cpp의 최대 1/120초 이동 분할과 함께 고속 관통을 방지한다.
#include "race/Collision.h"
#include "world/Track.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"

namespace
{
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
void Walls(const Track& t, Car& c)
{
	glm::vec3 nearest, tangent;
	float distance = Track_DistanceFromCenter(t, c.pos, &nearest, &tangent);
	if (distance < 0.0001f) return;
	glm::vec3 outward = (c.pos - nearest) / distance; outward.y = 0;
	float limit = std::max(0.1f, t.roadWidth * 0.5f - Radius(CarBox(c), outward));
	if (distance <= limit) return;
	c.pos = nearest + outward * limit;
	glm::vec3 velocity = Car_Forward(c) * c.speed;
	// 옆으로 스치며 진행할 때 매 프레임 속도가 반감되는 현상을 피한다.
	float impact = glm::dot(velocity, outward);
	if (impact > 0.0f)
	{
		float ratio = glm::clamp(impact / std::max(1.0f, std::fabs(c.speed)), 0.0f, 1.0f);
		c.speed *= 1.0f - 0.55f * ratio;
		c.slipAngle *= 0.5f;
	}
}
void Obstacles(const Track& t, Car& c)
{
	for (const Obstacle& o : t.obstacles)
	{
		Box b = { o.pos, {1, 0, 0}, {0, 0, 1}, o.size.x * 0.5f, o.size.z * 0.5f };
		glm::vec3 normal; float depth;
		if (!Overlap(CarBox(c), b, normal, depth)) continue;
		c.pos -= normal * (depth + 0.01f);
		if (glm::dot(Car_Forward(c) * c.speed, normal) > 0.0f) c.speed *= -0.2f;
		c.slipAngle *= 0.5f;
	}
}
}

void Collision_Update(Game& g)
{
	// 두 번 풀어 차량끼리 밀린 결과가 벽/장애물 안에 남지 않도록 한다.
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int i = 0; i < g.carCount; ++i)
		{
			Car& a = g.cars[i]; if (!a.active) continue;
			for (int j = i + 1; j < g.carCount; ++j)
			{
				Car& b = g.cars[j]; if (!b.active) continue;
				glm::vec3 normal; float depth;
				if (!Overlap(CarBox(a), CarBox(b), normal, depth)) continue;
				a.pos -= normal * (depth * 0.5f + 0.005f);
				b.pos += normal * (depth * 0.5f + 0.005f);
				float closing = glm::dot(Car_Forward(a) * a.speed - Car_Forward(b) * b.speed, normal);
				if (closing > 0.0f) { a.speed *= 0.82f; b.speed *= 0.82f; }
			}
		}
		for (int i = 0; i < g.carCount; ++i)
			if (g.cars[i].active) { Obstacles(g.track, g.cars[i]); Walls(g.track, g.cars[i]); }
	}
}
