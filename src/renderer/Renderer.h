#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "../scene/Scene.h"
#include "../scene/Player.h"

class Renderer {
public:
    explicit Renderer(SDL_Renderer* r);

    // Camera offset in world units
    float camX = 0.0f;
    float camZ = 0.0f;
    const float scale = 40.0f; // px per world unit

    void render(const Scene& scene, const std::vector<Player>& players, int winW, int winH);

private:
    void drawNode(const SceneNode& node, int cx, int cy);
    void drawPlayer(const Player& p, int cx, int cy);

    SDL_Renderer* m_r = nullptr;
};
