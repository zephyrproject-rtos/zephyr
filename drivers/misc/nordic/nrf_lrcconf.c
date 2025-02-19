/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/event_device.h>

#include <hal/nrf_lrcconf.h>

#define DT_DRV_COMPAT nordic_nrf_lrcconf

static bool requested;

static void shim_request_constlat(void)
{
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

static void shim_release_constlat(void)
{
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

static void shim_request_latency(const struct device *dev, uint8_t event_state)
{
	ARG_UNUSED(dev);

	if (event_state && !requested) {
		shim_request_constlat();
		requested = true;
	} else if (requested) {
		shim_release_constlat();
		requested = false;
	}
}

static int shim_init(const struct device *dev)
{
	pm_event_device_init(PM_EVENT_DEVICE_DT_INST_GET(0));
	return 0;
}

PM_EVENT_DEVICE_DT_INST_DEFINE(0, shim_request_latency, 100, 2);
DEVICE_DT_INST_DEFINE(0, shim_init, NULL, NULL, NULL, PRE_KERNEL_1, 0, NULL);
