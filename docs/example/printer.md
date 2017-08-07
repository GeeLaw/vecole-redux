# Example: `printer.cpp`

This example compiles a circuit that computes `ax+b`, where `x` is the input from Alice, `a` and `b` are the inputs from Bob. After compilation, it prints the encoding circuit and the decoding circuit.

## `ExpressionPrinter<TContainer>` structure template

A structure that employs visitor pattern and uses `GateVisitorCRTP`to recursively print the expression determined by the gate.

It is also possible to use a similar design to output a C++ program that does the encoding/decoding.
