#pragma once
#include <string>
#include <optional>

struct UserInfo {
    std::string id;
    std::string username;
    std::string displayName;
    std::string avatarUrl;
    std::string role;
};

class Auth {
public:
    // Load saved session, verify with server, return user if valid
    static std::optional<UserInfo> tryAutoLogin();

    // Save session cookie to disk
    static void saveSession(const std::string& sessionId);

    // Clear saved session
    static void clearSession();

    // Get saved session cookie string for use in HTTP requests
    static std::string getSessionCookie();

    // Path to auth file
    static std::string authFilePath();

private:
    static std::string s_sessionCookie;
};
