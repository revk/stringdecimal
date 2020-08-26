// Functions to perform simple maths on decimal value strings
// (c) Copyright 2019 Andrews & Arnold Adrian Kennard
// This is stringdecimal with the stringdecimal_eval() function provided using xparse

#include "stringdecimal.h"
char *stringdecimal_eval(const char *sum, int *maxplacesp, const sd_opt_t *);   // Eval sum using brackets, +, -, *, /
char *stringdecimal_eval_f(char *sum, int *maxplacesp, const sd_opt_t *);       // and free sum
// Eval will return max decimal places found in any arg at maxplacesp
// Eval will limit final divide to maxdivide (set to INT_MAX to use the calculated maximum number of places seen in args, or INT_MIN to guess a sensible number of places)

// Using stringdecimal to build a higher layer parser
#ifdef	XPARSE_H
typedef struct stringdecimal_context_s stringdecimal_context_t;
struct stringdecimal_context_s {
   int maxdivide;
   int extraplaces;
   sd_round_t round;
   int *maxplacesp;
   const char *fail;
   const char *posn;
   unsigned char nocomma:1;
};
extern xparse_config_t stringdecimal_xparse;
#endif
