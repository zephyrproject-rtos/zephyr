/*
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Raspberry Pi RP235xx MCUs
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Raspberry Pi RP235xx (RP2350A, RP2350B, RP2354A, RP2354B).
 */

#if CONFIG_SOC_RESET_HOOK
#include <pico/runtime_init.h>
#if CONFIG_RISCV
#include <hardware/riscv_platform_timer.h>
#endif

void soc_reset_hook(void)
{
#if CONFIG_RISCV
	/* The kernel requires the mtimer to tick at the CPU frequency. */
	riscv_timer_set_fullspeed(true);
#else
	runtime_init_per_core_enable_coprocessors();
#endif
}

#endif /* CONFIG_SOC_RESET_HOOK */
