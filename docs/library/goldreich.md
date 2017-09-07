# `goldreich.hpp`

Defines utilities related to Goldreich’s functions in `Cryptography::Goldreich` namespace.

## `GoldreichGraph<TAU>` structure template

Template argument `TAU` is an allocator for `size_t`.

Members:

- `InputLength`: `k` in the paper, the number of seeds.
- `OutputLength`: `m` in the paper, the number of outputs.
- `A`: addition arity.
- `B`: multiplication arity.
- `Storage`: a `vector` of `size_t`s, the underlying storage of the parameters.
- `Resample(TRNG &)`: samples a Goldreich’s function.
- `SaveTo` and `LoadFrom` are (de)serialisation functions.

The layout of `Storage` is as the following:

- Each output is represented by a subarray of `A + B` elements.
- Inside each output, the first `A` elements are the summands, the following (and the last) `B` elements are the factors. The values are zero-based indices in the range `[0, InputLength)`.
- The length of `Storage` should be `OutputLength * (A + B)`.
