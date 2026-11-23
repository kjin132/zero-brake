// [담당: 유현조] 트랙 2종 — 중심선, 도로 메시, 체크포인트·부스트 패드 배치
#pragma once
#include "core/GameTypes.h"

void Track_Build(Track& t, int mapId);   // mapId 0 = 고속 서킷, 1 = 테크니컬 서킷
void Track_Draw(const Track& t);

// ---- 다른 모듈이 쓰는 질의 함수 (충돌·AI·미니맵) ----
int Track_NearestIndex(const Track& t, glm::vec3 p);   // 가장 가까운 중심선 점 번호
// p 에서 중심선까지 수평 거리. nearest = 중심선 위 가장 가까운 점, tangent = 그 지점 진행 방향
float Track_DistanceFromCenter(const Track& t, glm::vec3 p, glm::vec3* nearest, glm::vec3* tangent);
