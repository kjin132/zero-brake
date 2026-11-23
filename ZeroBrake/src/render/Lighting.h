// [담당: 진우석] 조명 — 셰이더의 빛 uniform 설정 (맵마다 다른 분위기)
#pragma once
#include "core/GameTypes.h"

void Lighting_Init();
void Lighting_Apply(const Game& g, glm::vec3 viewPos);   // 뷰포트마다 World_Draw 시작 시
