/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT7XX platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the RT7XX platforms.
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <soc.h>

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */
