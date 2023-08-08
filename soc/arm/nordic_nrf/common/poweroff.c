/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#else
#include <hal/nrf_regulators.h>
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
	nrf_power_system_off(NRF_POWER);
#else
	nrf_regulators_system_off(NRF_REGULATORS);
#endif

	CODE_UNREACHABLE;
}
