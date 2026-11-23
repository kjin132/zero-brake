// [공동·고정] 키보드 입력 (폴링 방식)
//  - Input_Down(GLFW_KEY_UP)    : 누르고 있는 동안 true  (가속, 조향)
//  - Input_Pressed(GLFW_KEY_C) : 누른 그 프레임에만 true (토글, 메뉴)
#pragma once
#include "core/Common.h"

void Input_Update();             // 매 프레임 한 번 (main 루프에서 호출)
bool Input_Down(int key);
bool Input_Pressed(int key);
