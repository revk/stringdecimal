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

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include "xparse.h"
#include "stringdecimal.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>
#include <limits.h>

// #define DEBUG

// Support functions

typedef struct sd_s sd_t;
struct sd_s
{                               // The structure used internally for digit sequences
   int mag;                     // Magnitude of first digit, e.g. 3 is hundreds, can start negative, e.g. 0.1 would be mag -1
   int sig;                     // Significant figures (i.e. size of d array) - logically unsigned but seriously C fucks up any maths with that
   char *d;                     // Digit array (normally m, or advanced in to m), digits 0-9 not characters '0'-'9'
   char neg:1;                  // Sign (set if -1)
   char m[];                    // Malloced space
};

static sd_t zero = { 0 };

static sd_t one = { 0, 1, (char[])
   {1}
};

// Safe free and NULL value
#define freez(x)	do{if(x)free(x);x=NULL;}while(0)

static sd_t *
make (int mag, int sig)
{                               // Initialise with space for digits
   sd_t *s = calloc (1, sizeof (*s) + sig);
   assert (s);
   // Leave neg as is if set
   if (!sig)
      mag = 0;
   s->mag = mag;
   s->sig = sig;
   s->d = s->m;
   return s;
}

static sd_t *
copy (sd_t * a)
{                               // Copy
   sd_t *r = make (a->mag, a->sig);
   r->neg = a->neg;
   if (a->sig)
      memcpy (r->d, a->d, a->sig);
   return r;
}

static sd_t *
norm (sd_t * s, char neg)
{                               // Normalise (striping leading/trailing 0s)
   if (!s)
      return s;
   while (s->sig && !s->d[0])
   {                            // Leading 0s
      s->d++;
      s->mag--;
      s->sig--;
   }
   while (s->sig && !s->d[s->sig - 1])
      s->sig--;                 // Trailing 0
   if (neg)
      s->neg ^= 1;
   if (!s->sig)
   {                            // zero
      s->mag = 0;
      s->neg = 0;
   }
   return s;
}

static sd_t *
parse2 (const char *v, const char **ep, int *placesp)
{                               // Parse in to s, and return next character
   if (!v)
      return NULL;
   if (ep)
      *ep = v;
   if (placesp)
      *placesp = 0;
   int places = 0;              // count places
   char neg = 0;
   if (*v == '+')
      v++;                      // Somewhat redundant
   else if (*v == '-')
   {
      neg ^= 1;                 // negative
      v++;
   }
   if (!isdigit (*v) && *v != '.')
      return NULL;              // Unexpected, we do allow leading dot
   while (*v == '0')
      v++;
   sd_t *s = NULL;
   const char *digits = v;
   if (isdigit (*v))
   {                            // Some initial digits
      int d = 0,
         p = 0,
         t = 0;
      while (1)
      {
         if (isdigit (*v))
         {
            if (*v == '0')
               t++;             // count trailing zeros
            else
               t = 0;
            d++;
         } else if (*v != ',')
            break;
         v++;
      }
      if (*v == '.')
      {
         v++;
         while (isdigit (*v))
         {
            places++;
            if (*v == '0')
               t++;             // count trailing zeros
            else
               t = 0;
            p++;
            v++;
         }
      }
      s = make (d - 1, d + p - t);
   } else if (*v == '.')
   {                            // No initial digits
      v++;
      int mag = -1,
         sig = 0,
         t = 0;
      while (*v == '0')
      {
         places++;
         mag--;
         v++;
      }
      digits = v;
      while (isdigit (*v))
      {
         places++;
         if (*v == '0')
            t++;                // count trailing zeros
         else
            t = 0;
         sig++;
         v++;
      }
      s = make (mag, sig - t);
   } else
      s = copy (&zero);
   // Load digits
   int q = 0;
   while (*digits && q < s->sig)
   {
      if (isdigit (*digits))
         s->d[q++] = *digits - '0';
      digits++;
   }
   if (*v == 'e' || *v == 'E')
   {                            // Exponent
      v++;
      int sign = 1,
         e = 0;
      if (*v == '+')
         v++;
      else if (*v == '-')
      {
         v++;
         sign = -1;
      }
      while (isdigit (*v))
         e = e * 10 + *v++ - '0';
      s->mag += e * sign;
   }
   if (ep)
      *ep = v;                  // End of parsing
   if (!s->sig)
      s->mag = 0;               // Zero
   else
      s->neg = neg;
   if (placesp)
      *placesp = places;
   return s;
}

static sd_t *
parse (const char *v)
{
   return parse2 (v, NULL, NULL);
}

const char *
stringdecimal_check (const char *v, int *placesp)
{
   const char *e = NULL;
   sd_t *s = parse2 (v, &e, placesp);
   if (!s)
      return NULL;
   freez (s);
   return e;
}

static char *
output (sd_t * s)
{                               // Convert to a string (malloced)
   if (!s)
      return NULL;
   int len = 0;
   if (s->neg)
      len++;
   if (s->mag < 0)
      len = s->sig - s->mag + 1;
   else if (s->sig > s->mag + 1)
      len = s->sig + s->mag + 1;
   else
      len = s->mag + 1;
   char *d = malloc (len + 1);
   if (!d)
      errx (1, "malloc");
   char *p = d;
   if (s->neg)
      *p++ = '-';
   int q = 0;
   if (s->mag < 0)
   {
      *p++ = '0';
      if (s->sig || s->mag < -1)
      {
         *p++ = '.';
         for (q = 0; q < -1 - s->mag; q++)
            *p++ = '0';
         for (q = 0; q < s->sig; q++)
            *p++ = '0' + s->d[q];
      }
   } else
   {
      while (q <= s->mag && q < s->sig)
         *p++ = '0' + s->d[q++];
      if (s->sig > s->mag + 1)
      {
         *p++ = '.';
         while (q < s->sig)
            *p++ = '0' + s->d[q++];
      } else
         while (q <= s->mag)
         {
            *p++ = '0';
            q++;
         }
   }
   *p = 0;
   return d;
}

#ifdef DEBUG
void
debugout (const char *msg, ...)
{
   fprintf (stderr, "%s:", msg);
   va_list ap;
   va_start (ap, msg);
   sd_t *s;
   while ((s = va_arg (ap, sd_t *)))
   {
      char *v = output (s);
      fprintf (stderr, " %s", v);
      freez (v);
   }
   va_end (ap);
   fprintf (stderr, "\n");
}
#else
#define debugout(msg,...)
#endif

static char *
output_free (sd_t * s, int n, ...)
{                               // Convert to string and clean number of extra sd_t
   char *r = output (s);
   freez (s);
   va_list ap;
   va_start (ap, n);
   while (n--)
   {
      s = va_arg (ap, sd_t *);
      freez (s);
   }
   va_end (ap);
   return r;
}

// Low level maths functions

static int
ucmp (sd_t * a, sd_t * b, int boffset)
{                               // Unsigned compare (magnitude only), if a>b then 1, if a<b then -1, else 0 for equal
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("ucmp", a, b, NULL);
   if (!a->sig && !b->sig)
      return 0;                 // Zeros
   if (a->sig && !b->sig)
      return 1;                 // Zero compare
   if (!a->sig && b->sig)
      return -1;                // Zero compare
   if (a->mag > boffset + b->mag)
      return 1;                 // Simple magnitude difference
   if (a->mag < boffset + b->mag)
      return -1;                // Simple magnitude difference
   int sig = a->sig;            // Max digits to compare
   if (b->sig < sig)
      sig = b->sig;
   int p;
   for (p = 0; p < sig && a->d[p] == b->d[p]; p++);
   if (p < sig)
   {                            // Compare digit
      if (a->d[p] < b->d[p])
         return -1;
      if (a->d[p] > b->d[p])
         return 1;
   }
   // Compare length
   if (a->sig > p)
      return 1;                 // More digits
   if (b->sig > p)
      return -1;                // More digits
   return 0;
}

static sd_t *
uadd (sd_t * a, sd_t * b, char neg, int boffset)
{                               // Unsigned add (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("uadd", a, b, NULL);
   int mag = a->mag;            // Max mag
   if (boffset + b->mag > a->mag)
      mag = boffset + b->mag;
   mag++;                       // allow for extra digit
   int end = a->mag - a->sig;
   if (boffset + b->mag - b->sig < end)
      end = boffset + b->mag - b->sig;
   sd_t *r = make (mag, mag - end);
   int c = 0;
   for (int p = end + 1; p <= mag; p++)
   {
      int v = c;
      if (p <= a->mag && p > a->mag - a->sig)
         v += a->d[a->mag - p];
      if (p <= boffset + b->mag && p > boffset + b->mag - b->sig)
         v += b->d[boffset + b->mag - p];
      c = 0;
      if (v >= 10)
      {
         c = 1;
         v -= 10;
      }
      r->d[r->mag - p] = v;
   }
   if (c)
      errx (1, "Carry add error %d", c);
   return norm (r, neg);
}

static sd_t *
usub (sd_t * a, sd_t * b, char neg, int boffset)
{                               // Unsigned sub (i.e. final sign already set in r) and set in r if needed, and assumes b<=a already
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("usub", a, b, NULL);
   int mag = a->mag;            // Max mag
   if (boffset + b->mag > a->mag)
      mag = boffset + b->mag;
   int end = a->mag - a->sig;
   if (boffset + b->mag - b->sig < end)
      end = boffset + b->mag - b->sig;
   sd_t *r = make (mag, mag - end);
   int c = 0;
   for (int p = end + 1; p <= mag; p++)
   {
      int v = c;
      if (p <= a->mag && p > a->mag - a->sig)
         v += a->d[a->mag - p];
      if (p <= boffset + b->mag && p > boffset + b->mag - b->sig)
         v -= b->d[boffset + b->mag - p];
      c = 0;
      if (v < 0)
      {
         c = -1;
         v += 10;
      }
      r->d[r->mag - p] = v;
   }
   if (c)
      errx (1, "Carry sub error %d", c);
   return norm (r, neg);
}

static void
makebase (sd_t * r[9], sd_t * a)
{                               // Make array of multiples of a, 1 to 9, used for multiply and divide
   r[0] = copy (a);
   for (int n = 1; n < 9; n++)
      r[n] = uadd (r[n - 1], a, 0, 0);
}

static void
free_base (sd_t * r[9])
{
   for (int n = 0; n < 9; n++)
      freez (r[n]);
}

static sd_t *
umul (sd_t * a, sd_t * b, char neg)
{                               // Unsigned mul (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("umul", a, b, NULL);
   sd_t *base[9];
   makebase (base, b);
   sd_t *r = NULL;
   for (int p = 0; p < a->sig; p++)
      if (a->d[p])
      {                         // Add
         sd_t *sum = uadd (r, base[a->d[p] - 1], 0, a->mag - p);
         if (sum)
         {
            freez (r);
            r = sum;
         }
      }
   free_base (base);
   return norm (r, neg);
}

static sd_t *
udiv (sd_t * a, sd_t * b, char neg, int maxplaces, char round, sd_t ** rem)
{                               // Unsigned div (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("udiv", a, b, NULL);
   if (!b->sig)
      return NULL;              // Divide by zero
   sd_t *base[9];
   makebase (base, b);
   int mag = a->mag - b->mag;
   if (mag < -maxplaces)
      mag = -maxplaces;         // Limit to places
   int sig = mag + maxplaces + 1;       // Limit to places
   sd_t *r = make (mag, sig);
   sd_t *v = copy (a);
#ifdef DEBUG
   fprintf (stderr, "Divide %d->%d\n", mag, mag - sig + 1);
#endif
   for (int p = mag; p > mag - sig; p--)
   {
      int n = 0;
      while (n < 9 && ucmp (v, base[n], p) > 0)
         n++;
      //debugout ("udiv rem", v, NULL);
      if (n)
      {
         sd_t *s = usub (v, base[n - 1], 0, p);
         if (s)
         {
            freez (v);
            v = s;
         }
      }
      r->d[mag - p] = n;
      if (!v->sig)
         break;
   }
   if (round != STRINGDECIMAL_ROUND_TRUNCATE && v->sig)
   {                            // Rounding
      if (!round)
         round = STRINGDECIMAL_ROUND_BANKING;   // Default
      if (neg)
      {                         // reverse logic for +/-
         if (round == STRINGDECIMAL_ROUND_FLOOR)
            round = STRINGDECIMAL_ROUND_CEILING;
         else if (round == STRINGDECIMAL_ROUND_CEILING)
            round = STRINGDECIMAL_ROUND_FLOOR;
      }
      int shift = mag - sig;
      int diff = ucmp (v, base[4], shift);
      if (((round == STRINGDECIMAL_ROUND_UP || round == STRINGDECIMAL_ROUND_CEILING) && v->sig) // Round up anyway
          || (round == STRINGDECIMAL_ROUND_ROUND && diff >= 0)  // Round 0.5 and above up
          || (round == STRINGDECIMAL_ROUND_BANKING && diff > 0) // Round up
          || (round == STRINGDECIMAL_ROUND_BANKING && !diff && (r->d[r->sig - 1] & 1)))
      {                         // Add one
         if (rem)
         {                      // Adjust remainder, goes negative
            base[0]->mag += shift + 1;
            sd_t *s = usub (base[0], v, 0, 0);
            base[0]->mag -= shift + 1;
            freez (v);
            v = s;
            v->neg ^= 1;
         }
         // Adjust r
         sd_t *s = uadd (r, &one, 0, r->mag - r->sig + 1);
         freez (r);
         r = s;
      }
   }
   free_base (base);
   if (rem)
   {
      if (b->neg)
         v->neg ^= 1;
      if (neg)
         v->neg ^= 1;
      *rem = v;
   } else
      freez (v);
   return norm (r, neg);
}

static int
scmp (sd_t * a, sd_t * b)
{
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout ("scmp", a, b, NULL);
   if (a->neg && !b->neg)
      return -1;
   if (!a->neg && b->neg)
      return 1;
   if (a->neg && b->neg)
      return -ucmp (a, b, 0);
   return ucmp (a, b, 0);
}

static sd_t *
sadd (sd_t * a, sd_t * b)
{                               // Low level add
   debugout ("sadd", a, b, NULL);
   if (a->neg && !b->neg)
   {                            // Reverse subtract
      sd_t *t = a;
      a = b;
      b = t;
   }
   if (!a->neg && b->neg)
   {                            // Subtract
      int d = ucmp (a, b, 0);
      if (d < 0)
         return usub (b, a, 1, 0);
      return usub (a, b, 0, 0);
   }
   if (a->neg && b->neg)
      return uadd (a, b, 1, 0); // Negative reply
   return uadd (a, b, 0, 0);
}

static sd_t *
ssub (sd_t * a, sd_t * b)
{
   debugout ("ssub", a, b, NULL);
   if (a->neg && !b->neg)
      return uadd (a, b, 1, 0);
   if (!a->neg && b->neg)
      return uadd (a, b, 0, 0);
   char neg = 0;
   if (a->neg && b->neg)
      neg ^= 1;                 // Invert output
   int d = ucmp (a, b, 0);
   if (!d)
      return copy (&zero);      // Zero
   if (d < 0)
      return usub (b, a, 1 - neg, 0);
   return usub (a, b, neg, 0);
}

static sd_t *
smul (sd_t * a, sd_t * b)
{
   debugout ("smul", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return umul (a, b, 1);
   return umul (a, b, 0);
}

static sd_t *
sdiv (sd_t * a, sd_t * b, int maxplaces, char round, sd_t ** rem)
{
   debugout ("sdiv", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return udiv (a, b, 1, maxplaces, round, rem);
   return udiv (a, b, 0, maxplaces, round, rem);
}

static sd_t *
srnd (sd_t * a, int places, char round)
{
   if (!a)
      return NULL;
   debugout ("srnd", a, NULL);
   int decimals = a->sig - a->mag - 1;
   if (decimals < 0)
      decimals = 0;
   sd_t *z (void)
   {
      sd_t *r = copy (&zero);
      r->mag = -places;
      if (places > 0)
         r->mag--;
      return r;
   }
   if (!a->sig)
      return z ();
   if (decimals == places)
      return copy (a);          // Already that many places
   if (decimals > places)
   {                            // more places, needs truncating
      int sig = a->sig - (decimals - places);
      sd_t *r;
      if (sig <= 0)
      {
         r = z ();
         if (sig < 0)
            return r;
         sig = 0;               // Allow rounding
         if (r->mag >= 0)
            r->mag--;
      } else
      {
         r = make (a->mag, sig);
         memcpy (r->d, a->d, sig);
      }
      if (round != STRINGDECIMAL_ROUND_TRUNCATE)
      {
         int p = sig;
         char up = 0;
         if (!round)
            round = STRINGDECIMAL_ROUND_BANKING;
         if (a->neg)
         {                      // reverse logic for +/-
            if (round == STRINGDECIMAL_ROUND_FLOOR)
               round = STRINGDECIMAL_ROUND_CEILING;
            else if (round == STRINGDECIMAL_ROUND_CEILING)
               round = STRINGDECIMAL_ROUND_FLOOR;
         }
         if (round == STRINGDECIMAL_ROUND_CEILING || round == STRINGDECIMAL_ROUND_UP)
         {                      // Up (away from zero) if not exact
            while (p < a->sig && !a->d[p])
               p++;
            if (p < a->sig)
               up = 1;          // not exact
         } else if (round == STRINGDECIMAL_ROUND_ROUND && a->d[p] >= 5) // Up if .5 or above
            up = 1;
         else if (round == STRINGDECIMAL_ROUND_BANKING && a->d[p] > 5)
            up = 1;             // Up as more than .5
         else if (round == STRINGDECIMAL_ROUND_BANKING && a->d[p] == 5)
         {                      // Bankers, check exactly .5
            p++;
            while (p < a->sig && !a->d[p])
               p++;
            if (p < a->sig)
               up = 1;          // greater than .5
            else if (a->d[sig - 1] & 1)
               up = 1;          // exactly .5 and odd, so move to even
         }
         if (up)
         {                      // Round up (away from 0)
            sd_t *s = uadd (r, &one, 0, r->mag - r->sig + 1);
            freez (r);
            r = s;
         }
      }
      r->neg = a->neg;
      return r;
   }
   if (decimals < places)
   {                            // Artificially extend places, non normalised
      int sig = a->sig + (places - decimals);
      if (a->mag > 0)
         sig = a->mag + 1 + places;
      sd_t *r = make (a->mag, sig);
      memcpy (r->d, a->d, a->sig);
      r->neg = a->neg;
      return r;
   }
   return NULL;                 // Uh
}

// Maths string functions

char *
stringdecimal_add (const char *a, const char *b)
{                               // Simple add
   sd_t *A = parse (a);
   sd_t *B = parse (b);
   sd_t *R = sadd (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_sub (const char *a, const char *b)
{                               // Simple subtract
   sd_t *A = parse (a);
   sd_t *B = parse (b);
   sd_t *R = ssub (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_mul (const char *a, const char *b)
{                               // Simple multiply
   sd_t *A = parse (a);
   sd_t *B = parse (b);
   sd_t *R = smul (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_div (const char *a, const char *b, int maxdivide, char round, char **rem)
{                               // Simple divide - to specified number of places, with remainder
   sd_t *B = parse (b);
   if (B && !B->sig)
   {
      freez (B);
      return NULL;              // Divide by zero
   }
   sd_t *A = parse (a);
   sd_t *REM = NULL;
   sd_t *R = sdiv (A, B, maxdivide, round, &REM);
   if (rem)
      *rem = output (REM);
   return output_free (R, 3, A, B, REM);
};

char *
stringdecimal_rnd (const char *a, int places, char round)
{                               // Round to specified number of places
   sd_t *A = parse (a);
   sd_t *R = srnd (A, places, round);
   return output_free (R, 1, A);
};

int
stringdecimal_cmp (const char *a, const char *b)
{                               // Compare
   sd_t *A = parse (a);
   sd_t *B = parse (b);
   int r = scmp (A, B);
   freez (A);
   freez (B);
   return r;
}

// Parsing
#include "xparse.c"
typedef struct parse_value_s parse_value_t;
struct parse_value_s
{
   sd_t *d;                     // Denominator
   sd_t *n;                     // Numerator
   int places;
};

// Parse Support functions
static void *
parse_operand (void *context, const char *p, const char **end)
{                               // Parse an operand, malloc value (or null if error), set end
   parse_value_t *v = malloc (sizeof (*v));
   assert (v);
   v->n = parse2 (p, end, &v->places);
   v->d = NULL;
   return v;
}

static void *
parse_final (void *context, void *v)
{                               // Final processing
   parse_context_t *C = context;
   parse_value_t *V = v;
   if (!V)
      return NULL;
   if (C->maxplacesp)
      *(C->maxplacesp) = V->places;
   char *r = NULL;
   if (V->d)
   {
      r = output (sdiv (V->n, V->d, C->maxdivide == INT_MAX ? V->places : C->maxdivide, C->round, NULL));       // Simple divide to get answer
      if (!r)
         C->fail = "Division failure";
   } else
      r = output (V->n);        // Last remaining operand is the answer
   return r;                    // A string
}

static void
parse_dispose (void *context, void *v)
{                               // Disposing of an operand
   parse_value_t *V = v;
   if (!V)
      return;
   freez (V->d);
   freez (V->n);
   freez (V);
}

static void
parse_fail (void *context, const char *failure, const char *posn)
{                               // Reporting an error
   parse_context_t *C = context;
   C->fail = failure;
}

static parse_value_t *
parse_bin (parse_value_t * l, parse_value_t * r)
{                               // Basic details for binary operator
   if (!l || !r)
      return NULL;
   parse_value_t *v = malloc (sizeof (*v));
   assert (v);
   v->places = l->places;
   if (r->places > v->places)
      v->places = r->places;
   v->n = NULL;
   v->d = NULL;
   return v;
}

static parse_value_t *
parse_bin_add (parse_value_t * l, parse_value_t * r, sd_t ** ap, sd_t ** bp)
{
   *ap = *bp = NULL;
   parse_value_t *v = parse_bin (l, r);
   if (v)
   {
      if ((l->d || l->d) && scmp (l->d ? : &one, r->d ? : &one))
      {                         // Multiply out numerators
         *ap = smul (l->n, r->d ? : &one);
         *bp = smul (r->n, l->d ? : &one);
      }
   }
   return v;
}

// Parse Functions
static void *
parse_add (void *context, void *data, void *l, void *r)
{
   sd_t *a,
    *b;
   parse_value_t *L = l,
      *R = r,
      *v = parse_bin_add (l, r, &a, &b);
   if (v)
   {
      v->n = sadd (a ? : L->n, b ? : R->n);
      if (L->d || R->d)
      {
         if (!scmp (L->d ? : &one, R->d ? : &one))
            v->d = copy (L->d ? : &one);
         else
            v->d = smul (L->d ? : &one, R->d ? : &one);
      }
      freez (a);
      freez (b);
   }
   return v;
}

static void *
parse_sub (void *context, void *data, void *l, void *r)
{
   parse_value_t *R = r;
   R->n->neg = 1 - R->n->neg;
   return parse_add (context, data, l, r);
}

static void *
parse_div (void *context, void *data, void *l, void *r)
{
   parse_context_t *C = context;
   parse_value_t *L = l,
      *R = r,
      *v = parse_bin (l, r);
   if (!R->n->sig)
      C->fail = "Divide by zero";
   else if (!L->d && !R->d)
   {                            // Simple - making a new rational
      v->n = copy (L->n);
      v->d = copy (R->n);
   } else
   {                            // Cross divide
      v->n = smul (L->n, R->d ? : &one);
      v->d = smul (L->d ? : &one, R->n);
   }
   return v;
}

static void *
parse_mul (void *context, void *data, void *l, void *r)
{
   parse_value_t *L = l,
      *R = r,
      *v = parse_bin (l, r);
   if (L->d || R->d)
      v->d = smul (L->d ? : &one, R->d ? : &one);
   v->n = smul (L->n, R->n);
   return v;
}

// List of functions
xparse_op_t parse_uniary[] = {
   {NULL},
};

xparse_op_t parse_binary[] = {
 {op: "+", level: 3, func:parse_add},
 {op: "-", level: 3, func:parse_sub},
 {op: "/", level: 4, func:parse_div},
 {op: "*", level: 4, func:parse_mul},
   {NULL},
};

// Parse Config
xparse_config_t parse_config = {
 unary:parse_uniary,
 binary:parse_binary,
 operand:parse_operand,
 final:parse_final,
 dispose:parse_dispose,
 fail:parse_fail,
};

// Parse
char *
stringdecimal_eval2 (const char *sum, int maxdivide, char round, int *maxplacesp)
{
 parse_context_t context = { maxdivide: maxdivide, round: round, maxplacesp:maxplacesp };
   // TODO error logic
   return xparse (&parse_config, &context, sum, NULL);
}


// Old parse, being replaces, work in progress TODO

#define ops	\
	op(ADD,"+",4,2)	\
	op(SUB,"-",4,2)	\
	op(NEG,"-",9,-1)\
	op(MUL,"*",5,2)	\
	op(DIV,"/",5,2)	\
	op(GE,">=",0,2)	\
	op(GT,">",0,2)	\
	op(LE,"<=",0,2)	\
	op(NE,"<>",0,2)	\
	op(LT,"<",0,2)	\
	op2(EQ,"==",0,2)	\
	op(EQ,"=",0,2)	\
	op(NOT,"!",9,-1)\
	op2(AND,"&&",2,2)\
	op(AND,"&",2,2)	\
	op2(OR,"||",1,2)\
	op(OR,"|",1,2)	\
	op2(NOT,"¬",9,-1)\
	op2(NOT,"~",9,-1)\
	op2(AND,"∧",2,2)\
	op2(OR,"∨",1,2)\
	op2(EQ,"≸",0,2)	\
	op2(EQ,"≹",0,2)	\
	op2(NE,"!=",0,2)\
	op2(NE,"≠",0,2)	\
	op2(NE,"≶",0,2)	\
	op2(NE,"≷",0,2)	\
	op2(MUL,"×",2,2)\
	op2(SUB,"−",1,2)	\
	op2(NEG,"−",9,-1)\
	op2(DIV,"÷",2,2)\
	op2(GE,"≥",0,2)	\
	op2(GE,"≮",0,2)	\
	op2(LE,"≤",0,2)	\
	op2(LE,"≯",0,2)	\
	op2(GT,"≰",0,2)	\
	op2(LT,"≱",0,2)	\

enum
{
#define op(label,tag,level,ops) OP_##label,
#define op2(label,tag,level,ops)
   ops
#undef op
#undef op2
};

struct
{
   const char *tag;
   int len;
   int level;
   int operands;
   int operator;
} op[] = {
#define op(label,tag,level,ops) {tag,sizeof(tag)-1,level,ops,OP_##label},
#define op2(label,tag,level,ops) {tag,sizeof(tag)-1,level,ops,OP_##label},
   ops
#undef op
#undef op2
};

char *
stringdecimal_eval (const char *sum, int maxdivide, char round, int *maxplacesp)
{                               // Parse a sum and process it using basic maths
   if (maxplacesp)
      *maxplacesp = 0;
   if (!sum)
      return NULL;
   const char *fail = NULL;
   int level = 0;               // Brackets and operator level
   int operators = 0,
      operatormax = 0;
   int operands = 0,
      operandmax = 0;
   struct operator_s
   {
      int level;
      int args;
      int operator;
   } *operator = NULL;          // Operator stack
   struct operand_s
   {
      sd_t *n;
      sd_t *d;
      int places;
   } *operand = NULL;
   void operate (void)
   {                            // Do top operation on stack
      if (!operators--)
         errx (1, "Bad operate");
      sd_t *r = NULL;           // result
      sd_t *d = NULL;           // denominator for rational
      if (operands < operator[operators].args)
         errx (1, "Expecting %d args, only %d present", operator[operators].args, operands);
      operands -= operator[operators].args;
      // The operators (A and optionally B)
      sd_t *an = operand[operands + 0].n;
      sd_t *ad = operand[operands + 0].d;
      int p = operand[operands + 0].places;     // Max places of args
      sd_t *bn = NULL,
         *bd = NULL;
      if (operator[operators].args > 1)
      {
         bn = operand[operands + 1].n;
         bd = operand[operands + 1].d;
         if (operand[operands + 1].places > p)
            p = operand[operands + 1].places;
         debugout (op[operator[operators].operator].tag, an, ad ? : &one, bn, bd ? : &one, NULL);
      } else
         debugout (op[operator[operators].operator].tag, an, ad ? : &one, NULL);

      switch (operator[operators].operator)
      {
      case OP_AND:
      case OP_OR:
         {
            int A = scmp (an, &zero),
               B = scmp (bn, &zero);
            switch (operator[operators].operator)
            {
            case OP_AND:
               r = copy (A && B ? &one : &zero);
               break;
            case OP_OR:
               r = copy (A || B ? &one : &zero);
               break;
            }
            p = 1;              // logic
         }
         break;
      case OP_NOT:
         r = copy (scmp (an, &zero) ? &zero : &one);
         p = 1;                 // logic
         break;
      case OP_NEG:
         r = copy (an);
         if (r->sig)
            r->neg ^= 1;
         break;
      case OP_SUB:
         if (bn->sig)
            bn->neg ^= 1;       // invert second arg and do add
         // Drop through
      case OP_ADD:
      case OP_EQ:
      case OP_NE:
      case OP_LT:
      case OP_LE:
      case OP_GT:
      case OP_GE:
         {
            sd_t *a = NULL,
               *b = NULL;
            if ((ad || bd) && scmp (ad ? : &one, bd ? : &one))
            {                   // Multiply out
               a = smul (an, bd ? : &one);
               b = smul (bn, ad ? : &one);
            }
            switch (operator[operators].operator)
            {
            case OP_SUB:
            case OP_ADD:
               r = sadd (a ? : an, b ? : bn);
               if (ad || bd)
               {
                  if (!scmp (ad ? : &one, bd ? : &one))
                     d = copy (ad ? : &one);
                  else
                     d = smul (ad ? : &one, bd ? : &one);
               }
               break;
            case OP_EQ:
               r = copy (scmp (a ? : an, b ? : bn) == 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            case OP_NE:
               r = copy (scmp (a ? : an, b ? : bn) != 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            case OP_LT:
               r = copy (scmp (a ? : an, b ? : bn) < 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            case OP_LE:
               r = copy (scmp (a ? : an, b ? : bn) <= 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            case OP_GT:
               r = copy (scmp (a ? : an, b ? : bn) > 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            case OP_GE:
               r = copy (scmp (a ? : an, b ? : bn) >= 0 ? &one : &zero);
               p = 1;           // Logic
               break;
            }
            freez (a);
            freez (b);
         }
         break;
      case OP_MUL:
         {
            if (ad || bd)
               d = smul (ad ? : &one, bd ? : &one);
            r = smul (an, bn);
         }
         break;
      case OP_DIV:
         {
            if (!bn->sig)
               fail = "!!Divide by zero";
            else if (!ad && !bd)
            {                   // Simple - making a new rational
               r = copy (an);
               d = copy (bn);
            } else
            {                   // Cross divide
               r = smul (an, bd ? : &one);
               d = smul (ad ? : &one, bn);
            }
         }
         break;
      default:
         errx (1, "Bad operator %c", operator[operators].operator);
      }
      for (int n = 0; n < operator[operators].args; n++)
      {                         // Free args
         freez (operand[operands + n].n);
         freez (operand[operands + n].d);
      }
      if (r)
      {
         if (d && d->sig == 1 && d->d[0] == 1)
         {                      // Simple powers of 10 - no need for rationals
            if (d->neg)
               r->neg ^= 1;
            r->mag -= d->mag;
            freez (d);
            d = NULL;
         }
         debugout ("Result", r, d, NULL);
         if (operands + 1 > operandmax)
         {
            operandmax += 10;
            operand = realloc (operand, operandmax * sizeof (*operand));
         }
         operand[operands].n = r;
         operand[operands].d = d;
         operand[operands].places = p;
         operands++;
      } else if (!fail)
         fail = "!!Maths error";        // Should not happen
   }

   void addop (char op, int level, int args)
   {                            // Add an operator
      if (args < 0)
         args = 0 - args;       // USed for prexif unary ops, don't run stack
      else
         while (operators && operator[operators - 1].level >= level)
            operate ();         // Clear stack of pending ops
      if (operators + 1 > operatormax)
         operator = realloc (operator, (operatormax += 10) * sizeof (*operator));
      operator[operators].operator = op;
      operator[operators].level = level;
      operator[operators].args = args;
      operators++;
   }

   while (!fail)
   {                            // Main parse loop
      // Prefix operators and open brackets
      if (*sum == '!' && sum[1] == '!')
      {
         fail = "!!Error";
         break;
      }
      while (1)
      {
         if (*sum == '(')
         {
            level += 10;
            sum++;
            continue;
         }
         if (isspace (*sum))
         {
            sum++;
            continue;
         }
         int q = 0;
         for (q = 0; q < sizeof (op) / sizeof (*op); q++)
            if (op[q].operands < 0 && !strncmp (op[q].tag, sum, op[q].len))
            {
               addop (op[q].operator, level + op[q].level, op[q].operands);
               sum += op[q].len;
               break;
            }
         if (q < sizeof (op) / sizeof (*op))
            continue;
         break;
      }
      // Operand
      if (*sum == '!' && sum[1] == '!')
      {
         fail = "!!Error";
         break;
      }
      const char *was = sum;
      int places = 0;
      sd_t *v = parse2 (sum, &sum, &places);
      if (sum == was)
      {
         fail = "!!Missing operand";
         break;
      }
      // Add the operand
      if (operands + 1 > operandmax)
      {
         operandmax += 10;
         operand = realloc (operand, operandmax * sizeof (*operand));
      }
      operand[operands].n = v;
      operand[operands].d = NULL;       // Assumed not ration
      operand[operands].places = places;
      operands++;
      // Postfix operators and close brackets
      while (1)
      {
         if (*sum == ')')
         {
            if (!level)
            {
               fail = "!!Too many close brackets";
               break;
            }
            level -= 10;
         } else if (!isspace (*sum))
            break;
         sum++;
      }
      if (!*sum)
         break;                 // clean exit after last operand
      // Operator
      int q = 0;
      for (q = 0; q < sizeof (op) / sizeof (*op); q++)
         if (op[q].operands > 0 && !strncmp (op[q].tag, sum, op[q].len))
         {
            sum += op[q].len;
            addop (op[q].operator, level + op[q].level, op[q].operands);
            break;
         }
      if (q < sizeof (op) / sizeof (*op))
         continue;
      fail = "!!Missing/unknown operator";
      break;
   }

   while (!fail && operators)
      operate ();               // Final operators
   if (!fail)
   {                            // Done cleanly?
      if (level)
         fail = "!!Unclosed brackets";
      else
      {                         // Clear operators
         if (operands != 1)
            errx (1, "Bad eval - operands %d", operands);       // Should not happen
      }
   }
   char *r = NULL;
   if (!fail && operands)
   {
      if (maxplacesp)
         *maxplacesp = operand[0].places;
      if (operand[0].d)
      {
         r = output (sdiv (operand[0].n, operand[0].d, maxdivide == INT_MAX ? operand[0].places : maxdivide, round, NULL));     // Simple divide to get answer
         if (!r)
            fail = "!!Division failure";
      } else
         r = output (operand[0].n);     // Last remaining operand is the answer
   }
   for (int i = 0; i < operands; i++)
   {
      freez (operand[i].n);
      freez (operand[i].d);
   }
   freez (operand);
   freez (operator);
   if (fail)
   {
      freez (r);
      return strdup (fail);
   }
   return r;
}

// Version with frees

char *
stringdecimal_add_cf (const char *a, char *b)
{                               // Simple add with free second arg
   char *r = stringdecimal_add (a, b);
   freez (b);
   return r;
};

char *
stringdecimal_add_ff (char *a, char *b)
{                               // Simple add with free both args
   char *r = stringdecimal_add (a, b);
   freez (a);
   freez (b);
   return r;
};

char *
stringdecimal_sub_cf (const char *a, char *b)
{                               // Simple subtract with free second arg
   char *r = stringdecimal_sub (a, b);
   freez (b);
   return r;
};

char *
stringdecimal_sub_fc (char *a, const char *b)
{                               // Simple subtract with free first arg
   char *r = stringdecimal_sub (a, b);
   freez (a);
   return r;
};

char *
stringdecimal_sub_ff (char *a, char *b)
{                               // Simple subtract with fere both args
   char *r = stringdecimal_sub (a, b);
   freez (a);
   freez (b);
   return r;
};

char *
stringdecimal_mul_cf (const char *a, char *b)
{                               // Simple multiply with second arg
   char *r = stringdecimal_mul (a, b);
   freez (b);
   return r;
};

char *
stringdecimal_mul_ff (char *a, char *b)
{                               // Simple multiply with free both args
   char *r = stringdecimal_mul (a, b);
   freez (a);
   freez (b);
   return r;
};

char *
stringdecimal_div_cf (const char *a, char *b, int maxdivide, char round, char **rem)
{                               // Simple divide with free second arg
   char *r = stringdecimal_div (a, b, maxdivide, round, rem);
   freez (b);
   return r;
};

char *
stringdecimal_div_fc (char *a, const char *b, int maxdivide, char round, char **rem)
{                               // Simple divide with free first arg
   char *r = stringdecimal_div (a, b, maxdivide, round, rem);
   freez (a);
   return r;
};

char *
stringdecimal_div_ff (char *a, char *b, int maxdivide, char round, char **rem)
{                               // Simple divide with free both args
   char *r = stringdecimal_div (a, b, maxdivide, round, rem);
   freez (a);
   freez (b);
   return r;
};

char *
stringdecimal_rnd_f (char *a, int places, char round)
{                               // Round to specified number of places with free arg
   char *r = stringdecimal_rnd (a, places, round);
   freez (a);
   return r;
};

int
stringdecimal_cmp_fc (char *a, const char *b)
{                               // Compare with free first arg
   int r = stringdecimal_cmp (a, b);
   freez (a);
   return r;
};

int
stringdecimal_cmp_cf (const char *a, char *b)
{                               // Compare with free second arg
   int r = stringdecimal_cmp (a, b);
   freez (b);
   return r;
};

int
stringdecimal_cmp_ff (char *a, char *b)
{                               // Compare with free both args
   int r = stringdecimal_cmp (a, b);
   freez (a);
   freez (b);
   return r;
};

char *
stringdecimal_eval_f (char *sum, int places, char round, int *maxplacesp)
{                               // Eval and free
   char *r = stringdecimal_eval (sum, places, round, maxplacesp);
   freez (sum);
   return r;
}


#ifndef LIB
// Test function main build
int
main (int argc, const char *argv[])
{
   int roundplaces = 2;
   int divplaces = INT_MAX;     // Use number of places seen
   char round = 0;
   char rnd = 0;
   if (argc <= 1)
      errx (1, "-p<places> to round, -d<places> to limit division, -c/-f/-u/-t/-r/-b to set rounding type (default b)");
   for (int a = 1; a < argc; a++)
   {
      const char *s = argv[a];
      if (*s == '-' && isalpha (s[1]))
      {
         if (*s == '-' && s[1] == 'p')
         {
            if (s[2])
            {
               roundplaces = atoi (s + 2);
               rnd = 1;
            } else
               rnd = 0;
            continue;
         }
         if (*s == '-' && s[1] == 'd' && isdigit (s[2]))
         {
            divplaces = atoi (s + 2);
            continue;
         }
         if (*s == '-' && s[1] && strchr ("CFUTRB", toupper (s[1])) && !s[2])
         {
            round = toupper (s[1]);
            rnd = 1;
            continue;
         }
         errx (1, "Unknown arg %s", s);
      }
      char *res = stringdecimal_eval2 (s, (divplaces == INT_MAX && rnd) ? roundplaces : divplaces, round, NULL);
      if (rnd)
         res = stringdecimal_rnd_f (res, roundplaces, round);
      if (res)
         printf ("%s\n", res);
      freez (res);
   }
   return 0;
}
#endif
