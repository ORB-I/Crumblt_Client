#include "SceneLoader.h"
#include "../core/Http.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstdio>

using json = nlohmann::json;

void SceneLoader::load(const std::string& gameId, Scene& scene) {
    auto res = Http::get("https://crumblt.com/api/games/load_scene.php?game_id=" + gameId);
    if (!res.ok) throw std::runtime_error("[SceneLoader] HTTP error: " + res.body);

    auto j = json::parse(res.body);
    if (!j.value("success", false))
        throw std::runtime_error("[SceneLoader] API error: " + res.body);

    for (auto& node : j["scene"])
        scene.addRoot(parseNode(node));

    printf("[SceneLoader] Loaded scene for game %s\n", gameId.c_str());
}

std::shared_ptr<SceneNode> SceneLoader::parseNode(const json& j) {
    auto node = std::make_shared<SceneNode>();
    node->id   = j.value("id",   "");
    node->name = j.value("name", "");
    node->type = j.value("type", "");

    if (j.contains("pos")) {
        node->posX = j["pos"][0]; node->posY = j["pos"][1]; node->posZ = j["pos"][2];
    }
    if (j.contains("size")) {
        node->sizeX = j["size"][0]; node->sizeY = j["size"][1]; node->sizeZ = j["size"][2];
    }
    if (j.contains("color")) {
        node->colorR = j["color"][0]; node->colorG = j["color"][1]; node->colorB = j["color"][2];
    }
    if (j.contains("children")) {
        for (auto& child : j["children"])
            node->children.push_back(parseNode(child));
    }
    return node;
}
