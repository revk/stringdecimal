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

#define	STRINGDECIMAL_ROUND_TRUNCATE	'T' // Towards zero
#define	STRINGDECIMAL_ROUND_UP		'U' // Away from zero
#define	STRINGDECIMAL_ROUND_FLOOR	'F' // Towards -ve
#define	STRINGDECIMAL_ROUND_CEILING	'C' // Towards +ve
#define	STRINGDECIMAL_ROUND_ROUND	'R' // Away from zero if 0.5 or more
#define	STRINGDECIMAL_ROUND_BANKING	'B' // Away from zero if above 0.5, or 0.5 exactly and goes to even

char * stringdecimal_add(const char *a,const char *b);	// Simple add
char * stringdecimal_sub(const char *a,const char *b);	// Simple subtract
char * stringdecimal_mul(const char *a,const char *b);	// Simple multiply
char * stringdecimal_div(const char *a,const char *b,int maxplaces,char round,char **rem);	// Simple divide - to specified number of places, with remainder
char * stringdecimal_rnd(const char *a,int places,char round); // Round to specified number of places
int stringdecimal_cmp(const char *a,const char *b);	// Compare. -1 if a<b, 1 if a>b, 0 if a==b
char *stringdecimal_eval(const char *sum,int maxplaces,char round); // Eval sum using brackets, +, -, *, /

// Variations with freeing
char * stringdecimal_add_cf(const char *a,char *b);	// Simple add with free second arg
char * stringdecimal_add_ff(char *a,char *b);	// Simple add with free both args
char * stringdecimal_sub_fc(char *a,const char *b);	// Simple subtract with free first arg
char * stringdecimal_sub_cf(const char *a,char *b);	// Simple subtract with free second arg
char * stringdecimal_sub_ff(char *a,char *b);	// Simple subtract with fere both args
char * stringdecimal_mul_cf(const char *a,char *b);	// Simple multiply with second arg
char * stringdecimal_mul_ff(char *a,char *b);	// Simple multiply with free both args
char * stringdecimal_div_fc(char *a,const char *b,int maxplaces,char round,char **rem);	// Simple divide with free first arg
char * stringdecimal_div_cf(const char *a,char *b,int maxplaces,char round,char **rem);	// Simple divide with free second arg
char * stringdecimal_div_ff(char *a,char *b,int maxplaces,char round,char **rem);	// Simple divide with free both args
char * stringdecimal_rnd_f(char *a,int places,char round); // Round to specified number of places with free arg
int stringdecimal_cmp_fc(char *a,const char *b);	// Compare with free first arg
int stringdecimal_cmp_cf(const char *a,char *b);	// Compare with free second arg
int stringdecimal_cmp_ff(char *a,char *b);	// Compare with free both args
char *stringdecimal_eval_f(char *sum,int maxplaces,char round); // Eval with free sum
