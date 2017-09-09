# `garbled_circuits2.hpp` and `garbled_circuits2_impl`

On-the-fly version of garbled circuits in the namespace `Cryptography::ArithmeticCircuits::Garbled2`.

The implementation is so large that they are splitted into four files in the folder `garbled_circuits2_impl`:

- `data.hpp` defines the data model.
- `configure.hpp` implements the compiler that given a `TwoPartyCircuit`, outputs its *configuration* in garbled form.
- `garble.hpp` implements the encoding algorithm that given a `TwoPartyCircuit` and appropriate random generation utilities, outputs one possible garbled form (represented by keys and key pairs) of the circuit.
- `ungarble.hpp` implements the decoding algorithm that given a `TwoPartyCircuit` and one possible garbled form (represented by keys that come from the randomness and the input, therefore no “key pairs”), outputs the output of the original circuit.

It should be well noted that the configuration, the garbled form and the algorithms are strongly cohesive in the sense that the convetion of the layout of the keys are implicit. Care must be taken to not mix the result with outside libraries. In contrast, `garbled_circuits.hpp` provides utilities that compile `TwoPartyCircuit`s into encoding and decoding circuits, where the the layout of the keys are explicit in the compiled circuits.

## `Configuration<TAllocSizeT>` structure template

Represents the configuration of a garbled circuit. `TAllocSizeT` is an allocator type for `size_t` and defaults to `std::allocator<size_t>`.

The configuration of a garbled circuit of a `TwoPartyCircuit` is the number of offline keys (those that do not depend on the input of Alice or Bob), the number of key pairs (the coefficients and the intercepts, or the vector `a_i(r)` and `b_i(r)`) of each input of Alice, and that of Bob.

`ResetPreserveConfiguration` resets the counters but keeps the dimensional sizes of `AliceEncoding` and `BobEncoding`. It is required to call this method before passing a `Configuration` into `Ungarble`, which uses the object as the counter.

## `KeyPairs<TRing, TAllocRing, TAllocRingVec>` structure template

Template argument `TRing` is the ring type. `TAllocRing` should be an allocator of `TRing` and defaults to `std::allocator<TRing>`. `TAllocRingVec` should be an allocator of `std::vector<TRing, TAllocRing>` and defaults to `std::allocator<std::vector<TRing, TAllocRing>>`.

Represents one possible result of garbled circuits. The concrete values of the input are not yet considered, therefore the “keys” for the input of Alice and Bob are really “key pairs”.

The two member methods are useful for optimisation and usage.

- `ApplyConfiguration` clears the structure and reserves at least the final size that a compilation wiil require. If the circuit is quite large, it is useful to first compute the size and reserve. However, even if the circuit is small, `KeyPairs` and `Keys` objects are required to be `ApplyConfiguration`ed before being passed into the calls so that they have the correct size in the higher dimension.
- `ResetPreserveConfiguration` clears the structure, keeping the reserved size effective. For successive garbling of the same circuit, it is idiomatic to use `ResetPreserveConfiguration` before each call to `Garble` instead of using a fresh new object.

## `Keys<TRing, TAllocRing, TAllocRingVec>` structure template

The template arguments have the same meaning as those of `KeyPairs`’.

Represents one possible result of garbled circuit with the input fixed. Usually computed from a `KeyPairs` with the inputs and part of them might be obiliviously evaluated per application of vector-OLE on NC0 functionalities.

The two memeber methods are useful for optimisation. See that part of `KeyPairs`.

## `void Configure(TPC &circuit, CONF &config)` function template

Finds out the configuration of the garbled form of `circuit` and stores it in `config`.

`TPC` is an instantiation of `TwoPartyCircuit` and `CONF` is an instantiation of `Configuration`.

The parameter `circuit` is ***not*** modified during the execution. The parameter `config` need not be clean when passed into the call because the call will clean it.

## `void Garble(TPC &circuit, KP &keyparis, RNG &next, DIST &dist, Ring const &zero = 0, Ring const &one = 1)` function template

Compiles `circuit` into its garbled formed stored in `keypairs` with random bit source `next` and distribution of ring elements `dist`. The ring zero and identity are provided as `zero` and `one` arguments.

`TPC` is an instantiation of `TwoPartyCircuit`. `KP` is an instantiation of `KeyPairs`. `RNG` should be a random generator (as defined in standard C++). `DIST` should be a distribution (as defined in standard C++) over the ring elements. `Ring` is the ring type implied by `KP`.

The input `circuit` is ***not*** modified during the execution. It is assumed that `keypairs` is reset before being passed into the call (e.g., using `ResetPreserveConfiguration`). Internal states of `next` and `dist` might well advance.

## `void Ungarble(TPC &circuit, CONF &config, K &keys, TOutputIt outputIt)` function template

`TPC` is an instantiation of `TwoPartyCircuit`. `CONF` is an instantiation of `Configuration`. `K` is an instantiation of `Keys`. `TOutputIt` is an output iterator type and need *not* be a forward iterator type.

The input `circuit` is ***not*** modified during the execution. `config` is used as a counter (thus modified) and should have the configuration of the garbled circuit, and should be reset before being passed into the call. Ring elements in `keys` might be *moved* (thus modified), but its configuration shall not change. `outputIt` should be able to be put as many ring elements as `circuit` has outputs following its position.
