/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2835 SoC IRQ glue. With CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
 * selected the ARMv6 ISR wrapper calls the z_soc_irq_* hooks below
 * instead of a GIC driver. Each hook delegates to the BCM2835 ARMC
 * intc driver (drivers/interrupt_controller/intc_bcm2835_armctrl.c).
 *
 * Unlike the BCM2710 (Pi 2 / Pi 3 / Zero 2 W) which cascades the
 * ARMC under a BCM2836 ARM-local intc, the BCM2835 (Pi 1 / Pi Zero /
 * Pi Zero W) has only the ARMC peripheral aggregator at 0x2000b200,
 * wired directly to the ARM core IRQ line. The Zephyr IRQ space is
 * the single ARMC range 32..127; 0..31 is unused (no BCM2836
 * ARM-local intc).
 *
 * The intc driver maps its own MMIO via device_map() inside the init
 * helper called below. z_soc_irq_init is invoked from
 * z_arm_interrupt_init in z_prep_c, before z_cstart -- the MMU is up
 * but driver-model initialisation has not yet happened, so the init
 * helper is called directly rather than registered as SYS_INIT.
 *
 * Single-core only.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>

void z_soc_irq_init(void)
{
	bcm2835_armctrl_ic_init();
}

void z_soc_irq_enable(unsigned int irq)
{
	if (BCM283X_IRQ_IS_ARMC(irq)) {
		bcm2835_armctrl_ic_irq_enable(irq);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
	if (BCM283X_IRQ_IS_ARMC(irq)) {
		bcm2835_armctrl_ic_irq_disable(irq);
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (BCM283X_IRQ_IS_ARMC(irq)) {
		return bcm2835_armctrl_ic_irq_is_enabled(irq);
	}
	return 0;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	/* No per-IRQ priority register on the ARMC -- intentional no-op. */
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

unsigned int z_soc_irq_get_active(void)
{
	unsigned int irq = bcm2835_armctrl_ic_irq_get_active();

	/*
	 * The ARM ISR wrapper unmasks IRQs globally around the ISR call
	 * to support nested handlers. With a GIC that's fine -- ack-on-
	 * read prevents the same source from re-entering. The BCM2835
	 * ARMC has no such ack: a level-triggered source stays asserted
	 * until the peripheral's own state machine clears it, so
	 * unmasking globally would re-fire the same IRQ before its ISR
	 * has had a chance to run.
	 *
	 * Workaround: mask the source here, let the ISR run, then
	 * re-enable in z_soc_irq_eoi. Brackets the ISR with a per-source
	 * mask the way GIC's running-priority would.
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
	 * source is no longer asserted (normal case), or we're returning
	 * from a spurious dispatch and there's nothing to re-fire.
	 */
	if (irq < CONFIG_NUM_IRQS) {
		z_soc_irq_enable(irq);
	}
}
