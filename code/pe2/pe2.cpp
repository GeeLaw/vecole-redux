#define _CRT_SECURE_NO_WARNINGS

#include"pch.hpp"

using namespace Cryptography;
using namespace Cryptography::ArithmeticCircuits;
using namespace Cryptography::Goldreich;
using namespace Encoding::Erasure;
using namespace Encoding::LubyTransform;
using namespace Encoding::SparseLinearCode;
using namespace Networking;

using Clock = std::chrono::high_resolution_clock;

#include"common.hpp"
#include"alice.hpp"
#include"bob.hpp"

int main(int argc, char **argv)
{
    auto parseCommandLine = ParseCommandLine(argc, argv);
    if (parseCommandLine > 0)
        PrintUsage();
    if (parseCommandLine != 0)
        return parseCommandLine;
    Networking::SocketWrappers::SocketInitialiser init;
    if (!init.Good())
    {
        PrintHelpfulInformation("Could not initialise socket.");
        return -55;
    }
    return CommandLineParameters.IsAlice
        ? PlayAlice()
        : PlayBob();
}
