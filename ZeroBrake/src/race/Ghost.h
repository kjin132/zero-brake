// [담당: 진우석 주담당 / 유현조 렌더링 연동] 최고 랩 기록 + 반투명 고스트 재생
#pragma once
#include "core/GameTypes.h"

void Ghost_StartRace(Game& g);
void Ghost_BeginLap(const Game& g);
void Ghost_InvalidateLap();
float Ghost_BestTime(int mapId, CarType type);                    // 출발 시 현재 맵/차종의 저장 기록 선택
void Ghost_Update(Game& g, float dt);             // 매 프레임: 현재 랩 기록 + 고스트(직전 랩) 위치 계산
void Ghost_OnPlayerLap(Game& g, float lapTime);   // 플레이어 1바퀴 완료 (RaceRules 가 호출)
bool Ghost_Visible();                             // 지금 고스트가 보이는지 (테스트용)
void Ghost_Draw(const Game& g);                   // 반투명으로 그리기 (World_Draw 맨 마지막)
