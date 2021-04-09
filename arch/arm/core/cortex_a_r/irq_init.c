/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-R interrupt initialization
 */

#include <arch/cpu.h>
#include <drivers/interrupt_controller/gic.h>

/**
 *
 * @brief Initialize interrupts
 *
 * @return N/A
 */
void z_arm_interrupt_init(void)
{
	/*
	 * Initialise interrupt controller.
	 */
#ifdef CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
	/* Invoke SoC-specific interrupt controller initialisation */
	z_soc_irq_init();
#endif
}
