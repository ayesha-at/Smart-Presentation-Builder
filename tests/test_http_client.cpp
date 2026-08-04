// Tests for HttpClient.h/.cpp. Rather than depending on Python (or any
// external process) being available to act as a test server, this spins
// up a tiny, self-contained TCP server directly in the test binary, using
// the same cross-platform socket pattern HttpClient.cpp itself uses
// (POSIX on Linux/macOS, Winsock on Windows) - so this test has zero
// dependencies and runs the same way everywhere CTest does.
//
// IMPORTANT: the server thread's accept() call is bounded by select()
// with a timeout, and startup is synchronized via an atomic flag rather
// than a fixed sleep. An earlier version used accept() with no timeout at
// all and a blind sleep_for(200ms) before connecting - if the client-side
// connection didn't succeed for any CI-environment-specific reason, that
// accept() would block forever, server.join() would then also block
// forever, and the whole CI job would hang until GitHub's default 6-hour
// job timeout killed it. This version cannot hang: every wait has an
// explicit bound.
#include "HttpClient.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <atomic>
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

    // Starts a one-shot server on `port`: accepts at most one connection,
    // reads the request (ignored - we always send the same canned
    // response), sends `response` verbatim, then closes.
    //
    // `ready` is set to true the moment the socket is actually bound and
    // listening - the caller waits on this (bounded) instead of guessing
    // with a fixed sleep, which removes the connect-before-listening race
    // entirely rather than just making it statistically less likely.
    //
    // accept() is bounded by select() with a timeout, so if no client
    // ever connects (for whatever reason), this function still returns
    // within a few seconds instead of blocking forever - that's what
    // actually caused the CI hang this replaced.
    void runOneShotServer(int port, const string& response, atomic<bool>& ready)
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        SockHandle listener = socket(AF_INET, SOCK_STREAM, 0);
        if (listener == INVALID_SOCK) { ready = true; return; } // let the waiting side time out and fail clearly

        int opt = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons((unsigned short)port);

        if (::bind(listener, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(listener, 1) != 0)
        {
            closeSock(listener);
            ready = true; // still signal - the test will fail on the HTTP call, not hang
            return;
        }

        ready = true; // genuinely listening now - safe for the client to connect

        // Bounded wait for a connection: select() with a timeout means
        // accept() below is only reached once we know a connection is
        // actually available, and if one never arrives, this whole
        // function still returns after the timeout instead of blocking
        // forever.
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listener, &readSet);
        timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int selectResult = ::select((int)listener + 1, &readSet, nullptr, nullptr, &tv);
        if (selectResult > 0)
        {
            SockHandle client = ::accept(listener, nullptr, nullptr);
            if (client != INVALID_SOCK)
            {
                char buf[4096];
                recv(client, buf, sizeof(buf), 0); // read (and discard) the request
                send(client, response.c_str(), (int)response.size(), 0);
                closeSock(client);
            }
        }
        // selectResult == 0 means "timed out waiting for a connection" -
        // fall through and close the listener either way.

        closeSock(listener);
    }

    // Waits (bounded, up to 2 seconds) for the server thread to actually
    // be listening before returning - replaces a fixed sleep_for(), which
    // either races on a slow CI runner or wastes time on a fast one.
    void waitUntilReady(atomic<bool>& ready)
    {
        for (int i = 0; i < 200 && !ready; i++)
            this_thread::sleep_for(chrono::milliseconds(10));
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

        atomic<bool> ready{false};
        thread server(runOneShotServer, port, httpResponse, ref(ready));
        waitUntilReady(ready);

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

        atomic<bool> ready{false};
        thread server(runOneShotServer, port, httpResponse, ref(ready));
        waitUntilReady(ready);

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

        atomic<bool> ready{false};
        thread server(runOneShotServer, port, httpResponse, ref(ready));
        waitUntilReady(ready);

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
