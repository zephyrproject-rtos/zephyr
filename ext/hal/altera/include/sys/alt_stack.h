/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/

#ifndef __ALT_STACK_H__
#define __ALT_STACK_H__

/*
 * alt_stack.h is the nios2 specific implementation of functions used by the
 * stack overflow code.
 */

#include "nios2.h"

#include "alt_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


extern char * alt_stack_limit_value;

#ifdef ALT_EXCEPTION_STACK
extern char __alt_exception_stack_pointer[];  /* set by the linker */
#endif /* ALT_EXCEPTION_STACK */


/*
 * alt_stack_limit can be called to determine the current value of the stack
 * limit register.
 */

static ALT_INLINE char * ALT_ALWAYS_INLINE alt_stack_limit (void)
{
  char * limit;
  NIOS2_READ_ET(limit);

  return limit; 
}

/*
 * alt_stack_pointer can be called to determine the current value of the stack
 * pointer register.
 */

static ALT_INLINE char * ALT_ALWAYS_INLINE alt_stack_pointer (void)
{
  char * pointer;
  NIOS2_READ_SP(pointer);

  return pointer; 
}


#ifdef ALT_EXCEPTION_STACK

/*
 * alt_exception_stack_pointer returns the normal stack pointer from
 * where it is stored on the exception stack (uppermost 4 bytes).  This
 * is really only useful during exception processing, and is only
 * available if a separate exception stack has been configured.
 */

static ALT_INLINE char * ALT_ALWAYS_INLINE alt_exception_stack_pointer (void)
{
  return (char *) *(alt_u32 *)(__alt_exception_stack_pointer - sizeof(alt_u32));
}

#endif /* ALT_EXCEPTION_STACK */


/*
 * alt_set_stack_limit can be called to update the current value of the stack
 * limit register.
 */

static ALT_INLINE void ALT_ALWAYS_INLINE alt_set_stack_limit (char * limit)
{
  alt_stack_limit_value = limit;
  NIOS2_WRITE_ET(limit);
}

/*
 * alt_report_stack_overflow reports that a stack overflow happened.
 */

static ALT_INLINE void ALT_ALWAYS_INLINE alt_report_stack_overflow (void)
{
  NIOS2_REPORT_STACK_OVERFLOW();
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_STACK_H__ */
