// =====================================================================================
//  AI 경쟁차 주행
//
//  [레이싱 라인] 트랙마다 한 번 계산 (모든 AI 공유)
//   - 중심선을 도로 폭 안에서 반복해서 펴서(라플라시안 스무딩) 곡률이 가장 작은 경로를 만든다
//     → 자연스럽게 "바깥 → 안쪽(에이펙스) → 바깥"으로 코너를 돈다
//   - 부스트 패드 위치에서는 라인을 패드 위로 고정 → 패드를 반드시 밟고 지나감
//
//  [속도 계획] 이 게임 물리에서 코너 한계는 "조향 속도"다 (CarPhysics: 회전각속도 = steerRate)
//   - 반지름 R 커브의 최고 속도 = 회전각속도 × R   → 차종마다 코너 속도가 다름
//   - 앞 130m 까지 각 지점의 코너 속도와 제동 거리로 "지금 낼 수 있는 최고 속도"를 계산
//   - 부스트 중이면 최고 속도 1.25배까지 끝까지 밟음
//
//  [조향] ① 라인의 휘어짐만큼 미리 꺾기(피드포워드) ② 앞쪽 목표점 추적(퓨어 퍼슛)
//         ③ 지금 옆 위치 오차 바로잡기
//
//  [부스트(Shift)] 게이지는 놀리면 손해 (안 쓸 때만 초당 0.08 충전)
//   - 가속 중이고 다음 제동까지 시간이 충분하면 사용 → 코너 많은 맵에서도 코너 탈출마다 사용
//   - 게이지가 거의 꽉 차면 무조건 사용 (충전이 버려지지 않게)
//
//  [주변 차·장애물] 앞에 느린 차가 라인을 막으면 넓은 쪽으로 추월, 자리가 없으면 거리 유지
//  [막힘 탈출] 거의 멈춘 채 1초 → 후진하면서 방향을 돌려 빠져나옴
//
//  난이도: DIFFICULTY (전체), kSkill (AI 순서별)
// =====================================================================================
#include "race/AIDriver.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "world/Track.h"
#include <unordered_map>

namespace
{
// ---- 난이도 ----
const float DIFFICULTY = 1.0f;                                  // 전체 AI 실력 (0.9 = 쉬움, 1.0 = 최고)
const float kSkill[MAX_CARS] = { 1.0f, 1.0f, 0.985f, 0.97f };   // [0]=플레이어 자리, AI 1·2·3번째 차

// ---- 주행 상수 ----
const float CORNER_MARGIN = 3.0f;    // 코너 한계 속도 배율. 측정 결과 이 게임은 코너에서 거의 감속 안 하는 게 가장 빠름 (아주 급한 곳만 감속)
const float BRAKE_USE = 0.92f;       // 제동력의 몇 %를 계획에 쓸지
const float EDGE_GAP = 2.3f;         // 레이싱 라인이 도로 가장자리에서 떨어지는 거리 (차 중심 기준, m)
const float PLAN_DIST = 130.0f;      // 속도 계획을 위해 앞을 보는 거리 (m)
const float CAR_GAP = 7.0f;          // 따라갈 때 앞차와 유지할 거리 (m)
const float LAT_GAIN = 0.12f;        // 옆 위치 오차 1m 당 추가 조향량
const float BOOST_SCALE = 1.25f;     // 부스트 중 최고 속도 배율 (CarAbility 와 같게)

// ---- 레이싱 라인 ----
struct RacingLine
{
	int mapId = -1;
	size_t count = 0;
	glm::vec3 first = glm::vec3(0.0f);
	std::vector<glm::vec3> pts;      // 레이싱 라인 점 (중심선 점과 1:1)
	std::vector<glm::vec3> right;    // 각 중심선 점의 오른쪽 방향
	std::vector<float> offset;       // 중심선에서 레이싱 라인까지 옆 거리 (오른쪽 +)
	std::vector<float> radius;       // 곡률 반지름 (m)
	std::vector<float> turn;         // 부호 있는 곡률 (1/m, 왼쪽으로 돌면 +)
	std::vector<float> spacing;      // 다음 점까지 거리
	std::vector<char> pinned;        // 패드 위로 고정된 점
};
RacingLine sLine;

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
	sLine.right.resize(n); sLine.offset.assign(n, 0.0f); sLine.pts = c; sLine.pinned.assign(n, 0);
	for (int i = 0; i < n; i++) { glm::vec3 tg = TangentAt(c, i); sLine.right[i] = glm::vec3(-tg.z, 0, tg.x); }

	// 부스트 패드: 가장 가까운 중심선 점 앞뒤 몇 칸을 패드 위치로 고정
	for (const glm::vec3& pad : t.boostPads)
	{
		int p = Track_NearestIndex(t, pad);
		float lat = glm::dot(pad - c[p], sLine.right[p]);
		for (int k = -2; k <= 2; k++)
		{
			int i = (p + k + n) % n;
			sLine.pinned[i] = 1;
			sLine.offset[i] = lat;
			sLine.pts[i] = c[i] + sLine.right[i] * lat;
		}
	}

	// 반복 스무딩: 각 점을 양옆 점의 중간으로 당기되 도로 폭 안에서만
	const float limit = t.roadWidth * 0.5f - EDGE_GAP;
	for (int iter = 0; iter < 600; iter++)
		for (int i = 0; i < n; i++)
		{
			if (sLine.pinned[i]) continue;
			glm::vec3 mid = (sLine.pts[(i - 1 + n) % n] + sLine.pts[(i + 1) % n]) * 0.5f;
			float want = glm::dot(mid - c[i], sLine.right[i]);
			sLine.offset[i] = glm::clamp(glm::mix(sLine.offset[i], want, 0.6f), -limit, limit);
			sLine.pts[i] = c[i] + sLine.right[i] * sLine.offset[i];
		}

	// 곡률: 앞뒤 약 6m 떨어진 점 3개를 지나는 원
	sLine.spacing.resize(n); sLine.radius.resize(n); sLine.turn.resize(n);
	float avg = 0.0f;
	for (int i = 0; i < n; i++) { sLine.spacing[i] = glm::length(sLine.pts[(i + 1) % n] - sLine.pts[i]); avg += sLine.spacing[i]; }
	avg /= n;
	int k = std::max(1, (int)std::round(6.0f / std::max(0.1f, avg)));
	for (int i = 0; i < n; i++)
	{
		glm::vec3 a = sLine.pts[(i - k + n) % n], b = sLine.pts[i], d = sLine.pts[(i + k) % n];
		float ab = glm::length(b - a), bd = glm::length(d - b), ad = glm::length(d - a);
		float cr = glm::cross(b - a, d - a).y;                  // 부호 = 회전 방향
		float r = std::fabs(cr) < 1e-4f ? 1e4f : (ab * bd * ad) / (2.0f * std::fabs(cr));
		sLine.radius[i] = r;
		sLine.turn[i] = (cr > 0 ? 1.0f : -1.0f) / r;           // +: 왼쪽(yaw 증가) 회전 (진행 +z 에서 왼쪽 = +x)
	}
	std::vector<float> r = sLine.radius;                        // 짧은 급커브를 놓치지 않게 주변 최솟값
	for (int i = 0; i < n; i++)
		for (int j = -k; j <= k; j++) sLine.radius[i] = std::min(sLine.radius[i], r[(i + j + n) % n]);
}

void EnsureLine(const Track& t)
{
	if (t.center.size() < 3) return;
	if (sLine.mapId != t.mapId || sLine.count != t.center.size() || sLine.first != t.center[0]) BuildLine(t);
}

// 지난번 위치 근처에서만 가장 가까운 점 (트랙이 가까이 지나가는 곳에서 엉뚱한 구간을 잡지 않게)
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
	if (bestD > t.roadWidth * t.roadWidth * 2.0f) best = Track_NearestIndex(t, p);
	return best;
}

float Lateral(const Track& t, int i, glm::vec3 p) { return glm::dot(p - t.center[i], sLine.right[i]); }

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
	s.bias = (index % 2 == 0 ? 0.3f : -0.3f) * (1.0f - car.aiSkill) * 10.0f;   // 실력이 낮을수록 라인이 조금 어긋남
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
	const float halfRoad = t.roadWidth * 0.5f - spec.size.x * 0.5f - 0.7f;

	int idx = LocalNearest(t, car.pos, st.index);
	st.index = idx; car.aiTarget = idx;
	const float speed = car.speed;
	const glm::vec3 fwd = Car_Forward(car);
	const glm::vec3 tangentHere = TangentAt(t.center, idx);
	const float myLat = Lateral(t, idx, car.pos);

	// ---------------- 주변 차·장애물 ----------------
	struct Blocker { float ahead; float lo, hi; float passLat; bool canPass; };
	std::vector<Blocker> blockers;
	float followSpeed = 1e9f, nearestAhead = 1e9f;
	auto consider = [&](glm::vec3 pos, float otherSpeed, float width) {
		glm::vec3 rel = pos - car.pos; rel.y = 0.0f;
		if (glm::length(rel) > 42.0f) return;                      // 실제로 가까운 것만 (트랙 다른 구간 제외)
		float ahead = glm::dot(rel, tangentHere);
		if (ahead < 1.0f || ahead > 40.0f) return;
		if (speed - otherSpeed < -1.0f) return;                    // 앞차가 더 빠르면 신경 안 씀
		otherSpeed = std::max(0.0f, otherSpeed);                   // 후진 중인 차를 따라 후진하지 않게
		int oi = LocalNearest(t, pos, idx);
		int along = (oi - idx + n) % n;                            // 트랙을 따라 앞쪽에 있는지
		if (along > n / 4) return;
		float otherLat = Lateral(t, oi, pos);
		float need = width * 0.5f + spec.size.x * 0.5f + 1.2f;    // 중심끼리 이만큼 떨어져야 안전
		float lineLat = sLine.offset[oi] + st.bias;
		bool onLine = std::fabs(otherLat - lineLat) <= need;
		bool onPath = std::fabs(otherLat - myLat) <= need;
		if (!onLine && !onPath) return;
		nearestAhead = std::min(nearestAhead, ahead);
		Blocker b{ ahead, otherLat - need, otherLat + need, 0.0f, true };
		bool leftOk = b.lo > -halfRoad, rightOk = b.hi < halfRoad;
		if (leftOk && rightOk) b.passLat = std::fabs(b.lo - lineLat) < std::fabs(b.hi - lineLat) ? b.lo : b.hi;
		else if (leftOk) b.passLat = b.lo;
		else if (rightOk) b.passLat = b.hi;
		else b.canPass = false;
		if (!b.canPass) followSpeed = std::min(followSpeed, otherSpeed + std::max(0.0f, ahead - CAR_GAP) * 0.8f);
		else if (ahead < CAR_GAP + 3.0f && std::fabs(otherLat - myLat) < width * 0.5f + spec.size.x * 0.5f + 0.4f)
			followSpeed = std::min(followSpeed, otherSpeed + std::max(0.0f, ahead - 4.5f) * 0.8f);   // 아직 못 비켰으면 들이받지 않게
		blockers.push_back(b);
	};
	for (int i = 0; i < g.carCount; i++)
	{
		const Car& o = g.cars[i];
		if (&o == &car || !o.active) continue;
		consider(o.pos, glm::dot(Car_Velocity(o), fwd), CarSpec_Get(o.type).size.x);
	}
	for (const Obstacle& o : t.obstacles) consider(o.pos, 0.0f, std::max(o.size.x, o.size.z));

	// ---------------- 바로 옆 차 (나란히 달릴 때) ----------------
	// 옆에 차가 있으면 그쪽으로는 비켜 갈 수 없게 옆 위치의 범위를 좁힘
	// (출발 그리드처럼 나란히 서 있을 때 레이싱 라인으로 끼어들다 옆 차를 들이받지 않게)
	float latMin = -1e9f, latMax = 1e9f;
	for (int i = 0; i < g.carCount; i++)
	{
		const Car& o = g.cars[i];
		if (&o == &car || !o.active) continue;
		glm::vec3 rel = o.pos - car.pos; rel.y = 0.0f;
		if (glm::length(rel) > 14.0f) continue;
		float alongside = glm::dot(rel, tangentHere);
		const CarSpec& os = CarSpec_Get(o.type);
		float reach = (spec.size.z + os.size.z) * 0.5f + 2.5f;      // 앞뒤로 이만큼 안이면 "옆"
		if (std::fabs(alongside) > reach) continue;
		float otherLat = Lateral(t, idx, o.pos);
		float sep = (spec.size.x + os.size.x) * 0.5f + 1.4f;     // 옆 차와 이만큼(중심 기준) 떨어져 달림
		if (otherLat > myLat) latMax = std::min(latMax, otherLat - sep);   // 오른쪽에 있음
		else latMin = std::max(latMin, otherLat + sep);                   // 왼쪽에 있음
	}
	if (latMin > latMax) { float mid = (latMin + latMax) * 0.5f; latMin = latMax = mid; }   // 양옆이 막혔으면 가운데

	const float look = glm::clamp(3.0f + std::fabs(speed) * 0.3f, 5.0f, 18.0f);
	// 앞쪽 d(m) 지점에서 가야 할 옆 위치: 레이싱 라인, 막는 물체를 다 지나칠 때까지는 비켜 간 위치
	auto laneAt = [&](int i, float d) {
		float lat = sLine.offset[i] + (sLine.pinned[i] ? 0.0f : st.bias);
		for (const Blocker& b : blockers)
			if (b.canPass && d > b.ahead - 12.0f && d < b.ahead + 6.0f + look && lat > b.lo && lat < b.hi)
				lat = b.passLat;
		lat = glm::clamp(lat, std::max(-halfRoad, std::min(latMin, halfRoad)), std::min(halfRoad, std::max(latMax, -halfRoad)));
		return lat;
	};
	bool avoiding = false;
	for (const Blocker& b : blockers) if (b.canPass) avoiding = true;

	// ---------------- 조향 ----------------
	float lookDist = 0.0f;
	int ti = Advance(idx, look, &lookDist);
	glm::vec3 target = t.center[ti] + sLine.right[ti] * laneAt(ti, lookDist);
	glm::vec3 delta = target - car.pos;
	float diff = WrapAngle(std::atan2(delta.x, delta.z) - car.yaw);
	float speedFactor = glm::clamp(std::fabs(speed) / 8.0f, 0.15f, 1.0f);
	float yawRateMax = omega * speedFactor;
	// ① 피드포워드: 앞쪽 라인 곡률만큼 미리 꺾음 (피할 때는 절반만)
	int fi = Advance(idx, std::fabs(speed) * 0.15f);
	float ffWeight = avoiding ? 0.5f : 1.0f;
	float pathTurn = (sLine.turn[idx] + sLine.turn[fi]) * 0.5f * ffWeight;
	float steer = -(speed * pathTurn) / yawRateMax;
	// ② 퓨어 퍼슛: 목표점까지 원호 곡률에서 피드포워드 몫을 뺀 나머지
	float pursuitTurn = 2.0f * std::sin(diff) / std::max(2.0f, glm::length(delta));
	steer += -(speed * (pursuitTurn - pathTurn)) / yawRateMax * 0.8f;
	// ③ 지금 옆 위치 오차
	float latError = myLat - laneAt(idx, 0.0f);                // + 면 너무 오른쪽
	if (speed > 5.0f) steer -= glm::clamp(latError * LAT_GAIN, -0.35f, 0.35f);
	in.steer = glm::clamp(steer, -1.0f, 1.0f);
	if (std::fabs(speed) < 2.0f) in.steer = glm::clamp(-diff * 2.0f, -1.0f, 1.0f);

	// ---------------- 속도 계획 ----------------
	const float baseMax = spec.maxSpeed * car.aiSkill;
	const bool boosted = car.boostTime > 0.0f || st.boosting;
	const float topSpeed = baseMax * (boosted ? BOOST_SCALE : 1.0f);   // 부스트 중엔 1.25배까지 밟음
	const float cornerScale = avoiding ? 0.82f : 1.0f;
	const float brakeA = spec.brake * BRAKE_USE;
	float desired = topSpeed;
	float travelled = 0.0f, brakeDist = 1e9f;
	for (int j = idx, steps = 0; travelled < PLAN_DIST && steps < n; steps++)
	{
		float corner = omega * sLine.radius[j] * CORNER_MARGIN * cornerScale * car.aiSkill;
		float allowed = std::sqrt(corner * corner + 2.0f * brakeA * std::max(0.0f, travelled - 2.0f));
		desired = std::min(desired, allowed);
		// 지금 속도로 계속 가면 몇 m 뒤에 브레이크를 밟아야 하는지 (부스트 판단용)
		if (brakeDist > 1e8f && corner < speed)
			brakeDist = std::max(0.0f, travelled - (speed * speed - corner * corner) / (2.0f * brakeA));
		travelled += sLine.spacing[j];
		j = (j + 1) % n;
	}
	desired = std::min(desired, followSpeed);
	if (std::fabs(diff) > 0.9f) desired = std::min(desired, 10.0f);   // 크게 틀어졌으면 천천히

	in.throttle = speed < desired ? 1.0f : 0.0f;
	in.brake = speed > desired + 0.5f ? glm::clamp((speed - desired) / 4.0f, 0.0f, 1.0f) : 0.0f;

	// ---------------- 부스트 (Shift) ----------------
	{
		float timeToBrake = brakeDist / std::max(1.0f, speed);
		bool accelerating = in.throttle > 0.0f && speed < topSpeed - 0.5f && speed > 6.0f;
		bool roomAhead = nearestAhead > 12.0f || avoiding;
		bool worth = accelerating && roomAhead && timeToBrake > 0.7f && car.padBoostTime <= 0.0f;
		if (car.boostGauge <= 0.02f) st.boosting = false;                         // 다 쓰면 한 번 떼야 다시 됨
		else if (worth && (car.boostGauge > 0.3f || st.boosting)) st.boosting = true;
		else if (worth && car.boostGauge > 0.95f) st.boosting = true;            // 꽉 차면 버리지 말고 사용
		else st.boosting = false;
		in.boost = st.boosting;
	}

	// ---------------- 막혔을 때 탈출 ----------------
	if (st.reverseTimer > 0.0f)
	{
		st.reverseTimer -= dt;
		in = CarInput();
		in.brake = 1.0f;                                   // 거의 멈춘 상태에서 브레이크 = 후진
		in.steer = diff > 0.0f ? 1.0f : -1.0f;              // 후진 중엔 반대로 꺾어야 앞머리가 목표를 향함
		return in;
	}
	if (!car.finished && in.throttle > 0.0f && std::fabs(speed) < 1.5f) st.stuckTimer += dt;
	else st.stuckTimer = std::max(0.0f, st.stuckTimer - dt * 2.0f);
	if (st.stuckTimer > 1.0f || (std::fabs(diff) > 2.2f && std::fabs(speed) < 4.0f))
	{
		st.stuckTimer = 0.0f;
		st.reverseTimer = 0.9f;
	}
	return in;
}
