#ifndef __ALT_LEGACY_IRQ_H__
#define __ALT_LEGACY_IRQ_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2009 Altera Corporation, San Jose, California, USA.           *
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

/*
 * This file provides prototypes and inline implementations of certain routines
 * used by the legacy interrupt API. Do not include this in your driver or 
 * application source files, use "sys/alt_irq.h" instead to access the proper
 * public API.
 */
 
#include <errno.h>
#include "system.h"

#ifndef NIOS2_EIC_PRESENT

#include "nios2.h"
#include "alt_types.h"

#include "sys/alt_irq.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * alt_irq_register() can be used to register an interrupt handler. If the 
 * function is succesful, then the requested interrupt will be enabled upon 
 * return.
 */
extern int alt_irq_register (alt_u32 id, 
                             void*   context, 
                             alt_isr_func handler);

/*
 * alt_irq_disable() disables the individual interrupt indicated by "id".
 */
static ALT_INLINE int ALT_ALWAYS_INLINE alt_irq_disable (alt_u32 id)
{
  alt_irq_context  status;
  extern volatile alt_u32 alt_irq_active;

  status = alt_irq_disable_all ();

  alt_irq_active &= ~(1 << id);
  NIOS2_WRITE_IENABLE (alt_irq_active);

  alt_irq_enable_all(status);

  return 0;
}

/*
 * alt_irq_enable() enables the individual interrupt indicated by "id".
 */
static ALT_INLINE int ALT_ALWAYS_INLINE alt_irq_enable (alt_u32 id)
{
  alt_irq_context  status;
  extern volatile alt_u32 alt_irq_active;

  status = alt_irq_disable_all ();

  alt_irq_active |= (1 << id);
  NIOS2_WRITE_IENABLE (alt_irq_active);

  alt_irq_enable_all(status);

  return 0;
}

#ifndef ALT_EXCEPTION_STACK
/*
 * alt_irq_initerruptable() should only be called from within an ISR. It is used
 * to allow higer priority interrupts to interrupt the current ISR. The input
 * argument, "priority", is the priority, i.e. interrupt number of the current
 * interrupt.
 *
 * If this function is called, then the ISR is required to make a call to
 * alt_irq_non_interruptible() before returning. The input argument to
 * alt_irq_non_interruptible() is the return value from alt_irq_interruptible().
 *
 * Care should be taken when using this pair of functions, since they increasing
 * the system overhead associated with interrupt handling.
 *
 * If you are using an exception stack then nested interrupts won't work, so
 * these functions are not available in that case.
 */
static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_irq_interruptible (alt_u32 priority)
{
  extern volatile alt_u32 alt_priority_mask;
  extern volatile alt_u32 alt_irq_active;

  alt_u32 old_priority;

  old_priority      = alt_priority_mask;
  alt_priority_mask = (1 << priority) - 1;

  NIOS2_WRITE_IENABLE (alt_irq_active & alt_priority_mask);

  NIOS2_WRITE_STATUS (1);

  return old_priority; 
}

/*
 * See Comments above for alt_irq_interruptible() for an explanation of the use of this
 * function.
 */
static ALT_INLINE void ALT_ALWAYS_INLINE alt_irq_non_interruptible (alt_u32 mask)
{
  extern volatile alt_u32 alt_priority_mask;
  extern volatile alt_u32 alt_irq_active;

  NIOS2_WRITE_STATUS (0);  

  alt_priority_mask = mask;

  NIOS2_WRITE_IENABLE (mask & alt_irq_active);  
}
#endif /* ALT_EXCEPTION_STACK */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NIOS2_EIC_PRESENT */

#endif /* __ALT_LEGACY_IRQ_H__ */
