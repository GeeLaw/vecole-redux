# `sparseuse.cpp`

This example loads a fast sparse linear code from the standard input stream, tests it for a certain number of times and prints the estimated probability of successful decoding under certain erasure to the standard output.

The concrete parameters are:

- Number of tests: 20000.
- Erasure method: erase exactly a quarter of the codes.

It also checks whether the decoded result coincides with the unencoded values and reports such error once found.
