/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Versatile/PB SoC support
 *
 * Interrupt control for the PL190 Vectored Interrupt Controller, used here in
 * its non-vectored mode: the ISR simply looks at VICIRQSTATUS and picks the
 * lowest numbered pending source. That is enough for an ARMv5 core, which has
 * no interrupt priorities of its own beyond the IRQ/FIQ split.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/arm/arm9/mmu.h>
#include <soc.h>

#define VIC_NODE DT_NODELABEL(vic)
#define VIC_BASE DT_REG_ADDR(VIC_NODE)

#define VIC_IRQSTATUS  (VIC_BASE + 0x00)
#define VIC_INTSELECT  (VIC_BASE + 0x0c)
#define VIC_INTENABLE  (VIC_BASE + 0x10)
#define VIC_INTENCLEAR (VIC_BASE + 0x14)
#define VIC_SOFTINT    (VIC_BASE + 0x18)
#define VIC_SOFTINTCLR (VIC_BASE + 0x1c)
#define VIC_PROTECTION (VIC_BASE + 0x20)

void z_soc_irq_init(void)
{
	/* Mask everything, route everything to IRQ (not FIQ). */
	sys_write32(0xffffffffU, VIC_INTENCLEAR);
	sys_write32(0xffffffffU, VIC_SOFTINTCLR);
	sys_write32(0x00000000U, VIC_INTSELECT);
	sys_write32(0x00000000U, VIC_PROTECTION);
}

void z_soc_irq_enable(unsigned int irq)
{
	sys_write32(BIT(irq), VIC_INTENABLE);
}

void z_soc_irq_disable(unsigned int irq)
{
	sys_write32(BIT(irq), VIC_INTENCLEAR);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	/*
	 * VICINTENABLE reads back the current mask, so a read is enough; there
	 * is no separate "enabled" register on the PL190.
	 */
	return (sys_read32(VIC_INTENABLE) & BIT(irq)) != 0;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	ARG_UNUSED(irq);
	ARG_UNUSED(prio);
	ARG_UNUSED(flags);

	/* The PL190 has no priority levels in non-vectored mode. */
}

unsigned int z_soc_irq_get_active(void)
{
	uint32_t status = sys_read32(VIC_IRQSTATUS);

	unsigned int irq;

	if (status == 0U) {
		return CONFIG_NUM_IRQS;
	}

	irq = (unsigned int)__builtin_ctz(status);

	/*
	 * A software interrupt stays asserted until it is cleared, so take it
	 * now, as the interrupt is taken. A trigger from the handler then
	 * raises the line again.
	 */
	sys_write32(BIT(irq), VIC_SOFTINTCLR);

	return irq;
}

void z_soc_irq_eoi(unsigned int irq)
{
	ARG_UNUSED(irq);

	/*
	 * The PL190 tracks the interrupt line level, so the peripheral's own
	 * interrupt clear is the end-of-interrupt. Nothing to do here.
	 */
}

void z_versatile_irq_trigger(unsigned int irq)
{
	sys_write32(BIT(irq), VIC_SOFTINT);
}

/*
 * Only the SDRAM is worth caching; everything else is left as the default
 * identity device mapping. The cached map runs from address 0, where the
 * SDRAM starts and the low exception vectors live, to the end of the image's
 * memory.
 */
#define SDRAM_NODE        DT_CHOSEN(zephyr_sram)
#define SDRAM_CACHED_SIZE (DT_REG_ADDR(SDRAM_NODE) + DT_REG_SIZE(SDRAM_NODE))

static const struct armv5_mmu_region versatile_mmu_regions[] = {
	ARMV5_MMU_REGION("sdram", 0x00000000U, SDRAM_CACHED_SIZE, ARMV5_MMU_MEM_CACHED),
};

const struct armv5_mmu_config soc_mmu_config = {
	.regions = versatile_mmu_regions,
	.num_regions = ARRAY_SIZE(versatile_mmu_regions),
};
