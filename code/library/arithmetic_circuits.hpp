#ifndef ARITHMETIC_CIRCUITS_HPP_
#define ARITHMETIC_CIRCUITS_HPP_

#include<vector>
#include"cryptography.hpp"

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

    typedef unsigned GateHandle;

    constexpr GateHandle InvalidGateHandle = 0u - 1u;

    namespace GateKind
    {
        typedef unsigned Type;
        constexpr Type Invalid = 0u;
        constexpr Type ConstZero = 1u;
        constexpr Type ConstOne = 2u;
        constexpr Type ConstMinusOne = 3u;
        constexpr Type InputGate = 4u;
        constexpr Type AdditionGate = 5u;
        constexpr Type NegationGate = 6u;
        constexpr Type SubtractionGate = 7u;
        constexpr Type MultiplicationGate = 8u;
    };

    struct ConstZeroData { };
    struct ConstOneData { };
    struct ConstMinusOneData { };
    struct InputGateData
    {
        AgentFlag::Type Agent;
        unsigned MajorIndex, MinorIndex;
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
    };

}
}

#undef CREATE_RESETTER_
#undef CALL_VISIT_
#undef CREATE_INSERTGATE_

#endif // ARITHMETIC_CIRCUITS_HPP_
