#pragma once
#include <scene/Player.h>
#include <scene/Scene.h>

#include <raylib.h>
#include <vector>

class Renderer3D {
  public:
    float camX = 0.0f;
    float camZ = 0.0f;

    Renderer3D() {
        m_camera.position   = {-10.0f, 12.0f, 10.0f};
        m_camera.target     = {0.0f, 0.0f, 0.0f};
        m_camera.up         = {0.0f, 1.0f, 0.0f};
        m_camera.fovy       = 60.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
    }
    ~Renderer3D() = default;

    void beginFrame();
    void draw3D(const Scene &scene, const std::vector<Player> &players);
    void endFrame(const std::vector<Player> &players);
    void render(const Scene &scene, const std::vector<Player> &players, int w, int h);

  private:
    Camera3D m_camera    = {};
    float    m_yaw       = -135.0f; // degrees around Y axis
    float    m_pitch     = 30.0f;   // degrees up/down
    float    m_distance  = 14.0f;   // distance from target
    Vector2  m_lastMouse = {};
    bool     m_dragging  = false;

    void drawNode(const SceneNode &node);
};
