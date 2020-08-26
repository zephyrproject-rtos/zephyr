/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTERRUPT_UTIL_H_
#define INTERRUPT_UTIL_H_

#include <ztest.h>

#define MS_TO_US(ms)  (ms * USEC_PER_MSEC)

#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>

static inline uint32_t get_available_nvic_line(uint32_t initial_offset)
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
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering. Return the NVIC line
					 * number.
					 */
					break;
				}
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
	|| defined(CONFIG_CPU_CORTEX_M0PLUS) || defined(CONFIG_CPU_CORTEX_M1)
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}

#elif defined(CONFIG_GIC)
#include <drivers/interrupt_controller/gic.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>

static inline void trigger_irq(int irq)
{
	printk("Triggering irq : %d\n", irq);

	/* Ensure that the specified IRQ number is a valid SGI interrupt ID */
	zassert_true(irq <= 15, "%u is not a valid SGI interrupt ID", irq);

	/*
	 * Generate a software generated interrupt and forward it to the
	 * requesting CPU.
	 */
#if CONFIG_GIC_VER <= 2
	sys_write32(GICD_SGIR_TGTFILT_REQONLY | GICD_SGIR_SGIINTID(irq),
		    GICD_SGIR);
#else
	gic_raise_sgi(irq, GET_MPIDR(), BIT(MPIDR_TO_CORE(GET_MPIDR())));
#endif
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
