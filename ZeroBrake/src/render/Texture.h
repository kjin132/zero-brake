// [담당: 진우석] 텍스처 — 코드로 만든 무늬 텍스처 + (나중에) 이미지 파일 로드
#pragma once
#include "core/Common.h"

enum class TexId { Road = 0, Grass, Checker, Curb, WindowsDay, WindowsNight, Asphalt, Count };
// WindowsDay/Night: 건물 창문 무늬 (DrawOpt.uvScale 로 층수만큼 반복)
// Asphalt: 도심 맵 바닥(보도·광장)

void Texture_Init();             // Game_Init 에서 한 번
GLuint Texture_Get(TexId id);    // 0 이면 아직 없음 → 텍스처 없이 색만 그려짐
