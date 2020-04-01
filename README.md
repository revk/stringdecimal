# stringdecimal

Simple library that does basic decimal maths to any precision on C strings.

See stringdecimal.h for the various calls available. String answers are malloced, and functions exist to free the arguments used, making it simple to do a series of sums if you wish.

This includes functions for add, subtract, multiply, compare, divide and rounding.

Whilst add, subtract, and multiple always work to whatever precision is needed, division takes a number of decimal places to limit the result. Allows various common rounding algorithms, including (default) bankers rounding.

A general purpose "eval" function parses sums using +, -, ÷, ×, (, and ) to produce an answer. ASCII/alternative versions /, \*, also work. Internally this uses rational numbers if you have any division other than by a power of 10, so only does the one division at the end to specified limit of decimal places. Hence 1000/7*7 is 1, not 994 or some other "nearly 1" answer. It also understands operator precisdence, so 1+2\*3 is 7, not 9.

Additional comparison operators return 1 for true and 0 for false: <, >, ≤, ≥, =, ≠. ASCII/alternative versions >=, <=, !=, == also work.

Additional logic operators return 1 for true and 0 for false, and assume non zero is true: ∧, ∨, and unary ¬. ASCII/alternative versions &&, ||, ! also work.

The code actually includes a general expression parse which allows extensions on the stringdecimal logic if needed.

And yes, it will happily (if you have the memory) work out 1e1000000000+1 if you ask it to.
