#include"../library/goldreich.hpp"
#include<cstdio>
#include<ctime>

bool ParseCommandLine(int const argc, char ** const argv);
void PrintUsage();

typedef std::mt19937 RNG;

Cryptography::Goldreich::GoldreichGraph<> gg;
RNG rng((unsigned)time(nullptr));

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        PrintUsage();
        return -1;
    }
    gg.Resample(rng);
    gg.SaveTo(stdout);
    return 0;
}


bool ParseUInt(char const *arg, char const *name, unsigned *p, unsigned mini, unsigned maxi)
{
    if (sscanf(arg, "%u", p) != 1)
    {
        fprintf(stderr, "The format for %s is incorrect.\n", name);
        return false;
    }
    if (*p < mini || *p > maxi)
    {
        fprintf(stderr, "The allowed range of %s is [%u, %u].\n", name, mini, maxi);
        return false;
    }
    return true;
}

bool ParseCommandLine(int const argc, char ** const argv)
{
    if (argc < 4 || argc > 5)
        return false;
    if (!ParseUInt(argv[1], "a", &gg.A, 3, 50))
        return false;
    if (!ParseUInt(argv[2], "b", &gg.B, 3, 50))
        return false;
    if (!ParseUInt(argv[3], "i", &gg.InputLength,
        gg.A + gg.B + 100, 20000))
        return false;
    gg.OutputLength = gg.InputLength * gg.InputLength;
    if (argc >= 5 && !ParseUInt(argv[4], "o", &gg.OutputLength,
        gg.InputLength,
        gg.InputLength * gg.InputLength * gg.InputLength))
        return false;
    return true;
}

void PrintUsage()
{
    fputs
    (
        "Usage: goldgen a b i [o]\n\n"
        "    a: the additive arity, minimum 3, maximum 50.\n"
        "    b: the multiplicative arity, minimum 3, maximum 50.\n"
        "    i: the number of inputs, minimum a+b+100, maximum 20000.\n"
        "    o: optional, minimum i, maximum i*i*i, defaults to i*i",
        stderr
    );
}
