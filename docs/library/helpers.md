# `helpers.hpp`

Helper methods in `Helpers` namespace.

## `SaveSizeTRange` function template

Template arugment `TIt` is a forward iterator type that iterates `size_t`. Formal parameters are:

- `TIt begin`: the begin iterator of the range to save.
- `TIt end`: the end iterator of the range to save.
- `FILE *fp`: the `FILE` pointer.

Before calling the function, it is assumed the content in `fp` already has a delimiter. After the call, the content in `fp` will be delimited by a newline character.

## `LoadSizeTRange` function template

Template argument `TIt` is a forward output iterator type that iterates `size_t`. Formal parameters are:

- `TIt begin`: the begin iterator of the range to load.
- `TIt end`: the end iterator of the range to load.
- `FILE *fp`: the `FILE` pointer.

Return `true` if the loading was successful.
