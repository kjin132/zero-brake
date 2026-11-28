// 제목 표시줄 대신 화면 오른쪽 위에 그리는 창 버튼 (최소화 / 최대화 / 닫기)
#pragma once
#include "core/Common.h"

void WindowBar_Update(bool& quitRequested);   // 마우스 클릭·창 끌기 처리 (매 프레임)
void WindowBar_Draw();                        // 버튼 그리기 (화면 맨 마지막)
float WindowBar_Height();                     // 버튼 높이 (1280x720 기준 픽셀)
