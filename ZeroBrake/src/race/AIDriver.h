// [담당: 진우석] AI 경쟁차 주행 — 트랙 중심선을 따라감
#pragma once
#include "core/GameTypes.h"

void AIDriver_Init(Game& g, Car& car, int index);              // 출발 전 (차마다 실력 차이)
CarInput AIDriver_Think(const Game& g, Car& car, float dt);    // 매 프레임 조작값 결정
