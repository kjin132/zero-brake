// =====================================================================================
//  [공동·고정] 게임 전체 흐름 (상태 전환 + 각 담당 모듈 호출 순서)
//  - 이 파일은 처음에 모든 모듈 호출을 다 연결해 두었으므로 평소에는 수정할 일이 없음
//  - 기능 구현은 각자 담당 파일(vehicle/, race/, world/, view/, ui/, render/)에서
// =====================================================================================
#pragma once
#include "core/GameTypes.h"

struct CameraView;

void Game_Init(Game& g);                 // 프로그램 시작 시 한 번
void Game_Update(Game& g, float dt);     // 매 프레임 (dt = 지난 프레임부터 걸린 시간, 초)
void Game_Draw(Game& g);                 // 매 프레임 그리기

void Game_StartRace(Game& g);            // 선택 화면 → 카운트다운 (차량·트랙 생성)

// 3D 세계 한 번 그리기 — 뷰포트(메인/미러/미니맵)마다 Viewports 가 호출
void World_Draw(const Game& g, const CameraView& cam, bool drawPlayer = true);
