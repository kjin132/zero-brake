// [담당: 유현조] 카메라 — 3인칭 추적 / 1인칭 운전석 / 후방 미러 / 미니맵
#pragma once
#include "core/GameTypes.h"

struct CameraView
{
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
	glm::vec3 eye = glm::vec3(0.0f);
};

void Camera_Reset(const Game& g);           // 출발 시 카메라를 차 뒤로
void Camera_Update(const Game& g, float dt);  // 매 프레임 부드럽게 따라가기

CameraView Camera_Main(const Game& g, float aspect);     // g.viewMode 에 따라 3인칭/1인칭
CameraView Camera_Mirror(const Game& g, float aspect);   // 차 뒤쪽을 보는 시점
CameraView Camera_Minimap(const Game& g, float aspect);  // 위에서 내려다본 직각 투영
