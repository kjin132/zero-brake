// 조명·텍스처·반투명·안개 처리 (render/Lighting.cpp 에서 빛 uniform 설정)
//  - 방향광(태양/달) 1개 + 점광원(가로등) 최대 8개 + 스포트라이트(차량 전조등) 최대 4개
//  - 퐁 조명: 주변광 + 난반사 + 정반사
//  - emissive: 스스로 빛나는 정도 (전조등 유리, 가로등 갓, 밤 건물 창문)
//  - 안개: 멀리 있는 물체가 하늘색으로 흐려짐 (원근감)
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
out vec4 FragColor;

uniform vec4 uColor;          // rgb = 색, a = 투명도 (고스트·유리창·연기)
uniform int  useLighting;     // 0이면 조명 없이 색 그대로 (HUD 등)
uniform int  useTexture;      // 1이면 텍스처 색 × uColor
uniform sampler2D tex;
uniform float emissive;

uniform vec3 lightDir;        // 방향광 방향 — 빛이 "가는" 방향
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;

#define MAX_POINT 8
uniform int  pointCount;
uniform vec3 pointPos[MAX_POINT];
uniform vec3 pointColor[MAX_POINT];

#define MAX_SPOT 4
uniform int  spotCount;
uniform vec3 spotPos[MAX_SPOT];
uniform vec3 spotDir[MAX_SPOT];
uniform vec3 spotColor;
uniform float spotInnerCos;
uniform float spotOuterCos;

uniform int  fogOn;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

// 퐁 반사 (난반사 + 정반사)
vec3 Phong(vec3 N, vec3 L, vec3 V, vec3 color, float shininess, float specStrength)
{
	float diff = max(dot(N, L), 0.0);
	vec3 R = reflect(-L, N);
	float spec = diff > 0.0 ? pow(max(dot(V, R), 0.0), shininess) : 0.0;
	return (diff + specStrength * spec) * color;
}

void main()
{
	vec4 base = uColor;
	if (useTexture == 1) base *= texture(tex, UV);

	if (useLighting == 0) { FragColor = base; return; }

	vec3 N = normalize(Normal);
	vec3 V = normalize(viewPos - FragPos);

	// 방향광
	vec3 light = ambientStrength * lightColor;
	light += Phong(N, normalize(-lightDir), V, lightColor, 32.0, 0.25);

	// 점광원 (거리 감쇠)
	for (int i = 0; i < pointCount; i++)
	{
		vec3 toL = pointPos[i] - FragPos;
		float d = length(toL);
		float att = 1.0 / (1.0 + 0.09 * d + 0.012 * d * d);
		light += att * Phong(N, toL / d, V, pointColor[i], 16.0, 0.4);
	}

	// 스포트라이트 (원뿔 안쪽만, 가장자리는 부드럽게)
	for (int i = 0; i < spotCount; i++)
	{
		vec3 toL = spotPos[i] - FragPos;
		float d = length(toL);
		vec3 L = toL / d;
		float theta = dot(-L, normalize(spotDir[i]));
		float cone = clamp((theta - spotOuterCos) / (spotInnerCos - spotOuterCos), 0.0, 1.0);
		float att = 1.0 / (1.0 + 0.045 * d + 0.0045 * d * d);
		light += cone * att * Phong(N, L, V, spotColor, 24.0, 0.3);
	}

	vec3 color = light * base.rgb + emissive * base.rgb;

	if (fogOn == 1)
	{
		float dist = length(viewPos - FragPos);
		float f = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
		color = mix(color, fogColor, f);
	}
	FragColor = vec4(color, base.a);
}
