// =====================================================================================
//  [담당: 진우석] 차량 주행
//  지금은 "일단 움직이는" 기본 버전. 차종별 감각·드리프트 연동 등은 여기서 다듬기.
//  - 핸드브레이크·부스트 효과는 CarAbility(유현조)가 주는 배율을 받아서 적용
// =====================================================================================
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "vehicle/CarAbility.h"

glm::vec3 Car_Forward(const Car& car) { return glm::vec3(std::sin(car.yaw), 0.0f, std::cos(car.yaw)); }
glm::vec3 Car_Right(const Car& car) { return glm::vec3(-std::cos(car.yaw), 0.0f, std::sin(car.yaw)); }

glm::mat4 Car_Matrix(const Car& car)
{
	glm::mat4 m = glm::translate(glm::mat4(1.0f), car.pos);
	return glm::rotate(m, car.yaw, glm::vec3(0, 1, 0));
}

void CarPhysics_Reset(Car& car, const Track& track, int gridIndex)
{
	// 출발선 뒤로 2열 그리드: 0 1 / 2 3
	glm::vec3 fwd(std::sin(track.startYaw), 0, std::cos(track.startYaw));
	glm::vec3 right(-std::cos(track.startYaw), 0, std::sin(track.startYaw));
	float side = (gridIndex % 2 == 0) ? -1.0f : 1.0f;
	float back = 6.0f + 7.0f * (gridIndex / 2);

	car.pos = track.startPos - fwd * back + right * side * (track.roadWidth * 0.22f);
	car.yaw = track.startYaw;
	car.speed = 0.0f;
	car.slipAngle = 0.0f;
	car.steerAngle = 0.0f;
	car.wheelSpin = 0.0f;
	car.safePos = car.pos;
	car.safeYaw = car.yaw;
}

void CarPhysics_Update(Car& car, const CarInput& in, float dt)
{
	const CarSpec& s = CarSpec_Get(car.type);
	float maxSpeed = s.maxSpeed * CarAbility_MaxSpeedScale(car) * (car.isPlayer ? 1.0f : car.aiSkill);

	// --- 가속 / 제동 / 후진 ---
	if (in.throttle > 0.0f)
		car.speed += s.accel * in.throttle * CarAbility_AccelScale(car) * dt;
	if (in.brake > 0.0f)
	{
		if (car.speed > 0.5f) car.speed -= s.brake * in.brake * dt;   // 제동
		else car.speed -= s.accel * 0.6f * in.brake * dt;             // 후진
	}
	// 자연 감속 (마찰·공기저항)
	float drag = 2.0f + 0.02f * car.speed * car.speed;
	if (in.throttle == 0.0f && in.brake == 0.0f)
	{
		if (car.speed > 0) car.speed = std::max(0.0f, car.speed - drag * dt);
		else car.speed = std::min(0.0f, car.speed + drag * dt);
	}
	car.speed = glm::clamp(car.speed, -s.reverseMax, maxSpeed);

	// --- 조향: 속도가 있어야 회전 (정지 상태에선 앞바퀴만 꺾임) ---
	car.steerAngle = glm::mix(car.steerAngle, in.steer * 30.0f, glm::clamp(10.0f * dt, 0.0f, 1.0f));
	float speedFactor = glm::clamp(std::fabs(car.speed) / 8.0f, 0.0f, 1.0f);
	float dir = (car.speed >= 0.0f) ? 1.0f : -1.0f;
	float steerRate = s.steerRate * CarAbility_SteerScale(car);
	car.yaw -= glm::radians(in.steer * steerRate * speedFactor * dir) * dt;

	// --- 미끄러짐 (핸드브레이크 시 CarAbility 가 grip 을 낮춤) ---
	float grip = s.grip * CarAbility_GripScale(car);
	car.slipAngle = glm::mix(car.slipAngle, 0.0f, glm::clamp(grip * 6.0f * dt, 0.0f, 1.0f));

	// --- 이동 ---
	float moveYaw = car.yaw + car.slipAngle;
	car.pos += glm::vec3(std::sin(moveYaw), 0.0f, std::cos(moveYaw)) * car.speed * dt;

	// --- 바퀴 회전 (반지름 0.35m 기준) ---
	car.wheelSpin += glm::degrees(car.speed * dt / 0.35f);
	car.wheelSpin = std::fmod(car.wheelSpin, 360.0f);
}
