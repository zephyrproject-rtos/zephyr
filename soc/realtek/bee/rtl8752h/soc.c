/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <clock_manager.h>
#include <system_rtl876x.h>
#include <rtl_boot_record.h>

#include "utils.h"

void soc_early_reset_hook(void)
{
	rtl_boot_stage_record(START_PLATFORM_INIT);

	/* enable cache */
	share_cache_ram();

	/* Initialize cpu clock source and apply calibrations */
	set_active_mode_clk_src();

	/* Initialize 32 kHz clock source */
	set_up_32k_clk_src();

	rtl_boot_stage_record(AON_BOOT_DONE);

	/* enable clocks, vendor regs, RAM power control, TRNG */
	hal_setup_hardware();

	hal_setup_cpu();
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	platform_delay_us(usec_to_wait);
}
#endif
