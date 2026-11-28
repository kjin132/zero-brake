// =====================================================================================
//  AI 경쟁차 주행
//
//  1) 레이싱 라인: 트랙마다 한 번, 중심선을 도로 폭 안에서 반복해서 펴서(라플라시안 스무딩)
//     "바깥 → 안쪽(에이펙스) → 바깥"으로 도는 최소 곡률 경로를 만든다.
//  2) 속도 계획: 이 게임 물리에서 차가 돌 수 있는 한계는 조향 속도(steerRate)다.
//     반지름 R 커브의 최고 속도 ≈ 회전각속도 × R  → 차종마다 코너 속도가 다름.
//     앞쪽 커브까지의 제동 거리를 계산해서 미리 감속한다.
//  3) 조향: 속도에 비례해 앞을 보는 지점(퓨어 퍼슛)을 향해 필요한 회전량만큼 핸들을 꺾는다.
//  4) 주변 차·장애물: 앞에 느린 차가 있으면 공간이 넓은 쪽으로 추월, 자리가 없으면 거리 유지.
//  5) 막혔을 때: 거의 멈춘 채 1초가 지나면 후진하면서 방향을 돌려 빠져나온다.
//  6) 부스트: 앞이 긴 직선이고 앞차가 없으면 Shift 부스트 사용.
//
//  난이도 조절: 아래 kSkill (차 순서별 실력) 과 DIFFICULTY 숫자를 바꾸면 됨
// =====================================================================================
#include "race/AIDriver.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "world/Track.h"
#include <unordered_map>

namespace
{
// ---- 난이도 ----
const float DIFFICULTY = 1.0f;                                // 전체 AI 실력 배율 (0.9 = 쉬움, 1.0 = 보통)
const float kSkill[MAX_CARS] = { 1.0f, 0.97f, 0.94f, 0.91f };  // AI 1·2·3번째 차 실력 (속도·코너링 배율)

// ---- 주행 상수 ----
const float CORNER_MARGIN = 0.82f;   // 이론상 코너 한계 속도의 몇 %로 돌지 (작을수록 안전)
const float BRAKE_USE = 0.8f;        // 제동력의 몇 %를 계획에 쓸지
const float LOOK_AHEAD_MIN = 6.0f, LOOK_AHEAD_MAX = 20.0f;
const float CAR_GAP = 7.0f;          // 앞차와 유지할 거리 (m)
const float LAT_GAIN = 0.12f;        // 옆 위치 오차 1m 당 추가 조향량

// ---- 레이싱 라인 (트랙마다 1개, 모든 AI 공유) ----
struct RacingLine
{
	int mapId = -1;
	size_t count = 0;
	glm::vec3 first = glm::vec3(0.0f);
	std::vector<glm::vec3> pts;      // 레이싱 라인 점 (중심선 점과 1:1)
	std::vector<glm::vec3> right;    // 각 중심선 점의 오른쪽 방향
	std::vector<float> offset;       // 중심선에서 레이싱 라인까지 옆 거리 (오른쪽 +)
	std::vector<float> radius;       // 레이싱 라인 곡률 반지름 (m)
	std::vector<float> spacing;      // 다음 점까지 거리
};
RacingLine sLine;

// ---- 차마다 기억하는 상태 ----
struct AIState
{
	int index = 0;             // 마지막으로 찾은 중심선 점 번호
	float bias = 0.0f;         // 차마다 조금씩 다른 주행선
	float stuckTimer = 0.0f;
	float reverseTimer = 0.0f;
	bool boosting = false;
};
std::unordered_map<const Car*, AIState> sState;

glm::vec3 TangentAt(const std::vector<glm::vec3>& p, int i)
{
	int n = (int)p.size();
	glm::vec3 d = p[(i + 1) % n] - p[(i - 1 + n) % n];
	d.y = 0.0f;
	return glm::normalize(d);
}

float WrapAngle(float a) { return std::remainder(a, 2.0f * PI); }

void BuildLine(const Track& t)
{
	const auto& c = t.center;
	int n = (int)c.size();
	sLine = RacingLine();
	sLine.mapId = t.mapId; sLine.count = c.size(); sLine.first = c[0];
	sLine.right.resize(n); sLine.offset.assign(n, 0.0f); sLine.pts = c;
	for (int i = 0; i < n; i++) { glm::vec3 tg = TangentAt(c, i); sLine.right[i] = glm::vec3(-tg.z, 0, tg.x); }

	// 반복 스무딩: 각 점을 양옆 점의 중간으로 당기되, 도로 폭 안(가장자리 여유 2.4m)에서만
	const float limit = t.roadWidth * 0.5f - 2.4f;
	for (int iter = 0; iter < 400; iter++)
		for (int i = 0; i < n; i++)
		{
			glm::vec3 mid = (sLine.pts[(i - 1 + n) % n] + sLine.pts[(i + 1) % n]) * 0.5f;
			float want = glm::dot(mid - c[i], sLine.right[i]);
			sLine.offset[i] = glm::clamp(glm::mix(sLine.offset[i], want, 0.6f), -limit, limit);
			sLine.pts[i] = c[i] + sLine.right[i] * sLine.offset[i];
		}

	// 곡률 반지름: 앞뒤 약 6m 떨어진 점 3개로 원을 그려 계산
	sLine.spacing.resize(n); sLine.radius.resize(n);
	float avg = 0.0f;
	for (int i = 0; i < n; i++) { sLine.spacing[i] = glm::length(sLine.pts[(i + 1) % n] - sLine.pts[i]); avg += sLine.spacing[i]; }
	avg /= n;
	int k = std::max(1, (int)std::round(6.0f / std::max(0.1f, avg)));
	for (int i = 0; i < n; i++)
	{
		glm::vec3 a = sLine.pts[(i - k + n) % n], b = sLine.pts[i], d = sLine.pts[(i + k) % n];
		float ab = glm::length(b - a), bd = glm::length(d - b), ad = glm::length(d - a);
		glm::vec3 cr = glm::cross(b - a, d - a);
		float area2 = std::fabs(cr.y);                       // 삼각형 넓이 × 2
		sLine.radius[i] = area2 < 1e-4f ? 1e4f : (ab * bd * ad) / (2.0f * area2);
	}
	// 반지름은 주변 최솟값으로 (짧은 급커브를 놓치지 않게)
	std::vector<float> r = sLine.radius;
	for (int i = 0; i < n; i++)
		for (int j = -k; j <= k; j++) sLine.radius[i] = std::min(sLine.radius[i], r[(i + j + n) % n]);
}

void EnsureLine(const Track& t)
{
	if (t.center.size() < 3) return;
	if (sLine.mapId != t.mapId || sLine.count != t.center.size() || sLine.first != t.center[0]) BuildLine(t);
}

// 지난번 위치 근처에서만 가장 가까운 점을 찾음 (트랙이 가까이 지나가는 곳에서 엉뚱한 구간을 잡지 않게)
int LocalNearest(const Track& t, glm::vec3 p, int hint)
{
	int n = (int)t.center.size();
	int best = hint; float bestD = 1e30f;
	for (int k = -15; k <= 60; k++)
	{
		int i = ((hint + k) % n + n) % n;
		glm::vec3 d = t.center[i] - p; float dd = d.x * d.x + d.z * d.z;
		if (dd < bestD) { bestD = dd; best = i; }
	}
	if (bestD > t.roadWidth * t.roadWidth * 2.0f) best = Track_NearestIndex(t, p);   // 너무 멀면 전체 검색 (복귀 직후 등)
	return best;
}

float Lateral(const Track& t, int i, glm::vec3 p) { return glm::dot(p - t.center[i], sLine.right[i]); }

// 앞쪽으로 dist(m) 만큼 떨어진 점 번호
int Advance(int i, float dist, float* travelled = nullptr)
{
	int n = (int)sLine.pts.size(); float d = 0.0f; int j = i;
	while (d < dist) { d += sLine.spacing[j]; j = (j + 1) % n; if (j == i) break; }
	if (travelled) *travelled = d;
	return j;
}
}

void AIDriver_Init(Game& g, Car& car, int index)
{
	EnsureLine(g.track);
	car.aiSkill = kSkill[glm::clamp(index, 0, MAX_CARS - 1)] * DIFFICULTY;
	car.aiTarget = Track_NearestIndex(g.track, car.pos);
	AIState s;
	s.index = car.aiTarget;
	s.bias = (index % 2 == 0 ? 0.5f : -0.5f) * (1.0f - car.aiSkill) * 6.0f;   // 실력이 낮을수록 라인이 조금 어긋남
	sState[&car] = s;
}

CarInput AIDriver_Think(const Game& g, Car& car, float dt)
{
	CarInput in;
	const Track& t = g.track;
	int n = (int)t.center.size();
	if (n < 3) return in;
	EnsureLine(t);
	AIState& st = sState[&car];
	const CarSpec& spec = CarSpec_Get(car.type);
	const float omega = glm::radians(spec.steerRate);            // 최대 회전 각속도 (rad/s)
	const float halfRoad = t.roadWidth * 0.5f - spec.size.x * 0.5f - 1.0f;   // 벽과 1m 여유

	int idx = LocalNearest(t, car.pos, st.index);
	st.index = idx; car.aiTarget = idx;
	float speed = car.speed;
	glm::vec3 fwd = Car_Forward(car);

	// ---------------- 주변 차·장애물 찾기 ----------------
	// "막는 물체" = 앞 38m 안에서 내 레이싱 라인과 겹치는 차·장애물.
	// 그 물체 근처 구간에서만 옆으로 비켜 지나가고(추월), 비킬 자리가 없으면 속도를 맞춰 따라감
	struct Blocker { float ahead; float lo, hi; float passLat; bool canPass; };
	std::vector<Blocker> blockers;
	float followSpeed = 1e9f, nearestAhead = 1e9f;
	glm::vec3 tangentHere = TangentAt(t.center, idx);
	float myLat = Lateral(t, idx, car.pos);
	auto consider = [&](glm::vec3 pos, float otherSpeed, float width) {
		glm::vec3 rel = pos - car.pos; rel.y = 0.0f;
		float ahead = glm::dot(rel, tangentHere);
		if (ahead < 1.0f || ahead > 38.0f) return;
		if (speed - otherSpeed < -1.0f) return;                    // 앞차가 더 빠르면 신경 안 씀
		otherSpeed = std::max(0.0f, otherSpeed);                   // 후진 중인 차를 따라 후진하지 않게
		int oi = LocalNearest(t, pos, idx);
		float otherLat = Lateral(t, oi, pos);
		float need = width * 0.5f + spec.size.x * 0.5f + 1.3f;    // 중심끼리 이만큼 떨어져야 안 부딪힘 (추종 오차 여유 포함)
		float lineLat = sLine.offset[oi] + st.bias;
		bool onLine = std::fabs(otherLat - lineLat) <= need;       // 계획한 라인과 겹침
		bool onPath = std::fabs(otherLat - myLat) <= need;         // 지금 달리는 위치 그대로 가도 겹침
		if (!onLine && !onPath) return;
		nearestAhead = std::min(nearestAhead, ahead);
		Blocker b{ ahead, otherLat - need, otherLat + need, 0.0f, true };
		bool leftOk = b.lo > -halfRoad, rightOk = b.hi < halfRoad;
		if (leftOk && rightOk) b.passLat = std::fabs(b.lo - lineLat) < std::fabs(b.hi - lineLat) ? b.lo : b.hi;
		else if (leftOk) b.passLat = b.lo;
		else if (rightOk) b.passLat = b.hi;
		else b.canPass = false;
		if (!b.canPass) followSpeed = std::min(followSpeed, otherSpeed + std::max(0.0f, ahead - CAR_GAP) * 0.8f);
		// 추월 중이라도 아직 옆으로 다 비키지 못했으면 들이받지 않게 속도를 맞춤
		else if (ahead < CAR_GAP + 3.0f && std::fabs(otherLat - myLat) < width * 0.5f + spec.size.x * 0.5f + 0.4f)
			followSpeed = std::min(followSpeed, otherSpeed + std::max(0.0f, ahead - 4.5f) * 0.8f);
		blockers.push_back(b);
	};
	for (int i = 0; i < g.carCount; i++)
	{
		const Car& o = g.cars[i];
		if (&o == &car || !o.active) continue;
		consider(o.pos, glm::dot(Car_Velocity(o), fwd), CarSpec_Get(o.type).size.x);
	}
	for (const Obstacle& o : t.obstacles) consider(o.pos, 0.0f, std::max(o.size.x, o.size.z));

	// 앞쪽 d(m) 지점에서 가야 할 옆 위치: 레이싱 라인, 단 막는 물체를 다 지나칠 때까지는 비켜 간 위치
	// (조향 목표점이 물체보다 앞에 있어도 추월이 끝날 때까지 옆 위치 유지)
	const float look = glm::clamp(4.0f + std::fabs(speed) * 0.35f, LOOK_AHEAD_MIN, LOOK_AHEAD_MAX);
	auto laneAt = [&](int i, float d) {
		float lat = sLine.offset[i] + st.bias;
		for (const Blocker& b : blockers)
			if (b.canPass && d > b.ahead - 12.0f && d < b.ahead + 6.0f + look && lat > b.lo && lat < b.hi)
				lat = b.passLat;
		return glm::clamp(lat, -halfRoad, halfRoad);
	};

	// ---------------- 조향: 앞쪽 목표점 (퓨어 퍼슛) ----------------
	float lookDist = 0.0f;
	int ti = Advance(idx, look, &lookDist);
	float targetLat = laneAt(ti, lookDist);
	glm::vec3 target = t.center[ti] + sLine.right[ti] * targetLat;
	glm::vec3 delta = target - car.pos;
	float diff = WrapAngle(std::atan2(delta.x, delta.z) - car.yaw);
	float curvature = 2.0f * std::sin(diff) / std::max(2.0f, glm::length(delta));    // 목표점까지 원호의 곡률
	float speedFactor = glm::clamp(std::fabs(speed) / 8.0f, 0.15f, 1.0f);
	float steer = -speed * curvature / (omega * speedFactor);
	// 지금 위치가 가야 할 옆 위치에서 벗어난 만큼 바로잡기 (멀리 있는 목표점만 보면 옆으로 흘러감)
	float latError = myLat - laneAt(idx, 0.0f);          // + 면 너무 오른쪽
	if (speed > 5.0f) steer -= glm::clamp(latError * LAT_GAIN, -0.35f, 0.35f);   // 오른쪽으로 벗어나면 왼쪽(-)으로
	in.steer = glm::clamp(steer, -1.0f, 1.0f);
	if (std::fabs(speed) < 2.0f) in.steer = glm::clamp(-diff * 2.0f, -1.0f, 1.0f);
	// 피해 가는 중이면 라인 밖(더 급한 커브)을 달리므로 코너를 더 천천히
	bool avoiding = false;
	for (const Blocker& b : blockers) if (b.canPass) avoiding = true;
	float cornerScale = avoiding ? 0.8f : 1.0f;

	// ---------------- 속도 계획 ----------------
	float maxSpeed = spec.maxSpeed * car.aiSkill;
	float desired = maxSpeed;
	float travelled = 0.0f;
	for (int j = idx, steps = 0; travelled < 110.0f && steps < n; steps++)
	{
		float corner = omega * sLine.radius[j] * CORNER_MARGIN * cornerScale * car.aiSkill;   // 이 지점에서 돌 수 있는 속도
		float allowed = std::sqrt(corner * corner + 2.0f * spec.brake * BRAKE_USE * std::max(0.0f, travelled - 3.0f));
		desired = std::min(desired, allowed);
		travelled += sLine.spacing[j];
		j = (j + 1) % n;
	}
	bool straightAhead = desired >= maxSpeed * 0.98f;
	desired = std::min(desired, followSpeed);
	if (std::fabs(diff) > 0.9f) desired = std::min(desired, 10.0f);   // 크게 틀어졌으면 천천히

	in.throttle = speed < desired ? 1.0f : 0.0f;
	in.brake = speed > desired + 0.8f ? glm::clamp((speed - desired) / 6.0f, 0.0f, 1.0f) : 0.0f;

	// ---------------- 부스트: 긴 직선 + 앞이 비었을 때 ----------------
	if (car.boostGauge <= 0.02f) st.boosting = false;
	else if (straightAhead && nearestAhead > 30.0f && speed > maxSpeed * 0.6f && car.boostGauge > 0.5f) st.boosting = true;
	if (!straightAhead || nearestAhead < 15.0f) st.boosting = false;
	in.boost = st.boosting;

	// ---------------- 막혔을 때 탈출 (후진하며 방향 돌리기) ----------------
	if (st.reverseTimer > 0.0f)
	{
		st.reverseTimer -= dt;
		in = CarInput();
		in.brake = 1.0f;                                   // 거의 멈춘 상태에서 브레이크 = 후진
		in.steer = diff > 0.0f ? 1.0f : -1.0f;              // 후진 중엔 반대로 꺾어야 앞머리가 목표를 향함
		return in;
	}
	bool tryingToMove = in.throttle > 0.0f;
	if (!car.finished && tryingToMove && std::fabs(speed) < 1.5f) st.stuckTimer += dt;
	else st.stuckTimer = std::max(0.0f, st.stuckTimer - dt * 2.0f);
	if (st.stuckTimer > 1.0f || (std::fabs(diff) > 2.2f && std::fabs(speed) < 4.0f))
	{
		st.stuckTimer = 0.0f;
		st.reverseTimer = 0.9f;
	}
	return in;
}
