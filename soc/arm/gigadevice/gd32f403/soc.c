/*
 * Copyright (c) 2021, ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GD32 MCU series initialization code
 *
 * This module provides routines to initialize and support board-level
 * hardware for the GigaDevice GD32 SoC.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int gigadevice_gd32_soc_init(void)
{
	uint32_t key;


	key = irq_lock();

	SystemInit();
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(gigadevice_gd32_soc_init, PRE_KERNEL_1, 0);
