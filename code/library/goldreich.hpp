#ifndef GOLDREICH_HPP_
#define GOLDREICH_HPP_

#include<set>
#include<vector>
#include<random>

namespace Cryptography
{
namespace Goldreich
{
    template <typename TAllocUnsigned = std::allocator<unsigned>>
    struct GoldreichGraph
    {
        typedef std::vector<unsigned, TAllocUnsigned> UnsignedVec;

        unsigned InputLength, OutputLength;
        unsigned A, B;
        UnsignedVec Storage;

        template <typename TRandomGenerator>
        void Resample(TRandomGenerator &next)
        {
            std::uniform_int_distribution<unsigned> indexDist(0u, InputLength - 1u);
            std::set<unsigned> used;
            Storage.clear();
            Storage.resize((A + B) * OutputLength);
            auto it = Storage.data();
            for (unsigned i = OutputLength; i != 0u; --i)
            {
                used.clear();
                for (unsigned j = A + B; j != 0u; --j)
                {
                    unsigned index;
                    do
                        index = indexDist(next);
                    while (!used.insert(index).second);
                    *it++ = index;
                }
            }
        }

        void SaveTo(FILE *fp) const
        {
            fprintf(fp, "%u %u %u %u %u\n", InputLength, OutputLength,
                A, B, (unsigned)Storage.size());
            for (auto i = Storage.data(), j = Storage.data() + Storage.size();
                i != j; ++i)
                fprintf(fp, "%u ", *i);
            fputc('\n', fp);
        }

        bool LoadFrom(FILE *fp)
        {
            unsigned sz;
            if (fscanf(fp, "%u%u%u%u%u", &InputLength, &OutputLength,
                &A, &B, &sz) != 5)
                return false;
            Storage.clear();
            Storage.resize(sz);
            for (auto i = Storage.data(); sz--; ++i)
                if (fscanf(fp, "%u", i) != 1)
                    return false;
            return true;
        }
    };
}
}

#endif // GOLDREICH_HPP_
