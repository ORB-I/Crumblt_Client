#include "core/App.h"
#include "core/Auth.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <string>

static std::string getQueryParam(const std::string& uri, const std::string& key) {
    auto pos = uri.find('?');
    if (pos == std::string::npos) return "";
    std::string query = uri.substr(pos + 1);
    std::string search = key + "=";
    auto kpos = query.find(search);
    if (kpos == std::string::npos) return "";
    auto vstart = kpos + search.size();
    auto vend = query.find('&', vstart);
    return query.substr(vstart, vend == std::string::npos ? vend : vend - vstart);
}

static std::string parseGameIdFromUri(const std::string& uri) {
    const std::string prefix = "crumblt://play/";
    if (uri.rfind(prefix, 0) != 0) return "";
    std::string rest = uri.substr(prefix.size());
    auto qpos = rest.find('?');
    return qpos == std::string::npos ? rest : rest.substr(0, qpos);
}

int main(int argc, char* argv[]) {
    AppConfig cfg;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--game-id") == 0 && i + 1 < argc) { cfg.gameId  = argv[++i]; continue; }
        if (strcmp(argv[i], "--session") == 0 && i + 1 < argc) { cfg.session = argv[++i]; continue; }

        std::string arg = argv[i];
        if (arg.rfind("crumblt://", 0) == 0) {
            cfg.gameId  = parseGameIdFromUri(arg);
            cfg.session = getQueryParam(arg, "session");
        }
    }

    printf("[Client] gameId:  %s\n", cfg.gameId.c_str());
    printf("[Client] session: %s\n", cfg.session.c_str());

    if (cfg.session.empty()) {
        auto saved = Auth::tryAutoLogin();
        if (saved)
            printf("[Client] Auto-login as %s\n", saved->username.c_str());
    }

    if (cfg.gameId.empty()) {
        printf("[Client] Error: --game-id or crumblt://play/<id> is required\n");
        return 1;
    }

    App app(cfg);
    return app.run();
}
