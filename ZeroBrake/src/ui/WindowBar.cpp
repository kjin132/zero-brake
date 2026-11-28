// =====================================================================================
//  창 버튼: 윈도우 기본 제목 표시줄(흰색 창 테두리)을 없애고 게임 화면 위에 직접 그림
//  - 오른쪽 위: 최소화 / 최대화(복원) / 닫기
//  - 위쪽 띠를 마우스로 끌면 창 이동, 두 번 클릭하면 최대화 ↔ 복원
// =====================================================================================
#include "ui/WindowBar.h"
#include "ui/Hud.h"
#include "core/Mesh.h"

namespace
{
const float BTN_W = 46.0f, BTN_H = 30.0f;
int sHover = -1;                 // 0 최소화, 1 최대화, 2 닫기
int sPressed = -1;
bool sPrevDown = false, sDragging = false;
double sDragX = 0, sDragY = 0, sLastClick = -10;

float BtnX(int i) { return 1280.0f - BTN_W * (3 - i); }

// 커서 위치를 HUD 좌표(1280x720)로
bool CursorHud(float& x, float& y, double& rawX, double& rawY)
{
	int w, h; glfwGetWindowSize(gWindow, &w, &h);
	if (w <= 0 || h <= 0) return false;
	glfwGetCursorPos(gWindow, &rawX, &rawY);
	x = (float)(rawX * 1280.0 / w); y = (float)(rawY * 720.0 / h);
	return true;
}
int HitButton(float x, float y)
{
	if (y < 0 || y >= BTN_H) return -1;
	for (int i = 0; i < 3; ++i) if (x >= BtnX(i) && x < BtnX(i) + BTN_W) return i;
	return -1;
}
bool Maximized() { return glfwGetWindowAttrib(gWindow, GLFW_MAXIMIZED) == GLFW_TRUE; }
void ToggleMaximize() { if (Maximized()) glfwRestoreWindow(gWindow); else glfwMaximizeWindow(gWindow); }

// 회전된 막대 (X 표시용)
void Bar(float cx, float cy, float len, float thick, float angle, glm::vec4 color)
{
	glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f));
	m = glm::rotate(m, angle, glm::vec3(0, 0, 1));
	m = glm::scale(m, glm::vec3(len, thick, 0.01f));
	DrawOpt o; o.color = color; o.lit = false;
	Draw_Mesh(gCube, m, o);
}
}

float WindowBar_Height() { return BTN_H; }

void WindowBar_Update(bool& quitRequested)
{
	float x, y; double rawX, rawY;
	if (!CursorHud(x, y, rawX, rawY)) { sHover = -1; return; }
	bool down = glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	bool pressed = down && !sPrevDown, released = !down && sPrevDown;
	sPrevDown = down;
	sHover = HitButton(x, y);

	if (pressed)
	{
		sPressed = sHover;
		if (sPressed < 0 && y >= 0 && y < BTN_H)            // 위쪽 띠: 창 끌기 / 두 번 클릭
		{
			double now = glfwGetTime();
			if (now - sLastClick < 0.35) { ToggleMaximize(); sLastClick = -10; }
			else
			{
				sLastClick = now;
				if (!Maximized()) { sDragging = true; sDragX = rawX; sDragY = rawY; }
			}
		}
	}
	if (sDragging && down)
	{
		int wx, wy; glfwGetWindowPos(gWindow, &wx, &wy);
		int dx = (int)(rawX - sDragX), dy = (int)(rawY - sDragY);
		if (dx || dy) glfwSetWindowPos(gWindow, wx + dx, wy + dy);
	}
	if (released)
	{
		if (sPressed >= 0 && sPressed == sHover)
		{
			if (sPressed == 0) glfwIconifyWindow(gWindow);
			else if (sPressed == 1) ToggleMaximize();
			else quitRequested = true;
		}
		sPressed = -1; sDragging = false;
	}
}

void WindowBar_Draw()
{
	Hud_Begin();
	glm::vec4 icon(1.0f, 1.0f, 1.0f, 0.9f);
	for (int i = 0; i < 3; ++i)
	{
		float bx = BtnX(i), cx = bx + BTN_W * 0.5f, cy = BTN_H * 0.5f;
		glm::vec4 bg = glm::vec4(0.02f, 0.035f, 0.06f, 0.55f);
		if (sHover == i) bg = (i == 2) ? glm::vec4(0.85f, 0.12f, 0.15f, 0.95f) : glm::vec4(0.25f, 0.32f, 0.4f, 0.85f);
		Hud_Rect(bx, 0, BTN_W, BTN_H, bg);
		if (i == 0) Hud_Rect(cx - 6, cy, 12, 1.6f, icon);                 // ─
		else if (i == 1)
		{
			float s = 10;                                                   // □ (최대화 상태면 겹친 네모)
			float ox = Maximized() ? 1.5f : 0.0f;
			Hud_Rect(cx - s / 2 + ox, cy - s / 2 - ox, s, 1.4f, icon);
			Hud_Rect(cx - s / 2 + ox, cy + s / 2 - 1.4f - ox, s, 1.4f, icon);
			Hud_Rect(cx - s / 2 + ox, cy - s / 2 - ox, 1.4f, s, icon);
			Hud_Rect(cx + s / 2 - 1.4f + ox, cy - s / 2 - ox, 1.4f, s, icon);
			if (Maximized())
			{
				Hud_Rect(cx - s / 2 - 1.5f, cy - s / 2 + 1.5f, s - 1.5f, 1.4f, icon);
				Hud_Rect(cx - s / 2 - 1.5f, cy - s / 2 + 1.5f, 1.4f, s - 1.5f, icon);
			}
		}
		else { Bar(cx, cy, 14, 1.6f, PI / 4, icon); Bar(cx, cy, 14, 1.6f, -PI / 4, icon); }   // ✕
	}
	Hud_End();
}
