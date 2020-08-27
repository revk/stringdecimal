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
#ifndef	STRINGDECIMAL_H
#define	STRINGDECIMAL_H
typedef enum {
   STRINGDECIMAL_ROUND_TRUNCATE = 'T',  // Towards zero
   STRINGDECIMAL_ROUND_UP = 'U',        // Away from zero
   STRINGDECIMAL_ROUND_FLOOR = 'F',     // Towards -ve
   STRINGDECIMAL_ROUND_CEILING = 'C',   // Towards +ve
   STRINGDECIMAL_ROUND_ROUND = 'R',     // Away from zero if 0.5 or more
   STRINGDECIMAL_ROUND_BANKING = 'B',   // Away from zero if above 0.5, or 0.5 exactly and goes to even
} sd_round_t;

typedef struct {
   sd_round_t round;            // Type of rounding
   int places;                  // Number of places
   int extraplaces;             // Extra places used for divide when places is INT_MIN
   unsigned char nocomma:1;     // Do not allow comma when parsing numbers
} sd_opt_t;

#define	sd_copy_opts(o)	round:o.round,extraplaces:o.extraplaces,nocomma:o.nocomma
#define	sd_opts			\
	sd_field(int,places)	\
	sd_field(sd_round_t,round)	\
	sd_field(int,extraplaces)	\
	sd_flag(nocomma)		\


#define	sd_field(t,v) t v;
#define	sd_flag(v) unsigned char v:1;
typedef struct {                // String / string opts
   const char *a;
   const char *b;
    sd_opts                     //
   unsigned char a_free:1;
   unsigned char b_free:1;
} sd_opts_ss_t;
typedef struct {                // String / string opts
   char *a;
    sd_opts                     //
   unsigned char a_free:1;
} sd_opts_s_t;

// These _cf, _fc, _ff options free one or both args
// The stringdecimal functions are high level

char *stringdecimal_add_opts(sd_opts_ss_t);
#define	stringdecimal_add(...)		stringdecimal_add_opts((sd_opts_ss_t){__VA_ARGS__})
#define	stringdecimal_add_cf(...)	stringdecimal_add_opts((sd_opts_ss_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_add_fc(...)	stringdecimal_add_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_add_ff(...)	stringdecimal_add_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_sub_opts(sd_opts_ss_t);
#define	stringdecimal_sub(...)		stringdecimal_sub_opts((sd_opts_ss_t){__VA_ARGS__})
#define	stringdecimal_sub_cf(...)	stringdecimal_sub_opts((sd_opts_ss_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_sub_fc(...)	stringdecimal_sub_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_sub_ff(...)	stringdecimal_sub_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1,b_free:1})
char *stringdecimal_mul_opts(sd_opts_ss_t);
#define	stringdecimal_mul(...)		stringdecimal_mul_opts((sd_opts_ss_t){__VA_ARGS__})
#define	stringdecimal_mul_cf(...)	stringdecimal_mul_opts((sd_opts_ss_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_mul_fc(...)	stringdecimal_mul_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_mul_ff(...)	stringdecimal_mul_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1,b_free:1})

typedef struct {
   const char *a;
   const char *b;
   char **remp;
    sd_opts                     //
   unsigned char a_free:1;
   unsigned char b_free:1;
} stringdecimal_div_t;
char *stringdecimal_div_opts(stringdecimal_div_t);
#define	stringdecimal_div(...)		stringdecimal_div_opts((sd_opts_ss_t){__VA_ARGS__})
#define	stringdecimal_div_cf(...)	stringdecimal_div_opts((sd_opts_ss_t){__VA_ARGS__,b_free:1})
#define	stringdecimal_div_fc(...)	stringdecimal_div_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1})
#define	stringdecimal_div_ff(...)	stringdecimal_div_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1,b_free:1})

char *stringdecimal_rnd_opts(sd_opts_s_t);
#define	stringdecimal_rnd(...)		stringdecimal_rnd_opts((sd_opts_s_t){__VA_ARGS__})
#define	stringdecimal_rnd_f(...)	stringdecimal_rnd_opts((sd_opts_s_t){__VA_ARGS__,a_free:1})

int stringdecimal_cmp_opts(sd_opts_ss_t);
#define stringdecimal_cmp(...)          stringdecimal_cmp_opts((sd_opts_ss_t){__VA_ARGS__})
#define stringdecimal_cmp_cf(...)       stringdecimal_cmp_opts((sd_opts_ss_t){__VA_ARGS__,b_free:1})
#define stringdecimal_cmp_fc(...)       stringdecimal_cmp_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1})
#define stringdecimal_cmp_ff(...)       stringdecimal_cmp_opts((sd_opts_ss_t){__VA_ARGS__,a_free:1,b_free:1})

typedef struct {
   const char *a;
   int *placesp;
    sd_opts                     //
   unsigned char a_free:1;
} stringdecimal_places_t;
const char *stringdecimal_check_opts(stringdecimal_places_t);
#define	stringdecimal_check(...)	stringdecimal_check_opts((stringdecimal_places_t){__VA_ARGS__})
#define	stringdecimal_check_f(...)	stringdecimal_check_opts((stringdecimal_places_t){__VA_ARGS__,a_free:1})

// Low level functions allow construction of an expression using rational maths
typedef struct sd_s sd_t;
typedef sd_t *sd_p;

typedef struct {
   sd_p p;
   sd_opts unsigned char a_free:1;
   unsigned rat:1;
} sd_output_opts_t;
sd_p sd_parse_opts(stringdecimal_places_t);
#define	sd_parse(...)		sd_parse_opts((stringdecimal_places_t){__VA_ARGS__})
#define	sd_parse_f(...)		sd_parse_opts((stringdecimal_places_t){__VA_ARGS__,a_free:1})
void *sd_free(sd_p);            // Free sd_p
sd_p sd_copy(sd_p p);           // Make copy
sd_p sd_int(long long);         // Make from integer
sd_p sd_float(long double);     // Make from float
char *sd_output_opts(sd_output_opts_t);
#define	sd_output(...)		sd_output_opts((sd_output_opts_t){__VA_ARGS__})
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
#endif
