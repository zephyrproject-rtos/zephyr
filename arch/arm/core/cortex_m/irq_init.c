/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M interrupt initialization
 *
 */

#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their priority set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 */

void z_arm_interrupt_init(void)
{
	int irq = 0;

/* CONFIG_2ND_LVL_ISR_TBL_OFFSET could be treated as total number of level1 interrupts */
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && defined(CONFIG_2ND_LVL_ISR_TBL_OFFSET)
	for (; irq < CONFIG_2ND_LVL_ISR_TBL_OFFSET; irq++) {
#else
	for (; irq < CONFIG_NUM_IRQS; irq++) {
#endif
		NVIC_SetPriority((IRQn_Type)irq, _IRQ_PRIO_OFFSET);
	}
}
