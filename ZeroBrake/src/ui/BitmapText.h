#pragma once
#include "core/Common.h"
// 작은 안내 문자는 5x7 비트맵, 계기판 숫자는 SevenSeg를 사용한다.
float Text_Width(const std::string& text, float height);
void Text_Draw(float x, float y, const std::string& text, float height, glm::vec4 color);
