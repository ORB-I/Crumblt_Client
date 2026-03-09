#pragma once
#include <string>
#include <vector>
#include <memory>

class Scene;
class PhotonClient;
struct SceneNode; // Forward declare

struct LoadedScript {
    std::string name;
    std::string source;
};

class ScriptRuntime {
public:
    ScriptRuntime(Scene* scene, PhotonClient* photon);
    ~ScriptRuntime();

    void loadFromScene(Scene& scene);
    void loadScripts(const std::string& gameId);
    void start();
    void update(float dt);
    void stop();

private:
    void execScript(const LoadedScript& script);
    void bindGlobals();
    void createPartMetatable(); // <-- ADDED

    Scene*        m_scene  = nullptr;
    PhotonClient* m_photon = nullptr;
    void*         m_L      = nullptr;
    bool          m_started = false;

    std::vector<LoadedScript> m_scripts;
};
