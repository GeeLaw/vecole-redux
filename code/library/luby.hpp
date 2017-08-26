#ifndef LUBY_HPP_
#define LUBY_HPP_

#include<cmath>
#include<vector>
#include<set>
#include<utility>
#include<random>
#include<cstring>

namespace Encoding
{
namespace LubyTransform
{
    struct RobustSolitonDistribution
    {
        /* k in LT Codes (FOCS 02)
         * w in vector OLE
         */
        unsigned InputSymbolSize;
        /* c in LT Codes */
        double C;
        /* delta in LT Codes */
        double Delta;
        /* R in LT Codes */
        double RCached;
        /* k/R in LT Codes */
        unsigned KOverRCached;
        /* beta in LT Codes */
        double BetaCached;
        /* v in LT Codes
         * rounded up to a multiple of 4
         */
        unsigned OutputSymbolSizeCached;
        /* Call this method after non-cache
         * values have been edited.
         */
        void InvalidateCache()
        {
            RCached = C * std::log(InputSymbolSize / Delta) * std::sqrt((double)InputSymbolSize);
            KOverRCached = (unsigned)(InputSymbolSize / RCached + 0.5);
            auto beta = std::log(RCached / Delta);
            for (unsigned i = KOverRCached - 1u; i != 0u; --i)
                beta += 1.0 / i;
            BetaCached = 1.0 + beta * RCached / InputSymbolSize;
            OutputSymbolSizeCached = (unsigned)(InputSymbolSize * BetaCached + 0.5);
            /* Round up to a multiple of 4 */
            OutputSymbolSizeCached += ((4 - (OutputSymbolSizeCached & 3)) & 3);
        }
        /* Ideal ingredient, i is 1-based. */
        double Rho(unsigned i) const
        {
            if (i == 1u)
                return 1.0 / InputSymbolSize;
            if (i > InputSymbolSize)
                return 0.0;
            return 1.0 / i / (i - 1u);
        }
        /* Added part for robust distribution. */
        double Tau(unsigned i) const
        {
            if (i < KOverRCached)
                return RCached / i / InputSymbolSize;
            if (i > KOverRCached)
                return 0.0;
            return RCached * std::log(RCached / Delta) / InputSymbolSize;
        }
        /* The distribution itself. */
        double Mu(unsigned i) const
        {
            return (Rho(i) + Tau(i)) / BetaCached;
        }
        /* r in [0, 1]
         * increasing function of r
         */
        unsigned SampleDegree(double r) const
        {
            if (r >= 1.0)
                return InputSymbolSize;
            if (r <= 0.0)
                return 1u;
            unsigned d = 0u;
            for (auto s = 0.0; s < r && d != InputSymbolSize; s += Mu(++d))
                ;
            return d;
        }
    };

    struct LubyBin
    {
        unsigned Index;
        unsigned Degree;
        template <typename TRandomIt>
        TRandomIt const GetBegin(TRandomIt const &i) const
        {
            return i + Index;
        }
        template <typename TRandomIt>
        TRandomIt const GetEnd(TRandomIt const &i) const
        {
            return i + Index + Degree;
        }
        friend bool operator < (LubyBin const &a, LubyBin const &b)
        {
            return a.Degree < b.Degree;
        }
    };

    template
    <
        typename TForwardInputIt1,
        typename TRandomAccessInputIt2,
        typename TForwardInputOutputIt3,
        typename TForwardInputIt4,
        typename TRandomAccessInputIt5
    >
    void LTEncode
    (
        /* [binsBegin, binsEnd) = LubyBin[encodedSize] */
        TForwardInputIt1 binsBegin,
        TForwardInputIt1 const &binsEnd,
        /* unsigned[] */
        TRandomAccessInputIt2 const &storage,
        /* F[encodedSize] = 0 */
        TForwardInputOutputIt3 encoded,
        /* bool[encodedSize] */
        TForwardInputIt4 notNoisy,
        /* F[decodedSize] */
        TRandomAccessInputIt5 const &decoded
    )
    {
        for (; binsBegin != binsEnd;
            ++binsBegin, ++encoded, ++notNoisy)
            if (*notNoisy)
                for (auto j = binsBegin->GetBegin(storage),
                    k = binsBegin->GetEnd(storage);
                    j != k; ++j)
                    *encoded += decoded[*j];
    }

    template
    <
        typename TRandomAccessInputOutputIt1,
        typename TBidirectionalInputOutputIt2,
        typename TRandomAccessInputOutputIt3,
        typename TRandomAccessInputOutputIt4,
        typename TBidirectionalInputIt5,
        typename TBidirectionalInputOutputIt6
    >
    bool LTDecodeDestructive
    (
        /* [solvedBegin, solvedEnd) = bool[decodedSize] = false, destructive */
        TRandomAccessInputOutputIt1 const &solvedBegin,
        TRandomAccessInputOutputIt1 const &solvedEnd,
        /* [binsBegin, binsEnd) = LubyBin[encodedSize], destructive */
        TBidirectionalInputOutputIt2 const &binsBegin,
        TBidirectionalInputOutputIt2 binsEnd,
        /* unsigned[], destructive */
        TRandomAccessInputOutputIt3 const &storage,
        /* F[decodedSize] = 0 */
        TRandomAccessInputOutputIt4 const &decoded,
        /* [notNoisyBegin, notNoisyEnd) = bool[encodedSize] */
        TBidirectionalInputIt5 notNoisyBegin,
        TBidirectionalInputIt5 notNoisyEnd,
        /* [encodedBegin, encodedEnd) = F[encodedSize], destructive */
        TBidirectionalInputOutputIt6 const &encodedBegin,
        TBidirectionalInputOutputIt6 encodedEnd
    )
    {
        unsigned remaining = solvedEnd - solvedBegin;
        /* Round 1:
         *     - release deg = 1
         *     - try releasing deg = 2
         *     - remove noisy positions
         * The do ... while (false) is there
         * to create a scope.
         */
        do
        {
            auto bins = binsBegin;
            auto encoded = encodedBegin;
            for (bool shouldAdvance = false, wasAdvancing = true;
                notNoisyBegin != notNoisyEnd;
                wasAdvancing = shouldAdvance, shouldAdvance = false)
            {
                if (wasAdvancing ? *notNoisyBegin++ : *--notNoisyEnd)
                {
                    auto const deg = bins->Degree;
                    if (deg == 1u)
                    {
                        unsigned t = storage[bins->Index];
                        if (!solvedBegin[t])
                        {
                            solvedBegin[t] = true;
                            --remaining;
                            decoded[t] = std::move(*encoded);
                        }
                    }
                    else if (deg == 2u)
                    {
                        unsigned const t1 = storage[bins->Index];
                        unsigned const t2 = storage[bins->Index + 1];
                        if (solvedBegin[t1] && !solvedBegin[t2])
                        {
                            solvedBegin[t2] = true;
                            --remaining;
                            decoded[t2] = std::move(*encoded) - decoded[t1];
                        }
                        else if (!solvedBegin[t1] && solvedBegin[t2])
                        {
                            solvedBegin[t1] = true;
                            --remaining;
                            decoded[t1] = std::move(*encoded) - decoded[t2];
                        }
                        else if (!solvedBegin[t1] && !solvedBegin[t2])
                        {
                            shouldAdvance = true;
                        }
                    }
                    else if (deg > 2u)
                    {
                        shouldAdvance = true;
                    }
                }
                /* Either move the last bin here (current bin is discarded)
                 * or advance the iterators (current bin is kept) */
                if (shouldAdvance)
                {
                    ++bins;
                    ++encoded;
                }
                else
                {
                    *bins = *--binsEnd;
                    *encoded = std::move(*--encodedEnd);
                }
            }
        } while (false);
        /* Round 2: solve the rest */
        for (bool newRelease = (remaining != solvedEnd - solvedBegin);
            newRelease && remaining != 0u; )
        {
            newRelease = false;
            auto bins = binsBegin;
            auto encoded = encodedBegin;
            for (; bins != binsEnd; )
            {
                /* Remove released positions from this codeword. */
                auto const storageBegin = bins->GetBegin(storage);
                auto storageEnd = bins->GetEnd(storage);
                for (auto storageIt = storageBegin;
                    storageIt != storageEnd; )
                    if (solvedBegin[*storageIt])
                    {
                        *encoded -= decoded[*storageIt];
                        *storageIt = *--storageEnd;
                    }
                    else
                    {
                        ++storageIt;
                    }
                unsigned newDeg = storageEnd - storageBegin;
                /* If remaining degree is 1:
                 *     - release this position
                 *     - set newRelease
                 *     - set newDeg = 0 for removal
                 */
                if (newDeg == 1u)
                {
                    solvedBegin[*storageBegin] = true;
                    --remaining;
                    decoded[*storageBegin] = std::move(*encoded);
                    newRelease = true;
                    newDeg = 0u;
                }
                /* Either move the last bin here (current bin is discarded)
                 * or advance the iterators (current bin is kept) */
                if (newDeg != 0u)
                {
                    bins->Degree = newDeg;
                    ++bins;
                    ++encoded;
                }
                else
                {
                    *bins = *--binsEnd;
                    *encoded = std::move(*--encodedEnd);
                }
            }
        }
        return remaining == 0u;
    }

    template
    <
        typename TAllocLubyBin = std::allocator<LubyBin>,
        typename TAllocUnsigned = std::allocator<unsigned>
    >
    struct LTCode
    {
        typedef std::vector<LubyBin, TAllocLubyBin> LubyBinVec;
        typedef std::vector<unsigned, TAllocUnsigned> UnsignedVec;

        unsigned InputSymbolSize;
        LubyBinVec Bins;
        UnsignedVec Storage;

        LTCode() = default;
        LTCode(LTCode const &) = default;
        LTCode(LTCode &&) = default;
        ~LTCode() = default;

        /* Does not propagate allocator.
         * Opitmized with memcpy.
         */
        LTCode &operator = (LTCode const &other)
        {
            if (this == &other)
                return *this;
            InputSymbolSize = other.InputSymbolSize;
            auto const szBins = other.Bins.size();
            Bins.resize(szBins);
            std::memcpy(Bins.data(), other.Bins.data(), szBins * sizeof(LubyBin));
            auto const szStorage = other.Storage.size();
            Storage.resize(szStorage);
            std::memcpy(Storage.data(), other.Storage.data(), szStorage * sizeof(unsigned));
            return *this;
        }

        LTCode &operator = (LTCode &&) = default;

        template
        <
            typename TForwardInputOutputIt3,
            typename TForwardInputIt4,
            typename TRandomAccessInputIt5
        >
        void Encode
        (
            TForwardInputOutputIt3 encoded,
            TForwardInputIt4 notNoisy,
            TRandomAccessInputIt5 decoded
        ) const
        {
            auto binsBegin = Bins.data();
            auto binsEnd = binsBegin + Bins.size();
            auto storage = Storage.data();
            LTEncode
            (
                binsBegin, binsEnd, storage,
                encoded, notNoisy, decoded
            );
        }

        /* Optimization tip: this does not shrink the containers! */
        template
        <
            typename TRandomAccessInputOutputIt1,
            typename TRandomAccessInputOutputIt4,
            typename TBidirectionalInputIt5,
            typename TBidirectionalInputOutputIt6
        >
        bool DecodeDestructive
        (
            TRandomAccessInputOutputIt1 solvedBegin,
            TRandomAccessInputOutputIt1 solvedEnd,
            TRandomAccessInputOutputIt4 decoded,
            TBidirectionalInputIt5 notNoisyBegin,
            TBidirectionalInputIt5 notNoisyEnd,
            TBidirectionalInputOutputIt6 encodedBegin,
            TBidirectionalInputOutputIt6 encodedEnd
        )
        {
            auto binsBegin = Bins.data();
            auto binsEnd = binsBegin + Bins.size();
            auto storage = Storage.data();
            return LTDecodeDestructive
                (
                    solvedBegin, solvedEnd,
                    binsBegin, binsEnd, storage,
                    decoded,
                    notNoisyBegin, notNoisyEnd,
                    encodedBegin, encodedEnd
                );
        }

        bool LoadFrom(FILE *fp)
        {
            if (fscanf(fp, "%u", &InputSymbolSize) != 1)
                return false;
            unsigned oss;
            if (fscanf(fp, "%u", &oss) != 1)
                return false;
            Bins.resize(oss);
            unsigned totalDeg = 0u;
            for (auto bin = Bins.data(),
                binEnd = Bins.data() + oss;
                bin != binEnd; ++bin)
            {
                if (fscanf(fp, "%u", &bin->Degree) != 1)
                    return false;
                bin->Index = totalDeg;
                totalDeg += bin->Degree;
            }
            Storage.resize(totalDeg);
            for (auto storage = Storage.data(),
                storageEnd = Storage.data() + totalDeg;
                storage != storageEnd; ++storage)
                if (fscanf(fp, "%u", storage) != 1)
                    return false;
            return true;
        }

        void SaveTo(FILE *fp) const
        {
            fprintf(fp, "%u %u\n", InputSymbolSize, (unsigned)Bins.size());
            auto const binEnd = Bins.data() + Bins.size();
            for (auto bin = Bins.data(); bin != binEnd; ++bin)
                fprintf(fp, "%u ", bin->Degree);
            fputc('\n', fp);
            for (auto bin = Bins.data(); bin != binEnd; ++bin, fputc('\n', fp))
                for (auto i = bin->GetBegin(Storage.data()),
                    j = bin->GetEnd(Storage.data());
                    i != j; ++i)
                    fprintf(fp, "%u ", *i);
        }
    };

    template
    <
        typename TAllocLubyBin,
        typename TAllocUnsigned,
        typename TRandomGenerator
    >
    void CreateLTCode
    (
        RobustSolitonDistribution const &dist,
        LTCode<TAllocLubyBin, TAllocUnsigned> &code,
        TRandomGenerator &next
    )
    {
        std::uniform_real_distribution<double> rDist(0.0, 1.0);
        std::uniform_int_distribution<unsigned> iDist(0u, dist.InputSymbolSize - 1u);
        std::set<unsigned, std::less<unsigned>, TAllocUnsigned> currentBin;
        code.InputSymbolSize = dist.InputSymbolSize;
        code.Bins.resize(dist.OutputSymbolSizeCached);
        code.Storage.clear();
        for (unsigned i = 0u;
            i != dist.OutputSymbolSizeCached;
            ++i)
        {
            unsigned deg = dist.SampleDegree(rDist(next));
            currentBin.clear();
            while (currentBin.size() != deg)
                currentBin.insert(iDist(next));
            code.Bins[i].Index = code.Storage.size();
            code.Bins[i].Degree = deg;
            code.Storage.insert(code.Storage.end(), currentBin.begin(), currentBin.end());
        }
    }

}
}

#endif // LUBY_HPP_
