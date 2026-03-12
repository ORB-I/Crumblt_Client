#include "Auth.h"

#include "Http.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

std::string Auth::s_sessionCookie;

std::string Auth::authFilePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path);
    std::filesystem::create_directories(std::string(path) + "\\Crumblt");
    return std::string(path) + "\\Crumblt\\session.json";
#else
    const char *home = getenv("HOME");
    std::string dir  = std::string(home) + "/.config/crumblt";
    std::filesystem::create_directories(dir);
    return dir + "/session.json";
#endif
}

void Auth::saveSession(const std::string &sessionId) {
    s_sessionCookie = "PHPSESSID=" + sessionId;
    try {
        nlohmann::json j = {{"session", sessionId}};
        std::ofstream  f(authFilePath());
        f << j.dump(2);
    } catch (...) {}
}

void Auth::clearSession() {
    s_sessionCookie.clear();
    std::filesystem::remove(authFilePath());
}

std::string Auth::getSessionCookie() {
    return s_sessionCookie;
}

std::optional<UserInfo> Auth::tryAutoLogin() {
    std::string sessionId;
    try {
        std::ifstream f(authFilePath());
        if (!f.is_open()) return std::nullopt;
        nlohmann::json j;
        f >> j;
        sessionId = j.value("session", "");
    } catch (...) {
        return std::nullopt;
    }
    if (sessionId.empty()) return std::nullopt;

    s_sessionCookie = "PHPSESSID=" + sessionId;

    // Verify session is still valid
    auto res = Http::get("https://crumblt.com/api/auth/me.php");
    if (!res.ok) {
        clearSession();
        return std::nullopt;
    }

    try {
        auto     data = nlohmann::json::parse(res.body);
        auto    &u    = data["user"];
        UserInfo info;
        info.id          = u.value("id", "");
        info.username    = u.value("username", "");
        info.displayName = u.value("display_name", "");
        info.avatarUrl   = u.value("avatar_url", "");
        info.role        = u.value("role", "user");
        return info;
    } catch (...) {
        clearSession();
        return std::nullopt;
    }
}
