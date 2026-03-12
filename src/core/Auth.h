#pragma once
#include <optional>
#include <string>

struct UserInfo {
    std::string id;
    std::string username;
    std::string displayName;
    std::string avatarUrl;
    std::string role;
};

class Auth {
  public:
    static std::optional<UserInfo> tryAutoLogin();
    static void        saveSession(const std::string &sessionId);
    static void        clearSession();
    static std::string getSessionCookie();
    static std::string authFilePath();

  private:
    static std::string s_sessionCookie;
};
