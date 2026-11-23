// =====================================================================================
//  [담당: 유현조] 차량 3종 외형
//  - 차 행렬(크기 없음)을 부모로, 부품마다 Draw_Box / Draw_Cylinder 로 그림
//  - 모델 좌표: +z = 앞, +y = 위, 바닥 y = 0
//  TODO(현조): 밸런스형 / 스피드형(낮고 긴 스포츠카) / 코너형(짧고 작은 차체) 모양 다르게
// =====================================================================================
#include "vehicle/CarModel.h"
#include "vehicle/CarSpecs.h"
#include "vehicle/CarPhysics.h"
#include "core/Mesh.h"

void CarModel_DrawAt(CarType type, const glm::mat4& M, float steerAngle, float wheelSpin, float alpha)
{
	const CarSpec& s = CarSpec_Get(type);
	glm::vec3 sz = s.size;                   // 가로, 높이, 길이
	glm::vec4 body(s.color, alpha);
	glm::vec4 dark(s.color * 0.6f, alpha);
	glm::vec4 glass(0.55f, 0.8f, 0.95f, alpha * 0.6f);
	glm::vec4 tire(0.1f, 0.1f, 0.1f, alpha);

	float bodyH = sz.y * 0.45f;
	float wheelR = 0.35f;

	// 차체 / 캐빈 / 앞유리 / 스포일러
	Draw_Box(M, { 0, wheelR + bodyH * 0.5f, 0 }, { sz.x, bodyH, sz.z }, body);
	Draw_Box(M, { 0, wheelR + bodyH + sz.y * 0.2f, -sz.z * 0.08f }, { sz.x * 0.8f, sz.y * 0.4f, sz.z * 0.45f }, dark);
	Draw_Box(M, { 0, wheelR + bodyH + sz.y * 0.2f, sz.z * 0.15f }, { sz.x * 0.75f, sz.y * 0.32f, 0.05f }, glass);
	Draw_Box(M, { 0, wheelR + bodyH + 0.25f, -sz.z * 0.47f }, { sz.x, 0.06f, 0.35f }, tire);

	// 바퀴 4개: 앞바퀴만 조향, 모두 회전
	float wx = sz.x * 0.5f, wz = sz.z * 0.32f;
	for (int i = 0; i < 4; i++)
	{
		float x = (i % 2 == 0) ? -wx : wx;
		float z = (i < 2) ? wz : -wz;
		glm::mat4 w = glm::translate(M, { x, wheelR, z });
		if (i < 2) w = glm::rotate(w, glm::radians(-steerAngle), glm::vec3(0, 1, 0));  // 앞바퀴 조향
		w = glm::rotate(w, glm::radians(wheelSpin), glm::vec3(1, 0, 0));               // 굴러감
		Draw_Cylinder(w, glm::vec3(0), { 0.3f, wheelR * 2.0f, wheelR * 2.0f }, tire);
	}
}

void CarModel_Draw(const Car& car)
{
	CarModel_DrawAt(car.type, Car_Matrix(car), car.steerAngle, car.wheelSpin, 1.0f);
}
