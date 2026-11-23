// =====================================================================================
//  [담당: 유현조] 다중 뷰포트
//  - 순서: 메인 → 후방 미러(위쪽 가운데) → 미니맵(오른쪽 위)
//  - 각 뷰포트마다 glScissor 로 그 영역만 지우고 World_Draw 호출
//  - glViewport 의 (x, y)는 창 "왼쪽 아래"가 원점
//  TODO(현조): 위치·크기 조정, 테두리, 미니맵 차량 표시 모양
// =====================================================================================
#include "view/Viewports.h"
#include "view/Camera.h"
#include "game/Game.h"
#include "core/Mesh.h"
#include "core/Shader.h"

static void ClearRegion(int x, int y, int w, int h, glm::vec3 color)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}

void Viewports_Draw(const Game& g)
{
	// ① 메인 화면
	glViewport(0, 0, gWidth, gHeight);
	World_Draw(g, Camera_Main(g, (float)gWidth / gHeight));

	// ② 후방 미러 (위쪽 가운데)
	int mw = gWidth * 30 / 100, mh = gHeight * 12 / 100;
	int mx = (gWidth - mw) / 2, my = gHeight - mh - 12;
	ClearRegion(mx - 3, my - 3, mw + 6, mh + 6, glm::vec3(0.1f));          // 테두리
	ClearRegion(mx, my, mw, mh, glm::vec3(0.55f, 0.75f, 0.95f));
	glViewport(mx, my, mw, mh);
	World_Draw(g, Camera_Mirror(g, (float)mw / mh));

	// ③ 미니맵 (오른쪽 위)
	if (g.showMinimap)
	{
		int s = gHeight * 30 / 100;
		int x = gWidth - s - 12, y = gHeight - s - 12;
		ClearRegion(x - 3, y - 3, s + 6, s + 6, glm::vec3(0.1f));
		ClearRegion(x, y, s, s, glm::vec3(0.2f, 0.3f, 0.2f));
		glViewport(x, y, s, s);
		CameraView mm = Camera_Minimap(g, 1.0f);
		World_Draw(g, mm);

		// 차량 위치를 크게 표시 (위에서 보면 차가 너무 작아서)
		glDisable(GL_DEPTH_TEST);
		for (int i = 0; i < g.carCount; i++)
		{
			const Car& c = g.cars[i];
			if (!c.active) continue;
			glm::vec4 col = c.isPlayer ? glm::vec4(1, 0.2f, 0.1f, 1) : glm::vec4(1, 1, 0.2f, 1);
			DrawOpt o; o.color = col; o.lit = false;
			float r = c.isPlayer ? 7.0f : 5.0f;
			Draw_Mesh(gCube, glm::scale(glm::translate(glm::mat4(1.0f), c.pos + glm::vec3(0, 5, 0)), glm::vec3(r, 1, r)), o);
		}
		glEnable(GL_DEPTH_TEST);
	}

	glViewport(0, 0, gWidth, gHeight);
}
