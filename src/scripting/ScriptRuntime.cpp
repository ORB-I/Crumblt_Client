#include "ScriptRuntime.h"
#include "../scene/Scene.h"
#include "../network/PhotonClient.h"
#include "../core/Http.h"

// Add these includes
#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <memory>

using json = nlohmann::json;

// ---- Lua binding helpers ----

// workspace.getPart(name) -> Part object or nil
static int lua_workspace_getPart(lua_State* L) {
    auto* scene = (Scene*)lua_touserdata(L, lua_upvalueindex(1));
    const char* name = luaL_checkstring(L, 1);
    auto node = scene->findById(name);
    if (!node) {
        lua_pushnil(L);
        return 1;
    }

    // Store shared_ptr on heap
    auto** partPtr = (std::shared_ptr<SceneNode>**)lua_newuserdata(L, sizeof(std::shared_ptr<SceneNode>*));
    *partPtr = new std::shared_ptr<SceneNode>(node);

    // Set metatable
    luaL_getmetatable(L, "Part");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_color3_new(lua_State* L) {
    float r = (float)luaL_checknumber(L, 1);
    float g = (float)luaL_checknumber(L, 2);
    float b = (float)luaL_checknumber(L, 3);

    lua_newtable(L);
    lua_pushnumber(L, r); lua_setfield(L, -2, "r");
    lua_pushnumber(L, g); lua_setfield(L, -2, "g");
    lua_pushnumber(L, b); lua_setfield(L, -2, "b");
    return 1;
}

static int lua_color3_fromRGB(lua_State* L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);

    lua_newtable(L);
    lua_pushnumber(L, r/255.0f); lua_setfield(L, -2, "r");
    lua_pushnumber(L, g/255.0f); lua_setfield(L, -2, "g");
    lua_pushnumber(L, b/255.0f); lua_setfield(L, -2, "b");
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
    createPartMetatable();
    bindGlobals();
}

ScriptRuntime::~ScriptRuntime() {
    stop();
}

void ScriptRuntime::createPartMetatable() {
    lua_State* L = (lua_State*)m_L;

    luaL_newmetatable(L, "Part");

    // __gc to clean up shared_ptr
    lua_pushcfunction(L, [](lua_State* L) -> int {
        auto** partPtr = (std::shared_ptr<SceneNode>**)lua_touserdata(L, 1);
        delete partPtr; // Free the shared_ptr
        return 0;
    }, "__gc");
    lua_setfield(L, -2, "__gc");

    // __index for property access
    lua_pushcfunction(L, [](lua_State* L) -> int {
        auto** partPtr = (std::shared_ptr<SceneNode>**)lua_touserdata(L, 1);
        auto& node = **partPtr;
        const char* key = lua_tostring(L, 2);

        if (strcmp(key, "Color") == 0) {
            lua_newtable(L);
            lua_pushnumber(L, node->colorR); lua_setfield(L, -2, "r");
            lua_pushnumber(L, node->colorG); lua_setfield(L, -2, "g");
            lua_pushnumber(L, node->colorB); lua_setfield(L, -2, "b");
            return 1;
        }
        else if (strcmp(key, "Position") == 0) {
            lua_newtable(L);
            lua_pushnumber(L, node->posX); lua_setfield(L, -2, "x");
            lua_pushnumber(L, node->posY); lua_setfield(L, -2, "y");
            lua_pushnumber(L, node->posZ); lua_setfield(L, -2, "z");
            return 1;
        }
        else if (strcmp(key, "Size") == 0) {
            lua_newtable(L);
            lua_pushnumber(L, node->sizeX); lua_setfield(L, -2, "x");
            lua_pushnumber(L, node->sizeY); lua_setfield(L, -2, "y");
            lua_pushnumber(L, node->sizeZ); lua_setfield(L, -2, "z");
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }, "__index");
    lua_setfield(L, -2, "__index");

    // __newindex for property assignment
    lua_pushcfunction(L, [](lua_State* L) -> int {
        auto** partPtr = (std::shared_ptr<SceneNode>**)lua_touserdata(L, 1);
        auto& node = **partPtr;
        const char* key = lua_tostring(L, 2);

        if (strcmp(key, "Color") == 0 && lua_istable(L, 3)) {
            lua_getfield(L, 3, "r"); node->colorR = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "g"); node->colorG = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "b"); node->colorB = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            node->dirty = true;
        }
        else if (strcmp(key, "Position") == 0 && lua_istable(L, 3)) {
            lua_getfield(L, 3, "x"); node->posX = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "y"); node->posY = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "z"); node->posZ = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            node->dirty = true;
        }
        else if (strcmp(key, "Size") == 0 && lua_istable(L, 3)) {
            lua_getfield(L, 3, "x"); node->sizeX = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "y"); node->sizeY = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_getfield(L, 3, "z"); node->sizeZ = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            node->dirty = true;
        }

        return 0;
    }, "__newindex");
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1); // Pop metatable
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

    // Color3 table
    lua_newtable(L);
    lua_pushcfunction(L, lua_color3_new, "new");
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, lua_color3_fromRGB, "fromRGB");
    lua_setfield(L, -2, "fromRGB");
    lua_setglobal(L, "Color3");

    printf("[ScriptRuntime] Globals bound\n");
}

void ScriptRuntime::loadFromScene(Scene& scene) {
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
