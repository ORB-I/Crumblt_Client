#include "Renderer3D.h"

#include <cmath>
#include <raylib.h>
#include <rlgl.h>

void Renderer3D::render(const Scene &scene, const std::vector<Player> &players, int w, int h) {
    // RMB drag to orbit
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        m_lastMouse = GetMousePosition();
        m_dragging  = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) m_dragging = false;

    if (m_dragging) {
        Vector2 mouse = GetMousePosition();
        float   dx    = mouse.x - m_lastMouse.x;
        float   dy    = mouse.y - m_lastMouse.y;
        m_lastMouse   = mouse;
        m_yaw -= dx * 0.4f;
        m_pitch += dy * 0.4f;
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < 5.0f) m_pitch = 5.0f;
    }

    // Scroll to zoom
    float wheel = GetMouseWheelMove();
    m_distance -= wheel * 1.5f;
    if (m_distance < 3.0f) m_distance = 3.0f;
    if (m_distance > 40.0f) m_distance = 40.0f;

    // Compute camera position from yaw/pitch/distance
    float yawRad      = m_yaw * DEG2RAD;
    float pitchRad    = m_pitch * DEG2RAD;
    m_camera.target   = {camX, 0.5f, camZ};
    m_camera.position = {camX + m_distance * cosf(pitchRad) * sinf(yawRad),
                         m_distance * sinf(pitchRad),
                         camZ + m_distance * cosf(pitchRad) * cosf(yawRad)};

    BeginDrawing();
    ClearBackground({30, 30, 40, 255});

    BeginMode3D(m_camera);

    // Ground plane grid
    DrawGrid(40, 1.0f);

    // Draw scene nodes as cubes
    for (const auto &root : scene.roots()) {
        drawNode(*root);
    }

    // Draw players
    for (const auto &p : players) {
        Color   col = {(unsigned char)(p.colorR * 255),
                       (unsigned char)(p.colorG * 255),
                       (unsigned char)(p.colorB * 255),
                       255};
        Vector3 pos = {p.x, 1.0f, p.z};

        // Body
        DrawCylinder(pos, 0.35f, 0.35f, 1.6f, 8, col);
        DrawCylinderWires(pos, 0.35f, 0.35f, 1.6f, 8, {0, 0, 0, 120});

        // Head
        Vector3 headPos = {p.x, 2.8f, p.z};
        DrawSphere(headPos, 0.3f, col);
        DrawSphereWires(headPos, 0.3f, 6, 6, {0, 0, 0, 120});

        // Name label drawn in screen space below
    }

    EndMode3D();

    // Player name labels (2D screen-space, projected from 3D)
    for (const auto &p : players) {
        Vector3     headPos = {p.x, 3.2f, p.z};
        Vector2     screen  = GetWorldToScreen(headPos, m_camera);
        const char *name    = p.username.c_str();
        int         tw      = MeasureText(name, 14);
        DrawText(name, (int)screen.x - tw / 2, (int)screen.y, 14, WHITE);
    }

    EndDrawing();
}

void Renderer3D::drawNode(const SceneNode &node) {
    Vector3 pos  = {node.posX + node.sizeX * 0.5f,
                    node.posY + node.sizeY * 0.5f,
                    node.posZ + node.sizeZ * 0.5f};
    Vector3 size = {node.sizeX, node.sizeY, node.sizeZ};

    Color col = {(unsigned char)(node.colorR * 255),
                 (unsigned char)(node.colorG * 255),
                 (unsigned char)(node.colorB * 255),
                 255};

    DrawCubeV(pos, size, col);
    DrawCubeWiresV(pos, size, {0, 0, 0, 80});

    for (const auto &child : node.children)
        drawNode(*child);
}
