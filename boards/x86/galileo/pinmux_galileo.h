/* pinmux_galileo.h */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PINMUX_GALILEO_PRIV_H
#define __PINMUX_GALILEO_PRIV_H

struct galileo_data {
	struct device *exp0;
	struct device *exp1;
	struct device *exp2;
	struct device *pwm0;

	/* GPIO<0>..GPIO<7> */
	struct device *gpio_dw;

	/* GPIO<8>..GPIO<9>, which means to pin 0 and 1 on core well. */
	struct device *gpio_core;

	/* GPIO_SUS<0>..GPIO_SUS<5> */
	struct device *gpio_resume;

	struct pin_config *mux_config;
};

struct galileo_data galileo_pinmux_driver;

int z_galileo_pinmux_set_pin(struct device *port, u8_t pin, u32_t func);

int z_galileo_pinmux_get_pin(struct device *port, u32_t pin, u32_t *func);

#endif /* __PINMUX_GALILEO_PRIV_H */
