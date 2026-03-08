#pragma once
#include <string>
#include <vector>
#include <memory>

class Scene;
class PhotonClient;

struct LoadedScript {
    std::string name;
    std::string source;
};

class ScriptRuntime {
public:
    ScriptRuntime(Scene* scene, PhotonClient* photon);
    ~ScriptRuntime();

    // Fetches scripts from API and compiles them
    void loadFromScene(Scene& scene);
    void loadScripts(const std::string& gameId);

    // Called once after load — fires each script's top-level code
    void start();

    // Called every frame — fires any registered onUpdate handlers
    void update(float dt);

    // Stop all scripts, clean up VM
    void stop();

private:
    void execScript(const LoadedScript& script);
    void bindGlobals(); // Exposes game API (workspace, players, etc.) to Lua

    Scene*        m_scene  = nullptr;
    PhotonClient* m_photon = nullptr;
    void*         m_L      = nullptr; // lua_State* — opaque to avoid header pollution
    bool          m_started = false;

    std::vector<LoadedScript> m_scripts;
};
