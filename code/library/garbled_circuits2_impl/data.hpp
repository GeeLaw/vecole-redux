template <typename TAllocSizeT = std::allocator<size_t>>
struct Configuration
{
    typedef std::vector<size_t, TAllocSizeT> SizeTVec;

    size_t OfflineEncoding;
    SizeTVec AliceEncoding, BobEncoding;

    void ResetPreserveConfiguration()
    {
        OfflineEncoding = 0;
        memset(AliceEncoding.data(), 0, AliceEncoding.size() * sizeof(size_t));
        memset(BobEncoding.data(), 0, BobEncoding.size() * sizeof(size_t));
    }
};

template
<
    typename TRing,
    typename TAllocRing = std::allocator<TRing>,
    typename TAllocRingVec = std::allocator<std::vector<TRing, TAllocRing>>
>
struct KeyPairs
{
    typedef std::vector<TRing, TAllocRing> RingVec;
    typedef std::vector<RingVec, TAllocRingVec> RingVec2;

    RingVec OfflineEncoding;
    RingVec2 AliceCoefficient, AliceIntercept;
    RingVec2 BobCoefficient, BobIntercept;

    template <typename TAllocSizeT>
    void ApplyConfiguration(Configuration<TAllocSizeT> const &config)
    {
        size_t sz;
        OfflineEncoding.clear();
        OfflineEncoding.reserve(config.OfflineEncoding);
        sz = config.AliceEncoding.size();
        AliceCoefficient.resize(sz);
        AliceIntercept.resize(sz);
        for (size_t i = 0; i != sz; ++i)
        {
            AliceCoefficient[i].clear();
            AliceCoefficient[i].reserve(config.AliceEncoding[i]);
            AliceIntercept[i].clear();
            AliceIntercept[i].reserve(config.AliceEncoding[i]);
        }
        sz = config.BobEncoding.size();
        BobCoefficient.resize(sz);
        BobIntercept.resize(sz);
        for (size_t i = 0; i != sz; ++i)
        {
            BobCoefficient[i].clear();
            BobCoefficient[i].reserve(config.BobEncoding[i]);
            BobIntercept[i].clear();
            BobIntercept[i].reserve(config.BobEncoding[i]);
        }
    }

    void ResetPreserveConfiguration()
    {
        OfflineEncoding.clear();
        for (auto &a : AliceCoefficient)
            a.clear();
        for (auto &a : AliceIntercept)
            a.clear();
        for (auto &b : BobCoefficient)
            b.clear();
        for (auto &b : BobIntercept)
            b.clear();
    }
};

template
<
    typename TRing,
    typename TAllocRing = std::allocator<TRing>,
    typename TAllocRingVec = std::allocator<std::vector<TRing, TAllocRing>>
>
struct Keys
{
    typedef std::vector<TRing, TAllocRing> RingVec;
    typedef std::vector<RingVec, TAllocRingVec> RingVec2;

    RingVec OfflineEncoding;
    RingVec2 AliceEncoding, BobEncoding;

    template <typename TAllocSizeT>
    void ApplyConfiguration(Configuration<TAllocSizeT> const &config)
    {
        size_t sz;
        OfflineEncoding.clear();
        OfflineEncoding.reserve(config.OfflineEncoding);
        AliceEncoding.resize(sz = config.AliceEncoding.size());
        for (size_t i = 0; i != sz; ++i)
        {
            AliceEncoding[i].clear();
            AliceEncoding[i].reserve(config.AliceEncoding[i]);
        }
        BobEncoding.resize(sz = config.BobEncoding.size());
        for (size_t i = 0; i != sz; ++i)
        {
            BobEncoding[i].clear();
            BobEncoding[i].reserve(config.BobEncoding[i]);
        }
    }

    void ResetPreserveConfiguration()
    {
        OfflineEncoding.clear();
        for (auto &a : AliceEncoding)
            a.clear();
        for (auto &b : BobEncoding)
            b.clear();
    }
};
