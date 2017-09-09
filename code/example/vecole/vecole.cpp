#include"../../library/socket_wrappers.hpp"
#include"../../library/erasure.hpp"
#include"../../library/sparse_code.hpp"
#include"../../library/luby.hpp"
#include"../../library/cryptography.hpp"
#include<cstdio>
#include<cstring>
#include<random>

void PrintUsage();
bool ParseCommandLine(int argc, char **argv);
bool Prepare();
int PlayAlice();
int PlayBob();

bool isAlice;
char const *serverAddr;
unsigned short port;
char const *sparse;
char const *luby;

typedef Cryptography::Z<4294967291u> Zp;
typedef std::mt19937 RNG;

unsigned K, U, V, W;
std::uniform_int_distribution<uint32_t> UZp(0u, 4294967295u);

Encoding::LubyTransform::LTCode<> InnerCode;
Encoding::SparseLinearCode::FastSparseLinearCode<Zp> OuterCode;
Networking::SocketWrappers::SocketUnique socketUnique;
RNG randSrc((unsigned)time(nullptr));

#define PARAM_K 182
#define PARAM_U 244
#define PARAM_V 33124
#define PARAM_W 10000

Zp randomVector[PARAM_K];
Zp encodedVector[PARAM_U + PARAM_V];
Zp messageVector[PARAM_W];
Zp messageVectorTmp[PARAM_W];
bool notNoisyVector[PARAM_U + PARAM_V];
bool solvedVector[PARAM_W];

int (*play)() = nullptr;

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        PrintUsage();
        return -1;
    }
    Networking::SocketWrappers::SocketInitialiser init;
    if (!init.Good())
    {
        fputs("Could not initialise socket.\n", stderr);
        return -2;
    }
    if (!Prepare())
    {
        return -3;
    }
    int retValue = play();
    socketUnique.Release();
    return retValue;
}

void PrintUsage()
{
    fprintf(stderr,
        "vecole { alice | ipv4 } port sparse luby < input [> output]\n"
        "    Executes vector-OLE for k=%d, u=%d, v=%d, w=%d.\n\n"
        "Parameters:\n"
        "    alice | ipv4: if the parameter is \"alice\", the program plays Alice;\n"
        "                  otherwise, it plays Bob and talks to Alice on ipv4.\n"
        "            port: the port number.\n"
        "          sparse: the file name of the pre-generated fast sparse code.\n"
        "            luby: the file name of the pre-generated LT code.\n"
        "           input: the input file; if the program plays Alice, it should\n"
        "                  contain a single field element; otherwise, it should\n"
        "                  contain a(1), ..., a(w), b(1), ..., b(w).\n"
        "          output: the output file, used for Alice.\n",
        PARAM_K, PARAM_U, PARAM_V, PARAM_W);
}

bool ParseCommandLine(int argc, char **argv)
{
    if (argc != 5)
    {
        return false;
    }
    isAlice = (strcmp(argv[1], "alice") == 0);
    serverAddr = argv[1];
    if (sscanf(argv[2], "%hu", &port) != 1)
    {
        return false;
    }
    sparse = argv[3];
    luby = argv[4];
    return true;
}

struct
{
    bool operator () (Zp &zp, FILE *fp) const
    {
        uintmax_t uv;
        if (fscanf(fp, "%ju", &uv) != 1)
            return false;
        zp = (uint32_t)uv;
        return true;
    }
} const LoadZp;

bool Prepare()
{
    FILE *f;
    f = fopen(sparse, "r");
    if (!f)
    {
        fputs("Could not open sparse code.\n", stderr);
        return false;
    }
    bool outerCodeSuccess = OuterCode.LoadFrom(f, LoadZp);
    fclose(f);
    if (!outerCodeSuccess)
    {
        fputs("Could not load sparse code.\n", stderr);
        return false;
    }
    K = OuterCode.K;
    U = OuterCode.U;
    V = OuterCode.V;
    f = fopen(luby, "r");
    if (!f)
    {
        fputs("Could not open LT code.\n", stderr);
        return false;
    }
    bool innerCodeSuccess = InnerCode.LoadFrom(f);
    fclose(f);
    if (!innerCodeSuccess)
    {
        fputs("Could not load LT code.\n", stderr);
        return false;
    }
    W = InnerCode.InputSymbolSize;
    if (K != PARAM_K
        || U != PARAM_U
        || V != PARAM_V
        || W != PARAM_W)
    {
        fprintf(stderr,
            "System parameter mismatch. (k=%d, u=%d, v=%d, w=%d)\n",
            PARAM_K, PARAM_U, PARAM_V, PARAM_W);
        return false;
    }
    if (isAlice)
    {
        fputs("Waiting for connection...\n", stderr);
        socketUnique = Networking::SocketWrappers::ServerConnectToClient(port);
        if (!socketUnique.IsValid())
        {
            fputs("Could not connect to client.\n", stderr);
            return false;
        }
        Networking::SocketWrappers::SocketConsumer pipe = socketUnique.RawValue();
        uint32_t endianPing;
        if (!pipe.Receive(4, &endianPing) || endianPing != 42)
        {
            fputs(
                "Endian ping failed.\n"
                "The client and the server must have the same endianness!\n",
                stderr);
            return false;
        }
        endianPing = ~0u;
        if (!pipe.Send(4, &endianPing))
        {
            fputs("Pong failed.\n", stderr);
            return false;
        }
        fputs("Connection established.\n", stderr);
        play = PlayAlice;
        return true;
    }
    else
    {
        fputs("Connecting...\n", stderr);
        socketUnique = Networking::SocketWrappers::ClientConnectToServer(serverAddr, port);
        if (!socketUnique.IsValid())
        {
            fputs("Could not connect to server.\n", stderr);
            return false;
        }
        Networking::SocketWrappers::SocketConsumer pipe = socketUnique.RawValue();
        uint32_t endianPing = 42;
        if (!pipe.Send(4, &endianPing)
            || !pipe.Receive(4, &endianPing)
            || !endianPing)
        {
            fputs("Ping-pong failed.\n", stderr);
            return false;
        }
        fputs("Connection established.\n", stderr);
        play = PlayBob;
        return true;
    }
}

struct AlwaysNotNoisy
{
    AlwaysNotNoisy &operator ++ ()
    {
        return *this;
    }
    AlwaysNotNoisy operator ++ (int)
    {
        return *this;
    }
    bool operator * () const
    {
        return true;
    }
};

int PlayAlice()
{
    Networking::SocketWrappers::SocketConsumer pipe = socketUnique.RawValue();
    Zp x;
    if (!LoadZp(x, stdin))
    {
        fputs("Could not input coefficient x.\n", stderr);
        return -4;
    }
    /* Sample random b'. */
    for (auto &z : messageVectorTmp)
        z = UZp(randSrc);
    /* Sample random r'. */
    for (auto &z : randomVector)
        z = UZp(randSrc);
        fputs("Receiving E(r,a)+e from Bob.\n", stderr);
    if (!pipe.Receive(sizeof encodedVector, encodedVector))
    {
        fputs("Could not receive E(r,a)+e from Bob.\n", stderr);
        return -4;
    }
    /* Compute E(xr+r',xa+b')+xe = x(E(r,a)+e)+E(r',b'). */
    for (auto &z : encodedVector)
        z *= x;
    OuterCode.EncodeBothParts(encodedVector, AlwaysNotNoisy(), randomVector);
    InnerCode.Encode(encodedVector + U, AlwaysNotNoisy(), messageVectorTmp);
    fputs("Sending E(xr+r',xa+b') to Bob.\n", stderr);
    if (!pipe.Send(sizeof encodedVector, encodedVector))
    {
        fputs("Could not emulate OT of E(xr+r', xa+b') with Bob.\n", stderr);
        return -4;
    }
    if (!pipe.Send(sizeof encodedVector, encodedVector))
    {
        fputs("Could not send E(xr+r', xa+b') to Bob.\n", stderr);
        return -4;
    }
    uint8_t didBobSucceed;
    fputs("Receiving didBobSucceed from Bob.\n", stderr);
    if (!pipe.Receive(1, &didBobSucceed))
    {
        fputs("Could not receive whether Bob successfully decoded the result.\n", stderr);
        return -4;
    }
    if (!didBobSucceed)
    {
        puts("Bob failed to decode the mess.");
        return -5;
    }
    fputs("Receiving b+xa+b from Bob.\n", stderr);
    if (!pipe.Receive(sizeof messageVector, messageVector))
    {
        fputs("Could not receive b+xa+b' from Bob.\n", stderr);
        return -4;
    }
    uint8_t aliceDisconnects = 42;
    fputs("Sending signal of disconnection to Bob.\n", stderr);
    if (!pipe.Send(1, &aliceDisconnects))
    {
        fputs("Could not send signal of disconnection to Bob.\n", stderr);
        return -4;
    }
    for (unsigned i = 0u; i != W; ++i)
        printf("%u\n", (unsigned)(messageVector[i] - messageVectorTmp[i]));
    fputs("Done.\n", stderr);
    return 0;
}

struct
{
    Zp operator () (Zp z) const
    {
        return z.Inverse();
    }
} const InvertZp;

int PlayBob()
{
    Networking::SocketWrappers::SocketConsumer pipe = socketUnique.RawValue();
    for (auto &z : messageVector)
        if (!LoadZp(z, stdin))
        {
            fputs("Could not read a.\n", stderr);
            return -4;
        }
    for (auto &z : messageVectorTmp)
        if (!LoadZp(z, stdin))
        {
            fputs("Could not read b.\n", stderr);
            return -4;
        }
    memset(notNoisyVector, true, sizeof notNoisyVector);
    /* Sample random e. */
    Encoding::Erasure::EraseSubsetExact(
        notNoisyVector, notNoisyVector + U,
        U / 4, randSrc);
    Encoding::Erasure::EraseSubsetExact(
        notNoisyVector + U, notNoisyVector + U + V,
        V / 4, randSrc);
    /* Sample random r'. */
    for (auto &z : randomVector)
        z = UZp(randSrc);
    /* Compute E(r,a). */
    memset(encodedVector, 0, sizeof encodedVector);
    OuterCode.EncodeBothParts(encodedVector, notNoisyVector, randomVector);
    InnerCode.Encode(encodedVector + U, notNoisyVector + U, messageVector);
    /* Compute E(r,a)+e. */
    for (unsigned i = 0u; i != U + V; ++i)
        if (!notNoisyVector[i])
            encodedVector[i] = UZp(randSrc);
    fputs("Sending E(r,a)+e to Alice.\n", stderr);
    if (!pipe.Send(sizeof encodedVector, encodedVector))
    {
        fputs("Could not send E(r,a)+e to Alice.\n", stderr);
        return -4;
    }
    fputs("Emulating OT.\n", stderr);
    if (!pipe.Skip(sizeof encodedVector))
    {
        fputs("Could not emulate OT with Alice.\n", stderr);
        return -4;
    }
    fputs("Receiving E(xr+r',xa+b) from Alice.\n", stderr);
    if (!pipe.Receive(sizeof encodedVector, encodedVector))
    {
        fputs("Could not receive E(xr+r',xa+b') from Alice.\n", stderr);
        return -4;
    }
    uint8_t didBobSucceed = 0;
    if (!OuterCode.DecodeFromUpperPartAutomatic(
        encodedVector,
        notNoisyVector,
        randomVector, InvertZp))
    {
        fputs("Outer code decoding failed.\n", stderr);
        pipe.Send(1, &didBobSucceed);
        return -4;
    }
    for (auto &z : randomVector)
        z = -z;
    OuterCode.EncodeLowerPart(encodedVector + U, notNoisyVector + U, randomVector);
    memset(messageVector, 0, sizeof messageVector);
    if (!InnerCode.DecodeDestructive(
        solvedVector, solvedVector + W,
        messageVector,
        notNoisyVector + U, notNoisyVector + U + V,
        encodedVector + U, encodedVector + U + V))
    {
        fputs("Inner code decoding failed.\n", stderr);
        pipe.Send(1, &didBobSucceed);
        return -4;
    }
    didBobSucceed = ~0;
    if (!pipe.Send(1, &didBobSucceed))
    {
        fputs("Could not send signal of successful decoding to Alice.\n", stderr);
        return -4;
    }
    for (unsigned i = 0u; i != W; ++i)
        messageVector[i] += messageVectorTmp[i];
    fputs("Sending b+xa+b' to Alice.\n", stderr);
    if (!pipe.Send(sizeof messageVector, messageVector))
    {
        fputs("Coult not send b+xa+b' to Alice.\n", stderr);
        return -4;
    }
    uint8_t aliceDisconnects;
    fputs("Receiving signal of disconnection from Alice.\n", stderr);
    if (!pipe.Receive(1, &aliceDisconnects) || aliceDisconnects != 42)
    {
        fputs("Could not receive signal of disconnection from Alice.\n", stderr);
        return -4;
    }
    fputs("Done.\n", stderr);
    return 0;
}
