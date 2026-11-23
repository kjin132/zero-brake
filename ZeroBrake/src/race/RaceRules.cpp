// =====================================================================================
//  [담당: 진우석] 경기 규칙
//  - 체크포인트는 순서대로 + 정해진 방향으로 지나가야 인정 (역주행·지름길 방지)
//  - checkpoints[0] = 출발·결승선. 0번을 지나면 1바퀴 완료
//  - 순위 = (완료 바퀴, 통과한 체크포인트 수, 다음 체크포인트까지 거리) 순
//  TODO(우석): 제한 시간 값, 완주 후 처리, 복귀 위치 안전 검사
// =====================================================================================
#include "race/RaceRules.h"
#include "race/Ghost.h"
#include "vehicle/CarAbility.h"

static float sPrevSide[MAX_CARS];   // 직전 프레임에 다음 체크포인트 평면의 어느 쪽이었는지

static float SideOf(const Checkpoint& cp, glm::vec3 p) { return glm::dot(p - cp.pos, cp.forward); }

static bool IsWithinGate(const Checkpoint& cp, glm::vec3 p)
{
	glm::vec3 right = glm::normalize(glm::cross(cp.forward, glm::vec3(0, 1, 0)));
	return std::fabs(glm::dot(p - cp.pos, right)) <= cp.halfWidth + 1.0f;
}

// 순위 계산용 진행도
static float Progress(const Track& t, const Car& c)
{
	int n = (int)t.checkpoints.size();
	int passed = (c.nextCheckpoint - 1 + n) % n;        // 이번 바퀴에 지난 체크포인트 수
	if (c.nextCheckpoint == 0) passed = n - 1;
	float distToNext = glm::length(t.checkpoints[c.nextCheckpoint].pos - c.pos);
	return c.lap * 1000.0f + passed * 10.0f - distToNext * 0.001f;
}

void RaceRules_Start(Game& g)
{
	for (int i = 0; i < g.carCount; i++)
	{
		g.cars[i].lap = 0;
		g.cars[i].nextCheckpoint = (int)g.track.checkpoints.size() > 1 ? 1 : 0;
		g.cars[i].finished = false;
		sPrevSide[i] = SideOf(g.track.checkpoints[g.cars[i].nextCheckpoint], g.cars[i].pos);
	}
	g.race = RaceInfo();
}

static void OnCheckpoint(Game& g, int carIndex)
{
	Car& c = g.cars[carIndex];
	const Track& t = g.track;
	int n = (int)t.checkpoints.size();
	int passedIndex = c.nextCheckpoint;

	c.safePos = t.checkpoints[passedIndex].pos;
	c.safeYaw = std::atan2(t.checkpoints[passedIndex].forward.x, t.checkpoints[passedIndex].forward.z);

	if (passedIndex == 0)     // 결승선 → 1바퀴 완료
	{
		c.lap++;
		if (c.isPlayer)
		{
			float lapTime = g.race.time - g.race.lapStartTime;
			g.race.lastLap = lapTime;
			if (g.race.bestLap == 0.0f || lapTime < g.race.bestLap) g.race.bestLap = lapTime;
			g.race.lapStartTime = g.race.time;
			Ghost_OnPlayerLap(g, lapTime);
		}
		if (c.lap >= g.settings.laps)
		{
			c.finished = true;
			c.finishTime = g.race.time;
			if (c.isPlayer) g.race.playerFinished = true;
		}
	}
	c.nextCheckpoint = (passedIndex + 1) % n;
	sPrevSide[carIndex] = SideOf(t.checkpoints[c.nextCheckpoint], c.pos);
}

void RaceRules_Update(Game& g, float dt)
{
	g.race.time += dt;
	if (g.race.time >= g.settings.timeLimit) g.race.timeOver = true;

	const Track& t = g.track;
	for (int i = 0; i < g.carCount; i++)
	{
		Car& c = g.cars[i];
		if (!c.active || c.finished) continue;

		// 체크포인트 통과: 평면의 뒤(-)에서 앞(+)으로 넘어가고, 도로 폭 안일 때
		const Checkpoint& cp = t.checkpoints[c.nextCheckpoint];
		float side = SideOf(cp, c.pos);
		if (sPrevSide[i] < 0.0f && side >= 0.0f && IsWithinGate(cp, c.pos))
			OnCheckpoint(g, i);
		else
			sPrevSide[i] = side;

		// 부스트 패드
		for (const glm::vec3& pad : t.boostPads)
			if (glm::length(glm::vec2(c.pos.x - pad.x, c.pos.z - pad.z)) < 2.5f)
				CarAbility_OnBoostPad(c);
	}

	// 순위
	for (int i = 0; i < g.carCount; i++)
	{
		int rank = 1;
		float pi = g.cars[i].finished ? 1e6f - g.cars[i].finishTime : Progress(t, g.cars[i]);
		for (int j = 0; j < g.carCount; j++)
		{
			if (i == j) continue;
			float pj = g.cars[j].finished ? 1e6f - g.cars[j].finishTime : Progress(t, g.cars[j]);
			if (pj > pi) rank++;
		}
		g.cars[i].rank = rank;
	}
}

void RaceRules_Respawn(Game& g, Car& car)
{
	car.pos = car.safePos;
	car.yaw = car.safeYaw;
	car.speed = 0.0f;
	car.slipAngle = 0.0f;
	for (int i = 0; i < g.carCount; i++)
		if (&g.cars[i] == &car)
			sPrevSide[i] = SideOf(g.track.checkpoints[car.nextCheckpoint], car.pos);
}

bool RaceRules_IsOver(const Game& g)
{
	return g.race.playerFinished || g.race.timeOver;
}
