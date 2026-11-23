// [담당: 유현조] 7세그먼트 글자 — 막대 7개를 켜고 꺼서 숫자·일부 영문을 표시
//  화면 좌표(픽셀, 왼쪽 위 원점)로 그림. Hud_Begin() 뒤에서 호출
//  지원 문자: 0-9  A b C d E F G H I J L n O P r S t U y  - : . / 공백
#pragma once
#include "core/Common.h"

// size = 글자 높이(픽셀). 반환값 = 그린 글자 폭 (다음 글자 x 계산용)
float SevenSeg_Text(float x, float y, const std::string& text, float size, glm::vec4 color);
float SevenSeg_TextWidth(const std::string& text, float size);
