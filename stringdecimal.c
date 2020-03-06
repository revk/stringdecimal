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

#include "stringdecimal.h"
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <err.h>

//#define DEBUG

// Support functions

typedef struct sd_s sd_t;
struct sd_s
{                               // The structure used internally for digit sequences
   int mag;                     // Magnitude of first digit, e.g. 3 is hundreds, can start negative, e.g. 0.1 would be mag -1
   int sig;                     // Significant figures (i.e. size of d array) - logically unsigned but seriously C fucks up any maths with that
   char *d;                     // Digit array (normally m, or advanced in to m), digits 0-9 not characters '0'-'9'
   char *m;                     // Malloced space
   char neg:1;                  // Sign (set if -1)
};

static sd_t zero = { 0 };

static sd_t one = { 0, 1, (char[])
   {1}
};

static sd_t *
make (sd_t * s, int mag, int sig)
{                               // Initialise with space for digits
   if (!s && !(s = calloc (1, sizeof (*s))))
      errx (1, "malloc");
   // Leave neg as is if set
   s->mag = mag;
   s->sig = sig;
   if (!sig)
      return s;                 // zero
   if (!(s->m = calloc (1, sig)))
      errx (1, "malloc");
   s->d = s->m;
   return s;
}

static sd_t *
copy (sd_t * a)
{                               // Copy
   sd_t *r = make (NULL, a->mag, a->sig);
   r->neg = a->neg;
   if (a->sig)
      memcpy (r->d, a->d, a->sig);
   return r;
}

static sd_t *
clean (sd_t * s)
{                               // Clean up (freeing malloc), returns s so can be used in free if s was malloced as well.
   if (!s)
      return s;
   if (s->m)
      free (s->m);
   memset (s, 0, sizeof (*s));
   return s;
}

static sd_t *
norm (sd_t * s)
{                               // Normalise (striping leading/trailing 0s)
   if (!s && !(s = calloc (1, sizeof (*s))))
      errx (1, "malloc");
   while (s->sig && !s->d[0])
   {                            // Leading 0s
      s->d++;
      s->mag--;
      s->sig--;
   }
   while (s->sig && !s->d[s->sig - 1])
      s->sig--;                 // Trailing 0
   if (!s->sig)
   {                            // zero
      s->mag = 0;
      s->neg = 0;
   }
   return s;
}

static sd_t *
neg (sd_t * s)
{                               // Negate
   if (!s && !(s = calloc (1, sizeof (*s))))
      errx (1, "malloc");
   s->neg ^= 1;
   return s;
}

static void
dump (sd_t * s)
{
   if (!s)
      fprintf (stderr, "NULL\n");
   else
   {
      if (s->neg)
         fprintf (stderr, " Negative");
      if (!s->m)
         fprintf (stderr, " Static");
      if (!s->d)
         fprintf (stderr, " No digits");
      else
      {
         fprintf (stderr, " Digits=");
         for (int n = 0; n < s->sig; n++)
            fprintf (stderr, "%c", '0' + s->d[n]);
      }
      fprintf (stderr, "\n");
   }
}

static const char *
parse (sd_t * s, const char *v)
{                               // Parse in to s, and return next character
   if (!s)
      return NULL;
   memset (s, 0, sizeof (*s));
   if (!v)
      return v;
   const char *e = v;
   if (*v == '+')
      v++;                      // Somewhat redundant
   else if (*v == '-')
   {
      s->neg ^= 1;              // negative
      v++;
   }
   if (!isdigit (*v) && *v != '.')
      return e;                 // Unexpected, we do allow leading dot
   while (*v == '0')
      v++;
   const char *digits = v;
   if (isdigit (*v))
   {                            // Some initial digits
      int d = 0,
         p = 0;
      while (isdigit (*v))
      {
         d++;
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
      make (s, d - 1, d + p);
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
      make (s, mag, sig);
   }
   if (!s->sig)
   {                            // Zero
      s->mag = 0;
      s->neg = 0;
      return v;
   }
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
   return v;
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

static char *
output_clean (sd_t * s)
{                               // Convert to string and clean
   char *r = output (s);
   clean (s);
   return r;
}

static char *
output_cleans (sd_t * s, ...)
{                               // Convert to string and clean null sep list of structures
   char *r = output (s);
   clean (s);
   va_list ap;
   va_start (ap, s);
   while ((s = va_arg (ap, sd_t *)))
      clean (s);
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
uadd (sd_t * r, sd_t * a, sd_t * b, int boffset)
{                               // Unsigned add (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   int mag = a->mag;            // Max mag
   if (boffset + b->mag > a->mag)
      mag = boffset + b->mag;
   mag++;                       // allow for extra digit
   int end = a->mag - a->sig;
   if (boffset + b->mag - b->sig < end)
      end = boffset + b->mag - b->sig;
   r = make (r, mag, mag - end);
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
   return norm (r);
}

static sd_t *
usub (sd_t * r, sd_t * a, sd_t * b, int boffset)
{                               // Unsigned sub (i.e. final sign already set in r) and set in r if needed, and assumes b<=a already
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   int mag = a->mag;            // Max mag
   if (boffset + b->mag > a->mag)
      mag = boffset + b->mag;
   int end = a->mag - a->sig;
   if (boffset + b->mag - b->sig < end)
      end = boffset + b->mag - b->sig;
   r = make (r, mag, mag - end);
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
   return norm (r);
}

static void
makebase (sd_t * r[9], sd_t * a)
{                               // Make array of multiples of a, 1 to 9, used for multiply and divide
   r[0] = copy (a);
   for (int n = 1; n < 9; n++)
      r[n] = uadd (NULL, r[n - 1], a, 0);
}

static void
free_base (sd_t * r[9])
{
   for (int n = 0; n < 9; n++)
      free (clean (r[n]));
}

static sd_t *
umul (sd_t * r, sd_t * a, sd_t * b)
{                               // Unsigned mul (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   sd_t *base[9];
   makebase (base, b);
   sd_t *c = NULL;
   for (int p = 0; p < a->sig; p++)
      if (a->d[p])
      {                         // Add
         sd_t *m = base[a->d[p] - 1];
         m->mag += a->mag - p;
         sd_t *sum = uadd (NULL, c, m, 0);
         if (c)
            free (clean (c));
         c = sum;
         m->mag -= a->mag - p;
      }
   free_base (base);
   if (!r)
      return c;                 // New
   if (!c)
      memset (r, 0, sizeof (*r));
   else
   {
      *r = *c;
      free (c);
   }
   return norm (r);
}

static sd_t *
udiv (sd_t * r, sd_t * a, sd_t * b, int places, char round, sd_t * rem)
{                               // Unsigned div (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   if (!b->sig)
      return NULL;              // Divide by zero
   sd_t *base[9];
   makebase (base, b);
   int mag = a->mag - b->mag;
   if (mag < -places)
      mag = -places;            // Limit to places
   int sig = mag + places + 1;  // Limit to places
   r = make (r, mag, sig);
   sd_t *v = copy (a);
   for (int p = mag; p > mag - sig; p--)
   {
      int n = 0;
#ifdef DEBUG
      char *V = output (v);
      fprintf (stderr, "v=%s p=%d\n", V, p);
      free (V);
#endif
      while (n < 9 && ucmp (v, base[n], p) > 0)
         n++;
      if (n)
      {
         sd_t *s = usub (NULL, v, base[n - 1], p);
         free (clean (v));
         v = s;
      }
      r->d[mag - p] = n;
      if (!v->sig)
         break;
   }
   if (!round)
      round = 'B';
   if (r->neg)
   {                            // reverse logic for +/-
      if (round == 'F')
         round = 'C';
      else if (round == 'C')
         round = 'F';
   }
   if (round != 'T')
   {                            // Rounding
      int shift = mag - sig;
      int diff = ucmp (v, base[4], shift);
#ifdef DEBUG
      char *V = output (v);
      base[4]->mag += shift;
      char *B = output (base[4]);
      base[4]->mag -= shift;
      fprintf (stderr, "v=%s diff=%d mag=%d sig=%d \nb=%s shift=%d\n", V, diff, mag, sig, B, shift);
      free (V);
      free (B);
#endif
      if (((round == 'U' || round == 'C') && v->sig)    // Round up anyway
          || (round == 'R' && diff >= 0)        // Round 0.5 and above up
          || (round == 'B' && diff > 0) // Round up
          || (round == 'B' && !diff && (r->d[r->sig - 1] & 1)))
      {                         // Add one
         sd_t *s;
#if 0
         // Adjust remainder
         s = usub (NULL, v, &one, r->mag - r->sig + 1);
         free (clean (v));
         v = s;
#endif
         // Adjust r
         s = uadd (NULL, r, &one, r->mag - r->sig + 1);
         s->neg = r->neg;
         clean (r);
         *r = *s;
         free (s);
      }
   }
   free_base (base);
   if (rem)
      *rem = *v;
   else
      clean (v);
   free (v);
   return norm (r);
}

static int
scmp (sd_t * a, sd_t * b)
{
#ifdef DEBUG
   char *A = output (a),
      *B = output (b);
   fprintf (stderr, "Do %s>%s\n", A, B);
   free (A);
   free (B);
#endif
   if (a->neg && !b->neg)
      return -1;
   if (!a->neg && b->neg)
      return 1;
   if (a->neg && b->neg)
      return -ucmp (a, b, 0);
   return ucmp (a, b, 0);
}

static sd_t *
sadd (sd_t * r, sd_t * a, sd_t * b)
{                               // Low level add
   if (!r)
      r = malloc (sizeof (*r));
   memset (r, 0, sizeof (*r));
#ifdef DEBUG
   char *A = output (a),
      *B = output (b);
   fprintf (stderr, "Do %s+%s\n", A, B);
   free (A);
   free (B);
#endif
   if (a->neg && !b->neg)
      return usub (r, b, a, 0);
   if (!a->neg && b->neg)
      return usub (r, a, b, 0);
   if (a->neg && b->neg)
      r->neg ^= 1;
   return uadd (r, a, b, 0);
}

static sd_t *
ssub (sd_t * r, sd_t * a, sd_t * b)
{
   if (!r)
      r = malloc (sizeof (*r));
   memset (r, 0, sizeof (*r));
#ifdef DEBUG
   char *A = output (a),
      *B = output (b);
   fprintf (stderr, "Do %s-%s\n", A, B);
   free (A);
   free (B);
#endif
   if (a->neg && !b->neg)
   {
      r->neg ^= 1;
      return uadd (r, a, b, 0);
   }
   if (!a->neg && b->neg)
      return uadd (r, a, b, 0);
   if (a->neg && b->neg)
      r->neg ^= 1;              // Invert output
   int d = ucmp (a, b, 0);
   if (!d)
      return r;                 // Zero
   if (d < 0)
   {                            // a<b so answer will be negative b-a
      r->neg ^= 1;
      return usub (r, b, a, 0);
   }
   return usub (r, a, b, 0);
}

static sd_t *
smul (sd_t * r, sd_t * a, sd_t * b)
{
   if (!r)
      r = malloc (sizeof (*r));
   memset (r, 0, sizeof (*r));
#ifdef DEBUG
   char *A = output (a),
      *B = output (b);
   fprintf (stderr, "Do %s*%s\n", A, B);
   free (A);
   free (B);
#endif
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      r->neg ^= 1;
   return umul (r, a, b);
}

static sd_t *
sdiv (sd_t * r, sd_t * a, sd_t * b, int places, char round, sd_t * rem)
{
   if (!r)
      r = malloc (sizeof (*r));
   memset (r, 0, sizeof (*r));
#ifdef DEBUG
   char *A = output (a),
      *B = output (b);
   fprintf (stderr, "Do %s/%s (%d%c)\n", A, B, places, round ? : 'B');
   free (A);
   free (B);
#endif
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      r->neg ^= 1;
   return udiv (r, a, b, places, round, rem);
}

static sd_t *
srnd (sd_t * r, sd_t * a, int places, char round, sd_t * rem)
{
   if (!r)
      r = calloc (1, sizeof (*r));
#ifdef DEBUG
   char *A = output (a);
   fprintf (stderr, "Do %s (%d%c)\n", A, places, round ? : 'B');
   free (A);
#endif
   sd_t *v = copy (a);
   *r = *v;
   free (v);
   if (!round)
      round = 'B';
   if (r->neg)
   {                            // reverse logic for +/-
      if (round == 'F')
         round = 'C';
      else if (round == 'C')
         round = 'F';
   }
   if (r->sig - r->mag - 1 == places)
      return r;                 // Already that many places
   if (r->sig - r->mag - 1 > places)
   {
      int sig = r->mag + 1 + places;
      int p = sig;
      char up = 0;
      if (round == 'C' || round == 'U')
      {
         while (p < r->sig && !r->d[p])
            p++;
         if (p < r->sig)
            up = 1;
      } else if (round == 'R' && r->d[p] >= 5)
         up = 1;
      else if (round == 'B' && r->d[p] > 5)
         up = 1;
      else if (round == 'B' && r->d[p] == 5)
      {                         // Bankers
         p++;
         while (p < r->sig && !r->d[p])
            p++;
         if (p < r->sig)
            up = 1;
         else if (r->d[sig - 1] && 1)
            up = 1;
      }
      r->sig = sig;
      if (up)
      {                         // Round
         sd_t *s = uadd (NULL, r, &one, r->mag - r->sig + 1);
         s->neg = r->neg;
         clean (r);
         *r = *s;
         free (s);
      }
   }
   if (r->sig - r->mag - 1 < places)
   {                            // Artificially extend places, non normalised
      int sig = r->mag + 1 + places;
      char *m = calloc (1, sig);
      if (!m)
         errx (1, "malloc");
      memcpy (m, r->d, r->sig);
      free (r->m);
      r->m = r->d = m;
      r->sig = sig;
      return r;
   }
   return r;
}

// Maths string functions

char *
stringdecimal_add (const char *a, const char *b)
{                               // Simple add
   sd_t A = { 0 }, B = { 0 }, R = { 0 };
   parse (&A, a);
   parse (&B, b);
   sadd (&R, &A, &B);
   return output_cleans (&R, &A, &B, NULL);
};

char *
stringdecimal_sub (const char *a, const char *b)
{                               // Simple subtract
   sd_t A = { 0 }, B = { 0 }, R = { 0 };
   parse (&A, a);
   parse (&B, b);
   ssub (&R, &A, &B);
   return output_cleans (&R, &A, &B, NULL);
};

char *
stringdecimal_mul (const char *a, const char *b)
{                               // Simple multiply
   sd_t A = { 0 }, B = { 0 }, R = { 0 };
   parse (&A, a);
   parse (&B, b);
   smul (&R, &A, &B);
   return output_cleans (&R, &A, &B, NULL);
};

char *
stringdecimal_div (const char *a, const char *b, int places, char round, char **rem)
{                               // Simple divide - to specified number of places, with remainder
   sd_t A = { 0 }, B = { 0 }, R = { 0 }, REM = { 0 };
   parse (&A, a);
   parse (&B, b);
   sdiv (&R, &A, &B, places, round, &REM);
   if (rem)
      *rem = output (&REM);
   return output_cleans (&R, &A, &B, &REM, NULL);
};

char *
stringdecimal_rnd (const char *a, int places, char round, char **rem)
{                               // Round to specified number of places
   sd_t A = { 0 }, R = { 0 }, REM = { 0 };
   parse (&A, a);
   srnd (&R, &A, places, round, &REM);
   if (rem)
      *rem = output (&REM);
   return output_cleans (&R, &A, &REM, NULL);
};

int
stringdecimal_cmp (const char *a, const char *b)
{                               // Compare
   sd_t A = { 0 }, B = { 0 };
   parse (&A, a);
   parse (&B, b);
   int r = scmp (&A, &B);
   clean (&A);
   clean (&B);
   return r;
}

char *
stringdecimal_eval (const char *sum, int places, char round)
{                               // Parse a sum and process it using basic maths
   if (!sum)
      return NULL;
   int level = 0;               // Brackets
   int operators = 0,
      operatormax = 0;
   int operands = 0,
      operandmax = 0;
   struct operator_s
   {
      int level;
      char operator;
   } *operator = NULL;
   sd_t **operand = NULL;
   void operate (void)
   {                            // Do top operation
      if (!operators--)
         errx (1, "Bad operate");
      sd_t *r = NULL;           // result
      switch (operator[operators].operator)
      {
      case '+':
         if (operands < 2)
            errx (1, "Bad operands +");
         operands -= 2;
         r = sadd (NULL, operand[operands + 0], operand[operands + 1]);
         free (clean (operand[operands + 0]));
         free (clean (operand[operands + 1]));
         break;
      case '-':
         if (operands < 2)
            errx (1, "Bad operands -");
         operands -= 2;
         r = ssub (NULL, operand[operands + 0], operand[operands + 1]);
         free (clean (operand[operands + 0]));
         free (clean (operand[operands + 1]));
         break;
      case '*':
         if (operands < 2)
            errx (1, "Bad operands *");
         operands -= 2;
         r = smul (NULL, operand[operands + 0], operand[operands + 1]);
         free (clean (operand[operands + 0]));
         free (clean (operand[operands + 1]));
         break;
      case '/':
         if (operands < 2)
            errx (1, "Bad operands /");
         operands -= 2;
         r = sdiv (NULL, operand[operands + 0], operand[operands + 1], places, round, NULL);
         free (clean (operand[operands + 0]));
         free (clean (operand[operands + 1]));
         break;
      default:
         errx (1, "Bad operator %c", operator[operators].operator);
      }
      if (r)
      {
         if (operands + 1 > operandmax)
            operand = realloc (operand, (operandmax += 10) * sizeof (*operand));
         operand[operands++] = r;
      }
   }
   while (*sum)
   {
      while (*sum)
      {
         // Operand
         while (isspace (*sum))
            sum++;
         if (*sum == '(')
         {
            level += 10;
            sum++;
            continue;
         }
         const char *was = sum;
         sd_t *v = calloc (1, sizeof (*v));
         if (!v)
            errx (1, "malloc");
         sum = parse (v, sum);
         if (sum == was)
            return "* Missing operand";
         if (operands + 1 > operandmax)
            operand = realloc (operand, (operandmax += 10) * sizeof (*operand));
         operand[operands++] = v;
         break;
      }
      if (!*sum)
         break;                 // clean exit
      while (*sum)
      {
         // Operator
         while (isspace (*sum))
            sum++;
         if (*sum == ')')
         {
            if (!level)
               return "* Too many close brackets";
            level -= 10;
            sum++;
            continue;
         }
         void addop (char op, int level)
         {
            while (operators && operator[operators - 1].level >= level)
               operate ();
            if (operators + 1 > operatormax)
               operator = realloc (operator, (operatormax += 10) * sizeof (*operator));
            operator[operators].operator = op;
            operator[operators].level = level;
            operators++;
         }
         if (*sum == '-' || *sum == '+')
         {
            addop (*sum++, level + 0);
            break;
         }
         if (*sum == '*' || *sum == '/')
         {
            addop (*sum++, level + 1);
            break;
         }
         return "* Missing operator";
      }
      if (!*sum)
         return "* Missing operand";
   }
   if (level)
      return "* Unclosed brackets";
   while (operators)
      operate ();
   if (operands != 1)
      errx (1, "Bad eval - operands %d", operands);
   char *r = output (operand[0]);
   free (clean (operand[0]));
   free (operand);
   free (operator);
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
stringdecimal_rnd_f (char *a, int places, char round, char **rem)
{                               // Round to specified number of places with free arg
   char *r = stringdecimal_rnd (a, places, round, rem);
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
   int places = 2;
   char round = 0;
   char rnd = 0;
   for (int a = 1; a < argc; a++)
   {
      const char *s = argv[a];
      if (*s == '-' && s[1] == 'r')
      {
         if (s[2])
            places = atoi (s + 2);
         rnd = 1;
         continue;
      }
      if (*s == '-' && s[1] == 'p')
      {
         rnd = 0;
         if (s[2])
            places = atoi (s + 2);
         continue;
      }
      if (*s == '-' && s[1] && strchr ("CFUTRB", toupper (s[1])))
      {
         round = toupper (s[1]);
         continue;
      }
#if 0
      sd_t x = { 0 };
      fprintf (stderr, "Input [%s]\n", s);
      const char *p = parse (&x, s);
      if (!p)
         fprintf (stderr, "Parse fail\n");
      else if (*p)
         fprintf (stderr, "Extra [%s]\n", p);
      dump (&x);
      fprintf (stderr, "Output [%s]\n", output_clean (&x));
#endif
      char *res = stringdecimal_eval (s, places, round);
      if (rnd)
         res = stringdecimal_rnd_f (res, places, round, NULL);
      fprintf (stderr, "[%s] = [%s]\n", s, res);
      if (res)
         free (res);
   }
   return 0;
}
#endif
