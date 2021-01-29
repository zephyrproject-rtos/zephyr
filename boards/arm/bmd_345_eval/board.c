/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>

#define MODE_PIN	36	/* P1.04 */
#define A_SEL_PIN	34	/* P1.02 */

static int bmd_345_eval_pa_lna_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret;
	const struct device *mode_asel_port_dev;

	mode_asel_port_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio1)));
	if (!mode_asel_port_dev) {
		return -EIO;
	}

	ret = gpio_pin_configure(mode_asel_port_dev, MODE_PIN,
				 GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure(mode_asel_port_dev, A_SEL_PIN,
				 GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

SYS_INIT(bmd_345_eval_pa_lna_init,  POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
