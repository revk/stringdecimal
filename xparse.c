// Functions to perform simple expression parsing
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

#include <ctype.h>
#include "xparse.h"

// The parse function
void *
xparse (xparse_config_t * config, void *context, const char *sum, const char **end)
{
   if (end)
      *end = NULL;
   const char *fail = NULL;
   int level = 0;               // Bracketing level
   int operators = 0,
      operatormax = 0;
   struct operator_s
   {
      int level;
      int args;
      xparse_operate *func;
      void *data;
   } *operator = NULL;          // Operator stack
   int operands = 0,
      operandmax = 0;
   void **operand = NULL;
   void addarg (void *v)
   {
      if (operands + 1 > operandmax)
      {
         operandmax += 10;
         operand = realloc (operand, operandmax * sizeof (*operand));
      }
      operand[operands] = v;
      operands++;
   }
   void operate (void)
   {
      if (!operators--)
      {
         fail = "Mission operator";
         return;
      }
      int args = operator[operators].args;
      if (operands < args)
      {
         fail = "Missing args";
         return;
      }
      if (args > 2)
      {
         fail = "Cannot handle more than 2 args at present";
         return;
      }
      void *v = NULL;
      if (args == 1)
         v = operator[operators].func (context, operator[operators].data, NULL, operand[operands - 1]);
      else
         v = operator[operators].func (context, operator[operators].data, operand[operands - 2], operand[operands - 1]);
      while (args--)
      {
         operands--;
         if (operand[operands] != v)
            config->dispose (context, operand[operands]);
      }
      if (!v)
      {
         fail = "Operation failed";
         return;
      }
      addarg (v);
   }
   void addop (const xparse_op_t * op, int level, int args)
   {                            // Add an operator
      if (args < 0)
         args = 0 - args;       // Used for prefix unary ops, don't run stack
      else
         while (operators && operator[operators - 1].level >= level)
            operate ();         // Clear stack of pending ops
      if (operators + 1 > operatormax)
         operator = realloc (operator, (operatormax += 10) * sizeof (*operator));
      operator[operators].func = op->func;
      operator[operators].data = op->data;
      operator[operators].level = level;
      operator[operators].args = args;
      operators++;
   }
   int comp (const char *a, const char *b)
   {
      if (!a || !b)
         return 0;
      int l = 0;
      while (a[l] && b[l] && a[l] == b[l])
         l++;
      if (!a[l])
         return l;
      return 0;
   }
   while (*sum && !fail)
   {
      // Prefix operators and open brackets
      if (*sum == '!' && sum[1] == '!')
      {
         fail = "Error";
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
         // Unary operators
         int q = 0,
            l;
         for (q = 0; config->unary[q].op; q++)
            if ((l = comp (config->unary[q].op, sum)) || (l = comp (config->unary[q].op2, sum)))
            {
               sum += l;
               addop (&config->unary[q], level + config->unary[q].level, -1);
               break;
            }
         if (config->unary[q].op)
            continue;
         break;
      }
      // Operand
      const char *was = sum;
      void *v = config->operand (context, sum, &sum);
      if (!v || sum == was)
      {
         fail = "Missing operand";
         break;
      }
      // Add the operand
      addarg (v);
      // Postfix operators and close brackets
      while (1)
      {
         if (*sum == ')')
         {
            if (!level)
            {
               fail = "Too many close brackets";
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
      int q = 0,
         l;
      for (q = 0; config->binary[q].op; q++)
         if ((l = comp (config->binary[q].op, sum)) || (l = comp (config->binary[q].op2, sum)))
         {
            sum += l;
            addop (&config->binary[q], level + config->binary[q].level, 2);
            break;
         }
      if (config->binary[q].op)
         continue;
      if (!end || level)
         fail = "Missing/unknown operator";
      break;
   }
   while (!fail && operators)
      operate ();               // Final operators
   void *v = NULL;
   if (!fail && operands == 1)
      v = config->final (context, operand[0]);
   while (operands)
      config->dispose (context, operand[--operands]);
   if (operand)
      free (operand);
   if (operator)
      free (operator);
   if (fail)
      config->fail (context, fail, sum);
   if (end)
      *end = sum;
   return v;
}
