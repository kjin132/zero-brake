// [공동·고정] 메시(VAO/VBO) 만들기 + 기본 도형 + 그리기 도우미
#include "core/Mesh.h"
#include "core/Shader.h"

Mesh gCube, gCylinder, gQuad;

Mesh Mesh_Create(const std::vector<Vertex>& verts, GLenum mode)
{
	Mesh m;
	m.count = (GLsizei)verts.size();
	m.mode = mode;
	glGenVertexArrays(1, &m.vao);
	glBindVertexArray(m.vao);
	glGenBuffers(1, &m.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
	return m;
}

void Mesh_Destroy(Mesh& m)
{
	if (m.vbo) glDeleteBuffers(1, &m.vbo);
	if (m.vao) glDeleteVertexArrays(1, &m.vao);
	m = Mesh();
}

// 사각형 a b c d (바깥에서 볼 때 반시계) → 삼각형 2개
static void AddQuad(std::vector<Vertex>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n)
{
	v.push_back({ a, n, {0, 0} }); v.push_back({ b, n, {1, 0} }); v.push_back({ c, n, {1, 1} });
	v.push_back({ a, n, {0, 0} }); v.push_back({ c, n, {1, 1} }); v.push_back({ d, n, {0, 1} });
}

static Mesh MakeCube()
{
	std::vector<Vertex> v;
	const float h = 0.5f;
	glm::vec3 p[8] = {
		{-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h},   // 앞 (+z)
		{-h,-h,-h}, { h,-h,-h}, { h, h,-h}, {-h, h,-h}    // 뒤 (-z)
	};
	AddQuad(v, p[0], p[1], p[2], p[3], { 0, 0, 1 });   // 앞
	AddQuad(v, p[1], p[5], p[6], p[2], { 1, 0, 0 });   // 오른쪽
	AddQuad(v, p[5], p[4], p[7], p[6], { 0, 0,-1 });   // 뒤
	AddQuad(v, p[4], p[0], p[3], p[7], {-1, 0, 0 });   // 왼쪽
	AddQuad(v, p[3], p[2], p[6], p[7], { 0, 1, 0 });   // 위
	AddQuad(v, p[4], p[5], p[1], p[0], { 0,-1, 0 });   // 아래
	return Mesh_Create(v);
}

static Mesh MakeCylinder(int seg = 20)
{
	std::vector<Vertex> v;
	const float r = 0.5f, hx = 0.5f;
	for (int i = 0; i < seg; i++)
	{
		float a0 = 2.0f * PI * i / seg, a1 = 2.0f * PI * (i + 1) / seg;
		glm::vec3 d0(0, std::cos(a0), std::sin(a0)), d1(0, std::cos(a1), std::sin(a1));
		glm::vec3 L0 = glm::vec3(-hx, 0, 0) + d0 * r, L1 = glm::vec3(-hx, 0, 0) + d1 * r;
		glm::vec3 R0 = glm::vec3(hx, 0, 0) + d0 * r, R1 = glm::vec3(hx, 0, 0) + d1 * r;
		float u0 = (float)i / seg, u1 = (float)(i + 1) / seg;
		// 옆면
		v.push_back({ L0, d0, {u0, 0} }); v.push_back({ R1, d1, {u1, 1} }); v.push_back({ R0, d0, {u0, 1} });
		v.push_back({ L0, d0, {u0, 0} }); v.push_back({ L1, d1, {u1, 0} }); v.push_back({ R1, d1, {u1, 1} });
		// 양쪽 뚜껑
		v.push_back({ {hx, 0, 0}, {1, 0, 0}, {0.5f, 0.5f} }); v.push_back({ R0, {1, 0, 0}, {0, 0} }); v.push_back({ R1, {1, 0, 0}, {1, 0} });
		v.push_back({ {-hx, 0, 0}, {-1, 0, 0}, {0.5f, 0.5f} }); v.push_back({ L1, {-1, 0, 0}, {1, 0} }); v.push_back({ L0, {-1, 0, 0}, {0, 0} });
	}
	return Mesh_Create(v);
}

static Mesh MakeQuad()
{
	std::vector<Vertex> v;
	const float h = 0.5f;
	AddQuad(v, { -h, 0, h }, { h, 0, h }, { h, 0, -h }, { -h, 0, -h }, { 0, 1, 0 });
	return Mesh_Create(v);
}

void Mesh_InitPrimitives()
{
	gCube = MakeCube();
	gCylinder = MakeCylinder();
	gQuad = MakeQuad();
}

void Draw_Mesh(const Mesh& mesh, const glm::mat4& model, const DrawOpt& opt)
{
	Shader_SetMat4("model", model);
	Shader_SetVec4("uColor", opt.color);
	Shader_SetInt("useLighting", opt.lit ? 1 : 0);
	if (opt.texture)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, opt.texture);
		Shader_SetInt("tex", 0);
		Shader_SetInt("useTexture", 1);
	}
	else Shader_SetInt("useTexture", 0);

	glBindVertexArray(mesh.vao);
	glDrawArrays(mesh.mode, 0, mesh.count);
}

void Draw_Box(const glm::mat4& parent, glm::vec3 pos, glm::vec3 size, glm::vec4 color)
{
	glm::mat4 m = glm::scale(glm::translate(parent, pos), size);
	DrawOpt o; o.color = color;
	Draw_Mesh(gCube, m, o);
}

void Draw_Cylinder(const glm::mat4& parent, glm::vec3 pos, glm::vec3 size, glm::vec4 color)
{
	glm::mat4 m = glm::scale(glm::translate(parent, pos), size);
	DrawOpt o; o.color = color;
	Draw_Mesh(gCylinder, m, o);
}
