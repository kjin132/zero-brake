// =====================================================================================
//  [공동·고정] 모든 파일이 맨 위에 include 하는 공통 헤더
//  - 수업 컴그 프로젝트와 같은 include 경로 (GL/glew.h, GL/glfw3.h, gl/glm/...)
//  - 라이브러리 링크를 여기서 처리 → vcxproj 설정을 건드릴 필요 없음
//  - 이 파일은 되도록 수정하지 않는다. 필요한 include는 각자 자기 .cpp 에 추가.
// =====================================================================================
#pragma once

#include <GL/glew.h>        // 반드시 glfw3.h 보다 먼저
#include <GL/glfw3.h>

#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>

#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glfw3.lib")
#endif

constexpr float PI = 3.14159265f;

// 화면 크기 (main.cpp 의 창 크기 콜백에서 갱신)
extern int gWidth, gHeight;
extern GLFWwindow* gWindow;
