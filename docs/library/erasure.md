# `erasure.hpp`

The file defines utilities in `Encoding::Erasure` namespace.

## `EraseSubsetExact` function template

Template arguments:

- `TIt` a random access iterator type.
- `TRG` a random generator type.

Formal parameters:

- `TIt const begin` the begin iterator of `notNoisy` array.
- `TIt const end` the end iterator of `notNoisy` array.
- `unsigned count` the count of elements to erase.
- `TRG &next` the random generator.

Usage:

> The `[begin, end)` should be a boolean array, in which, initially, there must be at least `count` elements that are `true`. The array indicates whether a position is **not** noisy (`true` implies that the position is **not** erased). Upon returning, the function sets `count` elements in `[begin, end)` to `false`, all of which are originally `true`. It uses `next` to produce candidate positions for erasure.
