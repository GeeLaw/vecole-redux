template <TPC_TYPENAMES_, CONF_TYPENAMES_, KEYS_TYPENAMES_>
struct Ungarble
    : GateVisitorCRTP
    <
        Ungarble
        <
            TPC_TYPENAME_ARGS_,
            CONF_TYPENAME_ARGS_,
            KEYS_TYPENAME_ARGS_
        >,
        TKRing()
    >
{
    typedef TwoPartyCircuit<TPC_TYPENAME_ARGS_> TwoPartyCircuitType;
    typedef Configuration<CONF_TYPENAME_ARGS_> ConfigurationType;
    typedef Keys<KEYS_TYPENAME_ARGS_> KeysType;

    Gate *circuit;
    ConfigurationType &conf;
    KeysType &keys;

    template <typename TOutputIt>
    Ungarble(
        TwoPartyCircuitType &tpc,
        ConfigurationType &config,
        KeysType &kt,
        TOutputIt outputIt
    ) : circuit(tpc.Gates.data()), conf(config), keys(kt)
    {
        for (auto const ao : tpc.AliceOutput)
        {
            *outputIt = this->VisitDispatcher(circuit + ao);
            ++outputIt;
        }
    }

private:
    friend class GateVisitorCRTP
        <
            Ungarble
            <
                TPC_TYPENAME_ARGS_,
                CONF_TYPENAME_ARGS_,
                KEYS_TYPENAME_ARGS_
            >,
            TKRing()
        >;

    TKRing VisitUnmatched(Gate *)
    {
        std::exit(-99);
    }

    TKRing VisitConstZero(Gate *)
    {
        return std::move(keys.OfflineEncoding[conf.OfflineEncoding++]);
    }

    TKRing VisitConstOne(Gate *)
    {
        return std::move(keys.OfflineEncoding[conf.OfflineEncoding++]);
    }

    TKRing VisitConstMinusOne(Gate *)
    {
        return std::move(keys.OfflineEncoding[conf.OfflineEncoding++]);
    }

    TKRing VisitInputGate(Gate *that)
    {
        auto const &g = that->AsInputGate;
        auto const index = g.MajorIndex;
        auto const owner = g.Agent;
        if (owner == AgentFlag::Alice)
            return std::move(keys.AliceEncoding[index][conf.AliceEncoding[index]++]);
        if (owner == AgentFlag::Bob)
            return std::move(keys.BobEncoding[index][conf.BobEncoding[index]++]);
        exit(-99);
    }

    TKRing VisitAdditionGate(Gate *that)
    {
        auto const &g = that->AsAdditionGate;
        auto &&g1 = this->VisitDispatcher(circuit + g.Augend);
        auto &&g2 = this->VisitDispatcher(circuit + g.Addend);
        return std::move(g1) + std::move(g2);
    }

    TKRing VisitNegationGate(Gate *that)
    {
        auto const &g = that->AsNegationGate;
        return this->VisitDispatcher(circuit + g.Target);
    }

    TKRing VisitSubtractionGate(Gate *that)
    {
        auto const &g = that->AsSubtractionGate;
        auto &&g1 = this->VisitDispatcher(circuit + g.Minuend);
        auto &&g2 = this->VisitDispatcher(circuit + g.Subtrahend);
        return std::move(g1) - std::move(g2);
    }

    TKRing VisitMultiplicationGate(Gate *that)
    {
        auto const &g = that->AsMultiplicationGate;
        auto const g1 = circuit + g.Multiplier;
        auto const g2 = circuit + g.Multiplicand;
        auto &&x1 = this->VisitDispatcher(g1);
        auto &&x2 = this->VisitDispatcher(g2);
        auto &&x3 = this->VisitDispatcher(g1);
        auto &&x4 = this->VisitDispatcher(g2);
        return std::move(x1) * std::move(x2) + (std::move(x3) + std::move(x4));
    }
};
