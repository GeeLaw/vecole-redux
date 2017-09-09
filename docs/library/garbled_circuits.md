# `garlbed_circuits.hpp`

This file defines utilities in `Cryptography::ArithmeticCircuits::Garbled` namespace, which one uses to represent and build garbled circuits.

## `KeyPair` structure

Pair of `GateHandle`s in an affine encoding specifying the linear part (`Coefficient`) and the constant part (`Intercept`).

The structure has two static member function templates `SaveRange` and `LoadRange` for serialisation and deserialisation.

## `EncodingCircuit<TAG, TAKP, TAKPV, TAH>` structure template

It represents an encoding circuit. The template arguments are allocators. Members:

- `Gates` is a vector of `Gate`s, deciding the handles.
- `Randomness` is a vector of `GateHandle`s, storing the input gates to which a fresh new random value should be supplied when computing the random encoding.
- `OfflineEncoding` is a vector of `GateHandle`s, storing the gates that produce the offline part (constant part) of the encoding.
- `AliceEncoding` is a jagged two-dimensional vector of `KeyPair`s, the k-th vector of which is stores the gates that produce the key pairs of the k-th input from Alice.
- `BobEncoding` is similar to `AliceEncoding`, just for Bob.
- `SaveTo` and `LoadFrom` (de)serialises the structure.

## `DecodingCircuit<TAG, TAH, TAHV>` structure template

It represents a decoding circuit. The template arguments are allocators. Members:

- `Gates` is the old friend that stores all the `Gate`s.
- `OfflineEncoding` is a vector of `GateHandle`s, which are input gates in the decoding circuit and should be fed with the values received from the encoder.
- `AliceEncoding` is a jagged two-dimensional vector of `GateHandle`s, which are input gates in the decoding circuit and should be fed with the results of vector-OLE (suppose Bob does the random encoding).
- `BobEncoding` is similar to `AliceEncoding`, which are input gates in the decoding circuit and should be fed with the encoding for Bob’s part of input.
- `AliceOutput` is a vector of `GateHandle`s, which produce the output for Alice.
- `SaveTo` and `LoadFrom` (de)serialises the structure.

## `CompileToDare` function template

The function has three arguments, a reference `circuit` to `TwoPartyCircuit<blah>`, a reference `encoder` to `EncodingCircuit<blah>` and a reference `decoder` to `DecodingCircuit<blah>`. It compiles `circuit` and stores the resulting pair of circuits in `encoder` and `decoder`.

If `circuit` is well-formed, the compiler guarantees that upon returning from the call:

- `circuit` is lefted unchanged.
- `encoder` and `decoder` are well-formed.
- `encoder.AliceEncoding.size()` equals `circuit.AliceInputEnd - circuit.AliceInputBegin`.
  - Semantically, `encoder.AliceEncoding[i]` stores the key pairs for `circuit.Gates[circuit.AliceInputBegin + i]`.
- `encoder.BobEncoding.size()` equals `circuit.BobInputEnd - circuit.BobInputBegin`.
  - Semantically, `encoder.BobEncoding[i]` stores the key pairs for `circuit.Gates[circuit.BobInputBegin + i]`.
- `decoder.AliceOutput.size()` equals `circuit.AliceOutput.size()`.
- `encoder.OfflineEncoding.size()` equals `decoder.OfflineEncoding.size()`.
- For each `i`, `encoder.AliceEncoding[i].size()` equals `decoder.AliceEncoding[i].size()`.
- For each `i`, `encoder.BobEncoding[i].size()` equals `decoder.BobEncoding[i].size()`.
- `encoder` and `decoder` have the following property: a `Gate` with larger handle depends only on `Gate`s with smaller handle, i.e., the `Gate`s are topologically sorted. (N.B.: The input `circuit` does not have to satisfy this condition.)
- If one supplies random values to random input gates in the `encoder`, computes the encoding, does the linear evaluation and supplies the encoding to `decoder`, `decoder` will produce the outputs that equal the output that would be produced if the same input were supplied to `circuit`.
- The encoding from `encoder` and linear evaluation depends only on the outputs.

## `_CompilerImpl::CircuitGarbler<blah>` structure template

This structure implements the compiling process. It employs the previously mentioned visitor pattern and uses `GateVisitorCRTP`.

To understand how the process is done, refer to the paper and the comments in the code.
