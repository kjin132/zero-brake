// =====================================================================================
//  배경
//  - 고속 서킷(낮): 잔디 + 나무, 출발 직선 옆 관중석 (개방적인 야외 경기장)
//  - 테크니컬 서킷(밤, 도심): 보도 바닥 + 창문에 불 켜진 건물 + 가로등
//  - 두 맵 공통: 급커브 바깥쪽에 방향 표지판(노란 바탕 + 검은 화살표)
//  - 배치는 Build 에서 한 번 정하고, Draw 는 그리기만
// =====================================================================================
#include "world/Scenery.h"
#include "world/Track.h"
#include "core/Mesh.h"
#include "render/Texture.h"
#include "render/Lighting.h"
#include <random>

struct Prop { glm::vec3 pos; glm::vec3 size; glm::vec4 color; int kind; float yaw; };
enum PropKind { BOX = 0, TREE = 1, BUILDING = 2 };

struct Sign { glm::vec3 pos; float yaw; float dir; };        // dir: +1 = 화살표가 운전자 기준 왼쪽, -1 = 오른쪽
struct Lamp { glm::vec3 base; float yaw; };                  // yaw: 팔이 도로 쪽을 향하는 방향
struct Stand { glm::vec3 pos; float yaw; float length; };    // 관중석 (트랙을 바라봄)
struct Fan { glm::vec3 pos; glm::vec4 color; };

static std::vector<Prop> sProps;
static std::vector<Sign> sSigns;
static std::vector<Lamp> sLamps;
static std::vector<glm::vec3> sLightPos;
static std::vector<Stand> sStands;
static std::vector<Fan> sFans;
static glm::vec3 sGroundCenter(0.0f);
static float sGroundSize = 600.0f;
static int sMapId = 0;

static const float LAMP_HEIGHT = 7.0f;
static const float LAMP_ARM = 1.8f;

static glm::vec3 Tangent(const Track& t, int i)
{
	int n = (int)t.center.size();
	glm::vec3 d = t.center[(i + 1) % n] - t.center[(i - 1 + n) % n];
	d.y = 0.0f;
	return glm::normalize(d);
}
static glm::vec3 RightOf(glm::vec3 tg) { return glm::vec3(-tg.z, 0.0f, tg.x); }   // Car_Right 와 같은 공식
static float YawOf(glm::vec3 dir) { return std::atan2(dir.x, dir.z); }

static float WrapAngle(float a)
{
	while (a > PI) a -= 2 * PI;
	while (a < -PI) a += 2 * PI;
	return a;
}

// 도로에서 충분히 떨어져 있는지 (배경 물체가 도로를 막지 않게)
static bool ClearOfRoad(const Track& t, glm::vec3 p, float margin)
{
	glm::vec3 near;
	return Track_DistanceFromCenter(t, p, &near, nullptr) > t.roadWidth * 0.5f + margin;
}

// ---------------------------------------------------------------- 배치
static void BuildSigns(const Track& t)
{
	// 방향이 크게 꺾이는 곳(앞뒤 6칸 접선 각도 차이가 큰 곳)마다 바깥쪽에 표지판
	int n = (int)t.center.size();
	std::vector<float> turn(n);
	for (int i = 0; i < n; i++)
		turn[i] = WrapAngle(YawOf(Tangent(t, (i + 6) % n)) - YawOf(Tangent(t, (i - 6 + n) % n)));

	int last = -1000;
	for (int i = 0; i < n; i++)
	{
		float a = std::fabs(turn[i]);
		if (a < glm::radians(38.0f)) continue;
		bool peak = true;
		for (int k = -8; k <= 8 && peak; k++)
			if (std::fabs(turn[(i + k + n) % n]) > a) peak = false;
		if (!peak || i - last < 25) continue;
		last = i;

		// yaw 가 커지면 왼쪽으로 도는 것 → 바깥쪽 = 오른쪽
		float left = turn[i] > 0.0f ? 1.0f : -1.0f;
		glm::vec3 approach = Tangent(t, (i - 8 + n) % n);           // 코너에 들어오는 방향
		glm::vec3 outside = RightOf(Tangent(t, i)) * left;
		glm::vec3 p = t.center[i] + outside * (t.roadWidth * 0.5f + 4.0f);
		sSigns.push_back({ p, YawOf(approach), left });
	}
}

static void BuildLamps(const Track& t)
{
	int n = (int)t.center.size();
	for (int i = 0, k = 0; i < n; i += 16, k++)
	{
		glm::vec3 tg = Tangent(t, i);
		float side = (k % 2 == 0) ? 1.0f : -1.0f;
		glm::vec3 out = RightOf(tg) * side;
		glm::vec3 base = t.center[i] + out * (t.roadWidth * 0.5f + 2.6f);
		if (!ClearOfRoad(t, base, 1.5f)) continue;
		float yaw = YawOf(-out);                                    // 팔이 도로 쪽으로
		sLamps.push_back({ base, yaw });
		sLightPos.push_back(base - out * LAMP_ARM + glm::vec3(0, LAMP_HEIGHT - 0.6f, 0));
	}
}

static void BuildStand(const Track& t, std::mt19937& rng)
{
	// 출발 직선 중 트랙 안쪽이 아닌 쪽(바깥)에 관중석
	int n = (int)t.center.size();
	int idx = n / 30;
	glm::vec3 tg = Tangent(t, idx), right = RightOf(tg);
	float off = t.roadWidth * 0.5f + 9.0f;
	glm::vec3 near;
	float dR = Track_DistanceFromCenter(t, t.center[idx] + right * 40.0f, &near, nullptr);
	float dL = Track_DistanceFromCenter(t, t.center[idx] - right * 40.0f, &near, nullptr);
	float side = dR > dL ? 1.0f : -1.0f;
	glm::vec3 p = t.center[idx] + right * side * off;
	float yaw = YawOf(-right * side);                                // 관중석 앞면이 트랙을 봄
	Stand st{ p, yaw, 44.0f };
	sStands.push_back(st);

	// 관중 (단마다 무작위 색 상자)
	std::uniform_real_distribution<float> r01(0.0f, 1.0f);
	glm::mat4 M = glm::rotate(glm::translate(glm::mat4(1.0f), p), yaw, glm::vec3(0, 1, 0));
	for (int tier = 0; tier < 4; tier++)
		for (float x = -st.length * 0.5f + 1.0f; x < st.length * 0.5f - 1.0f; x += 1.1f)
		{
			if (r01(rng) < 0.25f) continue;
			glm::vec3 local(x, 1.0f + tier * 1.0f + 0.35f, 1.6f - tier * 1.6f);
			glm::vec3 col = glm::vec3(r01(rng), r01(rng), r01(rng)) * 0.8f + 0.15f;
			sFans.push_back({ glm::vec3(M * glm::vec4(local, 1.0f)), glm::vec4(col, 1.0f) });
		}
}

void Scenery_Build(const Track& t)
{
	sProps.clear(); sSigns.clear(); sLamps.clear(); sLightPos.clear(); sStands.clear(); sFans.clear();
	sMapId = t.mapId;
	if (t.center.empty()) return;

	// 트랙 중심 → 바닥 위치
	glm::vec3 mn(1e9f), mx(-1e9f);
	for (const glm::vec3& p : t.center) { mn = glm::min(mn, p); mx = glm::max(mx, p); }
	sGroundCenter = (mn + mx) * 0.5f;
	sGroundSize = std::max(mx.x - mn.x, mx.z - mn.z) + 300.0f;

	std::mt19937 rng(42 + t.mapId);
	std::uniform_real_distribution<float> r01(0.0f, 1.0f);

	BuildSigns(t);
	if (t.mapId == 0) BuildStand(t, rng);
	else BuildLamps(t);

	// 관중석과 겹치지 않게
	auto nearStand = [&](glm::vec3 p) {
		for (const Stand& s : sStands) if (glm::length(p - s.pos) < s.length * 0.5f + 10.0f) return true;
		return false;
	};

	int n = (int)t.center.size();
	for (int i = 0; i < n; i += 6)
	{
		glm::vec3 side = RightOf(Tangent(t, i));
		for (int s = -1; s <= 1; s += 2)
		{
			glm::vec3 p = t.center[i] + side * (float)s * (t.roadWidth * 0.5f + 9.0f + r01(rng) * 25.0f);
			if (!ClearOfRoad(t, p, 7.0f) || nearStand(p)) continue;

			if (t.mapId == 0)
				sProps.push_back({ p, glm::vec3(3.0f, 4.0f + r01(rng) * 2.0f, 3.0f), glm::vec4(0.2f, 0.55f, 0.25f, 1.0f), TREE, 0.0f });
			else
			{
				float h = 10.0f + r01(rng) * 26.0f;
				glm::vec3 c = glm::vec3(0.55f, 0.55f, 0.6f) + glm::vec3(r01(rng), r01(rng), r01(rng)) * 0.3f;
				float w = 7.0f + r01(rng) * 5.0f, d = 7.0f + r01(rng) * 5.0f;
				if (!ClearOfRoad(t, p, std::max(w, d) * 0.5f + 5.0f)) continue;   // 큰 건물이 도로를 덮지 않게
				sProps.push_back({ p, glm::vec3(w, h, d), glm::vec4(c, 1.0f), BUILDING, YawOf(Tangent(t, i)) });
			}
		}
	}
}

const std::vector<glm::vec3>& Scenery_LightPositions() { return sLightPos; }

// ---------------------------------------------------------------- 그리기
static void DrawSign(const Sign& s)
{
	glm::mat4 M = glm::rotate(glm::translate(glm::mat4(1.0f), s.pos), s.yaw, glm::vec3(0, 1, 0));
	glm::vec4 black(0.05f, 0.05f, 0.05f, 1.0f), yellow(1.0f, 0.82f, 0.1f, 1.0f);
	// 기둥 2개 + 판
	Draw_Box(M, { -1.6f, 0.9f, 0.1f }, { 0.15f, 1.8f, 0.15f }, glm::vec4(0.5f, 0.5f, 0.55f, 1));
	Draw_Box(M, { 1.6f, 0.9f, 0.1f }, { 0.15f, 1.8f, 0.15f }, glm::vec4(0.5f, 0.5f, 0.55f, 1));
	DrawOpt board; board.color = yellow; board.emissive = Lighting_IsNight(sMapId) ? 0.5f : 0.0f;
	Draw_Mesh(gCube, glm::scale(glm::translate(M, { 0, 2.4f, 0 }), glm::vec3(4.2f, 1.3f, 0.12f)), board);

	// 화살표 3개 (다가오는 운전자가 보는 면 = 판의 -z 쪽).
	// 운전자 기준 왼쪽 = 판의 +x, 오른쪽 = -x  → dir(+1 왼쪽) 방향으로 꺾쇠를 그림
	for (int k = -1; k <= 1; k++)
	{
		float cx = k * 1.2f;
		for (int up = -1; up <= 1; up += 2)
		{
			glm::mat4 b = glm::translate(M, { cx - 0.08f * s.dir, 2.4f + 0.2f * up, -0.08f });
			b = glm::rotate(b, glm::radians(-40.0f * up * s.dir), glm::vec3(0, 0, 1));
			Draw_Box(b, glm::vec3(0), { 0.55f, 0.16f, 0.04f }, black);
		}
	}
}

static void DrawLamp(const Lamp& l)
{
	glm::mat4 M = glm::rotate(glm::translate(glm::mat4(1.0f), l.base), l.yaw, glm::vec3(0, 1, 0));
	glm::vec4 metal(0.3f, 0.32f, 0.36f, 1.0f);
	Draw_Box(M, { 0, LAMP_HEIGHT * 0.5f, 0 }, { 0.22f, LAMP_HEIGHT, 0.22f }, metal);
	Draw_Box(M, { 0, LAMP_HEIGHT - 0.1f, LAMP_ARM * 0.5f }, { 0.14f, 0.14f, LAMP_ARM }, metal);
	DrawOpt glow; glow.color = glm::vec4(1.0f, 0.85f, 0.55f, 1.0f); glow.lit = false;   // 갓은 스스로 빛남
	Draw_Mesh(gCube, glm::scale(glm::translate(M, { 0, LAMP_HEIGHT - 0.3f, LAMP_ARM }), glm::vec3(0.5f, 0.2f, 0.7f)), glow);
}

static void DrawStand(const Stand& s)
{
	glm::mat4 M = glm::rotate(glm::translate(glm::mat4(1.0f), s.pos), s.yaw, glm::vec3(0, 1, 0));
	glm::vec4 concrete(0.72f, 0.72f, 0.75f, 1.0f), seat(0.15f, 0.35f, 0.75f, 1.0f);
	// 계단식 단 4개 (앞쪽 +z 가 트랙)
	for (int tier = 0; tier < 4; tier++)
	{
		float h = 1.0f + tier * 1.0f;
		Draw_Box(M, { 0, h * 0.5f, 1.6f - tier * 1.6f }, { s.length, h, 1.6f }, tier % 2 ? seat : concrete);
	}
	// 지붕 + 기둥
	Draw_Box(M, { 0, 7.5f, -1.0f }, { s.length + 2.0f, 0.3f, 7.5f }, glm::vec4(0.85f, 0.2f, 0.15f, 1.0f));
	for (float x = -s.length * 0.5f; x <= s.length * 0.5f + 0.1f; x += s.length / 4.0f)
		Draw_Box(M, { x, 3.75f, -4.4f }, { 0.4f, 7.5f, 0.4f }, glm::vec4(0.4f, 0.4f, 0.45f, 1.0f));
	// 광고판
	Draw_Box(M, { 0, 0.6f, 3.0f }, { s.length, 1.2f, 0.2f }, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));
	for (int k = 0; k < 6; k++)
		Draw_Box(M, { -s.length * 0.5f + 4.0f + k * (s.length - 8.0f) / 5.0f, 0.6f, 3.12f }, { 4.0f, 0.8f, 0.05f },
			k % 2 ? glm::vec4(0.9f, 0.3f, 0.15f, 1.0f) : glm::vec4(0.1f, 0.7f, 0.9f, 1.0f));
}

void Scenery_Draw()
{
	bool night = Lighting_IsNight(sMapId);

	// 바닥: 낮 = 잔디, 밤 도심 = 아스팔트 보도
	glm::mat4 g = glm::scale(glm::translate(glm::mat4(1.0f), sGroundCenter + glm::vec3(0, -0.02f, 0)), glm::vec3(sGroundSize, 1.0f, sGroundSize));
	DrawOpt ground;
	if (!night) { ground.color = glm::vec4(0.45f, 0.75f, 0.4f, 1.0f); ground.texture = Texture_Get(TexId::Grass); }
	else { ground.color = glm::vec4(0.8f, 0.8f, 0.85f, 1.0f); ground.texture = Texture_Get(TexId::Asphalt); }
	ground.uvScale = glm::vec2(sGroundSize / 8.0f);
	Draw_Mesh(gQuad, g, ground);

	for (const Prop& p : sProps)
	{
		if (p.kind == TREE)   // 나무: 줄기 + 잎 2단
		{
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, 1.0f, 0), glm::vec3(0.6f, 2.0f, 0.6f), glm::vec4(0.45f, 0.3f, 0.15f, 1.0f));
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, 2.0f + p.size.y * 0.5f, 0), p.size, p.color);
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, 2.0f + p.size.y + 0.6f, 0), p.size * glm::vec3(0.6f, 0.3f, 0.6f), p.color * 1.1f);
		}
		else if (p.kind == BUILDING)   // 건물: 창문 텍스처, 밤엔 창문 불빛
		{
			glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), p.pos + glm::vec3(0, p.size.y * 0.5f, 0)), p.yaw, glm::vec3(0, 1, 0));
			DrawOpt o;
			o.color = p.color;
			o.texture = Texture_Get(night ? TexId::WindowsNight : TexId::WindowsDay);
			o.uvScale = glm::vec2((p.size.x + p.size.z) * 0.5f / 12.0f, p.size.y / 13.0f);   // 창문 1칸 ≈ 3m × 3.2m
			o.emissive = night ? 0.9f : 0.0f;
			Draw_Mesh(gCube, glm::scale(m, p.size), o);
			Draw_Box(m, { 0, p.size.y * 0.5f + 0.4f, 0 }, { p.size.x * 0.6f, 0.8f, p.size.z * 0.6f }, p.color * 0.7f);   // 옥상
		}
		else
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, p.size.y * 0.5f, 0), p.size, p.color);
	}

	for (const Stand& s : sStands) DrawStand(s);
	for (const Fan& f : sFans) Draw_Box(glm::mat4(1.0f), f.pos, glm::vec3(0.45f, 0.7f, 0.45f), f.color);
	for (const Sign& s : sSigns) DrawSign(s);
	for (const Lamp& l : sLamps) DrawLamp(l);
}
