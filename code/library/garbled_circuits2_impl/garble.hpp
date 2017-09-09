template
<
    TPC_TYPENAMES_, KEYPAIRS_TYPENAMES_,
    typename TRandomGenerator, typename TRingDist
>
struct Garble
    : GateVisitorCRTP
    <
        Garble
        <
            TPC_TYPENAME_ARGS_, KEYPAIRS_TYPENAME_ARGS_,
            TRandomGenerator, TRingDist
        >,
        void(TKPRing &&, TKPRing &&)
    >
{
    typedef TwoPartyCircuit<TPC_TYPENAME_ARGS_> TwoPartyCircuitType;
    typedef KeyPairs<KEYPAIRS_TYPENAME_ARGS_> KeyPairsType;

    Gate *circuit;
    KeyPairsType &keypairs;
    TRandomGenerator &next;
    TRingDist &ringDist;
    TKPRing const &oneGate;
    TKPRing const &zeroGate;

    Garble(
        TwoPartyCircuitType &tpc,
        KeyPairsType &kp,
        TRandomGenerator &rng,
        TRingDist &rd,
        TKPRing const &one = 1,
        TKPRing const &zero = 0
    ) : circuit(tpc.Gates.data()), keypairs(kp),
        next(rng), ringDist(rd),
        oneGate(one), zeroGate(zero)
    {
        for (auto const ao : tpc.AliceOutput)
        {
            TKPRing oneCopy = oneGate;
            TKPRing zeroCopy = zeroGate;
            VisitDispatcher(circuit + ao, std::move(oneCopy), std::move(zeroCopy));
        }
    }

private:
    friend class GateVisitorCRTP
        <
            Garble
            <
                TPC_TYPENAME_ARGS_, KEYPAIRS_TYPENAME_ARGS_,
                TRandomGenerator, TRingDist
            >,
            void(TKPRing &&, TKPRing &&)
        >;

    void VisitUnmatched(Gate *, TKPRing &&, TKPRing &&)
    {
        std::exit(-99);
    }

    void VisitConstZero(Gate *, TKPRing &&k, TKPRing &&b)
    {
        keypairs.OfflineEncoding.push_back(std::move(b));
    }

    void VisitConstOne(Gate *, TKPRing &&k, TKPRing &&b)
    {
        keypairs.OfflineEncoding.push_back(k + b);
    }

    void VisitConstMinusOne(Gate *, TKPRing &&k, TKPRing &&b)
    {
        keypairs.OfflineEncoding.push_back(b - k);
    }

    void VisitInputGate(Gate *that, TKPRing &&k, TKPRing &&b)
    {
        auto const &g = that->AsInputGate;
        auto const index = g.MajorIndex;
        auto const owner = g.Agent;
        if (owner == AgentFlag::Alice)
        {
            keypairs.AliceCoefficient[index].push_back(std::move(k));
            keypairs.AliceIntercept[index].push_back(std::move(b));
            return;
        }
        if (owner == AgentFlag::Bob)
        {
            keypairs.BobCoefficient[index].push_back(std::move(k));
            keypairs.BobIntercept[index].push_back(std::move(b));
            return;
        }
        exit(-99);
    }

    void VisitAdditionGate(Gate *that, TKPRing &&k, TKPRing &&b)
    {
        auto const &g = that->AsAdditionGate;
        auto &&r = ringDist(next);
        b -= r;
        auto kCopy = k;
        VisitDispatcher(circuit + g.Augend, std::move(k), std::move(r));
        VisitDispatcher(circuit + g.Addend, std::move(kCopy), std::move(b));
    }

    void VisitNegationGate(Gate *that, TKPRing &&k, TKPRing &&b)
    {
        auto const &g = that->AsNegationGate;
        VisitDispatcher(circuit + g.Target, -k, std::move(b));
    }

    void VisitSubtractionGate(Gate *that, TKPRing &&k, TKPRing &&b)
    {
        auto const &g = that->AsSubtractionGate;
        auto &&r = ringDist(next);
        auto kCopy = k;
        VisitDispatcher(circuit + g.Minuend, std::move(k), b + r);
        VisitDispatcher(circuit + g.Subtrahend, std::move(kCopy), std::move(r));
    }

    void VisitMultiplicationGate(Gate *that, TKPRing &&k, TKPRing &&b)
    {
        auto const &g = that->AsMultiplicationGate;
        auto &&r1 = ringDist(next);
        auto &&r2 = ringDist(next);
        auto &&r3 = ringDist(next);
        auto &&kr2 = k * r2;
        b -= r1*r2 + r3;
        auto oneCopy = oneGate;
        auto const g1 = circuit + g.Multiplier;
        auto const g2 = circuit + g.Multiplicand;
        VisitDispatcher(g1, std::move(k), -r1);
        VisitDispatcher(g2, std::move(oneCopy), -r2);
        VisitDispatcher(g1, std::move(kr2), std::move(r3));
        VisitDispatcher(g2, std::move(r1), std::move(b));
    }
};
