typedef Z<4294967291u> Zp;
typedef std::mt19937 RNG;

struct ZpUniformDistribution
{
    std::uniform_int_distribution<uint32_t> underlying;
    
    ZpUniformDistribution()
        : underlying(0u, 4294967290u)
    { }

    template <typename TRandomGenerator>
    Zp operator () (TRandomGenerator &next)
    {
        return underlying(next);
    }
};

ZpUniformDistribution MakeUZp()
{
    return {};
}

void PrintHelpfulInformation(char const *info)
{
    fputs(info, stderr);
    fputc('\n', stderr);
}

struct ExecutionContext
{
    struct CommunicationTag
    {
        typedef char const *PCString;
        SocketWrappers::SocketUnique Socket1, Socket2, Socket3;
        PCString Error1, Error2, Error3;
        CommunicationTag()
            : Error1(nullptr), Error2(nullptr), Error3(nullptr)
        { }
        bool HasErrors()
        {
            return Error1 || Error2 || Error3;
        }
        void PrintErrors()
        {
            if (Error1)
            {
                PrintHelpfulInformation("Error on Bob key transfer:");
                PrintHelpfulInformation(Error1);
            }
            if (Error2)
            {
                PrintHelpfulInformation("Error on vector OLE:");
                PrintHelpfulInformation(Error2);
            }
            if (Error3)
            {
                PrintHelpfulInformation("Error on eliminating cryptographic blinding:");
                PrintHelpfulInformation(Error3);
            }
        }
    } Communication;
    struct VectorOLETag
    {
        size_t K, U, V, W;
        LTCode<> LubyCode, LubyCodeSurrogate;
        FastSparseLinearCode<Zp> SparseCode;
        std::vector<Zp> VecR;
        std::vector<Zp> VecE;
        std::vector<Zp> VecM;
        std::vector<Zp> VecMTmp;
        VectorOLETag()
            : K(0), U(0), V(0), W(0)
        { }
        struct TrueIteratorTag
        {
            TrueIteratorTag &operator ++ ()
            {
                return *this;
            }
            TrueIteratorTag operator ++ (int)
            {
                return *this;
            }
            bool operator * () const
            {
                return true;
            }
        } ItNeverNoisy; /* It = iterator, not a grammar mistake here. */
        std::vector<bool> VecNotNoisy;
        std::vector<bool> VecSolved;
    } VectorOLE;
    struct PseudorandomOLETag
    {
        size_t K, M;
        GoldreichGraph<> GoldreichFunc;
        TwoPartyCircuit<> Circuit;
        Garbled2::Configuration<> Config;
        Garbled2::Configuration<> ConfigSurrogate;
        Garbled2::KeyPairs<Zp> KeyPairs;
        Garbled2::Keys<Zp> Keys;
        PseudorandomOLETag()
            : K(0), M(0)
        { }
    } PseudorandomOLE;
    struct AliceTag
    {
        /* input */
        std::vector<Zp> VecX;
        /* random seed vector s for G(s) */
        std::vector<Zp> VecS;
        /* result of pseudorandom-OLE */
        std::vector<Zp> VecU;
        /* D = x - G(s), sent to Bob;
         * v = a * D + b - c, received from Bob;
         * z = a * x + b = u + v, computed by Alice.
         */
        std::vector<Zp> VecDVZ;
    } Alice;
    struct BobTag
    {
        /* input */
        std::vector<Zp> VecA, VecB;
        /* random */
        std::vector<Zp> VecC;
        /* D = x - G(s), from Alice;
         * v = a * D + b - c, sent to Alice.
         */
        std::vector<Zp> VecDV;
        /* buffers Bob's keys */
        std::vector<Zp> VecBuf;
        /* temporary vector for Gaussian elimination */
        std::vector<Zp> VecGE;
    } Bob;
    struct StatisticsTag
    {
        double TotalSeconds;
        size_t SuccessfulVectorOLE;
        size_t UnsuccessfulVectorOLE;
        size_t AliceKeyLength, BobKeyLength;
        size_t VectorOLEPerBatchOLE;
        StatisticsTag()
            : TotalSeconds(0),
            SuccessfulVectorOLE(0),
            UnsuccessfulVectorOLE(0),
            AliceKeyLength(0), BobKeyLength(0),
            VectorOLEPerBatchOLE(0)
        { }
    } Statistics;
};

struct
{
    typedef char const *PCString;
    bool IsAlice;
    PCString ServerAddress;
    SocketWrappers::PortType Port1, Port2, Port3;
    PCString LubyCode, SparseCode, GoldreichFunc;
    PCString AliceX, BobA, BobB;
    size_t ExecutionCount;
} CommandLineParameters;

constexpr uint64_t PingMessage = 0x42de0135245310ed;
constexpr uint64_t PongMessage = 0x4201356738573920;
constexpr uint64_t HelloMessage = 0x4242424242424242;
constexpr uint64_t ByeByeMessage = 0x8888888888888888;
constexpr uint64_t SuccessfulVecOleMessage = 0x6666666666666666;
constexpr uint64_t FailedVecOleMessage = 0x0000000000000000;

void PrintUsage()
{
    fputs
    (
        "Usage: pe2 { alice | ipv4 }\n"
        "           port1 port2 port3\n"
        "           luby sparse prg\n"
        "           { x | a b } count\n\n"
        "Parameters:\n"
        "     alice: the literal string \"alice\", runs the\n"
        "            program as Alice.\n"
        "      ipv4: an IPv4 address where Alice is located,\n"
        "            runs the program as Bob.\n"
        "   port<n>: 3 different ports through which the two\n"
        "            agents communicate.\n"
        "      luby: the file name of Luby code.\n"
        "    sparse: the file name of sparse linear code.\n"
        "       prg: the file name of Goldreich's function.\n"
        "         x: the file name of Alice's input.\n"
        "      a, b: the file names of Bob's inputs.\n"
        "     count: the number of batches to run.\n",
        stderr
    );
}

struct
{
    bool operator () (Zp &zp, FILE *fp) const
    {
        uintmax_t uv;
        if (fscanf(fp, "%ju", &uv) != 1)
            return false;
        zp = (uint32_t)uv;
        return true;
    }
    template <typename T>
    bool LoadRange(T begin, T end, FILE *fp) const
    {
        for (; begin != end; ++begin)
        {
            uintmax_t uv;
            if (fscanf(fp, "%ju", &uv) != 1)
                return false;
            *begin = (Zp)(uint32_t)uv;
        }
        return true;
    }
} const LoadZp;

struct
{
    Zp const operator () (Zp const v) const
    {
        return v.Inverse();
    }
} InverseZp;

int ParseCommandLine(int argc, char **argv)
{
    if (argc != 10 && argc != 11)
        return 1;
    if (strcmp(argv[1], "alice") == 0)
    {
        if (argc != 10)
            return 1;
        CommandLineParameters.IsAlice = true;
    }
    else
    {
        if (argc != 11)
            return 1;
        CommandLineParameters.IsAlice = false;
        CommandLineParameters.ServerAddress = argv[1];
    }
    uintmax_t port;
    if (sscanf(argv[2], "%ju", &port) != 1 || port < 1 || port > 65535)
    {
        PrintHelpfulInformation("port1: the argument must be a port (1-65535).");
        return -1;
    }
    CommandLineParameters.Port1 = (uint16_t)port;
    if (sscanf(argv[3], "%ju", &port) != 1 || port < 1 || port > 65535)
    {
        PrintHelpfulInformation("port2: the argument must be a port (1-65535).");
        return -1;
    }
    CommandLineParameters.Port2 = (uint16_t)port;
    if (CommandLineParameters.Port1 == CommandLineParameters.Port2)
    {
        PrintHelpfulInformation("port2: the argument must be different from port1.");
        return -2;
    }
    if (sscanf(argv[4], "%ju", &port) != 1 || port < 1 || port > 65535)
    {
        PrintHelpfulInformation("port3: the argument must be a port (1-65535).");
        return -1;
    }
    CommandLineParameters.Port3 = (uint16_t)port;
    if (CommandLineParameters.Port1 == CommandLineParameters.Port3)
    {
        PrintHelpfulInformation("port3: the argument must be different from port1.");
        return -2;
    }
    if (CommandLineParameters.Port2 == CommandLineParameters.Port3)
    {
        PrintHelpfulInformation("port3: the argument must be different from port2.");
        return -2;
    }
    CommandLineParameters.LubyCode = argv[5];
    CommandLineParameters.SparseCode = argv[6];
    CommandLineParameters.GoldreichFunc = argv[7];
    char const *countBuffer;
    if (CommandLineParameters.IsAlice)
    {
        CommandLineParameters.AliceX = argv[8];
        countBuffer = argv[9];
    }
    else
    {
        CommandLineParameters.BobA = argv[8];
        CommandLineParameters.BobB = argv[9];
        countBuffer = argv[10];
    }
    uintmax_t count;
    if (sscanf(countBuffer, "%ju", &count) != 1 || count < 1 || count > 5000000)
    {
        PrintHelpfulInformation("count: must be a natural number from 1 to 5000000.");
        return -3;
    }
    CommandLineParameters.ExecutionCount = count;
    return 0;
}

char const *LoadLuby(ExecutionContext *context)
{
    FILE *fp = fopen(CommandLineParameters.LubyCode, "r");
    if (!fp)
        return "luby: Could not open file.";
    auto loadResult = context->VectorOLE.LubyCode.LoadFrom(fp);
    context->VectorOLE.LubyCodeSurrogate = context->VectorOLE.LubyCode;
    fclose(fp);
    return loadResult ? nullptr : "luby: File is not valid Luby code.";
}

char const *LoadSparse(ExecutionContext *context)
{
    FILE *fp = fopen(CommandLineParameters.SparseCode, "r");
    if (!fp)
        return "sprase: Could not open file.";
    auto loadResult = context->VectorOLE.SparseCode.LoadFrom(fp, LoadZp);
    fclose(fp);
    return loadResult ? nullptr : "sparse: File is not valid sprase linear code.";
}

char const *LoadGoldreichFunc(ExecutionContext *context)
{
    FILE *fp = fopen(CommandLineParameters.GoldreichFunc, "r");
    if (!fp)
        return "prg: Could not open file.";
    auto loadResult = context->PseudorandomOLE.GoldreichFunc.LoadFrom(fp);
    fclose(fp);
    return loadResult ? nullptr : "prg: File is not valid Goldreich's function.";
}

struct BuildPseudorandomOLECircuit
{
    ExecutionContext *context;

    BuildPseudorandomOLECircuit(ExecutionContext *context_)
        : context(context_)
    { }

    GateHandle CreateProduct(GateHandle const *factors, size_t sz) const
    {
        if (sz == 0)
            exit(-99);
        if (sz == 1)
            return *factors;
        size_t halfSz = sz / 2;
        GateHandle g1 = CreateProduct(factors, halfSz);
        GateHandle g2 = CreateProduct(factors + halfSz, sz - halfSz);
        auto &tpc = context->PseudorandomOLE.Circuit;
        return tpc.InsertGate(MultiplicationGateData{ g1, g2 });
    }

    GateHandle CreateSum(GateHandle const *summands, size_t sz) const
    {
        if (sz == 0)
            exit(-99);
        if (sz == 1)
            return *summands;
        size_t halfSz = sz / 2;
        GateHandle g1 = CreateSum(summands, halfSz);
        GateHandle g2 = CreateSum(summands + halfSz, sz - halfSz);
        auto &tpc = context->PseudorandomOLE.Circuit;
        return tpc.InsertGate(AdditionGateData{ g1, g2 });
    }

    void operator () () const
    {
        PrintHelpfulInformation("Building pseudorandom-OLE functionality.");
        auto &prgole = context->PseudorandomOLE;
        auto &gg = prgole.GoldreichFunc;
        auto &tpc = prgole.Circuit;
        tpc.AliceInputBegin = 0;
        tpc.AliceInputEnd = gg.InputLength;
        tpc.BobInputBegin = gg.InputLength;
        tpc.BobInputEnd = gg.InputLength + gg.OutputLength + gg.OutputLength;
        for (size_t i = 0; i != gg.InputLength; ++i)
            tpc.InsertGate(InputGateData{ AgentFlag::Alice, i, 0 });
        for (size_t i = 0; i != gg.OutputLength; ++i)
            tpc.InsertGate(InputGateData{ AgentFlag::Bob, i, 0 });
        for (size_t i = 0; i != gg.OutputLength; ++i)
            tpc.InsertGate(InputGateData{ AgentFlag::Bob, gg.OutputLength + i, 0 });
        std::vector<GateHandle> summands, factors, outputSummands;
        summands.resize(gg.A);
        factors.resize(gg.B + 1);
        outputSummands.resize(3);
        auto storage = gg.Storage.data();
        for (size_t i = 0; i != gg.OutputLength; ++i)
        {
            outputSummands[0] = gg.InputLength + gg.OutputLength + i;
            for (size_t j = 0; j != gg.A; ++j, ++storage)
                summands[j] = *storage;
            for (size_t j = 0; j != gg.B; ++j, ++storage)
                factors[j] = *storage;
            factors[gg.B] = gg.InputLength + i;
            outputSummands[1] = tpc.InsertGate(
                MultiplicationGateData{
                    gg.InputLength + i,
                    CreateSum(summands.data(), gg.A)
                });
            outputSummands[2] = CreateProduct(factors.data(), gg.B + 1);
            tpc.AliceOutput.push_back(CreateSum(outputSummands.data(), 3));
        }
        PrintHelpfulInformation("Finished building pseudorandom-OLE functionality.");
    }
};

char const *InitCommonContext(ExecutionContext *context)
{
    PrintHelpfulInformation("Initialising common execution context.");
    auto &vecole = context->VectorOLE;
    auto &prgole = context->PseudorandomOLE;
    auto &luby = vecole.LubyCode;
    auto &sparse = vecole.SparseCode;
    auto &stat = context->Statistics;
    char const *ret;
    if ((ret = LoadLuby(context)) != nullptr)
        return ret;
    if ((ret = LoadSparse(context)) != nullptr)
        return ret;
    if ((ret = LoadGoldreichFunc(context)) != nullptr)
        return ret;
    if (luby.Bins.size() != sparse.V)
        return "luby, sparse: Luby code output does not match sparse linear code.";
    vecole.K = sparse.K;
    vecole.U = sparse.U;
    vecole.V = sparse.V;
    vecole.W = luby.InputSymbolSize;
    vecole.VecR.resize(vecole.K);
    vecole.VecE.resize(vecole.U + vecole.V);
    vecole.VecM.resize(vecole.W);
    vecole.VecMTmp.resize(vecole.W);
    BuildPseudorandomOLECircuit{context}();
    prgole.K = prgole.GoldreichFunc.InputLength;
    prgole.M = prgole.GoldreichFunc.OutputLength;
    Garbled2::Configure(prgole.Circuit, prgole.Config);
    prgole.ConfigSurrogate = prgole.Config;
    prgole.KeyPairs.ApplyConfiguration(prgole.Config);
    prgole.Keys.ApplyConfiguration(prgole.Config);
    for (auto i : prgole.Config.AliceEncoding)
    {
        stat.VectorOLEPerBatchOLE += (i + vecole.W - 1) / vecole.W;
        stat.AliceKeyLength += i;
    }
    for (auto i : prgole.Config.BobEncoding)
        stat.BobKeyLength += i;
    PrintHelpfulInformation("Finished initialising common execution context.");
    return nullptr;
}

template <typename TIt, typename TRandomGenerator, typename TDistribution>
struct SampleRandomVectorTag
{
    TIt begin, end;
    TRandomGenerator *next;
    TDistribution *dist;

    SampleRandomVectorTag(TIt begin_, TIt end_, TRandomGenerator &next_, TDistribution &dist_)
        : begin(begin_), end(end_), next(&next_), dist(&dist_)
    { }

    void operator () ()
    {
        for (; begin != end; ++begin)
            *begin = (*dist)(*next);
    }
};

template <typename TIt, typename TRandomGenerator, typename TDistribution>
SampleRandomVectorTag<TIt, TRandomGenerator, TDistribution>
SampleRandomVector(TIt begin, TIt end, TRandomGenerator &next, TDistribution &dist)
{
    return SampleRandomVectorTag<TIt, TRandomGenerator, TDistribution>
        (begin, end, next, dist);
}

void PrintStatistics(ExecutionContext const *context)
{
    auto const n = CommandLineParameters.ExecutionCount;
    auto const &prgole = context->PseudorandomOLE;
    auto const &stat = context->Statistics;
    fprintf(stderr,
        "Statistics:\n"
        "               Total time: %.6f min\n"
        "      Time per  batch OLE: %.6f s\n"
        "      Time per vector OLE: %.6f ms\n"
        "      Time per        OLE: %.6f us\n"
        "Vector OLEs per batch OLE: %zu\n"
        "         Alice key length: %zu\n"
        "           Bob key length: %zu\n"
        "       Failed vector OLEs: %zu\n"
        "   Successful vector OLEs: %zu\n",
        stat.TotalSeconds / 60,
        stat.TotalSeconds / n,
        stat.TotalSeconds * 1000 / (stat.UnsuccessfulVectorOLE + stat.SuccessfulVectorOLE),
        stat.TotalSeconds * 1000000 / n / prgole.M,
        stat.VectorOLEPerBatchOLE,
        stat.AliceKeyLength,
        stat.BobKeyLength,
        stat.UnsuccessfulVectorOLE,
        stat.SuccessfulVectorOLE);
}
