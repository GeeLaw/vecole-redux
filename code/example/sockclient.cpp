#include"../library/socket_wrappers.hpp"
#include<cstdio>

using namespace Networking::SocketWrappers;

constexpr unsigned short LuckyPort = 14393;
unsigned char buffer[257];

int main(int argc, char **argv)
{
    char const *serverAddr = "127.0.0.1";
    if (argc == 2)
    {
        serverAddr = argv[1];
    }
    else if (argc != 1)
    {
        fputs("Usage: sockclient [ipv4]\n"
              " ipv4: The IPv4 address of the server, defaults to 127.0.0.1.\n",
              stderr);
        return -1;
    }
    SocketInitialiser socketInitialiser_;
    if (!socketInitialiser_.Good())
    {
        fputs("Could not initialise socket.\n", stderr);
        return -2;
    }
    SocketUnique clientSock = ClientConnectToServer(serverAddr, LuckyPort);
    if (!clientSock.IsValid())
    {
        fputs("Invalid socket returned from ClientConnectToServer.\n", stderr);
        return -3;
    }
    SocketConsumer client = clientSock.RawValue();
    unsigned char ping[1] = { 42 };
    if (!client.Send(1, ping))
        return -4;
    if (!client.Receive(1, ping))
        return -4;
    if (ping[0] != 42)
    {
        fputs("Invalid pong.\n", stderr);
        return -4;
    }
    scanf("%255[^\r\n]", buffer + 1);
    buffer[0] = (unsigned char)strlen((char const *)(buffer + 1));
    if (!client.Send(1u + buffer[0], buffer))
        return -5;
    if (!client.Skip(4))
        return -5;
    if (!client.Receive(1, buffer))
        return -5;
    if (!client.Receive(buffer[0], buffer + 1))
        return -5;
    (buffer + 1)[buffer[0]] = 0;
    puts((char const *)(buffer + 1));
    return 0;
}
