// =====================================================================================
//  [담당: 유현조] HUD
//  - 왼쪽 위: LAP / 시간 / BEST,  왼쪽 아래(또는 오른쪽 아래): 속도·부스트,  순위
//  TODO(현조): 배치·색 다듬기, 패널 배경, 결과/카운트다운과 통일감
// =====================================================================================
#include "ui/Hud.h"
#include "ui/SevenSeg.h"
#include "core/Mesh.h"
#include "core/Shader.h"

void Hud_Begin()
{
	glDisable(GL_DEPTH_TEST);
	Shader_SetMat4("view", glm::mat4(1.0f));
	Shader_SetMat4("projection", glm::ortho(0.0f, (float)gWidth, (float)gHeight, 0.0f, -1.0f, 1.0f));
}

void Hud_End()
{
	glEnable(GL_DEPTH_TEST);
}

void Hud_Rect(float x, float y, float w, float h, glm::vec4 color)
{
	glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x + w * 0.5f, y + h * 0.5f, 0.0f));
	m = glm::scale(m, glm::vec3(w, h, 0.01f));
	DrawOpt o; o.color = color; o.lit = false;
	Draw_Mesh(gCube, m, o);
}

std::string Hud_FormatTime(float s)
{
	if (s < 0) s = 0;
	int m = (int)(s / 60.0f);
	float sec = s - m * 60.0f;
	char buf[32];
	snprintf(buf, sizeof(buf), "%02d:%05.2f", m, sec);
	return buf;
}

void Hud_DrawRace(const Game& g)
{
	const Car& p = g.cars[0];
	Hud_Begin();

	glm::vec4 panel(0.05f, 0.07f, 0.12f, 0.6f);
	glm::vec4 yellow(1.0f, 0.82f, 0.3f, 1.0f), cyan(0.45f, 0.9f, 1.0f, 1.0f), white(1.0f);

	// 왼쪽 위: LAP, POS, 시간
	Hud_Rect(16, 16, 380, 190, panel);
	SevenSeg_Text(32, 30, "LAP", 22, yellow);
	int lapShown = std::min(p.lap + 1, g.settings.laps);
	SevenSeg_Text(32, 60, std::to_string(lapShown) + "/" + std::to_string(g.settings.laps), 44, yellow);
	SevenSeg_Text(210, 30, "POS", 22, cyan);
	SevenSeg_Text(210, 60, std::to_string(p.rank) + "/" + std::to_string(g.carCount), 44, cyan);
	SevenSeg_Text(32, 125, Hud_FormatTime(g.race.time - g.race.lapStartTime), 44, white);
	if (g.race.bestLap > 0.0f)
		SevenSeg_Text(32, 178, "bESt " + Hud_FormatTime(g.race.bestLap), 18, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));

	// 오른쪽 아래: 속도(km/h) + 부스트 게이지
	float x = (float)gWidth - 336, y = (float)gHeight - 176;
	Hud_Rect(x, y, 320, 160, panel);
	int kmh = (int)(std::fabs(p.speed) * 3.6f);
	SevenSeg_Text(x + 20, y + 16, std::to_string(kmh), 64, white);
	float gauge = p.boostGauge;
	Hud_Rect(x + 20, y + 110, 280, 22, glm::vec4(1, 1, 1, 0.15f));
	Hud_Rect(x + 20, y + 110, 280 * gauge, 22, p.boostTime > 0.0f ? glm::vec4(1, 0.5f, 0.1f, 1) : glm::vec4(0, 0.85f, 1, 1));

	// 남은 제한 시간 (30초 이하일 때 빨갛게)
	float remain = g.settings.timeLimit - g.race.time;
	glm::vec4 rc = remain < 30.0f ? glm::vec4(1, 0.3f, 0.3f, 1) : glm::vec4(0.85f, 0.85f, 0.85f, 1);
	SevenSeg_Text(16, (float)gHeight - 46, Hud_FormatTime(remain), 30, rc);

	Hud_End();
}
