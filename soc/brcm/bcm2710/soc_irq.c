/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2710 SoC IRQ glue. With CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
 * selected the arm64 ISR wrapper calls the z_soc_irq_* hooks below
 * instead of a GIC driver. Each hook delegates to the BCM283x intc
 * driver (drivers/interrupt_controller/) responsible for the IRQ
 * range:
 *
 *     0..9     BCM2836 ARM-local intc (per-core timers, mailboxes,
 *              the GPU cascade hidden at IRQ 8, and the PMU)
 *     32..127  BCM2835 ARMC peripheral aggregator (basic bank +
 *              GPU bank 1/2), cascaded under L1 IRQ 8
 *
 * IRQ 8 is never registered with an ISR. When it fires, the cascade
 * is invisible to the rest of Zephyr: z_soc_irq_get_active sees the
 * GPU bit on the L1 controller and then walks the ARMC pending
 * registers to return the actual 32..127 peripheral IRQ.
 *
 * Single-core only (CORE_ID 0). SMP would extend this to per-core
 * mailbox IPIs and dynamic GPU-IRQ routing.
 *
 * The intc drivers map their own MMIO via device_map() inside the
 * init helpers called below. Both run before any SYS_INIT priority
 * (z_soc_irq_init is invoked from z_arm64_interrupt_init in
 * z_prep_c, before z_cstart) -- the MMU is up, but driver model
 * initialisation has not yet happened, so the init helpers are
 * called directly rather than registered as SYS_INIT entries.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>

void z_soc_irq_init(void)
{
	bcm2836_l1_intc_init();
	bcm2835_armctrl_ic_init();
}

void z_soc_irq_enable(unsigned int irq)
{
	if (BCM283X_IRQ_IS_L1(irq)) {
		bcm2836_l1_intc_irq_enable(irq);
	} else if (BCM283X_IRQ_IS_ARMC(irq)) {
		bcm2835_armctrl_ic_irq_enable(irq);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
	if (BCM283X_IRQ_IS_L1(irq)) {
		bcm2836_l1_intc_irq_disable(irq);
	} else if (BCM283X_IRQ_IS_ARMC(irq)) {
		bcm2835_armctrl_ic_irq_disable(irq);
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (BCM283X_IRQ_IS_L1(irq)) {
		return bcm2836_l1_intc_irq_is_enabled(irq);
	}
	if (BCM283X_IRQ_IS_ARMC(irq)) {
		return bcm2835_armctrl_ic_irq_is_enabled(irq);
	}
	return 0;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	/* No per-IRQ priority register on either intc -- intentional no-op. */
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

unsigned int z_soc_irq_get_active(void)
{
	unsigned int irq = bcm2836_l1_intc_irq_get_active();

	if (irq == BCM2836_L1_IRQ_GPU_BIT) {
		/* GPU cascade: walk down into the ARMC. ISRs are never
		 * registered at index 8, so the cascade is invisible to
		 * the rest of Zephyr.
		 */
		irq = bcm2835_armctrl_ic_irq_get_active();
	}

	/*
	 * The arm64 isr_wrapper unmasks IRQs globally (daifclr) around
	 * the ISR call to support nested handlers. With a GIC that's
	 * fine -- ack-on-read prevents the same source from re-entering
	 * the handler. The BCM2835/2836 intc pair has no such ack: a
	 * level-triggered source stays asserted until the peripheral's
	 * own state machine clears it, so unmasking globally would
	 * re-fire the same IRQ before its ISR has had a chance to run.
	 *
	 * Workaround: mask the source here, let the ISR run, then
	 * re-enable in z_soc_irq_eoi. Brackets the ISR with a
	 * per-source mask the way GIC's running-priority would.
	 */
	if (irq < CONFIG_NUM_IRQS) {
		z_soc_irq_disable(irq);
	}
	return irq;
}

void z_soc_irq_eoi(unsigned int irq)
{
	/*
	 * Re-enable the source masked by z_soc_irq_get_active. By now
	 * either the ISR cleared the underlying peripheral and the
	 * source is no longer asserted (normal case), or we're
	 * returning from a spurious dispatch and there's nothing to
	 * re-fire.
	 */
	if (irq < CONFIG_NUM_IRQS) {
		z_soc_irq_enable(irq);
	}
}
