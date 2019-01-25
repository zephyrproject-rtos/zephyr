/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <uart.h>
#include <device.h>
#include <init.h>

#if defined(CONFIG_IEEE802154_CC2520)

#include <ieee802154/cc2520.h>
#include <gpio.h>

static struct cc2520_gpio_configuration cc2520_gpios[CC2520_GPIO_IDX_MAX] = {
	{ .dev = NULL, .pin = DT_CC2520_GPIO_VREG_EN, },
	{ .dev = NULL, .pin = DT_CC2520_GPIO_RESET, },
	{ .dev = NULL, .pin = DT_CC2520_GPIO_FIFO, },
	{ .dev = NULL, .pin = DT_CC2520_GPIO_CCA, },
	{ .dev = NULL, .pin = DT_CC2520_GPIO_SFD, },
	{ .dev = NULL, .pin = DT_CC2520_GPIO_FIFOP, },
};

struct cc2520_gpio_configuration *cc2520_configure_gpios(void)
{
	const int flags_noint_out = GPIO_DIR_OUT;
	const int flags_noint_in = GPIO_DIR_IN;
	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				  GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	struct device *gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_VREG_EN_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_VREG_EN].pin,
			   flags_noint_out);
	cc2520_gpios[CC2520_GPIO_IDX_VREG_EN].dev = gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_RESET_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_RESET].pin,
			   flags_noint_out);
	cc2520_gpios[CC2520_GPIO_IDX_RESET].dev = gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_SFD_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_SFD].pin,
			   flags_int_in);
	cc2520_gpios[CC2520_GPIO_IDX_SFD].dev = gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_FIFOP_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_FIFOP].pin,
			   flags_int_in);
	cc2520_gpios[CC2520_GPIO_IDX_FIFOP].dev = gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_FIFO_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_FIFO].pin,
			   flags_noint_in);
	cc2520_gpios[CC2520_GPIO_IDX_FIFO].dev = gpio;

	gpio = device_get_binding(DT_CC2520_GPIO_CCA_DRV_NAME);
	gpio_pin_configure(gpio, cc2520_gpios[CC2520_GPIO_IDX_CCA].pin,
			   flags_noint_in);
	cc2520_gpios[CC2520_GPIO_IDX_CCA].dev = gpio;

	return cc2520_gpios;
}

#endif /* CONFIG_IEEE802154_CC2520 */
