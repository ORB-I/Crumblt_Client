#include "Scene.h"

void Scene::addRoot(std::shared_ptr<SceneNode> node) {
    m_roots.push_back(std::move(node));
}

std::shared_ptr<SceneNode> Scene::findById(const std::string& id) {
    for (auto& r : m_roots) {
        auto found = findIn(r, id);
        if (found) return found;
    }
    return nullptr;
}

std::shared_ptr<SceneNode> Scene::findIn(const std::shared_ptr<SceneNode>& node, const std::string& id) {
    if (node->id == id) return node;
    for (auto& c : node->children) {
        auto found = findIn(c, id);
        if (found) return found;
    }
    return nullptr;
}
