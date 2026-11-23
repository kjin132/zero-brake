// =====================================================================================
//  [공동·합의 후 수정] 두 사람이 같이 쓰는 데이터 구조
//
//  ※ 충돌이 가장 나기 쉬운 파일입니다.
//     - 수정하기 전에 반드시 pull 하고, 상대에게 알린 뒤 "이 파일만" 단독 커밋 → 바로 push
//     - 필드를 지우거나 이름을 바꾸지 말고 "추가"만 하기
//     - 각 필드 옆 [우석]/[현조] 는 값을 주로 바꾸는 담당자
// =====================================================================================
#pragma once
#include "core/Common.h"

// ---------------- 게임 상태 ----------------
enum class GameState
{
	Title,      // 타이틀 화면
	Select,     // 차량·맵·AI 수 선택
	Countdown,  // 3-2-1
	Race,       // 주행
	Pause,      // 일시정지
	Result      // 결과 화면
};

// ---------------- 차량 ----------------
enum class CarType { Balance = 0, Speed = 1, Corner = 2, Count = 3 };

// 차종별 성능 수치 [우석]  — vehicle/CarSpecs.cpp 에서 값 설정
struct CarSpec
{
	const char* name;
	float accel;        // 가속력 (m/s^2)
	float brake;        // 제동력 (m/s^2)
	float maxSpeed;     // 최고 속도 (m/s)
	float reverseMax;   // 후진 최고 속도 (m/s)
	float steerRate;    // 조향 속도 (도/초)
	float grip;         // 미끄러짐 회복 정도 (0~1)
	glm::vec3 size;     // 충돌 박스 크기 (가로, 높이, 길이)
	glm::vec3 color;    // 기본 색 [현조가 외형과 같이 조정 가능]
};

// 한 프레임의 조작 입력 (플레이어는 키보드, AI는 AIDriver 가 채움)
struct CarInput
{
	float throttle = 0.0f;  // 0~1  (↑)
	float brake = 0.0f;     // 0~1  (↓)
	float steer = 0.0f;     // -1(왼쪽) ~ +1(오른쪽)  (←/→)
	bool handbrake = false; // Space
	bool boost = false;     // Shift
};

struct Car
{
	bool active = false;
	bool isPlayer = false;
	CarType type = CarType::Balance;

	// 주행 상태 [우석: CarPhysics]
	glm::vec3 pos = glm::vec3(0.0f);
	float yaw = 0.0f;          // 진행 방향 (라디안, +z가 0, y축 회전)
	float speed = 0.0f;        // 앞 방향 속도 (m/s, 음수 = 후진)
	float slipAngle = 0.0f;    // 드리프트 시 차체 방향과 진행 방향 차이 (라디안)
	float steerAngle = 0.0f;   // 앞바퀴 조향각 (도) → CarModel 이 그릴 때 사용
	float wheelSpin = 0.0f;    // 바퀴 회전각 (도)   → CarModel 이 그릴 때 사용

	// 특수 기능 [현조: CarAbility]
	bool handbrakeOn = false;
	float boostTime = 0.0f;    // 남은 부스트 시간 (초)
	float padBoostTime = 0.0f; // 패드의 2초 효과: Shift와 분리
	bool boostExhausted = false; // 게이지 소진 후 Shift를 놓아야 재사용
	int boostPadIndex = -1; // 패드에 진입한 순간에만 발동
	bool startedLap = false; // 첫 출발선 통과는 랩 시작, 완주 아님
	float boostGauge = 1.0f;   // 0~1

	// 경기 진행 [우석: RaceRules / AIDriver]
	int lap = 0;               // 완료한 바퀴 수
	int nextCheckpoint = 1;    // 다음에 통과해야 할 체크포인트 번호 (0 = 결승선)
	int rank = 1;
	bool finished = false;
	float finishTime = 0.0f;
	float aiSkill = 1.0f;      // AI 최고 속도 배율
	int aiTarget = 0;          // AI 가 향하는 중심선 점 번호
	glm::vec3 safePos = glm::vec3(0.0f);  // R 복귀 위치
	float safeYaw = 0.0f;
};

// ---------------- 트랙 [현조: world/Track] ----------------
struct Checkpoint
{
	glm::vec3 pos;       // 도로 중심 위치
	glm::vec3 forward;   // 통과해야 하는 방향
	float halfWidth;     // 도로 폭 / 2
};

struct Obstacle
{
	glm::vec3 pos;       // 바닥 중심
	glm::vec3 size;      // 가로, 높이, 길이 (AABB)
};

struct Track
{
	int mapId = 0;                       // 0 = 고속 서킷, 1 = 테크니컬 서킷
	std::vector<glm::vec3> center;       // 도로 중심선 점들 (순환, 진행 방향 순서)
	float roadWidth = 14.0f;
	std::vector<Checkpoint> checkpoints; // [0] = 출발·결승선
	glm::vec3 startPos = glm::vec3(0.0f);
	float startYaw = 0.0f;
	std::vector<glm::vec3> boostPads;    // 부스트 패드 위치 (밟으면 CarAbility_OnBoostPad)
	std::vector<Obstacle> obstacles;     // 라바콘·방호벽 등 (충돌은 [우석] Collision)
};

// ---------------- 경기 설정 / 기록 ----------------
struct RaceSettings
{
	CarType playerCar = CarType::Balance;  // 선택 화면 [현조]
	int mapId = 0;                         // 0, 1
	int aiCount = 3;                       // 1~3
	int laps = 3;
	float timeLimit = 300.0f;              // 제한 시간 (초) [우석]
};

struct RaceInfo
{
	float time = 0.0f;        // 출발 후 경과 시간
	float lapStartTime = 0.0f;
	float lastLap = 0.0f;
	float bestLap = 0.0f;     // 0 = 기록 없음
	bool timeOver = false;
	bool playerFinished = false;
};

constexpr int MAX_CARS = 4;   // 플레이어 1 + AI 최대 3

enum class ViewMode { Chase = 0, FirstPerson = 1 };

// ---------------- 게임 전체 ----------------
struct Game
{
	GameState state = GameState::Title;
	RaceSettings settings;
	Track track;
	Car cars[MAX_CARS];        // [0] = 플레이어
	int carCount = 1;
	RaceInfo race;

	float countdown = 3.0f;    // Countdown 상태 남은 시간
	int menuRow = 0;           // 선택 화면 커서 [현조]
	ViewMode viewMode = ViewMode::Chase;  // C 키 [현조]
	bool showMinimap = true;
	bool quitRequested = false;
};
