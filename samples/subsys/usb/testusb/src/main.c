/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#ifndef CONFIG_USB_DEVICE_AUTO_ENABLE
#include <usb/usb_device.h>
#endif
LOG_MODULE_REGISTER(main);

void main(void)
{
#ifndef CONFIG_USB_DEVICE_AUTO_ENABLE
	int ret;

	ret = usb_enable();
	if (ret < 0) {
		LOG_ERR("Failed to enable USB device");
		return;
	}
#endif
	LOG_INF("entered main.");
}

