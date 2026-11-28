// [담당: 진우석] 조명 — 셰이더의 빛 uniform 설정 (맵마다 다른 분위기)
//  맵 0 고속 서킷 = 맑은 낮,  맵 1 테크니컬(도심) 서킷 = 밤 (가로등 + 전조등)
#pragma once
#include "core/GameTypes.h"

void Lighting_Init();
// 뷰포트마다 World_Draw 시작 시. fog = false 이면 안개 끔 (미니맵처럼 위에서 볼 때)
void Lighting_Apply(const Game& g, glm::vec3 viewPos, bool fog = true);

glm::vec3 Lighting_SkyColor(int mapId);   // 하늘(배경 지우기) 색
bool Lighting_IsNight(int mapId);         // 전조등·창문 불빛을 켤지
