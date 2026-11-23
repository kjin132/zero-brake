// [진우석] 조명·텍스처·반투명 처리 (render/Lighting.cpp 에서 uniform 설정)
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
out vec4 FragColor;

uniform vec4 uColor;          // rgb = 색, a = 투명도 (고스트·유리창)
uniform int  useLighting;     // 0이면 조명 없이 색 그대로 (HUD 등)
uniform int  useTexture;      // 1이면 텍스처 색 × uColor
uniform sampler2D tex;

uniform vec3 lightDir;        // 방향광(태양) 방향 — 빛이 "가는" 방향
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;

void main()
{
	vec4 base = uColor;
	if (useTexture == 1) base *= texture(tex, UV);

	if (useLighting == 0) { FragColor = base; return; }

	vec3 N = normalize(Normal);
	vec3 L = normalize(-lightDir);
	vec3 ambient = ambientStrength * lightColor;
	vec3 diffuse = max(dot(N, L), 0.0) * lightColor;

	vec3 V = normalize(viewPos - FragPos);
	vec3 R = reflect(-L, N);
	vec3 specular = 0.25 * pow(max(dot(V, R), 0.0), 32.0) * lightColor;

	FragColor = vec4((ambient + diffuse + specular) * base.rgb, base.a);
}
