#pragma once
#include <string>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include "Http.h"
#include "../scene/Scene.h"
#include "../scene/Player.h"
#include "../renderer/Renderer.h"

struct AppConfig {
    std::string gameId;
    std::string session;
    std::string title = "Crumblt";
    int width  = 1280;
    int height = 720;
};

class App {
public:
    explicit App(AppConfig cfg);
    ~App();
    int run();

private:
    void init();
    void shutdown();
    void pollEvents();
    void update(float dt);
    void render();

    AppConfig              m_cfg;
    SDL_Window*            m_window      = nullptr;
    SDL_Renderer*          m_sdlRenderer = nullptr;
    bool                   m_running     = false;
    uint64_t               m_lastTime    = 0;

    std::string            m_gameName;
    std::string            m_username;

    Scene                  m_scene;
    std::vector<Player>    m_players;
    Player*                m_localPlayer = nullptr;

    std::unique_ptr<Renderer> m_renderer;

    // Input state
    bool m_keyW = false, m_keyA = false, m_keyS = false, m_keyD = false;
    bool m_keyUp = false, m_keyDown = false, m_keyLeft = false, m_keyRight = false;
};
