template <TPC_TYPENAMES_, CONF_TYPENAMES_>
struct Configure
    : GateVisitorCRTP
    <
        Configure<TPC_TYPENAME_ARGS_, CONF_TYPENAME_ARGS_>,
        void()
    >
{
    typedef TwoPartyCircuit<TPC_TYPENAME_ARGS_> TwoPartyCircuitType;
    typedef Configuration<CONF_TYPENAME_ARGS_> ConfigurationType;

    Gate *circuit;
    ConfigurationType &config;

    Configure(
        TwoPartyCircuitType &tpc,
        ConfigurationType &conf
    ) : circuit(tpc.Gates.data()), config(conf)
    {
        config.OfflineEncoding = 0;
        auto const alice = tpc.AliceInputEnd - tpc.AliceInputBegin;
        auto const bob = tpc.BobInputEnd - tpc.BobInputBegin;
        config.AliceEncoding.resize(alice);
        config.BobEncoding.resize(bob);
        memset(config.AliceEncoding.data(), 0, alice * sizeof(size_t));
        memset(config.BobEncoding.data(), 0, bob * sizeof(size_t));
        for (auto const ao : tpc.AliceOutput)
            this->VisitDispatcher(circuit + ao);
    }

private:
    friend class GateVisitorCRTP
        <
            Configure<TPC_TYPENAME_ARGS_, CONF_TYPENAME_ARGS_>,
            void()
        >;

    void VisitUnmatched(Gate *)
    {
        std::exit(-99);
    }

    void VisitConstZero(Gate *)
    {
        ++config.OfflineEncoding;
    }

    void VisitConstOne(Gate *)
    {
        ++config.OfflineEncoding;
    }

    void VisitConstMinusOne(Gate *)
    {
        ++config.OfflineEncoding;
    }

    void VisitInputGate(Gate *that)
    {
        auto const &g = that->AsInputGate;
        auto const index = g.MajorIndex;
        auto const owner = g.Agent;
        if (owner == AgentFlag::Alice)
        {
            ++config.AliceEncoding[index];
            return;
        }
        if (owner == AgentFlag::Bob)
        {
            ++config.BobEncoding[index];
            return;
        }
        exit(-99);
    }

    void VisitAdditionGate(Gate *that)
    {
        auto const &g = that->AsAdditionGate;
        VisitDispatcher(circuit + g.Augend);
        VisitDispatcher(circuit + g.Addend);
    }

    void VisitNegationGate(Gate *that)
    {
        auto const &g = that->AsNegationGate;
        VisitDispatcher(circuit + g.Target);
    }

    void VisitSubtractionGate(Gate *that)
    {
        auto const &g = that->AsSubtractionGate;
        VisitDispatcher(circuit + g.Minuend);
        VisitDispatcher(circuit + g.Subtrahend);
    }

    void VisitMultiplicationGate(Gate *that)
    {
        auto const &g = that->AsMultiplicationGate;
        auto const g1 = circuit + g.Multiplier;
        auto const g2 = circuit + g.Multiplicand;
        VisitDispatcher(g1);
        VisitDispatcher(g2);
        VisitDispatcher(g1);
        VisitDispatcher(g2);
    }
};
