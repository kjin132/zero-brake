// =====================================================================================
//  [담당: 유현조] HUD
//  - 왼쪽 위: LAP / 시간 / BEST,  왼쪽 아래(또는 오른쪽 아래): 속도·부스트,  순위
//  TODO(현조): 배치·색 다듬기, 패널 배경, 결과/카운트다운과 통일감
// =====================================================================================
#include "ui/Hud.h"
#include "ui/SevenSeg.h"
#include "ui/BitmapText.h"
#include "race/Ghost.h"
#include "vehicle/CarSpecs.h"
#include "core/Mesh.h"
#include "core/Shader.h"

void Hud_Begin()
{
	glDisable(GL_DEPTH_TEST);
	Shader_SetMat4("view", glm::mat4(1.0f));
	Shader_SetMat4("projection", glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f));
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
	glm::vec4 panel(0.035f, 0.055f, 0.09f, 0.88f), gold(1, 0.8f, 0.25f, 1), cyan(0.2f, 0.85f, 1, 1), white(1);
	Hud_Rect(18, 18, 300, 212, panel); Hud_Rect(18, 18, 4, 212, cyan);
	Text_Draw(34, 32, "바퀴", 17, gold); Text_Draw(180, 32, "순위", 17, cyan);
	SevenSeg_Text(34, 56, std::to_string(std::min(p.lap + 1, g.settings.laps)) + "/" + std::to_string(g.settings.laps), 38, gold);
	SevenSeg_Text(180, 56, std::to_string(p.rank) + "/" + std::to_string(g.carCount), 38, cyan);
	Text_Draw(34, 110, "현재 랩", 17, cyan);
	SevenSeg_Text(34, 130, Hud_FormatTime(p.startedLap ? g.race.time - g.race.lapStartTime : 0), 33, white);
	float best = Ghost_BestTime(g.settings.mapId, g.settings.playerCar);
	Text_Draw(34, 180, "최고 " + (best > 0 ? Hud_FormatTime(best) : "--:--.--"), 17, gold);
	Text_Draw(34, 206, "총 시간 " + Hud_FormatTime(g.race.time), 17, white);

	float x = 950, y = 546;
	Hud_Rect(x, y, 312, 156, panel);
	SevenSeg_Text(x + 18, y + 15, std::to_string((int)(std::fabs(p.speed) * 3.6f)), 58, white);
	Text_Draw(x + 228, y + 57, "KM/H", 14, cyan);
	for (int i = 0; i < 26; ++i)
	{
		float ratio = std::fabs(p.speed) / CarSpec_Get(p.type).maxSpeed;
		glm::vec4 color = i < 20 ? cyan : gold;
		if (i / 26.0f > ratio) color = glm::vec4(1, 1, 1, 0.13f);
		Hud_Rect(x + 18 + i * 10.5f, y + 87, 8, 17, color);
	}
	Text_Draw(x + 18, y + 118, p.boostTime > 0 ? "부스트 중" : "부스트", 17, p.boostTime > 0 ? gold : cyan);
	Hud_Rect(x + 110, y + 115, 182, 16, glm::vec4(1, 1, 1, 0.12f));
	Hud_Rect(x + 110, y + 115, 182 * glm::clamp(p.boostGauge, 0.0f, 1.0f), 16, p.boostTime > 0 ? gold : cyan);
	float remain = std::max(0.0f, g.settings.timeLimit - g.race.time);
	Text_Draw(20, 660, "남은 시간", 17, cyan);
	SevenSeg_Text(20, 680, Hud_FormatTime(remain), 24, remain < 30 ? glm::vec4(1, 0.3f, 0.2f, 1) : white);
	Text_Draw(278, 686, "C 시점 전환   M 지도   P 일시정지   R 복귀", 17, white);
	if (p.handbrakeOn) Text_Draw(540, 610, "드리프트", 23, gold);
	if (g.state == GameState::Race && g.race.time < 1.0f)
		Text_Draw((1280 - Text_Width("출발!", 70)) * 0.5f, 280, "출발!", 70, cyan);
	Hud_End();
}
