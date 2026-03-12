#include "Renderer.h"

Renderer::Renderer(SDL_Renderer *r)
    : m_r(r) {}

void Renderer::render(const Scene &scene, const std::vector<Player> &players, int winW, int winH) {
    // Background
    SDL_SetRenderDrawColor(m_r, 26, 26, 46, 255);
    SDL_RenderClear(m_r);

    // Camera center in pixels
    int cx = winW / 2 - (int)(camX * scale);
    int cy = winH / 2 - (int)(camZ * scale);

    // Grid
    SDL_SetRenderDrawColor(m_r, 50, 50, 80, 120);
    int gridSize = (int)scale;
    int startX   = cx % gridSize;
    if (startX > 0) startX -= gridSize;
    int startY = cy % gridSize;
    if (startY > 0) startY -= gridSize;
    for (int x = startX; x < winW; x += gridSize)
        SDL_RenderDrawLine(m_r, x, 0, x, winH);
    for (int y = startY; y < winH; y += gridSize)
        SDL_RenderDrawLine(m_r, 0, y, winW, y);

    // Scene nodes
    for (auto &root : scene.roots())
        drawNode(*root, cx, cy);

    // Players
    for (auto &p : players)
        drawPlayer(p, cx, cy);
}

void Renderer::drawNode(const SceneNode &node, int cx, int cy) {
    if (node.type == "Part") {
        int x = cx + (int)(node.posX * scale) - (int)(node.sizeX * scale * 0.5f);
        int y = cy + (int)(node.posZ * scale) - (int)(node.sizeZ * scale * 0.5f);
        int w = (int)(node.sizeX * scale);
        int h = (int)(node.sizeZ * scale);

        SDL_SetRenderDrawColor(m_r,
                               (uint8_t)(node.colorR * 255),
                               (uint8_t)(node.colorG * 255),
                               (uint8_t)(node.colorB * 255),
                               220);
        SDL_Rect rect{x, y, w, h};
        SDL_RenderFillRect(m_r, &rect);

        SDL_SetRenderDrawColor(m_r, 255, 255, 255, 40);
        SDL_RenderDrawRect(m_r, &rect);
    }
    for (auto &child : node.children)
        drawNode(*child, cx, cy);
}

void Renderer::drawPlayer(const Player &p, int cx, int cy) {
    const float pSize = 0.8f; // player is 0.8 units wide
    int         x     = cx + (int)(p.x * scale) - (int)(pSize * scale * 0.5f);
    int         y     = cy + (int)(p.z * scale) - (int)(pSize * scale * 0.5f);
    int         s     = (int)(pSize * scale);

    // Fill
    SDL_SetRenderDrawColor(
        m_r, (uint8_t)(p.colorR * 255), (uint8_t)(p.colorG * 255), (uint8_t)(p.colorB * 255), 255);
    SDL_Rect rect{x, y, s, s};
    SDL_RenderFillRect(m_r, &rect);

    // Outline
    SDL_SetRenderDrawColor(m_r, 255, 255, 255, 180);
    SDL_RenderDrawRect(m_r, &rect);

    // Direction dot (front = +Z)
    SDL_SetRenderDrawColor(m_r, 255, 255, 255, 200);
    SDL_Rect dot{x + s / 2 - 2, y + s - 6, 4, 4};
    SDL_RenderFillRect(m_r, &dot);
}
