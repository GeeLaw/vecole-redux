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

constexpr unsigned SmallSampleSize = 500;
constexpr unsigned LargeSampleSize = 20000;

typedef Cryptography::Z<4294967291u> Zp;
typedef std::mt19937 RNG;

typedef FastSparseLinearCode<Zp> FSLCode;

char const *ofn;
unsigned k, d, u, uErased, v, vErased;
bool *boolArray;
Zp *plain, *encoded, *decoded, *tempMatrix;
RNG rng((unsigned)time(nullptr));
std::uniform_int_distribution<uint32_t> UZp(0u, 4294967290u);

bool ParseCommandLine(int const argc, char ** const argv);
void PrintUsage();
double TestSparseCode(FSLCode const &code, unsigned const count);

struct
{
    void operator () (Zp const zp, FILE *fp) const
    {
        fprintf(fp, "%ju\n", (uintmax_t)(uint32_t)zp);
    }
} const SaveZp;

int main(int argc, char **argv)
{
    if (!ParseCommandLine(argc, argv))
    {
        PrintUsage();
        return -1;
    }
    FSLCode code;
    code.K = k;
    code.D = d;
    code.U = u;
    code.V = v;
    double bestSuccessRate = 0.0;
    char outputFileName[40];
    unsigned candidateIndex = 0u;
    while (true)
    {
        code.Resample(rng, UZp);
        double successRate = TestSparseCode(code, SmallSampleSize);
        if (successRate < 0)
            return -1;
        if (successRate <= bestSuccessRate)
            continue;
        fprintf(stderr, "\nFound a good candidate (%f%%, sample size = %u), testing more.\n", successRate * 100, SmallSampleSize);
        successRate = TestSparseCode(code, LargeSampleSize);
        if (successRate < 0)
            return -1;
        if (successRate <= bestSuccessRate)
        {
            fputs("Further test finished: discarded.\n", stderr);
            continue;
        }
        fprintf(stderr, "Further test finished: saving (%f%%, sample size = %u).\n", successRate * 100, LargeSampleSize);
        sprintf(outputFileName, "%s.%03u.sparse", ofn, candidateIndex++);
        FILE *fp = fopen(outputFileName, "w");
        if (!fp)
        {
            fprintf(stderr, "Could not open %s for writing.\n", outputFileName);
            return -1;
        }
        code.SaveTo(fp, SaveZp);
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
    if (argc < 3 || argc > 6)
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
    if (!ParseUInt(argv[2], "k", &k, 100, 300))
        return false;
    d = 10;
    u = (k + 2) / 3 * 4;
    v = k * k;
    if (argc >= 4 && !ParseUInt(argv[3], "d", &d, 5, 50))
        return false;
    if (argc >= 5 && !ParseUInt(argv[4], "u", &u, u, 10 * u))
        return false;
    if (argc >= 6 && !ParseUInt(argv[5], "v", &v, k, k * k * k))
        return false;
    uErased = u / 4;
    vErased = v / 4;
    boolArray = new bool[u + v];
    plain = new Zp[k];
    encoded = new Zp[u + v];
    decoded = new Zp[k];
    tempMatrix = new Zp[(u - uErased) * (k + 1u)];
    return true;
}

void PrintUsage()
{
    fputs
    (
        "Usage: sparsegen ofn k [d] [u] [v]\n\n"
        "  ofn: the prefix of output file.\n"
        "    k: the length of random vector, 182 or 240 (100 ~ 300).\n"
        "    d: the sparsity parameter, default = 10 (5 ~ 50).\n"
        "    u: the length of top rows,\n"
        "       default = minimum = 4*ceiling(k/3),\n"
        "       maximum = 10 * default.\n"
        "    v: the length of bottom rows, default = k*k,\n"
        "       minimum = k, maximum = k * k * k.\n",
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
