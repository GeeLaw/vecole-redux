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
        unsigned Column;
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
        unsigned D,
        unsigned count,
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
                for (unsigned i = 0u; i != D; ++i, ++entries)
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
        unsigned K,
        unsigned D,
        unsigned U,
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
        unsigned kPlus1 = K + 1u;
        unsigned validRows = 0u;
        for (; U--; ++encoded, ++notNoisy)
            if (*notNoisy)
            {
                for (unsigned i = 0u; i != D; ++i, ++entries)
                    matrix[validRows * kPlus1 + entries->Column] += entries->Value;
                matrix[validRows * kPlus1 + K] += *encoded;
                ++validRows;
            }
            else
                std::advance(entries, D);
        if (validRows < K)
            return false;
        /* elimination */
        for (unsigned i = 0u; ; ++i)
        {
            if (!(bool)matrix[i * kPlus1 + i])
            {
                bool success = false;
                for (unsigned j = i + 1u; j != validRows; ++j)
                    if ((bool)matrix[j * kPlus1 + i])
                    {
                        std::swap_ranges
                        (
                            matrix + i * kPlus1,
                            matrix + (i + 1u) * kPlus1,
                            matrix + j * kPlus1
                        );
                        success = true;
                        break;
                    }
                if (!success)
                    return false;
            }
            auto invLeading = inverse(matrix[i * kPlus1 + i]);
            for (unsigned k = i; k <= K; ++k)
                matrix[i * kPlus1 + k] *= invLeading;
            if (i + 1u == K)
                break;
            for (unsigned j = i + 1u; j != validRows; ++j)
                if ((bool)matrix[j * kPlus1 + i])
                {
                    auto leading = matrix[j * kPlus1 + i];
                    for (unsigned k = i + 1u; k <= K; ++k)
                        matrix[j * kPlus1 + k] -= leading * matrix[i * kPlus1 + k];
                    matrix[j * kPlus1 + i] = 0;
                }
        }
        /* substitution */
        for (unsigned i = K - 1u; i != 0u; --i)
            for (unsigned j = i - 1u; j != 0u - 1u; --j)
                if ((bool)matrix[j * kPlus1 + i])
                    matrix[j * kPlus1 + K] -=
                        std::move(matrix[j * kPlus1 + i])
                        * matrix[i * kPlus1 + K];
        /* move the results to decoded */
        matrix += K;
        for (unsigned i = 0u; i != K; ++i, ++decoded, matrix += K + 1u)
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

        unsigned K;
        unsigned D;
        unsigned U;
        unsigned V;
        EntryVec Entries;

        template
        <
            typename TRandomGenerator,
            typename TRingDistribution
        >
        void Resample(TRandomGenerator &next, TRingDistribution &valDist)
        {
            std::uniform_int_distribution<unsigned> columnDist(0, K - 1u);
            Entries.clear();
            Entries.reserve((U + V) * D);
            std::set<unsigned> dedup;
            for (unsigned i = U + V; i != 0u; --i)
            {
                dedup.clear();
                for (unsigned j = D; j != 0u; ++j)
                {
                    unsigned col;
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
            matrix.resize(U * (K + 1u));
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
            auto j = i + Entries.size();
            fprintf(fp, "%u %u %u %u %u\n", K, D, U, V, (unsigned)(j - i));
            for (; i != j; ++i)
            {
                fprintf(fp, "%u ", i->Column);
                saveRing(i->Value, fp);
                fputc('\n', fp);
            }
        }

        template <typename TLoadRing>
        bool LoadFrom(FILE *fp, TLoadRing loadRing)
        {
            unsigned sz;
            if (fscanf(fp, "%u%u%u%u%u", &K, &D, &U, &V, &sz) != 5)
                return false;
            Entries.clear();
            Entries.reserve(sz);
            unsigned uInt;
            TRing ringElem;
            while (sz--)
            {
                if (fscanf(fp, "%u", &uInt) != 1)
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
