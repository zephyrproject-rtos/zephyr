/*
 * Copyright (c) 2019 SixOctets Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <gpio.h>
#include <sys/printk.h>

#define AMP_CPS_GPIO_PIN 6

#if defined(CONFIG_BT_CTLR_GPIO_PA) || defined(CONFIG_BT_CTLR_GPIO_LNA)
static int amp_cps_init(struct device *dev)
{
	struct device *gpio;

	gpio = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio) {
		printk("Could not bind device \"%s\"\n", DT_GPIO_P0_DEV_NAME);
		return -ENODEV;
	}

	gpio_pin_configure(gpio, AMP_CPS_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(gpio, AMP_CPS_GPIO_PIN, 0);

	return 0;
}

DEVICE_INIT(amp_cps, "Amp CPS", amp_cps_init, NULL, NULL, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif
