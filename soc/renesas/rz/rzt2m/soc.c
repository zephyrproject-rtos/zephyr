/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * Copyright (c) 2025-2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bsp_api.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/irq.h>

extern void bsp_global_system_counter_init(void);

void *gp_renesas_isr_context[BSP_ICU_VECTOR_MAX_ENTRIES + BSP_CORTEX_VECTOR_TABLE_ENTRIES];
IRQn_Type g_current_interrupt_num[32];
uint8_t g_current_interrupt_pointer;

/* IMP_BTCMREGIONR fields */
#define BTCMREGIONR_BASEADDRESS_MASK GENMASK(31, 13)
#define BTCMREGIONR_ENABLEEL2        BIT(1)
#define BTCMREGIONR_ENABLEEL10       BIT(0)

#define BTCMREGIONR_BASEADDRESS DT_REG_ADDR(DT_NODELABEL(cpu0_btcm))

#define BTCMREGIONR_ENABLE                                                                         \
	(BTCMREGIONR_BASEADDRESS | BTCMREGIONR_ENABLEEL10 | BTCMREGIONR_ENABLEEL2)

BUILD_ASSERT((BTCMREGIONR_BASEADDRESS & ~BTCMREGIONR_BASEADDRESS_MASK) == 0,
	     "BTCM base address does not fit IMP_BTCMREGIONR.BASEADDRESS");

void soc_reset_init(void);

/*
 * Called from z_arm_reset() before it enters the kernel. A CPU0 software reset
 * restarts the core from the reset vector without the boot ROM, which leaves the
 * BTCM disabled: enable it before the kernel text and the interrupt, main and
 * idle stacks in it are used.
 */
__attribute__((naked)) void soc_reset_hook(void)
{
	__asm__ volatile(
		"mrc p15, 0, r0, c9, c1, 1\n"
		"ldr r1, =%[mask]\n"
		"bic r0, r0, r1\n"
		"ldr r1, =%[enable]\n"
		"orr r0, r0, r1\n"
		"dsb\n"
		"mcr p15, 0, r0, c9, c1, 1\n"
		"isb\n"
		"b soc_reset_init\n"
		:
		: [mask] "i"(BTCMREGIONR_BASEADDRESS_MASK), [enable] "i"(BTCMREGIONR_ENABLE));
}

void soc_reset_init(void)
{
	/* Enable peripheral port access at EL1 and EL0 */
	__asm__ volatile("mrc p15, 0, r0, c15, c0, 0\n");
	__asm__ volatile("orr r0, #1\n");
	__asm__ volatile("mcr p15, 0, r0, c15, c0, 0\n");
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Enable i-cache and d-cache */
	sys_cache_instr_enable();
	sys_cache_data_enable();
}

void soc_early_init_hook(void)
{
	/* Configure system clocks. */
	bsp_clock_init();

	/* Initialize SystemCoreClock variable. */
	SystemCoreClockUpdate();

	/* Initialize global system counter. The counter is enabled and is incrementing. */
	bsp_global_system_counter_init();
}

unsigned int z_soc_irq_get_active(void)
{
	int intid = arm_gic_get_active();

	g_current_interrupt_num[g_current_interrupt_pointer++] = intid;

	return intid;
}

void z_soc_irq_eoi(unsigned int intid)
{
	g_current_interrupt_pointer--;
	arm_gic_eoi(intid);
}

void z_soc_irq_enable(unsigned int irq)
{
	arm_gic_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	arm_gic_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return arm_gic_irq_is_enabled(irq);
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	arm_gic_irq_set_priority(irq, prio, flags);
}

void z_soc_irq_init(void)
{
	g_current_interrupt_pointer = 0;
}

/* Porting FSP IRQ configuration by an empty function */
/* Let Zephyr handle IRQ configuration */
void bsp_irq_core_cfg(void)
{
	/* Do nothing */
}
