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
// Value strings are optional minus sign, digits, option decimal place plus digits, any precision
// Functions return malloced string answers (or NULL for error)
// Functions have variants to free arguments
//
// Add, Sub, Mul all work to necessary places
// Div works to specified number of places, specified rounding rule, and can return remainder value

#define	STRINGDECIMAL_ROUND_TRUNCATE	'T'     // Towards zero
#define	STRINGDECIMAL_ROUND_UP		'U'     // Away from zero
#define	STRINGDECIMAL_ROUND_FLOOR	'F'     // Towards -ve
#define	STRINGDECIMAL_ROUND_CEILING	'C'     // Towards +ve
#define	STRINGDECIMAL_ROUND_ROUND	'R'     // Away from zero if 0.5 or more
#define	STRINGDECIMAL_ROUND_BANKING	'B'     // Away from zero if above 0.5, or 0.5 exactly and goes to even

char *stringdecimal_add(const char *a, const char *b);  // Simple add
char *stringdecimal_sub(const char *a, const char *b);  // Simple subtract
char *stringdecimal_mul(const char *a, const char *b);  // Simple multiply
char *stringdecimal_div(const char *a, const char *b, int maxplaces, char round, char **rem);   // Simple divide - to specified number of places, with remainder
char *stringdecimal_rnd(const char *a, int places, char round); // Round to specified number of places
int stringdecimal_cmp(const char *a, const char *b);    // Compare. -1 if a<b, 1 if a>b, 0 if a==b
const char *stringdecimal_check(const char *, int *);   // Check if a is number, return first non number after, else NULL if not valid, sets number of decimal places
char *stringdecimal_eval(const char *sum, int maxdivide, char round, int *maxplacesp);  // Eval sum using brackets, +, -, *, /
// Eval will return max decimal places found in any arg at maxplacesp
// Eval will limit final divide to maxdivide (set to INT_MAX to use the calculated maximum number of places seen in args, or INT_MIN to guess a sensible max places)

// Variations with freeing
char *stringdecimal_add_cf(const char *a, char *b);     // Simple add with free second arg
char *stringdecimal_add_ff(char *a, char *b);   // Simple add with free both args
char *stringdecimal_sub_fc(char *a, const char *b);     // Simple subtract with free first arg
char *stringdecimal_sub_cf(const char *a, char *b);     // Simple subtract with free second arg
char *stringdecimal_sub_ff(char *a, char *b);   // Simple subtract with fere both args
char *stringdecimal_mul_cf(const char *a, char *b);     // Simple multiply with second arg
char *stringdecimal_mul_ff(char *a, char *b);   // Simple multiply with free both args
char *stringdecimal_div_fc(char *a, const char *b, int maxdivide, char round, char **rem);      // Simple divide with free first arg
char *stringdecimal_div_cf(const char *a, char *b, int maxdivide, char round, char **rem);      // Simple divide with free second arg
char *stringdecimal_div_ff(char *a, char *b, int maxdivide, char round, char **rem);    // Simple divide with free both args
char *stringdecimal_rnd_f(char *a, int places, char round);     // Round to specified number of places with free arg
int stringdecimal_cmp_fc(char *a, const char *b);       // Compare with free first arg
int stringdecimal_cmp_cf(const char *a, char *b);       // Compare with free second arg
int stringdecimal_cmp_ff(char *a, char *b);     // Compare with free both args

// Low level functions allow construction of an expression using rational maths
typedef struct sd_s sd_t;
typedef sd_t *sd_p;

sd_p sd_parse(const char *);    // Parse number to an sd_p
sd_p sd_parse_f(char *);        // Parse number to an sd_p (free arg)
void *sd_free(sd_p);            // Free sd_p
sd_p sd_copy(sd_p p);           // Make copy
sd_p sd_int(long long);         // Make from integer
sd_p sd_float(long double);     // Make from float
char *sd_output_rat(sd_p);      // Output as number or (number/number)
char *sd_output(sd_p, int places, char round);  // Output
char *sd_output_f(sd_p, int places, char round);        // Output (free arg)
int sd_places(sd_p);            // Max places of any operand so far
int sd_iszero(sd_p);            // If zero value
int sd_isneg(sd_p);             // If negative value
int sd_ispos(sd_p);             // If positive value
char *sd_num(sd_p);             // Text of numerator (usually for debug)
char *sd_dom(sd_p);             // Text of denominator (usually for debug)

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
int sd_cmp(sd_p, sd_p);         // Compare
int sd_cmp_ff(sd_p, sd_p);      // Compare free all args
int sd_cmp_fc(sd_p, sd_p);      // Compare free first arg
int sd_cmp_cf(sd_p, sd_p);      // Compare free second arg
int sd_abs_cmp(sd_p, sd_p);     // Compare absolute values
int sd_abs_cmp_ff(sd_p, sd_p);  // Compare absolute values free all args
int sd_abs_cmp_fc(sd_p, sd_p);  // Compare absolute values free first arg
int sd_abs_cmp_cf(sd_p, sd_p);  // Compare absolute values free second arg
