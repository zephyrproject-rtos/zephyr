/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Cortex-A interrupt initialisation
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

/**
 * @brief Initialise interrupts
 *
 * This function invokes the ARM Generic Interrupt Controller (GIC) driver to
 * initialise the interrupt system on the SoCs that use the GIC as the primary
 * interrupt controller.
 *
 * When a custom interrupt controller is used, however, the SoC layer function
 * is invoked for SoC-specific interrupt system initialisation.
 */
void z_arm64_interrupt_init(void)
{
#ifdef CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
	/* Invoke SoC-specific interrupt controller initialisation */
	z_soc_irq_init();
#endif
}
