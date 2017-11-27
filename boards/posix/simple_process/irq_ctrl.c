/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "posix_soc.h"
#include "irq_ctrl.h"

/********************************
 * Trivial IRQ controller model *
 ********************************/

static uint64_t irq_status;
/**
 * HW IRQ controller model provided by this board
 * (just throws the interrupt to the CPU)
 */
void hw_irq_controller(irq_type_t irq)
{
	/*No interrupt masking or any other fancy feature*/

	irq_status |= 1 << irq;
	ps_interrupt_raised();
}

/**
 * Function for SW to clear interrupts in this interrupt controller
 */
void hw_irq_controller_clear_irqs(void)
{
	irq_status = 0;
}


/**
 * Function for SW to get the status in the interrupt controller
 */
uint64_t hw_irq_controller_get_irq_status(void)
{
	return irq_status;
}
/*
 * End of IRQ controller
 */

