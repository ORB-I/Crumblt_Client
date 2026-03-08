#include "ScriptRuntime.h"
#include "../scene/Scene.h"
#include "../network/PhotonClient.h"
#include "../core/Http.h"

#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using json = nlohmann::json;

// ---- Lua binding helpers ----

// workspace.getPart(name) -> {x,y,z, sx,sy,sz, r,g,b} or nil
static int lua_workspace_getPart(lua_State* L) {
    auto* scene = (Scene*)lua_touserdata(L, lua_upvalueindex(1));
    const char* name = luaL_checkstring(L, 1);
    auto node = scene->findById(name);
    if (!node) { lua_pushnil(L); return 1; }
    lua_newtable(L);
    auto setf = [&](const char* k, float v) {
        lua_pushstring(L, k); lua_pushnumber(L, v); lua_settable(L, -3);
    };
    setf("x", node->posX); setf("y", node->posY); setf("z", node->posZ);
    setf("sx", node->sizeX); setf("sy", node->sizeY); setf("sz", node->sizeZ);
    setf("r", node->colorR); setf("g", node->colorG); setf("b", node->colorB);
    return 1;
}

// workspace.movePart(id, x, y, z)
static int lua_workspace_movePart(lua_State* L) {
    auto* scene  = (Scene*)lua_touserdata(L, lua_upvalueindex(1));
    auto* photon = (PhotonClient*)lua_touserdata(L, lua_upvalueindex(2));
    const char* id = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);
    auto node = scene->findById(id);
    if (node) {
        node->posX = x; node->posY = y; node->posZ = z;
        node->dirty = true;
        photon->sendNodeUpdate(id, x, y, z);
    }
    return 0;
}

// print() override — outputs to stdout
static int lua_print(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        if (i > 1) printf("\t");
        printf("%s", luaL_tolstring(L, i, nullptr));
        lua_pop(L, 1);
    }
    printf("\n");
    return 0;
}

// ---- ScriptRuntime ----

ScriptRuntime::ScriptRuntime(Scene* scene, PhotonClient* photon)
    : m_scene(scene), m_photon(photon) {
    m_L = luaL_newstate();
    luaL_openlibs((lua_State*)m_L);
    bindGlobals();
}

ScriptRuntime::~ScriptRuntime() {
    stop();
}

void ScriptRuntime::bindGlobals() {
    lua_State* L = (lua_State*)m_L;

    // Override print
    lua_pushcfunction(L, lua_print, "print");
    lua_setglobal(L, "print");

    // workspace table
    lua_newtable(L);

    // workspace.getPart
    lua_pushlightuserdata(L, m_scene);
    lua_pushcclosure(L, lua_workspace_getPart, "getPart", 1);
    lua_setfield(L, -2, "getPart");

    // workspace.movePart
    lua_pushlightuserdata(L, m_scene);
    lua_pushlightuserdata(L, m_photon);
    lua_pushcclosure(L, lua_workspace_movePart, "movePart", 2);
    lua_setfield(L, -2, "movePart");

    lua_setglobal(L, "workspace");

    printf("[ScriptRuntime] Globals bound\n");
}

void ScriptRuntime::loadFromScene(Scene& scene) {
    // Fetch scripts from API
    // We only need the gameId — pull it from the first root node's parent context
    // For now, scripts are fetched separately via the API
    // The caller (App) should have set session already via Auth

    // Find game_id from HTTP — we need it passed in; for now use a workaround:
    // Stored in scene root name (set by SceneLoader if needed), or passed via App
    // TODO: pass gameId directly to ScriptRuntime
    // For now we skip auto-load here and expose loadScripts(gameId) explicitly
    printf("[ScriptRuntime] loadFromScene called — use loadScripts(gameId) to fetch\n");
}

void ScriptRuntime::loadScripts(const std::string& gameId) {
    auto res = Http::get("https://crumblt.com/api/games/load_scripts.php?game_id=" + gameId);
    if (!res.ok) { printf("[ScriptRuntime] Failed to load scripts\n"); return; }

    auto j = json::parse(res.body);
    if (!j.value("success", false)) { printf("[ScriptRuntime] API error\n"); return; }

    for (auto& s : j["scripts"]) {
        LoadedScript ls;
        ls.name   = s.value("name",   "script");
        ls.source = s.value("source", "");
        m_scripts.push_back(std::move(ls));
    }
    printf("[ScriptRuntime] Loaded %zu scripts\n", m_scripts.size());
}

void ScriptRuntime::start() {
    if (m_started) return;
    m_started = true;
    for (auto& s : m_scripts)
        execScript(s);
}

void ScriptRuntime::update(float dt) {
    if (!m_started) return;
    lua_State* L = (lua_State*)m_L;

    // Call global onUpdate(dt) if it exists
    lua_getglobal(L, "onUpdate");
    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, dt);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            printf("[ScriptRuntime] onUpdate error: %s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }
}

void ScriptRuntime::stop() {
    if (!m_L) return;
    lua_close((lua_State*)m_L);
    m_L       = nullptr;
    m_started = false;
}

void ScriptRuntime::execScript(const LoadedScript& script) {
    lua_State* L = (lua_State*)m_L;

    size_t outSize = 0;
    char* err = nullptr;
    char* bytecode = luau_compile(script.source.c_str(), script.source.size(), nullptr, &outSize);

    if (!bytecode) {
        printf("[ScriptRuntime] Compile error in '%s'\n", script.name.c_str());
        return;
    }

    if (luau_load(L, script.name.c_str(), bytecode, outSize, 0) != LUA_OK) {
        printf("[ScriptRuntime] Load error in '%s': %s\n", script.name.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1);
        free(bytecode);
        return;
    }
    free(bytecode);

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        printf("[ScriptRuntime] Runtime error in '%s': %s\n", script.name.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        printf("[ScriptRuntime] Executed '%s'\n", script.name.c_str());
    }
}
