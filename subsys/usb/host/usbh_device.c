/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include "usbh_device.h"

#define USBH_USB_DEVICE_COUNT		1

static struct usb_device udevs[USBH_USB_DEVICE_COUNT];

struct usb_device *usbh_device_get_any(struct usbh_contex *const ctx)
{
	udevs->ctx = ctx;

	return udevs;
}
