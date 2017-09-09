# `sparse_code.hpp`

This file defines utilities for sparse linear code in `Encoding::SparseLinearCode` namespace.

## `SparseMatrixEntry<TRing>` structure template

Defines an entry in a row-first sparse matrix.

## `SparseEncode` function template

Template arguments:

- `TForwardInputOutputIt1` should be a forward input/output iterator type. The iterator is used to output the encoded values.
- `TInputIt2` should be an input iterator type (it is not required to be a forward iterator as it is single-pass). The iterator is used to deteremine whether a position is noisy.
- `TRandomAccessInputIt3` should be a random-access input iterator type. The iterator is used to read the decoded (unencoded) elements.
- `TForwardInputIt4` should be a forward input iterator type. The iterator is used to read entries of the sparse matrix.

Formal parameters:

- `D`: a `size_t`, the sparsity parameter that specifies the entries each row;
- `count`: a `size_t`, the number of rows;
- `encoded`: a reference to `TForwardInputOutputIt1`, the begin iterator of encoded elements. It must have at least `count` elements from the value passed into the call. When the call returns, the iterator is advanced by `count`.
- `notNoisy`: a reference to `TInputIt2`, the begin iterator of a range of boolean values. It must have at least `count` elements from the value passed into the call. When the call returns, the iterator is advanced by `count`.
- `decoded`: a `TRandomAccessInputIt3`, the begin iterator of decoded (unencoded) elements. It must have at least `K` elements from the value passed into the call, where `K` is implicit in the `entries` array.
- `entries`: a reference to `TForwardInputIt4`, the begin iterator of entries of the sparse matrix. It must have at least `D` times `count` elements from the value passed into the call. When the call returns, the iterator is advanced by `D` times `count`.

Semantics:

Computes certain rows of `M * r` where `M` is the sparse matrix represented by `D` and `entries`, and where `r` is the random vector represented by `decoded`. The result is **added** to the corresponding positions of `encoded`.

## `DefaultInverseFunctor` constant object

A functor object of anonymous type. It is semantically equivalent to the template:

```C++
template <typename T>
T const DefaultInverseFunctor(T &&t)
{
    return 1 / std::forward<T>(t);
}
```

## `SparseDecodeDestructive` function template

Template arguments:

- `TInputIt1`: an input iterator type, used to read the encoded elements.
- `TInputIt2`: an input iterator type, used to determine whether an encoded position is noisy.
- `TOutputIt3`: an output iterator type, used to store the decoded elements should decoding be successful.
- `TForwardInputIt4`: a forward input iterator type, used to read the entries of the sparse matrix.
- `TRandomAccessInputOutputIt5`: a random-access input/output iterator type, used to access the temporary for Gaussian elimination.
- `TInverseFunctor`: a functor type specifying how the inverse is computed.

Formal parameters:

- `K`: a `size_t`, the number of decoded elements.
- `D`: a `size_t`, the sparsity parameter of the sparse matrix.
- `U`: a `size_t`, the number of rows in the encoded vector that can be used to perform Gaussian elimination, i.e., the “top part” of the vector as described in the paper.
- `encoded`: a `TInputIt1` that has at least `U` elements from the value on.
- `notNoisy`: a `TInputIt2` that has at least `U` boolean elements from the value on.
- `decoded`: a `TOutputIt3` that allows writing at least `K` elements from the value on.
- `entries`: a `TForwardInputIt4` that has at least `D` times `U` elements from the value on.
- `matrix`: a `TRandomAccessInputOutputIt5` that has at least `D` times `#[not noisy]` elements **initialised to zero** from the value on, where `#[not noisy]` is the number of `true`s among the first `U` elements from `notNoisy`. The values are rewritten during the computation, which is why the function is called “Destructive”.
- `inverse`: a `TInverseFunctor` that defaults to `DefaultInverseFunctor`.

Semantics:

The call first densifies the sparse matrix into an augmented matrix, keeping those noiseless rows. Then it performs Gaussian elimination, trying to solve the linear system. If there is a unique solution, it **moves** the solution to `decoded` and returns `true`. Otherwise, `decoded` is **not** written to and `false` is returned.

## `FastSparseLinearCode<TRing, TAllocEntry>` structure template

Template arguments:

- `TRing`: the field element type.
- `TAllocEntry`: defaults to `std::allocator<SparseMatrixEntry<TRing>>`.

Member types:

- `Entry` is `SparseMatrixEntry<TRing>`.
- `EntryVec` is `std::vector<Entry, TAllocEntry>`.

Member variables:

- `K`: a `size_t`, the length of the random vector.
- `D`: a `size_t`, the sparsity parameter of the sparse matrix.
- `U`: a `size_t`, the length of the top part.
- `V`: a `size_t`, the length of the bottom part.
- `Entries`: an `EntryVec`, each `D` elements form a row, and there shall be `D` times `(U + V)` elements.

Member function templates:

- `void Resample<TRNG, TRD>(TRNG &next, TRD &valDist)`
  - `TRNG` is a random number generator type.
  - `TRD` is a distribution type over the field.
  - `rng` is a reference to the random number generator.
  - `valDist` is a reference to the distribution.
  - Semantics: clears `Entries` and create a newly sample one from `K`, `D`, `U` and `V`. When the call returns, the internal states of the random number generator and the distribution are updated.
- `EncodeBothParts`, `EncodeUpperPart` and `EncodeLowerPart` are functions that performs matrix multiplication. **These functions do not normally modify the iterator passed into them — by default the iterators are passed by value.**
- `DecodeFromUpperPartDestructive` decodes from the upper part with custom temporary matrix.
- `DecodeFromUpperPartAutomatic` decodes from the upper part with automatically allocated and deallocated temporary matrix. General user should avoid using this function template as each call allocates new memory and can cause performance issue when used repeatedly.
- `void SaveTo<TSaveRing>(FILE *fp, TSaveRing saveRing)`
  - `TSaveRing` is a functor type.
  - `fp` is the file to which the sparse matrix is saved. The file should be opened with `w`.
  - `saveRing(ringElement, fp)` should serialise `ringElement` to `fp`.
- `void LoadFrom<TLoadRing>(FILE *fp, TLoadRing loadRing)`
  - `TLoadRing` is a functor type.
  - `fp` is the file from which the sparse matrix is loaded. The file should be opened with `r`.
  - `loadRing(ringElementReference, fp)` should deserialise a ring element from `fp` and save it to the reference `ringElementReference` upon success. The call should return a boolean value indicating whether deserialisation was successful.
