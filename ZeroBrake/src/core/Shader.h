// [공동·고정] 셰이더 로드 + uniform 설정 도우미
#pragma once
#include "core/Common.h"

// 셰이더 파일 두 개를 읽어 프로그램을 만들고 현재 프로그램으로 사용
GLuint Shader_Load(const char* vertFile, const char* fragFile);
GLuint Shader_Current();

void Shader_SetMat4(const char* name, const glm::mat4& m);
void Shader_SetVec3(const char* name, const glm::vec3& v);
void Shader_SetVec4(const char* name, const glm::vec4& v);
void Shader_SetFloat(const char* name, float f);
void Shader_SetInt(const char* name, int i);
