// =====================================================================================
//  [담당: 진우석 주담당 / 유현조 렌더링 연동]
//  - 랩 동안 0.05초마다 (시간, 위치, 방향)을 저장 → 최고 랩이면 고스트 기록으로 교체
//  - 다음 랩에서 랩 경과 시간에 맞춰 두 샘플 사이를 보간해서 재생
//  - 고스트는 충돌·순위 계산에 넣지 않음
//  TODO(우석): 파일 저장(save/), 맵·차종별 기록 분리
// =====================================================================================
#include "race/Ghost.h"
#include "vehicle/CarModel.h"

struct GhostSample { float t; glm::vec3 pos; float yaw; };

static std::vector<GhostSample> sRecording;   // 이번 랩 기록 중
static std::vector<GhostSample> sBest;        // 최고 랩 기록
static float sBestTime = 0.0f;
static float sSampleTimer = 0.0f;
static int sMapId = -1;
static CarType sCarType = CarType::Balance;

static bool sVisible = false;
static glm::vec3 sPos;
static float sYaw = 0.0f;

void Ghost_StartRace(Game& g)
{
	// 맵이나 차종이 바뀌면 예전 기록은 쓰지 않음
	if (sMapId != g.settings.mapId || sCarType != g.settings.playerCar)
	{
		sBest.clear();
		sBestTime = 0.0f;
		sMapId = g.settings.mapId;
		sCarType = g.settings.playerCar;
	}
	sRecording.clear();
	sSampleTimer = 0.0f;
	sVisible = false;
}

void Ghost_Update(Game& g, float dt)
{
	const Car& p = g.cars[0];
	float lapT = g.race.time - g.race.lapStartTime;

	// 기록
	sSampleTimer += dt;
	if (sSampleTimer >= 0.05f)
	{
		sSampleTimer = 0.0f;
		sRecording.push_back({ lapT, p.pos, p.yaw });
	}

	// 재생 위치 계산
	sVisible = false;
	if (sBest.size() < 2 || lapT > sBest.back().t) return;
	for (size_t i = 0; i + 1 < sBest.size(); i++)
	{
		if (sBest[i + 1].t >= lapT)
		{
			float k = (lapT - sBest[i].t) / std::max(0.0001f, sBest[i + 1].t - sBest[i].t);
			sPos = glm::mix(sBest[i].pos, sBest[i + 1].pos, k);
			float dy = sBest[i + 1].yaw - sBest[i].yaw;
			while (dy > PI) dy -= 2 * PI;
			while (dy < -PI) dy += 2 * PI;
			sYaw = sBest[i].yaw + dy * k;
			sVisible = true;
			break;
		}
	}
}

void Ghost_OnPlayerLap(Game& g, float lapTime)
{
	if (sBestTime == 0.0f || lapTime < sBestTime)
	{
		sBest = sRecording;
		sBestTime = lapTime;
	}
	sRecording.clear();
	sSampleTimer = 0.0f;
}

void Ghost_Draw(const Game& g)
{
	if (!sVisible) return;
	glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), sPos), sYaw, glm::vec3(0, 1, 0));
	glDepthMask(GL_FALSE);                       // 반투명: 깊이 버퍼에 쓰지 않음
	CarModel_DrawAt(g.settings.playerCar, m, 0.0f, 0.0f, 0.35f);
	glDepthMask(GL_TRUE);
}
