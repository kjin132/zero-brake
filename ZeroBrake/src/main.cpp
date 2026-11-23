// =====================================================================================
//  [공동·고정] 프로그램 시작점 — 창 만들기, 메인 루프
//  게임 내용은 game/Game.cpp 와 각 담당 모듈에 있음. 이 파일은 수정하지 않는다.
// =====================================================================================
#include "core/Common.h"
#include "core/Shader.h"
#include "core/Mesh.h"
#include "core/Input.h"
#include "game/Game.h"
#include <iostream>

int gWidth = 1280, gHeight = 720;
GLFWwindow* gWindow = nullptr;

static void framebuffer_size_callback(GLFWwindow*, int w, int h)
{
	if (w == 0 || h == 0) return;   // 최소화
	gWidth = w; gHeight = h;
}

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	gWindow = glfwCreateWindow(gWidth, gHeight, "ZERO BRAKE", nullptr, nullptr);
	if (!gWindow) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(gWindow);
	glfwSwapInterval(1);
	glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) { std::cerr << "GLEW 초기화 실패\n"; return -1; }

	int fbw, fbh;
	glfwGetFramebufferSize(gWindow, &fbw, &fbh);
	framebuffer_size_callback(gWindow, fbw, fbh);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!Shader_Load("shaders/basic.vert", "shaders/basic.frag")) return -1;
	Mesh_InitPrimitives();

	static Game game;
	Game_Init(game);

	double last = glfwGetTime();
	while (!glfwWindowShouldClose(gWindow) && !game.quitRequested)
	{
		double now = glfwGetTime();
		float dt = (float)(now - last);
		last = now;
		if (dt > 0.05f) dt = 0.05f;   // 창 이동 등으로 멈췄다 돌아올 때 순간이동 방지

		glfwPollEvents();
		Input_Update();
		Game_Update(game, dt);
		Game_Draw(game);
		glfwSwapBuffers(gWindow);
	}

	glfwDestroyWindow(gWindow);
	glfwTerminate();
	return 0;
}
