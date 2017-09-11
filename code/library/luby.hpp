#ifndef LUBY_HPP_
#define LUBY_HPP_

#include<cmath>
#include<vector>
#include<set>
#include<utility>
#include<random>
#include<cstring>
#include"helpers.hpp"

namespace Encoding
{
namespace LubyTransform
{
    struct RobustSolitonDistribution
    {
        /* k in LT Codes (FOCS 02)
         * w in vector OLE
         */
        size_t InputSymbolSize;
        /* c in LT Codes */
        double C;
        /* delta in LT Codes */
        double Delta;
        /* R in LT Codes */
        double RCached;
        /* k/R in LT Codes */
        size_t KOverRCached;
        /* beta in LT Codes */
        double BetaCached;
        /* v in LT Codes
         * rounded up to a multiple of 4
         */
        size_t OutputSymbolSizeCached;
        /* Call this method after non-cache
         * values have been edited.
         */
        void InvalidateCache()
        {
            RCached = C * std::log(InputSymbolSize / Delta) * std::sqrt((double)InputSymbolSize);
            KOverRCached = (size_t)(InputSymbolSize / RCached + 0.5);
            auto beta = std::log(RCached / Delta);
            for (size_t i = KOverRCached - 1; i; --i)
                beta += 1.0 / i;
            BetaCached = 1.0 + beta * RCached / InputSymbolSize;
            OutputSymbolSizeCached = (size_t)(InputSymbolSize * BetaCached + 0.5);
            /* Round up to a multiple of 4 */
            OutputSymbolSizeCached += ((4 - (OutputSymbolSizeCached & 3)) & 3);
        }
        /* Ideal ingredient, i is 1-based. */
        double Rho(size_t i) const
        {
            if (i == 1)
                return 1.0 / InputSymbolSize;
            if (i > InputSymbolSize)
                return 0.0;
            return 1.0 / i / (i - 1);
        }
        /* Added part for robust distribution. */
        double Tau(size_t i) const
        {
            if (i < KOverRCached)
                return RCached / i / InputSymbolSize;
            if (i > KOverRCached)
                return 0.0;
            return RCached * std::log(RCached / Delta) / InputSymbolSize;
        }
        /* The distribution itself. */
        double Mu(size_t i) const
        {
            return (Rho(i) + Tau(i)) / BetaCached;
        }
        /* r in [0, 1]
         * increasing function of r
         */
        size_t SampleDegree(double r) const
        {
            if (r >= 1.0)
                return InputSymbolSize;
            if (r <= 0.0)
                return 1;
            size_t d = 0;
            for (auto s = 0.0; s < r && d != InputSymbolSize; s += Mu(++d))
                ;
            return d;
        }
    };

    struct LubyBin
    {
        size_t Index;
        size_t Degree;
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
        /* size_t[] */
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
        /* size_t[], destructive */
        TRandomAccessInputOutputIt3 const &storage,
        /* F[decodedSize] */
        TRandomAccessInputOutputIt4 const &decoded,
        /* [notNoisyBegin, notNoisyEnd) = bool[encodedSize] */
        TBidirectionalInputIt5 notNoisyBegin,
        TBidirectionalInputIt5 notNoisyEnd,
        /* [encodedBegin, encodedEnd) = F[encodedSize], destructive */
        TBidirectionalInputOutputIt6 const &encodedBegin,
        TBidirectionalInputOutputIt6 encodedEnd
    )
    {
        size_t remaining = solvedEnd - solvedBegin;
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
                    if (deg == 1)
                    {
                        size_t t = storage[bins->Index];
                        if (!solvedBegin[t])
                        {
                            solvedBegin[t] = true;
                            --remaining;
                            decoded[t] = std::move(*encoded);
                        }
                    }
                    else if (deg == 2)
                    {
                        size_t const t1 = storage[bins->Index];
                        size_t const t2 = storage[bins->Index + 1];
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
                    else if (deg > 2)
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
            newRelease && remaining; )
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
                size_t newDeg = storageEnd - storageBegin;
                /* If remaining degree is 1:
                 *     - release this position
                 *     - set newRelease
                 *     - set newDeg = 0 for removal
                 */
                if (newDeg == 1)
                {
                    solvedBegin[*storageBegin] = true;
                    --remaining;
                    decoded[*storageBegin] = std::move(*encoded);
                    newRelease = true;
                    newDeg = 0;
                }
                /* Either move the last bin here (current bin is discarded)
                 * or advance the iterators (current bin is kept) */
                if (newDeg)
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
        return !remaining;
    }

    template
    <
        typename TAllocLubyBin = std::allocator<LubyBin>,
        typename TAllocSizeT = std::allocator<size_t>
    >
    struct LTCode
    {
        typedef std::vector<LubyBin, TAllocLubyBin> LubyBinVec;
        typedef std::vector<size_t, TAllocSizeT> SizeTVec;

        size_t InputSymbolSize;
        LubyBinVec Bins;
        SizeTVec Storage;

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
            std::memcpy(Storage.data(), other.Storage.data(), szStorage * sizeof(size_t));
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
            if (fscanf(fp, "%zu", &InputSymbolSize) != 1)
                return false;
            size_t oss;
            if (fscanf(fp, "%zu", &oss) != 1)
                return false;
            Bins.clear();
            Bins.resize(oss);
            size_t totalDeg = 0;
            for (auto bin = Bins.data(),
                binEnd = Bins.data() + oss;
                bin != binEnd; ++bin)
            {
                if (fscanf(fp, "%zu", &bin->Degree) != 1)
                    return false;
                bin->Index = totalDeg;
                totalDeg += bin->Degree;
            }
            Storage.clear();
            Storage.resize(totalDeg);
            return Helpers::LoadSizeTRange(
                Storage.data(),
                Storage.data() + Storage.size(),
                fp);
        }

        void SaveTo(FILE *fp) const
        {
            fprintf(fp, "%zu %zu\n", InputSymbolSize, Bins.size());
            auto const binEnd = Bins.data() + Bins.size();
            for (auto bin = Bins.data(); bin != binEnd; ++bin)
                fprintf(fp, "%zu ", bin->Degree);
            fputc('\n', fp);
            for (auto bin = Bins.data(); bin != binEnd; ++bin, fputc('\n', fp))
                for (auto i = bin->GetBegin(Storage.data()),
                    j = bin->GetEnd(Storage.data());
                    i != j; ++i)
                    fprintf(fp, "%zu ", *i);
            fputc('\n', fp);
        }
    };

    template
    <
        typename TAllocLubyBin,
        typename TAllocSizeT,
        typename TRandomGenerator
    >
    void CreateLTCode
    (
        RobustSolitonDistribution const &dist,
        LTCode<TAllocLubyBin, TAllocSizeT> &code,
        TRandomGenerator &next
    )
    {
        std::uniform_real_distribution<double> rDist(0.0, 1.0);
        std::uniform_int_distribution<size_t> iDist(0, dist.InputSymbolSize - 1);
        std::set<size_t, std::less<size_t>, TAllocSizeT> currentBin;
        code.InputSymbolSize = dist.InputSymbolSize;
        code.Bins.resize(dist.OutputSymbolSizeCached);
        code.Storage.clear();
        for (size_t i = 0;
            i != dist.OutputSymbolSizeCached;
            ++i)
        {
            size_t deg = dist.SampleDegree(rDist(next));
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
