/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/event_device.h>

#include <nrfx_power.h>

#define DT_DRV_COMPAT nordic_nrf_power

static bool requested;

static void shim_request_latency(const struct device *dev, uint8_t event_state)
{
	ARG_UNUSED(dev);

	if (event_state && !requested) {
		(void)nrfx_power_constlat_mode_request();
		requested = true;
	} else if (requested) {
		(void)nrfx_power_constlat_mode_free();
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
