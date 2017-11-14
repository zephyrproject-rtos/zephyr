/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sys/alt_irq.h"
#include "altera_common.h"

#define ALTERA_MAX_IRQ		32

static alt_isr_func alt_hal_isr[ALTERA_MAX_IRQ];

#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
int alt_ic_isr_register(alt_u32 ic_id, alt_u32 irq,
		alt_isr_func isr, void *isr_context, void *flags)

{
  if (irq <= ALTERA_MAX_IRQ)
  {
    alt_hal_isr[irq] = isr;
    return 0;
  }
  else
  {
    return -EINVAL;
  }

}
#else
int alt_irq_register(alt_u32 irq, void* context, alt_isr_func isr)
{
  if (irq <= ALTERA_MAX_IRQ)
  {
    alt_hal_isr[irq] = isr;
    return 0;
  }
  else
  {
    return -EINVAL;
  }
}
#endif

void alt_handle_irq(void* base, alt_u32 id)
{
 /* Null pointer check for handler function */
 if (!alt_hal_isr[id])
	 return;

#ifdef ALT_ENHANCED_INTERRUPT_API_PRESENT
  alt_hal_isr[id]((void*)base);
#else
  alt_hal_isr[id]((void*)base, id);
#endif
}


/* Add Altera HAL extern function definations here */ 

/*
 * alt_tick() should only be called by the system clock driver. This is used
 * to notify the system that the system timer period has expired.
 */

void alt_tick (void)
{
}
