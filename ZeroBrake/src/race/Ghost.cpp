// 맵/차종별 최고 랩 시간은 파일에 저장(HUD '최고'), 고스트는 이번 경기의 바로 이전 랩을 재생한다.
// 첫 랩에는 고스트 없음, 2랩부터 직전 랩이 같은 시간축으로 같이 달림.
#include "race/Ghost.h"
#include "vehicle/CarModel.h"
#include <array>
#include <filesystem>
#include <fstream>

namespace
{
struct Sample { float t; glm::vec3 pos; float yaw; };
struct Record { float time = 0; std::vector<Sample> samples; };
std::array<Record, 6> sRecords;
std::vector<Sample> sRecording;
std::vector<Sample> sPrevLap;      // 이번 경기 직전 랩 (고스트로 재생)
int sKey = 0;
bool sLoaded = false, sVisible = false, sValid = true;
float sSampleTimer = 0, sYaw = 0;
glm::vec3 sPos;
int Key(int map, CarType type) { return glm::clamp(map, 0, 1) * 3 + glm::clamp((int)type, 0, 2); }
std::filesystem::path SavePath(int key)
{
	// 실행 위치가 Debug/Release/IDE 중 어디든 같은 프로젝트 save 폴더를 사용한다.
	std::error_code ec;
	auto path = std::filesystem::current_path(ec);
	if (ec) return {};
	if (std::filesystem::exists(path / "ZeroBrake.sln", ec)) {}
	else if (std::filesystem::exists(path / "src", ec)) path = path.parent_path();
	else if (path.parent_path().filename() == "build") path = path.parent_path().parent_path();
	return path / "save" / ("ghost_" + std::to_string(key) + ".txt");
}
bool Finite(const Sample& s)
{
	return std::isfinite(s.t) && std::isfinite(s.pos.x) && std::isfinite(s.pos.y) &&
		std::isfinite(s.pos.z) && std::isfinite(s.yaw) && glm::length(s.pos) < 10000.0f;
}
void Load()
{
	if (sLoaded) return;
	sLoaded = true;
	for (int key = 0; key < 6; ++key)
	{
		std::ifstream in(SavePath(key));
		std::string version; int fileKey; size_t count; Record r;
		if (!(in >> version >> fileKey >> r.time >> count) || version != "ZB_GHOST_1" ||
			fileKey != key || !std::isfinite(r.time) || r.time <= 0 || r.time > 3600 || count < 2 || count > 100000) continue;
		bool valid = true;
		for (size_t i = 0; i < count; ++i)
		{
			Sample s;
			if (!(in >> s.t >> s.pos.x >> s.pos.y >> s.pos.z >> s.yaw) || !Finite(s) ||
				s.t < 0 || s.t > r.time + 0.001f || (i && s.t <= r.samples.back().t)) { valid = false; break; }
			r.samples.push_back(s);
		}
		if (valid && r.samples.front().t == 0 && std::fabs(r.samples.back().t - r.time) < 0.001f)
			sRecords[key] = std::move(r);
	}
}
void Save(int key)
{
	const Record& r = sRecords[key];
	auto path = SavePath(key); if (path.empty()) return;
	std::error_code ec; std::filesystem::create_directories(path.parent_path(), ec);
	if (ec) return;
	auto temporary = path; temporary += ".tmp";
	{
		std::ofstream out(temporary); if (!out) return;
		out.precision(9);
		out << "ZB_GHOST_1 " << key << ' ' << r.time << ' ' << r.samples.size() << '\n';
		for (const Sample& s : r.samples)
			out << s.t << ' ' << s.pos.x << ' ' << s.pos.y << ' ' << s.pos.z << ' ' << s.yaw << '\n';
		out.flush(); if (!out) return;
	}
	// Windows의 기존 파일 rename 제약을 피하고 완성된 임시 파일을 복사한다.
	std::filesystem::copy_file(temporary, path, std::filesystem::copy_options::overwrite_existing, ec);
	if (!ec) std::filesystem::remove(temporary, ec);
}
}
float Ghost_BestTime(int mapId, CarType type) { Load(); return sRecords[Key(mapId, type)].time; }
void Ghost_StartRace(Game& g)
{
	Load(); sKey = Key(g.settings.mapId, g.settings.playerCar);
	sRecording.clear(); sPrevLap.clear(); sVisible = false; sSampleTimer = 0; sValid = true;
}
void Ghost_BeginLap(const Game& g)
{
	sRecording.clear(); sRecording.push_back({ 0, g.cars[0].pos, g.cars[0].yaw });
	sSampleTimer = 0; sValid = true;
}
void Ghost_InvalidateLap() { sValid = false; }
void Ghost_Update(Game& g, float dt)
{
	const Car& p = g.cars[0];
	float t = g.race.time - g.race.lapStartTime;
	sVisible = false;
	if (!p.startedLap || p.finished) return;
	sSampleTimer += dt;
	if (sValid && sSampleTimer >= 0.05f && !sRecording.empty() && t > sRecording.back().t)
	{
		sSampleTimer = std::fmod(sSampleTimer, 0.05f);
		sRecording.push_back({ t, p.pos, p.yaw });
	}
	const auto& best = sPrevLap;
	if (best.size() < 2 || t > best.back().t) return;
	auto next = std::upper_bound(best.begin(), best.end(), t, [](float time, const Sample& s) { return time < s.t; });
	if (next == best.begin()) return;
	if (next == best.end()) --next;
	const Sample& a = *(next - 1); const Sample& b = *next;
	float k = glm::clamp((t - a.t) / std::max(0.0001f, b.t - a.t), 0.0f, 1.0f);
	sPos = glm::mix(a.pos, b.pos, k);
	float dy = std::remainder(b.yaw - a.yaw, 2 * PI);
	sYaw = a.yaw + dy * k; sVisible = true;
}
void Ghost_OnPlayerLap(Game& g, float lapTime)
{
	sPrevLap.clear();                    // 리스폰 등으로 망가진 랩이면 다음 랩엔 고스트 없음
	if (sValid && !sRecording.empty() && lapTime > sRecording.back().t && g.race.time <= g.settings.timeLimit)
	{
		sRecording.push_back({ lapTime, g.cars[0].pos, g.cars[0].yaw });
		sPrevLap = sRecording;
		Record& best = sRecords[sKey];
		if (best.time == 0 || lapTime < best.time)
		{
			best.time = lapTime; best.samples = sRecording; Save(sKey);
		}
	}
	Ghost_BeginLap(g);
}
bool Ghost_Visible() { return sVisible; }
void Ghost_Draw(const Game& g)
{
	if (!sVisible) return;
	glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), sPos), sYaw, glm::vec3(0, 1, 0));
	glDepthMask(GL_FALSE);
	CarModel_DrawAt(g.settings.playerCar, m, 0, 0, 0.3f);
	glDepthMask(GL_TRUE);
}
