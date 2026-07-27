#pragma once
#include <string>

// A minimal HTTP/1.1 client over raw TCP sockets - no libcurl dependency.
// Only supports what talking to a local server (Ollama on localhost)
// needs: GET/POST with a JSON body, non-chunked or chunked responses.
// Works on Linux, macOS, and Windows (uses Winsock there).
class HttpClient
{
public:
    // Returns true if the request round-tripped at all (even a 404 or 500
    // counts as "true" - check statusCode for that). Returns false only for
    // connection-level failures (server unreachable, DNS failure, timeout),
    // and fills `error` with a human-readable reason.
    static bool post(const std::string& host, int port, const std::string& path,
                      const std::string& jsonBody, int& statusCode, std::string& responseBody,
                      std::string& error, int timeoutSeconds = 120);

    static bool get(const std::string& host, int port, const std::string& path,
                     int& statusCode, std::string& responseBody, std::string& error,
                     int timeoutSeconds = 10);
};
