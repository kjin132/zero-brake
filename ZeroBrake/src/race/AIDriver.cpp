// 거리 기반 선행 목표 + 곡률에 맞춘 감속. 점 밀도가 다른 두 맵에 같은 규칙을 적용한다.
#include "race/AIDriver.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "world/Track.h"

void AIDriver_Init(Game& g, Car& car, int index)
{
	static const float skill[MAX_CARS] = { 1, 0.94f, 0.88f, 0.82f };
	car.aiSkill = skill[glm::clamp(index, 0, MAX_CARS - 1)];
	car.aiTarget = Track_NearestIndex(g.track, car.pos);
}
CarInput AIDriver_Think(const Game& g, Car& car, float)
{
	CarInput in;
	const auto& pts = g.track.center;
	int n = (int)pts.size(); if (n < 3) return in;
	const CarSpec& spec = CarSpec_Get(car.type);
	int nearest = Track_NearestIndex(g.track, car.pos);
	car.aiTarget = nearest;
	float look = glm::clamp(5.0f + std::fabs(car.speed) * 0.32f, 6.0f, 18.0f);
	float distance = 0;
	int target = nearest;
	while (distance < look)
	{
		int next = (target + 1) % n;
		distance += glm::length(pts[next] - pts[target]); target = next;
		if (target == nearest) break;
	}
	glm::vec3 tangent = glm::normalize(pts[(target + 1) % n] - pts[(target - 1 + n) % n]);
	glm::vec3 right(-tangent.z, 0, tangent.x);
	float lane = car.aiSkill > 0.91f ? -0.8f : (car.aiSkill < 0.85f ? 0.8f : 0.0f);
	for (const Obstacle& o : g.track.obstacles)
	{
		glm::vec3 delta = o.pos - car.pos;
		if (glm::length(delta) < 30.0f && glm::dot(delta, Car_Forward(car)) > 0)
			lane = glm::dot(o.pos - pts[target], right) > 0 ? -1.4f : 1.4f;
	}
	glm::vec3 delta = pts[target] + right * lane - car.pos;
	float diff = std::remainder(std::atan2(delta.x, delta.z) - car.yaw, 2 * PI);
	float curvature = 2 * std::sin(diff) / std::max(2.0f, glm::length(delta));
	float speedFactor = glm::clamp(std::fabs(car.speed) / 8.0f, 0.15f, 1.0f);
	in.steer = glm::clamp(-car.speed * curvature / (glm::radians(spec.steerRate) * speedFactor), -1.0f, 1.0f);
	if (std::fabs(car.speed) < 2.0f) in.steer = glm::clamp(-diff * 2, -1.0f, 1.0f);

	float desired = spec.maxSpeed * car.aiSkill;
	distance = 0;
	// 앞쪽 코너까지의 제동 거리도 고려한다.
	for (int k = 1; k < n && distance < 65.0f; ++k)
	{
		int at = (nearest + k) % n;
		glm::vec3 a = pts[at] - pts[(at - 1 + n) % n];
		glm::vec3 b = pts[(at + 1) % n] - pts[at];
		float la = glm::length(a), lb = glm::length(b);
		distance += la;
		if (la < 0.001f || lb < 0.001f) continue;
		float angle = std::acos(glm::clamp(glm::dot(a / la, b / lb), -1.0f, 1.0f));
		float bend = angle / ((la + lb) * 0.5f);
		float cornerSpeed = std::sqrt(11.5f / std::max(bend, 0.0001f)) * car.aiSkill;
		float allowed = std::sqrt(cornerSpeed * cornerSpeed + 2 * spec.brake * 0.65f * std::max(0.0f, distance - 7.0f));
		desired = std::min(desired, allowed);
	}
	if (std::fabs(diff) > 0.9f) desired = std::min(desired, 10.0f);
	in.throttle = car.speed < desired ? 1.0f : 0.0f;
	in.brake = car.speed > desired + 1.0f ? glm::clamp((car.speed - desired) / 7.0f, 0.0f, 1.0f) : 0.0f;
	return in;
}
