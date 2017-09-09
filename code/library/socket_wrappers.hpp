#ifndef SOCKET_WRAPPERS_HPP_
#define SOCKET_WRAPPERS_HPP_

#include<cstdio>
#include<cstring>
#include<cstdint>

#ifndef _WIN32

#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<errno.h>

/* Poly-fill Windows-specific identifiers used in the code. */
#define WSAStartup(ARG1, ARG2) (0)
#define WSACleanup() (0)
#define WSAGetLastError() (errno)
#define closesocket(ARG) (close(ARG))
#define WSADATA int

#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#endif // WIN32_LEAN_AND_MEAN

#include<windows.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")

#endif // _WIN32

namespace Networking
{
namespace SocketWrappers
{
    typedef uint16_t PortType;

    struct SocketInitialiser
    {
        SocketInitialiser()
            : success(false)
        {
            int wsaStartup = WSAStartup(MAKEWORD(2, 2), &wsa);
            if (wsaStartup == 0)
                success = true;
        }
        SocketInitialiser(SocketInitialiser const &) = delete;
        SocketInitialiser(SocketInitialiser &&) = delete;
        SocketInitialiser &operator = (SocketInitialiser const &) = delete;
        SocketInitialiser &operator = (SocketInitialiser &&) = delete;
        ~SocketInitialiser()
        {
            if (success)
                WSACleanup();
        }
        bool Good() const
        {
            return success;
        }
    private:
        bool success;
        WSADATA wsa;
    };

    struct SocketConsumer
    {
        SocketConsumer(SOCKET target)
            : socket_(target)
        { }
        SocketConsumer(SocketConsumer const &) = default;
        SocketConsumer(SocketConsumer &&) = default;
        SocketConsumer &operator = (SocketConsumer const &) = default;
        SocketConsumer &operator = (SocketConsumer &&) = default;
        ~SocketConsumer() = default;
        bool Send(size_t sz, void const *buf) const
        {
            for (auto buffer = (uint8_t const *)buf; sz; )
            {
                int length = (int)(sz < 8388608 ? sz : 8388608);
                int newlySent = send(socket_, (char const *)buffer, length, 0);
                if (newlySent == -1)
                {
                    fprintf(stderr, "send failed with %d.\n", WSAGetLastError());
                    return false;
                }
                sz -= (size_t)newlySent;
                buffer += newlySent;
            }
            return true;
        }
        bool Receive(size_t sz, void *buf) const
        {
            for (auto buffer = (uint8_t *)buf; sz; )
            {
                int length = (int)(sz < 8388608 ? sz : 8388608);
                int newlyReceived = recv(socket_, (char *)buffer, length, 0);
                if (newlyReceived == -1)
                {
                    fprintf(stderr, "recv failed with %d.\n", WSAGetLastError());
                    return false;
                }
                sz -= (size_t)newlyReceived;
                buffer += newlyReceived;
            }
            return true;
        }
        bool Skip(size_t sz) const
        {
            #ifndef _WIN32
                /* Use MSG_TRUNC. Win32 is known to not support it. */
                char * const garbage = nullptr;
                int const truncFlag = MSG_TRUNC;
            #else
                static uint8_t garbage_storage[8388608];
                char * const garbage = (char *)garbage_storage;
                int const truncFlag = 0;
            #endif
            while (sz)
            {
                int length = (int)(sz < 8388608 ? sz : 8388608);
                int newlySkipped = recv(socket_, garbage, length, truncFlag);
                if (newlySkipped == -1)
                {
                    fprintf(stderr, "recv failed with %d.\n", WSAGetLastError());
                    return false;
                }
                sz -= (size_t)newlySkipped;
            }
            return true;
        }
    private:
        SOCKET socket_;
    };

    struct SocketUnique
    {
        SocketUnique()
            : socket_(INVALID_SOCKET)
        { }
        SocketUnique(SOCKET target)
            : socket_(target)
        { }
        SocketUnique(SocketUnique const &) = delete;
        SocketUnique(SocketUnique &&other)
            : socket_(other.socket_)
        {
            other.socket_ = INVALID_SOCKET;
        }
        SocketUnique &operator = (SocketUnique const &) = delete;
        SocketUnique &operator = (SocketUnique &&other)
        {
            if (this == &other)
                return *this;
            socket_ = other.socket_;
            other.socket_ = INVALID_SOCKET;
            return *this;
        }
        ~SocketUnique()
        {
            if (socket_ != INVALID_SOCKET)
                closesocket(socket_);
        }
        bool IsValid() const
        {
            return socket_ != INVALID_SOCKET;
        }
        SOCKET RawValue() const
        {
            return socket_;
        }
        SOCKET Release()
        {
            SOCKET result = socket_;
            socket_ = INVALID_SOCKET;
            return result;
        }
    private:
        SOCKET socket_;
    };

    SOCKET ServerConnectToClient(PortType port)
    {
        SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSock == INVALID_SOCKET)
        {
            fprintf(stderr, "socket failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        SocketUnique serverSockUnique(serverSock);
        sockaddr_in sai;
        memset(&sai, 0, sizeof sai);
        sai.sin_family = AF_INET;
        sai.sin_addr.s_addr = INADDR_ANY;
        sai.sin_port = htons(port);
        int bound = bind(serverSock, (sockaddr *)&sai, sizeof sai);
        if (bound == SOCKET_ERROR)
        {
            fprintf(stderr, "bind failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        int listened = listen(serverSock, 5);
        if (listened == SOCKET_ERROR)
        {
            fprintf(stderr, "listen failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        int sailen = sizeof sai;
        SOCKET sock = accept(serverSock, (sockaddr *)&sai, &sailen);
        if (sock == INVALID_SOCKET)
        {
            fprintf(stderr, "accept failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        SocketUnique sockUnique(sock);
        int trueValue = 1;
        int setoptResult = setsockopt(
            sock, IPPROTO_TCP, TCP_NODELAY,
            (char const *)&trueValue, sizeof trueValue);
        if (setoptResult == SOCKET_ERROR)
        {
            fprintf(stderr, "setsockopt failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        return sockUnique.Release();
    }

    SOCKET ClientConnectToServer(char const *server, PortType port)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET)
        {
            fprintf(stderr, "socket failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        SocketUnique sockUnique(sock);
        sockaddr_in sai;
        memset(&sai, 0, sizeof sai);
        sai.sin_family = AF_INET;
        sai.sin_addr.s_addr = inet_addr(server);
        sai.sin_port = htons(port);
        int connectResult = connect(sock, (sockaddr *)&sai, sizeof sai);
        if (connectResult == SOCKET_ERROR)
        {
            fprintf(stderr, "connect failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        int trueValue = 1;
        int setoptResult = setsockopt(
            sock, IPPROTO_TCP, TCP_NODELAY,
            (char const *)&trueValue, sizeof trueValue);
        if (setoptResult == SOCKET_ERROR)
        {
            fprintf(stderr, "setsockopt failed with %d.\n", WSAGetLastError());
            return INVALID_SOCKET;
        }
        return sockUnique.Release();
    }

}
}


#ifndef _WIN32

#undef WSAStartup
#undef WSACleanup
#undef WSAGetLastError
#undef closesocket
#undef WSADATA

#endif // _WIN32

#endif // SOCKET_WRAPPERS_HPP_
