#include "App.h"

#include "Auth.h"
#include "Http.h"

#include <scene/SceneLoader.h>

#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <rlImGui.h>

App::App(AppConfig cfg)
    : m_cfg(std::move(cfg)) {}
App::~App() {
    shutdown();
}

void App::showError(int code, const std::string &message) {
    m_error     = {code, message};
    m_showError = true;
}

void App::init() {
    printf("[Client] init start\n");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(m_cfg.width, m_cfg.height, ("Crumblt - " + m_cfg.gameId).c_str());
    SetTargetFPS(60);

    rlImGuiSetup(true); // true = dark theme
    ImGui::StyleColorsDark();

    if (!m_cfg.session.empty()) Auth::saveSession(m_cfg.session);

    // Fetch username
    try {
        auto res = Http::get("https://crumblt.com/api/auth/me.php");
        auto j   = nlohmann::json::parse(res.body);
        if (j.contains("user")) {
            m_username = j["user"].value("username", "unknown");
        } else {
            showError(405, "You are not authenticated. Please log in again.");
        }
    } catch (...) {
        showError(500, "Failed to reach the authentication server.");
        m_username = "unknown";
    }

    // Fetch game name
    try {
        auto res = Http::get("https://crumblt.com/api/games/get.php?id=" + m_cfg.gameId);
        auto j   = nlohmann::json::parse(res.body);
        if (j.value("success", false)) {
            m_gameName = j["game"].value("name", m_cfg.gameId);
        } else {
            showError(404, "Game not found. It may have been deleted or made private.");
            m_gameName = m_cfg.gameId;
        }
    } catch (...) {
        showError(500, "Failed to fetch game information.");
        m_gameName = m_cfg.gameId;
    }

    // Set window title to game name
    SetWindowTitle(("Crumblt - " + m_gameName).c_str());

    // Load scene
    try {
        SceneLoader::load(m_cfg.gameId, m_scene);
        printf("[Client] Scene loaded, %zu root nodes\n", m_scene.roots().size());
    } catch (std::exception &e) {
        printf("[Client] Scene load failed: %s\n", e.what());
        showError(503, std::string("Failed to load the game scene: ") + e.what());
    }

    // Spawn local player
    Player local;
    local.id       = "local";
    local.username = m_username;
    local.isLocal  = true;
    local.colorR   = 1.0f;
    local.colorG   = 0.85f;
    local.colorB   = 0.0f;
    m_players.push_back(local);
    m_localPlayer = &m_players.back();

    // Wire Photon callbacks
    m_photon.onPlayerJoin([this](int actorNr, const std::string &uname) {
        printf("[Photon] Player joined: %d (%s)\n", actorNr, uname.c_str());
        Player p;
        p.id       = std::to_string(actorNr);
        p.username = uname;
        p.isLocal  = false;
        p.colorR   = 0.3f;
        p.colorG   = 0.7f;
        p.colorB   = 1.0f;
        m_players.push_back(p);
        for (auto &pl : m_players)
            if (pl.isLocal) {
                m_localPlayer = &pl;
                break;
            }
    });

    m_photon.onPlayerLeave([this](int actorNr) {
        printf("[Photon] Player left: %d\n", actorNr);
        std::string id = std::to_string(actorNr);
        m_players.erase(std::remove_if(m_players.begin(),
                                       m_players.end(),
                                       [&](const Player &p) { return p.id == id; }),
                        m_players.end());
        for (auto &p : m_players)
            if (p.isLocal) {
                m_localPlayer = &p;
                break;
            }
    });

    m_photon.onPlayerMove([this](int actorNr, float x, float z) {
        std::string id = std::to_string(actorNr);
        for (auto &p : m_players)
            if (p.id == id) {
                p.x = x;
                p.z = z;
                break;
            }
    });

    m_photon.onChat([this](const std::string &uname, const std::string &msg) {
        m_chatMessages.push_back({uname, msg});
        if (m_chatMessages.size() > 100) m_chatMessages.erase(m_chatMessages.begin());
        m_scrollToBottom = true;
    });

    m_photon.onError([this](int code, const std::string &msg) { showError(code, msg); });

    // Connect to Photon
    m_photon.connect(m_cfg.gameId, m_username);

    // Tell server we're in the game
    try {
        Http::post("https://crumblt.com/api/games/player_join.php",
                   "{\"game_id\":\"" + m_cfg.gameId + "\"}");
    } catch (...) {}

    m_renderer = std::make_unique<Renderer3D>();
    m_running  = true;
    printf("[Client] Ready! Game: %s  User: %s\n", m_gameName.c_str(), m_username.c_str());
}

void App::shutdown() {
    try {
        Http::post("https://crumblt.com/api/games/player_leave.php",
                   "{\"game_id\":\"" + m_cfg.gameId + "\"}");
    } catch (...) {}

    m_photon.disconnect();
    m_renderer.reset();
    rlImGuiShutdown();
    CloseWindow();
}

int App::run() {
    try {
        init();
    } catch (std::exception &e) {
        printf("[Client] Init failed: %s\n", e.what());
        return 1;
    }
    while (m_running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
        pollEvents();
        update(dt);
        render();
    }
    return 0;
}

void App::pollEvents() {
    // Raylib handles OS events internally — we just read key state
    if (!m_chatFocused) {
        m_keyW     = IsKeyDown(KEY_W);
        m_keyS     = IsKeyDown(KEY_S);
        m_keyA     = IsKeyDown(KEY_A);
        m_keyD     = IsKeyDown(KEY_D);
        m_keyUp    = IsKeyDown(KEY_UP);
        m_keyDown  = IsKeyDown(KEY_DOWN);
        m_keyLeft  = IsKeyDown(KEY_LEFT);
        m_keyRight = IsKeyDown(KEY_RIGHT);
    } else {
        m_keyW = m_keyS = m_keyA = m_keyD = false;
        m_keyUp = m_keyDown = m_keyLeft = m_keyRight = false;
    }

    if (IsKeyPressed(KEY_ESCAPE)) m_running = false;
}

void App::update(float dt) {
    m_photon.update();

    if (m_localPlayer) {
        float spd = m_localPlayer->speed;
        if (m_keyW || m_keyUp) m_localPlayer->z -= spd * dt;
        if (m_keyS || m_keyDown) m_localPlayer->z += spd * dt;
        if (m_keyA || m_keyLeft) m_localPlayer->x -= spd * dt;
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
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    // 3D scene (calls BeginDrawing / EndDrawing internally)
    m_renderer->render(m_scene, m_players, w, h);

    // ImGui overlays — drawn after EndDrawing via rlImGui
    rlImGuiBegin();

    // Game name top center
    ImGui::SetNextWindowPos({(float)w * 0.5f, 24.0f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowBgAlpha(0.4f);
    ImGui::Begin("##gamename",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextUnformatted(m_gameName.c_str());
    ImGui::End();

    // Username + connection status bottom left
    ImGui::SetNextWindowPos({8.0f, (float)h - 32.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.4f);
    ImGui::Begin("##username",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted(("@" + m_username).c_str());
    ImGui::SameLine();
    if (m_photon.isConnected()) {
        ImGui::TextColored({0.3f, 1.0f, 0.3f, 1.0f}, " * %d online", m_photon.playerCount());
    } else {
        ImGui::TextColored({1.0f, 0.5f, 0.2f, 1.0f}, " o connecting...");
    }
    ImGui::End();

    // Chat window
    ImGui::SetNextWindowPos({8.0f, (float)h - 180.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({320.0f, 140.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("##chat",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);
    ImGui::BeginChild("##chatlog", {304.0f, 100.0f}, false, ImGuiWindowFlags_NoScrollbar);
    for (auto &msg : m_chatMessages)
        ImGui::TextWrapped("[%s] %s", msg.username.c_str(), msg.text.c_str());
    if (m_scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }
    ImGui::EndChild();
    ImGui::SetNextItemWidth(280.0f);
    bool hitEnter = ImGui::InputText(
        "##chatinput", m_chatInput, sizeof(m_chatInput), ImGuiInputTextFlags_EnterReturnsTrue);
    m_chatFocused = ImGui::IsItemActive();
    ImGui::SameLine();
    if ((hitEnter || ImGui::Button("->")) && m_chatInput[0] != '\0') {
        std::string msg(m_chatInput);
        m_chatMessages.push_back({m_username, msg});
        m_scrollToBottom = true;
        m_photon.sendChat(msg);
        m_chatInput[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::End();

    // Position debug bottom right
    if (m_localPlayer) {
        ImGui::SetNextWindowPos(
            {(float)w - 8.0f, (float)h - 32.0f}, ImGuiCond_Always, {1.0f, 1.0f});
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::Begin("##pos",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%.1f, %.1f", m_localPlayer->x, m_localPlayer->z);
        ImGui::End();
    }

    // Error popup
    if (m_showError) {
        float pw = 400.0f, ph = 140.0f;
        ImGui::SetNextWindowPos({(float)w / 2 - pw / 2, (float)h / 2 - ph / 2}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({pw, ph}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.95f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.18f, 0.18f, 0.18f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Border, {0.35f, 0.35f, 0.35f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Button, {0.28f, 0.28f, 0.28f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.38f, 0.38f, 0.38f, 1.0f});
        ImGui::Begin("##error",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
        ImGui::SetCursorPosX(16.0f);
        ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Error code %d", m_error.code);
        ImGui::SetCursorPosX(16.0f);
        ImGui::Separator();
        ImGui::SetCursorPosX(16.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);
        ImGui::PushTextWrapPos(pw - 16.0f);
        ImGui::TextUnformatted(m_error.message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::SetCursorPos({pw - 106.0f, ph - 36.0f});
        if (ImGui::Button("Close Game", {90.0f, 24.0f})) m_running = false;
        ImGui::End();
        ImGui::PopStyleColor(4);
    }

    rlImGuiEnd();
}
