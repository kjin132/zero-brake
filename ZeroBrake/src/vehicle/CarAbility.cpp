// =====================================================================================
//  [담당: 유현조] 핸드브레이크(드리프트)·부스트
//  - 제안서: Space = 감속 + 미끄러짐, Shift = 게이지 부스트, 부스트 패드 = 2초 가속
//  TODO(현조): 발동 조건, 중복 적용 방지, 게이지 충전 규칙, 드리프트 각도 정하기
// =====================================================================================
#include "vehicle/CarAbility.h"

static const float BOOST_DURATION = 2.0f;     // 부스트 지속 시간 (초)
static const float BOOST_SPEED_SCALE = 1.25f; // 부스트 중 최고 속도 배율
static const float GAUGE_USE_PER_SEC = 0.5f;  // Shift 부스트 게이지 소모량
static const float GAUGE_FILL_PER_SEC = 0.08f;

void CarAbility_Update(Car& car, const CarInput& in, float dt)
{
	// 핸드브레이크
	car.handbrakeOn = in.handbrake && std::fabs(car.speed) > 3.0f;
	if (car.handbrakeOn)
	{
		car.speed *= (1.0f - 0.6f * dt);                  // 약간 감속
		car.slipAngle += in.steer * glm::radians(40.0f) * dt;  // 진행 방향이 밀림 (드리프트)
		car.slipAngle = glm::clamp(car.slipAngle, glm::radians(-35.0f), glm::radians(35.0f));
	}

	// Shift 게이지 부스트
	if (in.boost && car.boostGauge > 0.0f)
	{
		car.boostGauge = std::max(0.0f, car.boostGauge - GAUGE_USE_PER_SEC * dt);
		car.boostTime = std::max(car.boostTime, 0.1f);
	}
	else
	{
		car.boostGauge = std::min(1.0f, car.boostGauge + GAUGE_FILL_PER_SEC * dt);
	}

	if (car.boostTime > 0.0f) car.boostTime = std::max(0.0f, car.boostTime - dt);
}

void CarAbility_OnBoostPad(Car& car)
{
	car.boostTime = BOOST_DURATION;   // 다시 밟으면 시간만 새로 채움 (중복 누적 X)
}

float CarAbility_MaxSpeedScale(const Car& car) { return car.boostTime > 0.0f ? BOOST_SPEED_SCALE : 1.0f; }
float CarAbility_AccelScale(const Car& car) { return car.boostTime > 0.0f ? 1.6f : 1.0f; }
float CarAbility_SteerScale(const Car& car) { return car.handbrakeOn ? 1.4f : 1.0f; }
float CarAbility_GripScale(const Car& car) { return car.handbrakeOn ? 0.15f : 1.0f; }
