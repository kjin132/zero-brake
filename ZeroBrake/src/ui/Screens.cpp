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
static const char* MAPS[] = { "고속 서킷", "테크니컬 서킷" };
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
	Title("Enter로 선택 | 서킷 카드를 클릭하세요");
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
	Text_Draw(70, 38, "3D 아케이드 레이싱", 18, CYAN);
	Center("ZERO BRAKE", 88, 66, WHITE);
	Center("차량 3종, 서킷 2개. 가장 먼저 결승선을 통과하세요.", 180, 19, MUTED);
	for (int i = 0; i < 2; ++i)
	{
		float x = 170.0f + i * 490;
		Hud_Rect(x, 496, 450, 90, glm::vec4(0.08f, 0.13f, 0.2f, 0.95f));
		Hud_Rect(x, 496, 4, 90, i == 0 ? CYAN : GOLD);
		Text_Draw(x + 22, 516, MAPS[i], 19, WHITE);
		Text_Draw(x + 22, 554, i == 0 ? "넓은 도로 / 고속 직선 구간" : "연속 코너 / 도심 코스", 17, MUTED);
	}
	Center("Enter로 시작  /  서킷 카드를 클릭해 선택", 625, 18, CYAN);
	Center("↑ 가속  ↓ 제동·후진  ← → 조향  Space 드리프트  Shift 부스트  Q 종료", 678, 17, MUTED);
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
	Text_Draw(60, 45, "경기 설정", 40, WHITE);
	Text_Draw(60, 107, "차량과 서킷, 상대 차량 수를 선택하세요", 18, MUTED);
	const char* labels[] = { "차량", "서킷", "AI 상대 수" };
	const std::string values[] = { CarSpec_Get(g.settings.playerCar).name, MAPS[g.settings.mapId], std::to_string(g.settings.aiCount) };
	for (int i = 0; i < 3; ++i)
	{
		float y = 176.0f + i * 126;
		bool selected = i == g.menuRow;
		Hud_Rect(60, y, 560, 102, selected ? glm::vec4(0.09f, 0.23f, 0.3f, 1) : glm::vec4(0.055f, 0.09f, 0.14f, 1));
		if (selected) Hud_Rect(60, y, 4, 102, CYAN);
		Text_Draw(82, y + 16, labels[i], 17, MUTED);
		Text_Draw(82, y + 49, values[i], i == 1 ? 21.0f : 25.0f, selected ? CYAN : WHITE);
		Text_Draw(565, y + 49, "<>" , 18, MUTED);
	}
	const CarSpec& spec = CarSpec_Get(g.settings.playerCar);
	const char* stats[] = { "최고 속도", "가속력", "조향 성능" };
	float amounts[] = { spec.maxSpeed / 60, spec.accel / 20, spec.steerRate / 140 };
	for (int i = 0; i < 3; ++i)
	{
		float y = 440.0f + i * 36;
		Text_Draw(750, y, stats[i], 17, MUTED);
		Hud_Rect(930, y, 250, 17, glm::vec4(1, 1, 1, 0.1f));
		Hud_Rect(930, y, 250 * amounts[i], 17, CYAN);
	}
	float best = Ghost_BestTime(g.settings.mapId, g.settings.playerCar);
	Text_Draw(750, 565, "개인 최고 기록", 17, GOLD);
	Text_Draw(750, 593, best > 0 ? Hud_FormatTime(best) : "아직 기록이 없습니다", 22, WHITE);
	Text_Draw(60, 598, "총 3바퀴 / 제한 시간 5분", 18, GOLD);
	Text_Draw(60, 668, "↑ ↓ 항목 선택  ← → 설정 변경  Enter 경기 시작  Esc 뒤로", 18, CYAN);
	Hud_End();
}
void Screens_DrawCountdown(const Game& g)
{
	Hud_Begin();
	Hud_Rect(540, 260, 200, 210, glm::vec4(0.02f, 0.04f, 0.08f, 0.8f));
	SevenSeg_Text(600, 280, std::to_string(std::max(1, (int)std::ceil(g.countdown))), 140, GOLD);
	Center("출발 준비", 452, 18, WHITE); Hud_End();
}
void Screens_DrawPause(const Game&)
{
	Hud_Begin(); Hud_Rect(0, 0, 1280, 720, glm::vec4(0, 0, 0, 0.75f));
	Center("일시정지", 280, 58, WHITE); Center("P 계속하기   Esc 처음으로   Q 종료", 388, 18, CYAN); Hud_End();
}
bool Screens_UpdateResult(Game& g)
{
	Title(g.race.timeOver ? "시간 초과 | Enter로 다시 시작" : "경기 종료 | Enter로 다시 시작");
	return Input_Pressed(GLFW_KEY_ENTER) || Input_Pressed(GLFW_KEY_KP_ENTER);
}
void Screens_DrawResult(const Game& g)
{
	Hud_Begin(); Hud_Rect(0, 0, 1280, 720, glm::vec4(0.015f, 0.025f, 0.045f, 0.93f));
	Center(g.race.timeOver ? "시간 초과" : (g.cars[0].rank == 1 ? "우승!" : "경기 종료"), 45, 42, g.race.timeOver ? glm::vec4(1, 0.3f, 0.2f, 1) : GOLD);
	Text_Draw(150, 135, "순위", 18, MUTED); Text_Draw(260, 135, "참가자 / 차량", 18, MUTED);
	Text_Draw(690, 135, "상태", 18, MUTED); Text_Draw(970, 135, "기록", 18, MUTED);
	for (int rank = 1; rank <= g.carCount; ++rank)
		for (int i = 0; i < g.carCount; ++i)
		{
			const Car& c = g.cars[i]; if (!c.active || c.rank != rank) continue;
			float y = 175.0f + (rank - 1) * 72;
			Hud_Rect(130, y, 1020, 60, c.isPlayer ? glm::vec4(0.08f, 0.22f, 0.29f, 1) : glm::vec4(1, 1, 1, 0.04f));
			Text_Draw(152, y + 20, std::to_string(rank), 22, c.isPlayer ? CYAN : WHITE);
			Text_Draw(260, y + 20, std::string(c.isPlayer ? "플레이어 / " : "AI / ") + CarSpec_Get(c.type).name, 18, WHITE);
			std::string status = c.finished ? "완주" : "바퀴 " + std::to_string(std::min(c.lap + 1, g.settings.laps)) + "/" + std::to_string(g.settings.laps);
			if (g.race.timeOver && c.isPlayer) status = "시간 초과";
			Text_Draw(690, y + 20, status, 17, c.finished ? GOLD : MUTED);
			Text_Draw(970, y + 20, c.finished ? Hud_FormatTime(c.finishTime) : "--:--.--", 17, WHITE);
		}
	Text_Draw(150, 505, "총 시간 " + Hud_FormatTime(g.race.time), 19, WHITE);
	Text_Draw(150, 549, "이번 경기 최고 " + (g.race.bestLap > 0 ? Hud_FormatTime(g.race.bestLap) : "--:--.--"), 17, MUTED);
	float best = Ghost_BestTime(g.settings.mapId, g.settings.playerCar);
	Text_Draw(690, 549, "개인 최고 " + (best > 0 ? Hud_FormatTime(best) : "--:--.--"), 17, GOLD);
	Center("Enter 다시 시작   Q 종료", 660, 19, CYAN); Hud_End();
}
