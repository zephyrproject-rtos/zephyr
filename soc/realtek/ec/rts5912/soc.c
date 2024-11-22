/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com> / Titan Chen
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#if defined(CONFIG_RTS5912_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
	/* Returning false prevent device goes to sleep mode */
	return false;
}
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Apply device related preinit configuration */
}
