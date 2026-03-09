#include "App.h"
#include "Auth.h"
#include "../scene/SceneLoader.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstdio>

App::App(AppConfig cfg) : m_cfg(std::move(cfg)) {}
App::~App() { shutdown(); }

void App::init() {
    printf("[Client] init start\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        throw std::runtime_error(SDL_GetError());

    m_window = SDL_CreateWindow(
        ("Crumblt - " + m_cfg.gameId).c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        m_cfg.width, m_cfg.height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) throw std::runtime_error(SDL_GetError());

    m_sdlRenderer = SDL_CreateRenderer(m_window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_sdlRenderer) throw std::runtime_error(SDL_GetError());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_sdlRenderer);
    ImGui_ImplSDLRenderer2_Init(m_sdlRenderer);

    if (!m_cfg.session.empty())
        Auth::saveSession(m_cfg.session);

    // Fetch username
    try {
        auto res = Http::get("https://crumblt.com/api/auth/me.php");
        auto j = nlohmann::json::parse(res.body);
        if (j.contains("user"))
            m_username = j["user"].value("username", "unknown");
    } catch (...) { m_username = "unknown"; }

    // Fetch game name
    try {
        auto res = Http::get("https://crumblt.com/api/games/get.php?id=" + m_cfg.gameId);
        auto j = nlohmann::json::parse(res.body);
        if (j.value("success", false))
            m_gameName = j["game"].value("name", m_cfg.gameId);
    } catch (...) { m_gameName = m_cfg.gameId; }

    // Load scene
    try {
        SceneLoader::load(m_cfg.gameId, m_scene);
        printf("[Client] Scene loaded, %zu root nodes\n", m_scene.roots().size());
    } catch (std::exception& e) {
        printf("[Client] Scene load failed: %s\n", e.what());
    }

    // Spawn local player
    Player local;
    local.id       = "local";
    local.username = m_username;
    local.isLocal  = true;
    local.colorR = 1.0f; local.colorG = 0.85f; local.colorB = 0.0f;
    m_players.push_back(local);
    m_localPlayer = &m_players.back();

    // Wire Photon callbacks
    m_photon.onPlayerJoin([this](int actorNr, const std::string& uname) {
        printf("[Photon] Player joined: %d (%s)\n", actorNr, uname.c_str());
        Player p;
        p.id       = std::to_string(actorNr);
        p.username = uname;
        p.isLocal  = false;
        p.colorR = 0.3f; p.colorG = 0.7f; p.colorB = 1.0f; // blue for remote
        m_players.push_back(p);
        // FIX: vector may have reallocated — reset local player pointer
        for (auto& pl : m_players)
            if (pl.isLocal) { m_localPlayer = &pl; break; }
    });

    m_photon.onPlayerLeave([this](int actorNr) {
        printf("[Photon] Player left: %d\n", actorNr);
        std::string id = std::to_string(actorNr);
        m_players.erase(std::remove_if(m_players.begin(), m_players.end(),
            [&](const Player& p){ return p.id == id; }), m_players.end());
        // Reset local player pointer in case vector reallocated
        for (auto& p : m_players)
            if (p.isLocal) { m_localPlayer = &p; break; }
    });

    m_photon.onPlayerMove([this](int actorNr, float x, float z) {
        std::string id = std::to_string(actorNr);
        for (auto& p : m_players) {
            if (p.id == id) { p.x = x; p.z = z; break; }
        }
    });

    // Connect to Photon
    m_photon.connect(m_cfg.gameId, m_username);

    m_renderer = std::make_unique<Renderer>(m_sdlRenderer);
    m_running  = true;
    m_lastTime = SDL_GetTicks64();
    printf("[Client] Ready! Game: %s  User: %s\n", m_gameName.c_str(), m_username.c_str());
}

void App::shutdown() {
    m_photon.disconnect();
    m_renderer.reset();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (m_sdlRenderer) { SDL_DestroyRenderer(m_sdlRenderer); m_sdlRenderer = nullptr; }
    if (m_window)      { SDL_DestroyWindow(m_window);        m_window      = nullptr; }
    SDL_Quit();
}

int App::run() {
    try { init(); }
    catch (std::exception& e) {
        printf("[Client] Init failed: %s\n", e.what());
        return 1;
    }
    while (m_running) {
        pollEvents();
        uint64_t now = SDL_GetTicks64();
        float dt = (now - m_lastTime) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        m_lastTime = now;
        update(dt);
        render();
    }
    return 0;
}

void App::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT) m_running = false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) m_running = false;

        bool down = (e.type == SDL_KEYDOWN);
        bool up   = (e.type == SDL_KEYUP);
        if (down || up) {
            switch (e.key.keysym.sym) {
                case SDLK_w:     m_keyW    = down; break;
                case SDLK_s:     m_keyS    = down; break;
                case SDLK_a:     m_keyA    = down; break;
                case SDLK_d:     m_keyD    = down; break;
                case SDLK_UP:    m_keyUp   = down; break;
                case SDLK_DOWN:  m_keyDown = down; break;
                case SDLK_LEFT:  m_keyLeft = down; break;
                case SDLK_RIGHT: m_keyRight= down; break;
                default: break;
            }
        }
    }
}

void App::update(float dt) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Photon tick
    m_photon.update();

    if (m_localPlayer) {
        float spd = m_localPlayer->speed;
        if (m_keyW || m_keyUp)    m_localPlayer->z -= spd * dt;
        if (m_keyS || m_keyDown)  m_localPlayer->z += spd * dt;
        if (m_keyA || m_keyLeft)  m_localPlayer->x -= spd * dt;
        if (m_keyD || m_keyRight) m_localPlayer->x += spd * dt;

        // Camera follows player
        m_renderer->camX = m_localPlayer->x;
        m_renderer->camZ = m_localPlayer->z;

        // Send position 20x/sec
        m_sendTimer += dt;
        if (m_sendTimer >= 0.05f) {
            m_sendTimer = 0.0f;
            m_photon.sendPlayerMove(m_localPlayer->x, m_localPlayer->z);
        }
    }
}

void App::render() {
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);

    m_renderer->render(m_scene, m_players, w, h);

    // Game name top center
    ImGui::SetNextWindowPos({(float)w * 0.5f, 24.0f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowBgAlpha(0.4f);
    ImGui::Begin("##gamename", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextUnformatted(m_gameName.c_str());
    ImGui::End();

    // Username + connection status bottom left
    ImGui::SetNextWindowPos({8.0f, (float)h - 32.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.4f);
    ImGui::Begin("##username", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted(("@" + m_username).c_str());
    ImGui::SameLine();
    if (m_photon.isConnected()) {
        ImGui::TextColored({0.3f,1.0f,0.3f,1.0f}, " ● %d online", m_photon.playerCount());
    } else {
        ImGui::TextColored({1.0f,0.5f,0.2f,1.0f}, " ○ connecting...");
    }
    ImGui::End();

    // Position debug bottom right
    if (m_localPlayer) {
        ImGui::SetNextWindowPos({(float)w - 8.0f, (float)h - 32.0f}, ImGuiCond_Always, {1.0f, 1.0f});
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::Begin("##pos", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%.1f, %.1f", m_localPlayer->x, m_localPlayer->z);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_sdlRenderer);
    SDL_RenderPresent(m_sdlRenderer);
}
