#ifndef __ALT_IRQ_H__
#define __ALT_IRQ_H__

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
 * alt_irq.h is the Nios II specific implementation of the interrupt controller 
 * interface.
 *
 * Nios II includes optional support for an external interrupt controller. 
 * When an external controller is present, the "Enhanced" interrupt API
 * must be used to manage individual interrupts. The enhanced API also
 * supports the processor's internal interrupt controller. Certain API
 * members are accessible from either the "legacy" or "enhanced" interrpt
 * API. 
 *
 * Regardless of which API is in use, this file should be included by
 * application code and device drivers that register ISRs or manage interrpts.
 */
#include <errno.h>

#include "nios2.h"
#include "alt_types.h"
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Macros used by alt_irq_enabled
 */
#define ALT_IRQ_ENABLED  1
#define ALT_IRQ_DISABLED 0  

/* 
 * Number of available interrupts in internal interrupt controller.
 */
#define ALT_NIRQ NIOS2_NIRQ

/*
 * Used by alt_irq_disable_all() and alt_irq_enable_all().
 */
typedef int alt_irq_context;

/* ISR Prototype */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
typedef void (*alt_isr_func)(void* isr_context);
#else
typedef void (*alt_isr_func)(void* isr_context, alt_u32 id);
#endif

/*
 * The following protypes and routines are supported by both
 * the enhanced and legacy interrupt APIs
 */
 
/*
 * alt_irq_enabled can be called to determine if the processor's global
 * interrupt enable is asserted. The return value is zero if interrupts 
 * are disabled, and non-zero otherwise.
 *
 * Whether the internal or external interrupt controller is present, 
 * individual interrupts may still be disabled. Use the other API to query
 * a specific interrupt. 
 */
static ALT_INLINE int ALT_ALWAYS_INLINE alt_irq_enabled (void)
{
  int status;

  NIOS2_READ_STATUS (status);

  return status & NIOS2_STATUS_PIE_MSK; 
}

/*
 * alt_irq_disable_all() 
 *
 * This routine inhibits all interrupts by negating the status register PIE 
 * bit. It returns the previous contents of the CPU status register (IRQ 
 * context) which can be used to restore the status register PIE bit to its 
 * state before this routine was called.
 */
static ALT_INLINE alt_irq_context ALT_ALWAYS_INLINE 
       alt_irq_disable_all (void)
{
  alt_irq_context context;

  NIOS2_READ_STATUS (context);

  NIOS2_WRITE_STATUS (context & ~NIOS2_STATUS_PIE_MSK);
  
  return context;
}

/*
 * alt_irq_enable_all() 
 *
 * Enable all interrupts that were previously disabled by alt_irq_disable_all()
 *
 * This routine accepts a context to restore the CPU status register PIE bit
 * to the state prior to a call to alt_irq_disable_all().
 
 * In the case of nested calls to alt_irq_disable_all()/alt_irq_enable_all(), 
 * this means that alt_irq_enable_all() does not necessarily re-enable
 * interrupts.
 *
 * This routine will perform a read-modify-write sequence to restore only
 * status.PIE if the processor is configured with options that add additional 
 * writeable status register bits. These include the MMU, MPU, the enhanced 
 * interrupt controller port, and shadow registers. Otherwise, as a performance
 * enhancement, status is overwritten with the prior context. 
 */
static ALT_INLINE void ALT_ALWAYS_INLINE 
       alt_irq_enable_all (alt_irq_context context)
{
#if (NIOS2_NUM_OF_SHADOW_REG_SETS > 0) || (defined NIOS2_EIC_PRESENT) || \
    (defined NIOS2_MMU_PRESENT) || (defined NIOS2_MPU_PRESENT)
  alt_irq_context status;
  
  NIOS2_READ_STATUS (status);
  
  status &= ~NIOS2_STATUS_PIE_MSK;
  status |= (context & NIOS2_STATUS_PIE_MSK);
  
  NIOS2_WRITE_STATUS (status);
#else
  NIOS2_WRITE_STATUS (context);
#endif
}

/*
 * The function alt_irq_init() is defined within the auto-generated file
 * alt_sys_init.c. This function calls the initilization macros for all
 * interrupt controllers in the system at config time, before any other
 * non-interrupt controller driver is initialized.
 *
 * The "base" parameter is ignored and only present for backwards-compatibility.
 * It is recommended that NULL is passed in for the "base" parameter.
 */
extern void alt_irq_init (const void* base);

/*
 * alt_irq_cpu_enable_interrupts() enables the CPU to start taking interrupts.
 */
static ALT_INLINE void ALT_ALWAYS_INLINE 
       alt_irq_cpu_enable_interrupts (void)
{
    NIOS2_WRITE_STATUS(NIOS2_STATUS_PIE_MSK
#if defined(NIOS2_EIC_PRESENT) && (NIOS2_NUM_OF_SHADOW_REG_SETS > 0)
    | NIOS2_STATUS_RSIE_MSK
#endif      
      );
}


/*
 * Prototypes for the enhanced interrupt API.
 */
#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
/*
 * alt_ic_isr_register() can be used to register an interrupt handler. If the
 * function is succesful, then the requested interrupt will be enabled upon 
 * return.
 */
extern int alt_ic_isr_register(alt_u32 ic_id,
                        alt_u32 irq,
                        alt_isr_func isr,
                        void *isr_context,
                        void *flags);

/* 
 * alt_ic_irq_enable() and alt_ic_irq_disable() enable/disable a specific 
 * interrupt by using IRQ port and interrupt controller instance.
 */
int alt_ic_irq_enable (alt_u32 ic_id, alt_u32 irq);
int alt_ic_irq_disable(alt_u32 ic_id, alt_u32 irq);        

 /* 
 * alt_ic_irq_enabled() indicates whether a specific interrupt, as
 * specified by IRQ port and interrupt controller instance is enabled.
 */        
alt_u32 alt_ic_irq_enabled(alt_u32 ic_id, alt_u32 irq);

#else 
/*
 * Prototypes for the legacy interrupt API.
 */
#include "priv/alt_legacy_irq.h"
#endif 


/*
 * alt_irq_pending() returns a bit list of the current pending interrupts.
 * This is used by alt_irq_handler() to determine which registered interrupt
 * handlers should be called.
 *
 * This routine is only available for the Nios II internal interrupt
 * controller.
 */
#ifndef NIOS2_EIC_PRESENT
static ALT_INLINE alt_u32 ALT_ALWAYS_INLINE alt_irq_pending (void)
{
  alt_u32 active;

  NIOS2_READ_IPENDING (active);

  return active;
}
#endif 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_IRQ_H__ */
