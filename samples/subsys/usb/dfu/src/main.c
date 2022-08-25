/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample app for USB DFU class driver. */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
LOG_MODULE_REGISTER(main);

void main(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("This device supports USB DFU class.\n");
}
