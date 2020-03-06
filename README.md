* stringdecimal
Simple library that does basic decimal maths to any precision on strings.

This includes functions for add, subtract, multiple, compare, divide and rounding.
Whilst add, subtract, and multiple always work to whatever precision is needed, division takes a number if places to stop infinite places being generated.
Various standard rounding, including (default) bankers rounding.

Also includes a simple expression processing allowing +, -, *, / and brackets.

Maths is actually done in decimal, so not designed for speed.
