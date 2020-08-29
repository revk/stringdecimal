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
#ifdef	EVAL
#include "xparse.h"
#include "stringdecimaleval.h"
#else
#include "stringdecimal.h"
#endif
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>
#include <limits.h>

char sd_comma = ',';
char sd_point = '.';

static const char *sup[10] = { "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹" };

#define	si		\
	u(Y,24)		\
	u(Z,21)		\
	u(E,18)		\
	u(P,15)		\
	u(T,12)		\
	u(G,9)		\
	u(M,6)		\
	u(k,3)		\
	u(h,2)		\
	u(da,1)		\
	u(d,-1)		\
	u(c,-2)		\
	u(mc,-6)	\
	u(m,-3)		\
	u(μ,-6)		\
	u(µ,-6)		\
	u(u,-6)		\
	u(n,-9)		\
	u(p,-12)	\
	u(f,-15)	\
	u(a,-18)	\
	u(z,-21)	\
	u(y,-24)	\
	u(%,-2)		\
	u(‰,-3)		\
	u(‱,-4)		\

#define	ieee				\
	u(Ki,1024L)			\
	u(Mi,1048576L)			\
	u(Gi,1073741824LL)		\
	u(Ti,1099511627776LL)		\
	u(Pi,1125899906842624LL)	\
	u(Ei,1152921504606846976LL)	\

//#define DEBUG

// Support functions

typedef struct sd_val_s sd_val_t;
struct sd_val_s {               // The structure used internally for digit sequences
   int mag;                     // Magnitude of first digit, e.g. 3 is hundreds, can start negative, e.g. 0.1 would be mag -1
   int sig;                     // Significant figures (i.e. size of d array) - logically unsigned but seriously C fucks up any maths with that
   char *d;                     // Digit array (normally m, or advanced in to m), digits 0-9 not characters '0'-'9'
   char neg:1;                  // Sign (set if -1)
   char m[];                    // Malloced space
};

static sd_val_t zero = { 0 };

static sd_val_t one = { 0, 1, (char[])
   { 1 }
};

static sd_val_t two = { 0, 1, (char[])
   { 2 }
};


//static sd_val_t two = { 0, 1, (char[]) { 2 } };

struct sd_s {
   sd_val_t *n;                 // Numerator
   sd_val_t *d;                 // Denominator
   int places;                  // Max places seen
};

static void sd_rational(sd_p p);

// Safe free and NULL value
#define freez(x)	do{if(x)free((void*)(x));x=NULL;}while(0)

static sd_val_t *make(int mag, int sig)
{                               // Initialise with space for digits
   sd_val_t *s = calloc(1, sizeof(*s) + sig);
   assert(s);
   // Leave neg as is if set
   if (!sig)
      mag = 0;
   s->mag = mag;
   s->sig = sig;
   s->d = s->m;
   return s;
}

static sd_val_t *copy(sd_val_t * a)
{                               // Copy
   if (!a)
      return a;
   sd_val_t *r = make(a->mag, a->sig);
   r->neg = a->neg;
   if (a->sig)
      memcpy(r->d, a->d, a->sig);
   return r;
}

static sd_val_t *norm(sd_val_t * s, char neg)
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

typedef struct {
   const char *v;
   const char **end;
   int *placesp;
   unsigned char nocomma:1;
   unsigned char comma:1;
} parse_t;
#define	parse(...)	parse_opts((parse_t){__VA_ARGS__})
static sd_val_t *parse_opts(parse_t o)
{                               // Parse in to s, and return next character
   int comp(const char *a, const char *b) {
      if (!a || !b)
         return 0;
      int l = 0;
      while (a[l] && b[l] && a[l] == b[l])
         l++;
      if (!a[l])
         return l;
      return 0;
   }
   char numeric = 0;
   int getdigit(const char *p, const char **pp) {
      if (numeric >= 0 && isdigit(*p))
      {
         if (pp)
            (*pp) = p + 1;
         numeric = 1;           // Only expecting digits now
         return *p - '0';
      }
      if (numeric > 0)
         return -1;             // Looking for numbers only
      for (int q = 0, l; q < 10; q++)
         if ((l = comp(sup[q], p)))
         {
            if (pp)
               *pp = p + l;
            numeric = -1;
            return q;
         }
      return -1;
   }
   if (!o.v)
      return NULL;
   if (o.end)
      *o.end = o.v;
   if (o.placesp)
      *o.placesp = 0;
   int places = 0;              // count places
   char neg = 0;
   if (*o.v == '+')
      o.v++;                    // Somewhat redundant
   else if (*o.v == '-')
   {
      neg ^= 1;                 // negative
      o.v++;
   }
   if (getdigit(o.v, NULL) < 0 && *o.v != sd_point)
      return NULL;              // Unexpected, we do allow leading dot
   while (!getdigit(o.v, NULL))
      getdigit(o.v, &o.v);
   sd_val_t *s = NULL;
   const char *digits = o.v;
   if (getdigit(o.v, NULL) >= 0)
   {                            // Some initial digits
      int d = 0,
          p = 0,
          t = 0;
      while (*o.v)
      {
         const char *q = o.v;
         if (!o.nocomma && sd_comma && *q++ == sd_comma && getdigit(q, &q) >= 0 && getdigit(q, &q) >= 0 && getdigit(q, &q) >= 0 && getdigit(q, &q) < 0)
         {                      // Skip valid commas in numbers
            o.v++;
            continue;
         }
         int v;
         if ((v = getdigit(o.v, &o.v)) < 0)
            break;
         if (!v)
            t++;                // count trailing zeros
         else
            t = 0;
         d++;
      }
      if (*o.v == sd_point)
      {
         o.v++;
         int v;
         while ((v = getdigit(o.v, &o.v)) >= 0)
         {
            places++;
            if (!v)
               t++;             // count trailing zeros
            else
               t = 0;
            p++;
         }
      }
      s = make(d - 1, d + p - t);
   } else if (*o.v == sd_point)
   {                            // No initial digits
      o.v++;
      if (getdigit(o.v, NULL) < 0)
         return NULL;
      int mag = -1,
          sig = 0,
          t = 0;
      while (!getdigit(o.v, NULL))
      {
         getdigit(o.v, &o.v);
         places++;
         mag--;
      }
      digits = o.v;
      int v;
      while ((v = getdigit(o.v, &o.v)) >= 0)
      {
         places++;
         if (!v)
            t++;                // count trailing zeros
         else
            t = 0;
         sig++;
      }
      s = make(mag, sig - t);
   } else
      s = copy(&zero);
   // Load digits
   int q = 0;
   while (*digits && q < s->sig)
   {
      int v = getdigit(digits, &digits);
      if (v >= 0)
         s->d[q++] = v;
      else
         digits++;              // Advance over non digits, e.g. comma, point
   }
   if ((*o.v == 'e' || *o.v == 'E') && (((o.v[1] == '+' || o.v[1] == '-') && getdigit(o.v + 2, NULL) >= 0) || getdigit(o.v + 1, NULL) >= 0))
   {                            // Exponent (may clash with E SI prefix if not careful)
      o.v++;
      int sign = 1,
          e = 0;
      if (*o.v == '+')
         o.v++;
      else if (*o.v == '-')
      {
         o.v++;
         sign = -1;
      }
      int v;
      while ((v = getdigit(o.v, &o.v)) >= 0)
         e = e * 10 + v;        // Only advances if digit
      s->mag += e * sign;
   }
   if (o.end)
      *o.end = o.v;             // End of parsing
   if (!s->sig)
      s->mag = 0;               // Zero
   else
      s->neg = neg;
   if (o.placesp)
      *o.placesp = places;
   return s;
}

const char *sd_check_opts(sd_parse_t o)
{
   const char *e = NULL;
 sd_val_t *s = parse(o.a, end: &e, nocomma:o.nocomma);
   if (!s)
      return NULL;
   freez(s);
   if (o.end)
      *o.end = e;
   if (o.a_free)
      freez(o.a);
   return e;
}

static char *output(sd_val_t * s, char comma)
{                               // Convert to a string (malloced)
   if (!s)
      return NULL;
   if (!sd_comma)
      comma = 0;                // No commas
   int len = s->sig;            // sig figs
   if (s->mag + 1 > s->sig)
      len = s->mag + 1;         // mag
   if (s->sig > s->mag + 1)
      len++;                    // point
   if (s->mag < 0)
      len -= s->mag;            // zeros
   if (comma && s->mag > 0)
      len += s->mag / 3;        // commas
   if (s->neg)
      len++;                    // sign
   char *d = malloc(len + 1);
   if (!d)
      errx(1, "malloc");
   char *p = d;
   if (s->neg)
      *p++ = '-';
   int q = 0;
   if (s->mag < 0)
   {
      *p++ = '0';
      if (s->sig || s->mag < -1)
      {
         *p++ = sd_point;
         for (q = 0; q < -1 - s->mag; q++)
            *p++ = '0';
         for (q = 0; q < s->sig; q++)
            *p++ = '0' + s->d[q];
      }
   } else
   {
      while (q <= s->mag && q < s->sig)
      {
         *p++ = '0' + s->d[q++];
         if (comma && q < s->mag && !((s->mag - q + 1) % 3))
            *p++ = sd_comma;
      }
      if (s->sig > s->mag + 1)
      {
         *p++ = sd_point;
         while (q < s->sig)
            *p++ = '0' + s->d[q++];
      } else
         while (q <= s->mag)
         {
            *p++ = '0';
            q++;
            if (comma && q < s->mag && !((s->mag - q + 1) % 3))
               *p++ = sd_comma;
         }
   }
   *p = 0;
   assert(p == d + len);
   return d;
}

#ifdef DEBUG
void debugout(const char *msg, ...)
{
   fprintf(stderr, "%s:", msg);
   va_list ap;
   va_start(ap, msg);
   sd_val_t *s;
   while ((s = va_arg(ap, sd_val_t *)))
   {
      char *v = output(s);
      fprintf(stderr, " %s", v);
      freez(v);
   }
   va_end(ap);
   fprintf(stderr, "\n");
}

void sd_debugout(const char *msg, ...)
{
   fprintf(stderr, "%s:", msg);
   va_list ap;
   va_start(ap, msg);
   sd_t *s;
   while ((s = va_arg(ap, sd_t *)))
   {
      char *v = output(s->n);
      fprintf(stderr, " %s", v);
      freez(v);
      if (s->d)
      {
         char *v = output(s->d);
         fprintf(stderr, "/%s", v);
         freez(v);
      }
   }
   va_end(ap);
   fprintf(stderr, "\n");
}
#else
#define debugout(msg,...)
#define sd_debugout(msg,...)
#endif

typedef struct {
   sd_val_t *a;
   sd_val_t *b;
   sd_val_t *c;
   sd_val_t *d;
   unsigned char comma:1;
} output_t;
#define output_f(...)	output_opts((output_t){__VA_ARGS__})
static char *output_opts(output_t o)
{                               // Convert first arg to string, but free multiple args
   char *r = output(o.a, o.comma);
   freez(o.a);
   freez(o.b);
   freez(o.c);
   freez(o.d);
   return r;
}

// Low level maths functions

static int ucmp(sd_val_t * a, sd_val_t * b, int boffset)
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

static sd_val_t *uadd(sd_val_t * a, sd_val_t * b, char neg, int boffset)
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
   sd_val_t *r = make(mag, mag - end);
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
      errx(1, "Carry add error %d", c);
   return norm(r, neg);
}

static sd_val_t *usub(sd_val_t * a, sd_val_t * b, char neg, int boffset)
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
   sd_val_t *r = make(mag, mag - end);
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
      errx(1, "Carry sub error %d", c);
   return norm(r, neg);
}

static void makebase(sd_val_t * r[9], sd_val_t * a)
{                               // Make array of multiples of a, 1 to 9, used for multiply and divide
   r[0] = copy(a);
   for (int n = 1; n < 9; n++)
      r[n] = uadd(r[n - 1], a, 0, 0);
}

static void free_base(sd_val_t * r[9])
{
   for (int n = 0; n < 9; n++)
      freez(r[n]);
}

static sd_val_t *umul(sd_val_t * a, sd_val_t * b, char neg)
{                               // Unsigned mul (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("umul", a, b, NULL);
   sd_val_t *base[9];
   makebase(base, b);
   sd_val_t *r = copy(&zero);
   for (int p = 0; p < a->sig; p++)
      if (a->d[p])
      {                         // Add
         sd_val_t *sum = uadd(r, base[a->d[p] - 1], 0, a->mag - p);
         if (sum)
         {
            freez(r);
            r = sum;
         }
      }
   free_base(base);
   return norm(r, neg);
}

static sd_val_t *udiv(sd_val_t * a, sd_val_t * b, char neg, sd_val_t ** rem, int places, sd_round_t round)
{                               // Unsigned div (i.e. final sign already set in r) and set in r if needed
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   //debugout ("udiv", a, b, NULL);
   if (!b->sig)
      return NULL;              // Divide by zero
   sd_val_t *base[9];
   makebase(base, b);
   int mag = a->mag - b->mag;
   if (mag < -places)
      mag = -places;            // Limit to places
   int sig = mag + places + 1;  // Limit to places
   sd_val_t *r = make(mag, sig);
   sd_val_t *v = copy(a);
#ifdef DEBUG
   fprintf(stderr, "Divide %d->%d\n", mag, mag - sig + 1);
#endif
   for (int p = mag; p > mag - sig; p--)
   {
      int n = 0;
      while (n < 9 && ucmp(v, base[n], p) >= 0)
         n++;
      //debugout ("udiv rem", v, NULL);
      if (n)
      {
         sd_val_t *s = usub(v, base[n - 1], 0, p);
         if (s)
         {
            freez(v);
            v = s;
         }
      }
      r->d[mag - p] = n;
      if (!v->sig)
         break;
   }
   if (round != SD_ROUND_TRUNCATE && v->sig)
   {                            // Rounding
      if (!round)
         round = SD_ROUND_BANKING;      // Default
      if (neg)
      {                         // reverse logic for +/-
         if (round == SD_ROUND_FLOOR)
            round = SD_ROUND_CEILING;
         else if (round == SD_ROUND_CEILING)
            round = SD_ROUND_FLOOR;
      }
      int shift = mag - sig;
      int diff = ucmp(v, base[4], shift);
      if (round == SD_ROUND_UP ||       // Round up
          round == SD_ROUND_CEILING     // Round up
          || (round == SD_ROUND_ROUND && diff >= 0)     // Round up if 0.5 and above up
          || (round == SD_ROUND_BANKING && diff > 0)    // Round up if above 0.5
          || (round == SD_ROUND_BANKING && !diff && (r->d[r->sig - 1] & 1))     // Round up if 0.5 and odd previous digit
          )
      {                         // Add one
         if (rem)
         {                      // Adjust remainder, goes negative
            base[0]->mag += shift + 1;
            sd_val_t *s = usub(base[0], v, 0, 0);
            base[0]->mag -= shift + 1;
            freez(v);
            v = s;
            v->neg ^= 1;
         }
         // Adjust r
         sd_val_t *s = uadd(r, &one, 0, r->mag - r->sig + 1);
         freez(r);
         r = s;
      }
   }
   free_base(base);
   if (rem)
   {
      if (b->neg)
         v->neg ^= 1;
      if (neg)
         v->neg ^= 1;
      *rem = v;
   } else
      freez(v);
   return norm(r, neg);
}

static int scmp(sd_val_t * a, sd_val_t * b)
{
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout("scmp", a, b, NULL);
   if (a->neg && !b->neg)
      return -1;
   if (!a->neg && b->neg)
      return 1;
   if (a->neg && b->neg)
      return -ucmp(a, b, 0);
   return ucmp(a, b, 0);
}

static sd_val_t *sadd(sd_val_t * a, sd_val_t * b)
{                               // Low level add
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout("sadd", a, b, NULL);
   if (a->neg && !b->neg)
   {                            // Reverse subtract
      sd_val_t *t = a;
      a = b;
      b = t;
   }
   if (!a->neg && b->neg)
   {                            // Subtract
      int d = ucmp(a, b, 0);
      if (d < 0)
         return usub(b, a, 1, 0);
      return usub(a, b, 0, 0);
   }
   if (a->neg && b->neg)
      return uadd(a, b, 1, 0);  // Negative reply
   return uadd(a, b, 0, 0);
}

static sd_val_t *ssub(sd_val_t * a, sd_val_t * b)
{
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout("ssub", a, b, NULL);
   if (a->neg && !b->neg)
      return uadd(a, b, 1, 0);
   if (!a->neg && b->neg)
      return uadd(a, b, 0, 0);
   char neg = 0;
   if (a->neg && b->neg)
      neg ^= 1;                 // Invert output
   int d = ucmp(a, b, 0);
   if (!d)
      return copy(&zero);       // Zero
   if (d < 0)
      return usub(b, a, 1 - neg, 0);
   return usub(a, b, neg, 0);
}

static sd_val_t *smul(sd_val_t * a, sd_val_t * b)
{
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout("smul", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return umul(a, b, 1);
   return umul(a, b, 0);
}

static sd_val_t *sdiv(sd_val_t * a, sd_val_t * b, sd_val_t ** rem, int places, sd_round_t round)
{
   if (!a)
      a = &zero;
   if (!b)
      b = &zero;
   debugout("sdiv", a, b, NULL);
   if ((a->neg && !b->neg) || (!a->neg && b->neg))
      return udiv(a, b, 1, rem, places, round);
   return udiv(a, b, 0, rem, places, round);
}

static sd_val_t *srnd(sd_val_t * a, int places, sd_round_t round)
{
   debugout("srnd", a, NULL);
   if (!a)
      return NULL;
   int decimals = a->sig - a->mag - 1;
   if (decimals < 0)
      decimals = 0;
   sd_val_t *z(void) {
      sd_val_t *r = copy(&zero);
      r->mag = -places;
      if (places > 0)
         r->mag--;
      return r;
   }
   if (!a->sig)
      return z();
   if (decimals == places)
      return copy(a);           // Already that many places
   if (decimals > places)
   {                            // more places, needs truncating
      int sig = a->sig - (decimals - places);
      sd_val_t *r;
      if (sig <= 0)
      {
         r = z();
         if (sig < 0)
            return r;
         sig = 0;               // Allow rounding
         if (r->mag >= 0)
            r->mag--;
      } else
      {
         r = make(a->mag, sig);
         memcpy(r->d, a->d, sig);
      }
      if (round != SD_ROUND_TRUNCATE)
      {
         int p = sig;
         char up = 0;
         if (!round)
            round = SD_ROUND_BANKING;
         if (a->neg)
         {                      // reverse logic for +/-
            if (round == SD_ROUND_FLOOR)
               round = SD_ROUND_CEILING;
            else if (round == SD_ROUND_CEILING)
               round = SD_ROUND_FLOOR;
         }
         if (round == SD_ROUND_CEILING || round == SD_ROUND_UP)
         {                      // Up (away from zero) if not exact
            while (p < a->sig && !a->d[p])
               p++;
            if (p < a->sig)
               up = 1;          // not exact
         } else if (round == SD_ROUND_ROUND && a->d[p] >= 5)    // Up if .5 or above
            up = 1;
         else if (round == SD_ROUND_BANKING && a->d[p] > 5)
            up = 1;             // Up as more than .5
         else if (round == SD_ROUND_BANKING && a->d[p] == 5)
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
            sd_val_t *s = uadd(r, &one, 0, r->mag - r->sig + 1);
            freez(r);
            r = s;
            decimals = r->sig - r->mag - 1;
            if (decimals < 0)
               decimals = 0;
            if (decimals < places)
            {
               int sig = r->sig + (places - decimals);
               if (r->mag > 0)
                  sig = r->mag + 1 + places;
               sd_val_t *s = make(r->mag, sig);
               memcpy(s->d, r->d, r->sig);
               s->neg = r->neg;
               freez(r);
               r = s;
            }
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
      sd_val_t *r = make(a->mag, sig);
      memcpy(r->d, a->d, a->sig);
      r->neg = a->neg;
      return r;
   }
   return NULL;                 // Uh
}

// Maths string functions

char *stringdecimal_add_opts(stringdecimal_binary_t o)
{                               // Simple add
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
 sd_val_t *B = parse(o.b, nocomma:o.nocomma);
   sd_val_t *R = sadd(A, B);
 char *ret = output_f(R, A, B, comma:o.comma);
   if (o.a_free)
      freez(o.a);
   if (o.b_free)
      freez(o.b);
   return ret;
};

char *stringdecimal_sub_opts(stringdecimal_binary_t o)
{                               // Simple subtract
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
 sd_val_t *B = parse(o.b, nocomma:o.nocomma);
   sd_val_t *R = ssub(A, B);
 char *ret = output_f(R, A, B, comma:o.comma);
   if (o.a_free)
      freez(o.a);
   if (o.b_free)
      freez(o.b);
   return ret;
};

char *stringdecimal_mul_opts(stringdecimal_binary_t o)
{                               // Simple multiply
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
 sd_val_t *B = parse(o.b, nocomma:o.nocomma);
   sd_val_t *R = smul(A, B);
 char *ret = output_f(R, A, B, comma:o.comma);
   if (o.a_free)
      freez(o.a);
   if (o.b_free)
      freez(o.b);
   return ret;
};

char *stringdecimal_div_opts(stringdecimal_div_t o)
{                               // Simple divide - to specified number of places, with remainder
 sd_val_t *B = parse(o.b, nocomma:o.nocomma);
   if (B && !B->sig)
   {
      freez(B);
      return NULL;              // Divide by zero
   }
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
   sd_val_t *REM = NULL;
   sd_val_t *R = sdiv(A, B, &REM, o.places, o.round);
   if (o.remainder)
      *o.remainder = output(REM, o.comma);
   if (o.a_free)
      freez(o.a);
   if (o.b_free)
      freez(o.b);
 return output_f(R, A, B, REM, comma:o.comma);
};

char *stringdecimal_rnd_opts(stringdecimal_unary_t o)
{                               // Round to specified number of places
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
   sd_val_t *R = srnd(A, o.places, o.round);
 char *ret = output_f(R, A, comma:o.comma);
   if (o.a_free)
      freez(o.a);
   return ret;
};

int stringdecimal_cmp_opts(stringdecimal_binary_t o)
{                               // Compare
 sd_val_t *A = parse(o.a, nocomma:o.nocomma);
 sd_val_t *B = parse(o.b, nocomma:o.nocomma);
   int r = scmp(A, B);
   freez(A);
   freez(B);
   if (o.a_free)
      freez(o.a);
   if (o.b_free)
      freez(o.b);
   return r;
}

static struct sd_s sd_zero = { &zero };

static sd_p sd_tidy(sd_p v)
{                               // Check answer
   if (v && v->d && v->d->neg)
   {                            // Normalise sign
      v->n->neg ^= 1;
      v->d->neg = 0;
   }
   if (v && v->n && v->d && v->d->sig == 1 && v->d->d[0] == 1)
   {                            // Power of 10 denominator
      v->n->mag -= v->d->mag;
      freez(v->d);
   }
   return v;
}

static sd_p sd_new(sd_p l, sd_p r)
{                               // Basic details for binary operator
   sd_p v = calloc(1, sizeof(*v));
   assert(v);
   if (l)
      v->places = l->places;
   if (r && r->places > v->places)
      v->places = r->places;
   return v;
}

static sd_p sd_cross(sd_p l, sd_p r, sd_val_t ** ap, sd_val_t ** bp)
{                               // Cross multiply
   sd_debugout("sd_cross", l, r, NULL);
   *ap = *bp = NULL;
   sd_p v = sd_new(l, r);
   if ((l->d || r->d) && scmp(l->d ? : &one, r->d ? : &one))
   {                            // Multiply out numerators
      *ap = smul(l->n, r->d ? : &one);
      *bp = smul(r->n, l->d ? : &one);
      debugout("sd_crossed", *ap, *bp, NULL);
   }
   return v;
}

sd_p sd_copy(sd_p p)
{                               // Copy (create if p is NULL)
   if (!p || !p->n)
      p = &sd_zero;
   sd_p v = calloc(1, sizeof(*v));
   assert(v);
   v->places = p->places;
   if (p->n)
      v->n = copy(p->n);
   if (p->d)
      v->d = copy(p->d);
   return v;
}

sd_p sd_parse_opts(sd_parse_t o)
{
   int places = 0;
 sd_val_t *n = parse(o.a, placesp: &places, nocomma:o.nocomma);
   if (!n)
      return NULL;
   sd_p v = calloc(1, sizeof(*v));
   assert(v);
   v->places = places;
   v->n = n;
   v->d = NULL;
   if (o.a_free)
      freez(o.a);
   return v;
}

sd_p sd_int(long long v)
{
   char temp[40];
   snprintf(temp, sizeof(temp), "%lld", v);
   return sd_parse(temp);
}

sd_p sd_float(long double v)
{
   char temp[50];
   snprintf(temp, sizeof(temp), "%.32Le", v);
   return sd_parse(temp);
}

void *sd_free(sd_p p)
{                               // Free
   if (!p)
      return p;
   freez(p->d);
   freez(p->n);
   freez(p);
   return NULL;
}

int sd_places(sd_p p)
{                               // Max places seen
   if (!p)
      return INT_MIN;
   return p->places;
}

char *sd_output_opts(sd_output_opts_t o)
{                               // Output
   if (!o.p)
      o.p = &sd_zero;
   if (!o.format)
   {                            // Defaults
      if (!o.places)
      {
         o.format = SD_FORMAT_EXTRA;    // Extra for divide
         o.places = 3;
      } else
         o.format = SD_FORMAT_EXACT;    // Exact places
   }
   char *r = NULL;
   switch (o.format)
   {
   case SD_FORMAT_RATIONAL:    // Rational
      {                         // rational mode
         sd_p c = sd_copy(o.p);
         sd_rational(c);        // Normalise to integers
         sd_val_t *rem = NULL;
         sd_val_t *res = sdiv(c->n, c->d, &rem, 0, SD_ROUND_TRUNCATE);
         if (!rem->sig)
            r = output(res, o.comma);   // No remainder, so integer
         freez(rem);
         freez(res);
         if (!r)
         {                      // Rational
            char *n = output(c->n, o.comma);
            char *d = output(c->d, o.comma);
            if (asprintf(&r, "(%s/%s)", n, d) < 0)
               errx(1, "malloc");
            freez(d);
            freez(n);
         }
         sd_free(c);
      }
      break;
   case SD_FORMAT_LIMIT:
      if (o.p->d)
       r = output_f(sdiv(o.p->n, o.p->d, NULL, o.places, o.round), comma:o.comma);
      else
         return output(o.p->n, o.comma);
      break;
   case SD_FORMAT_EXACT:
      if (o.p->d)
       r = output_f(srnd(sdiv(o.p->n, o.p->d, NULL, o.places, o.round), o.places, o.round), comma:o.comma);
      else
       r = output_f(srnd(o.p->n, o.places, o.round), comma:o.comma);
      break;
   case SD_FORMAT_INPUT:
      if (o.p->d)
       r = output_f(srnd(sdiv(o.p->n, o.p->d, NULL, o.p->places + o.places, o.round), o.places, o.round), comma:o.comma);
      else
       r = output_f(srnd(o.p->n, o.p->places + o.places, o.round), comma:o.comma);
      break;
   case SD_FORMAT_EXTRA:
      if (o.p->d)
      {
         sd_p c = sd_copy(o.p);
         sd_rational(c);
       r = output_f(sdiv(c->n, c->d, NULL, c->d->mag + o.places, o.round), comma:o.comma);
         sd_free(c);
      } else
         return output(o.p->n, o.comma);
      break;
   case SD_FORMAT_MAX:
      if (o.p->d)
       r = output_f(sdiv(o.p->n, o.p->d, NULL, o.p->places + o.places, o.round), comma:o.comma);
      else
         return output(o.p->n, o.comma);
      break;
   case SD_FORMAT_EXP:
      {
         sd_val_t *q = NULL;
         sd_p c = sd_copy(o.p);
         int exp = c->n->mag;
         // Work out exp
         if (c->d)
            exp -= c->d->mag;
         c->n->mag -= exp;
         if (c->d && ucmp(c->n, c->d, 0) < 0)
         {
            exp--;
            c->n->mag++;
         }
         void try(void) {       // Try formatting - we have to allow for possibly rounding up and adding a digit, so may have to try twice
            freez(q);
            if (c->d)
            {
               if (o.places < 0)
                  q = sdiv(c->n, c->d, NULL, (c->n->sig > c->d->sig ? c->n->sig : c->d->sig) - o.places - 1, o.round);
               else
                  q = srnd(sdiv(c->n, c->d, NULL, o.places, o.round), o.places, o.round);
            } else if (o.places < 0)
               q = copy(c->n);  // Just do as many digits as needed
            else
               q = srnd(c->n, o.places, o.round);
         }
         try();
         if (q->mag > 0)
         {                      // Rounded up, try again
            exp += q->mag;
            c->n->mag -= q->mag;
            try();
         }
       char *v = output_f(q, comma:o.comma);
         if (asprintf(&r, "%se%+d", v, exp) < 0)
            errx(1, "malloc");
         freez(v);
         sd_free(c);
      }
      break;
   default:
      fprintf(stderr, "Unknown format %c\n", o.format);
      break;
   }
   if (o.p_free)
      sd_free(o.p);
   return r;
}

sd_p sd_neg_i(sd_p p)
{                               // Negate (in situ)
   if (!p)
      return p;
   if (p->n->sig)
      p->n->neg ^= 1;
   return p;
}

sd_p sd_neg(sd_p p)
{                               // Negate
   return sd_neg_i(sd_copy(p));
}

sd_p sd_abs_i(sd_p p)
{                               // Absolute (in situ)
   if (!p)
      return p;
   p->n->neg = 0;
   return p;
}

sd_p sd_abs(sd_p p)
{                               // Absolute
   return sd_abs_i(sd_copy(p));
}

sd_p sd_inv_i(sd_p p)
{                               // Reciprocal (in situ)
   if (!p)
      return p;
   sd_val_t *d = p->d;
   if (!d)
      d = copy(&one);
   p->d = p->n;
   p->n = d;
   return p;
}

sd_p sd_inv(sd_p p)
{                               // Reciprocal
   return sd_inv_i(sd_copy(p));
}

sd_p sd_10_i(sd_p p, int shift)
{                               // Adjust by power of 10 (in situ)
   if (!p || !p->n)
      return p;
   p->n->mag += shift;
   return p;
}

sd_p sd_10(sd_p p, int shift)
{                               // Adjust by power of 10
   return sd_10_i(sd_copy(p), shift);
}

int sd_iszero(sd_p p)
{                               // Is zero
   return !p || !p->n || !p->n->sig;
}

int sd_isneg(sd_p p)
{                               // Is negative (denominator always positive)
   return p && p->n && p->n->neg;
}

int sd_ispos(sd_p p)
{                               // Is positive (denominator always positive)
   return p && p->n && p->n->sig && !p->n->neg;
}

static void sd_rational(sd_p p)
{                               // Normalise to integers
   if (!p || !p->n)
      return;
   if (!p->d)
      p->d = copy(&one);
   int s,
    shift = p->n->sig - p->n->mag - 1;
   if ((s = p->d->sig - p->d->mag - 1) > shift)
      shift = s;
   p->n->mag += shift;
   p->d->mag += shift;
}

sd_p sd_add(sd_p l, sd_p r)
{                               // Add
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_debugout("sd_add", l, r, NULL);
   sd_val_t *a,
   *b;
   sd_p v = sd_cross(l, r, &a, &b);
   if (v)
   {
      v->n = sadd(a ? : l->n, b ? : r->n);
      if (l->d || r->d)
      {
         if (!scmp(l->d ? : &one, r->d ? : &one))
            v->d = copy(l->d ? : &one);
         else
            v->d = smul(l->d ? : &one, r->d ? : &one);
      }
      freez(a);
      freez(b);
   }
   return sd_tidy(v);
};

sd_p sd_add_ff(sd_p l, sd_p r)
{                               // Add free all args
   sd_p o = sd_add(l, r);
   sd_free(l);
   sd_free(r);
   return o;
};

sd_p sd_add_fc(sd_p l, sd_p r)
{                               // Add free first arg
   sd_p o = sd_add(l, r);
   sd_free(l);
   return o;
};

sd_p sd_add_cf(sd_p l, sd_p r)
{                               // Add free second arg
   sd_p o = sd_add(l, r);
   sd_free(r);
   return o;
};

sd_p sd_sub(sd_p l, sd_p r)
{                               // Subtract
   if (r && r->n)
      r->n->neg ^= 1;
   sd_p o = sd_add(l, r);
   if (r && r->n)
      r->n->neg ^= 1;
   return o;
};

sd_p sd_sub_ff(sd_p l, sd_p r)
{                               // Subtract free all args
   sd_p o = sd_sub(l, r);
   sd_free(l);
   sd_free(r);
   return o;
};

sd_p sd_sub_fc(sd_p l, sd_p r)
{                               // Subtract free first arg
   sd_p o = sd_sub(l, r);
   sd_free(l);
   return o;
};

sd_p sd_sub_cf(sd_p l, sd_p r)
{                               // Subtract free second arg
   sd_p o = sd_sub(l, r);
   sd_free(r);
   return o;
};

sd_p sd_mul(sd_p l, sd_p r)
{                               // Multiply
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_debugout("sd_mul", l, r, NULL);
   sd_p v = sd_new(l, r);
   if (r->d && !scmp(l->n, r->d))
   {                            // Cancel out
      v->n = copy(r->n);
      v->d = copy(l->d);
      if (l->n->neg)
         v->n->neg ^= 1;
   } else if (l->d && !scmp(r->n, l->d))
   {                            // Cancel out
      v->n = copy(l->n);
      v->d = copy(r->d);
      if (r->n->neg)
         v->n->neg ^= 1;
   } else
   {                            // Multiple
      if (l->d || r->d)
         v->d = smul(l->d ? : &one, r->d ? : &one);
      v->n = smul(l->n, r->n);
   }
   return sd_tidy(v);
};

sd_p sd_mul_ff(sd_p l, sd_p r)
{                               // Multiply free all args
   sd_p o = sd_mul(l, r);
   sd_free(l);
   sd_free(r);
   return o;
};

sd_p sd_mul_fc(sd_p l, sd_p r)
{                               // Multiply free first arg
   sd_p o = sd_mul(l, r);
   sd_free(l);
   return o;
};

sd_p sd_mul_cf(sd_p l, sd_p r)
{                               // Multiply free second arg
   sd_p o = sd_mul(l, r);
   sd_free(r);
   return o;
};

sd_p sd_div(sd_p l, sd_p r)
{                               // Divide
   if (!l)
      l = &sd_zero;
   if (!r || !r->n || !r->n->sig)
      return NULL;
   sd_debugout("sd_div", l, r, NULL);
   sd_p v = sd_new(l, r);
   if (!l->d && !r->d)
   {                            // Simple - making a new rational
      v->n = copy(l->n);
      v->d = copy(r->n);
   } else
   {                            // Flip and multiply 
      sd_val_t *t = r->n;
      r->n = r->d ? : copy(&one);
      r->d = t;
      r->n->neg = r->d->neg;
      r->d->neg = 0;
      return sd_mul(l, r);
   }
   return sd_tidy(v);
};

sd_p sd_div_ff(sd_p l, sd_p r)
{                               // Divide free all args
   sd_p o = sd_div(l, r);
   sd_free(l);
   sd_free(r);
   return o;
};

sd_p sd_div_fc(sd_p l, sd_p r)
{                               // Divide free first arg
   sd_p o = sd_div(l, r);
   sd_free(l);
   return o;
};

sd_p sd_div_cf(sd_p l, sd_p r)
{                               // Divide free second arg
   sd_p o = sd_div(l, r);
   sd_free(r);
   return o;
};

sd_p sd_pow(sd_p l, sd_p r)
{
   if (r->n->neg)
      return NULL;
   sd_val_t *p = NULL;
   sd_val_t *rem = NULL;
   p = udiv(r->n, r->d ? : &one, 0, &rem, 0, SD_ROUND_TRUNCATE);
   if (rem->sig)
   {
      freez(p);
      freez(rem);
      return NULL;              // Not integer
   }
   freez(rem);
   if (p->sig > p->mag + 1)
   {                            // Not integer
      freez(p);
      return NULL;
   }
   sd_p m = sd_copy(l);
   sd_p res = sd_int(1);
   while (p->sig)
   {
      sd_val_t *p2 = udiv(p, &two, 0, &rem, 0, SD_ROUND_TRUNCATE);
      freez(p);
      p = p2;
      if (rem->sig)
         res = sd_mul_fc(res, m);
      freez(rem);
      if (!p->sig)
         break;
      m = sd_mul_fc(m, m);
   }
   freez(p);
   sd_free(m);
   return res;
}

sd_p sd_pow_ff(sd_p l, sd_p r)
{                               // Integer power free all args
   sd_p o = sd_pow(l, r);
   sd_free(l);
   sd_free(r);
   return o;
};

sd_p sd_pow_fc(sd_p l, sd_p r)
{                               // Integer power free first arg
   sd_p o = sd_pow(l, r);
   sd_free(l);
   return o;
};

sd_p sd_pow_cf(sd_p l, sd_p r)
{                               // Integer power free second arg
   sd_p o = sd_pow(l, r);
   sd_free(r);
   return o;
};

int sd_abs_cmp(sd_p l, sd_p r)
{                               // Compare absolute values
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_val_t *a,
   *b;
   sd_p v = sd_cross(l, r, &a, &b);
   int diff = ucmp(a ? : l->n, b ? : r->n, 0);
   sd_free(v);
   freez(a);
   freez(b);
   return diff;
};

int sd_abs_cmp_ff(sd_p l, sd_p r)
{                               // Compare free all args
   int diff = sd_abs_cmp(l, r);
   sd_free(l);
   sd_free(r);
   return diff;
};

int sd_abs_cmp_fc(sd_p l, sd_p r)
{                               // Compare free first arg
   int diff = sd_abs_cmp(l, r);
   sd_free(l);
   return diff;
};

int sd_abs_cmp_cf(sd_p l, sd_p r)
{                               // Compare free second arg
   int diff = sd_abs_cmp(l, r);
   sd_free(r);
   return diff;
};

int sd_cmp(sd_p l, sd_p r)
{                               // Compare
   if (!l)
      l = &sd_zero;
   if (!r)
      r = &sd_zero;
   sd_val_t *a,
   *b;
   sd_p v = sd_cross(l, r, &a, &b);
   int diff = scmp(a ? : l->n, b ? : r->n);
   sd_free(v);
   freez(a);
   freez(b);
   return diff;
};

int sd_cmp_ff(sd_p l, sd_p r)
{                               // Compare free all args
   int diff = sd_cmp(l, r);
   sd_free(l);
   sd_free(r);
   return diff;
};

int sd_cmp_fc(sd_p l, sd_p r)
{                               // Compare free first arg
   int diff = sd_cmp(l, r);
   sd_free(l);
   return diff;
};

int sd_cmp_cf(sd_p l, sd_p r)
{                               // Compare free second arg
   int diff = sd_cmp(l, r);
   sd_free(r);
   return diff;
};

#ifdef	EVAL
// Parsing
#include "xparse.c"
// Parse Support functions
static void *parse_operand(void *context, const char *p, const char **end)
{                               // Parse an operand, malloc value (or null if error), set end
   stringdecimal_context_t *C = context;
   sd_p v = calloc(1, sizeof(*v));
   assert(v);
 v->n = parse(p, end: end, placesp: &v->places, nocomma: C->nocomma, comma:C->comma);
   v->d = NULL;
   return v;
}

static void *parse_final(void *context, void *v)
{                               // Final processing
   stringdecimal_context_t *C = context;
   if (C->raw)
      return v;
   sd_p V = v;
   if (!V)
      return NULL;
 return sd_output(V, places: V->places, places: C->places, format: C->format, round: C->round, comma:C->comma);
}

static void parse_dispose(void *context, void *v)
{                               // Disposing of an operand
   sd_free(v);
}

static void parse_fail(void *context, const char *failure, const char *posn)
{                               // Reporting an error
   stringdecimal_context_t *C = context;
   if (!C->fail)
      C->fail = failure;
   C->posn = posn;
}

#define MATCH_LT 1
#define MATCH_GT 2
#define MATCH_EQ 4
#define MATCH_NE 8
static sd_p parse_bin_cmp(sd_p l, sd_p r, int match)
{
   sd_val_t *a,
   *b;
   sd_p L = l,
       R = r,
       v = sd_cross(l, r, &a, &b);
   int diff = scmp(a ? : L->n, b ? : R->n);
   if (((match & MATCH_LT) && diff < 0) || ((match & MATCH_GT) && diff > 0) || ((match & MATCH_EQ) && diff == 0) || ((match & MATCH_NE) && diff != 0))
      v->n = copy(&one);
   else
      v->n = copy(&zero);
   v->places = 0;
   return v;
}

// Parse Functions
static void *parse_si(void *context, void *data, void **a)
{                               // SI prefix (suffix on number)
   return sd_10_i(*a, (long) data);
}

static void *parse_ieee(void *context, void *data, void **a)
{                               // IEEE prefix (suffix on number)
   long n = (long) data;
   return sd_mul_cf(*a, sd_int(n));
}

static void *parse_add(void *context, void *data, void **a)
{
   return sd_add(a[0], a[1]);
}

static void *parse_sub(void *context, void *data, void **a)
{
   return sd_sub(a[0], a[1]);
}

static void *parse_div(void *context, void *data, void **a)
{
   stringdecimal_context_t *C = context;
   sd_p o = sd_div(a[0], a[1]);
   if (!o)
      C->fail = "Divide by zero";
   return o;
}

static void *parse_mul(void *context, void *data, void **a)
{
   return sd_mul(a[0], a[1]);
}

static void *parse_pow(void *context, void *data, void **a)
{
   stringdecimal_context_t *C = context;
   sd_p L = a[0],
       R = a[1];
   sd_p o = sd_pow(L, R);
   if (!o)
      C->fail = "Power must be positive integer";
   return o;
}

static void *parse_neg(void *context, void *data, void **a)
{
   sd_p A = *a;
   A->n->neg ^= 1;
   return A;
}

static void *parse_abs(void *context, void *data, void **a)
{
   sd_p A = *a;
   A->n->neg = 0;
   return A;
}

static void *parse_not(void *context, void *data, void **a)
{
   sd_p A = *a;
   sd_p v = sd_new(A, NULL);
   v->n = copy(!A->n->sig ? &one : &zero);
   v->places = 0;
   return v;
}

static void *parse_eq(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_EQ);
}

static void *parse_ne(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_NE);
}

static void *parse_gt(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_GT);
}

static void *parse_ge(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_EQ | MATCH_GT);
}

static void *parse_le(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_EQ | MATCH_LT);
}

static void *parse_lt(void *context, void *data, void **a)
{
   return parse_bin_cmp(a[0], a[1], MATCH_LT);
}

static void *parse_and(void *context, void *data, void **a)
{
   sd_p L = a[0],
       R = a[1];
   if (!L->n->sig)
      return L;                 // False
   return R;                    // Whatever second argument is
}

static void *parse_or(void *context, void *data, void **a)
{
   sd_p L = a[0],
       R = a[1];
   if (L->n->sig)
      return L;                 // True
   return R;                    // Whatever second argument is
}

static void *parse_cond(void *context, void *data, void **a)
{
   if (!sd_iszero(a[0]))
      return a[1] ? : a[0];     // Allow for null second operator
   return a[2];
}

// List of functions - as pre C operator precedence with comma as 1, and postfix as 15
// e.g. https://www.tutorialspoint.com/cprogramming/c_operators_precedence.htm
static xparse_op_t parse_unary[] = {
   // Postfix would be 15
 { op: "-", level: 14, func:parse_neg },
 { op: "!", op2: "¬", level: 14, func:parse_not },
 { op: "|", op2: "||", level: 14, func:parse_abs },
   { NULL },
};

static xparse_op_t parse_post[] = {
#define	u(p,n)	{ op:#p,level:15,func:parse_ieee,data:(void*)n},
   ieee
#undef u
#define	u(p,n)	{ op:#p,level:15,func:parse_si,data:(void*)n},
       si
#undef u
   { NULL },
};

static xparse_op_t parse_binary[] = {
 { op: "^", level: 14, func:parse_pow },
 { op: "/", op2: "÷", level: 13, func:parse_div },
 { op: "*", op2: "×", level: 13, func:parse_mul },
   // % would be 14, we should add that
 { op: "+", level: 12, func:parse_add },
 { op: "-", op2: "−", level: 12, func:parse_sub },
   // Shift would be 11
 { op: ">=", op2: "≥", level: 10, func:parse_ge },
 { op: "<=", op2: "≤", level: 10, func:parse_le },
 { op: "!=", op2: "≠", level: 10, func:parse_ne },
 { op: ">", op2: "≰", level: 10, func:parse_gt },
 { op: "<", op2: "≱", level: 10, func:parse_lt },
 { op: "==", op2: "=", level: 9, func:parse_eq },
   // & would be 8
   // ^ would be 7
   // | would be 6
 { op: "&&", op2: "∧", level: 5, func:parse_and },
 { op: "||", op2: "∨", level: 4, func:parse_or },
   { NULL },
};

static xparse_op_t parse_ternary[] = {
 { op: "?", op2: ":", level: 3, func:parse_cond },
   // Assignment would be 2
   // Comma would be 1
   { NULL },
};

static xparse_map_t parse_map[] = {
   { "¼", "(1/4)" },
   { "½", "(1/2)" },
   { "¾", "(3/4)" },
   { "⅐", "(1/7)" },
   { "⅑", "(1/9)" },
   { "⅒", "(1/10)" },
   { "⅓", "(1/3)" },
   { "⅔", "(2/3)" },
   { "⅕", "(1/5)" },
   { "⅖", "(2/5)" },
   { "⅗", "(3/5)" },
   { "⅘", "(4/5)" },
   { "⅙", "(1/6)" },
   { "⅚", "(5/6)" },
   { "⅛", "(1/8)" },
   { "⅜", "(3/8)" },
   { "⅝", "(5/8)" },
   { "⅞", "(7/8)" },
   { "⅟", "1/" },
   { "↉", "(0/3)" },
   { NULL },
};

// Parse Config (optionally public to allow building layers on top)
xparse_config_t stringdecimal_xparse = {
 unary:parse_unary,
 post:parse_post,
 binary:parse_binary,
 ternary:parse_ternary,
 operand:parse_operand,
 final:parse_final,
 dispose:parse_dispose,
 fail:parse_fail,
 map:parse_map,
};

// Parse
char *stringdecimal_eval_opts(stringdecimal_unary_t o)
{
 stringdecimal_context_t context = { places: o.places, format: o.format, round: o.round, nocomma: o.nocomma, comma:o.comma };
   char *ret = xparse(&stringdecimal_xparse, &context, o.a, NULL);
   if (!ret)
      assert(asprintf(&ret, "!!%s at %.*s", context.fail, 10, context.posn) >= 0);
   if (o.a_free)
      freez(o.a);
   return ret;
}
#endif

#ifndef LIB
// Test function main build
#include <popt.h>
int main(int argc, const char *argv[])
{
   int places = 0;
   char *round = "";
   char *format = "";
   int comma = 0;
   int nocomma = 0;
   {                            // POPT
      poptContext optCon;       // context for parsing command-line options
      const struct poptOption optionsTable[] = {
         { "places", 'p', POPT_ARG_INT, &places, 0, "Places", "N" },
         { "format", 'f', POPT_ARG_STRING, &format, 0, "Format", "-/=/+/>/*/e//" },
         { "round", 'r', POPT_ARG_STRING, &round, 0, "Rounding", "T/U/F/C/R/B" },
         { "no-comma", 'n', POPT_ARG_NONE, &nocomma, 0, "No comma in input" },
         { "comma", 'c', POPT_ARG_NONE, &comma, 0, "Comma in output" },
         POPT_AUTOHELP { }
      };
      optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp(optCon, "Sums");
      int c;
      if ((c = poptGetNextOpt(optCon)) < -1)
         errx(1, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));
      if (!poptPeekArg(optCon))
      {
         poptPrintUsage(optCon, stderr, 0);
         return -1;
      }
      const char *s;
      while ((s = poptGetArg(optCon)))
      {
       char *res = stringdecimal_eval(s, places: places, format: *format, round: *round, comma: comma, nocomma:nocomma);
         if (res)
            printf("%s\n", res);
         freez(res);
      }
      poptFreeContext(optCon);
   }
   return 0;
}
#endif
