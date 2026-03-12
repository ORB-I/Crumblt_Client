#pragma once
#include "Scene.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

class SceneLoader {
  public:
    static void load(const std::string &gameId, Scene &scene);

  private:
    static std::shared_ptr<SceneNode> parseNode(const nlohmann::json &j);
};
