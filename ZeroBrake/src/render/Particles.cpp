// =====================================================================================
//  파티클 효과
//  - 부스트 중: 배기구 2곳에서 주황 불꽃(짧게) + 회색 연기(길게)
//  - 핸드브레이크 드리프트 중: 뒷바퀴에서 흰 타이어 연기
//  - 입자는 위로 떠오르며 커지고 투명해짐 → 수명이 끝나면 제거
//  - 그리기: 카메라에서 먼 것부터 (반투명 정렬), 깊이 버퍼에는 쓰지 않음
// =====================================================================================
#include "render/Particles.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarSpecs.h"
#include "core/Mesh.h"
#include <random>

struct Particle
{
	glm::vec3 pos, vel;
	float life, maxLife;
	float size0, size1;     // 처음 크기 → 마지막 크기
	glm::vec3 color;
	float alpha0;
	bool glow;              // true = 조명 무시 (불꽃)
};

static const int MAX_PARTICLES = 600;
static std::vector<Particle> sParts;
static std::mt19937 sRng(1234);
static float sEmitAcc[MAX_CARS][2];   // 차마다 [배기, 타이어] 생성 누적 시간
static float sSmokeShade = 0.55f;      // 연기 밝기 (밤 맵은 어둡게)

static float Rand(float a, float b) { std::uniform_real_distribution<float> d(a, b); return d(sRng); }

void Particles_Clear()
{
	sParts.clear();
	for (auto& a : sEmitAcc) a[0] = a[1] = 0.0f;
}

int Particles_Count() { return (int)sParts.size(); }

static void Spawn(const Particle& p)
{
	if ((int)sParts.size() >= MAX_PARTICLES) return;
	sParts.push_back(p);
}

static void EmitExhaust(const Car& c, int idx, float dt)
{
	const CarSpec& s = CarSpec_Get(c.type);
	glm::vec3 fwd = Car_Forward(c), right = Car_Right(c);
	sEmitAcc[idx][0] += dt;
	const float interval = 1.0f / 50.0f;
	while (sEmitAcc[idx][0] >= interval)
	{
		sEmitAcc[idx][0] -= interval;
		for (float side : { -1.0f, 1.0f })
		{
			glm::vec3 pipe = c.pos - fwd * (s.size.z * 0.5f + 0.1f) + right * side * (s.size.x * 0.22f) + glm::vec3(0, 0.55f, 0);
			glm::vec3 back = -fwd * Rand(3.0f, 6.0f) + glm::vec3(Rand(-0.6f, 0.6f), Rand(0.3f, 1.0f), Rand(-0.6f, 0.6f));
			// 불꽃 (짧고 밝게)
			Spawn({ pipe, back + fwd * c.speed * 0.6f, 0.0f, Rand(0.12f, 0.2f), 0.35f, 0.15f,
				glm::mix(glm::vec3(1.0f, 0.85f, 0.3f), glm::vec3(1.0f, 0.35f, 0.05f), Rand(0.0f, 1.0f)), 0.9f, true });
			// 연기 (느리게 퍼짐)
			Spawn({ pipe, back * 0.5f + fwd * c.speed * 0.3f, 0.0f, Rand(0.5f, 0.8f), 0.25f, 0.9f,
				glm::vec3(sSmokeShade * Rand(0.85f, 1.1f)), 0.22f, false });
		}
	}
}

static void EmitTireSmoke(const Car& c, int idx, float dt)
{
	const CarSpec& s = CarSpec_Get(c.type);
	glm::vec3 fwd = Car_Forward(c), right = Car_Right(c);
	sEmitAcc[idx][1] += dt;
	const float interval = 1.0f / 30.0f;
	while (sEmitAcc[idx][1] >= interval)
	{
		sEmitAcc[idx][1] -= interval;
		for (float side : { -1.0f, 1.0f })
		{
			glm::vec3 wheel = c.pos - fwd * (s.size.z * 0.32f) + right * side * (s.size.x * 0.5f) + glm::vec3(0, 0.25f, 0);
			glm::vec3 v(Rand(-1.0f, 1.0f), Rand(0.4f, 1.2f), Rand(-1.0f, 1.0f));
			Spawn({ wheel, v + fwd * c.speed * 0.15f, 0.0f, Rand(0.6f, 1.0f), 0.35f, 1.5f, glm::vec3(sSmokeShade * 1.6f), 0.25f, false });
		}
	}
}

void Particles_Update(const Game& g, float dt)
{
	sSmokeShade = g.settings.mapId == 1 ? 0.32f : 0.55f;
	for (int i = 0; i < g.carCount && i < MAX_CARS; i++)
	{
		const Car& c = g.cars[i];
		if (!c.active) continue;
		if (c.boostTime > 0.0f || c.padBoostTime > 0.0f) EmitExhaust(c, i, dt);
		else sEmitAcc[i][0] = 0.0f;
		if (c.handbrakeOn && std::fabs(c.speed) > 6.0f) EmitTireSmoke(c, i, dt);
		else sEmitAcc[i][1] = 0.0f;
	}

	for (Particle& p : sParts)
	{
		p.life += dt;
		p.pos += p.vel * dt;
		p.vel *= std::exp(-2.5f * dt);          // 공기 저항
		p.vel.y += (p.glow ? 0.0f : 0.8f) * dt; // 연기는 위로 떠오름
	}
	sParts.erase(std::remove_if(sParts.begin(), sParts.end(), [](const Particle& p) { return p.life >= p.maxLife; }), sParts.end());
}

void Particles_Draw(glm::vec3 viewPos)
{
	if (sParts.empty()) return;
	std::vector<const Particle*> order;
	order.reserve(sParts.size());
	for (const Particle& p : sParts) order.push_back(&p);
	std::sort(order.begin(), order.end(), [&](const Particle* a, const Particle* b) {
		return glm::length(a->pos - viewPos) > glm::length(b->pos - viewPos);   // 먼 것부터
	});

	glDepthMask(GL_FALSE);
	for (const Particle* p : order)
	{
		float t = p->life / p->maxLife;
		float size = glm::mix(p->size0, p->size1, t);
		DrawOpt o;
		o.color = glm::vec4(p->color, p->alpha0 * (1.0f - t));
		o.lit = false;                       // 연기는 밝기를 sSmokeShade 로 직접 정함 (조명 받으면 각진 면이 도드라짐)
		glm::mat4 m = glm::translate(glm::mat4(1.0f), p->pos);
		m = glm::rotate(m, p->size0 * 7.0f + t * 2.0f, glm::vec3(0.3f, 1.0f, 0.2f));   // 입자마다 조금씩 다른 각도
		Draw_Mesh(gCube, glm::scale(m, glm::vec3(size)), o);
	}
	glDepthMask(GL_TRUE);
}
