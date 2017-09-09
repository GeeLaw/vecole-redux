#define _CRT_SECURE_NO_WARNINGS

#include"../library/cryptography.hpp"
#include"../library/arithmetic_circuits.hpp"
#include"../library/garbled_circuits.hpp"

#include<cstdio>

using namespace Cryptography;
using namespace Cryptography::ArithmeticCircuits;
using namespace Cryptography::ArithmeticCircuits::Garbled;

template <typename T>
struct GateMemory
{
    bool HasValue;
    T Value;
    GateMemory() : HasValue(false) { }
    GateMemory(GateMemory const &) = default;
    GateMemory(GateMemory &&) = default;
    GateMemory &operator = (GateMemory const &) = default;
    GateMemory &operator = (GateMemory &&) = default;
    GateMemory(T const &value) : HasValue(true), Value(value) { }
    GateMemory(T &&value) : HasValue(true), Value(std::move(value)) { }
};

template <typename TGateContainer, typename TMemoryContainer>
struct GatePerformer : GateVisitorCRTP<GatePerformer<TGateContainer, TMemoryContainer>, void(unsigned)>
{
    GatePerformer(TGateContainer &cont, TMemoryContainer &mem)
        : gates(cont), memory(mem)
    { }
    template <typename T>
    void PlaceValue(unsigned index, T const &value)
    {
        memory[index].HasValue = true;
        memory[index].Value = value;
    }
    void Evaluate(unsigned index)
    {
        if (!memory[index].HasValue)
            this->VisitDispatcher(&gates[index], index);
    }
private:
    TGateContainer &gates;
    TMemoryContainer &memory;
    friend class GateVisitorCRTP<GatePerformer<TGateContainer, TMemoryContainer>, void(unsigned)>;
    void VisitUnmatched(Gate *, unsigned)
    {
        fputs("Unmatched gate!\n", stderr);
        exit(-99);
    }
    void VisitConstZero(Gate *, unsigned index)
    {
        PlaceValue(index, 0);
    }
    void VisitConstOne(Gate *, unsigned index)
    {
        PlaceValue(index, 1);
    }
    void VisitConstMinusOne(Gate *, unsigned index)
    {
        PlaceValue(index, 1);
        PlaceValue(index, -memory[index].Value);
    }
    void VisitInputGate(Gate *, unsigned index)
    {
        if (!memory[index].HasValue)
            exit(-99);
    }
    void VisitAdditionGate(Gate *that, unsigned index)
    {
        auto &g = that->AsAdditionGate;
        Evaluate(g.Augend);
        Evaluate(g.Addend);
        PlaceValue(index, memory[g.Addend].Value + memory[g.Augend].Value);
    }
    void VisitNegationGate(Gate *that, unsigned index)
    {
        auto &g = that->AsNegationGate;
        Evaluate(g.Target);
        PlaceValue(index, -memory[g.Target].Value);
    }
    void VisitSubtractionGate(Gate *that, unsigned index)
    {
        auto &g = that->AsSubtractionGate;
        Evaluate(g.Minuend);
        Evaluate(g.Subtrahend);
        PlaceValue(index, memory[g.Minuend].Value - memory[g.Subtrahend].Value);
    }
    void VisitMultiplicationGate(Gate *that, unsigned index)
    {
        auto &g = that->AsMultiplicationGate;
        Evaluate(g.Multiplier);
        Evaluate(g.Multiplicand);
        PlaceValue(index, memory[g.Multiplier].Value * memory[g.Multiplicand].Value);
    }
};

template <typename TGateContainer, typename TMemoryContainer>
GatePerformer<TGateContainer, TMemoryContainer> MakePerformer(TGateContainer &cont, TMemoryContainer &mem)
{
    return{ cont, mem };
}

int main()
{
    /* Creates and compiles a one-shot OLE circuit. */
    TwoPartyCircuit<> circuit;
    auto const aliceX = circuit.InsertGate(InputGateData{ AgentFlag::Alice, 0, 0 });
    auto const bobA = circuit.InsertGate(InputGateData{ AgentFlag::Bob, 0, 0 });
    auto const bobB = circuit.InsertGate(InputGateData{ AgentFlag::Bob, 1, 0 });
    circuit.AliceInputBegin = aliceX;
    circuit.AliceInputEnd = aliceX + 1;
    circuit.BobInputBegin = bobA;
    circuit.BobInputEnd = bobB + 1;
    auto const AX = circuit.InsertGate(MultiplicationGateData{ bobA, aliceX });
    auto const AXB = circuit.InsertGate(AdditionGateData{ AX, bobB });
    circuit.AliceOutput.push_back(AXB);
    Garbled::EncodingCircuit<> encoderStorage;
    Garbled::DecodingCircuit<> decoderStorage;
    CompileToDare(circuit, encoderStorage, decoderStorage);
    /* Running the compiled circuits. */
    std::vector<int> offlineEncodingResult;
    std::vector<std::vector<int>> aliceEncodingResult;
    std::vector<std::vector<int>> bobEncodingResult;
    /* Creates an encoding from input. */
    do
    {
        std::vector<GateMemory<int>> memory;
        memory.resize(encoderStorage.Gates.size());
        auto encoderPerformer = MakePerformer(encoderStorage.Gates, memory);
        for (auto const r : encoderStorage.Randomness)
            encoderPerformer.PlaceValue(r, rand() % 300 - 150);
        for (auto const oe : encoderStorage.OfflineEncoding)
        {
            encoderPerformer.Evaluate(oe);
            offlineEncodingResult.push_back(memory[oe].Value);
        }
        printf("%zu Alice's input(s):\n", encoderStorage.AliceEncoding.size());
        aliceEncodingResult.resize(encoderStorage.AliceEncoding.size());
        for (int i = 0, j = encoderStorage.AliceEncoding.size();i != j;++i)
        {
            int val;
            scanf("%d", &val);
            auto &target = aliceEncodingResult[i];
            for (auto const &kp : encoderStorage.AliceEncoding[i])
            {
                encoderPerformer.Evaluate(kp.Coefficient);
                encoderPerformer.Evaluate(kp.Intercept);
                target.push_back(val * memory[kp.Coefficient].Value + memory[kp.Intercept].Value);
            }
        }
        printf("%zu Bob's input(s):\n", encoderStorage.BobEncoding.size());
        bobEncodingResult.resize(encoderStorage.BobEncoding.size());
        for (int i = 0, j = encoderStorage.BobEncoding.size();i != j;++i)
        {
            int val;
            scanf("%d", &val);
            auto &target = bobEncodingResult[i];
            for (auto const &kp : encoderStorage.BobEncoding[i])
            {
                encoderPerformer.Evaluate(kp.Coefficient);
                encoderPerformer.Evaluate(kp.Intercept);
                target.push_back(val * memory[kp.Coefficient].Value + memory[kp.Intercept].Value);
            }
        }
    } while (false);
    /* Decodes the encoding. */
    do
    {
        std::vector<GateMemory<int>> memory;
        memory.resize(decoderStorage.Gates.size());
        auto decoderPerformer = MakePerformer(decoderStorage.Gates, memory);
        for (int i = 0, I = decoderStorage.OfflineEncoding.size(); i != I; ++i)
            decoderPerformer.PlaceValue(decoderStorage.OfflineEncoding[i], offlineEncodingResult[i]);
        for (int i = 0, I = decoderStorage.AliceEncoding.size(); i != I; ++i)
        {
            auto const &alice = decoderStorage.AliceEncoding[i];
            for (int j = 0, J = alice.size(); j != J; ++j)
                decoderPerformer.PlaceValue(alice[j], aliceEncodingResult[i][j]);
        }
        for (int i = 0, I = decoderStorage.BobEncoding.size(); i != I; ++i)
        {
            auto const &bob = decoderStorage.BobEncoding[i];
            for (int j = 0, J = bob.size(); j != J; ++j)
                decoderPerformer.PlaceValue(bob[j], bobEncodingResult[i][j]);
        }
        puts("Alice's output(s):");
        for (auto const ao : decoderStorage.AliceOutput)
        {
            decoderPerformer.Evaluate(ao);
            printf("%d\n", (int)memory[ao].Value);
        }
    } while (false);
    return 0;
}
