/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTERRUPT_UTIL_H_
#define INTERRUPT_UTIL_H_

#include <ztest.h>

#define MS_TO_US(ms)  (K_MSEC(ms) * USEC_PER_MSEC)

#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>

static inline u32_t get_available_nvic_line(u32_t initial_offset)
{
	int i;

	for (i = initial_offset - 1; i >= 0; i--) {

		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/*
				 * If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line and return the NVIC line number.
				 */
				NVIC_ClearPendingIRQ(i);
				break;
			}
		}
	}

	zassert_true(i >= 0, "No available IRQ line\n");

	return i;
}

static inline void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU) || defined(CONFIG_CPU_CORTEX_M0) \
	|| defined(CONFIG_CPU_CORTEX_M0PLUS)
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}

#elif defined(CONFIG_GIC)
#include <drivers/interrupt_controller/gic.h>

static inline void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);

	/* Ensure that the specified IRQ number is a valid SGI interrupt ID */
	zassert_true(irq <= 15, "%u is not a valid SGI interrupt ID", irq);

	/*
	 * Generate a software generated interrupt and forward it to the
	 * requesting CPU.
	 */
	sys_write32(GICD_SGIR_TGTFILT_REQONLY | GICD_SGIR_SGIINTID(irq),
		    GICD_SGIR);
}

#elif defined(CONFIG_CPU_ARCV2)
static inline void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, irq);
}
#else
/* for not supported architecture */
#define NO_TRIGGER_FROM_SW
#endif

#endif /* INTERRUPT_UTIL_H_ */
