/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>

void main(void)
{
#if defined(CONFIG_USB_DEVICE_STACK)
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("usb enable error %d\n", ret);
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB_DEVICE_STACK */
}
