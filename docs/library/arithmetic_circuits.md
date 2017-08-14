# `arithmetic_circuits.hpp`

This file defines utilities in `Cryptography::ArithmeticCircuits` namespace, which is used to represent and manipulate arithmetic circuits.

## `GateHandle` type

The alias of `unsigned`. Throughout the code, the `Gate`s are stored in an array (or more precisely, a `std::vector`), and they are reference by their index. It should be well noted that the handle is *always* the index of the gate in the `Gates` array. Such attention is important for input gates.

N.B. A gate always belongs to some circuit, and its handle is circuit-specific.

## `GateKind` enumeration

See `Gate` structure.

## `Gate` structure

The structure holds the data of a gate.

- `Id` stores the handle of this `Gate`, which should coincide with the index.
- `Kind` is a `GateKind::Type` enumeration, which indicates what kind of gate this `Gate` is.
- `AsXxxKindOfGate` are the fields that occupy the same underlying memory location, among which only the one specified by `Kind` should be used. They are of `XxxKindOfGateData` type, which give further detail about the gate. For example, an input gate has its owner (`Agent`), major and minor indices (`MajorIndex` and `MinorIndex`) stored in `AsInputGate`.

Notes on specific kinds of gates:

- `ConstZero`, `ConstOne` and `ConstMinusOne` are gates that have no input and produce the constant indicated by its name. None of them has further detail.
- `InputGate`, as described earlier.
- `AdditionGate` has `Augend` and `Addend`, handles to the two summands.
- `NegationGate` has `Target`, handle to the gate negated.
- `SubtractionGate` has `Minuend` and `Subtrahend`, handles to the minuend and the subtrahend.
- `MultiplicationGate` has `Multiplier` and `Multiplicand`, handles to the two factors.

## `GateVisitorCRTP<TVisitor, TRet(TArgs...)>` class template

A helper class template that many would find useful. The template adopts CRTP (curiously recurring template pattern) and is used to implement visitor pattern.

For example, if a visitor `V` visits `Gate`s with extra parameters of type `T1` and type `T2`, and the visiting produces a return value of type `T3`, one might find the following pattern in the code:

```C++
struct V : GateVisitorCRTP<V, T3(T1, T2)>
{
    T3 Visit(Gate *that, T1 arg1, T2 arg2)
    {
        // VisitDispatcher is a private function
        // provided by GateVisitorCRTP that calls
        // the correct function depending on “that”.
        return VisitDispatcher(that, arg1, arg2);
    }
private:
    friend class GateVisitorCRTP<V, T3(T1, T2)>;
    T3 VisitUnmatched(Gate *, T1, T2)
    {
        // Usually there is some error-handling logic.
    }
    T3 VisitInputGate(Gate *that, T1 arg1, T2 arg2)
    {
        auto const &inputGateData = that->AsInputGate;
        // Logic for visiting an input gate.
    }
    // Other VisitXxxKindOfGate logic omitted.
};
```

## `CircuitCRTP<TC>` structure template

A helper structure that creates `InsertGate` member function for derived structures.

`InsertGate` takes one of the gate data structures as argument and returns the handle to the newly inserted `Gate`. It is assumed that `Gates` is a member of the derived structure with `size` and `push_back` member functions available to `CircuitCRTP<TC>`.

## `TwoPartyCircuit<TA, TAH>` structure template

This structure holds necessary information for a two-party circuit, where Alice and Bob provide inputs and Alice gets some outputs.

The template arguments are allocators that allows allocating memory for `std::vector`. They are often chosen as the default values. Use default values with empty argument list, i.e., `TwoPartyCircuit<>`.

Members:

- `Gates` is a vector of `Gate`s, in which the indices of `Gate`s are the handles.
- `AliceInputBegin` and `AliceInputEnd` specifies a left-close-right-open interval `[begin, end)` where the input gates of Alice’s reside. In other words, handle `AliceInputBegin`, `AliceInputBegin + 1`, …, `AliceInputEnd - 1` are the handles to all of Alice’s inputs.
- `BobInputBegin` and `BobInputEnd` are similar to those of Alice’s.
- `AliceOutput` is a vector of `GateHandle`s. It stores, in the desired order, handles to the output gates.
