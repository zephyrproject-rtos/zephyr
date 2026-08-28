/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2836 ARM-local interrupt controller (also present on BCM2710 /
 * Pi Zero 2 W). Drives per-core ARM generic timer IRQs, four mailbox
 * IRQs per core (used as IPIs in SMP-capable kernels), the per-core
 * PMU IRQ, and the cascaded GPU IRQ that aggregates everything
 * pending on the BCM2835 ARMC peripheral controller.
 *
 * This is the platform's root interrupt controller: the architecture
 * interrupt control functions map onto it, and it walks the GPU cascade
 * into the ARMC on acknowledge. Single-core only (CORE_ID 0) until SMP
 * arrives.
 */

#define DT_DRV_COMPAT brcm_bcm2836_l1_intc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/intc_root.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/* Register offsets from the L1 intc base. */
#define L1_GPU_INT_ROUTING_OFF   0x0c
#define L1_PMU_ROUTING_SET_OFF   0x10
#define L1_PMU_ROUTING_CLR_OFF   0x14
#define L1_TIMER_INT_CTRL_OFF(c) (0x40 + (c) * 4)
#define L1_MBOX_INT_CTRL_OFF(c)  (0x50 + (c) * 4)
#define L1_IRQ_SOURCE_OFF(c)     (0x60 + (c) * 4)

/* L1 IRQ_SOURCE bit masks. */
#define L1_SRC_TIMER_MASK 0x000fU /* bits 0..3 */
#define L1_SRC_MBOX_MASK  0x00f0U /* bits 4..7 */

/* TODO: revisit when SMP arrives. */
#define CORE_ID 0

static mm_reg_t l1_base;

static int intc_bcm2836_l1_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * Map the L1 intc register bank. With CONFIG_KERNEL_DIRECT_MAP=y
	 * (selected on BCM2710) device_map() identity-maps so the
	 * physical address from the DT remains valid as a virtual one.
	 */
	device_map(&l1_base, DT_INST_REG_ADDR(0), DT_INST_REG_SIZE(0),
		   K_MEM_CACHE_NONE);

	/*
	 * Mask all locally-routed sources for core 0. The GPU routing
	 * register defaults to core 0 after reset; force it so the
	 * value is deterministic across firmware variants.
	 */
	sys_write32(0, l1_base + L1_TIMER_INT_CTRL_OFF(CORE_ID));
	sys_write32(0, l1_base + L1_MBOX_INT_CTRL_OFF(CORE_ID));
	sys_write32(0, l1_base + L1_GPU_INT_ROUTING_OFF);

	return 0;
}

static void l1_irq_enable(unsigned int irq)
{
	if (irq <= 3) {
		mm_reg_t reg = l1_base + L1_TIMER_INT_CTRL_OFF(CORE_ID);

		sys_write32(sys_read32(reg) | BIT(irq), reg);
	} else if (irq <= 7) {
		mm_reg_t reg = l1_base + L1_MBOX_INT_CTRL_OFF(CORE_ID);

		sys_write32(sys_read32(reg) | BIT(irq - 4), reg);
	} else if (irq == BCM2836_L1_IRQ_PMU_BIT) {
		sys_write32(BIT(CORE_ID), l1_base + L1_PMU_ROUTING_SET_OFF);
	}
	/*
	 * IRQ 8 (GPU cascade) is implicit: the L1 routing register
	 * already steers it to CORE_ID and no per-IRQ enable bit exists.
	 */
}

static void l1_irq_disable(unsigned int irq)
{
	if (irq <= 3) {
		mm_reg_t reg = l1_base + L1_TIMER_INT_CTRL_OFF(CORE_ID);

		sys_write32(sys_read32(reg) & ~BIT(irq), reg);
	} else if (irq <= 7) {
		mm_reg_t reg = l1_base + L1_MBOX_INT_CTRL_OFF(CORE_ID);

		sys_write32(sys_read32(reg) & ~BIT(irq - 4), reg);
	} else if (irq == BCM2836_L1_IRQ_PMU_BIT) {
		sys_write32(BIT(CORE_ID), l1_base + L1_PMU_ROUTING_CLR_OFF);
	}
}

static int l1_irq_is_enabled(unsigned int irq)
{
	if (irq <= 3) {
		return !!(sys_read32(l1_base + L1_TIMER_INT_CTRL_OFF(CORE_ID)) & BIT(irq));
	}
	if (irq <= 7) {
		return !!(sys_read32(l1_base + L1_MBOX_INT_CTRL_OFF(CORE_ID)) & BIT(irq - 4));
	}
	if (irq == BCM2836_L1_IRQ_GPU_BIT) {
		/* No mask for the cascade -- always live. */
		return 1;
	}
	/* PMU has no readback path; report disabled conservatively. */
	return 0;
}

static unsigned int l1_irq_get_active(void)
{
	uint32_t src = sys_read32(l1_base + L1_IRQ_SOURCE_OFF(CORE_ID));

	if (src == 0) {
		return CONFIG_NUM_IRQS;
	}
	if (src & L1_SRC_TIMER_MASK) {
		return __builtin_ctz(src & L1_SRC_TIMER_MASK); /* 0..3 */
	}
	if (src & L1_SRC_MBOX_MASK) {
		return __builtin_ctz(src & L1_SRC_MBOX_MASK);  /* 4..7 */
	}
	if (src & BIT(BCM2836_L1_IRQ_PMU_BIT)) {
		return BCM2836_L1_IRQ_PMU_BIT;
	}
	if (src & BIT(BCM2836_L1_IRQ_GPU_BIT)) {
		return BCM2836_L1_IRQ_GPU_BIT;
	}
	return CONFIG_NUM_IRQS;
}

/*
 * The L1 controller is the platform's root interrupt controller: it owns the
 * flat BCM283x interrupt space, controlling its own lines directly and the
 * peripheral lines through the ARMC aggregator behind the GPU cascade.
 */
void intc_root_enable(unsigned int irq)
{
	if (BCM283X_IRQ_IS_L1(irq)) {
		l1_irq_enable(irq);
	} else if (BCM283X_IRQ_IS_ARMC(irq)) {
		intc_bcm2835_armctrl_irq_enable(irq);
	}
}

/* Sources masked at acknowledge and unmasked at end of interrupt */
static ATOMIC_DEFINE(bracketed_irqs, CONFIG_NUM_IRQS);

void intc_root_disable(unsigned int irq)
{
	if (irq < CONFIG_NUM_IRQS) {
		/* Whoever disables the source now owns its masked state */
		atomic_clear_bit(bracketed_irqs, irq);
	}

	if (BCM283X_IRQ_IS_L1(irq)) {
		l1_irq_disable(irq);
	} else if (BCM283X_IRQ_IS_ARMC(irq)) {
		intc_bcm2835_armctrl_irq_disable(irq);
	}
}

int intc_root_is_enabled(unsigned int irq)
{
	if (BCM283X_IRQ_IS_L1(irq)) {
		return l1_irq_is_enabled(irq);
	}
	if (BCM283X_IRQ_IS_ARMC(irq)) {
		return intc_bcm2835_armctrl_irq_is_enabled(irq);
	}
	return 0;
}

void intc_root_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/* No per-IRQ priority register on either controller */
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);
}

unsigned int intc_root_get_active(void)
{
	unsigned int irq = l1_irq_get_active();

	if (irq == BCM2836_L1_IRQ_GPU_BIT) {
		/* GPU cascade: walk down into the ARMC */
		irq = intc_bcm2835_armctrl_irq_get_active();
	}

	/*
	 * Neither controller has an acknowledge or end-of-interrupt: a level
	 * source stays asserted until its handler services it. Mask it here
	 * and unmask it at end of interrupt so nested handlers do not re-enter
	 * on the same source.
	 */
	if (irq < CONFIG_NUM_IRQS) {
		intc_root_disable(irq);
		atomic_set_bit(bracketed_irqs, irq);
	}
	return irq;
}

void intc_root_eoi(unsigned int irq)
{
	/* Unmask only if the handler did not disable the source itself */
	if (irq < CONFIG_NUM_IRQS && atomic_test_and_clear_bit(bracketed_irqs, irq)) {
		intc_root_enable(irq);
	}
}

DEVICE_DT_INST_DEFINE(0, intc_bcm2836_l1_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
