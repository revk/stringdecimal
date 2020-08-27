// Functions to perform simple maths on decimal value strings
// (c) Copyright 2019 Andrews & Arnold Adrian Kennard
// This is stringdecimal with the stringdecimal_eval() function provided using xparse

#include "stringdecimal.h"
char *stringdecimal_eval_opts(stringdecimal_places_t);
#define stringdecimal_eval(...)         stringdecimal_eval_opts((stringdecimal_places_t){__VA_ARGS__})
#define stringdecimal_eval_f(...)       stringdecimal_eval_opts((stringdecimal_places_t){__VA_ARGS__,a_free:1})

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
