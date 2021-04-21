/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <usb/usb_device.h>
#include <net/net_config.h>

void main(void)
{
#if defined(CONFIG_USB)
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("usb enable error %d\n", ret);
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB */
}
