#pragma once
#include <memory>
#include <string>
#include <vector>

struct SceneNode {
    std::string id;
    std::string name;
    std::string type; // "Part", "Folder", etc.

    float posX = 0, posY = 0, posZ = 0;
    float sizeX = 1, sizeY = 1, sizeZ = 1;
    float colorR = 0.8f, colorG = 0.2f, colorB = 0.2f;

    std::vector<std::shared_ptr<SceneNode>> children;

    // Network sync: set by PhotonClient when a remote player moves this node
    bool dirty = false;
};

class Scene {
  public:
    Scene() = default;

    void                                           addRoot(std::shared_ptr<SceneNode> node);
    std::shared_ptr<SceneNode>                     findById(const std::string &id);
    const std::vector<std::shared_ptr<SceneNode>> &roots() const { return m_roots; }

  private:
    std::shared_ptr<SceneNode> findIn(const std::shared_ptr<SceneNode> &node,
                                      const std::string                &id);

    std::vector<std::shared_ptr<SceneNode>> m_roots;
};
