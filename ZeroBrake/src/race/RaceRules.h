// [담당: 진우석] 경기 규칙 — 체크포인트·랩·순위·제한 시간·복귀
#pragma once
#include "core/GameTypes.h"

void RaceRules_Start(Game& g);                 // 출발 직전 초기화
void RaceRules_Update(Game& g, float dt);      // 매 프레임 (Race 상태)
void RaceRules_Respawn(Game& g, Car& car);     // R 키: 마지막 체크포인트로 복귀
bool RaceRules_IsOver(const Game& g);          // 결과 화면으로 넘어갈지
