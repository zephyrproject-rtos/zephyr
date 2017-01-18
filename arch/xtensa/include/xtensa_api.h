/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
  Xtensa-specific API for RTOS ports.
******************************************************************************/

#ifndef __XTENSA_API_H__
#define __XTENSA_API_H__

#include <xtensa/hal.h>
#include "xtensa_rtos.h"
#include "xtensa_context.h"


/* Typedef for C-callable interrupt handler function */
typedef void (*xt_handler)(void *);

/* Typedef for C-callable exception handler function */
typedef void (*xt_exc_handler)(XtExcFrame *);


/*
-------------------------------------------------------------------------------
  Call this function to set a handler for the specified exception.

    n        - Exception number (type)
    f        - Handler function address, NULL to uninstall handler.

  The handler will be passed a pointer to the exception frame, which is created
  on the stack of the thread that caused the exception.

  If the handler returns, the thread context will be restored and the faulting
  instruction will be retried. Any values in the exception frame that are
  modified by the handler will be restored as part of the context. For details
  of the exception frame structure see xtensa_context.h.
-------------------------------------------------------------------------------
*/
extern xt_exc_handler _xt_set_exception_handler(int n, xt_exc_handler f);


/*
-------------------------------------------------------------------------------
  Call this function to set a handler for the specified interrupt.
 
    n        - Interrupt number.
    f        - Handler function address, NULL to uninstall handler.
    arg      - Argument to be passed to handler.
-------------------------------------------------------------------------------
*/
#if XT_RTOS_IS_ZEPHYR_OS
#define _xt_set_interrupt_handler _irq_handler_set /* Map it to Zephyr's one */
#else
extern xt_handler _xt_set_interrupt_handler(int n, xt_handler f, void * arg);
#endif


/*
-------------------------------------------------------------------------------
  Call this function to enable the specified interrupts.

    mask     - Bit mask of interrupts to be enabled.
-------------------------------------------------------------------------------
*/
extern void _xt_ints_on(unsigned int mask);


/*
-------------------------------------------------------------------------------
  Call this function to disable the specified interrupts.

    mask     - Bit mask of interrupts to be disabled.
-------------------------------------------------------------------------------
*/
extern void _xt_ints_off(unsigned int mask);


/*
-------------------------------------------------------------------------------
  Call this function to set the specified (s/w) interrupt.
-------------------------------------------------------------------------------
*/
static inline void _xt_set_intset(unsigned int arg)
{
    xthal_set_intset(arg);
}


/*
-------------------------------------------------------------------------------
  Call this function to clear the specified (s/w or edge-triggered)
  interrupt.
-------------------------------------------------------------------------------
*/
static inline void _xt_set_intclear(unsigned int arg)
{
    xthal_set_intclear(arg);
}


#endif /* __XTENSA_API_H__ */

