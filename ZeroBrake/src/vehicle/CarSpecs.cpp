// =====================================================================================
//  [담당: 진우석] 차종별 성능 수치
//  - 제안서: 밸런스형(고르게) / 스피드형(최고속도↑ 조향↓) / 코너형(가속·조향↑ 최고속도↓)
//  - 숫자는 직접 주행해 보며 조정. 외형(size, color)은 유현조와 맞춰서 정하기
// =====================================================================================
#include "vehicle/CarSpecs.h"

static const CarSpec kSpecs[(int)CarType::Count] = {
	//  name       accel brake maxSpd rev  steer grip  size(가로,높이,길이)        color
	{ "BALANCE",   14.0f, 26.0f, 42.0f, 10.0f, 95.0f, 0.90f, {2.0f, 1.3f, 4.0f}, {0.90f, 0.30f, 0.15f} },
	{ "SPEED",     12.0f, 24.0f, 52.0f, 10.0f, 75.0f, 0.85f, {2.0f, 1.1f, 4.6f}, {0.15f, 0.40f, 0.90f} },
	{ "CORNER",    17.0f, 30.0f, 36.0f, 10.0f, 120.0f, 0.95f, {1.8f, 1.3f, 3.4f}, {0.20f, 0.75f, 0.35f} },
};

const CarSpec& CarSpec_Get(CarType type)
{
	int i = glm::clamp((int)type, 0, (int)CarType::Count - 1);
	return kSpecs[i];
}
