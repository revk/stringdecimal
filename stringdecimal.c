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

typedef struct sd_val_s sd_val_t;
struct sd_val_s
{                               // The structure used internally for digit sequences
   int mag;                     // Magnitude of first digit, e.g. 3 is hundreds, can start negative, e.g. 0.1 would be mag -1
   int sig;                     // Significant figures (i.e. size of d array) - logically unsigned but seriously C fucks up any maths with that
   char *d;                     // Digit array (normally m, or advanced in to m), digits 0-9 not characters '0'-'9'
   char neg:1;                  // Sign (set if -1)
   char m[];                    // Malloced space
};

static sd_val_t zero = { 0 };

static sd_val_t one = { 0, 1, (char[])
   {1}
};

// Safe free and NULL value
#define freez(x)	do{if(x)free(x);x=NULL;}while(0)

static sd_val_t *
make (int mag, int sig)
{                               // Initialise with space for digits
   sd_val_t *s = calloc (1, sizeof (*s) + sig);
   assert (s);
   // Leave neg as is if set
   if (!sig)
      mag = 0;
   s->mag = mag;
   s->sig = sig;
   s->d = s->m;
   return s;
}

static sd_val_t *
copy (sd_val_t * a)
{                               // Copy
   if (!a)
      return a;
   sd_val_t *r = make (a->mag, a->sig);
   r->neg = a->neg;
   if (a->sig)
      memcpy (r->d, a->d, a->sig);
   return r;
}

static sd_val_t *
norm (sd_val_t * s, char neg)
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

static sd_val_t *
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
   sd_val_t *s = NULL;
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

static sd_val_t *
parse (const char *v)
{
   return parse2 (v, NULL, NULL);
}

const char *
stringdecimal_check (const char *v, int *placesp)
{
   const char *e = NULL;
   sd_val_t *s = parse2 (v, &e, placesp);
   if (!s)
      return NULL;
   freez (s);
   return e;
}

static char *
output (sd_val_t * s)
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
   sd_val_t *s;
   while ((s = va_arg (ap, sd_val_t *)))
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
output_free (sd_val_t * s, int n, ...)
{                               // Convert to string and clean number of extra sd_val_t
   char *r = output (s);
   freez (s);
   va_list ap;
   va_start (ap, n);
   while (n--)
   {
      s = va_arg (ap, sd_val_t *);
      freez (s);
   }
   va_end (ap);
   return r;
}

// Low level maths functions

static int
ucmp (sd_val_t * a, sd_val_t * b, int boffset)
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

static sd_val_t *
uadd (sd_val_t * a, sd_val_t * b, char neg, int boffset)
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
   sd_val_t *r = make (mag, mag - end);
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

static sd_val_t *
usub (sd_val_t * a, sd_val_t * b, char neg, int boffset)
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
   sd_val_t *r = make (mag, mag - end);
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
makebase (sd_val_t * r[9], sd_val_t * a)
{                               // Make array of multiples of a, 1 to 9, used for multiply and divide
   r[0] = copy (a);
   for (int n = 1; n < 9; n++)
      r[n] = uadd (r[n - 1], a, 0, 0);
}

static void
free_base (sd_val_t * r[9])
{
   for (int n = 0; n < 9; n++)
      freez (r[n]);
}

static sd_val_t *
umul (sd_val_t * a, sd_val_t * b, char neg)
{                               // Unsigned mul (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("umul", a, b, NULL);
   sd_val_t *base[9];
   makebase (base, b);
   sd_val_t *r = copy(&zero);
   for (int p = 0; p < a->sig; p++)
      if (a->d[p])
      {                         // Add
         sd_val_t *sum = uadd (r, base[a->d[p] - 1], 0, a->mag - p);
         if (sum)
         {
            freez (r);
            r = sum;
         }
      }
   free_base (base);
   return norm (r, neg);
}

static sd_val_t *
udiv (sd_val_t * a, sd_val_t * b, char neg, int maxplaces, char round, sd_val_t ** rem)
{                               // Unsigned div (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("udiv", a, b, NULL);
   if (!b->sig)
      return NULL;              // Divide by zero
   sd_val_t *base[9];
   makebase (base, b);
   int mag = a->mag - b->mag;
   if (mag < -maxplaces)
      mag = -maxplaces;         // Limit to places
   int sig = mag + maxplaces + 1;       // Limit to places
   sd_val_t *r = make (mag, sig);
   sd_val_t *v = copy (a);
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
         sd_val_t *s = usub (v, base[n - 1], 0, p);
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
            sd_val_t *s = usub (base[0], v, 0, 0);
            base[0]->mag -= shift + 1;
            freez (v);
            v = s;
            v->neg ^= 1;
         }
         // Adjust r
         sd_val_t *s = uadd (r, &one, 0, r->mag - r->sig + 1);
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
scmp (sd_val_t * a, sd_val_t * b)
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

static sd_val_t *
sadd (sd_val_t * a, sd_val_t * b)
{                               // Low level add
   debugout ("sadd", a, b, NULL);
   if (a->neg && !b->neg)
   {                            // Reverse subtract
      sd_val_t *t = a;
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

static sd_val_t *
ssub (sd_val_t * a, sd_val_t * b)
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

static sd_val_t *
smul (sd_val_t * a, sd_val_t * b)
{
   debugout ("smul", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return umul (a, b, 1);
   return umul (a, b, 0);
}

static sd_val_t *
sdiv (sd_val_t * a, sd_val_t * b, int maxplaces, char round, sd_val_t ** rem)
{
   debugout ("sdiv", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return udiv (a, b, 1, maxplaces, round, rem);
   return udiv (a, b, 0, maxplaces, round, rem);
}

static sd_val_t *
srnd (sd_val_t * a, int places, char round)
{
   if (!a)
      return NULL;
   debugout ("srnd", a, NULL);
   int decimals = a->sig - a->mag - 1;
   if (decimals < 0)
      decimals = 0;
   sd_val_t *z (void)
   {
      sd_val_t *r = copy (&zero);
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
      sd_val_t *r;
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
            sd_val_t *s = uadd (r, &one, 0, r->mag - r->sig + 1);
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
      sd_val_t *r = make (a->mag, sig);
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
   sd_val_t *A = parse (a);
   sd_val_t *B = parse (b);
   sd_val_t *R = sadd (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_sub (const char *a, const char *b)
{                               // Simple subtract
   sd_val_t *A = parse (a);
   sd_val_t *B = parse (b);
   sd_val_t *R = ssub (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_mul (const char *a, const char *b)
{                               // Simple multiply
   sd_val_t *A = parse (a);
   sd_val_t *B = parse (b);
   sd_val_t *R = smul (A, B);
   return output_free (R, 2, A, B);
};

char *
stringdecimal_div (const char *a, const char *b, int maxdivide, char round, char **rem)
{                               // Simple divide - to specified number of places, with remainder
   sd_val_t *B = parse (b);
   if (B && !B->sig)
   {
      freez (B);
      return NULL;              // Divide by zero
   }
   sd_val_t *A = parse (a);
   sd_val_t *REM = NULL;
   sd_val_t *R = sdiv (A, B, maxdivide, round, &REM);
   if (rem)
      *rem = output (REM);
   return output_free (R, 3, A, B, REM);
};

char *
stringdecimal_rnd (const char *a, int places, char round)
{                               // Round to specified number of places
   sd_val_t *A = parse (a);
   sd_val_t *R = srnd (A, places, round);
   return output_free (R, 1, A);
};

int
stringdecimal_cmp (const char *a, const char *b)
{                               // Compare
   sd_val_t *A = parse (a);
   sd_val_t *B = parse (b);
   int r = scmp (A, B);
   freez (A);
   freez (B);
   return r;
}

// Low level
struct sd_s
{
   sd_val_t *n;                 // Numerator
   sd_val_t *d;                 // Denominator
   int places;
};

static struct sd_s sd_zero = { &zero };

static sd_p
sd_tidy (sd_p v)
{                               // Check answer
   if (v && v->d && v->d->neg)
   {                            // Normalise sign
      v->n->neg ^= 1;
      v->d->neg = 0;
   }
   if (v && v->n && v->d && v->d->sig == 1 && v->d->d[0] == 1)
   {                            // Power of 10 denominator
      v->n->mag -= v->d->mag;
      freez (v->d);
   }
   return v;
}

static sd_p
sd_new (sd_p l, sd_p r)
{                               // Basic details for binary operator
   sd_p v = calloc (1, sizeof (*v));
   assert (v);
   if (l)
      v->places = l->places;
   if (r && r->places > v->places)
      v->places = r->places;
   return v;
}

static sd_p
sd_cross (sd_p l, sd_p r, sd_val_t ** ap, sd_val_t ** bp)
{                               // Cross multiply
   *ap = *bp = NULL;
   sd_p v = sd_new (l, r);
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

sd_p
sd_copy (sd_p p)
{                               // Copy (create if p is NULL)
   sd_p v = calloc (1, sizeof (*v));
   assert (v);
   if (p)
   {
      v->places = p->places;
      if (p->n)
         v->n = copy (p->n);
      if (p->d)
         v->d = copy (p->d);
   }
   return v;
}

sd_p
sd_parse (const char *val)
{
   sd_p v = calloc (1, sizeof (*v));
   assert (v);
   v->n = parse2 (val, NULL, &v->places);
   v->d = NULL;
   return v;
}

sd_p
sd_parse_f (char *val)
{                               // Parse (free arg)
   sd_p r = sd_parse (val);
   freez (val);
   return r;
}

sd_p
sd_int (long long v)
{
   char temp[40];
   snprintf (temp, sizeof (temp), "%lld", v);
   return sd_parse (temp);
}

sd_p
sd_float (long double v)
{
   char temp[40];
   snprintf (temp, sizeof (temp), "%32Le", v);
   return sd_parse (temp);
}

void *
sd_free (sd_p p)
{                               // Free
   if (!p)
      return p;
   freez (p->d);
   freez (p->n);
   freez (p);
   return NULL;
}

int
sd_places (sd_p p)
{                               // Max places seen
   if (!p)
      return INT_MIN;
   return p->places;
}

char *
sd_output (sd_p p, int places, char round)
{                               // Output
   if (!p)
      return NULL;
   char *r = NULL;
   if (p->d)
      r = output_free (sdiv (p->n, p->d, places == INT_MAX ? p->places : places, round, NULL), 0);      // Simple divide to get answer
   else
   {
      sd_val_t *R = srnd (p->n, places == INT_MAX ? p->places : places, round);
      r = output_free (R, 0);
   }
   return r;
}

char *
sd_output_f (sd_p p, int places, char round)
{                               // Output free arg
   char *r = sd_output (p, places, round);
   sd_free (p);
   return r;
}

sd_p
sd_neg_i (sd_p p)
{                               // Negate (in situ)
   if (!p)
      return p;
   if (p->n->sig)
      p->n->neg ^= 1;
   return p;
}

sd_p
sd_neg (sd_p p)
{                               // Negate
   return sd_neg_i (sd_copy (p));
}

sd_p
sd_abs_i (sd_p p)
{                               // Absolute (in situ)
   if (!p)
      return p;
   p->n->neg = 0;
   return p;
}

sd_p
sd_abs (sd_p p)
{                               // Absolute
   return sd_abs_i (sd_copy (p));
}

sd_p
sd_inv_i (sd_p p)
{                               // Reciprocal (in situ)
   if (!p)
      return p;
   sd_val_t *d = p->d;
   if (!d)
      d = copy (&one);
   p->d = p->n;
   p->n = d;
   return p;
}

sd_p
sd_inv (sd_p p)
{                               // Reciprocal
   return sd_inv_i (sd_copy (p));
}

sd_p
sd_10_i (sd_p p, int shift)
{                               // Adjust by power of 10 (in situ)
   if (!p)
      return p;
   p->n->mag += shift;
   return p;
}

sd_p
sd_10 (sd_p p, int shift)
{                               // Adjust by power of 10
   return sd_10_i (sd_copy (p), shift);
}

int
sd_iszero (sd_p p)
{                               // Is zero
   return !p || !p->n->sig;
}

int
sd_isneg (sd_p p)
{                               // Is negative (denominator always positive)
   return p && p->n->neg;
}

int
sd_ispos (sd_p p)
{                               // Is positive (denominator always positive)
   return p && p->n->sig && !p->n->neg;
}

static void
sd_rational (sd_p p)
{                               // Normalise to integers
   if (!p)
      return;
   int shift = 0,
      s;
   if (p->d && (s = p->d->sig - p->d->mag - 1) > shift)
      shift = s;
   if ((s = p->n->sig - p->n->mag - 1) > shift)
      shift = s;
   p->n->mag += shift;
   p->d->mag += shift;
}

char *
sd_num (sd_p p)
{                               // Numerator as string
   sd_rational (p);
   return output (p->n);
}

char *
sd_den (sd_p p)
{                               // Denominator as string
   sd_rational (p);
   return output (p->d ? : &one);
}

sd_p
sd_add (sd_p l, sd_p r)
{                               // Add
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_val_t *a,
    *b;
   sd_p v = sd_cross (l, r, &a, &b);
   if (v)
   {
      v->n = sadd (a ? : l->n, b ? : r->n);
      if (l->d || r->d)
      {
         if (!scmp (l->d ? : &one, r->d ? : &one))
            v->d = copy (l->d ? : &one);
         else
            v->d = smul (l->d ? : &one, r->d ? : &one);
      }
      freez (a);
      freez (b);
   }
   return sd_tidy (v);
};

sd_p
sd_add_ff (sd_p l, sd_p r)
{                               // Add free all args
   sd_p o = sd_add (l, r);
   sd_free (l);
   sd_free (r);
   return o;
};

sd_p
sd_add_fc (sd_p l, sd_p r)
{                               // Add free first arg
   sd_p o = sd_add (l, r);
   sd_free (l);
   return o;
};

sd_p
sd_add_cf (sd_p l, sd_p r)
{                               // Add free second arg
   sd_p o = sd_add (l, r);
   sd_free (r);
   return o;
};

sd_p
sd_sub (sd_p l, sd_p r)
{                               // Subtract
   if (r)
      r->n->neg ^= 1;
   sd_p o = sd_add (l, r);
   if (r)
      r->n->neg ^= 1;
   return o;
};

sd_p
sd_sub_ff (sd_p l, sd_p r)
{                               // Subtract free all args
   sd_p o = sd_sub (l, r);
   sd_free (l);
   sd_free (r);
   return o;
};

sd_p
sd_sub_fc (sd_p l, sd_p r)
{                               // Subtract free first arg
   sd_p o = sd_sub (l, r);
   sd_free (l);
   return o;
};

sd_p
sd_sub_cf (sd_p l, sd_p r)
{                               // Subtract free second arg
   sd_p o = sd_sub (l, r);
   sd_free (r);
   return o;
};

sd_p
sd_mul (sd_p l, sd_p r)
{                               // Multiply
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_p v = sd_new (l, r);
   if (l->d || r->d)
      v->d = smul (l->d ? : &one, r->d ? : &one);
   v->n = smul (l->n, r->n);
   return sd_tidy (v);
};

sd_p
sd_mul_ff (sd_p l, sd_p r)
{                               // Multiply free all args
   sd_p o = sd_mul (l, r);
   sd_free (l);
   sd_free (r);
   return o;
};

sd_p
sd_mul_fc (sd_p l, sd_p r)
{                               // Multiply free first arg
   sd_p o = sd_mul (l, r);
   sd_free (l);
   return o;
};

sd_p
sd_mul_cf (sd_p l, sd_p r)
{                               // Multiply free second arg
   sd_p o = sd_mul (l, r);
   sd_free (r);
   return o;
};

sd_p
sd_div (sd_p l, sd_p r)
{                               // Divide
   if (!l)
      l = &sd_zero;
   if (!r || !r->n || !r->n->sig)
      return NULL;
   sd_p v = sd_new (l, r);
   if (!l->d && !r->d)
   {                            // Simple - making a new rational
      v->n = copy (l->n);
      v->d = copy (r->n);
   } else
   {                            // Cross divide
      v->n = smul (l->n, r->d ? : &one);
      v->d = smul (l->d ? : &one, r->n);
   }
   return sd_tidy (v);
};

sd_p
sd_div_ff (sd_p l, sd_p r)
{                               // Divide free all args
   sd_p o = sd_div (l, r);
   sd_free (l);
   sd_free (r);
   return o;
};

sd_p
sd_div_fc (sd_p l, sd_p r)
{                               // Divide free first arg
   sd_p o = sd_div (l, r);
   sd_free (l);
   return o;
};

sd_p
sd_div_cf (sd_p l, sd_p r)
{                               // Divide free second arg
   sd_p o = sd_div (l, r);
   sd_free (r);
   return o;
};

int
sd_abs_cmp (sd_p l, sd_p r)
{                               // Compare absolute values
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_val_t *a,
    *b;
   sd_p v = sd_cross (l, r, &a, &b);
   int diff = ucmp (a ? : l->n, b ? : r->n, 0);
   sd_free (v);
   freez (a);
   freez (b);
   return diff;
};

int
sd_abs_cmp_ff (sd_p l, sd_p r)
{                               // Compare free all args
   int diff = sd_abs_cmp (l, r);
   sd_free (l);
   sd_free (r);
   return diff;
};

int
sd_abs_cmp_fc (sd_p l, sd_p r)
{                               // Compare free first arg
   int diff = sd_abs_cmp (l, r);
   sd_free (l);
   return diff;
};

int
sd_abs_cmp_cf (sd_p l, sd_p r)
{                               // Compare free second arg
   int diff = sd_abs_cmp (l, r);
   sd_free (r);
   return diff;
};

int
sd_cmp (sd_p l, sd_p r)
{                               // Compare
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_val_t *a,
    *b;
   sd_p v = sd_cross (l, r, &a, &b);
   int diff = scmp (a ? : l->n, b ? : r->n);
   sd_free (v);
   freez (a);
   freez (b);
   return diff;
};

int
sd_cmp_ff (sd_p l, sd_p r)
{                               // Compare free all args
   int diff = sd_cmp (l, r);
   sd_free (l);
   sd_free (r);
   return diff;
};

int
sd_cmp_fc (sd_p l, sd_p r)
{                               // Compare free first arg
   int diff = sd_cmp (l, r);
   sd_free (l);
   return diff;
};

int
sd_cmp_cf (sd_p l, sd_p r)
{                               // Compare free second arg
   int diff = sd_cmp (l, r);
   sd_free (r);
   return diff;
};

// Parsing
#include "xparse.c"
// Parse Support functions
static void *
parse_operand (void *context, const char *p, const char **end)
{                               // Parse an operand, malloc value (or null if error), set end
   sd_p v = calloc (1, sizeof (*v));
   assert (v);
   v->n = parse2 (p, end, &v->places);
   v->d = NULL;
   return v;
}

static void *
parse_final (void *context, void *v)
{                               // Final processing
   stringdecimal_context_t *C = context;
   sd_p V = v;
   if (!V)
      return NULL;
   if (C->maxplacesp)
      *(C->maxplacesp) = V->places;
   char *r = NULL;
   if (V->d)
   {
      r = output_free (sdiv (V->n, V->d, C->maxdivide == INT_MAX ? V->places : C->maxdivide, C->round, NULL), 0);       // Simple divide to get answer
      if (!r)
         C->fail = "Division failure";
   } else
      r = output (V->n);        // Last remaining operand is the answer
   return r;                    // A string
}

static void
parse_dispose (void *context, void *v)
{                               // Disposing of an operand
   sd_free (v);
}

static void
parse_fail (void *context, const char *failure, const char *posn)
{                               // Reporting an error
   stringdecimal_context_t *C = context;
   C->fail = failure;
   C->posn = posn;
}

#define MATCH_LT 1
#define MATCH_GT 2
#define MATCH_EQ 4
#define MATCH_NE 8
static sd_p
parse_bin_cmp (sd_p l, sd_p r, int match)
{
   sd_val_t *a,
    *b;
   sd_p L = l,
      R = r,
      v = sd_cross (l, r, &a, &b);
   int diff = scmp (a ? : L->n, b ? : R->n);
   if (((match & MATCH_LT) && diff < 0) ||
       ((match & MATCH_GT) && diff > 0) || ((match & MATCH_EQ) && diff == 0) || ((match & MATCH_NE) && diff != 0))
      v->n = copy (&one);
   else
      v->n = copy (&zero);
   v->places = 0;
   return v;
}

// Parse Functions
static void *
parse_add (void *context, void *data, void *l, void *r)
{
   return sd_add (l, r);
}

static void *
parse_sub (void *context, void *data, void *l, void *r)
{
   return sd_sub (l, r);
}

static void *
parse_div (void *context, void *data, void *l, void *r)
{
   stringdecimal_context_t *C = context;
   sd_p o = sd_div (l, r);
   if (!o)
      C->fail = "Divide by zero";
   return o;
}

static void *
parse_mul (void *context, void *data, void *l, void *r)
{
   return sd_mul (l, r);
}

static void *
parse_neg (void *context, void *data, void *l, void *r)
{
   sd_p R = r;
   R->n->neg ^= 1;
   return r;
}

static void *
parse_not (void *context, void *data, void *l, void *r)
{
   sd_p R = r,
      v = sd_new (l, r);
   v->n = copy (!R->n->sig ? &one : &zero);
   v->places = 0;
   return v;
}

static void *
parse_eq (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_EQ);
}

static void *
parse_ne (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_NE);
}

static void *
parse_gt (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_GT);
}

static void *
parse_ge (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_EQ | MATCH_GT);
}

static void *
parse_le (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_EQ | MATCH_LT);
}

static void *
parse_lt (void *context, void *data, void *l, void *r)
{
   return parse_bin_cmp (l, r, MATCH_LT);
}

static void *
parse_and (void *context, void *data, void *l, void *r)
{
   sd_p L = l,
      R = r;
   if (!L->n->sig)
      return L;                 // False
   return R;                    // Whatever second argument is
}

static void *
parse_or (void *context, void *data, void *l, void *r)
{
   sd_p L = l,
      R = r;
   if (L->n->sig)
      return L;                 // True
   return R;                    // Whatever second argument is
}

// List of functions
static xparse_op_t parse_uniary[] = {
 {op: "-", level: 9, func:parse_neg},
 {op: "!", op2: "¬", level: 9, func:parse_not},
   {NULL},
};

static xparse_op_t parse_binary[] = {
 {op: "+", level: 5, func:parse_add},
 {op: "-", op2: "−", level: 5, func:parse_sub},
 {op: "/", op2: "÷", level: 6, func:parse_div},
 {op: "*", op2: "×", level: 6, func:parse_mul},
 {op: "==", op2: "=", level: 3, func:parse_eq},
 {op: ">=", op2: "≥", level: 3, func:parse_ge},
 {op: "<=", op2: "≤", level: 3, func:parse_le},
 {op: "!=", op2: "≠", level: 3, func:parse_ne},
 {op: ">", op2: "≰", level: 3, func:parse_gt},
 {op: "<", op2: "≱", level: 3, func:parse_lt},
 {op: "&&", op2: "∧", level: 2, func:parse_and},
 {op: "||", op2: "∨", level: 1, func:parse_or},
   {NULL},
};

// Parse Config (optionally public to allow building layers on top)
xparse_config_t stringdecimal_xparse = {
 unary:parse_uniary,
 binary:parse_binary,
 operand:parse_operand,
 final:parse_final,
 dispose:parse_dispose,
 fail:parse_fail,
};

// Parse
char *
stringdecimal_eval (const char *sum, int maxdivide, char round, int *maxplacesp)
{
 stringdecimal_context_t context = { maxdivide: maxdivide, round: round, maxplacesp:maxplacesp };
   char *ret = xparse (&stringdecimal_xparse, &context, sum, NULL);
   if (!ret)
      assert (asprintf (&ret, "!!%s at %.*s", context.fail, 10, context.posn) >= 0);
   return ret;
}

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
         if (s[1] == 'x')
         {                      // Simple test of sd
            sd_p r = sd_div_ff (sd_parse ("1.00"), sd_parse ("3.1"));
            fprintf (stderr, "places %d num %s den %s res %s\n", sd_places (r), sd_num (r), sd_den (r), sd_output (r, 2, 0));
            sd_free (r);
            continue;
         }
         if (s[1] == 'p')
         {
            if (s[2])
            {
               roundplaces = atoi (s + 2);
               rnd = 1;
            } else
               rnd = 0;
            continue;
         }
         if (s[1] == 'd' && isdigit (s[2]))
         {
            divplaces = atoi (s + 2);
            continue;
         }
         if (s[1] && strchr ("CFUTRB", toupper (s[1])) && !s[2])
         {
            round = toupper (s[1]);
            rnd = 1;
            continue;
         }
         errx (1, "Unknown arg %s", s);
      }
      char *res = stringdecimal_eval (s, (divplaces == INT_MAX && rnd) ? roundplaces : divplaces, round, NULL);
      if (rnd)
         res = stringdecimal_rnd_f (res, roundplaces, round);
      if (res)
         printf ("%s\n", res);
      freez (res);
   }
   return 0;
}
#endif
