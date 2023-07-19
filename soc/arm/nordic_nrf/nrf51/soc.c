/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF51 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF51 family processor.
 */

#include <zephyr/kernel.h>
#include <hal/nrf_power.h>
#include <soc/nrfx_coredep.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#ifdef CONFIG_NRF_STORE_REBOOT_TYPE_GPREGRET
/* Overrides the weak ARM implementation:
 * Set general purpose retention register and reboot
 * This is deprecated and has been replaced with the boot mode retention
 * subsystem
 */
void sys_arch_reboot(int type)
{
	nrf_power_gpregret_set(NRF_POWER, (uint8_t)type);
	NVIC_SystemReset();
}
#endif

#define DELAY_CALL_OVERHEAD_US 2

void arch_busy_wait(uint32_t time_us)
{
	if (time_us <= DELAY_CALL_OVERHEAD_US) {
		return;
	}

	time_us -= DELAY_CALL_OVERHEAD_US;
	nrfx_coredep_delay_us(time_us);
}
