# `vecole` folder

This example executes one round of vector OLE protocol.

## `vecole.cpp` program

A typical invocation for Alice is:

```
vecole alice 12345 sparse luby < alice > result
```

A typical invocation for Bob is:

```
vecole 127.0.0.1 12345 sparse luby < bob
```

Finally, compare the file `stdans` with `result`.

**Note** `stdans` uses `LF`. Windows uses `CR LF` for line-ending.

## `alice` input file

A sample input file for Alice. It contains only one field element.

## `bob` input file

A sample input file for Bob. It contains 20000 field elements.

## `sparse` parameter file

A sample fast sparse linear code.

## `luby` parameter file

A sample LT code.

## `stdans` file

The correct answer that should be produced by Alice when used with sample input files.
