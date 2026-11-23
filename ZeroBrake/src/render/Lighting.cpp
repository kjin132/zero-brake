// =====================================================================================
//  [담당: 진우석] 조명
//  - 지금: 방향광(태양) 하나 + 주변광
//  TODO(우석): 맵별 빛 색·방향, 필요하면 점광원/스포트라이트 추가 (basic.frag 도 같이 수정)
// =====================================================================================
#include "render/Lighting.h"
#include "core/Shader.h"

void Lighting_Init()
{
}

void Lighting_Apply(const Game& g, glm::vec3 viewPos)
{
	if (g.settings.mapId == 0)   // 고속 서킷: 맑은 낮
	{
		Shader_SetVec3("lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)));
		Shader_SetVec3("lightColor", glm::vec3(1.0f, 0.97f, 0.9f));
		Shader_SetFloat("ambientStrength", 0.35f);
	}
	else                         // 테크니컬 서킷(도심): 해질녘 느낌
	{
		Shader_SetVec3("lightDir", glm::normalize(glm::vec3(0.6f, -0.7f, 0.2f)));
		Shader_SetVec3("lightColor", glm::vec3(1.0f, 0.85f, 0.7f));
		Shader_SetFloat("ambientStrength", 0.3f);
	}
	Shader_SetVec3("viewPos", viewPos);
}
