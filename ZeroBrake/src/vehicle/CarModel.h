// [담당: 유현조] 차량 3종 외형 (계층 모델: 차체 → 캐빈·유리창·스포일러·바퀴)
//  바퀴 회전각·조향각은 Car 의 wheelSpin / steerAngle (진우석 CarPhysics 가 계산) 을 그대로 사용
#pragma once
#include "core/GameTypes.h"

void CarModel_Draw(const Car& car);
// 위치 행렬을 직접 주고 그리기 (고스트 재생, 선택 화면 미리보기용)
void CarModel_DrawAt(CarType type, const glm::mat4& carMatrix, float steerAngle, float wheelSpin, float alpha);
