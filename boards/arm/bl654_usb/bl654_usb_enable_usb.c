/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <usb/usb_device.h>

static int usb_enable_boot(const struct device *dev)
{
	ARG_UNUSED(dev);
	usb_enable(NULL);

	return 0;
}

SYS_INIT(usb_enable_boot, POST_KERNEL,
	 CONFIG_BOARD_ENABLE_USB_AUTOMATICALLY_INIT_PRIORITY);
