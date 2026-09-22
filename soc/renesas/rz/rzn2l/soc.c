/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/N2L Group
 */

#include <bsp_api.h>
#include <zephyr/init.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/irq.h>

extern void bsp_global_system_counter_init(void);

void *gp_renesas_isr_context[BSP_ICU_VECTOR_MAX_ENTRIES + BSP_CORTEX_VECTOR_TABLE_ENTRIES];
IRQn_Type g_current_interrupt_num[32];
uint8_t g_current_interrupt_pointer;

void soc_reset_hook(void)
{
	/* Enable peripheral port access at EL1 and EL0 */
	__asm__ volatile("mrc p15, 0, r0, c15, c0, 0\n");
	__asm__ volatile("orr r0, #1\n");
	__asm__ volatile("mcr p15, 0, r0, c15, c0, 0\n");
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
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
