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
#ifndef XPARSE_H
#define	XPARSE_H

// The operators take one or two args and return a result.
// The result must be malloced. The args should be left intact and are freed as needed later
typedef void *xparse_operate (void *context, void *l, void *r); // For unary prefix operators l is NULL
// Parse operand
typedef void *xparse_operand (void *context, const char *p, const char **end);  // Parse an operand, malloc value (or null if error), set end
// Final processing
typedef void *xparse_final (void *context, void *v);
// Disposing of an operand
typedef void xparse_free (void *context, void *v);
// Reporting an error
typedef void xparse_fail (void *context, const char *failure,const char *posn);

// The operator list - operators that are a subset of another must be later in the list
typedef struct xparse_op_s xparse_op_t;
struct xparse_op_s
{
   const char *op;              // Final entry should have NULL here, else this is the operator name
   char level;                  // Operator precedence 0-9
   xparse_operate *func;        // Function to do operation
};

// This is the top level config
typedef struct xparse_config_s xparse_config_t;
struct xparse_config_s
{
   const xparse_op_t *unary;    // unary operators
   const xparse_op_t *binary;   // binary operators
   xparse_operand *operand;     // operand parse
   xparse_final *final;         // final process operand
   xparse_free *dispose;	// Dispose of an operand
   xparse_fail *fail;		// Failure report
};

// The parse function
void *xparse (xparse_config_t * config, void *context, const char *sum, const char **end);

#endif
