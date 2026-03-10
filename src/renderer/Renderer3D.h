#pragma once
#include "../scene/Scene.h"
#include "../scene/Player.h"
#include <vector>

class Renderer3D {
public:
    float camX = 0.0f;
    float camZ = 0.0f;

    Renderer3D() = default;
    ~Renderer3D() = default;

    void render(const Scene& scene, const std::vector<Player>& players, int w, int h);
};
