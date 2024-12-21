/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_sys_event.h>

#if CONFIG_SOC_SERIES_NRF54HX

/*
 * The 54HX is not yet supported by an nrfx driver nor the system controller so
 * we implement an ISR and concurrent access safe reference counting implementation
 * here using the nrfx hal.
 */

#include <hal/nrf_lrcconf.h>

static struct k_spinlock global_constlat_lock;
static uint16_t global_constlat_count;

int nrf_sys_event_request_global_constlat(void)
{
	K_SPINLOCK(&global_constlat_lock) {
		if (global_constlat_count == 0) {
#if CONFIG_SOC_NRF54H20_CPUAPP
			nrf_lrcconf_task_trigger(NRF_LRCCONF010,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
#elif CONFIG_SOC_NRF54H20_CPURAD
			nrf_lrcconf_task_trigger(NRF_LRCCONF000,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
			nrf_lrcconf_task_trigger(NRF_LRCCONF020,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
#else
#error "unsupported"
#endif
		}

		global_constlat_count++;
	}

	return 0;
}

int nrf_sys_event_release_global_constlat(void)
{
	K_SPINLOCK(&global_constlat_lock) {
		if (global_constlat_count == 1) {
#if CONFIG_SOC_NRF54H20_CPUAPP
			nrf_lrcconf_task_trigger(NRF_LRCCONF010,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
#elif CONFIG_SOC_NRF54H20_CPURAD
			nrf_lrcconf_task_trigger(NRF_LRCCONF000,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
			nrf_lrcconf_task_trigger(NRF_LRCCONF020,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
#else
#error "unsupported"
#endif
		}

		global_constlat_count--;
	}

	return 0;
}

#else

/*
 * The nrfx power driver already contains an ISR and concurrent access safe reference
 * counting API so we just use it directly when available.
 */

#include <nrfx_power.h>

int nrf_sys_event_request_global_constlat(void)
{
	nrfx_err_t err;

	err = nrfx_power_constlat_mode_request();

	return (err == NRFX_SUCCESS || err == NRFX_ERROR_ALREADY) ? 0 : -EAGAIN;
}

int nrf_sys_event_release_global_constlat(void)
{
	nrfx_err_t err;

	err = nrfx_power_constlat_mode_free();

	return (err == NRFX_SUCCESS || err == NRFX_ERROR_BUSY) ? 0 : -EAGAIN;
}

#endif
