char const *InitBobContext(ExecutionContext *context)
{
    auto commonResult = InitCommonContext(context);
    if (commonResult)
        return commonResult;
    auto &prgole = context->PseudorandomOLE;
    auto &vecole = context->VectorOLE;
    auto &bob = context->Bob;
    bob.VecA.resize(prgole.M);
    bob.VecB.resize(prgole.M);
    FILE *fp = fopen(CommandLineParameters.BobA, "r");
    if (!fp)
        return "a: Could not open file.";
    auto loadResult = LoadZp.LoadRange(
        bob.VecA.data(), bob.VecA.data() + prgole.M,
        fp);
    fclose(fp);
    if (!loadResult)
        return "a: Could not load a from the file.";
    fp = fopen(CommandLineParameters.BobB, "r");
    if (!fp)
        return "b: Could not open file.";
    loadResult = LoadZp.LoadRange(
        bob.VecB.data(), bob.VecB.data() + prgole.M,
        fp);
    fclose(fp);
    if (!loadResult)
        return "b: Could not load b from the file.";
    bob.VecC.resize(prgole.M);
    bob.VecDV.resize(prgole.M);
    /* ~2M keys are sent in a batch to improve performance. */
    bob.VecBuf.resize(2097152);
    bob.VecGE.resize((vecole.U + vecole.V - vecole.U / 4 - vecole.V / 4) * (vecole.K + 1));
    return nullptr;
}

struct BobConnectsSocket
{
    char const *server;
    SocketWrappers::PortType port;
    SocketWrappers::SocketUnique *socketUnique;

    BobConnectsSocket(char const *server_, SocketWrappers::PortType port_, SocketWrappers::SocketUnique *su)
        : server(server_), port(port_), socketUnique(su)
    { }

    void operator () () const
    {
        SocketWrappers::SocketUnique tmpSocket = SocketWrappers::ClientConnectToServer(server, port);
        if (!tmpSocket.IsValid())
            return;
        SocketWrappers::SocketConsumer pipe = tmpSocket.RawValue();
        uint64_t pong;
        if (!pipe.Send(8, &PingMessage))
            return;
        if (!pipe.Receive(8, &pong) || pong != PongMessage)
            return;
        *socketUnique = std::move(tmpSocket);
    }
};

struct BobSendsBobsKeys
{
    ExecutionContext *context;

    BobSendsBobsKeys(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto &bob = context->Bob;
        auto const M = prgole.M;
        auto bobConfig = prgole.Config.BobEncoding.data();
        auto bobCoef = prgole.KeyPairs.BobCoefficient.data();
        auto bobInte = prgole.KeyPairs.BobIntercept.data();
        auto vecs = { bob.VecA.data(), bob.VecC.data() };
        auto const buffer = bob.VecBuf.data();
        auto const bufferSize = bob.VecBuf.size();
        auto bufferCurrent = buffer;
        auto bufferRemaining = bufferSize;
        SocketWrappers::SocketConsumer pipe = comm.Socket1.RawValue();
        if (!pipe.Send(8, &HelloMessage))
        {
            comm.Error1 = "Could not send hello message.";
            return;
        }
        for (auto vec : vecs)
        {
            for (auto i = vec, iend = vec + M; i != iend; ++i)
            {
                auto const *coef = (bobCoef++)->data();
                auto const *inte = (bobInte++)->data();
                for (size_t j = 0, jsz = *bobConfig++; j != jsz; ++j)
                {
                    /* send keys in batch */
                    if (!bufferRemaining)
                    {
                        if (!pipe.Send(sizeof(Zp) * bufferSize, buffer))
                        {
                            comm.Error1 = "Could not send batch of Bob's keys.";
                            return;
                        }
                        bufferCurrent = buffer;
                        bufferRemaining = bufferSize;
                    }
                    *bufferCurrent++ = *coef++ * *i + *inte++;
                    --bufferRemaining;
                }
            }
        }
        /* send the remaining keys */
        if (!pipe.Send(sizeof(Zp) * (bufferSize - bufferRemaining), buffer))
        {
            comm.Error1 = "Could not send last batch of Bob's keys.";
            return;
        }
        if (!pipe.Send(8, &ByeByeMessage))
        {
            comm.Error1 = "Could not send bye-bye message.";
            return;
        }
    }
};

struct BobDoesVecOle
{
    ExecutionContext *context;

    BobDoesVecOle(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto &vecole = context->VectorOLE;
        auto &stat = context->Statistics;
        auto const *aliceConfig = prgole.Config.AliceEncoding.data();
        auto const *aliceCoef = prgole.KeyPairs.AliceCoefficient.data();
        auto const *aliceInte = prgole.KeyPairs.AliceIntercept.data();
        auto const vecoleK = vecole.K;
        auto const prgoleK = prgole.K;
        auto const U = vecole.U;
        auto const V = vecole.V;
        auto const W = vecole.W;
        auto const vecR = vecole.VecR.data();
        auto const vecE = vecole.VecE.data();
        auto const vecM = vecole.VecM.data();
        auto const vecMTmp = vecole.VecMTmp.data();
        auto &vecNotNoisy = vecole.VecNotNoisy;
        auto &vecSolved = vecole.VecSolved;
        auto &sparse = vecole.SparseCode;
        auto &luby = vecole.LubyCode;
        auto &lubySurrogate = vecole.LubyCodeSurrogate;
        auto &bob = context->Bob;
        auto const vecGE = bob.VecGE.data();
        auto const vecGEsz = bob.VecGE.size();
        SocketWrappers::SocketConsumer pipe = comm.Socket2.RawValue();
        if (!pipe.Send(8, &HelloMessage))
        {
            comm.Error2 = "Could not send hello message.";
            return;
        }
        std::random_device randomSource;
        for (size_t i = 0; i != prgoleK; ++i)
        {
            auto const *aliceCoefI = (aliceCoef++)->data();
            auto const *aliceInteI = (aliceInte++)->data();
            for (size_t j = 0, jsz = *aliceConfig++; j != jsz; )
            {
                /* sample r */
                auto distR = MakeUZp();
                RNG nextR{randomSource()};
                std::thread sampR{SampleRandomVector(vecR, vecR + vecoleK, nextR, distR)};
                /* sample e */
                RNG nextSubset{randomSource()};
                vecNotNoisy.clear();
                vecNotNoisy.resize(U + V, true);
                EraseSubsetExact(vecNotNoisy.begin(), vecNotNoisy.begin() + U,
                    U / 4, nextSubset);
                EraseSubsetExact(vecNotNoisy.begin() + U, vecNotNoisy.begin() + U + V,
                    V / 4, nextSubset);
                /* prepare for computing E(r,a)+e */
                memset(vecE, 0, sizeof(Zp) * (U + V));
                if (jsz - j < W)
                {
                    memcpy(vecM, aliceCoefI, sizeof(Zp) * (jsz - j));
                    /* reset unused part of vecM to prevent accidental leakage */
                    memset(vecM + (jsz - j), 0, sizeof(Zp) * (W - (jsz - j)));
                    memcpy(vecMTmp, aliceInteI, sizeof(Zp) * (jsz - j));
                    /* resetting vecMTmp is unnecessary */
                }
                else
                {
                    memcpy(vecM, aliceCoefI, sizeof(Zp) * W);
                    memcpy(vecMTmp, aliceInteI, sizeof(Zp) * W);
                }
                /* finish sampling r */
                sampR.join();
                /* compute E(r,a) */
                sparse.EncodeBothParts(vecE, vecNotNoisy.begin(), vecR);
                luby.Encode(vecE + U, vecNotNoisy.begin() + U, vecM);
                /* compute E(r,a)+e */
                for (size_t i = 0; i != U + V; ++i)
                    if (!vecNotNoisy[i])
                        vecE[i] = distR(nextR);
                /* send E(r,a)+e to Alice */
                if (!pipe.Send(sizeof(Zp) * (U + V), vecE))
                {
                    comm.Error2 = "Could not send E(r,a)+e to Alice.";
                    return;
                }
                /* emulate OT */
                if (!pipe.Skip(sizeof(Zp) * (U + V)))
                {
                    comm.Error2 = "Could not emulate OT with Alice.";
                    return;
                }
                /* receive E(xr+r',xa+b') from Alice */
                if (!pipe.Receive(sizeof(Zp) * (U + V), vecE))
                {
                    comm.Error2 = "Could not receive E(xr+r',xa+b') from Alice.";
                    return;
                }
                /* try computing xa+b' */
                memset(vecGE, 0, sizeof(Zp) * vecGEsz);
                /* find xr+r' */
                if (!sparse.DecodeFromUpperPartDestructive(
                    vecE, vecNotNoisy.begin(),
                    vecR, vecGE, InverseZp))
                {
                    if (!pipe.Send(8, &FailedVecOleMessage))
                    {
                        comm.Error2 = "Could not send failed vector OLE message.";
                        return;
                    }
                    ++stat.UnsuccessfulVectorOLE;
                    continue;
                }
                /* compute -(xr+r') */
                for (auto z = vecR, zend = vecR + vecoleK; z != zend; ++z)
                    *z = -*z;
                /* find E(0,xa+b') */
                sparse.EncodeLowerPart(vecE + U, vecNotNoisy.begin() + U, vecR);
                vecSolved.clear();
                vecSolved.resize(W, false);
                lubySurrogate = luby;
                /* find xa+b' */
                if (!lubySurrogate.DecodeDestructive(
                    vecSolved.begin(), vecSolved.end(),
                    vecM,
                    vecNotNoisy.begin() + U,
                    vecNotNoisy.end(),
                    vecE + U, vecE + U + V))
                {
                    if (!pipe.Send(8, &FailedVecOleMessage))
                    {
                        comm.Error2 = "Could not send failed vector OLE message.";
                        return;
                    }
                    ++stat.UnsuccessfulVectorOLE;
                    continue;
                }
                if (!pipe.Send(8, &SuccessfulVecOleMessage))
                {
                    comm.Error2 = "Could not send successful vector OLE message.";
                    return;
                }
                /* compute b+xa+b' */
                for (size_t k = 0; k != W && j != jsz; ++k, ++j)
                    vecM[k] += vecMTmp[k];
                if (!pipe.Send(sizeof(Zp) * W, vecM))
                {
                    comm.Error2 = "Could not send b+xa+b' to Alice.";
                    return;
                }
                ++stat.SuccessfulVectorOLE;
            }
        }
        if (!pipe.Send(8, &ByeByeMessage))
        {
            comm.Error2 = "Could not send bye-bye message.";
            return;
        }
    }
};

struct BobEliminatesCryptoBlinding
{
    ExecutionContext *context;

    BobEliminatesCryptoBlinding(ExecutionContext *context_)
        : context(context_)
    { }

    void operator () () const
    {
        auto &comm = context->Communication;
        auto &prgole = context->PseudorandomOLE;
        auto const &gg = prgole.GoldreichFunc;
        auto &bob = context->Bob;
        auto const M = prgole.M;
        auto const vecA = bob.VecA.data();
        auto const vecB = bob.VecB.data();
        auto const vecC = bob.VecC.data();
        auto const vecDV = bob.VecDV.data();
        SocketWrappers::SocketConsumer pipe = comm.Socket3.RawValue();
        uint64_t payload;
        if (!pipe.Receive(8, &payload))
        {
            comm.Error3 = "Could not receive hello message.";
            return;
        }
        if (payload != HelloMessage)
        {
            comm.Error3 = "Bad hello message. Misaligned stream?";
            return;
        }
        /* receive D from Alice */
        if (!pipe.Receive(sizeof(Zp) * M, vecDV))
        {
            comm.Error3 = "Could not receive vector D = x - G(s).";
            return;
        }
        /* compute v=a*D+b-c */
        for (size_t i = 0; i != M; ++i)
            vecDV[i] = vecA[i] * vecDV[i] + (vecB[i] - vecC[i]);
        /* send v to Alice */
        if (!pipe.Send(sizeof(Zp) * M, vecDV))
        {
            comm.Error3 = "Could send vector v = a * (x - G(s)) + b - c.";
            return;
        }
        if (!pipe.Send(8, &ByeByeMessage))
        {
            comm.Error3 = "Could not send bye-bye message.";
            return;
        }
    }
};

int PlayBob()
{
    ExecutionContext context;
    auto &comm = context.Communication;
    auto &prgole = context.PseudorandomOLE;
    auto &bob = context.Bob;
    auto &stat = context.Statistics;    
    auto prepareResult = InitBobContext(&context);
    if (prepareResult)
    {
        PrintHelpfulInformation(prepareResult);
        return -10;
    }
    PrintHelpfulInformation("Connecting to Alice.");
    BobConnectsSocket bcs1{CommandLineParameters.ServerAddress, CommandLineParameters.Port1, &comm.Socket1};
    BobConnectsSocket bcs2{CommandLineParameters.ServerAddress, CommandLineParameters.Port2, &comm.Socket2};
    BobConnectsSocket bcs3{CommandLineParameters.ServerAddress, CommandLineParameters.Port3, &comm.Socket3};
    std::thread bcst1{bcs1};
    std::thread bcst2{bcs2};
    std::thread bcst3{bcs3};
    bcst1.join(); bcst2.join(); bcst3.join();
    if (!comm.Socket1.IsValid()
        || !comm.Socket2.IsValid()
        || !comm.Socket3.IsValid())
    {
        PrintHelpfulInformation("Could not connect to Alice or the endiannesses do not match.");
        return -11;
    }
    PrintHelpfulInformation("Connected to Alice.");
    PrintHelpfulInformation("Executing batch OLEs.");
    std::random_device randomSource;
    auto const K = prgole.K;
    auto const M = prgole.M;
    auto const vecC = bob.VecC.data();
    auto const vecDV = bob.VecDV.data();
    auto &configSurrogate = prgole.ConfigSurrogate;
    auto &circuit = prgole.Circuit;
    auto &keypairs = prgole.KeyPairs;
    auto startTime = Clock::now();
    for (auto i = CommandLineParameters.ExecutionCount; i--; )
    {
        RNG nextC{randomSource()}, nextGC{randomSource()};
        auto distC = MakeUZp(); auto distGC = MakeUZp();
        std::thread randC{SampleRandomVector(vecC, vecC + M, nextC, distC)};
        configSurrogate.ResetPreserveConfiguration();
        Garbled2::Garble(circuit, configSurrogate, keypairs, nextGC, distGC);
        randC.join();
        std::thread sendBob{BobSendsBobsKeys{&context}};
        std::thread unblinding{BobEliminatesCryptoBlinding{&context}};
        BobDoesVecOle{&context}();
        sendBob.join(); unblinding.join();
        if (comm.HasErrors())
        {
            comm.PrintErrors();
            return -12;
        }
    }
    auto endTime = Clock::now();
    PrintHelpfulInformation("Finished executing batch OLEs.");
    auto duration = endTime - startTime;
    stat.TotalSeconds = (double)duration.count() * Clock::period::num / Clock::period::den;
    PrintStatistics(&context);
    PrintHelpfulInformation("Done.");
    return 0;
}
