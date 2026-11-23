// =====================================================================================
//  [담당: 유현조] 화면
//  - 글자는 7세그먼트로 표시 가능한 것만 사용 (CAr, StAGE, AI, PAUSE ...)
//  - 창 제목줄에도 현재 선택을 영어로 표시 (디버그·안내용)
//  TODO(현조): 타이틀 연출, 선택 화면 차량 미리보기 위치, 결과 화면 순위표
// =====================================================================================
#include "ui/Screens.h"
#include "ui/Hud.h"
#include "ui/SevenSeg.h"
#include "core/Input.h"
#include "core/Shader.h"
#include "vehicle/CarSpecs.h"
#include "vehicle/CarModel.h"
#include "render/Lighting.h"

static const char* kMapNames[2] = { "HIGH-SPEED CIRCUIT", "TECHNICAL CIRCUIT" };

static void SetWindowTitle(const std::string& s) { glfwSetWindowTitle(gWindow, ("ZERO BRAKE | " + s).c_str()); }

// 화면 가운데에 차를 돌려서 보여주기 (타이틀·선택 화면)
static void DrawCarPreview(const Game& g, CarType type, float cx, float cy, float size)
{
	int vs = (int)size;
	glViewport((int)(cx - size / 2), (int)(gHeight - cy - size / 2), vs, vs);
	glm::vec3 eye(6.0f, 3.5f, 7.0f);
	Shader_SetMat4("view", glm::lookAt(eye, glm::vec3(0, 0.8f, 0), glm::vec3(0, 1, 0)));
	Shader_SetMat4("projection", glm::perspective(glm::radians(40.0f), 1.0f, 0.1f, 100.0f));
	Lighting_Apply(g, eye);
	float a = (float)glfwGetTime() * 0.8f;
	CarModel_DrawAt(type, glm::rotate(glm::mat4(1.0f), a, glm::vec3(0, 1, 0)), 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, gWidth, gHeight);
}

// ---------------------------------------------------------------- Title
void Screens_UpdateTitle(Game& g)
{
	SetWindowTitle("Press ENTER");
	if (Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER)) g.state = GameState::Select;
}

void Screens_DrawTitle(const Game& g)
{
	glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawCarPreview(g, CarType::Balance, gWidth * 0.5f, gHeight * 0.45f, gHeight * 0.6f);

	Hud_Begin();
	Hud_Rect(0, gHeight * 0.08f, (float)gWidth, 8, glm::vec4(0.9f, 0.3f, 0.15f, 1));
	std::string msg = "PrESS EntEr";
	float sz = 40.0f;
	if (std::fmod(glfwGetTime(), 1.0) < 0.7)
		SevenSeg_Text((gWidth - SevenSeg_TextWidth(msg, sz)) * 0.5f, gHeight * 0.82f, msg, sz, glm::vec4(1));
	Hud_End();
}

// ---------------------------------------------------------------- Select
bool Screens_UpdateSelect(Game& g)
{
	RaceSettings& s = g.settings;
	if (Input_Pressed(GLFW_KEY_UP)) g.menuRow = (g.menuRow + 2) % 3;
	if (Input_Pressed(GLFW_KEY_DOWN)) g.menuRow = (g.menuRow + 1) % 3;
	int d = (Input_Pressed(GLFW_KEY_RIGHT) ? 1 : 0) - (Input_Pressed(GLFW_KEY_LEFT) ? 1 : 0);
	if (d != 0)
	{
		if (g.menuRow == 0) s.playerCar = (CarType)(((int)s.playerCar + d + 3) % 3);
		if (g.menuRow == 1) s.mapId = (s.mapId + d + 2) % 2;
		if (g.menuRow == 2) s.aiCount = glm::clamp(s.aiCount + d, 1, 3);
	}
	if (Input_Pressed(GLFW_KEY_ESCAPE)) g.state = GameState::Title;

	SetWindowTitle(std::string("Car: ") + CarSpec_Get(s.playerCar).name + " | Map: " + kMapNames[s.mapId] +
		" | AI: " + std::to_string(s.aiCount) + "  (Arrows: change, ENTER: start)");
	return Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER);
}

void Screens_DrawSelect(const Game& g)
{
	glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawCarPreview(g, g.settings.playerCar, gWidth * 0.7f, gHeight * 0.5f, gHeight * 0.7f);

	Hud_Begin();
	const char* labels[3] = { "CAr", "StAGE", "AI" };
	int values[3] = { (int)g.settings.playerCar + 1, g.settings.mapId + 1, g.settings.aiCount };
	for (int i = 0; i < 3; i++)
	{
		float y = gHeight * 0.25f + i * 110.0f;
		bool sel = (g.menuRow == i);
		Hud_Rect(60, y - 14, 440, 86, sel ? glm::vec4(0.9f, 0.3f, 0.15f, 0.9f) : glm::vec4(1, 1, 1, 0.08f));
		SevenSeg_Text(84, y, labels[i], 56, glm::vec4(1));
		SevenSeg_Text(400, y, std::to_string(values[i]), 56, glm::vec4(1, 0.85f, 0.3f, 1));
	}
	Hud_End();
}

// ---------------------------------------------------------------- Countdown / Pause
void Screens_DrawCountdown(const Game& g)
{
	Hud_Begin();
	int n = (int)std::ceil(g.countdown);
	std::string s = std::to_string(std::max(n, 1));
	float sz = 200.0f;
	SevenSeg_Text((gWidth - SevenSeg_TextWidth(s, sz)) * 0.5f, (gHeight - sz) * 0.5f, s, sz, glm::vec4(1, 0.85f, 0.2f, 1));
	Hud_End();
}

void Screens_DrawPause(const Game& g)
{
	Hud_Begin();
	Hud_Rect(0, 0, (float)gWidth, (float)gHeight, glm::vec4(0, 0, 0, 0.5f));
	std::string s = "PAUSE";
	float sz = 90.0f;
	SevenSeg_Text((gWidth - SevenSeg_TextWidth(s, sz)) * 0.5f, (gHeight - sz) * 0.5f, s, sz, glm::vec4(1));
	Hud_End();
}

// ---------------------------------------------------------------- Result
bool Screens_UpdateResult(Game& g)
{
	SetWindowTitle(g.race.timeOver && !g.race.playerFinished ? "TIME OVER  (ENTER: retry, Q: quit)" : "FINISH  (ENTER: retry, Q: quit)");
	return Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER);
}

void Screens_DrawResult(const Game& g)
{
	Hud_Begin();
	Hud_Rect(0, 0, (float)gWidth, (float)gHeight, glm::vec4(0, 0, 0, 0.55f));
	float cx = gWidth * 0.5f;
	glm::vec4 yellow(1, 0.85f, 0.3f, 1), white(1);

	if (g.race.timeOver && !g.race.playerFinished)
	{
		std::string s = "FAIL";
		SevenSeg_Text(cx - SevenSeg_TextWidth(s, 120) * 0.5f, gHeight * 0.3f, s, 120, glm::vec4(1, 0.3f, 0.3f, 1));
	}
	else
	{
		std::string s = "POS " + std::to_string(g.cars[0].rank);
		SevenSeg_Text(cx - SevenSeg_TextWidth(s, 120) * 0.5f, gHeight * 0.22f, s, 120, yellow);
		std::string t = Hud_FormatTime(g.cars[0].finishTime);
		SevenSeg_Text(cx - SevenSeg_TextWidth(t, 60) * 0.5f, gHeight * 0.48f, t, 60, white);
	}
	std::string b = "bESt " + Hud_FormatTime(g.race.bestLap);
	SevenSeg_Text(cx - SevenSeg_TextWidth(b, 36) * 0.5f, gHeight * 0.62f, b, 36, white);
	std::string e = "PrESS EntEr";
	SevenSeg_Text(cx - SevenSeg_TextWidth(e, 32) * 0.5f, gHeight * 0.8f, e, 32, white);
	Hud_End();
}
