// Functions to perform simple maths on decimal value strings
// (c) Copyright 2019 Andrews & Arnold Adrian Kennard
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef	STRINGDECIMAL_H
#define	STRINGDECIMAL_H

// Perform basic decimal maths with arbitrary precision
// This library has two sets of functions.
//
// stringdecimal_* calls process strings containing a number and return a malloc'd string with the answer
// e.g. strindecimal_add("1","2") returns strdup("3")
// The _*fc, _*cf, _*ff, _f functions free one or both arguments
//
// sd_* calls allow processing of the sd_p type.
// You can convert string to sd_p with sd_parse
// You can convert sd_p to string with sd_output
//
// Note that these library calls have C++ style, even though they are C code.
// e.g. sd_output(v) outputs v with defaults, but sd_output(v,round:'F') does floor rounding, etc
// https://www.revk.uk/2020/08/pseudo-c-using-cpp.html


extern char sd_comma;           // Comma thousands character
extern char sd_point;           // Decimal point character
extern int sd_max;              // Max internal variable, characters as printed excluding comma/sign

// Rounding options

typedef enum {
   // Default (i.e. round:0) is 'B'
   SD_ROUND_TRUNCATE = 'T',     // Towards zero
   SD_ROUND_UP = 'U',           // Away from zero
   SD_ROUND_FLOOR = 'F',        // Towards -ve
   SD_ROUND_CEILING = 'C',      // Towards +ve
   SD_ROUND_ROUND = 'R',        // Away from zero if 0.5 or more
   SD_ROUND_BANKING = 'B',      // Away from zero if above 0.5, or 0.5 exactly and goes to even
} sd_round_t;

// TODO revise logic, and adjust jsql/xmlsql accordingly
// Decimal places formatting options (used with "places" argument)
// Default: If format is 0 (i.e. not set) and places is 0, then a format of '+' and places of 3 are used
// Default: If format is 0 and places is non zero then a format of '=' is used
typedef enum {
   SD_FORMAT_LIMIT = '-',       // Use as many places as necessary, limiting division, no padding 0's
   // places: sets the maximum for division, but more places may be used if not division
   SD_FORMAT_EXACT = '=',       // Include padding 0's as needed to specified number of places
   // places: sets the exact number of places, used for money, for example
   SD_FORMAT_EXTRA = '+',       // Try to use enough places even for division, no padding
   // places: is added to a guess of number of places for division
   SD_FORMAT_MAX = '>',         // Use as many places as necessary, limited to max places seen in args
   // places: is added to the max places seen
   SD_FORMAT_INPUT = '*',       // Round to the max places seen in the args
   // places: is added to the max places seen
   SD_FORMAT_EXP = 'e',         // Exponent (scientific notation)
   // Places: fixed number of places in mantissa, negative places make a guess
   SD_FORMAT_SI = 'S',          // SI suffix
   // Places: Max places
   SD_FORMAT_IEEE = 'I',        // IEEE suffix
   // Places: Max places
   SD_FORMAT_RATIONAL = '/',    // Output as integer or integer/integer, places is not used
   SD_FORMAT_FRACTION = '%',    // Try to output fractions
} sd_format_t;

typedef struct {                // Binary stringdecimal operations
   const char *a;               // First argument
   const char *b;               // Second argument
   int places;                  // Number of places
   sd_format_t format;          // Decimal places formatting
   sd_round_t round;            // Rounding
   unsigned char nocomma:1;     // Do not allow commas when parsing
   unsigned char a_free:1;      // Free first argument
   unsigned char b_free:1;      // Free second argument
   unsigned char comma:1;       // Add comma in output
   unsigned char nofrac:1;      // No fractions when parsing
   unsigned char nosi:1;        // No SI suffix when parsing
   unsigned char noieee:1;      // No IEEE suffix when parsing
   unsigned char unicode:1;     // Use Fractions on output
   const char **failure;        // Error string
} stringdecimal_binary_t;
typedef struct {                // Unary stringdecimal operations
   const char *a;               // Argument
   int places;                  // Number of places
   sd_format_t format;          // Decimal places formatting
   sd_round_t round;            // Rounding
   unsigned char nocomma:1;     // Do not allow commas when parsing
   unsigned char a_free:1;      // Free argument
   unsigned char comma:1;       // Add comma in output
   unsigned char nofrac:1;      // No fractions when parsing
   unsigned char nosi:1;        // No SI suffix when parsing
   unsigned char noieee:1;      // No IEEE suffix when parsing
   unsigned char unicode:1;     // Use Fractions on output
   const char **failure;        // Error string
} stringdecimal_unary_t;
typedef struct {                // Division stringdecimal operation
   const char *a;               // First argument
   const char *b;               // Second argument
   int places;                  // Number of places
   sd_format_t format;          // Decimal places formatting
   sd_round_t round;            // Rounding
   char **remainder;            // Store remainder value
   unsigned char nocomma:1;     // Do not allow commas when parsing
   unsigned char a_free:1;      // Free first argument
   unsigned char b_free:1;      // Free second argument
   unsigned char comma:1;       // Add comma in output
   unsigned char nofrac:1;      // No fractions when parsing
   unsigned char nosi:1;        // No SI suffix when parsing
   unsigned char noieee:1;      // No IEEE suffix when parsing
   unsigned char unicode:1;     // Use Fractions on output
   const char **failure;        // Error string
} stringdecimal_div_t;

// These _cf, _fc, _ff options free one or both args
// The stringdecimal functions are high level

char *stringdecimal_add_opts(stringdecimal_binary_t);
#define	stringdecimal_add(...)		stringdecimal_add_opts((stringdecimal_binary_t){__VA_ARGS__})
#define	stringdecimal_add_cf(...)	stringdecimal_add_opts((stringdecimal_binary_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_add_fc(...)	stringdecimal_add_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_add_ff(...)	stringdecimal_add_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_sub_opts(stringdecimal_binary_t);
#define	stringdecimal_sub(...)		stringdecimal_sub_opts((stringdecimal_binary_t){__VA_ARGS__})
#define	stringdecimal_sub_cf(...)	stringdecimal_sub_opts((stringdecimal_binary_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_sub_fc(...)	stringdecimal_sub_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_sub_ff(...)	stringdecimal_sub_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_mul_opts(stringdecimal_binary_t);
#define	stringdecimal_mul(...)		stringdecimal_mul_opts((stringdecimal_binary_t){__VA_ARGS__})
#define	stringdecimal_mul_cf(...)	stringdecimal_mul_opts((stringdecimal_binary_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_mul_fc(...)	stringdecimal_mul_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_mul_ff(...)	stringdecimal_mul_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_div_opts(stringdecimal_div_t);
#define	stringdecimal_div(...)		stringdecimal_div_opts((stringdecimal_binary_t){__VA_ARGS__})
#define	stringdecimal_div_cf(...)	stringdecimal_div_opts((stringdecimal_binary_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_div_fc(...)	stringdecimal_div_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_div_ff(...)	stringdecimal_div_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_rnd_opts(stringdecimal_unary_t);
#define	stringdecimal_rnd(...)		stringdecimal_rnd_opts((stringdecimal_unary_t){__VA_ARGS__})
#define	stringdecimal_rnd_f(...)	stringdecimal_rnd_opts((stringdecimal_unary_t){__VA_ARGS__,a_free:1})
int stringdecimal_cmp_opts(stringdecimal_binary_t);     // Return -ve, 0, or +ve
#define stringdecimal_cmp(...)          stringdecimal_cmp_opts((stringdecimal_binary_t){__VA_ARGS__})
#define stringdecimal_cmp_cf(...)       stringdecimal_cmp_opts((stringdecimal_binary_t){__VA_ARGS__,b_free:1})
#define stringdecimal_cmp_fc(...)       stringdecimal_cmp_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1})
#define stringdecimal_cmp_ff(...)       stringdecimal_cmp_opts((stringdecimal_binary_t){__VA_ARGS__,a_free:1,b_free:1})

// Low level functions allow construction of an expression using rational maths
typedef struct sd_s sd_t;
typedef sd_t *sd_p;

typedef struct {                // Parse options
   const char *a;               // Argument string
   const char **end;            // Where to store pointer for next character after parsed value
   unsigned char nocomma:1;     // Do not allow commas when parsing
   unsigned char a_free:1;      // Free argument
   unsigned char nofrac:1;      // No fractions when parsing
   unsigned char nosi:1;        // No SI suffix when parsing
   unsigned char noieee:1;      // No IEEE suffix when parsing
   const char **failure;        // Error report
} sd_parse_t;
typedef struct {                // Output options
   sd_p p;                      // Argument
   int places;                  // Number of places
   sd_format_t format;          // Decimal places formatting
   sd_round_t round;            // Rounding
   unsigned char p_free:1;      // Free argument
   unsigned char comma:1;       // Add comma in output
   unsigned char unicode:1;     // Use Fractions on output
   const char **failure;        // Error report
} sd_output_opts_t;

const char *sd_fail(sd_p);      // Failure string
void *sd_free(sd_p);            // Free sd_p
sd_p sd_copy(sd_p p);           // Make copy
sd_p sd_int(long long);         // Make from integer
sd_p sd_float(long double);     // Make from float

sd_p sd_parse_opts(sd_parse_t);
#define	sd_parse(...)		sd_parse_opts((sd_parse_t){__VA_ARGS__})
#define	sd_parse_f(...)		sd_parse_opts((sd_parse_t){__VA_ARGS__,a_free:1})
char *sd_output_opts(sd_output_opts_t); // Malloc'd output
#define	sd_output(...)		sd_output_opts((sd_output_opts_t){__VA_ARGS__})
#define	sd_output_f(...)		sd_output_opts((sd_output_opts_t){__VA_ARGS__,p_free:1})
const char *sd_check_opts(sd_parse_t);  // Returns NULL if not valid, else returns next character after parsed number
#define	sd_check(...)	sd_check_opts((sd_parse_t){__VA_ARGS__})
#define	sd_check_f(...)	sd_check_opts((sd_parse_t){__VA_ARGS__,a_free:1})

int sd_places(sd_p);            // Max places of any operand so far
int sd_iszero(sd_p);            // If zero value
int sd_isneg(sd_p);             // If negative value
int sd_ispos(sd_p);             // If positive value

sd_p sd_neg_i(sd_p);            // Negate (in place, returns arg)
sd_p sd_abs_i(sd_p);            // Absolute (in place, returns arg)
sd_p sd_inv_i(sd_p);            // Reciprocal (in place, returns arg)
sd_p sd_10_i(sd_p, int);        // Multiple by a power of 10 (in place, returns arg)
sd_p sd_neg(sd_p);              // Negate
sd_p sd_abs(sd_p);              // Absolute
sd_p sd_inv(sd_p);              // Reciprocal
sd_p sd_10(sd_p, int);          // Multiple by a power of 10

sd_p sd_add(sd_p, sd_p);        // Add
sd_p sd_add_ff(sd_p, sd_p);     // Add free all args
sd_p sd_add_fc(sd_p, sd_p);     // Add free first arg
sd_p sd_add_cf(sd_p, sd_p);     // Add free second arg
sd_p sd_sub(sd_p, sd_p);        // Subtract
sd_p sd_sub_ff(sd_p, sd_p);     // Subtract free all args
sd_p sd_sub_fc(sd_p, sd_p);     // Subtract free first arg
sd_p sd_sub_cf(sd_p, sd_p);     // Subtract free second arg
sd_p sd_mul(sd_p, sd_p);        // Multiply
sd_p sd_mul_ff(sd_p, sd_p);     // Multiply free all args
sd_p sd_mul_fc(sd_p, sd_p);     // Multiply free first arg
sd_p sd_mul_cf(sd_p, sd_p);     // Multiply free second arg
sd_p sd_div(sd_p, sd_p);        // Divide
sd_p sd_div_ff(sd_p, sd_p);     // Divide free all args
sd_p sd_div_fc(sd_p, sd_p);     // Divide free first arg
sd_p sd_div_cf(sd_p, sd_p);     // Divide free second arg
sd_p sd_pow(sd_p, sd_p);        // Integer power
sd_p sd_pow_ff(sd_p, sd_p);     // Integer power free all args
sd_p sd_pow_fc(sd_p, sd_p);     // Integer power free first arg
sd_p sd_pow_cf(sd_p, sd_p);     // Integer power free second arg
int sd_cmp(sd_p, sd_p);         // Compare
int sd_cmp_ff(sd_p, sd_p);      // Compare free all args
int sd_cmp_fc(sd_p, sd_p);      // Compare free first arg
int sd_cmp_cf(sd_p, sd_p);      // Compare free second arg
int sd_abs_cmp(sd_p, sd_p);     // Compare absolute values
int sd_abs_cmp_ff(sd_p, sd_p);  // Compare absolute values free all args
int sd_abs_cmp_fc(sd_p, sd_p);  // Compare absolute values free first arg
int sd_abs_cmp_cf(sd_p, sd_p);  // Compare absolute values free second arg
#endif
