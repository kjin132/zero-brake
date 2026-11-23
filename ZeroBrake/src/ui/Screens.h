// [담당: 유현조] 화면 — 타이틀 / 선택(차량·맵·AI 수) / 카운트다운 / 일시정지 / 결과
#pragma once
#include "core/GameTypes.h"

void Screens_UpdateTitle(Game& g);         // Enter → Select
bool Screens_UpdateSelect(Game& g);        // ↑↓ 항목, ←→ 값, Enter 면 true (경기 시작)
bool Screens_UpdateResult(Game& g);        // Enter 면 true (선택 화면으로)

void Screens_DrawTitle(const Game& g);
void Screens_DrawSelect(const Game& g);
void Screens_DrawCountdown(const Game& g);
void Screens_DrawPause(const Game& g);
void Screens_DrawResult(const Game& g);
