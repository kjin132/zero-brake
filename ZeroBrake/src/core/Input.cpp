// [공동·고정] 키보드 입력 (폴링 방식)
#include "core/Input.h"

static bool sNow[GLFW_KEY_LAST + 1];
static bool sPrev[GLFW_KEY_LAST + 1];

void Input_Update()
{
	for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; k++)
	{
		sPrev[k] = sNow[k];
		sNow[k] = glfwGetKey(gWindow, k) == GLFW_PRESS;
	}
}

bool Input_Down(int key) { return key >= 0 && key <= GLFW_KEY_LAST && sNow[key]; }
bool Input_Pressed(int key) { return key >= 0 && key <= GLFW_KEY_LAST && sNow[key] && !sPrev[key]; }
