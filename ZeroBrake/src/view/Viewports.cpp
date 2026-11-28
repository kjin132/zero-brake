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
#include "vehicle/CarSpecs.h"
#include "vehicle/CarPhysics.h"
#include "render/Lighting.h"

static void ClearRegion(int x, int y, int w, int h, glm::vec3 color)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
	glClearColor(color.r, color.g, color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}

// 미니맵용 화살표 (XZ 평면 삼각형, +z 방향을 가리킴, 위에서 보이게 법선 +y)
static const Mesh& ArrowMesh()
{
	static Mesh m;
	if (m.vao == 0)
	{
		glm::vec3 up(0, 1, 0);
		std::vector<Vertex> v = {
			{ { 0.0f, 0, 0.7f }, up, { 0.5f, 1 } }, { { -0.5f, 0, -0.5f }, up, { 0, 0 } }, { { 0.0f, 0, -0.2f }, up, { 0.5f, 0.2f } },
			{ { 0.0f, 0, 0.7f }, up, { 0.5f, 1 } }, { { 0.0f, 0, -0.2f }, up, { 0.5f, 0.2f } }, { { 0.5f, 0, -0.5f }, up, { 1, 0 } },
		};
		m = Mesh_Create(v);
	}
	return m;
}

static void DrawMapArrow(glm::vec3 pos, float yaw, float size, glm::vec4 color)
{
	DrawOpt o; o.color = color; o.lit = false;
	glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), pos), yaw, glm::vec3(0, 1, 0));
	Draw_Mesh(ArrowMesh(), glm::scale(m, glm::vec3(size, 1.0f, size)), o);
}

void Viewports_Draw(const Game& g)
{
	glm::vec3 sky = Lighting_SkyColor(g.settings.mapId);
	// ① 메인 화면
	glViewport(0, 0, gWidth, gHeight);
	World_Draw(g, Camera_Main(g, (float)gWidth / gHeight), g.viewMode != ViewMode::FirstPerson);

	// ② 후방 미러 (위쪽 가운데)
	int mw = gWidth * 30 / 100, mh = gHeight * 12 / 100;
	int mx = (gWidth - mw) / 2, my = gHeight - mh - 12;
	ClearRegion(mx - 3, my - 3, mw + 6, mh + 6, glm::vec3(0.1f));          // 테두리
	ClearRegion(mx, my, mw, mh, sky);
	glViewport(mx, my, mw, mh);
	World_Draw(g, Camera_Mirror(g, (float)mw / mh));

	// ③ 미니맵 (오른쪽 위)
	if (g.showMinimap)
	{
		int s = gHeight * 30 / 100;
		int top = 12 + gHeight * 34 / 720;               // 위쪽 창 버튼 밑으로
		int x = gWidth - s - 12, y = gHeight - s - top;
		ClearRegion(x - 3, y - 3, s + 6, s + 6, glm::vec3(0.1f));
		ClearRegion(x, y, s, s, glm::vec3(0.2f, 0.3f, 0.2f));
		glViewport(x, y, s, s);
		CameraView mm = Camera_Minimap(g, 1.0f);
		World_Draw(g, mm);

		// 위에서 보면 차가 너무 작아서 화살표로 표시
		glDisable(GL_DEPTH_TEST);
		// 트랙 진행 방향 (작은 회색 화살표) + 출발선 (흰 막대)
		int n = (int)g.track.center.size();
		for (int i = 0; i < n; i += std::max(1, n / 14))
		{
			glm::vec3 d = g.track.center[(i + 1) % n] - g.track.center[i];
			DrawMapArrow(g.track.center[i] + glm::vec3(0, 3, 0), std::atan2(d.x, d.z), 5.0f, glm::vec4(0.85f, 0.85f, 0.85f, 0.8f));
		}
		if (!g.track.checkpoints.empty())
		{
			DrawOpt line; line.color = glm::vec4(1); line.lit = false;
			glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), g.track.startPos + glm::vec3(0, 4, 0)), g.track.startYaw, glm::vec3(0, 1, 0));
			Draw_Mesh(gCube, glm::scale(m, glm::vec3(g.track.roadWidth + 4.0f, 0.5f, 2.5f)), line);
		}
		// 차량: AI = 차 색 화살표, 플레이어 = 크게 + 흰 테두리 (맨 위에)
		for (int pass = 0; pass < 2; pass++)
			for (int i = 0; i < g.carCount; i++)
			{
				const Car& c = g.cars[i];
				if (!c.active || (pass == 0) == c.isPlayer) continue;
				float size = c.isPlayer ? 13.0f : 9.0f;
				if (c.isPlayer) DrawMapArrow(c.pos + glm::vec3(0, 5, 0), c.yaw, size * 1.35f, glm::vec4(1));
				glm::vec3 col = c.isPlayer ? glm::vec3(1.0f, 0.25f, 0.1f) : CarSpec_Get(c.type).color;
				DrawMapArrow(c.pos + glm::vec3(0, 6, 0), c.yaw, size, glm::vec4(col, 1.0f));
			}
		glEnable(GL_DEPTH_TEST);
	}

	glViewport(0, 0, gWidth, gHeight);
}
