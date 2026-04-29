/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>

#if defined(CONFIG_USB_HOST_STACK) && !defined(CONFIG_USBH_DEFINE_CONTROLLER_FROM_DT)
USBH_CONTROLLER_DEFINE(sample_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
#endif

int main(void)
{
	return 0;
}
