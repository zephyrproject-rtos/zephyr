/*
 * Copyright (c) 2020 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <usb/usbd.h>
LOG_MODULE_REGISTER(main);

void main(void)
{
	struct device *dev;
	int ret = 0;

	dev = device_get_binding("USBD_CLASS_LB_0");

	usbd_cctx_register(dev);

	ret = usbd_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB device stack");
		return;
	}
}
