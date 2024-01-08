/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#ifdef CONFIG_USB_DEVICE_STACK
/*
 * Enable console on USB CDC_ACM
 */
static int sensortile_box_pro_usb_console_init(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return -1;
	}

	return 0;
}

/* needs to be done at Application */
SYS_INIT(sensortile_box_pro_usb_console_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif
