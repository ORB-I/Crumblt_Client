#include "Http.h"

#include "Auth.h"

#include <curl/curl.h>
#include <stdexcept>

static size_t writeCallback(char *ptr, size_t size, size_t nmemb, std::string *out) {
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

static CURL *makeCurl(const std::string &url, std::string &body) {
    CURL *curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CrumbltStudio/0.1");

    // Attach session cookie if we have one
    std::string cookie = Auth::getSessionCookie();
    if (!cookie.empty()) curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());

    return curl;
}

static HttpResponse finalize(CURL *curl, CURLcode res, std::string &body) {
    HttpResponse r;
    if (res == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        r.status = (int)code;
        r.ok     = (code >= 200 && code < 300);
        r.body   = std::move(body);

        // Extract Set-Cookie if present and save session
        struct curl_header *h = nullptr;
        if (curl_easy_header(curl, "Set-Cookie", 0, CURLH_HEADER, -1, &h) == CURLHE_OK) {
            std::string val(h->value);
            auto        pos = val.find("PHPSESSID=");
            if (pos != std::string::npos) {
                auto        end = val.find(';', pos);
                std::string sid =
                    val.substr(pos + 10, end == std::string::npos ? end : end - pos - 10);
                Auth::saveSession(sid);
            }
        }
    }
    curl_easy_cleanup(curl);
    return r;
}

HttpResponse Http::get(const std::string &url) {
    std::string body;
    CURL       *curl = makeCurl(url, body);
    CURLcode    res  = curl_easy_perform(curl);
    return finalize(curl, res, body);
}

HttpResponse Http::post(const std::string &url, const std::string &jsonBody) {
    std::string body;
    CURL       *curl = makeCurl(url, body);

    struct curl_slist *headers = nullptr;
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    return finalize(curl, res, body);
}

HttpResponse Http::postForm(const std::string                        &url,
                            const std::map<std::string, std::string> &fields) {
    std::string body;
    CURL       *curl = makeCurl(url, body);

    std::string encoded;
    for (auto &[k, v] : fields) {
        if (!encoded.empty()) encoded += '&';
        char *ek = curl_easy_escape(curl, k.c_str(), 0);
        char *ev = curl_easy_escape(curl, v.c_str(), 0);
        encoded += std::string(ek) + "=" + ev;
        curl_free(ek);
        curl_free(ev);
    }
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, encoded.c_str());

    CURLcode res = curl_easy_perform(curl);
    return finalize(curl, res, body);
}

void Http::globalInit() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
void Http::globalCleanup() {
    curl_global_cleanup();
}
