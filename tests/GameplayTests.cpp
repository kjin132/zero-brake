#include "core/GameTypes.h"
#include "core/Mesh.h"
#include "vehicle/CarPhysics.h"
#include "vehicle/CarAbility.h"
#include "vehicle/CarSpecs.h"
#include "race/RaceRules.h"
#include "race/Collision.h"
#include "race/AIDriver.h"
#include "race/Ghost.h"
#include "world/Track.h"
#include "view/Camera.h"
#include "game/Game.h"
#include "core/Shader.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

int gWidth = 1280, gHeight = 720;
GLFWwindow* gWindow = nullptr;
static int checks = 0;
static void Check(bool value, const char* name)
{
    ++checks;
    if (!value) throw std::runtime_error(name);
    std::cout << "PASS " << name << '\n';
}
static Game Gates()
{
    Game g; g.carCount = 1; g.cars[0].active = true; g.cars[0].isPlayer = true;
    g.cars[0].pos = {0,0,-2};
    g.track.roadWidth = 20;
    g.track.center = {{0,0,-100},{0,0,100},{100,0,100},{100,0,-100}};
    g.track.checkpoints = {{{0,0,0},{0,0,1},10}, {{0,0,10},{0,0,1},10}, {{0,0,20},{0,0,1},10}};
    RaceRules_Start(g); Ghost_StartRace(g); return g;
}
static void StepTo(Game& g, glm::vec3 p)
{
    g.cars[0].pos = p; RaceRules_Update(g, 0.1f); Ghost_Update(g, 0.1f);
}
static void RulesTests()
{
    Game g = Gates(); StepTo(g, {0,0,2});
    Check(g.cars[0].lap == 0 && g.cars[0].startedLap && g.cars[0].nextCheckpoint == 1, "first start crossing does not count a lap");
    StepTo(g, {0,0,22});
    Check(g.cars[0].nextCheckpoint == 2, "checkpoints cannot be skipped");
    StepTo(g, {0,0,18});
    Check(g.cars[0].nextCheckpoint == 2, "reverse crossing rejected");
    StepTo(g, {0,0,22}); StepTo(g, {0,0,-2}); StepTo(g, {0,0,2});
    Check(g.cars[0].lap == 1 && g.race.bestLap > 0, "ordered forward lap accepted");
    Check(Ghost_BestTime(0, CarType::Balance) > 0, "valid lap saved");
    float best = Ghost_BestTime(0, CarType::Balance);
    g.settings.mapId = 1; Ghost_StartRace(g);
    Check(Ghost_BestTime(1, CarType::Balance) == 0, "different circuit has separate ghost");
    g.settings.mapId = 0; Ghost_StartRace(g);
    Check(Ghost_BestTime(0, CarType::Balance) == best, "switching circuits preserves records");

    g = Gates(); StepTo(g, {25,0,-1}); StepTo(g, {25,0,1});
    Check(!g.cars[0].startedLap, "crossing outside gate rejected");
    g = Gates(); g.settings.timeLimit = 0.05f; StepTo(g, {0,0,-1});
    Check(g.race.timeOver && RaceRules_IsOver(g), "time limit ends race");
    g = Gates(); g.cars[0].safePos = {0,0,-5}; g.cars[0].safeYaw = 0;
    g.cars[0].boostTime = g.cars[0].padBoostTime = 2;
    RaceRules_Respawn(g, g.cars[0]);
    Check(g.cars[0].speed == 0 && g.cars[0].boostTime == 0 && g.cars[0].pos.z == -5, "respawn clears motion and boost");
    g = Gates(); g.track.boostPads.push_back({0,0,-2}); g.cars[0].speed = 10;
    RaceRules_Update(g, 0.01f);
    for (int i = 0; i < 240; ++i) { CarAbility_Update(g.cars[0], {}, 0.01f); RaceRules_Update(g, 0.01f); }
    Check(g.cars[0].boostTime == 0, "standing on pad cannot refresh boost indefinitely");
}
static void AbilityTests()
{
    Car c; c.speed = 25; CarInput in; in.boost = true; in.throttle = 1;
    for (int i = 0; i < 400; ++i) CarAbility_Update(c, in, 0.01f);
    Check(c.boostGauge == 0 && c.boostTime == 0 && c.boostExhausted, "held Shift stops at empty gauge");
    CarAbility_Update(c, {}, 1);
    Check(c.boostGauge > 0 && !c.boostExhausted, "release allows gauge recharge");
    CarAbility_OnBoostPad(c); float gauge = c.boostGauge;
    for (int i = 0; i < 199; ++i) CarAbility_Update(c, {}, 0.01f);
    Check(c.boostTime > 0 && c.boostGauge >= gauge, "pad boost independent of gauge");
    CarAbility_Update(c, {}, 0.02f);
    Check(c.boostTime == 0 && CarAbility_MaxSpeedScale(c) == 1, "pad expires after two seconds");
    c.speed = 50; CarPhysics_Update(c, {}, 1.0f/120);
    Check(c.speed > CarSpec_Get(c.type).maxSpeed, "boost expiration decelerates smoothly");
    c.finished = true; c.speed = 0; in = {}; in.brake = 1;
    for (int i = 0; i < 100; ++i) CarPhysics_Update(c, in, 0.01f);
    Check(c.speed == 0, "finished cars never reverse");
    c.finished = false; c.speed = 20; in = {}; in.handbrake = true; in.steer = 1;
    CarAbility_Update(c, in, 0.1f);
    Check(c.handbrakeOn && c.slipAngle > 0 && c.speed < 20, "handbrake produces controlled slip and deceleration");
    for (int type = 0; type < 3; ++type)
    {
        Car fast; fast.isPlayer = true; fast.type = (CarType)type;
        CarInput throttle; throttle.throttle = 1;
        for (int i = 0; i < 4000; ++i) CarPhysics_Update(fast, throttle, 0.01f);
        Check(fast.speed <= CarSpec_Get(fast.type).maxSpeed + 0.001f, "held throttle respects normal speed limit");
        fast.boostTime = 2;
        for (int i = 0; i < 4000; ++i) CarPhysics_Update(fast, throttle, 0.01f);
        Check(fast.speed <= CarSpec_Get(fast.type).maxSpeed * 1.25f + 0.001f, "held throttle respects boosted speed limit");
    }
}
static void CollisionTests()
{
    Game g = Gates(); g.track.obstacles.push_back({{0,0,0},{1.4f,1,1.4f}});
    Car& c = g.cars[0]; c.pos = {0,0,-4}; c.speed = 65;
    CarInput in; in.throttle = 1;
    bool contained = true;
    for (int i = 0; i < 120; ++i)
    {
        CarPhysics_Update(c, in, 1.0f/120); Collision_Update(g);
        contained = contained && c.pos.z <= -2.69f;
    }
    Check(contained, "maximum boost speed cannot tunnel through obstacle");
    g = Gates(); c = g.cars[0];
    g.cars[0].pos = {9.9f,0,30}; g.cars[0].yaw = 0.8f;
    Collision_Update(g);
    float support = std::fabs(std::cos(0.8f)) + 2 * std::fabs(std::sin(0.8f));
    Check(g.cars[0].pos.x + support <= 10.01f, "rotated car footprint stays within road");
    g = Gates(); g.carCount = 2; g.cars[1] = g.cars[0];
    Collision_Update(g);
    Check(glm::length(g.cars[1].pos - g.cars[0].pos) > 1.9f, "coincident cars separate deterministically");
}
static void CircuitTests()
{
    for (int map = 0; map < 2; ++map)
    {
        Game g; g.settings.mapId = map; g.carCount = 3;
        Track_Build(g.track, map); g.settings.timeLimit = 300;
        for (int i = 0; i < 3; ++i)
        {
            Car& c = g.cars[i]; c.active = true; c.type = (CarType)i;
            CarPhysics_Reset(c, g.track, i); AIDriver_Init(g, c, i+1);
        }
        RaceRules_Start(g); Camera_Reset(g);
        float dt = 1.0f/120;
        int steps = 0;
        while (steps++ < 300 * 120)
        {
            for (int i = 0; i < 3; ++i)
            {
                CarInput in = g.cars[i].finished ? CarInput{} : AIDriver_Think(g, g.cars[i], dt);
                if (g.cars[i].finished) in.brake = 1;
                CarAbility_Update(g.cars[i], in, dt); CarPhysics_Update(g.cars[i], in, dt);
            }
            Collision_Update(g); RaceRules_Update(g, dt); Camera_Update(g, dt);
            bool done = true; for (int i = 0; i < 3; ++i) done = done && g.cars[i].finished;
            if (done) break;
        }
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "map=" << map << " car=" << i << " lap=" << g.cars[i].lap << " next=" << g.cars[i].nextCheckpoint
                << " time=" << g.cars[i].finishTime << " pos=" << g.cars[i].pos.x << ',' << g.cars[i].pos.z << '\n';
            Check(g.cars[i].finished, "AI finishes three laps within time limit");
        }
        CameraView camera = Camera_Main(g, 16.0f/9);
        Check(std::isfinite(camera.view[0][0]) && std::isfinite(camera.proj[0][0]), "camera stays finite through full race");
    }
}
static void Snapshot(Game& g, const std::string& name)
{
    Game_Draw(g); glFinish();
    std::vector<unsigned char> pixels(gWidth * gHeight * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, gWidth, gHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream out(name + ".ppm", std::ios::binary);
    out << "P6\n" << gWidth << ' ' << gHeight << "\n255\n";
    for (int y = gHeight - 1; y >= 0; --y)
        out.write((const char*)pixels.data() + y * gWidth * 3, gWidth * 3);
    Check(glGetError() == GL_NO_ERROR, name.c_str());
}
static void RenderTests()
{
    auto root = std::filesystem::path(__FILE__).parent_path().parent_path() / "ZeroBrake";
    // Resource paths are relative to the game directory. Record tests ran in an isolated cwd.
    auto previous = std::filesystem::current_path();
    std::filesystem::current_path(root);
    Check(Shader_Load("shaders/basic.vert", "shaders/basic.frag"), "game shaders compile");
    std::filesystem::current_path(previous);
    glfwSetWindowSize(gWindow, gWidth, gHeight); glfwPollEvents();
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Game g; Game_Init(g); Snapshot(g, "title");
    g.state = GameState::Select; Snapshot(g, "select");
    Game_StartRace(g); Snapshot(g, "countdown");
    g.state = GameState::Race; Snapshot(g, "race-high-speed");
    g.viewMode = ViewMode::FirstPerson; Snapshot(g, "first-person");
    g.settings.mapId = 1; Game_StartRace(g); g.state = GameState::Race; Snapshot(g, "race-technical");
    g.state = GameState::Result;
    for (int i = 0; i < g.carCount; ++i) { g.cars[i].rank = i+1; g.cars[i].finished = true; g.cars[i].finishTime = 90.0f+i*2; }
    g.race.time = 90; g.race.bestLap = 29;
    Snapshot(g, "results");
    gWidth = 800; gHeight = 600; glfwSetWindowSize(gWindow, gWidth, gHeight); glfwPollEvents();
    g.state = GameState::Select; Snapshot(g, "select-800x600");
}
int main(int argc, char**)
{
    try
    {
        std::cout << std::unitbuf;
        if (argc > 1)
        {
            Check(Ghost_BestTime(0, CarType::Balance) > 0, "ghost loaded after process restart");
            Check(Ghost_BestTime(1, CarType::Speed) == 0, "corrupt ghost ignored safely");
            return 0;
        }
        if (!glfwInit()) throw std::runtime_error("GLFW init");
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        gWindow = glfwCreateWindow(64,64,"Gameplay tests",nullptr,nullptr);
        if (!gWindow) throw std::runtime_error("hidden GL context");
        glfwMakeContextCurrent(gWindow); glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW init");
        Mesh_InitPrimitives();
        AbilityTests(); CollisionTests(); RulesTests(); CircuitTests(); RenderTests();
        { std::ofstream corrupt("save/ghost_4.txt"); corrupt << "ZB_GHOST_1 4 5 999999999"; }
        glfwDestroyWindow(gWindow); glfwTerminate();
        std::cout << checks << " checks passed\n"; return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
