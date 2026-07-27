// Tests for HttpClient.h/.cpp. Rather than depending on Python (or any
// external process) being available to act as a test server, this spins
// up a tiny, self-contained TCP server directly in the test binary, using
// the same cross-platform socket pattern HttpClient.cpp itself uses
// (POSIX on Linux/macOS, Winsock on Windows) - so this test has zero
// dependencies and runs the same way everywhere CTest does.
#include "HttpClient.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SockHandle;
    static const SockHandle INVALID_SOCK = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int SockHandle;
    static const SockHandle INVALID_SOCK = -1;
#endif

using namespace std;

namespace
{
    void closeSock(SockHandle s)
    {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif
    }

    // Starts a one-shot server on `port`: accepts exactly one connection,
    // reads the request (ignored - we always send the same canned
    // response), sends `response` verbatim, then closes. Runs on a
    // background thread so the test can act as the client concurrently.
    void runOneShotServer(int port, const string& response)
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        SockHandle listener = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons((unsigned short)port);

        bind(listener, (sockaddr*)&addr, sizeof(addr));
        listen(listener, 1);

        SockHandle client = accept(listener, nullptr, nullptr);
        if (client != INVALID_SOCK)
        {
            char buf[4096];
            recv(client, buf, sizeof(buf), 0); // read (and discard) the request
            send(client, response.c_str(), (int)response.size(), 0);
            closeSock(client);
        }
        closeSock(listener);
    }
}

int main()
{
    // --- Test 1: a normal 200 response with Content-Length ---
    {
        int port = 19850;
        string body = R"({"message":"hello"})";
        string httpResponse = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: " + to_string(body.size()) + "\r\n"
                               "Connection: close\r\n\r\n" + body;

        thread server(runOneShotServer, port, httpResponse);
        this_thread::sleep_for(chrono::milliseconds(200)); // give the server a moment to bind+listen

        int statusCode = 0;
        string responseBody, error;
        bool ok = HttpClient::get("127.0.0.1", port, "/test", statusCode, responseBody, error, 5);

        server.join();

        assert(ok);
        assert(statusCode == 200);
        assert(responseBody == body);
    }

    // --- Test 2: a chunked-transfer-encoded response ---
    {
        int port = 19851;
        // Two chunks: "Hello, " (7 bytes) + "World!" (6 bytes), then the
        // terminating zero-length chunk.
        string chunkedBody = "7\r\nHello, \r\n6\r\nWorld!\r\n0\r\n\r\n";
        string httpResponse = "HTTP/1.1 200 OK\r\n"
                               "Transfer-Encoding: chunked\r\n"
                               "Connection: close\r\n\r\n" + chunkedBody;

        thread server(runOneShotServer, port, httpResponse);
        this_thread::sleep_for(chrono::milliseconds(200));

        int statusCode = 0;
        string responseBody, error;
        bool ok = HttpClient::get("127.0.0.1", port, "/test", statusCode, responseBody, error, 5);

        server.join();

        assert(ok);
        assert(statusCode == 200);
        assert(responseBody == "Hello, World!");
    }

    // --- Test 3: a non-200 status code is still a successful round-trip
    // (HttpClient distinguishes "got an HTTP response" from "the response
    // was a success") ---
    {
        int port = 19852;
        string body = R"({"error":"model not found"})";
        string httpResponse = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Length: " + to_string(body.size()) + "\r\n"
                               "Connection: close\r\n\r\n" + body;

        thread server(runOneShotServer, port, httpResponse);
        this_thread::sleep_for(chrono::milliseconds(200));

        int statusCode = 0;
        string responseBody, error;
        bool ok = HttpClient::post("127.0.0.1", port, "/api/generate", "{}", statusCode, responseBody, error, 5);

        server.join();

        assert(ok); // the request round-tripped fine
        assert(statusCode == 404); // the caller is responsible for checking this
        assert(responseBody == body);
    }

    // --- Test 4: nothing listening on the port at all - must fail
    // cleanly with a real error message, not hang or crash ---
    {
        int statusCode = 0;
        string responseBody, error;
        bool ok = HttpClient::get("127.0.0.1", 1, "/test", statusCode, responseBody, error, 3);
        assert(!ok);
        assert(!error.empty());
    }

    cout << "All HttpClient tests passed.\n";
    return 0;
}
