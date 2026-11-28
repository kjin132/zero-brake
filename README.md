# ZERO BRAKE

2026-2 컴퓨터 그래픽스 최종 프로젝트 — 3D 아케이드 레이싱 게임 (C++ / OpenGL 3.3 / GLFW · GLEW · GLM)

| 팀원 | GitHub | 담당 |
| --- | --- | --- |
| 진우석 | kjin132 | 차량 조작·충돌·복귀, AI 주행·경기 규칙, 조명·텍스처·반투명, 고스트 |
| 유현조 | dbguswh | 차량·맵 모델, 핸드브레이크·부스트, 카메라·다중 뷰포트, 선택 화면·HUD·결과 화면 |

## 실행 방법

1. `ZeroBrake.sln` 을 Visual Studio 2022 로 연다.
2. 구성 `Debug | x64` 선택 후 F5.
   - 수업 컴그 프로젝트처럼 `GL/glew.h`, `GL/glfw3.h`, `gl/glm/` 과 `glew32.lib`, `glfw3.lib` 이 VS에 설치되어 있어야 함
   - 라이브러리 링크는 `src/core/Common.h` 의 `#pragma comment` 가 처리 → 프로젝트 설정 변경 불필요
3. 빌드 결과물은 `build/` 폴더에 생김 (git 에 안 올라감).
4. 제출용 Release 실행 파일은 `build/Release/` 에 생기고, 빌드 후 `shaders/`, `assets/`, `glew32.dll`(PATH 에서 찾음) 이 자동 복사됨.
5. **제출용 배포 폴더**: 저장소 맨 위에서 PowerShell 로 `.\tools\MakeRelease.ps1` → `dist\ZeroBrake\` 폴더와 `dist\ZeroBrake.zip` 생성 (Release 빌드 + 리소스 + DLL + 실행방법.txt)

### 조작

| 키 | 동작 |
| --- | --- |
| ↑ / ↓ | 가속 / 브레이크·후진 |
| ← / → | 좌 / 우 조향 |
| Space | 핸드브레이크 (드리프트) |
| Shift | 부스트 |
| C | 3인칭 ↔ 1인칭 |
| M | 미니맵 켜기/끄기 |
| R | 마지막 체크포인트로 복귀 |
| P | 일시정지 (일시정지 중 Esc = 타이틀) |
| ↑ ↓ ← → / Enter | 선택 화면 (차량 · 스테이지 · AI 수) |
| Q | 종료 |

## 폴더 구조와 담당

**자기 담당 파일만 수정하면 충돌이 나지 않도록** 기능별로 파일을 나눠 두었습니다. 모든 파일 맨 위 주석에 `[담당: 이름]` 이 적혀 있습니다.

```
ZeroBrake/
├─ shaders/basic.vert, basic.frag   셰이더 (frag = 진우석: 조명·텍스처·반투명)
├─ assets/textures/                 이미지 텍스처 넣는 곳
└─ src/
   ├─ main.cpp                      [공동·고정] 창, 메인 루프
   ├─ core/                         [공동·고정] 공통 도구
   │  ├─ Common.h                   include, 라이브러리 링크
   │  ├─ GameTypes.h                ★ 공유 데이터 구조 (Car, Track, Game ...) — 합의 후 수정
   │  ├─ Shader.*  Mesh.*  Input.*  셰이더 로드, 기본 도형(육면체·원기둥·평면), 키 입력
   ├─ game/Game.*                   [공동·고정] 상태 전환 + 모듈 호출 순서
   │
   ├─ vehicle/CarSpecs.*            진우석  차종별 성능 수치
   ├─ vehicle/CarPhysics.*          진우석  가속·제동·조향·바퀴 회전
   ├─ race/Collision.*              진우석  벽·차량 충돌
   ├─ race/RaceRules.*              진우석  체크포인트·랩·순위·제한 시간·복귀
   ├─ race/AIDriver.*               진우석  AI 경로 주행
   ├─ race/Ghost.*                  진우석  최고 랩 기록·고스트 재생
   ├─ render/Lighting.*             진우석  조명
   ├─ render/Texture.*              진우석  텍스처 (도로·잔디·체커·연석·건물 창문)
   ├─ render/Particles.*            진우석  부스트 배기 불꽃·연기, 드리프트 타이어 연기
   │
   ├─ vehicle/CarModel.*            유현조  차량 3종 외형
   ├─ vehicle/CarAbility.*          유현조  핸드브레이크·부스트
   ├─ world/Track.*                 유현조  트랙 2종 (중심선·도로·체크포인트 배치)
   ├─ world/Scenery.*               유현조  배경
   ├─ view/Camera.*                 유현조  추적·1인칭·미러·미니맵 카메라
   ├─ view/Viewports.*              유현조  다중 뷰포트
   └─ ui/SevenSeg.* Hud.* Screens.* BitmapText.*  유현조  7세그먼트 숫자, 한글 글자, HUD, 타이틀·선택·결과 화면
tests/                              자동 검사 (tests/README.md)
tools/MakeRelease.ps1               제출용 배포 폴더·zip 만들기
```

### 모듈이 연결되는 방식

`game/Game.cpp` 가 매 프레임 아래 순서로 각 담당 함수를 부릅니다. 이 연결은 이미 다 되어 있으므로 **각자 자기 함수 안만 채우면** 됩니다.

```
UpdateRace:  AIDriver_Think → CarAbility_Update → CarPhysics_Update     (차량 이동)
             → Collision_Update → RaceRules_Update → Ghost_Update → Camera_Update
Draw:        Viewports_Draw ─┬─ World_Draw(메인)   = Lighting → Scenery → Track → CarModel → Ghost → Particles
                             ├─ World_Draw(미러)
                             └─ World_Draw(미니맵)
             → Hud_DrawRace / Screens_Draw...
```

서로의 기능이 만나는 곳은 **함수 하나로만** 연결됩니다.

- 바퀴 회전·조향 각도: 진우석이 `Car.wheelSpin / steerAngle` 계산 → 유현조 `CarModel` 이 그대로 사용
- 핸드브레이크·부스트: 유현조 `CarAbility_*Scale()` 배율 → 진우석 `CarPhysics` 가 곱해서 사용
- 트랙 거리·가까운 점: 유현조 `Track_DistanceFromCenter / Track_NearestIndex` → 진우석 충돌·AI 가 사용
- 고스트 그리기: 진우석 `Ghost_Draw` 가 유현조 `CarModel_DrawAt` 을 반투명으로 호출

## 충돌 안 나게 작업하는 규칙

1. **작업 시작 전에 항상 pull** (GitHub Desktop: `Fetch origin` → `Pull origin`)
2. **자기 담당 파일만 수정**. 상대 파일에 고칠 게 있으면 말로 요청하거나 이슈로 남기기
3. **공동 파일**(`GameTypes.h`, `Game.*`, `main.cpp`, `core/`, `ZeroBrake.vcxproj`)은
   - 바꾸기 전에 상대에게 알리고, pull → 그 파일만 수정 → 바로 commit & push
   - `GameTypes.h` 는 필드를 **지우거나 이름을 바꾸지 말고 추가만**
4. **새 파일은 자기 폴더 안에 만들기만 하면 자동으로 빌드에 포함**됨 (vcxproj 가 `src\**\*.cpp` 를 자동 포함)
   - VS 솔루션 탐색기 위쪽의 **"모든 파일 표시"** 를 켜면 폴더 구조 그대로 보임
   - VS가 `.vcxproj.filters` 를 만들어도 git 에서 무시되므로 신경 쓰지 않아도 됨
5. **커밋은 작게, 자주**. 기능 하나 끝날 때마다 commit → pull → push
6. 빌드가 깨진 상태로 push 하지 않기 (F5 로 실행 확인 후 push)
7. 줄바꿈·인코딩은 `.gitattributes`, `.editorconfig` 가 자동으로 맞춰 줌 (UTF-8 BOM, CRLF, 탭 들여쓰기)

### 큰 기능은 브랜치로 (선택)

```
git checkout -b woo/collision      # 진우석
git checkout -b hyun/track-map2    # 유현조
... 작업 · 커밋 ...
GitHub 에서 Pull Request → 상대가 확인 → main 에 merge
```

## 현재 상태

- 처음부터 끝까지 한 판이 돌아가는 기본 버전 (타이틀 → 선택 → 카운트다운 → 3바퀴 → 결과)
- 각 파일의 `TODO(이름)` 주석이 앞으로 채울 부분
