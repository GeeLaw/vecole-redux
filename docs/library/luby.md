# `luby.hpp`

This file defines utilities in `Encoding::LubyTransform` namespace.

## `RobustSolitonDistribution` structure

Represents a robust Soliton distribution. Users should tweak `InputSymbolSize`, `C` and `Delta` and call `InvalidateCache` before using the instance.

Calling `InvalidateCache` will recompute other fields from the three tweaked fields. Later, a usual user should use `SampleDegree` to obtain a degree from a random real number between 0 and 1.

## `LubyBin` structure

Represents a bin of Luby Transform code. This structure is coupled with storage pattern employed by `LTCode` structure template.

- `Index` field: indicates the starting index of participating input elements.
- `Degree` field: indicates the degree (number of participating inputs) of the bin.
- `TIt const GetBegin<TIt>(TIt const &i) const` function template: returns `i + Index`, the `begin` iterator for the range of indices of inputs occupied by the bin.
- `TIt const GetEnd<TIt>(TIt const &i) const` function template: returns `i + Index + Degree`, the `end` iterator for the range of indices of inputs occupied by the bin.

A producer of Luby Transform code should fill the underlying storage along with the `LubyBin`s.

A consumer of Luby Transform code should index into the underlying storage when using a `LubyBin`. It is often convenient to use `GetBegin` and `GetEnd` templates.

## `LTCode<TALB, TAU>` structure template

Template arguments:

- `TALB` is an allocator type for `LubyBin` and defaults to `std::allocator<LubyBin>`.
- `TAU` is an allocator type for `size_t` and defaults to `std::allocator<size_t>`.

Members:

- `InputSymbolSize` field: the number of inputs.
- `Bins` field: a vector of `LubyBin`s, the `size()` of which is the number of outputs.
- `Storage` field: the underlying storage of indices of participating inputs for the outputs.
  - The indices are 0-based. `Storage[bin.Index, bin.Index + bin.Degree)` contains the indices of inputs that will form the output of `bin`.
- `void Encode(TFIOIt3 encoded, TFIIt4 notNoisy, TRAIIt5 decoded) const` function template: encodes `decoded` into `encoded`.
  - `encoded` should have length at least `Bins.size()` and must be initialised to zero before passing to the call. The values are added to old values contained by `encoded`. The iterator should be an input/output iterator that dereferences to an ableian group type `TAbelianGroup`.
  - `notNoisy` should have length at least `Bins.size()` and indicates whether the call should produce encode at a position (for `notNoisy[i] == false`, `encoded[i]` is left untouched). The iterator should be an input iterator that dereferences to `bool`.
  - `decoded` should have length at least `InputSymbolSize` and is the unencoded group elements. The iterator should be random access input iterator that dereferences to the abelian group type `TAbelianGroup`.
- `bool DecodeDestructive(TRAIOIt1 sB, TRAIOIt1 sE, TRAIOIt4 d, TBIIt5 nB, IBIIt5 nE, TBIOIt6 eB, TBIOIt6 eE)` function template: decodes `[eB, eE)` into `d` and returns whether decoding was successful.
  - `TRAIOIt1`: a random access input/output iterator that represents the range of `solved` array.
  - `TRAOIt4`: a random access input/output iterator that represents the range of `decoded` array.
  - `TBIIt5`: a bidirectional input iterator that represents the range of `notNoisy` array.
  - `TBIOIt6`: a bidirectional input/output iterator that represents the range of `encoded` array.
  - `[sB, sE) = bool solved[InputSymbolSize]` must be initialised to `false` when passed to the call.
  - `d = TAbelianGroup decoded[InputSymbolSize]` **does not** have to initialised to zero.
  - `[nB, nE) = bool notNoisy[Bins.size()]` indicates whether a position in `encoded` is noisy (erased).
  - `[eB, eE) = TAbelianGroup encoded[Bins.size()]` contains the encoded symbols.
  - The call **is destructive** and will modify `solved`, `decoded`, `encoded` and containers of the calling `LTCode` object. However, it is guaranteed that the capacities of containers are not changed as the operation only **`remove`s** elements around without actually **`erase`-ing** them.
  - The call does not modify `notNoisy`!
  - If decoding is successful, `decoded` contains the decoded group elements. Otherwise, partial decoding might have happened and it is only guaranteed that the objects are in a consistent state, but their values are not meaningful.
- `bool LoadFrom(FILE *fp)` function: loads Luby Transform code from `fp` and returns whether loading was successful. The call **does not** check index validity at all and fails if and only if reading integers from the file has failed.
- `void SaveTo(FILE *fp)` function: saves Luby Transform code to `fp` for later loading.

Semantics:

> Represents a Luby Transform code that is ready to be used to encoding and decoding. It is **idiomatic**, during consecutive calls for decoding, to use a temporary `surrogate` object and assign the real code object to it every time before calling `DecodeDestructive` on `surrogate`. The structure has a custom copy-assignment operator and performs copying in the containers as fast as possible.

## `LTEncode`/`LTDecode` function templates

An iterator-based version of `LTCode::Encode`/`LTCode::Decode`. It is the actual underlying implementation and is used by `LTCode::Encode`/`LTCode::Decode`.

## `CreateLTCode` function template

Template arguments:

- `TALB`: an allocator type for `LubyBin`.
- `TAU`: an allocator type for `size_t`.
- `TRG`: a random generator type.

Formal parameters:

- `RobustSolitonDistribution const &dist`: the distribution used to create the Luby Transform code. Its `InvalidateCache` must be called before passing to the call and after any modification of its fields.
- `LTCode<TALB, TAU> &code`: the object used to store the code.
- `TRG &next`: the random number generator.

Semantics:

> Generates a Luby Transform code from the given distribution with the given random number generator. If the random number generator is a pseudo-random generator, and the state is fixed upon passing into the call, and the `dist` is also fixed, and floating-point operation has the same behaviour across the runs, the function will generated the same Luby Transform code.
