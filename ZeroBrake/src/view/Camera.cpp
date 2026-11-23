// =====================================================================================
//  [담당: 유현조] 카메라
//  - 3인칭: 목표 위치(차 뒤·위)로 현재 위치를 조금씩 보간 → 부드럽게 따라옴
//  - 속도가 빠를수록 시야각(FOV)이 넓어짐 (속도감)
//  TODO(현조): 거리·높이·추적 속도 조정, 급회전·복귀 시 흔들림 처리
// =====================================================================================
#include "view/Camera.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"

static const float CHASE_DIST = 9.0f;
static const float CHASE_HEIGHT = 3.8f;
static const float FOLLOW_SPEED = 6.0f;   // 클수록 빨리 따라옴

static glm::vec3 sEye(0.0f, 10.0f, -10.0f);
static glm::vec3 sLook(0.0f);
static float sFov = 60.0f;

static glm::vec3 ChaseTarget(const Car& c) { return c.pos - Car_Forward(c) * CHASE_DIST + glm::vec3(0, CHASE_HEIGHT, 0); }

void Camera_Reset(const Game& g)
{
	const Car& c = g.cars[0];
	sEye = ChaseTarget(c);
	sLook = c.pos + glm::vec3(0, 1.2f, 0);
	sFov = 60.0f;
}

void Camera_Update(const Game& g, float dt)
{
	const Car& c = g.cars[0];
	float k = glm::clamp(FOLLOW_SPEED * dt, 0.0f, 1.0f);
	sEye = glm::mix(sEye, ChaseTarget(c), k);
	sLook = glm::mix(sLook, c.pos + Car_Forward(c) * 4.0f + glm::vec3(0, 1.2f, 0), k * 1.5f);

	float speedRatio = glm::clamp(std::fabs(c.speed) / CarSpec_Get(c.type).maxSpeed, 0.0f, 1.3f);
	float targetFov = 60.0f + 18.0f * speedRatio + (c.boostTime > 0.0f ? 6.0f : 0.0f);
	sFov = glm::mix(sFov, targetFov, k);
}

CameraView Camera_Main(const Game& g, float aspect)
{
	CameraView v;
	const Car& c = g.cars[0];
	if (g.viewMode == ViewMode::FirstPerson)
	{
		v.eye = c.pos + Car_Forward(c) * 0.3f + glm::vec3(0, 1.4f, 0);
		v.view = glm::lookAt(v.eye, v.eye + Car_Forward(c), glm::vec3(0, 1, 0));
	}
	else
	{
		v.eye = sEye;
		v.view = glm::lookAt(sEye, sLook, glm::vec3(0, 1, 0));
	}
	v.proj = glm::perspective(glm::radians(sFov), aspect, 0.1f, 1000.0f);
	return v;
}

CameraView Camera_Mirror(const Game& g, float aspect)
{
	CameraView v;
	const Car& c = g.cars[0];
	v.eye = c.pos - Car_Forward(c) * 2.5f + glm::vec3(0, 1.8f, 0);
	v.view = glm::lookAt(v.eye, v.eye - Car_Forward(c) + glm::vec3(0, -0.05f, 0), glm::vec3(0, 1, 0));
	// 거울처럼 좌우 반전
	v.proj = glm::scale(glm::mat4(1.0f), glm::vec3(-1, 1, 1)) * glm::perspective(glm::radians(50.0f), aspect, 0.1f, 500.0f);
	return v;
}

CameraView Camera_Minimap(const Game& g, float aspect)
{
	CameraView v;
	glm::vec3 mn(1e9f), mx(-1e9f);
	for (const glm::vec3& p : g.track.center) { mn = glm::min(mn, p); mx = glm::max(mx, p); }
	glm::vec3 center = (mn + mx) * 0.5f;
	float half = std::max(mx.x - mn.x, mx.z - mn.z) * 0.5f + g.track.roadWidth;

	v.eye = center + glm::vec3(0, 200.0f, 0);
	v.view = glm::lookAt(v.eye, center, glm::vec3(0, 0, -1));   // 위에서 볼 땐 up 을 -z 로
	v.proj = glm::ortho(-half * aspect, half * aspect, -half, half, 1.0f, 500.0f);
	return v;
}
