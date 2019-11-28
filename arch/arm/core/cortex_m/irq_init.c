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

#include <arch/cpu.h>
#include <arch/arm/cortex_m/cmsis.h>

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their priority set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 * @return N/A
 */

void z_arm_int_lib_init(void)
{
	int irq = 0;

	for (; irq < CONFIG_NUM_IRQS; irq++) {
		NVIC_SetPriority((IRQn_Type)irq, _IRQ_PRIO_OFFSET);
	}
}
