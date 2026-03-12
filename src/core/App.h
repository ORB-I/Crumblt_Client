#pragma once

#include <network/PhotonClient.h>
#include <renderer/Renderer3D.h>
#include <scene/Player.h>
#include <scene/Scene.h>

#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

struct AppConfig {
    std::string gameId;
    std::string session;
    std::string title  = "Crumblt";
    int         width  = 1280;
    int         height = 720;
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

    AppConfig m_cfg;
    bool      m_running = false;

    std::string m_gameName;
    std::string m_username;

    Scene               m_scene;
    std::vector<Player> m_players;
    Player             *m_localPlayer = nullptr;

    PhotonClient m_photon;
    float        m_sendTimer = 0.0f;

    // Chat
    struct ChatMessage {
        std::string username;
        std::string text;
    };
    std::vector<ChatMessage> m_chatMessages;
    char                     m_chatInput[256] = {};
    bool                     m_chatFocused    = false;
    bool                     m_scrollToBottom = false;

    // Error popup
    struct AppError {
        int         code;
        std::string message;
    };
    bool     m_showError = false;
    AppError m_error;
    void     showError(int code, const std::string &message);

    std::unique_ptr<Renderer3D> m_renderer;

    bool m_keyW = false, m_keyA = false, m_keyS = false, m_keyD = false;
    bool m_keyUp = false, m_keyDown = false, m_keyLeft = false, m_keyRight = false;
};
