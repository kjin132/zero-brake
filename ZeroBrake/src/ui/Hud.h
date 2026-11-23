// [담당: 유현조] HUD — 화면에 고정된 정보 (바퀴 수·시간·순위·속도·부스트)
#pragma once
#include "core/GameTypes.h"

void Hud_Begin();   // 직각 투영(픽셀 좌표, 왼쪽 위 원점), 깊이 테스트·조명 끔
void Hud_End();     // 원래 상태로
void Hud_Rect(float x, float y, float w, float h, glm::vec4 color);   // 화면 사각형

void Hud_DrawRace(const Game& g);
std::string Hud_FormatTime(float seconds);   // 75.3 → "01:15.30"
