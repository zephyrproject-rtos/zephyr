/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 interrupt initialization
 */

#include <zephyr/arch/cpu.h>

/**
 * @brief Initialize interrupts
 *
 * ARMv5 cores have no architectural interrupt controller, so this always
 * defers to the SoC.
 */
void z_arm_interrupt_init(void)
{
	z_soc_irq_init();
}
