/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define ECHO_DATA_SIZE	4

static void ipm_callback(void *context, u32_t id, volatile void *data)
{
	struct device *ipm = context;
	int status;

	LOG_DBG("id 0x%x", id);

	LOG_HEXDUMP_DBG((void *)data, ECHO_DATA_SIZE, "data");

	status = ipm_send(ipm, 0, id, (void *)data, ECHO_DATA_SIZE);
	if (status) {
		LOG_ERR("ipm_send failed, error %d", status);
	}
}

void main(void)
{
	struct device *ipm;

	LOG_DBG("Starting IPM echo application");

	ipm = device_get_binding("IPM_0");
	if (!ipm) {
		LOG_ERR("Cannot get IPM device");
		return;
	}

	ipm_register_callback(ipm, ipm_callback, ipm);
	ipm_set_enabled(ipm, 1);

	LOG_DBG("IPM initialized");
}
