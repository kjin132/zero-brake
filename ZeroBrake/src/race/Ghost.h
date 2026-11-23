// [담당: 진우석 주담당 / 유현조 렌더링 연동] 최고 랩 기록 + 반투명 고스트 재생
#pragma once
#include "core/GameTypes.h"

void Ghost_StartRace(Game& g);                    // 출발 시 (맵·차종이 바뀌면 기록 초기화)
void Ghost_Update(Game& g, float dt);             // 매 프레임: 현재 랩 기록 + 고스트 위치 계산
void Ghost_OnPlayerLap(Game& g, float lapTime);   // 플레이어 1바퀴 완료 (RaceRules 가 호출)
void Ghost_Draw(const Game& g);                   // 반투명으로 그리기 (World_Draw 맨 마지막)
