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
 * Called from the SoC's z_soc_irq_* hooks via the bcm2836_l1_intc_*
 * entry points in <zephyr/drivers/interrupt_controller/intc_bcm283x.h>.
 * Register banks are per-core; each CPU addresses its own bank through
 * MPIDR (this_core() below). In SMP builds mailbox 0 carries the
 * inter-processor interrupts.
 */

#define DT_DRV_COMPAT brcm_bcm2836_l1_intc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>
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

/* Mailbox-0 offsets: write-set and read / write-1-to-clear, per core. */
#define L1_MBOX0_SET_OFF(c)   (0x80 + (c) * 0x10)
#define L1_MBOX0_RDCLR_OFF(c) (0xc0 + (c) * 0x10)

/* Current physical core id from MPIDR (flat single-cluster A53). */
static inline unsigned int this_core(void)
{
	uint64_t mpidr;

	__asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
	return (unsigned int)(mpidr & 0xffU);
}

static mm_reg_t l1_base;

void bcm2836_l1_intc_init(void)
{
	/*
	 * Map the L1 intc register bank. This is invoked from
	 * z_soc_irq_init() in soc_irq.c, which runs from
	 * z_arm64_interrupt_init() in z_prep_c -- after the MMU is up
	 * but before any SYS_INIT priority. We can't rely on a normal
	 * driver .init callback here. With CONFIG_KERNEL_DIRECT_MAP=y
	 * (selected on BCM2710) device_map() identity-maps so the
	 * physical address from the DT remains valid as a virtual one.
	 */
	device_map(&l1_base, DT_INST_REG_ADDR(0), DT_INST_REG_SIZE(0),
		   K_MEM_CACHE_NONE);

	/*
	 * Mask all locally-routed sources for the boot core. The GPU
	 * routing register defaults to core 0 after reset; force it so
	 * the value is deterministic across firmware variants.
	 */
	sys_write32(0, l1_base + L1_TIMER_INT_CTRL_OFF(this_core()));
	sys_write32(0, l1_base + L1_MBOX_INT_CTRL_OFF(this_core()));
	sys_write32(0, l1_base + L1_GPU_INT_ROUTING_OFF);
}

void bcm2836_l1_intc_irq_enable(unsigned int irq)
{
	if (irq <= 3) {
		mm_reg_t reg = l1_base + L1_TIMER_INT_CTRL_OFF(this_core());

		sys_write32(sys_read32(reg) | BIT(irq), reg);
	} else if (irq <= 7) {
		mm_reg_t reg = l1_base + L1_MBOX_INT_CTRL_OFF(this_core());

		sys_write32(sys_read32(reg) | BIT(irq - 4), reg);
	} else if (irq == BCM2836_L1_IRQ_PMU_BIT) {
		sys_write32(BIT(this_core()), l1_base + L1_PMU_ROUTING_SET_OFF);
	}
	/*
	 * IRQ 8 (GPU cascade) is implicit: the L1 routing register
	 * already steers it to a core and no per-IRQ enable bit exists.
	 */
}

void bcm2836_l1_intc_irq_disable(unsigned int irq)
{
	if (irq <= 3) {
		mm_reg_t reg = l1_base + L1_TIMER_INT_CTRL_OFF(this_core());

		sys_write32(sys_read32(reg) & ~BIT(irq), reg);
	} else if (irq <= 7) {
		mm_reg_t reg = l1_base + L1_MBOX_INT_CTRL_OFF(this_core());

		sys_write32(sys_read32(reg) & ~BIT(irq - 4), reg);
	} else if (irq == BCM2836_L1_IRQ_PMU_BIT) {
		sys_write32(BIT(this_core()), l1_base + L1_PMU_ROUTING_CLR_OFF);
	}
}

int bcm2836_l1_intc_irq_is_enabled(unsigned int irq)
{
	if (irq <= 3) {
		return !!(sys_read32(l1_base + L1_TIMER_INT_CTRL_OFF(this_core())) & BIT(irq));
	}
	if (irq <= 7) {
		return !!(sys_read32(l1_base + L1_MBOX_INT_CTRL_OFF(this_core())) & BIT(irq - 4));
	}
	if (irq == BCM2836_L1_IRQ_GPU_BIT) {
		/* No mask for the cascade -- always live. */
		return 1;
	}
	/* PMU has no readback path; report disabled conservatively. */
	return 0;
}

unsigned int bcm2836_l1_intc_irq_get_active(void)
{
	uint32_t src = sys_read32(l1_base + L1_IRQ_SOURCE_OFF(this_core()));

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

#ifdef CONFIG_SMP
void bcm2836_l1_intc_mbox0_raise(unsigned int core, uint32_t bits)
{
	sys_write32(bits, l1_base + L1_MBOX0_SET_OFF(core));
}

uint32_t bcm2836_l1_intc_mbox0_read_ack(void)
{
	/*
	 * Read the pending mailbox-0 bits for this core, then write the
	 * same value back to ack ONLY those bits. A concurrent raise
	 * landing between the read and the write is preserved: its bit
	 * stays set, the mailbox source stays asserted, and the
	 * level-triggered IRQ refires once the SoC's eoi hook re-enables
	 * the source. Blanket-clearing 0xffffffff would drop such a
	 * raise on the floor. Same convention as Linux's
	 * drivers/irqchip/irq-bcm2836.c (handle_ipi reads MAILBOX0_CLR,
	 * ipi_ack writes the seen bits back).
	 */
	mm_reg_t reg = l1_base + L1_MBOX0_RDCLR_OFF(this_core());
	uint32_t bits = sys_read32(reg);

	if (bits != 0U) {
		sys_write32(bits, reg);
	}
	return bits;
}
#endif /* CONFIG_SMP */
