// [담당: 진우석] 차량 주행 (가속·제동·후진·조향, 바퀴 회전각 계산)
#pragma once
#include "core/GameTypes.h"

// 출발 그리드 배치 (gridIndex 0 = 플레이어 자리)
void CarPhysics_Reset(Car& car, const Track& track, int gridIndex);
// 입력에 따라 한 프레임 이동
void CarPhysics_Update(Car& car, const CarInput& in, float dt);

glm::vec3 Car_Forward(const Car& car);   // 차 앞 방향 (월드)
glm::vec3 Car_Right(const Car& car);     // 차 오른쪽 방향 (월드)
glm::mat4 Car_Matrix(const Car& car);    // 차 위치·방향 행렬 (크기 없음) → CarModel·Camera 가 사용
glm::vec3 Car_MoveDir(const Car& car);    // 실제로 움직이는 방향 (드리프트 미끄러짐 포함)
glm::vec3 Car_Velocity(const Car& car);  // 실제 속도 벡터 = 진행 방향 × speed + 충돌 반동(bump)
