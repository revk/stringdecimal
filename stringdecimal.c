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
#include "stringdecimal.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>

//#define DEBUG

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

static sd_t *
make (int mag, int sig)
{                               // Initialise with space for digits
   sd_t *s = calloc (1, sizeof (*s) + sig);
   assert (s);
   // Leave neg as is if set
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
parse (const char *v, const char **ep)
{                               // Parse in to s, and return next character
   if (!v)
      return NULL;
   if (ep)
      *ep = v;
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
         p = 0;
      while (1)
      {
         if (isdigit (*v))
            d++;
         else if (*v != ',')
            break;
         v++;
      }
      if (*v == '.')
      {
         v++;
         const char *places = v;
         while (isdigit (*v))
         {
            p++;
            v++;
         }
         while (p && places[p - 1] == '0')
            p--;                // Training zero
      }
      s = make (d - 1, d + p);
   } else if (*v == '.')
   {                            // No initial digits
      v++;
      int mag = -1,
         sig = 0;
      while (*v == '0')
      {
         mag--;
         v++;
      }
      digits = v;
      while (isdigit (*v))
      {
         sig++;
         v++;
      }
      s = make (mag, sig);
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
   {                            // Zero
      s->mag = 0;
      return s;
   }
   return norm (s, neg);
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
      *p++ = '.';
      for (q = 0; q < -1 - s->mag; q++)
         *p++ = '0';
      for (q = 0; q < s->sig; q++)
         *p++ = '0' + s->d[q];
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
debugout (char *msg, ...)
{
   fprintf (stderr, "%s:", msg);
   va_list ap;
   va_start (ap, msg);
   sd_t *s;
   while ((s = va_arg (ap, sd_t *)))
   {
      char *v = output (s);
      fprintf (stderr, " %s", v);
      free (v);
   }
   va_end (ap);
   fprintf (stderr, "\n");
}
#else
#define debugout(msg,...)
#endif

static char *
output_free (sd_t * s, ...)
{                               // Convert to string and clean null sep list of structures
   char *r = output (s);
   free (s);
   va_list ap;
   va_start (ap, s);
   while ((s = va_arg (ap, sd_t *)))
      free (s);
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
      free (r[n]);
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
            if (r)
               free (r);
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
            free (v);
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
            free (v);
            v = s;
            v->neg ^= 1;
         }
         // Adjust r
         sd_t *s = uadd (r, &one, 0, r->mag - r->sig + 1);
         free (r);
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
      free (v);
   return norm (r, neg);
}

static int
scmp (sd_t * a, sd_t * b)
{
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
   debugout ("srnd", a, NULL);
   if (a->sig - a->mag - 1 == places)
      return copy (a);          // Already that many places
   if (a->sig - a->mag - 1 > places)
   {                            // more places, needs truncating
      int sig = a->mag + 1 + places;    // Next digit after number of places
      sd_t *r = make (a->mag, sig);
      memcpy (r->d, a->d, sig);
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
            sd_t *s = uadd (r, &one, r->mag - r->sig + 1, 0);
            free (r);
            r = s;
         }
      }
      r->neg = a->neg;
      return r;
   }
   if (a->sig - a->mag - 1 < places)
   {                            // Artificially extend places, non normalised
      int sig = a->mag + 1 + places;
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
   sd_t *A = parse (a, NULL);
   sd_t *B = parse (b, NULL);
   sd_t *R = sadd (A, B);
   return output_free (R, A, B, NULL);
};

char *
stringdecimal_sub (const char *a, const char *b)
{                               // Simple subtract
   sd_t *A = parse (a, NULL);
   sd_t *B = parse (b, NULL);
   sd_t *R = ssub (A, B);
   return output_free (R, A, B, NULL);
};

char *
stringdecimal_mul (const char *a, const char *b)
{                               // Simple multiply
   sd_t *A = parse (a, NULL);
   sd_t *B = parse (b, NULL);
   sd_t *R = smul (A, B);
   return output_free (R, A, B, NULL);
};

char *
stringdecimal_div (const char *a, const char *b, int maxplaces, char round, char **rem)
{                               // Simple divide - to specified number of places, with remainder
   sd_t *B = parse (b, NULL);
   if (!B->sig)
   {
      free (B);
      return NULL;              // Divide by zero
   }
   sd_t *A = parse (a, NULL);
   sd_t *REM = NULL;
   sd_t *R = sdiv (A, B, maxplaces, round, &REM);
   if (rem)
      *rem = output (REM);
   return output_free (R, A, B, REM, NULL);
};

char *
stringdecimal_rnd (const char *a, int places, char round)
{                               // Round to specified number of places
   sd_t *A = parse (a, NULL);
   sd_t *R = srnd (A, places, round);
   return output_free (R, A, NULL);
};

int
stringdecimal_cmp (const char *a, const char *b)
{                               // Compare
   sd_t *A = parse (a, NULL);
   sd_t *B = parse (b, NULL);
   int r = scmp (A, B);
   free (A);
   free (B);
   return r;
}

char *
stringdecimal_eval (const char *sum, int maxplaces, char round)
{                               // Parse a sum and process it using basic maths
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
      char operator;
   } *operator = NULL;          // Operator stack
   sd_t **operand = NULL;       // Operand stack
   sd_t **denominator = NULL;   // Operand stack denominator for rational (NULL entry for not a division, so /1)
   void operate (void)
   {                            // Do top operation on stack
      if (!operators--)
         errx (1, "Bad operate");
      sd_t *r = NULL;           // result
      sd_t *d = NULL;           // denominator for rational
      void pop (int n)
      {
         while (n--)
         {
            operands--;
            free (operand[operands]);
            if (denominator[operands])
               free (denominator[operands]);
         }
      }
      switch (operator[operators].operator)
      {
      case '-':
         if (operands)
            operand[operands - 1]->neg ^= 1;    // invert and do add
         // Drop through
      case '+':
         {
            if (operands < 2)
               errx (1, "Bad operands +");
            sd_t *an = operand[operands - 2];
            sd_t *ad = denominator[operands - 2];
            sd_t *bn = operand[operands - 1];
            sd_t *bd = denominator[operands - 1];
            debugout ("Add", an, ad ? : &one, bn, bd ? : &one, NULL);
            if (ad || bd)
            {                   // Rational maths
               if (!scmp (ad ? : &one, bd ? : &one))
               {                // Simple (same denominator)
                  r = sadd (an, bn);
                  d = copy (ad ? : &one);
               } else
               {                // cross multiply
                  sd_t *a = smul (an, bd ? : &one);
                  sd_t *b = smul (bn, ad ? : &one);
                  r = sadd (a, b);
                  free (a);
                  free (b);
                  d = smul (ad ? : &one, bd ? : &one);
               }
            } else
               r = sadd (an, bn);
            pop (2);
         }
         break;
      case '*':
         if (operands < 2)
            errx (1, "Bad operands *");
         {
            sd_t *an = operand[operands - 2];
            sd_t *ad = denominator[operands - 2];
            sd_t *bn = operand[operands - 1];
            sd_t *bd = denominator[operands - 1];
            debugout ("Mul", an, ad ? : &one, bn, bd ? : &one, NULL);
            if (ad || bd)
            {                   // Rational maths
               if (!ad && !bd)
                  r = smul (an, bn);    // Simple
               else
               {                // cross multiply
                  r = smul (an, bn);
                  d = smul (ad ? : &one, bd ? : &one);
               }
            } else
               r = smul (an, bn);
            pop (2);
         }
         break;
      case '/':
         if (operands < 2)
            errx (1, "Bad operands /");
         {
            sd_t *an = operand[operands - 2];
            sd_t *ad = denominator[operands - 2];
            sd_t *bn = operand[operands - 1];
            sd_t *bd = denominator[operands - 1];
            debugout ("Div", an, ad ? : &one, bn, bd ? : &one, NULL);
            if (!bn->sig)
               fail = "!Divide by zero";
            else if (!ad && !bd)
            {                   // Simple - making a new rational
               r = copy (an);
               d = copy (bn);
            } else
            {                   // Cross divide
               r = smul (an, bd ? : &one);
               d = smul (ad ? : &one, bn);
            }
            pop (2);
            if (!r && !fail)
               fail = "!Division error";
         }
         break;
      default:
         errx (1, "Bad operator %c", operator[operators].operator);
      }
      if (r)
      {
         if (d && d->sig == 1 && d->d[0] == 1)
         {                      // Simple powers of 10 - no need for rationals
            if (d->neg)
               r->neg ^= 1;
            r->mag -= d->mag;
            free (d);
            d = NULL;
         }
         debugout ("Result", r, d, NULL);
         if (operands + 1 > operandmax)
         {
            operandmax += 10;
            operand = realloc (operand, operandmax * sizeof (*operand));
            denominator = realloc (denominator, operandmax * sizeof (*denominator));
         }
         operand[operands] = r;
         denominator[operands] = d;
         operands++;
      }
   }
   void addop (char op, int level)
   {                            // Add an operator
      while (operators && operator[operators - 1].level >= level)
         operate ();
      if (operators + 1 > operatormax)
         operator = realloc (operator, (operatormax += 10) * sizeof (*operator));
      operator[operators].operator = op;
      operator[operators].level = level;
      operators++;
   }
   while (!fail)
   {                            // Main parse loop
      // Prefix operators and open brackets
      while (1)
      {
         if (*sum == '(')
            level += 10;
         else if (!isspace (*sum))
            break;
         sum++;
      }
      // Operand
      if (*sum == '!')
      {
         fail = sum;
         break;
      }
      const char *was = sum;
      sd_t *v = parse (sum, &sum);
      if (sum == was)
      {
         fail = "!Missing operand";
         break;
      }
      // Add the operand
      if (operands + 1 > operandmax)
      {
         operandmax += 10;
         operand = realloc (operand, operandmax * sizeof (*operand));
         denominator = realloc (denominator, operandmax * sizeof (*operand));
      }
      operand[operands] = v;
      denominator[operands] = NULL;     // Assumed not ration
      operands++;
      // Postfix operators and close brackets
      while (1)
      {
         if (*sum == ')')
         {
            if (!level)
            {
               fail = "!Too many close brackets";
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
      if (*sum == '-' || *sum == '+')
         addop (*sum++, level + 0);
      else if (*sum == '*' || *sum == '/')
         addop (*sum++, level + 1);
      else
      {
         fail = "!Missing/unknown operator";
         break;
      }
   }
   while (!fail && operators)
      operate ();               // Final operators
   if (!fail)
   {                            // Done cleanly?
      if (level)
         fail = "!Unclosed brackets";
      else
      {                         // Clear operators
         if (operands != 1)
            errx (1, "Bad eval - operands %d", operands);       // Should not happen
      }
   }
   char *r = NULL;
   if (!fail && operands)
   {
      if (denominator[0])
      {
         r = output (sdiv (operand[0], denominator[0], maxplaces, round, NULL));        // Simple divide to get answer
         if (!r)
            fail = "!Division failure";
      } else
         r = output (operand[0]);       // Last remaining operand is the answer
   }
   for (int i = 0; i < operands; i++)
   {
      free (operand[i]);
      if (denominator[i])
         free (denominator[i]);
   }
   free (operand);
   free (operator);
   if (fail)
   {
      if (r)
         free (r);
      return strdup (fail);
   }
   return r;
}

// Version with frees

char *
stringdecimal_add_cf (const char *a, char *b)
{                               // Simple add with free second arg
   char *r = stringdecimal_add (a, b);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_add_ff (char *a, char *b)
{                               // Simple add with free both args
   char *r = stringdecimal_add (a, b);
   if (a)
      free (a);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_sub_cf (const char *a, char *b)
{                               // Simple subtract with free second arg
   char *r = stringdecimal_sub (a, b);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_sub_fc (char *a, const char *b)
{                               // Simple subtract with free first arg
   char *r = stringdecimal_sub (a, b);
   if (a)
      free (a);
   return r;
};

char *
stringdecimal_sub_ff (char *a, char *b)
{                               // Simple subtract with fere both args
   char *r = stringdecimal_sub (a, b);
   if (a)
      free (a);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_mul_cf (const char *a, char *b)
{                               // Simple multiply with second arg
   char *r = stringdecimal_mul (a, b);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_mul_ff (char *a, char *b)
{                               // Simple multiply with free both args
   char *r = stringdecimal_mul (a, b);
   if (a)
      free (a);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_div_cf (const char *a, char *b, int places, char round, char **rem)
{                               // Simple divide with free second arg
   char *r = stringdecimal_div (a, b, places, round, rem);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_div_fc (char *a, const char *b, int places, char round, char **rem)
{                               // Simple divide with free first arg
   char *r = stringdecimal_div (a, b, places, round, rem);
   if (a)
      free (a);
   return r;
};

char *
stringdecimal_div_ff (char *a, char *b, int places, char round, char **rem)
{                               // Simple divide with free both args
   char *r = stringdecimal_div (a, b, places, round, rem);
   if (a)
      free (a);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_rnd_f (char *a, int places, char round)
{                               // Round to specified number of places with free arg
   char *r = stringdecimal_rnd (a, places, round);
   if (a)
      free (a);
   return r;
};

int
stringdecimal_cmp_fc (char *a, const char *b)
{                               // Compare with free first arg
   int r = stringdecimal_cmp (a, b);
   if (a)
      free (a);
   return r;
};

int
stringdecimal_cmp_cf (const char *a, char *b)
{                               // Compare with free second arg
   int r = stringdecimal_cmp (a, b);
   if (b)
      free (b);
   return r;
};

int
stringdecimal_cmp_ff (char *a, char *b)
{                               // Compare with free both args
   int r = stringdecimal_cmp (a, b);
   if (a)
      free (a);
   if (b)
      free (b);
   return r;
};

char *
stringdecimal_eval_f (char *sum, int places, char round)
{                               // Eval and free
   char *r = stringdecimal_eval (sum, places, round);
   if (sum)
      free (sum);
   return r;
}


#ifndef LIB
// Test function main build
int
main (int argc, const char *argv[])
{
   int roundplaces = 2;
   int divplaces = 100;
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
         if (*s == '-' && s[1] == 'q' && a + 2 <= argc)
         {
            char *rem = NULL;
            char *res = stringdecimal_div (argv[a + 1], argv[a + 2], divplaces, round, &rem);
            fprintf (stderr, "%s/%s = %s rem %s\n", argv[a + 1], argv[a + 2], res, rem);
            a += 2;
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
      char *res = stringdecimal_eval (s, divplaces, round);
      if (rnd)
         res = stringdecimal_rnd_f (res, roundplaces, round);
      printf ("%s\n", res);
      if (res)
         free (res);
   }
   return 0;
}
#endif
