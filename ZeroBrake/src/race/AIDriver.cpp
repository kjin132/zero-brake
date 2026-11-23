// =====================================================================================
//  [담당: 진우석] AI 경쟁차 주행
//  - 중심선 점(Track.center)을 웨이포인트로 사용, 몇 칸 앞 점을 향해 조향
//  - 코너(앞쪽 방향이 많이 꺾이는 곳)에서는 감속
//  TODO(우석): 차마다 주행 라인 차이, 코너 감속 구간 조정
// =====================================================================================
#include "race/AIDriver.h"
#include "vehicle/CarPhysics.h"
#include "world/Track.h"

void AIDriver_Init(Game& g, Car& car, int index)
{
	static const float skill[MAX_CARS] = { 1.0f, 0.92f, 0.86f, 0.80f };
	car.aiSkill = skill[glm::clamp(index, 0, MAX_CARS - 1)];
	car.aiTarget = Track_NearestIndex(g.track, car.pos);
}

CarInput AIDriver_Think(const Game& g, Car& car, float dt)
{
	CarInput in;
	const auto& pts = g.track.center;
	int n = (int)pts.size();
	if (n < 2) return in;

	// 가까워지면 다음 점으로
	int lookAhead = 6;
	car.aiTarget = Track_NearestIndex(g.track, car.pos);
	glm::vec3 target = pts[(car.aiTarget + lookAhead) % n];

	// 목표 방향과 현재 방향의 각도 차이 → 조향
	glm::vec3 toTarget = target - car.pos;
	toTarget.y = 0.0f;
	float targetYaw = std::atan2(toTarget.x, toTarget.z);
	float diff = targetYaw - car.yaw;
	while (diff > PI) diff -= 2 * PI;
	while (diff < -PI) diff += 2 * PI;
	in.steer = glm::clamp(-diff * 2.5f, -1.0f, 1.0f);   // diff > 0 이면 왼쪽(steer -)

	// 앞쪽 커브 정도 → 감속
	glm::vec3 a = pts[(car.aiTarget + 2) % n] - pts[car.aiTarget % n];
	glm::vec3 b = pts[(car.aiTarget + 14) % n] - pts[(car.aiTarget + 12) % n];
	float bend = 1.0f - glm::dot(glm::normalize(a), glm::normalize(b));   // 0 = 직선
	bool corner = bend > 0.25f;
	in.throttle = corner && car.speed > 22.0f ? 0.0f : 1.0f;
	in.brake = corner && car.speed > 28.0f ? 0.6f : 0.0f;
	return in;
}
