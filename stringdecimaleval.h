// Functions to perform simple maths on decimal value strings
// (c) Copyright 2019 Andrews & Arnold Adrian Kennard
// This is stringdecimal with the stringdecimal_eval() function provided using xparse

#include "stringdecimal.h"
char *stringdecimal_eval (const char *sum, int maxdivide, char round, int *maxplacesp); // Eval sum using brackets, +, -, *, /
char *stringdecimal_eval_f (char *sum, int places, char round, int *maxplacesp);        // and free sum
// Eval will return max decimal places found in any arg at maxplacesp
// Eval will limit final divide to maxdivide (set to INT_MAX to use the calculated maximum number of places seen in args)

// Using stringdecimal to build a higher layer parser
#ifdef	XPARSE_H
typedef struct stringdecimal_context_s stringdecimal_context_t;
struct stringdecimal_context_s
{
   int maxdivide;
   char round;
   int *maxplacesp;
   const char *fail;
   const char *posn;
};
extern xparse_config_t stringdecimal_xparse;
#endif
