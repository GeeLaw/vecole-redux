#ifndef ARITHMETIC_CIRCUITS_HPP_
#define ARITHMETIC_CIRCUITS_HPP_

#include<vector>
#include"cryptography.hpp"
#include"helpers.hpp"

#define CREATE_RESETTER_(T) \
    void Reset(T##Data value) { Kind = GateKind::T; As##T = value; } \
    Gate(GateHandle id, T##Data value) \
        : Id(id), Kind(GateKind::T), As##T(value) \
    { }

#define CALL_VISIT_(T) case GateKind::T: \
    return This->Visit##T(that, static_cast<TArgs>(args)...)

#define CREATE_INSERTGATE_(T) \
    GateHandle InsertGate(T##Data value) \
    { \
        auto This = static_cast<TCircuit *>(this); \
        GateHandle handle = This->Gates.size(); \
        This->Gates.push_back(Gate{ handle, value }); \
        return handle; \
    }

namespace Cryptography {
namespace ArithmeticCircuits
{
    struct Gate;

    typedef size_t GateHandle;

    constexpr GateHandle InvalidGateHandle = (GateHandle)0 - (GateHandle)1;

    namespace GateKind
    {
        typedef size_t Type;
        constexpr Type Invalid = 0;
        constexpr Type ConstZero = 1;
        constexpr Type ConstOne = 2;
        constexpr Type ConstMinusOne = 3;
        constexpr Type InputGate = 4;
        constexpr Type AdditionGate = 5;
        constexpr Type NegationGate = 6;
        constexpr Type SubtractionGate = 7;
        constexpr Type MultiplicationGate = 8;
    };

    struct ConstZeroData { };
    struct ConstOneData { };
    struct ConstMinusOneData { };
    struct InputGateData
    {
        AgentFlag::Type Agent;
        size_t MajorIndex, MinorIndex;
    };
    struct AdditionGateData
    {
        GateHandle Augend, Addend;
    };
    struct NegationGateData
    {
        GateHandle Target;
    };
    struct SubtractionGateData
    {
        GateHandle Minuend, Subtrahend;
    };
    struct MultiplicationGateData
    {
        GateHandle Multiplier, Multiplicand;
    };

    struct Gate
    {
        GateHandle Id;
        GateKind::Type Kind;
        union
        {
            ConstZeroData AsConstZero;
            ConstOneData AsConstOne;
            ConstMinusOneData AsConstMinusOne;
            InputGateData AsInputGate;
            AdditionGateData AsAdditionGate;
            NegationGateData AsNegationGate;
            SubtractionGateData AsSubtractionGate;
            MultiplicationGateData AsMultiplicationGate;
        };
        constexpr Gate() = default;
        constexpr Gate(Gate const &) = default;
        constexpr Gate(Gate &&) = default;
        Gate &operator = (Gate const &) = default;
        Gate &operator = (Gate &&) = default;
        ~Gate() = default;
        CREATE_RESETTER_(ConstZero)
        CREATE_RESETTER_(ConstOne)
        CREATE_RESETTER_(ConstMinusOne)
        CREATE_RESETTER_(InputGate)
        CREATE_RESETTER_(AdditionGate)
        CREATE_RESETTER_(NegationGate)
        CREATE_RESETTER_(SubtractionGate)
        CREATE_RESETTER_(MultiplicationGate)
    };

    namespace _CRTPHelper
    {
        template <typename T>
        struct MakeNiceReplacement
        {
            typedef T const &Type;
        };
        template <typename T>
        struct MakeNiceReplacement<T &>
        {
            typedef T &Type;
        };
        template <typename T>
        struct MakeNiceReplacement<T &&>
        {
            typedef T &&Type;
        };
    }

    template <typename TVisitor, typename TSignature>
    class GateVisitorCRTP
    {
        GateVisitorCRTP() = delete;
        GateVisitorCRTP(GateVisitorCRTP &&) = delete;
        GateVisitorCRTP(GateVisitorCRTP const &) = delete;
        GateVisitorCRTP &operator = (GateVisitorCRTP &&) = delete;
        GateVisitorCRTP &operator = (GateVisitorCRTP const &) = delete;
        ~GateVisitorCRTP() = delete;
    };

    template <typename TVisitor, typename TRet, typename ...TArgs>
    class GateVisitorCRTP<TVisitor, TRet(TArgs...)>
    {
        friend TVisitor;
        GateVisitorCRTP() = default;
        GateVisitorCRTP(GateVisitorCRTP &&) = default;
        GateVisitorCRTP(GateVisitorCRTP const &) = default;
        GateVisitorCRTP &operator = (GateVisitorCRTP &&) = default;
        GateVisitorCRTP &operator = (GateVisitorCRTP const &) = default;
        ~GateVisitorCRTP() = default;
        TRet VisitDispatcher(Gate *that,
            typename _CRTPHelper::MakeNiceReplacement<TArgs>::Type ...args)
        {
            TVisitor *This = static_cast<TVisitor *>(this);
            if (!that)
                goto fail;
            switch (that->Kind)
            {
                CALL_VISIT_(ConstZero);
                CALL_VISIT_(ConstOne);
                CALL_VISIT_(ConstMinusOne);
                CALL_VISIT_(InputGate);
                CALL_VISIT_(AdditionGate);
                CALL_VISIT_(NegationGate);
                CALL_VISIT_(SubtractionGate);
                CALL_VISIT_(MultiplicationGate);
            }
        fail:
            return This->VisitUnmatched(that, static_cast<TArgs>(args)...);
        }
    };

    struct GateSaver : GateVisitorCRTP<GateSaver, void (FILE *)>
    {
        void operator () (Gate gate, FILE *fp) const
        {
            SaveToImpl(&gate, fp);
        }
        void SaveTo(Gate gate, FILE *fp) const
        {
            SaveToImpl(&gate, fp);
        }
        template <typename TIt>
        void SaveRange(TIt begin, TIt end, FILE *fp) const
        {
            for (; begin != end; ++begin)
                SaveTo(*begin, fp);
        }
        template <typename TIt>
        void operator () (TIt begin, TIt end, FILE *fp) const
        {
            SaveRange(std::move(begin), std::move(end), fp);
        }
    private:
        void SaveToImpl(Gate *gate, FILE *fp) const
        {
            fprintf(fp, "%zu %zu", gate->Id, gate->Kind);
            GateSaver().VisitDispatcher(gate, fp);
            fputc('\n', fp);
        }
        friend class GateVisitorCRTP<GateSaver, void (FILE *)>;
        void VisitConstZero(Gate *, FILE *) { }
        void VisitConstOne(Gate *, FILE *) { }
        void VisitConstMinusOne(Gate *, FILE *) { }
        void VisitInputGate(Gate *that, FILE *fp)
        {
            fprintf(fp, " %zu %zu %zu",
                that->AsInputGate.Agent,
                that->AsInputGate.MajorIndex,
                that->AsInputGate.MinorIndex);
        }
        void VisitAdditionGate(Gate *that, FILE *fp)
        {
            fprintf(fp, " %zu %zu",
                that->AsAdditionGate.Augend,
                that->AsAdditionGate.Addend);
        }
        void VisitNegationGate(Gate *that, FILE *fp)
        {
            fprintf(fp, " %zu",
                that->AsNegationGate.Target);
        }
        void VisitSubtractionGate(Gate *that, FILE *fp)
        {
            fprintf(fp, " %zu %zu",
                that->AsSubtractionGate.Minuend,
                that->AsSubtractionGate.Subtrahend);
        }
        void VisitMultiplicationGate(Gate *that, FILE *fp)
        {
            fprintf(fp, " %zu %zu",
                that->AsMultiplicationGate.Multiplier,
                that->AsMultiplicationGate.Multiplicand);
        }
        void VisitUnmatched(Gate *, FILE *) { }
    };

    struct GateLoader : GateVisitorCRTP<GateLoader, bool (FILE *)>
    {
        Gate operator () (FILE *fp) const
        {
            return LoadFromImpl(fp);
        }
        Gate LoadFrom(FILE *fp) const
        {
            return LoadFromImpl(fp);
        }
        template <typename TIt>
        bool LoadRange(TIt begin, TIt end, FILE *fp) const
        {
            for (; begin != end; ++begin)
            {
                auto g = LoadFromImpl(fp);
                if (g.Id == InvalidGateHandle
                    || g.Kind == Invalid)
                    return false;
                *begin = g;
            }
            return true;
        }
        template <typename TIt>
        bool operator () (TIt begin, TIt end, FILE *fp) const
        {
            return LoadRange(std::move(begin), std::move(end), fp);
        }
    private:
        Gate LoadFromImpl(FILE *fp) const
        {
            Gate gate;
            if (fscanf(fp, "%zu%zu", &gate.Id, &gate.Kind) != 2
                || !GateLoader().VisitDispatcher(&gate, fp))
            {
                gate.Id = InvalidGateHandle;
                gate.Kind = GateKind::Invalid;
            }
            return gate;
        }
        friend class GateVisitorCRTP<GateLoader, bool (FILE *)>;
        bool VisitConstZero(Gate *, FILE *) { return true; }
        bool VisitConstOne(Gate *, FILE *) { return true; }
        bool VisitConstMinusOne(Gate *, FILE *) { return true; }
        bool VisitInputGate(Gate *that, FILE *fp)
        {
            return fscanf(fp, "%zu%zu%zu",
                &that->AsInputGate.Agent,
                &that->AsInputGate.MajorIndex,
                &that->AsInputGate.MinorIndex) == 3;
        }
        bool VisitAdditionGate(Gate *that, FILE *fp)
        {
            return fscanf(fp, "%zu%zu",
                &that->AsAdditionGate.Augend,
                &that->AsAdditionGate.Addend) == 2;
        }
        bool VisitNegationGate(Gate *that, FILE *fp)
        {
            return fscanf(fp, "%zu",
                &that->AsNegationGate.Target) == 1;
        }
        bool VisitSubtractionGate(Gate *that, FILE *fp)
        {
            return fscanf(fp, "%zu%zu",
                &that->AsSubtractionGate.Minuend,
                &that->AsSubtractionGate.Subtrahend) == 2;
        }
        bool VisitMultiplicationGate(Gate *that, FILE *fp)
        {
            return fscanf(fp, "%zu%zu",
                &that->AsMultiplicationGate.Multiplier,
                &that->AsMultiplicationGate.Multiplicand) == 2;
        }
        bool VisitUnmatched(Gate *, FILE *) { return false; }
    };

    template <typename TCircuit>
    struct CircuitCRTP
    {
        CREATE_INSERTGATE_(ConstZero)
        CREATE_INSERTGATE_(ConstOne)
        CREATE_INSERTGATE_(ConstMinusOne)
        CREATE_INSERTGATE_(InputGate)
        CREATE_INSERTGATE_(AdditionGate)
        CREATE_INSERTGATE_(NegationGate)
        CREATE_INSERTGATE_(SubtractionGate)
        CREATE_INSERTGATE_(MultiplicationGate)
    };

    template
    <
        typename TCAllocGate = std::allocator<Gate>,
        typename TCAllocHandle = std::allocator<GateHandle>
    >
    struct TwoPartyCircuit
        : CircuitCRTP<TwoPartyCircuit<TCAllocGate, TCAllocHandle>>
    {
        typedef std::vector<Gate, TCAllocGate> GateVec;
        typedef std::vector<GateHandle, TCAllocHandle> HandleVec;

        GateVec Gates;
        GateHandle AliceInputBegin, AliceInputEnd;
        GateHandle BobInputBegin, BobInputEnd;
        HandleVec AliceOutput;

        void SaveTo(FILE *fp) const
        {
            GateSaver gs;
            fprintf(fp, "%zu %zu %zu %zu %zu %zu\n",
                Gates.size(),
                AliceInputBegin, AliceInputEnd,
                BobInputBegin, BobInputEnd,
                AliceOutput.size());
            gs(Gates.data(), Gates.data() + Gates.size(), fp);
            Helpers::SaveSizeTRange(
                AliceOutput.data(),
                AliceOutput.data() + AliceOutput.size(),
                fp);
        }

        bool LoadFrom(FILE *fp)
        {
            GateLoader gl;
            size_t gatesSz, aoSize;
            if (fscanf(fp, "%zu%zu%zu%zu%zu%zu", &gatesSz,
                &AliceInputBegin, &AliceInputEnd,
                &BobInputBegin, &BobInputEnd,
                &aoSize) != 6)
                return false;
            Gates.clear();
            Gates.resize(gatesSz);
            AliceOutput.clear();
            AliceOutput.resize(aoSize);
            return gl(Gates.data(), Gates.data() + Gates.size(), fp)
                && Helpers::LoadSizeTRange(
                    AliceOutput.data(),
                    AliceOutput.data() + AliceOutput.size(),
                    fp);
        }
    };

}
}

#undef CREATE_RESETTER_
#undef CALL_VISIT_
#undef CREATE_INSERTGATE_

#endif // ARITHMETIC_CIRCUITS_HPP_
