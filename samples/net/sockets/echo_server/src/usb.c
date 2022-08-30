/*
 * Copyright (c) 2021 TiaC Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>

int init_usb(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Cannot enable USB (%d)", ret);
		return ret;
	}

	(void)net_config_init_app(NULL, "Initializing network");

	return 0;
}
