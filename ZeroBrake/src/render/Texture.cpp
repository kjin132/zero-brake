// =====================================================================================
//  [담당: 진우석] 텍스처
//  - 외부 라이브러리 없이 바로 쓸 수 있게 코드로 무늬(아스팔트, 잔디, 체커, 연석)를 생성
//  TODO(우석): 이미지 파일을 쓰려면 stb_image.h 를 src/render/ 에 넣고 Texture_LoadFile 추가
//              (assets/textures/ 에 이미지 저장)
// =====================================================================================
#include "render/Texture.h"
#include <random>

static GLuint sTex[(int)TexId::Count] = {};

static GLuint Upload(int w, int h, const std::vector<unsigned char>& rgb)
{
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return id;
}

// 기본색 + 노이즈
static GLuint MakeNoise(int size, glm::vec3 base, float noise, unsigned seed)
{
	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> d(-noise, noise);
	std::vector<unsigned char> px(size * size * 3);
	for (int i = 0; i < size * size; i++)
	{
		float n = d(rng);
		for (int c = 0; c < 3; c++)
			px[i * 3 + c] = (unsigned char)(glm::clamp(base[c] + n, 0.0f, 1.0f) * 255);
	}
	return Upload(size, size, px);
}

// 두 색 체커 (결승선, 연석)
static GLuint MakeChecker(int size, int cells, glm::vec3 a, glm::vec3 b)
{
	std::vector<unsigned char> px(size * size * 3);
	int cs = size / cells;
	for (int y = 0; y < size; y++)
		for (int x = 0; x < size; x++)
		{
			glm::vec3 c = (((x / cs) + (y / cs)) % 2 == 0) ? a : b;
			for (int k = 0; k < 3; k++) px[(y * size + x) * 3 + k] = (unsigned char)(c[k] * 255);
		}
	return Upload(size, size, px);
}

void Texture_Init()
{
	sTex[(int)TexId::Road] = MakeNoise(64, glm::vec3(0.32f), 0.05f, 1);
	sTex[(int)TexId::Grass] = MakeNoise(64, glm::vec3(0.30f, 0.55f, 0.25f), 0.07f, 2);
	sTex[(int)TexId::Checker] = MakeChecker(64, 8, glm::vec3(1.0f), glm::vec3(0.05f));
	sTex[(int)TexId::Curb] = MakeChecker(64, 2, glm::vec3(0.85f, 0.1f, 0.1f), glm::vec3(0.95f));
}

GLuint Texture_Get(TexId id) { return sTex[(int)id]; }
