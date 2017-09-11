#define _CRT_SECURE_NO_WARNINGS

#include<random>
#include<cstdio>
#include"../library/cryptography.hpp"

typedef Cryptography::Z<4294967291u> Zp;

#define BATCHSZ 238328

std::uniform_int_distribution<uint32_t> dist(0u, 4294967290u);
std::random_device next;

Zp X[BATCHSZ];
Zp A[BATCHSZ], B[BATCHSZ];
Zp Z[BATCHSZ];

int main()
{
    freopen("x", "wb", stdout);
    for (int i = 0; i != BATCHSZ; ++i)
        printf("%ju\n", (uintmax_t)(uint32_t)(X[i] = dist(next)));
    freopen("a", "wb", stdout);
    for (int i = 0; i != BATCHSZ; ++i)
        printf("%ju\n", (uintmax_t)(uint32_t)(A[i] = dist(next)));
    freopen("b", "wb", stdout);
    for (int i = 0; i != BATCHSZ; ++i)
        printf("%ju\n", (uintmax_t)(uint32_t)(B[i] = dist(next)));
    freopen("stdans", "wb", stdout);
    for (int i = 0; i != BATCHSZ; ++i)
        printf("%ju\n", (uintmax_t)(uint32_t)(Z[i] = X[i] * A[i] + B[i]));
    return 0;
}
