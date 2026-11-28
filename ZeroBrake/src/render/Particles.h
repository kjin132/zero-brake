// 파티클 효과 — 부스트 배기 불꽃·연기, 드리프트(핸드브레이크) 타이어 연기
//  반투명 육면체를 뒤에서부터(먼 것부터) 그려서 알파 블렌딩이 자연스럽게 보이게 함
#pragma once
#include "core/GameTypes.h"

void Particles_Clear();                          // 경기 시작 시
void Particles_Update(const Game& g, float dt);  // Race 상태에서 매 프레임 (생성 + 이동 + 소멸)
void Particles_Draw(glm::vec3 viewPos);          // World_Draw 의 반투명 단계
int  Particles_Count();                          // 테스트용
