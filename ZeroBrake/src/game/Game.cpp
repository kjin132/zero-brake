// =====================================================================================
//  [공동·고정] 게임 전체 흐름
//
//   Title ──Enter──▶ Select ──Enter──▶ Countdown ──3초──▶ Race ──완주/시간초과──▶ Result
//                      ▲                                   │  ▲                    │
//                      │                                   P  P                    │
//                      │                                   ▼  │                    │
//                      │                                   Pause                   │
//                      └──────────────────────Enter────────────────────────────────┘
//
//  각 상태에서 하는 일은 담당자 모듈 함수로 넘긴다. 여기서는 "순서"만 정한다.
// =====================================================================================
#include "game/Game.h"
#include "core/Input.h"
#include "core/Shader.h"

#include "vehicle/CarSpecs.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarAbility.h"
#include "vehicle/CarModel.h"
#include "race/Collision.h"
#include "race/RaceRules.h"
#include "race/AIDriver.h"
#include "race/Ghost.h"
#include "world/Track.h"
#include "world/Scenery.h"
#include "render/Lighting.h"
#include "render/Texture.h"
#include "view/Camera.h"
#include "view/Viewports.h"
#include "ui/Hud.h"
#include "ui/Screens.h"

// ---------------------------------------------------------------- 초기화
void Game_Init(Game& g)
{
	g = Game();
	Texture_Init();
	Lighting_Init();
	Track_Build(g.track, g.settings.mapId);
	Scenery_Build(g.track);
}

// 선택한 설정으로 트랙·차량을 만들고 카운트다운 시작
void Game_StartRace(Game& g)
{
	Track_Build(g.track, g.settings.mapId);
	Scenery_Build(g.track);

	g.carCount = 1 + glm::clamp(g.settings.aiCount, 1, MAX_CARS - 1);
	for (int i = 0; i < MAX_CARS; i++) g.cars[i] = Car();

	for (int i = 0; i < g.carCount; i++)
	{
		Car& c = g.cars[i];
		c.active = true;
		c.isPlayer = (i == 0);
		c.type = c.isPlayer ? g.settings.playerCar : (CarType)((i - 1) % (int)CarType::Count);
		CarPhysics_Reset(c, g.track, i);     // 출발 그리드 배치 [우석]
		if (!c.isPlayer) AIDriver_Init(g, c, i);
	}

	g.race = RaceInfo();
	RaceRules_Start(g);
	Ghost_StartRace(g);
	Camera_Reset(g);
	g.countdown = 3.0f;
	g.state = GameState::Countdown;
}

// 플레이어 키 입력 → CarInput
static CarInput ReadPlayerInput()
{
	CarInput in;
	in.throttle = Input_Down(GLFW_KEY_UP) ? 1.0f : 0.0f;
	in.brake = Input_Down(GLFW_KEY_DOWN) ? 1.0f : 0.0f;
	in.steer = (Input_Down(GLFW_KEY_RIGHT) ? 1.0f : 0.0f) - (Input_Down(GLFW_KEY_LEFT) ? 1.0f : 0.0f);
	in.handbrake = Input_Down(GLFW_KEY_SPACE);
	in.boost = Input_Down(GLFW_KEY_LEFT_SHIFT) || Input_Down(GLFW_KEY_RIGHT_SHIFT);
	return in;
}

// 주행 한 프레임 (Race 상태)
static void UpdateRace(Game& g, float dt)
{
	const int steps = std::max(1, (int)std::ceil(dt * 120.0f));
	const float step = dt / steps;
	for (int sub = 0; sub < steps; ++sub)
	{
	// 1) 차량 이동
	for (int i = 0; i < g.carCount; i++)
	{
		Car& c = g.cars[i];
		if (!c.active) continue;
		CarInput in;
		if (c.finished) in.brake = 1.0f;                 // 완주한 차는 멈춤
		else if (c.isPlayer) in = ReadPlayerInput();
		else in = AIDriver_Think(g, c, step);              // [우석]
		CarAbility_Update(c, in, step);                    // [현조] 핸드브레이크·부스트
		CarPhysics_Update(c, in, step);                    // [우석] 가속·조향·바퀴 회전
	}
	// 2) 충돌 보정 [우석]
	Collision_Update(g);
	// 3) 체크포인트·랩·순위·제한 시간 [우석]
	RaceRules_Update(g, step);
	// 4) 고스트 기록·재생 [우석]
	Ghost_Update(g, step);
		if (RaceRules_IsOver(g)) break;
	}
	// 5) 카메라 [현조]
	Camera_Update(g, dt);

	// 6) 키 입력 (공통 단축키)
	if (Input_Pressed(GLFW_KEY_R)) { RaceRules_Respawn(g, g.cars[0]); Camera_Reset(g); }
	if (Input_Pressed(GLFW_KEY_C)) g.viewMode = (g.viewMode == ViewMode::Chase) ? ViewMode::FirstPerson : ViewMode::Chase;
	if (Input_Pressed(GLFW_KEY_M)) g.showMinimap = !g.showMinimap;
	if (Input_Pressed(GLFW_KEY_P)) g.state = GameState::Pause;

	if (RaceRules_IsOver(g)) g.state = GameState::Result;
}

// ---------------------------------------------------------------- 매 프레임 갱신
void Game_Update(Game& g, float dt)
{
	if (Input_Pressed(GLFW_KEY_Q)) { g.quitRequested = true; return; }

	switch (g.state)
	{
	case GameState::Title:
		Screens_UpdateTitle(g);                          // [현조] Enter → Select
		break;
	case GameState::Select:
		if (Screens_UpdateSelect(g)) Game_StartRace(g);  // [현조] Enter 누르면 true
		break;
	case GameState::Countdown:
		g.countdown -= dt;
		Camera_Update(g, dt);
		if (g.countdown <= 0.0f) g.state = GameState::Race;
		break;
	case GameState::Race:
		UpdateRace(g, dt);
		break;
	case GameState::Pause:
		if (Input_Pressed(GLFW_KEY_P)) g.state = GameState::Race;
		if (Input_Pressed(GLFW_KEY_ESCAPE)) g.state = GameState::Title;
		break;
	case GameState::Result:
		if (Screens_UpdateResult(g)) g.state = GameState::Select;  // [현조] Enter → 다시 선택
		break;
	}
}

// ---------------------------------------------------------------- 3D 세계 그리기
void World_Draw(const Game& g, const CameraView& cam, bool drawPlayer)
{
	Shader_SetMat4("view", cam.view);
	Shader_SetMat4("projection", cam.proj);
	Lighting_Apply(g, cam.eye);                          // [우석]

	Scenery_Draw();                                      // [현조]
	Track_Draw(g.track);                                 // [현조]
	for (int i = 0; i < g.carCount; i++)
		if (g.cars[i].active && (drawPlayer || i != 0)) CarModel_Draw(g.cars[i]);  // [현조] 외형, 바퀴 각도는 [우석] 값 사용
	Ghost_Draw(g);                                       // 반투명은 맨 마지막 [우석·현조]
}

// ---------------------------------------------------------------- 매 프레임 그리기
void Game_Draw(Game& g)
{
	glViewport(0, 0, gWidth, gHeight);
	glClearColor(0.55f, 0.75f, 0.95f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(Shader_Current());

	switch (g.state)
	{
	case GameState::Title:
		Screens_DrawTitle(g);
		break;
	case GameState::Select:
		Screens_DrawSelect(g);
		break;
	case GameState::Countdown:
		Viewports_Draw(g);                               // [현조] 메인·미러·미니맵
		Hud_DrawRace(g);
		Screens_DrawCountdown(g);
		break;
	case GameState::Race:
	case GameState::Pause:
		Viewports_Draw(g);
		Hud_DrawRace(g);                                 // [현조]
		if (g.state == GameState::Pause) Screens_DrawPause(g);
		break;
	case GameState::Result:
		Viewports_Draw(g);
		Screens_DrawResult(g);
		break;
	}
}
