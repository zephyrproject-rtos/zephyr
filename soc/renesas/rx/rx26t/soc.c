/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief System/hardware module for RX SOC family
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <soc.h>

#include "platform.h"
#include "r_bsp_cpu.h"

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
#ifdef CONFIG_HAS_RENESAS_RX_RDP
	bsp_ram_initialize();
	bsp_interrupt_open();
	bsp_register_protect_open();
#if CONFIG_RENESAS_NONE_USED_PORT_INIT == 1
	/*
	 * This is the function that initializes the unused port.
	 * Please see datails on this in the "Handling of Unused Pins" section of PORT chapter
	 *  of RX MCU of User's manual.
	 * And please MUST set "BSP_PACKAGE_PINS" definition to your device of pin type in
	 * r_bsp_config.h Otherwise, the port may output without intention.
	 */
	bsp_non_existent_port_init();

#endif /* CONFIG_RENESAS_NONE_USED_PORT_INIT */
#endif /* CONFIG_HAS_RENESAS_RX_RDP */
}
