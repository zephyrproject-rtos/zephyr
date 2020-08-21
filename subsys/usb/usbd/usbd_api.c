/*
 * Copyright (c) 2016-2018 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <usb/usbd.h>
#include <sys/util.h>

#include <usbd_internal.h>

extern struct usbd_contex usbd_ctx;

int usbd_ep_set_stall(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usbd_ep_clear_stall(uint8_t ep)
{
	return usb_dc_ep_clear_stall(ep);
}

int usbd_wakeup_request(void)
{
	if (IS_ENABLED(CONFIG_USBD_DEVICE_REMOTE_WAKEUP)) {
		if (usbd_ctx.remote_wakeup) {
			return usb_dc_wakeup_request();
		}
		return -EACCES;
	} else {
		return -ENOTSUP;
	}
}
