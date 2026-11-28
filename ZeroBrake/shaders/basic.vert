// [공동] 기본 버텍스 셰이더 — 위치 / 법선 / 텍스처 좌표
#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 uvScale;         // 텍스처 반복 (기본 1,1)

out vec3 FragPos;
out vec3 Normal;
out vec2 UV;

void main()
{
	vec4 world = model * vec4(vPos, 1.0);
	gl_Position = projection * view * world;
	FragPos = world.xyz;
	Normal = mat3(transpose(inverse(model))) * vNormal;
	UV = vUV * uvScale;
}
