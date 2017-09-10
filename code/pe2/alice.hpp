char const *InitAliceContext(ExecutionContext *context)
{
    auto commonResult = InitCommonContext(context);
    if (commonResult)
        return commonResult;
    auto &prgole = context->PseudorandomOLE;
    auto &alice = context->Alice;
    alice.VecX.resize(prgole.M);
    FILE *fp = fopen(CommandLineParameters.AliceX, "r");
    if (!fp)
        return "x: Could not open file.";
    auto loadResult = LoadZp.LoadRange(
        alice.VecX.data(),
        alice.VecX.data() + prgole.M,
        fp);
    fclose(fp);
    if (!loadResult)
        return "x: Could not load x from the file.";
    alice.VecS.resize(prgole.K);
    alice.VecU.resize(prgole.M);
    alice.VecDVZ.resize(prgole.M);
    return nullptr;
}

struct AliceConnectsSocket
{
    SocketWrappers::PortType port;
    SocketWrappers::SocketUnique *socketUnique;

    AliceConnectsSocket(SocketWrappers::PortType p, SocketWrappers::SocketUnique *su)
        : port(p), socketUnique(su)
    { }

    void operator () () const
    {
        SocketWrappers::SocketUnique tmpSocket = SocketWrappers::ServerConnectToClient(port);
        if (!tmpSocket.IsValid())
            return;
        SocketWrappers::SocketConsumer pipe = tmpSocket.RawValue();
        uint64_t ping;
        if (!pipe.Receive(8, &ping) || ping != PingMessage)
            return;
        if (!pipe.Send(8, &PongMessage))
            return;
        *socketUnique = std::move(tmpSocket);
    }
};

struct AliceReceivesBobsKeys
{
    ExecutionContext *context;

    AliceReceivesBobsKeys(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto bobConfig = prgole.Config.BobEncoding.data();
        auto bobKeys = prgole.Keys.BobEncoding.data();
        SocketWrappers::SocketConsumer pipe = comm.Socket1.RawValue();
        uint64_t payload;
        if (!pipe.Receive(8, &payload))
        {
            comm.Error1 = "Could not receive hello message from Bob.";
            return;
        }
        if (payload != HelloMessage)
        {
            comm.Error1 = "Bad hello message. Misaligned stream?";
            return;
        }
        for (size_t i = 0, isz = 2 * prgole.M; i != isz; ++i)
        {
            if (!pipe.Receive(sizeof(Zp) * *bobConfig++, (bobKeys++)->data()))
            {
                comm.Error1 = "Could not receive Bob's keys.";
                return;
            }
        }
        if (!pipe.Receive(8, &payload))
        {
            comm.Error1 = "Could not receive bye-bye message.";
            return;
        }
        if (payload != ByeByeMessage)
        {
            comm.Error1 = "Bad bye-bye message. Misaligned stream?";
            return;
        }
    }
};

struct AliceDoesVecOle
{
    ExecutionContext *context;

    AliceDoesVecOle(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto &vecole = context->VectorOLE;
        auto &alice = context->Alice;
        auto &stat = context->Statistics;
        auto aliceConfig = prgole.Config.AliceEncoding.data();
        auto aliceKeys = prgole.Keys.AliceEncoding.data();
        auto const *vecS = alice.VecS.data();
        auto const vecR = vecole.VecR.data();
        auto const vecM = vecole.VecM.data();
        auto const vecMTmp = vecole.VecMTmp.data();
        auto const vecE = vecole.VecE.data();
        auto const itNeverNoisy = vecole.ItNeverNoisy;
        auto const U = vecole.U;
        auto const V = vecole.V;
        auto const W = vecole.W;
        auto const prgoleK = prgole.K;
        auto const vecoleK = vecole.K;
        auto &sparse = vecole.SparseCode;
        auto &luby = vecole.LubyCode;
        SocketWrappers::SocketConsumer pipe = comm.Socket2.RawValue();
        uint64_t payload;
        if (!pipe.Receive(8, &payload))
        {
            comm.Error2 = "Could not receive hello message.";
            return;
        }
        if (payload != HelloMessage)
        {
            comm.Error2 = "Bad hello message. Misaligned stream?";
            return;
        }
        std::random_device randomSource;
        for (size_t i = 0; i != prgoleK; ++i)
        {
            auto const s = *vecS++;
            auto aliceEncoding = (aliceKeys++)->data();
            for (size_t j = 0, jsz = *aliceConfig++; j != jsz; )
            {
                /* sample r' and b' */
                auto distRp = MakeUZp(), distBp = MakeUZp();
                RNG nextRp{randomSource()}, nextBp{randomSource()};
                auto sampRp = SampleRandomVector(vecR, vecR + vecoleK, nextRp, distRp);
                auto sampBp = SampleRandomVector(vecMTmp, vecMTmp + W, nextBp, distBp);
                std::thread randRp{sampRp};
                std::thread randBp{sampBp};
                /* receive E(r,a)+e from Bob */
                if (!pipe.Receive(sizeof(Zp) * (U + V), vecE))
                {
                    comm.Error2 = "Could not receive E(r,a)+e from Bob.";
                    return;
                }
                /* compute E(xr,xa) */
                for (auto k = U + V; k; vecE[--k] *= s)
                    ;
                /* compute E(xr+r',xa+b') */
                randRp.join();
                sparse.EncodeBothParts(vecE, itNeverNoisy, vecR);
                randBp.join();
                luby.Encode(vecE + U, itNeverNoisy, vecMTmp);
                /* send E(xr+r',xa+b') to Bob with OT emulation */
                if (!pipe.Send(sizeof(Zp) * (U + V), vecE))
                {
                    comm.Error2 = "Could not emulate OT of E(xr+r', xa+b') with Bob.";
                    return;
                }
                if (!pipe.Send(sizeof(Zp) * (U + V), vecE))
                {
                    comm.Error2 = "Could not send E(xr+r', xa+b') to Bob.";
                    return;
                }
                /* receive whether this vector OLE is successful*/
                if (!pipe.Receive(8, &payload))
                {
                    comm.Error2 = "Could not receive whether Bob successfully decoded the result.";
                    return;
                }
                /* if unsuccessful, retry */
                if (payload == FailedVecOleMessage)
                {
                    ++stat.UnsuccessfulVectorOLE;
                    continue;
                }
                if (payload != SuccessfulVecOleMessage)
                {
                    comm.Error2 = "Bad success/fail vector OLE message. Misaligned stream?";
                    return;
                }
                /* if successful, receive b+xa+b' from Bob */
                if (!pipe.Receive(sizeof(Zp) * W, vecM))
                {
                    comm.Error2 = "Could not receive b+xa+b' from Bob.";
                    return;
                }
                /* compute xa+b */
                for (size_t k = 0; j != jsz && k != W; ++j, ++k)
                    *aliceEncoding++ = vecM[k] - vecMTmp[k];
                ++stat.SuccessfulVectorOLE;
            }
        }
        if (!pipe.Receive(8, &payload))
        {
            comm.Error2 = "Could not receive bye-bye message.";
            return;
        }
        if (payload != ByeByeMessage)
        {
            comm.Error2 = "Bad bye-bye message. Misaligned stream?";
            return;
        }
    }
};

struct AliceUngarbles
{
    ExecutionContext *context;

    AliceUngarbles(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto &alice = context->Alice;
        auto vecU = alice.VecU.data();
        auto &circuit = prgole.Circuit;
        auto &configSurrogate = prgole.ConfigSurrogate;
        auto &keys = prgole.Keys;
        std::thread threadReceiveBobsKeys{AliceReceivesBobsKeys{context}};
        AliceDoesVecOle{context}();
        prgole.ConfigSurrogate.ResetPreserveConfiguration();
        threadReceiveBobsKeys.join();
        if (comm.HasErrors())
            return;
        Garbled2::Ungarble(circuit, configSurrogate, keys, vecU);
    }
};

struct AliceEliminatesCryptoBlinding
{
    ExecutionContext *context;

    AliceEliminatesCryptoBlinding(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto const &gg = prgole.GoldreichFunc;
        auto &alice = context->Alice;
        auto const M = prgole.M;
        auto const vecX = alice.VecX.data();
        auto const vecS = alice.VecS.data();
        auto const vecDVZ = alice.VecDVZ.data();
        auto const addArity = gg.A;
        auto const multArity = gg.B;
        auto const *storage = gg.Storage.data();
        SocketWrappers::SocketConsumer pipe = comm.Socket3.RawValue();
        if (!pipe.Send(8, &HelloMessage))
        {
            comm.Error3 = "Could not send hello message.";
            return;
        }
        /* compute D=x-G(s) */
        for (size_t i = 0; i != M; ++i)
        {
            Zp sum = 0;
            Zp prod = 1;
            for (auto j = addArity; j--; sum += vecS[*storage++])
                ;
            for (auto j = multArity; j--; prod *= vecS[*storage++])
                ;
            vecDVZ[i] = vecX[i] - sum - prod;
        }
        /* send D to Bob */
        if (!pipe.Send(sizeof(Zp) * M, vecDVZ))
        {
            comm.Error3 = "Could not send vector D = x - G(s).";
            return;
        }
        /* receive x*D+b-c from Bob */
        if (!pipe.Receive(sizeof(Zp) * M, vecDVZ))
        {
            comm.Error3 = "Could receive vector v = a * (x - G(s)) + b - c.";
            return;
        }
        uint64_t payload;
        if (!pipe.Receive(8, &payload))
        {
            comm.Error3 = "Could not receive bye-bye message.";
            return;
        }
        if (payload != ByeByeMessage)
        {
            comm.Error3 = "Bad bye-bye message. Misaligned stream?";
            return;
        }
    }
};

int PlayAlice()
{
    ExecutionContext context;
    auto &comm = context.Communication;
    auto &prgole = context.PseudorandomOLE;
    auto &alice = context.Alice;
    auto &stat = context.Statistics;
    auto prepareResult = InitAliceContext(&context);
    if (prepareResult)
    {
        PrintHelpfulInformation(prepareResult);
        return -10;
    }
    PrintHelpfulInformation("Connecting to Bob.");
    AliceConnectsSocket acs1{CommandLineParameters.Port1, &comm.Socket1};
    AliceConnectsSocket acs2{CommandLineParameters.Port2, &comm.Socket2};
    AliceConnectsSocket acs3{CommandLineParameters.Port3, &comm.Socket3};
    std::thread acst1{acs1};
    std::thread acst2{acs2};
    std::thread acst3{acs3};
    acst1.join(); acst2.join(); acst3.join();
    if (!comm.Socket1.IsValid()
        || !comm.Socket2.IsValid()
        || !comm.Socket3.IsValid())
    {
        PrintHelpfulInformation("Could not connect to Bob or the endiannesses do not match.");
        return -11;
    }
    PrintHelpfulInformation("Connected to Bob.");
    PrintHelpfulInformation("Executing batch OLEs.");
    auto const K = prgole.K;
    auto const M = prgole.M;
    auto const vecS = alice.VecS.data();
    auto const vecDVZ = alice.VecDVZ.data();
    auto const vecU = alice.VecU.data();
    std::random_device randomSource;
    auto startTime = Clock::now();
    for (auto i = CommandLineParameters.ExecutionCount; i--; )
    {
        RNG nextS{randomSource()};
        auto distS = MakeUZp();
        SampleRandomVector(vecS, vecS + K, nextS, distS)();
        std::thread unblinding{AliceEliminatesCryptoBlinding{&context}};
        AliceUngarbles{&context}();
        unblinding.join();
        if (comm.HasErrors())
        {
            comm.PrintErrors();
            return -12;
        }
        for (size_t j = 0; j != M; ++j)
            vecDVZ[j] += vecU[j];
    }
    auto endTime = Clock::now();
    PrintHelpfulInformation("Finished executing batch OLEs.");
    auto duration = endTime - startTime;
    stat.TotalSeconds = (double)duration.count() * Clock::period::num / Clock::period::den;
    PrintStatistics(&context);
    PrintHelpfulInformation("Printing the result of batch OLEs to stdout.");
    for (auto const &v : alice.VecDVZ)
        printf("%ju\n", (uintmax_t)(uint32_t)v);
    PrintHelpfulInformation("Done.");
    return 0;
}
