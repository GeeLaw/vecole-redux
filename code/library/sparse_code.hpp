#ifndef SPARSE_CODE_HPP_
#define SPARSE_CODE_HPP_

#include<vector>
#include<set>
#include<random>
#include<iterator>
#include<utility>

namespace Encoding
{
namespace SparseLinearCode
{
    template <typename TRing>
    struct SparseMatrixEntry
    {
        size_t Column;
        TRing Value;
    };

    template
    <
        typename TForwardInputOutputIt1,
        typename TInputIt2,
        typename TRandomAccessInputIt3,
        typename TForwardInputIt4
    >
    void SparseEncode
    (
        size_t D,
        size_t count,
        /* encoded = TRing[count] */
        TForwardInputOutputIt1 &encoded,
        /* notNoisy = bool[count] */
        TInputIt2 &notNoisy,
        TRandomAccessInputIt3 decoded,
        TForwardInputIt4 &entries
    )
    {
        for (; count--; ++encoded, ++notNoisy)
            if (*notNoisy)
                for (size_t i = 0; i != D; ++i, ++entries)
                    *encoded += entries->Value * decoded[entries->Column];
            else
                std::advance(entries, D);
    }

    struct
    {
        template <typename T>
        T const operator () (T &&t) const
        {
            return 1 / std::forward<T>(t);
        }
    } const DefaultInverseFunctor;

    template
    <
        typename TInputIt1,
        typename TInputIt2,
        typename TOutputIt3,
        typename TForwardInputIt4,
        typename TRandomAccessInputOutputIt5,
        typename TInverseFunctor
    >
    bool SparseDecodeDestructive
    (
        size_t K,
        size_t D,
        size_t U,
        /* encoded = TRing[U] */
        TInputIt1 encoded,
        /* notNoisy = bool[U] */
        TInputIt2 notNoisy,
        /* decoded = TRing[K] */
        TOutputIt3 decoded,
        TForwardInputIt4 entries,
        /* matrix = TRing[#[notNoisy] * (K + 1)], initialised to 0 */
        TRandomAccessInputOutputIt5 matrix,
        TInverseFunctor inverse = DefaultInverseFunctor
    )
    {
        size_t kPlus1 = K + 1;
        size_t validRows = 0;
        for (; U--; ++encoded, ++notNoisy)
            if (*notNoisy)
            {
                for (size_t i = 0; i != D; ++i, ++entries)
                    matrix[validRows * kPlus1 + entries->Column] += entries->Value;
                matrix[validRows * kPlus1 + K] += *encoded;
                ++validRows;
            }
            else
                std::advance(entries, D);
        if (validRows < K)
            return false;
        /* elimination */
        for (size_t i = 0; ; ++i)
        {
            if (!(bool)matrix[i * kPlus1 + i])
            {
                bool success = false;
                for (size_t j = i + 1; j != validRows; ++j)
                    if ((bool)matrix[j * kPlus1 + i])
                    {
                        std::swap_ranges
                        (
                            matrix + i * kPlus1,
                            matrix + (i + 1) * kPlus1,
                            matrix + j * kPlus1
                        );
                        success = true;
                        break;
                    }
                if (!success)
                    return false;
            }
            auto invLeading = inverse(matrix[i * kPlus1 + i]);
            for (size_t k = i; k <= K; ++k)
                matrix[i * kPlus1 + k] *= invLeading;
            if (i + 1 == K)
                break;
            for (size_t j = i + 1; j != validRows; ++j)
                if ((bool)matrix[j * kPlus1 + i])
                {
                    auto leading = matrix[j * kPlus1 + i];
                    for (size_t k = i + 1; k <= K; ++k)
                        matrix[j * kPlus1 + k] -= leading * matrix[i * kPlus1 + k];
                    matrix[j * kPlus1 + i] = 0;
                }
        }
        /* substitution */
        for (size_t i = K - 1; i; --i)
            for (size_t j = i - 1; j != (size_t)0 - (size_t)1; --j)
                if ((bool)matrix[j * kPlus1 + i])
                    matrix[j * kPlus1 + K] -=
                        std::move(matrix[j * kPlus1 + i])
                        * matrix[i * kPlus1 + K];
        /* move the results to decoded */
        matrix += K;
        for (size_t i = 0; i != K; ++i, ++decoded, matrix += K + 1)
            *decoded = std::move(*matrix);
        return true;
    }

    template
    <
        typename TRing,
        typename TAllocEntry = std::allocator<SparseMatrixEntry<TRing>>
    >
    struct FastSparseLinearCode
    {
        typedef SparseMatrixEntry<TRing> Entry;
        typedef std::vector<Entry, TAllocEntry> EntryVec;

        size_t K;
        size_t D;
        size_t U;
        size_t V;
        EntryVec Entries;

        template
        <
            typename TRandomGenerator,
            typename TRingDistribution
        >
        void Resample(TRandomGenerator &next, TRingDistribution &valDist)
        {
            std::uniform_int_distribution<size_t> columnDist(0, K - 1);
            Entries.clear();
            Entries.reserve((U + V) * D);
            std::set<size_t> dedup;
            for (size_t i = U + V; i; --i)
            {
                dedup.clear();
                for (size_t j = D; j; --j)
                {
                    size_t col;
                    do
                        col = columnDist(next);
                    while (!dedup.insert(col).second);
                    Entries.push_back({ col, valDist(next) });
                }
            }
        }

        template
        <
            typename TForwardInputOutputIt1,
            typename TForwardInputIt2,
            typename TRandomAccessInputIt3
        >
        void EncodeBothParts
        (
            /* encoded = TRing[U + V] */
            TForwardInputOutputIt1 encoded,
            /* notNoisy = bool[U + V] */
            TForwardInputIt2 notNoisy,
            TRandomAccessInputIt3 decoded
        ) const
        {
            auto entries = Entries.data();
            SparseEncode(D, U + V, encoded,
                notNoisy, decoded, entries);
        }

        template
        <
            typename TForwardInputOutputIt1,
            typename TForwardInputIt2,
            typename TRandomAccessInputIt3
        >
        void EncodeUpperPart
        (
            /* encoded = TRing[U] */
            TForwardInputOutputIt1 encoded,
            /* notNoisy = bool[U] */
            TForwardInputIt2 notNoisy,
            TRandomAccessInputIt3 decoded
        ) const
        {
            auto entries = Entries.data();
            SparseEncode(D, U, encoded,
                notNoisy, decoded, entries);
        }

        template
        <
            typename TForwardInputOutputIt1,
            typename TForwardInputIt2,
            typename TRandomAccessInputIt3
        >
        void EncodeLowerPart
        (
            /* encoded = TRing[V] */
            TForwardInputOutputIt1 encoded,
            /* notNoisy = bool[V] */
            TForwardInputIt2 notNoisy,
            TRandomAccessInputIt3 decoded
        ) const
        {
            auto entries = Entries.data() + D * U;
            SparseEncode(D, V, encoded,
                notNoisy, decoded, entries);
        }

        template
        <
            typename TInputIt1,
            typename TInputIt2,
            typename TOutputIt3,
            typename TRandomAccessInputOutputIt5,
            typename TInverseFunctor
        >
        bool DecodeFromUpperPartDestructive
        (
            TInputIt1 encoded,
            TInputIt2 notNoisy,
            TOutputIt3 decoded,
            TRandomAccessInputOutputIt5 matrix,
            TInverseFunctor inverse = DefaultInverseFunctor
        ) const
        {
            return SparseDecodeDestructive
            (
                K, D, U,
                encoded, notNoisy,
                decoded, Entries.data(),
                matrix, inverse
            );
        }

        template
        <
            typename TInputIt1,
            typename TInputIt2,
            typename TOutputIt3,
            typename TInverseFunctor
        >
        bool DecodeFromUpperPartAutomatic
        (
            TInputIt1 encoded,
            TInputIt2 notNoisy,
            TOutputIt3 decoded,
            TInverseFunctor const &inverse = DefaultInverseFunctor
        ) const
        {
            std::vector<TRing> matrix;
            matrix.resize(U * (K + 1));
            return SparseDecodeDestructive
            (
                K, D, U,
                encoded, notNoisy,
                decoded, Entries.data(),
                matrix.data(), inverse
            );
        }

        template <typename TSaveRing>
        void SaveTo(FILE *fp, TSaveRing saveRing) const
        {
            auto i = Entries.data();
            auto sz = Entries.size();
            fprintf(fp, "%zu %zu %zu %zu %zu\n", K, D, U, V, sz);
            for (auto j = i + sz; i != j; ++i)
            {
                fprintf(fp, "%zu\n", i->Column);
                saveRing(i->Value, fp);
            }
        }

        template <typename TLoadRing>
        bool LoadFrom(FILE *fp, TLoadRing loadRing)
        {
            size_t sz;
            if (fscanf(fp, "%zu%zu%zu%zu%zu", &K, &D, &U, &V, &sz) != 5)
                return false;
            Entries.clear();
            Entries.reserve(sz);
            size_t uInt;
            TRing ringElem;
            while (sz--)
            {
                if (fscanf(fp, "%zu", &uInt) != 1)
                    return false;
                if (!loadRing(ringElem, fp))
                    return false;
                Entries.push_back({ uInt, ringElem });
            }
            return true;
        }
    };
}
}

#endif // SPARSE_CODE_HPP_
