#define _CRT_SECURE_NO_WARNINGS

#include"../library/cryptography.hpp"
#include"../library/arithmetic_circuits.hpp"
#include"../library/garbled_circuits2.hpp"

#include<cstdio>
#include<vector>
#include<random>
#include<ctime>

using namespace Cryptography;
using namespace Cryptography::ArithmeticCircuits;

void BuildCircuit(TwoPartyCircuit<> &circuit);

struct StdoutIterator
{
    StdoutIterator &operator ++ ()
    {
        return *this;
    }
    StdoutIterator operator ++ (int)
    {
        return *this;
    }
    StdoutIterator &operator * ()
    {
        return *this;
    }
    StdoutIterator &operator = (int v)
    {
        printf("%d\n", v);
        return *this;
    }
};

int main()
{
    TwoPartyCircuit<> circuit;
    Garbled2::Configuration<> config, surrogateConfig;
    Garbled2::KeyPairs<int> kp;
    Garbled2::Keys<int> keys;
    std::mt19937 rng((unsigned)time(nullptr));
    std::uniform_int_distribution<int> ringDist(-100, 100);
    /* Build the circuit and configure
     * garbled circuit componenets.
     */
    BuildCircuit(circuit);
    Garbled2::Configure(circuit, config);
    kp.ApplyConfiguration(config);
    keys.ApplyConfiguration(config);
    surrogateConfig = config;
    /* Garble the circuit. */
    kp.ResetPreserveConfiguration();
    Garbled2::Garble(circuit, kp, rng, ringDist);
    /* Collect the input. */
    keys.ResetPreserveConfiguration();
    for (size_t i = 0,
        alice = circuit.AliceInputEnd - circuit.AliceInputBegin;
        i != alice; ++i)
    {
        fprintf(stderr, "%zu-th input of Alice:\n", i + 1);
        int v;
        scanf("%d", &v);
        for (size_t j = 0, enc = config.AliceEncoding[i];
            j != enc; ++j)
            keys.AliceEncoding[i].push_back(
                kp.AliceCoefficient[i][j] * v + kp.AliceIntercept[i][j]
            );
    }
    for (size_t i = 0,
        bob = circuit.BobInputEnd - circuit.BobInputBegin;
        i != bob; ++i)
    {
        fprintf(stderr, "%zu-th input of Bob:\n", i + 1);
        int v;
        scanf("%d", &v);
        for (size_t j = 0, enc = config.BobEncoding[i];
            j != enc; ++j)
            keys.BobEncoding[i].push_back(
                kp.BobCoefficient[i][j] * v + kp.BobIntercept[i][j]
            );
    }
    keys.OfflineEncoding = std::move(kp.OfflineEncoding);
    /* Ungarble the circuit. */
    surrogateConfig.ResetPreserveConfiguration();
    Garbled2::Ungarble(circuit, surrogateConfig, keys, StdoutIterator());
    return 0;
}

void BuildCircuit(TwoPartyCircuit<> &circuit)
{
    circuit = TwoPartyCircuit<>();
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
}
