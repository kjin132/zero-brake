// 핸드브레이크와 부스트. 패드 타이머와 Shift 게이지는 독립적으로 관리한다.
#include "vehicle/CarAbility.h"

void CarAbility_Update(Car& car, const CarInput& in, float dt)
{
	if (dt <= 0.0f) return;
	car.padBoostTime = std::max(0.0f, car.padBoostTime - dt);
	car.handbrakeOn = !car.finished && in.handbrake && std::fabs(car.speed) > 3.0f;
	if (car.handbrakeOn)
	{
		car.speed *= std::exp(-0.65f * dt);
		float target = in.steer * glm::radians(30.0f);
		car.slipAngle = glm::mix(car.slipAngle, target, 1.0f - std::exp(-4.0f * dt));
	}
	if (!in.boost) car.boostExhausted = false;
	bool manual = !car.finished && !car.handbrakeOn && in.boost &&
		!car.boostExhausted && car.boostGauge > 0.0f && car.speed > 2.0f && in.throttle > 0.0f;
	if (manual)
	{
		car.boostGauge = std::max(0.0f, car.boostGauge - 0.5f * dt);
		if (car.boostGauge <= 0.0f) car.boostExhausted = true;
	}
	else if (!in.boost)
		car.boostGauge = std::min(1.0f, car.boostGauge + (car.handbrakeOn ? 0.16f : 0.08f) * dt);
	if (car.finished) car.padBoostTime = 0.0f;
	car.boostTime = std::max(car.padBoostTime, manual ? 0.05f : 0.0f);
}

void CarAbility_OnBoostPad(Car& car)
{
	if (car.finished) return;
	car.padBoostTime = 2.0f;
	car.boostTime = std::max(car.boostTime, car.padBoostTime);
}

float CarAbility_MaxSpeedScale(const Car& car) { return car.boostTime > 0.0f ? 1.25f : 1.0f; }
float CarAbility_AccelScale(const Car& car) { return car.boostTime > 0.0f ? 1.6f : 1.0f; }
float CarAbility_SteerScale(const Car& car) { return car.handbrakeOn ? 1.35f : 1.0f; }
float CarAbility_GripScale(const Car& car) { return car.handbrakeOn ? 0.15f : 1.0f; }
