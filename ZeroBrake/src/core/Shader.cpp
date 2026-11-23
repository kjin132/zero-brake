// [공동·고정] 셰이더 로드 + uniform 설정 도우미
#include "core/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

static GLuint sProgram = 0;

static std::string ReadFile(const char* file)
{
	std::ifstream in(file);
	if (!in.is_open())
	{
		std::cerr << "셰이더 파일 없음: " << file << " (작업 폴더에 shaders 폴더가 있는지 확인)\n";
		return "";
	}
	std::stringstream ss;
	ss << in.rdbuf();
	std::string src = ss.str();
	if (src.size() >= 3 && (unsigned char)src[0] == 0xEF && (unsigned char)src[1] == 0xBB && (unsigned char)src[2] == 0xBF)
		src.erase(0, 3);   // UTF-8 BOM 제거 (있으면 셰이더 컴파일 에러)
	return src;
}

static GLuint Compile(GLenum type, const char* file)
{
	std::string src = ReadFile(file);
	const char* s = src.c_str();
	GLuint sh = glCreateShader(type);
	glShaderSource(sh, 1, &s, nullptr);
	glCompileShader(sh);
	GLint ok = 0;
	glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		GLchar log[1024];
		glGetShaderInfoLog(sh, 1024, nullptr, log);
		std::cerr << file << " 컴파일 실패\n" << log << "\n";
	}
	return sh;
}

GLuint Shader_Load(const char* vertFile, const char* fragFile)
{
	GLuint vs = Compile(GL_VERTEX_SHADER, vertFile);
	GLuint fs = Compile(GL_FRAGMENT_SHADER, fragFile);
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
	GLint ok = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		GLchar log[1024];
		glGetProgramInfoLog(prog, 1024, nullptr, log);
		std::cerr << "셰이더 링크 실패\n" << log << "\n";
		return 0;
	}
	glUseProgram(prog);
	sProgram = prog;
	return prog;
}

GLuint Shader_Current() { return sProgram; }

static GLint Loc(const char* name) { return glGetUniformLocation(sProgram, name); }

void Shader_SetMat4(const char* name, const glm::mat4& m) { glUniformMatrix4fv(Loc(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader_SetVec3(const char* name, const glm::vec3& v) { glUniform3fv(Loc(name), 1, glm::value_ptr(v)); }
void Shader_SetVec4(const char* name, const glm::vec4& v) { glUniform4fv(Loc(name), 1, glm::value_ptr(v)); }
void Shader_SetFloat(const char* name, float f) { glUniform1f(Loc(name), f); }
void Shader_SetInt(const char* name, int i) { glUniform1i(Loc(name), i); }
