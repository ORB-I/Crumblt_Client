#pragma once
#include <map>
#include <string>

struct HttpResponse {
    bool        ok     = false;
    int         status = 0;
    std::string body;
};

class Http {
  public:
    static HttpResponse get(const std::string &url);
    static HttpResponse post(const std::string &url, const std::string &jsonBody);
    static HttpResponse postForm(const std::string                        &url,
                                 const std::map<std::string, std::string> &fields);

    // Called once at startup/shutdown
    static void globalInit();
    static void globalCleanup();
};
