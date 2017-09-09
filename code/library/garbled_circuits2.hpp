#ifndef GARBLED_CIRCUITS2_HPP_
#define GARBLED_CIRCUITS2_HPP_

#include<cstdlib>
#include<cstring>
#include<vector>
#include"cryptography.hpp"
#include"arithmetic_circuits.hpp"

#define TPC_TYPENAMES_ \
    typename TCAllocGate, \
    typename TCAllocHandle

#define CONF_TYPENAMES_ \
    typename TConfAllocSizeT

#define KEYPAIRS_TYPENAMES_ \
    typename TKPRing, \
    typename TKPAllocRing, \
    typename TKPAllocRingVec

#define KEYS_TYPENAMES_ \
    typename TKRing, \
    typename TKAllocRing, \
    typename TKAllocRingVec

#define TPC_TYPENAME_ARGS_ \
    TCAllocGate, \
    TCAllocHandle

#define CONF_TYPENAME_ARGS_ \
    TConfAllocSizeT

#define KEYPAIRS_TYPENAME_ARGS_ \
    TKPRing, \
    TKPAllocRing, \
    TKPAllocRingVec

#define KEYS_TYPENAME_ARGS_ \
    TKRing, \
    TKAllocRing, \
    TKAllocRingVec

namespace Cryptography
{
namespace ArithmeticCircuits
{
namespace Garbled2
{

#include"./garbled_circuits2_impl/data.hpp"

    namespace _CompilerImpl
    {

#include"./garbled_circuits2_impl/configure.hpp"
#include"./garbled_circuits2_impl/garble.hpp"
#include"./garbled_circuits2_impl/ungarble.hpp"

    }

    template <TPC_TYPENAMES_, CONF_TYPENAMES_>
    void Configure
    (
        TwoPartyCircuit<TPC_TYPENAME_ARGS_> &circuit,
        Configuration<CONF_TYPENAME_ARGS_> &config
    )
    {
        _CompilerImpl::Configure
        <
            TPC_TYPENAME_ARGS_,
            CONF_TYPENAME_ARGS_
        > compile_(circuit, config);
    }

    template
    <
        TPC_TYPENAMES_,
        KEYPAIRS_TYPENAMES_,
        typename TRandomGenerator,
        typename TRingDistribution
    >
    void Garble
    (
        TwoPartyCircuit<TPC_TYPENAME_ARGS_> &circuit,
        KeyPairs<KEYPAIRS_TYPENAME_ARGS_> &keypairs,
        TRandomGenerator &next,
        TRingDistribution &ringDist,
        TKPRing const &one = 1,
        TKPRing const &zero = 0
    )
    {
        _CompilerImpl::Garble
        <
            TPC_TYPENAME_ARGS_,
            KEYPAIRS_TYPENAME_ARGS_,
            TRandomGenerator,
            TRingDistribution
        > compile_(circuit, keypairs, next, ringDist, one, zero);
    }

    template
    <
        TPC_TYPENAMES_,
        CONF_TYPENAMES_,
        KEYS_TYPENAMES_,
        typename TOutputIt
    >
    void Ungarble
    (
        TwoPartyCircuit<TPC_TYPENAME_ARGS_> &circuit,
        Configuration<CONF_TYPENAME_ARGS_> &config,
        Keys<KEYS_TYPENAME_ARGS_> &keys,
        TOutputIt outputIterator
    )
    {
        _CompilerImpl::Ungarble
        <
            TPC_TYPENAME_ARGS_,
            CONF_TYPENAME_ARGS_,
            KEYS_TYPENAME_ARGS_
        > compile_(circuit, config, keys, outputIterator);
    }

}
}
}

#undef TPC_TYPENAMES_
#undef CONF_TYPENAMES_
#undef KEYPAIRS_TYPENAMES_
#undef KEYS_TYPENAMES_
#undef TPC_TYPENAME_ARGS_
#undef CONF_TYPENAME_ARGS_
#undef KEYPAIRS_TYPENAME_ARGS_
#undef KEYS_TYPENAME_ARGS_

#endif // GARBLED_CIRCUITS2_HPP_
