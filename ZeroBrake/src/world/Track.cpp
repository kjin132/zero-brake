// =====================================================================================
//  [담당: 유현조] 트랙 2종
//  - 조절점(kMap0/kMap1)을 Catmull-Rom 곡선으로 이어 중심선을 만들고,
//    각 점의 좌우(수직 방향)로 도로 폭/2 만큼 벌려 도로 메시를 생성
//  - 조절점 순서 = 주행 방향, [0] 근처가 출발선
//  고속/테크니컬 도로, 16개 체크포인트, 패드와 장애물을 생성한다.
// =====================================================================================
#include "world/Track.h"
#include "core/Mesh.h"
#include "render/Texture.h"

// ---- 조절점 (x, z) ----
// 맵 0: 고속 서킷 — 긴 직선 + 완만한 곡선, 넓은 도로
static const glm::vec2 kMap0[] = {
	{0, 0}, {0, 80}, {10, 130}, {50, 150}, {110, 150}, {150, 130},
	{160, 80}, {160, 0}, {150, -50}, {110, -70}, {50, -70}, {10, -50}
};
// 맵 1: 테크니컬 서킷 — 연속 코너 + 급커브 + 짧은 직선
static const glm::vec2 kMap1[] = {
	{0, 0}, {0, 50}, {15, 75}, {45, 75}, {55, 50}, {75, 35}, {100, 50},
	{110, 85}, {140, 90}, {150, 60}, {130, 25}, {135, -10}, {110, -35},
	{75, -25}, {55, -45}, {25, -45}, {5, -30}
};

static const float kRoadWidth[2] = { 16.0f, 12.0f };
static const int kCheckpointCount = 16;

static Mesh sRoad, sCurbL, sCurbR;

static glm::vec2 CatmullRom(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float t)
{
	float t2 = t * t, t3 = t2 * t;
	return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

static glm::vec3 TangentAt(const Track& t, int i)
{
	int n = (int)t.center.size();
	glm::vec3 d = t.center[(i + 1) % n] - t.center[(i - 1 + n) % n];
	d.y = 0.0f;
	return glm::normalize(d);
}

// 중심선을 따라 띠 메시 만들기 (inner < outer, 중심에서 오른쪽으로의 거리. 왼쪽은 음수)
static Mesh BuildStrip(const Track& t, float inner, float outer, float height, float uvScaleV)
{
	std::vector<Vertex> v;
	int n = (int)t.center.size();
	float dist = 0.0f;
	for (int i = 0; i < n; i++)
	{
		int j = (i + 1) % n;
		glm::vec3 ti = TangentAt(t, i), tj = TangentAt(t, j);
		glm::vec3 li(-ti.z, 0, ti.x), lj(-tj.z, 0, tj.x);   // 진행 방향 오른쪽 (Car_Right 와 같은 공식)
		glm::vec3 up(0, 1, 0), h(0, height, 0);
		float seg = glm::length(t.center[j] - t.center[i]);
		float v0 = dist / uvScaleV, v1 = (dist + seg) / uvScaleV;
		dist += seg;

		glm::vec3 a = t.center[i] + li * inner + h, b = t.center[i] + li * outer + h;
		glm::vec3 c = t.center[j] + lj * outer + h, d = t.center[j] + lj * inner + h;
		// inner < outer 로 넘기면 위에서 볼 때 반시계(앞면)가 됨
		v.push_back({ a, up, {0, v0} }); v.push_back({ b, up, {1, v0} }); v.push_back({ c, up, {1, v1} });
		v.push_back({ a, up, {0, v0} }); v.push_back({ c, up, {1, v1} }); v.push_back({ d, up, {0, v1} });
	}
	return Mesh_Create(v);
}

void Track_Build(Track& t, int mapId)
{
	t = Track();
	t.mapId = mapId;
	t.roadWidth = kRoadWidth[mapId == 0 ? 0 : 1];

	const glm::vec2* cp = (mapId == 0) ? kMap0 : kMap1;
	int m = (mapId == 0) ? (int)(sizeof(kMap0) / sizeof(kMap0[0])) : (int)(sizeof(kMap1) / sizeof(kMap1[0]));

	// 조절점 사이를 촘촘한 점으로
	const int SUB = 20;
	for (int i = 0; i < m; i++)
		for (int k = 0; k < SUB; k++)
		{
			glm::vec2 p = CatmullRom(cp[(i - 1 + m) % m], cp[i], cp[(i + 1) % m], cp[(i + 2) % m], (float)k / SUB);
			t.center.push_back(glm::vec3(p.x, 0.0f, p.y));
		}

	int n = (int)t.center.size();
	for (int c = 0; c < kCheckpointCount; c++)
	{
		int idx = c * n / kCheckpointCount;
		t.checkpoints.push_back({ t.center[idx], TangentAt(t, idx), t.roadWidth * 0.5f });
	}
	t.startPos = t.checkpoints[0].pos;
	t.startYaw = std::atan2(t.checkpoints[0].forward.x, t.checkpoints[0].forward.z);

	// 부스트 패드: 직선 구간 예시 두 곳
	t.boostPads.push_back(t.center[n / 12]);
	t.boostPads.push_back(t.center[n / 2 + n / 12]);

	// 직선의 한쪽 차선에 작은 방호 블록을 배치한다. 중앙 주행선은 비워둔다.
	for (int idx : { n / 10, n / 2 + n / 10 })
	{
		glm::vec3 tg = TangentAt(t, idx), side(-tg.z, 0, tg.x);
		t.obstacles.push_back({ t.center[idx] + side * (t.roadWidth * 0.30f), {1.4f, 1.0f, 1.4f} });
	}

	Mesh_Destroy(sRoad); Mesh_Destroy(sCurbL); Mesh_Destroy(sCurbR);
	float hw = t.roadWidth * 0.5f;
	sRoad = BuildStrip(t, -hw, hw, 0.02f, t.roadWidth);
	sCurbL = BuildStrip(t, hw, hw + 1.2f, 0.04f, 2.4f);
	sCurbR = BuildStrip(t, -hw - 1.2f, -hw, 0.04f, 2.4f);
}

void Track_Draw(const Track& t)
{
	DrawOpt road; road.texture = Texture_Get(TexId::Road);
	Draw_Mesh(sRoad, glm::mat4(1.0f), road);

	DrawOpt curb; curb.texture = Texture_Get(TexId::Curb);
	Draw_Mesh(sCurbL, glm::mat4(1.0f), curb);
	Draw_Mesh(sCurbR, glm::mat4(1.0f), curb);

	// 출발·결승선 (체커 무늬)
	if (!t.checkpoints.empty())
	{
		const Checkpoint& s = t.checkpoints[0];
		glm::mat4 m = glm::translate(glm::mat4(1.0f), s.pos + glm::vec3(0, 0.05f, 0));
		m = glm::rotate(m, std::atan2(s.forward.x, s.forward.z), glm::vec3(0, 1, 0));
		m = glm::scale(m, glm::vec3(t.roadWidth, 1.0f, 2.0f));
		DrawOpt fin; fin.texture = Texture_Get(TexId::Checker);
		Draw_Mesh(gQuad, m, fin);
	}

	// 점선 중앙 차선과 출발선 아치로 진행 방향을 읽기 쉽게 한다.
	for (int i = 0; i < (int)t.center.size(); i += 5)
	{
		glm::vec3 tg = TangentAt(t, i);
		glm::mat4 m = glm::translate(glm::mat4(1), t.center[i] + glm::vec3(0, 0.045f, 0));
		m = glm::rotate(m, std::atan2(tg.x, tg.z), glm::vec3(0,1,0));
		Draw_Box(m, {0,0,0}, {0.16f,0.015f,1.8f}, glm::vec4(0.95f,0.87f,0.45f,1));
	}
	glm::mat4 gate = glm::rotate(glm::translate(glm::mat4(1), t.startPos), t.startYaw, glm::vec3(0,1,0));
	float post = t.roadWidth * 0.5f + 1.4f;
	Draw_Box(gate, {-post,3.0f,0}, {0.8f,6.0f,0.8f}, glm::vec4(0.18f,0.23f,0.3f,1));
	Draw_Box(gate, {post,3.0f,0}, {0.8f,6.0f,0.8f}, glm::vec4(0.18f,0.23f,0.3f,1));
	Draw_Box(gate, {0,6.0f,0}, {post*2+0.8f,0.9f,0.7f}, glm::vec4(0.12f,0.16f,0.22f,1));
	for (int i = -3; i <= 3; ++i)
		Draw_Box(gate, {(float)i*1.2f,6.0f,-0.38f}, {0.6f,0.55f,0.08f},
			i % 2 == 0 ? glm::vec4(1) : glm::vec4(0.1f,0.8f,1,1));

	// 부스트 패드
	for (const glm::vec3& p : t.boostPads)
	{
		glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), p + glm::vec3(0, 0.06f, 0)), glm::vec3(4.0f, 1.0f, 4.0f));
		DrawOpt o; o.color = glm::vec4(0.0f, 0.9f, 1.0f, 1.0f);
		Draw_Mesh(gQuad, m, o);
	}

	// 경계 방호벽 (낮은 상자)
	int n = (int)t.center.size();
	for (int i = 0; i < n; i += 4)
	{
		glm::vec3 tg = TangentAt(t, i), side(-tg.z, 0, tg.x);
		float off = t.roadWidth * 0.5f + 1.8f;
		glm::mat4 r = glm::rotate(glm::mat4(1.0f), std::atan2(tg.x, tg.z), glm::vec3(0, 1, 0));
		for (int s = -1; s <= 1; s += 2)
		{
			glm::mat4 m = glm::translate(glm::mat4(1.0f), t.center[i] + side * off * (float)s) * r;
			Draw_Box(m, { 0, 0.4f, 0 }, { 0.5f, 0.8f, 3.6f }, glm::vec4(0.85f, 0.85f, 0.88f, 1.0f));
		}
	}

	for (const Obstacle& o : t.obstacles)
		Draw_Box(glm::mat4(1.0f), o.pos + glm::vec3(0, o.size.y * 0.5f, 0), o.size, glm::vec4(1.0f, 0.5f, 0.1f, 1.0f));
}

int Track_NearestIndex(const Track& t, glm::vec3 p)
{
	int best = 0;
	float bestD = 1e30f;
	for (int i = 0; i < (int)t.center.size(); i++)
	{
		glm::vec3 d = t.center[i] - p;
		float dd = d.x * d.x + d.z * d.z;
		if (dd < bestD) { bestD = dd; best = i; }
	}
	return best;
}

float Track_DistanceFromCenter(const Track& t, glm::vec3 p, glm::vec3* nearest, glm::vec3* tangent)
{
	int n = (int)t.center.size();
	float bestD = 1e30f;
	glm::vec3 bestP(0.0f), bestT(0, 0, 1);
	for (int i = 0; i < n; i++)
	{
		glm::vec3 a = t.center[i], b = t.center[(i + 1) % n];
		glm::vec3 ab = b - a; ab.y = 0.0f;
		glm::vec3 ap = p - a; ap.y = 0.0f;
		float len2 = glm::dot(ab, ab);
		float k = len2 > 0.0f ? glm::clamp(glm::dot(ap, ab) / len2, 0.0f, 1.0f) : 0.0f;
		glm::vec3 q = a + ab * k;
		glm::vec3 d = p - q; d.y = 0.0f;
		float dd = glm::dot(d, d);
		if (dd < bestD) { bestD = dd; bestP = q; bestT = len2 > 0.0f ? glm::normalize(ab) : bestT; }
	}
	if (nearest) *nearest = bestP;
	if (tangent) *tangent = bestT;
	return std::sqrt(bestD);
}
