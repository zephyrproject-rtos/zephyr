/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(init_usb, CONFIG_USBH_LOG_LEVEL);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static int init_usb(void)
{
	int ret;

	ret = usbh_init(&uhs_ctx);
	if (ret) {
		LOG_ERR("Failed to initialize USB host. Error: %d", ret);
		return ret;
	}

	ret = usbh_enable(&uhs_ctx);
	if (ret) {
		LOG_ERR("Failed to enable USB host. Error: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(init_usb, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
