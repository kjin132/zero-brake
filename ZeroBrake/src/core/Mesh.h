// =====================================================================================
//  [공동·고정] 메시(VAO/VBO) 만들기 + 기본 도형 + 그리기 도우미
//  - 기본 도형: 육면체(크기 1, 중심 원점), 원기둥(x축 방향, 지름 1, 길이 1), 평면(XZ, 위를 봄)
//  - 모든 도형은 위치·법선·텍스처 좌표를 가짐 → 조명/텍스처 셰이더로 그대로 그릴 수 있음
// =====================================================================================
#pragma once
#include "core/Common.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Mesh
{
	GLuint vao = 0, vbo = 0;
	GLsizei count = 0;
	GLenum mode = GL_TRIANGLES;
};

// 정점 배열로 메시 생성 (트랙 도로처럼 직접 만든 모양에 사용)
Mesh Mesh_Create(const std::vector<Vertex>& verts, GLenum mode = GL_TRIANGLES);
void Mesh_Destroy(Mesh& m);
void Mesh_InitPrimitives();   // main 에서 한 번 호출

extern Mesh gCube;      // 크기 1, 중심 원점
extern Mesh gCylinder;  // x축 방향, 지름 1, 길이 1, 중심 원점 (바퀴용)
extern Mesh gQuad;      // XZ 평면, 크기 1, 중심 원점, 법선 +y (바닥·도로 조각)

// ---- 그리기 (현재 셰이더 = basic) ----
struct DrawOpt
{
	glm::vec4 color = glm::vec4(1.0f);
	GLuint texture = 0;      // 0 = 텍스처 없음
	bool lit = true;         // false = 조명 없이 색 그대로 (HUD 등)
	float emissive = 0.0f;   // 0~1: 스스로 빛나는 정도 (전조등, 가로등, 밤 건물 창문)
	glm::vec2 uvScale = glm::vec2(1.0f);  // 텍스처 반복 횟수 (건물 창문 등)
};

void Draw_Mesh(const Mesh& mesh, const glm::mat4& model, const DrawOpt& opt);
// parent * T(pos) * S(size) 육면체 — 수업 drawBox 와 같은 사용법
void Draw_Box(const glm::mat4& parent, glm::vec3 pos, glm::vec3 size, glm::vec4 color);
void Draw_Cylinder(const glm::mat4& parent, glm::vec3 pos, glm::vec3 size, glm::vec4 color);
