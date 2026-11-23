// =====================================================================================
//  [담당: 진우석] 충돌 처리
//  - 트랙 벽: 중심선에서 도로 폭/2 를 넘으면 안쪽으로 밀어내고 감속
//  - 차량끼리: 원(반지름) 겹침으로 서로 밀어냄
//  TODO(우석): 장애물 AABB, 고속 관통 방지(이동 분할), 감속 비율 조정
// =====================================================================================
#include "race/Collision.h"
#include "world/Track.h"
#include "vehicle/CarSpecs.h"

static void CollideWithWalls(const Track& t, Car& c)
{
	glm::vec3 nearest, tangent;
	float dist = Track_DistanceFromCenter(t, c.pos, &nearest, &tangent);
	float limit = t.roadWidth * 0.5f - CarSpec_Get(c.type).size.x * 0.5f;
	if (dist > limit)
	{
		glm::vec3 out = c.pos - nearest;
		out.y = 0.0f;
		if (glm::length(out) > 0.0001f) out = glm::normalize(out);
		c.pos = nearest + out * limit;      // 도로 안쪽으로 되돌림
		c.speed *= 0.5f;                    // 벽 충돌 감속
	}
}

static void CollideCars(Car& a, Car& b)
{
	float ra = CarSpec_Get(a.type).size.z * 0.4f;
	float rb = CarSpec_Get(b.type).size.z * 0.4f;
	glm::vec3 d = b.pos - a.pos;
	d.y = 0.0f;
	float len = glm::length(d);
	float minDist = ra + rb;
	if (len > 0.0001f && len < minDist)
	{
		glm::vec3 n = d / len;
		float push = (minDist - len) * 0.5f;
		a.pos -= n * push;
		b.pos += n * push;
		a.speed *= 0.9f;
		b.speed *= 0.9f;
	}
}

void Collision_Update(Game& g)
{
	for (int i = 0; i < g.carCount; i++)
		if (g.cars[i].active) CollideWithWalls(g.track, g.cars[i]);

	for (int i = 0; i < g.carCount; i++)
		for (int j = i + 1; j < g.carCount; j++)
			if (g.cars[i].active && g.cars[j].active) CollideCars(g.cars[i], g.cars[j]);
}
