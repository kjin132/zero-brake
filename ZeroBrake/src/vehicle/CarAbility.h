// [담당: 유현조] 핸드브레이크(드리프트)·부스트
//  CarPhysics(진우석)가 아래 배율 함수들을 불러서 속도·조향·접지력에 곱함
#pragma once
#include "core/GameTypes.h"

void CarAbility_Update(Car& car, const CarInput& in, float dt);  // 매 프레임, CarPhysics 보다 먼저
void CarAbility_OnBoostPad(Car& car);                            // 부스트 패드 밟았을 때 (RaceRules 가 호출)

float CarAbility_MaxSpeedScale(const Car& car);  // 부스트 중 > 1
float CarAbility_AccelScale(const Car& car);
float CarAbility_SteerScale(const Car& car);     // 핸드브레이크 중 > 1
float CarAbility_GripScale(const Car& car);      // 핸드브레이크 중 < 1
