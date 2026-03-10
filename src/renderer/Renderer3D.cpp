#include "Renderer3D.h"
#include <raylib.h>

void Renderer3D::render(const Scene& scene, const std::vector<Player>& players, int w, int h) {
    BeginDrawing();
    ClearBackground({30, 30, 30, 255});
    // TODO: 3D rendering goes here
    EndDrawing();
}
