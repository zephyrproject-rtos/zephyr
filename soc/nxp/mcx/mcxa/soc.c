/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxa platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxa platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	/* Release the I/O pads and certain peripherals to normal run mode state,
	 * for in Power Down mode they will be in a latched state.
	 */
	if ((CMC_GetSystemResetStatus(CMC) & kCMC_WakeUpReset) != 0UL) {
		SPC_ClearPeriphIOIsolationFlag(SPC0);
	}
#endif

	SystemInit();
}
#endif
