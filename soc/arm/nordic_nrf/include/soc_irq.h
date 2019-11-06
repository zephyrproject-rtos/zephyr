/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Nordic Semiconductor nRF91 family processors.
 */

#ifndef _SOC_IRQ_H_
#define _SOC_IRQ_H_

#ifdef CONFIG_DYNAMIC_INTERRUPTS
extern void nrf_sw_isr_direct(void);

/**
 * @brief Initialize a dynamic 'direct' interrupt handler.
 *
 * This routine initializes a dynamic interrupt handler for an IRQ. The IRQ
 * must be connected with irq_connect_dynamic and enabled via irq_enable()
 * before the interrupt handler begins servicing interrupts.
 *
 * These ISRs are designed for performance-critical interrupt handling and do
 * not go through common interrupt handling code. They must be implemented in
 * such a way that it is safe to put them directly in the software vector table.
 * For ISRs written in C, The ISR_DIRECT_DECLARE() macro will do this
 * automatically. For ISRs written in assembly it is entirely up to the
 * developer to ensure that the right steps are taken.
 *
 * This type of interrupt currently has a few limitations compared to normal
 * Zephyr interrupts:
 * - No parameters are passed to the ISR.
 * - No stack switch is done, the ISR will run on the interrupted context's
 *   stack, unless the architecture automatically does the stack switch in HW.
 * - Interrupt locking state is unchanged from how the HW sets it when the ISR
 *   runs. On arches that enter ISRs with interrupts locked, they will remain
 *   locked.
 * - Scheduling decisions are now optional, controlled by the return value of
 *   ISRs implemented with the ISR_DIRECT_DECLARE() macro
 * - The call into the OS to exit power management idle state is now optional.
 *   Normal interrupts always do this before the ISR is run, but when it runs
 *   is now controlled by the placement of a ISR_DIRECT_PM() macro, or omitted
 *   entirely.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param irq_p IRQ line number.
 * @param priority_p Interrupt priority.
 * @param flags_p Architecture-specific IRQ configuration flags.
 *
 * @return Interrupt vector assigned to this interrupt.
 */
#define NRF_IRQ_DIRECT_DYNAMIC(irq_p, priority_p, flags_p) \
	Z_ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, nrf_sw_isr_direct, flags_p)

#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#endif /* _SOC_IRQ_H_ */
