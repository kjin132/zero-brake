// 1280x720 논리 좌표로 UI를 그려 창 크기 변경에도 메뉴가 잘리지 않게 한다.
#include "ui/Screens.h"
#include "ui/Hud.h"
#include "ui/SevenSeg.h"
#include "ui/BitmapText.h"
#include "core/Input.h"
#include "core/Shader.h"
#include "vehicle/CarSpecs.h"
#include "vehicle/CarModel.h"
#include "render/Lighting.h"
#include "race/Ghost.h"

static const glm::vec4 WHITE(1), CYAN(0.2f, 0.85f, 1, 1), GOLD(1, 0.8f, 0.25f, 1), MUTED(0.6f, 0.69f, 0.78f, 1);
static const char* MAPS[] = { "HIGH-SPEED CIRCUIT", "TECHNICAL CIRCUIT" };
static void Title(const std::string& s) { glfwSetWindowTitle(gWindow, ("ZERO BRAKE | " + s).c_str()); }
static void Center(const std::string& text, float y, float size, glm::vec4 c)
{ Text_Draw((1280 - Text_Width(text, size)) * 0.5f, y, text, size, c); }
static void Background()
{
	glClearColor(0.025f, 0.04f, 0.065f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
static void Preview(const Game& g, CarType type, float x, float y, float size)
{
	glViewport((int)((x - size / 2) * gWidth / 1280), (int)((720 - y - size / 2) * gHeight / 720),
		std::max(1, (int)(size * gWidth / 1280)), std::max(1, (int)(size * gHeight / 720)));
	glm::vec3 eye(6, 3.5f, 7);
	Shader_SetMat4("view", glm::lookAt(eye, glm::vec3(0, 0.8f, 0), glm::vec3(0, 1, 0)));
	Shader_SetMat4("projection", glm::perspective(glm::radians(40.0f), (float)gWidth / gHeight * 720 / 1280, 0.1f, 100.0f));
	Lighting_Apply(g, eye);
	CarModel_DrawAt(type, glm::rotate(glm::mat4(1), (float)glfwGetTime() * 0.5f, glm::vec3(0, 1, 0)), 0, 0, 1);
	glViewport(0, 0, gWidth, gHeight);
}
void Screens_UpdateTitle(Game& g)
{
	Title("ENTER to select | Click a circuit card");
	static bool previous = false;
	bool down = glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	if (down && !previous)
	{
		double x, y; int w, h; glfwGetCursorPos(gWindow, &x, &y); glfwGetWindowSize(gWindow, &w, &h);
		x = x * 1280 / std::max(1, w); y = y * 720 / std::max(1, h);
		if (y >= 496 && y <= 586)
			for (int map = 0; map < 2; ++map)
				if (x >= 170 + map * 490 && x <= 620 + map * 490)
				{ g.settings.mapId = map; g.state = GameState::Select; }
	}
	previous = down;
	if (Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER)) g.state = GameState::Select;
}
void Screens_DrawTitle(const Game& g)
{
	Background(); Preview(g, CarType::Speed, 640, 335, 440);
	Hud_Begin();
	Text_Draw(70, 38, "OPENGL ARCADE RACING", 14, CYAN);
	Center("ZERO BRAKE", 88, 66, WHITE);
	Center("THREE CARS. TWO CIRCUITS. ONE FINISH LINE.", 180, 15, MUTED);
	for (int i = 0; i < 2; ++i)
	{
		float x = 170.0f + i * 490;
		Hud_Rect(x, 496, 450, 90, glm::vec4(0.08f, 0.13f, 0.2f, 0.95f));
		Hud_Rect(x, 496, 4, 90, i == 0 ? CYAN : GOLD);
		Text_Draw(x + 22, 516, MAPS[i], 19, WHITE);
		Text_Draw(x + 22, 554, i == 0 ? "WIDE ROADS / FAST STRAIGHTS" : "TIGHT TURNS / CITY STREETS", 12, MUTED);
	}
	Center("ENTER TO SELECT  /  CLICK A CIRCUIT", 625, 18, CYAN);
	Center("W/S DRIVE  A/D STEER  SPACE DRIFT  SHIFT BOOST  Q QUIT", 678, 12, MUTED);
	Hud_End();
}
bool Screens_UpdateSelect(Game& g)
{
	auto& s = g.settings;
	if (Input_Pressed(GLFW_KEY_UP)) g.menuRow = (g.menuRow + 2) % 3;
	if (Input_Pressed(GLFW_KEY_DOWN)) g.menuRow = (g.menuRow + 1) % 3;
	int d = (Input_Pressed(GLFW_KEY_RIGHT) ? 1 : 0) - (Input_Pressed(GLFW_KEY_LEFT) ? 1 : 0);
	if (g.menuRow == 0) s.playerCar = (CarType)(((int)s.playerCar + d + 3) % 3);
	if (g.menuRow == 1) s.mapId = (s.mapId + d + 2) % 2;
	if (g.menuRow == 2) s.aiCount = glm::clamp(s.aiCount + d, 1, 3);
	if (Input_Pressed(GLFW_KEY_ESCAPE)) { g.state = GameState::Title; return false; }
	Title(std::string(CarSpec_Get(s.playerCar).name) + " | " + MAPS[s.mapId]);
	return Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER);
}
void Screens_DrawSelect(const Game& g)
{
	Background(); Preview(g, g.settings.playerCar, 930, 300, 450);
	Hud_Begin();
	Text_Draw(60, 45, "RACE SETUP", 40, WHITE);
	Text_Draw(60, 107, "CHOOSE YOUR CAR AND CIRCUIT", 14, MUTED);
	const char* labels[] = { "CAR", "CIRCUIT", "AI OPPONENTS" };
	const std::string values[] = { CarSpec_Get(g.settings.playerCar).name, MAPS[g.settings.mapId], std::to_string(g.settings.aiCount) };
	for (int i = 0; i < 3; ++i)
	{
		float y = 176.0f + i * 126;
		bool selected = i == g.menuRow;
		Hud_Rect(60, y, 560, 102, selected ? glm::vec4(0.09f, 0.23f, 0.3f, 1) : glm::vec4(0.055f, 0.09f, 0.14f, 1));
		if (selected) Hud_Rect(60, y, 4, 102, CYAN);
		Text_Draw(82, y + 16, labels[i], 13, MUTED);
		Text_Draw(82, y + 49, values[i], i == 1 ? 21.0f : 25.0f, selected ? CYAN : WHITE);
		Text_Draw(565, y + 49, "<>" , 18, MUTED);
	}
	const CarSpec& spec = CarSpec_Get(g.settings.playerCar);
	const char* stats[] = { "TOP SPEED", "ACCELERATION", "HANDLING" };
	float amounts[] = { spec.maxSpeed / 60, spec.accel / 20, spec.steerRate / 140 };
	for (int i = 0; i < 3; ++i)
	{
		float y = 440.0f + i * 36;
		Text_Draw(750, y, stats[i], 12, MUTED);
		Hud_Rect(930, y, 250, 12, glm::vec4(1, 1, 1, 0.1f));
		Hud_Rect(930, y, 250 * amounts[i], 12, CYAN);
	}
	float best = Ghost_BestTime(g.settings.mapId, g.settings.playerCar);
	Text_Draw(750, 565, "PERSONAL BEST", 13, GOLD);
	Text_Draw(750, 593, best > 0 ? Hud_FormatTime(best) : "NO RECORD YET", 22, WHITE);
	Text_Draw(60, 598, "3 LAPS / 5 MINUTE TIME LIMIT", 14, GOLD);
	Text_Draw(60, 668, "UP/DOWN SELECT  LEFT/RIGHT CHANGE  ENTER START  ESC BACK", 14, CYAN);
	Hud_End();
}
void Screens_DrawCountdown(const Game& g)
{
	Hud_Begin();
	Hud_Rect(540, 260, 200, 210, glm::vec4(0.02f, 0.04f, 0.08f, 0.8f));
	SevenSeg_Text(600, 280, std::to_string(std::max(1, (int)std::ceil(g.countdown))), 140, GOLD);
	Center("GET READY", 452, 14, WHITE); Hud_End();
}
void Screens_DrawPause(const Game&)
{
	Hud_Begin(); Hud_Rect(0, 0, 1280, 720, glm::vec4(0, 0, 0, 0.75f));
	Center("PAUSED", 280, 58, WHITE); Center("P RESUME   ESC TITLE   Q QUIT", 388, 18, CYAN); Hud_End();
}
bool Screens_UpdateResult(Game& g)
{
	Title(g.race.timeOver ? "TIME OVER | ENTER to race again" : "FINISH | ENTER to race again");
	return Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER);
}
void Screens_DrawResult(const Game& g)
{
	Hud_Begin(); Hud_Rect(0, 0, 1280, 720, glm::vec4(0.015f, 0.025f, 0.045f, 0.93f));
	Center(g.race.timeOver ? "TIME OVER" : (g.cars[0].rank == 1 ? "VICTORY" : "RACE COMPLETE"), 45, 42, g.race.timeOver ? glm::vec4(1, 0.3f, 0.2f, 1) : GOLD);
	Text_Draw(150, 135, "POS", 14, MUTED); Text_Draw(260, 135, "DRIVER / CAR", 14, MUTED);
	Text_Draw(690, 135, "STATUS", 14, MUTED); Text_Draw(970, 135, "TIME", 14, MUTED);
	for (int rank = 1; rank <= g.carCount; ++rank)
		for (int i = 0; i < g.carCount; ++i)
		{
			const Car& c = g.cars[i]; if (!c.active || c.rank != rank) continue;
			float y = 175.0f + (rank - 1) * 72;
			Hud_Rect(130, y, 1020, 60, c.isPlayer ? glm::vec4(0.08f, 0.22f, 0.29f, 1) : glm::vec4(1, 1, 1, 0.04f));
			Text_Draw(152, y + 20, std::to_string(rank), 22, c.isPlayer ? CYAN : WHITE);
			Text_Draw(260, y + 20, std::string(c.isPlayer ? "YOU / " : "AI / ") + CarSpec_Get(c.type).name, 18, WHITE);
			std::string status = c.finished ? "FINISHED" : "LAP " + std::to_string(std::min(c.lap + 1, g.settings.laps)) + "/" + std::to_string(g.settings.laps);
			if (g.race.timeOver && c.isPlayer) status = "TIME LIMIT";
			Text_Draw(690, y + 20, status, 17, c.finished ? GOLD : MUTED);
			Text_Draw(970, y + 20, c.finished ? Hud_FormatTime(c.finishTime) : "--:--.--", 17, WHITE);
		}
	Text_Draw(150, 505, "TOTAL " + Hud_FormatTime(g.race.time), 19, WHITE);
	Text_Draw(150, 549, "RACE BEST " + (g.race.bestLap > 0 ? Hud_FormatTime(g.race.bestLap) : "--:--.--"), 17, MUTED);
	float best = Ghost_BestTime(g.settings.mapId, g.settings.playerCar);
	Text_Draw(690, 549, "PERSONAL " + (best > 0 ? Hud_FormatTime(best) : "--:--.--"), 17, GOLD);
	Center("ENTER RACE AGAIN   Q QUIT", 660, 19, CYAN); Hud_End();
}
