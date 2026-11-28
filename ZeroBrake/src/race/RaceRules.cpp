// 순서/방향/게이트 교차점을 검사하며 출발선 첫 통과와 랩 완주를 구분한다.
#include "race/RaceRules.h"
#include "race/Ghost.h"
#include "vehicle/CarAbility.h"
#include "vehicle/CarSpecs.h"
#include "world/Track.h"

static glm::vec3 sPrevious[MAX_CARS];
static float SideOf(const Checkpoint& cp, glm::vec3 p) { return glm::dot(p - cp.pos, cp.forward); }
static bool Crossed(const Checkpoint& cp, glm::vec3 from, glm::vec3 to)
{
	float a = SideOf(cp, from), b = SideOf(cp, to);
	if (a >= 0.0f || b < 0.0f) return false;
	glm::vec3 crossing = glm::mix(from, to, -a / (b - a));
	glm::vec3 side(-cp.forward.z, 0, cp.forward.x);
	return std::fabs(glm::dot(crossing - cp.pos, side)) <= cp.halfWidth;
}
static float Progress(const Track& t, const Car& c)
{
	if (!c.startedLap) return -glm::length(c.pos - t.startPos) * 0.001f;
	int n = (int)t.checkpoints.size();
	int passed = c.nextCheckpoint == 0 ? n - 1 : c.nextCheckpoint - 1;
	return c.lap * 1000.0f + passed * 10.0f - glm::length(t.checkpoints[c.nextCheckpoint].pos - c.pos) * 0.001f;
}
void RaceRules_Start(Game& g)
{
	for (int i = 0; i < g.carCount; ++i)
	{
		Car& c = g.cars[i];
		c.lap = 0; c.nextCheckpoint = 0; c.finished = false; c.startedLap = false;
		c.finishTime = 0; c.boostPadIndex = -1;
		sPrevious[i] = c.pos;
	}
	g.race = RaceInfo();
}
static void OnCheckpoint(Game& g, int index)
{
	Car& c = g.cars[index];
	int passed = c.nextCheckpoint;
	const Checkpoint& cp = g.track.checkpoints[passed];
	c.safePos = cp.pos + cp.forward * 2.5f;
	c.safeYaw = std::atan2(cp.forward.x, cp.forward.z);
	if (passed == 0)
	{
		if (!c.startedLap)
		{
			c.startedLap = true;
			if (c.isPlayer) { g.race.lapStartTime = g.race.time; Ghost_BeginLap(g); }
		}
		else
		{
			++c.lap;
			if (c.isPlayer)
			{
				float time = g.race.time - g.race.lapStartTime;
				g.race.lastLap = time;
				if (g.race.bestLap == 0 || time < g.race.bestLap) g.race.bestLap = time;
				Ghost_OnPlayerLap(g, time);
				g.race.lapStartTime = g.race.time;
			}
			if (c.lap >= g.settings.laps)
			{
				c.finished = true; c.finishTime = g.race.time;
				if (c.isPlayer) g.race.playerFinished = true;
			}
		}
	}
	c.nextCheckpoint = (passed + 1) % (int)g.track.checkpoints.size();
}
void RaceRules_Update(Game& g, float dt)
{
	g.race.time += dt;
	const Track& t = g.track;
	if (t.checkpoints.empty()) return;
	for (int i = 0; i < g.carCount; ++i)
	{
		Car& c = g.cars[i];
		if (!c.active || c.finished) continue;
		if (Crossed(t.checkpoints[c.nextCheckpoint], sPrevious[i], c.pos)) OnCheckpoint(g, i);
		sPrevious[i] = c.pos;
		int padIndex = -1;
		for (int p = 0; p < (int)t.boostPads.size(); ++p)
		{
			glm::vec3 d = c.pos - t.boostPads[p]; d.y = 0;
			if (glm::length(d) < 2.5f) { padIndex = p; break; }
		}
		if (padIndex >= 0 && padIndex != c.boostPadIndex && c.speed > 0.0f) CarAbility_OnBoostPad(c);
		c.boostPadIndex = padIndex;
	}
	for (int i = 0; i < g.carCount; ++i)
	{
		Car& a = g.cars[i]; if (!a.active) continue;
		a.rank = 1;
		float pa = a.finished ? 1e6f - a.finishTime : Progress(t, a);
		for (int j = 0; j < g.carCount; ++j)
		{
			const Car& b = g.cars[j]; if (i == j || !b.active) continue;
			float pb = b.finished ? 1e6f - b.finishTime : Progress(t, b);
			if (pb > pa || (pb == pa && j < i)) ++a.rank;
		}
	}
	// 제한 시각과 같은 스텝에 결승선을 통과해도 제한 초과를 성공으로 표시하지 않는다.
	if (g.race.time > g.settings.timeLimit) { g.race.timeOver = true; g.race.playerFinished = false; }
}
void RaceRules_Respawn(Game& g, Car& c)
{
	glm::vec3 fwd(std::sin(c.safeYaw), 0, std::cos(c.safeYaw));
	glm::vec3 side(-fwd.z, 0, fwd.x);
	glm::vec3 candidate = c.safePos;
	bool found = false;
	// 복귀 지점을 다른 차/장애물과 겹치지 않는 도로 위에서 찾는다.
	for (int row = 0; row < 6 && !found; ++row)
		for (int lane = 0; lane < 3 && !found; ++lane)
		{
			float lateral = lane == 0 ? 0.0f : (lane == 1 ? -2.5f : 2.5f);
			glm::vec3 p = c.safePos + fwd * (row * 5.5f) + side * lateral;
			// 복귀 검색이 다음 체크포인트를 건너뛰지 않도록 한다.
			if (!g.track.checkpoints.empty() && SideOf(g.track.checkpoints[c.nextCheckpoint], p) >= -0.5f) continue;
			if (Track_DistanceFromCenter(g.track, p, nullptr, nullptr) > g.track.roadWidth * 0.5f - 2.5f) continue;
			bool blocked = false;
			for (int i = 0; i < g.carCount; ++i)
				if (&g.cars[i] != &c && g.cars[i].active && glm::length(g.cars[i].pos - p) < 5.2f) blocked = true;
			for (const Obstacle& o : g.track.obstacles)
				if (std::fabs(p.x - o.pos.x) < o.size.x * 0.5f + 2.5f &&
					std::fabs(p.z - o.pos.z) < o.size.z * 0.5f + 2.5f) blocked = true;
			if (!blocked) { candidate = p; found = true; }
		}
	if (!found) return; // 빈자리가 없으면 기존 위치를 유지한다.
	c.pos = candidate; c.yaw = c.safeYaw; c.speed = 0; c.slipAngle = 0; c.steerAngle = 0; c.bump = glm::vec3(0.0f);
	c.handbrakeOn = false; c.boostTime = 0; c.padBoostTime = 0;
	for (int i = 0; i < g.carCount; ++i) if (&g.cars[i] == &c) sPrevious[i] = c.pos;
	if (c.isPlayer) Ghost_InvalidateLap();
}
bool RaceRules_IsOver(const Game& g) { return g.race.playerFinished || g.race.timeOver; }
