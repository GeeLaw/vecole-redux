#define _CRT_SECURE_NO_WARNINGS

#include<cstdio>
#include"../library/luby.hpp"
#include"../library/erasure.hpp"
#include"../library/cryptography.hpp"
#include<random>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<algorithm>

using namespace Encoding::Erasure;
using namespace Encoding::LubyTransform;

constexpr unsigned SmallSampleSize = 500;
constexpr unsigned LargeSampleSize = 20000;

typedef Cryptography::Z<4294967291u> Zp;
typedef std::mt19937 RNG;

char const *ofn;
unsigned w;
double c = 0.5;
unsigned v, vErased;
bool *boolArray;
Zp *plain, *encoded, *decoded;
RNG rng((unsigned)time(nullptr));
std::uniform_int_distribution<uint32_t> UZp(0u, 4294967290u);

bool ParseCommandLine(int const argc, char ** const argv);
void PrintUsage();
double TestLTCode(LTCode<> const &code, unsigned const count);

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        PrintUsage();
        return -1;
    }
    RobustSolitonDistribution rsd;
    rsd.InputSymbolSize = w;
    rsd.Delta = 0.01;
    rsd.C = c;
    for (rsd.InvalidateCache();
        rsd.OutputSymbolSizeCached <= v;
        rsd.C += 1e-5, rsd.InvalidateCache())
        ;
    for (; rsd.OutputSymbolSizeCached > v;
        rsd.C -= 1e-5, rsd.InvalidateCache())
        ;
    fprintf(stderr, "Found c = %f giving v = %zu.\n",
        rsd.C, rsd.OutputSymbolSizeCached);
    LTCode<> code;
    double bestSuccessRate = 0.0;
    char outputFileName[40];
    unsigned candidateIndex = 0u;
    while (true)
    {
        CreateLTCode(rsd, code, rng);
        std::sort(code.Bins.data(), code.Bins.data() + code.Bins.size());
        double successRate = TestLTCode(code, SmallSampleSize);
        if (successRate < 0)
            return -1;
        if (successRate <= bestSuccessRate)
            continue;
        fprintf(stderr, "\nFound a good candidate (%f%%, sample size = %u), testing more.\n", successRate * 100, SmallSampleSize);
        successRate = TestLTCode(code, LargeSampleSize);
        if (successRate < 0)
            return -1;
        if (successRate <= bestSuccessRate)
        {
            fputs("Further test finished: discarded.\n", stderr);
            continue;
        }
        fprintf(stderr, "Further test finished: saving (%f%%, sample size = %u).\n", successRate * 100, LargeSampleSize);
        sprintf(outputFileName, "%s.%03u.luby", ofn, candidateIndex++);
        FILE *fp = fopen(outputFileName, "w");
        if (!fp)
        {
            fprintf(stderr, "Could not open %s for writing.\n", outputFileName);
            return -1;
        }
        code.SaveTo(fp);
        fclose(fp);
        fputs("Further test finished: saved.\n", stderr);
        bestSuccessRate = successRate;
    }
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
    ofn = argv[1];
    if (ofn[0] == '\0'
        || ofn[1] == '\0'
        || ofn[2] == '\0')
    {
        fputs("The minimal length of ofn is 3.\n", stderr);
        return false;
    }
    for (char const *i = ofn; *i; ++i)
        if (!((*i >= '0' && *i <= '9')
            || (*i >= 'a' && *i <= 'z')
            || (*i >= 'A' && *i <= 'Z')
            || *i == '_'))
        {
            fputs("The allowed characters for ofn are 0-9, a-z, A-Z and _.\n", stderr);
            return false;
        }
        else if (i - ofn > 20)
        {
            fputs("The maximal length of ofn is 20.\n", stderr);
            return false;
        }
    if (!ParseUInt(argv[2], "w", &w, 5000, 40000))
        return false;
    if (!ParseUInt(argv[3], "v", &v, 2 * w, 4 * w))
        return false;
    if (argc >= 5)
    {
        if (sscanf(argv[4], "%lf", &c) != 1)
        {
            fputs("The format for c is incorrect.\n", stderr);
            return false;
        }
        if (c < 0.5 || c > 20)
        {
            fputs("The allowed range of c is [0.5, 20].\n", stderr);
            return false;
        }
    }
    vErased = v / 4;
    boolArray = new bool[w + v];
    plain = new Zp[w];
    encoded = new Zp[v];
    decoded = new Zp[w];
    return true;
}

void PrintUsage()
{
    fputs
    (
        "Usage: ltgen ofn w v [c]\n\n"
        "  ofn: the prefix of output file.\n"
        "    w: the number of inputs to LT code (10000 for k = 182, 20000 for k = 240).\n"
        "    v: the number of outputs from LT code (33124 for k = 182, 57600 for k = 240).\n"
        "    c: optional, minimum c in LT code, defaults to 0.5.",
        stderr
    );
}

double TestLTCode(LTCode<> const &code, unsigned const count)
{
    LTCode<> surrogate;
    unsigned success = 0u;
    for (unsigned i = 0u; i != count; ++i)
    {
        memset(boolArray, false, w * sizeof(bool));
        memset(boolArray + w, true, v * sizeof(bool));
        for (unsigned j = 0u; j != w; ++j)
            plain[j] = UZp(rng);
        EraseSubsetExact(boolArray + w, boolArray + w + v, vErased, rng);
        for (unsigned j = 0u; j != v; ++j)
            encoded[j] = 0;
        code.Encode(encoded, (bool const *)boolArray + w, (Zp const *)plain);
        surrogate = code;
        if (!surrogate.DecodeDestructive
        (
            boolArray, boolArray + w,
            decoded,
            boolArray + w, boolArray + w + v,
            encoded, encoded + v
        ))
            continue;
        for (unsigned j = 0u; j != w; ++j)
            if (decoded[j] != plain[j])
            {
                fputs("There is a mistake in Luby Transform algorithm.\n", stderr);
                fprintf(stderr, "Index %u: was %u, decoded to %u.\n", j, (unsigned)plain[j], (unsigned)decoded[j]);
                return -1.0;
            }
        ++success;
    }
    return (double)success / count;
}
