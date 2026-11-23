// =====================================================================================
//  [담당: 유현조] 배경
//  - 고속 서킷: 잔디 + 나무 (개방적인 야외 경기장)
//  - 테크니컬 서킷: 건물 + 표지판 (도심 분위기)
//  - 배치는 Build 에서 한 번 정하고, Draw 는 그리기만
//  TODO(현조): 관중석, 방향 표지판, 맵별 배치 다듬기
// =====================================================================================
#include "world/Scenery.h"
#include "world/Track.h"
#include "core/Mesh.h"
#include "render/Texture.h"
#include <random>

struct Prop { glm::vec3 pos; glm::vec3 size; glm::vec4 color; int kind; };   // kind 0 = 상자, 1 = 나무
static std::vector<Prop> sProps;
static glm::vec3 sGroundCenter(0.0f);
static float sGroundSize = 600.0f;

void Scenery_Build(const Track& t)
{
	sProps.clear();
	if (t.center.empty()) return;

	// 트랙 중심 → 잔디 바닥 위치
	glm::vec3 mn(1e9f), mx(-1e9f);
	for (const glm::vec3& p : t.center) { mn = glm::min(mn, p); mx = glm::max(mx, p); }
	sGroundCenter = (mn + mx) * 0.5f;
	sGroundSize = std::max(mx.x - mn.x, mx.z - mn.z) + 300.0f;

	std::mt19937 rng(42 + t.mapId);
	std::uniform_real_distribution<float> r01(0.0f, 1.0f);
	int n = (int)t.center.size();
	for (int i = 0; i < n; i += 6)
	{
		glm::vec3 a = t.center[(i + 1) % n] - t.center[(i - 1 + n) % n];
		glm::vec3 side = glm::normalize(glm::vec3(-a.z, 0, a.x));
		for (int s = -1; s <= 1; s += 2)
		{
			glm::vec3 p = t.center[i] + side * (float)s * (t.roadWidth * 0.5f + 8.0f + r01(rng) * 25.0f);
			glm::vec3 near;
			if (Track_DistanceFromCenter(t, p, &near, nullptr) < t.roadWidth * 0.5f + 6.0f) continue;   // 도로 위 X

			if (t.mapId == 0)
				sProps.push_back({ p, glm::vec3(3.0f, 4.0f + r01(rng) * 2.0f, 3.0f), glm::vec4(0.2f, 0.55f, 0.25f, 1.0f), 1 });
			else
			{
				float h = 8.0f + r01(rng) * 20.0f;
				glm::vec3 c = glm::vec3(0.55f, 0.55f, 0.6f) + glm::vec3(r01(rng), r01(rng), r01(rng)) * 0.25f;
				sProps.push_back({ p, glm::vec3(6.0f + r01(rng) * 4.0f, h, 6.0f + r01(rng) * 4.0f), glm::vec4(c, 1.0f), 0 });
			}
		}
	}
}

void Scenery_Draw()
{
	// 잔디 바닥 (텍스처 반복은 크기에 비례해 UV 를 늘리는 대신 여러 장으로 나눠도 됨)
	glm::mat4 g = glm::scale(glm::translate(glm::mat4(1.0f), sGroundCenter + glm::vec3(0, -0.02f, 0)), glm::vec3(sGroundSize, 1.0f, sGroundSize));
	DrawOpt grass; grass.color = glm::vec4(0.45f, 0.75f, 0.4f, 1.0f); grass.texture = Texture_Get(TexId::Grass);
	Draw_Mesh(gQuad, g, grass);

	for (const Prop& p : sProps)
	{
		if (p.kind == 1)   // 나무: 줄기 + 잎
		{
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, 1.0f, 0), glm::vec3(0.6f, 2.0f, 0.6f), glm::vec4(0.45f, 0.3f, 0.15f, 1.0f));
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, 2.0f + p.size.y * 0.5f, 0), p.size, p.color);
		}
		else               // 건물
			Draw_Box(glm::mat4(1.0f), p.pos + glm::vec3(0, p.size.y * 0.5f, 0), p.size, p.color);
	}
}
