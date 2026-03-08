#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "Scene.h"

class SceneLoader {
public:
    static void load(const std::string& gameId, Scene& scene);

private:
    static std::shared_ptr<SceneNode> parseNode(const nlohmann::json& j);
};
