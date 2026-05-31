/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2835 SoC IRQ glue. With CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
 * selected the ARM ISR wrapper calls the z_soc_irq_* hooks below
 * instead of a GIC driver. Each hook delegates to the BCM2835 ARMC
 * intc driver (drivers/interrupt_controller/intc_bcm2835_armctrl.c).
 *
 * Unlike the BCM2710, which cascades the ARMC under a BCM2836
 * ARM-local intc, the BCM2835 has only the ARMC peripheral aggregator
 * at 0x2000b200, wired directly to the ARM core IRQ line. The Zephyr
 * IRQ space is the ARMC range 32..127; 0..31 is unused.
 *
 * The intc driver maps its own MMIO via device_map() inside the init
 * helper called below. z_soc_irq_init is invoked from
 * z_arm_interrupt_init in z_prep_c, before z_cstart: the MMU is up
 * but driver model initialisation has not yet happened, so the init
 * helper is called directly rather than registered as SYS_INIT.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/*
 * Sources masked by z_soc_irq_get_active() for the duration of their ISR
 * (see the comment there). The record lets z_soc_irq_eoi() tell its own
 * mask apart from a disable the ISR performed itself, so an ISR that
 * disables its source to defer work to a thread stays disabled.
 */
static atomic_t bracketed_irqs[ATOMIC_BITMAP_SIZE(CONFIG_NUM_IRQS)];

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
	if (irq < CONFIG_NUM_IRQS) {
		/* Whoever disables the source now owns its masked state. */
		atomic_clear_bit(bracketed_irqs, irq);
	}

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
	/* No per-IRQ priority register on the ARMC, intentional no-op. */
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

unsigned int z_soc_irq_get_active(void)
{
	unsigned int irq = bcm2835_armctrl_ic_irq_get_active();

	/*
	 * The ISR wrapper may unmask IRQs around the ISR call to support
	 * nested handlers. With a GIC that is fine, since ack-on-read
	 * stops the same source from re-entering. The ARMC has no such
	 * ack: a level-triggered source stays asserted until the
	 * peripheral clears it, so it would re-fire before its ISR ran.
	 *
	 * Mask the source here, let the ISR run, then re-enable it in
	 * z_soc_irq_eoi, the way a GIC's running priority would.
	 */
	if (irq < CONFIG_NUM_IRQS) {
		z_soc_irq_disable(irq);
		atomic_set_bit(bracketed_irqs, irq);
	}
	return irq;
}

void z_soc_irq_eoi(unsigned int irq)
{
	/*
	 * Re-enable the source masked by z_soc_irq_get_active. If the ISR
	 * disabled the source itself the record is gone and the source is
	 * left masked, as the ISR intended.
	 */
	if (irq < CONFIG_NUM_IRQS && atomic_test_and_clear_bit(bracketed_irqs, irq)) {
		z_soc_irq_enable(irq);
	}
}
