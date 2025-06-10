/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/V2H Group
 */

#include <bsp_api.h>
#include <zephyr/init.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>

/* System core clock is set to 800 MHz after reset */
uint32_t SystemCoreClock BSP_SECTION_EARLY_INIT;
void *gp_renesas_isr_context[BSP_ICU_VECTOR_MAX_ENTRIES + BSP_CORTEX_VECTOR_TABLE_ENTRIES];
uint16_t g_current_interrupt_num[32];
uint8_t g_current_interrupt_pointer;

void soc_early_init_hook(void)
{
	bsp_clock_init();

	/* Initialize SystemCoreClock variable. */
	SystemCoreClockUpdate();

	/* Enable caches */
	sys_cache_instr_enable();
	sys_cache_data_enable();
}

unsigned int z_soc_irq_get_active(void)
{
	int intid = arm_gic_get_active();

	if (intid != 1023) {
		g_current_interrupt_num[g_current_interrupt_pointer++] = intid;
	}

	return intid;
}

void z_soc_irq_eoi(unsigned int intid)
{
	if (intid != 1023) {
		g_current_interrupt_pointer--;
	}
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
