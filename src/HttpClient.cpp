#include "HttpClient.h"
#include <sstream>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SockHandle;
    static const SockHandle INVALID_SOCK = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int SockHandle;
    static const SockHandle INVALID_SOCK = -1;
#endif

using namespace std;

namespace
{
    // Winsock needs one-time init/cleanup per process; POSIX needs nothing.
    struct NetworkInit
    {
#ifdef _WIN32
        NetworkInit() { WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa); }
        ~NetworkInit() { WSACleanup(); }
#endif
    };
    static NetworkInit g_networkInit;

    void closeSock(SockHandle s)
    {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }

    void setTimeout(SockHandle s, int seconds)
    {
#ifdef _WIN32
        DWORD ms = (DWORD)(seconds * 1000);
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&ms, sizeof(ms));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ms, sizeof(ms));
#else
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const void*)&tv, sizeof(tv));
#endif
    }

    SockHandle connectTo(const string& host, int port, string& error)
    {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo* result = nullptr;
        string portStr = to_string(port);
        int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
        if (rc != 0 || result == nullptr)
        {
            error = "Could not resolve host '" + host + "'";
            return INVALID_SOCK;
        }

        SockHandle sock = INVALID_SOCK;
        for (struct addrinfo* p = result; p != nullptr; p = p->ai_next)
        {
            sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock == INVALID_SOCK) continue;

            setTimeout(sock, 5); // connect-phase timeout

            if (connect(sock, p->ai_addr, (int)p->ai_addrlen) == 0)
                break; // connected

            closeSock(sock);
            sock = INVALID_SOCK;
        }
        freeaddrinfo(result);

        if (sock == INVALID_SOCK)
            error = "Could not connect to " + host + ":" + portStr +
                    " (is the server running?)";

        return sock;
    }

    bool sendAll(SockHandle sock, const string& data)
    {
        size_t sent = 0;
        while (sent < data.size())
        {
            int n = (int)send(sock, data.c_str() + sent, (int)(data.size() - sent), 0);
            if (n <= 0) return false;
            sent += (size_t)n;
        }
        return true;
    }

    // Reads the full raw HTTP response (headers + body) until the peer
    // closes the connection or we time out.
    string recvAll(SockHandle sock)
    {
        string result;
        char buf[8192];
        while (true)
        {
            int n = (int)recv(sock, buf, sizeof(buf), 0);
            if (n <= 0) break;
            result.append(buf, n);
        }
        return result;
    }

    string toLower(string s)
    {
        transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
        return s;
    }

    // Decodes an HTTP/1.1 chunked-transfer-encoded body.
    string decodeChunked(const string& body)
    {
        string out;
        size_t pos = 0;
        while (pos < body.size())
        {
            size_t lineEnd = body.find("\r\n", pos);
            if (lineEnd == string::npos) break;
            string sizeLine = body.substr(pos, lineEnd - pos);
            long chunkSize = strtol(sizeLine.c_str(), nullptr, 16);
            if (chunkSize <= 0) break;
            size_t dataStart = lineEnd + 2;
            if (dataStart + (size_t)chunkSize > body.size()) break;
            out.append(body, dataStart, (size_t)chunkSize);
            pos = dataStart + chunkSize + 2; // skip trailing \r\n after chunk
        }
        return out;
    }

    // Parses "HTTP/1.1 200 OK\r\nHeader: val\r\n...\r\n\r\n<body>" into
    // statusCode + body, handling Content-Length and chunked encoding.
    bool parseResponse(const string& raw, int& statusCode, string& body)
    {
        size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == string::npos) return false;

        string headerBlock = raw.substr(0, headerEnd);
        string rest = raw.substr(headerEnd + 4);

        size_t firstLineEnd = headerBlock.find("\r\n");
        string statusLine = headerBlock.substr(0, firstLineEnd);
        size_t sp1 = statusLine.find(' ');
        if (sp1 == string::npos) return false;
        statusCode = atoi(statusLine.c_str() + sp1 + 1);

        bool chunked = toLower(headerBlock).find("transfer-encoding: chunked") != string::npos;

        if (chunked)
        {
            body = decodeChunked(rest);
        }
        else
        {
            body = rest; // recvAll already read until the socket closed
        }
        return true;
    }

    bool doRequest(const string& method, const string& host, int port, const string& path,
                   const string& jsonBody, int& statusCode, string& responseBody,
                   string& error, int timeoutSeconds)
    {
        SockHandle sock = connectTo(host, port, error);
        if (sock == INVALID_SOCK) return false;

        setTimeout(sock, timeoutSeconds);

        ostringstream req;
        req << method << " " << path << " HTTP/1.1\r\n";
        req << "Host: " << host << "\r\n";
        req << "Connection: close\r\n";
        if (!jsonBody.empty())
        {
            req << "Content-Type: application/json\r\n";
            req << "Content-Length: " << jsonBody.size() << "\r\n";
        }
        req << "\r\n";
        req << jsonBody;

        if (!sendAll(sock, req.str()))
        {
            error = "Failed to send request to " + host + ":" + to_string(port);
            closeSock(sock);
            return false;
        }

        string raw = recvAll(sock);
        closeSock(sock);

        if (raw.empty())
        {
            error = "No response from " + host + ":" + to_string(port) +
                    " (timed out after " + to_string(timeoutSeconds) +
                    "s, or the connection closed before any data arrived). " +
                    "If Ollama is running, try a shorter prompt, a smaller/faster model, "
                    "or increase the timeout in AI Settings.";
            return false;
        }

        if (!parseResponse(raw, statusCode, responseBody))
        {
            error = "Received a malformed HTTP response";
            return false;
        }

        return true;
    }
}

bool HttpClient::post(const string& host, int port, const string& path,
                       const string& jsonBody, int& statusCode, string& responseBody,
                       string& error, int timeoutSeconds)
{
    return doRequest("POST", host, port, path, jsonBody, statusCode, responseBody, error, timeoutSeconds);
}

bool HttpClient::get(const string& host, int port, const string& path,
                      int& statusCode, string& responseBody, string& error, int timeoutSeconds)
{
    return doRequest("GET", host, port, path, "", statusCode, responseBody, error, timeoutSeconds);
}
