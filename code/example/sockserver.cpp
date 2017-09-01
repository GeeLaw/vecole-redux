#include"../library/socket_wrappers.hpp"
#include<cstdio>

using namespace Networking::SocketWrappers;

constexpr unsigned short LuckyPort = 14393;
unsigned char buffer[257];
unsigned char garbage[4] = { 1, 2, 3, 4 };

int main(int argc, char **argv)
{
    SocketInitialiser socketInitialiser_;
    if (!socketInitialiser_.Good())
    {
        fputs("Could not initialise socket.\n", stderr);
        return -2;
    }
    SocketUnique serverSock = ServerConnectToClient(LuckyPort);
    if (!serverSock.IsValid())
    {
        fputs("Invalid socket returned from ServerConnectToClient.\n", stderr);
        return -3;
    }
    SocketConsumer server = serverSock.RawValue();
    unsigned char ping[1] = { 42 };
    if (!server.Receive(1, ping))
        return -4;
    if (ping[0] != 42)
    {
        fputs("Invalid ping.\n", stderr);
        return -4;
    }
    if (!server.Send(1, ping))
        return -4;
    if (!server.Receive(1, buffer))
        return -5;
    if (!server.Receive(buffer[0], buffer + 1))
        return -5;
    (buffer + 1)[buffer[0]] = 0;
    puts((char const *)(buffer + 1));
    for (auto i = buffer + 1; *i; ++i)
        if (*i >= 'A' && *i <= 'Z')
            *i += 'a' - 'A';
        else if (*i >= 'a' && *i <= 'z')
            *i -= 'a' - 'A';
        else if (*i == '@')
            *i-- = '\0';
        else
            *i = '_';
    buffer[0] = (unsigned char)strlen((char const *)(buffer + 1));
    if (!server.Send(4, garbage))
        return -5;
    if (!server.Send(1u + buffer[0], buffer))
        return -5;
    return 0;
}
