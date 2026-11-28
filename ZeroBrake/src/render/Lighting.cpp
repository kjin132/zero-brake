// =====================================================================================
//  [담당: 진우석] 조명
//  - 방향광 1개 (낮 = 태양, 밤 = 달빛)
//  - 점광원: 가로등 (Scenery 가 위치를 알려줌). 셰이더 한도 8개 → 카메라에 가까운 8개만
//  - 스포트라이트: 차량 전조등 (밤 맵, 차 1대당 1개, 최대 4대)
//  - 안개: 멀리 있는 물체를 하늘색으로 흐리게 해서 깊이감을 줌
// =====================================================================================
#include "render/Lighting.h"
#include "core/Shader.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "world/Scenery.h"

static const int MAX_POINT = 8;   // basic.frag 의 MAX_POINT 와 같게
static const int MAX_SPOT = 4;    // basic.frag 의 MAX_SPOT 와 같게

void Lighting_Init()
{
}

bool Lighting_IsNight(int mapId) { return mapId == 1; }

glm::vec3 Lighting_SkyColor(int mapId)
{
	return Lighting_IsNight(mapId) ? glm::vec3(0.06f, 0.08f, 0.16f) : glm::vec3(0.55f, 0.75f, 0.95f);
}

static void SetArrayVec3(const char* name, int i, const glm::vec3& v)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%s[%d]", name, i);
	Shader_SetVec3(buf, v);
}

void Lighting_Apply(const Game& g, glm::vec3 viewPos, bool fog)
{
	int mapId = g.settings.mapId;
	bool night = Lighting_IsNight(mapId);

	if (!night)   // 고속 서킷: 맑은 낮
	{
		Shader_SetVec3("lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)));
		Shader_SetVec3("lightColor", glm::vec3(1.0f, 0.97f, 0.9f));
		Shader_SetFloat("ambientStrength", 0.35f);
	}
	else          // 테크니컬(도심) 서킷: 밤 — 약한 달빛 + 가로등 + 전조등
	{
		Shader_SetVec3("lightDir", glm::normalize(glm::vec3(0.5f, -0.8f, 0.3f)));
		Shader_SetVec3("lightColor", glm::vec3(0.45f, 0.5f, 0.75f));
		Shader_SetFloat("ambientStrength", 0.28f);
	}
	Shader_SetVec3("viewPos", viewPos);

	// ---- 가로등 (카메라에서 가까운 순서로 최대 8개) ----
	int pointCount = 0;
	if (night)
	{
		std::vector<glm::vec3> lamps = Scenery_LightPositions();
		std::sort(lamps.begin(), lamps.end(), [&](const glm::vec3& a, const glm::vec3& b) {
			return glm::length(a - viewPos) < glm::length(b - viewPos);
		});
		for (const glm::vec3& p : lamps)
		{
			if (pointCount >= MAX_POINT) break;
			SetArrayVec3("pointPos", pointCount, p);
			SetArrayVec3("pointColor", pointCount, glm::vec3(1.0f, 0.78f, 0.45f) * 1.6f);
			pointCount++;
		}
	}
	Shader_SetInt("pointCount", pointCount);

	// ---- 전조등 (밤 맵, 차 1대당 스포트라이트 1개) ----
	int spotCount = 0;
	if (night)
	{
		for (int i = 0; i < g.carCount && spotCount < MAX_SPOT; i++)
		{
			const Car& c = g.cars[i];
			if (!c.active) continue;
			glm::vec3 fwd = Car_Forward(c);
			float len = CarSpec_Get(c.type).size.z;
			SetArrayVec3("spotPos", spotCount, c.pos + fwd * (len * 0.5f) + glm::vec3(0, 0.8f, 0));
			SetArrayVec3("spotDir", spotCount, glm::normalize(fwd + glm::vec3(0, -0.12f, 0)));
			spotCount++;
		}
	}
	Shader_SetInt("spotCount", spotCount);
	Shader_SetVec3("spotColor", glm::vec3(1.0f, 0.95f, 0.8f) * 2.2f);
	Shader_SetFloat("spotInnerCos", std::cos(glm::radians(16.0f)));
	Shader_SetFloat("spotOuterCos", std::cos(glm::radians(26.0f)));

	// ---- 안개 ----
	Shader_SetInt("fogOn", fog ? 1 : 0);
	Shader_SetVec3("fogColor", Lighting_SkyColor(mapId));
	Shader_SetFloat("fogStart", night ? 60.0f : 120.0f);
	Shader_SetFloat("fogEnd", night ? 260.0f : 420.0f);
}
