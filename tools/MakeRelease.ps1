# =====================================================================================
#  제출용 배포 폴더 만들기
#  사용법 (저장소 맨 위 폴더에서 PowerShell):
#      .\tools\MakeRelease.ps1
#  결과:
#      dist\ZeroBrake\        실행 파일 + shaders + assets + glew32.dll + 실행방법.txt
#      dist\ZeroBrake.zip     위 폴더를 압축한 것 (제출용)
#  다른 PC 에서 dist\ZeroBrake\ZeroBrake.exe 를 더블클릭해 실행되는지 꼭 확인하기
# =====================================================================================
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

# ---- 1) Release x64 빌드 ----
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installation) { throw 'Visual Studio 2022 C++ 빌드 도구가 필요합니다.' }
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild (Join-Path $root 'ZeroBrake.sln') /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Release 빌드 실패' }

$out = Join-Path $root 'build\Release'
$exe = Join-Path $out 'ZeroBrake.exe'
if (-not (Test-Path $exe)) { throw "실행 파일이 없습니다: $exe" }

# ---- 2) 배포 폴더 구성 ----
$dist = Join-Path $root 'dist\ZeroBrake'
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item $exe $dist
Copy-Item (Join-Path $root 'ZeroBrake\shaders') (Join-Path $dist 'shaders') -Recurse
Copy-Item (Join-Path $root 'ZeroBrake\assets') (Join-Path $dist 'assets') -Recurse
Get-ChildItem (Join-Path $dist 'assets') -Recurse -Filter '.gitkeep' | Remove-Item -Force

# glew32.dll (빌드 폴더에 복사돼 있으면 그것, 없으면 PATH 에서 찾기)
$dllFound = $false
foreach ($dll in @('glew32.dll', 'glfw3.dll')) {
    $src = Join-Path $out $dll
    if (-not (Test-Path $src)) {
        $cmd = Get-Command $dll -ErrorAction SilentlyContinue
        if ($cmd) { $src = $cmd.Source }
    }
    if (Test-Path $src) { Copy-Item $src $dist; if ($dll -eq 'glew32.dll') { $dllFound = $true } }
}
if (-not $dllFound) { Write-Warning 'glew32.dll 을 찾지 못했습니다. 수업 OpenGL 설치 폴더에서 dist\ZeroBrake 에 직접 넣어 주세요.' }

@"
ZERO BRAKE — 2026-2 컴퓨터 그래픽스 최종 프로젝트
팀: 진우석, 유현조

[실행]
ZeroBrake.exe 를 실행 (shaders, assets 폴더와 glew32.dll 이 같은 폴더에 있어야 함)

[조작]
↑ / ↓      가속 / 브레이크·후진
← / →      조향
Space      핸드브레이크 (드리프트)
Shift      부스트
C          3인칭 ↔ 1인칭
M          미니맵 켜기/끄기
R          마지막 체크포인트로 복귀
P          일시정지 (일시정지 중 Esc = 타이틀)
Enter      선택 / 다음
Q          종료
"@ | Set-Content -Path (Join-Path $dist '실행방법.txt') -Encoding UTF8

# ---- 3) 압축 ----
$zip = Join-Path $root 'dist\ZeroBrake.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $dist -DestinationPath $zip
Write-Host "완료: $dist"
Write-Host "압축: $zip"
