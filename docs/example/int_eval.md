# `int_eval.cpp`

This example compiles a circuit that computes `ax+b`, where `x` is the input from Alice, `a` and `b` are the inputs from Bob. After compilation, it reads the three input values from the standard stream, encodes the input and prints the result of decoding. It uses `int` as the ring and the random values are drawn from -150 to 149.

## `GateMemory<T>` structure template

A structure that represents a record in the memoized memory.

## `GatePerformer<TGC, TMC>` structure template

A structure that employs visitor pattern and uses `GateVisitorCRTP`to do memoized computation (avoiding unnecessary recursion into gates whose value has been computed earlier).
