#define _CRT_SECURE_NO_WARNINGS

#include<cstdio>
#include"../library/sparse_code.hpp"
#include"../library/erasure.hpp"
#include"../library/cryptography.hpp"
#include<random>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<algorithm>

using namespace Encoding::Erasure;
using namespace Encoding::SparseLinearCode;

constexpr unsigned LargeSampleSize = 20000;

typedef Cryptography::Z<4294967291u> Zp;
typedef std::mt19937 RNG;

typedef FastSparseLinearCode<Zp> FSLCode;

unsigned k, d, u, uErased, v, vErased;
bool *boolArray;
Zp *plain, *encoded, *decoded, *tempMatrix;
RNG rng((unsigned)time(nullptr));
std::uniform_int_distribution<uint32_t> UZp(0u, 4294967290u);

void PrintUsage();
double TestSparseCode(FSLCode const &code, unsigned const count);

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

int main(int argc, char **argv)
{
    FSLCode code;
    if (!code.LoadFrom(stdin, LoadZp))
    {
        PrintUsage();
        return -1;
    }
    k = code.K;
    d = code.D;
    u = code.U;
    v = code.V;
    uErased = u / 4;
    vErased = v / 4;
    boolArray = new bool[u + v];
    plain = new Zp[k];
    encoded = new Zp[u + v];
    decoded = new Zp[k];
    tempMatrix = new Zp[(u - uErased) * (k + 1u)];
    double successRate = TestSparseCode(code, LargeSampleSize);
    if (successRate < 0)
        return -1;
    printf("%f%%, sample size = %u\n", successRate * 100, LargeSampleSize);
    return 0;
}

void PrintUsage()
{
    fputs
    (
        "Usage: sparsegen < saved.sparse > success.rate\n",
        stderr
    );
}

struct
{
    Zp const operator () (Zp v) const
    {
        /* Toy implementation using Fermat's little theorem. */
        Zp result = 1u;
        for (auto p = 4294967289u; p; p >>= 1, v *= v)
            if (p & 1u)
                result *= v;
        return result;
    }
} const InverseZp;

double TestSparseCode(FSLCode const &code, unsigned const count)
{
    unsigned success = 0u;
    for (unsigned i = 0u; i != count; ++i)
    {
        memset(boolArray, true, (u + v) * sizeof(bool));
        EraseSubsetExact(boolArray, boolArray + u, uErased, rng);
        EraseSubsetExact(boolArray + u, boolArray + u + v, vErased, rng);
        for (unsigned j = 0u; j != k; ++j)
            plain[j] = UZp(rng);
        memset(encoded, 0, (u + v) * sizeof(Zp));
        memset(tempMatrix, 0, (u - uErased) * (k + 1u) * sizeof(Zp));
        code.EncodeBothParts(encoded,
            (bool const *)boolArray,
            (Zp const *)plain);
        if (!code.DecodeFromUpperPartDestructive
        (
            (Zp const *)encoded,
            (bool const *)boolArray,
            decoded,
            tempMatrix, InverseZp
        ))
            continue;
        for (unsigned j = 0u; j != k; ++j)
            if (decoded[j] != plain[j])
            {
                fputs("There is a mistake in sparse linear code algorithm (phase 1).\n", stderr);
                fprintf(stderr, "Index %u: was %u, decoded to %u.\n", j, (unsigned)plain[j], (unsigned)decoded[j]);
                return -1.0;
            }
            else
                decoded[j] = -decoded[j];
        code.EncodeLowerPart(encoded + u,
            (bool const *)boolArray + u,
            (Zp const *)decoded);
        for (unsigned j = 0u; j != v; ++j)
            if ((bool)(unsigned)encoded[u + j])
            {
                fputs("There is a mistake in sparse linear code algorithm (phase 2).\n", stderr);
                fprintf(stderr, "Index u+%u: derandomised to %u.\n", j, (unsigned)encoded[u + j]);
                return -1.0;
            }
        ++success;
    }
    return (double)success / count;
}
