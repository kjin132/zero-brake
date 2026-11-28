// 배경 — 잔디/보도 바닥, 나무, 건물, 관중석, 코너 방향 표지판, 가로등
#pragma once
#include "core/GameTypes.h"

void Scenery_Build(const Track& t);   // 트랙이 정해진 뒤 배경 배치
void Scenery_Draw();

// 가로등 불빛 위치 (밤 맵) — Lighting 이 점광원으로 사용
const std::vector<glm::vec3>& Scenery_LightPositions();
