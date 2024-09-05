/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#include <power.h>
#else
#include <hal/nrf_regulators.h>
#endif
#if defined(CONFIG_SOC_SERIES_NRF54LX)
#include <helpers/nrfx_reset_reason.h>
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_SOC_SERIES_NRF54LX)
	nrfx_reset_reason_clear(UINT32_MAX);
#endif
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
	nrf_power_system_off(NRF_POWER);
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
	nrf_poweroff();
#else
	nrf_regulators_system_off(NRF_REGULATORS);
#endif

	CODE_UNREACHABLE;
}
