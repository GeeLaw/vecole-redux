#ifndef GOLDREICH_HPP_
#define GOLDREICH_HPP_

#include<set>
#include<vector>
#include<random>
#include"helpers.hpp"

namespace Cryptography
{
namespace Goldreich
{
    template <typename TAllocSizeT = std::allocator<size_t>>
    struct GoldreichGraph
    {
        typedef std::vector<size_t, TAllocSizeT> SizeTVec;

        size_t InputLength, OutputLength;
        size_t A, B;
        SizeTVec Storage;

        template <typename TRandomGenerator>
        void Resample(TRandomGenerator &next)
        {
            std::uniform_int_distribution<size_t> indexDist(0, InputLength - 1);
            std::set<size_t> used;
            Storage.clear();
            Storage.resize((A + B) * OutputLength);
            auto it = Storage.data();
            for (size_t i = OutputLength; i; --i)
            {
                used.clear();
                for (size_t j = A + B; j; --j)
                {
                    size_t index;
                    do
                        index = indexDist(next);
                    while (!used.insert(index).second);
                    *it++ = index;
                }
            }
        }

        void SaveTo(FILE *fp) const
        {
            fprintf(fp, "%zu %zu %zu %zu %zu\n", InputLength, OutputLength,
                A, B, Storage.size());
            Helpers::SaveSizeTRange(
                Storage.data(),
                Storage.data() + Storage.size(),
                fp);
        }

        bool LoadFrom(FILE *fp)
        {
            size_t sz;
            if (fscanf(fp, "%zu%zu%zu%zu%zu", &InputLength, &OutputLength,
                &A, &B, &sz) != 5)
                return false;
            Storage.clear();
            Storage.resize(sz);
            return Helpers::LoadSizeTRange(
                Storage.data(),
                Storage.data() + Storage.size(),
                fp);
        }
    };
}
}

#endif // GOLDREICH_HPP_
