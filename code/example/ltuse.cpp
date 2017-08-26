#define _CRT_SECURE_NO_WARNINGS

#include<cstdio>
#include"../library/luby.hpp"
#include"../library/erasure.hpp"
#include<random>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<algorithm>

using namespace Encoding::Erasure;
using namespace Encoding::LubyTransform;

constexpr unsigned LargeSampleSize = 20000;

typedef uint32_t Zp;
typedef std::mt19937 RNG;

unsigned w;
unsigned v, vErased;
bool *boolArray;
Zp *plain, *encoded, *decoded;
RNG rng((unsigned)time(nullptr));
std::uniform_int_distribution<uint32_t> UZp(0u, 4294967295u);

void PrintUsage();
double TestLTCode(LTCode<> const &code, unsigned const count);

int main(int argc, char **argv)
{
    LTCode<> code;
    if (!code.LoadFrom(stdin))
    {
        PrintUsage();
        return -1;
    }
    w = code.InputSymbolSize;
    v = code.Bins.size();
    vErased = v / 4;
    boolArray = new bool[w + v];
    plain = new Zp[w];
    encoded = new Zp[v];
    decoded = new Zp[w];
    double successRate = TestLTCode(code, LargeSampleSize);
    if (successRate < 0)
        return -1;
    printf("%f%%, sample size = %u\n", successRate * 100, LargeSampleSize);
    return 0;
}

void PrintUsage()
{
    fputs
    (
        "Usage: ltuse < saved.luby > success.rate\n",
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
        for (unsigned j = 0u; j != w; ++j)
            decoded[j] = 0;
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
