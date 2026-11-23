#pragma once
#include "core/Common.h"
// UTF-8 한글/영문 텍스트. Windows 맑은 고딕을 GPU 글리프 아틀라스로 캐싱한다.
// height는 글꼴 크기(논리 픽셀)이며, 계기판 숫자는 기존 SevenSeg를 사용한다.
bool Text_Init();
void Text_Shutdown();
float Text_Width(const std::string& text, float height);
void Text_Draw(float x, float y, const std::string& text, float height, glm::vec4 color);
