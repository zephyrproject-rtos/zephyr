/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC2520_H_
#define ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC2520_H_

#include <device.h>

enum cc2520_gpio_index {
	CC2520_GPIO_IDX_VREG_EN	= 0,
	CC2520_GPIO_IDX_RESET,
	CC2520_GPIO_IDX_FIFO,
	CC2520_GPIO_IDX_CCA,
	CC2520_GPIO_IDX_SFD,
	CC2520_GPIO_IDX_FIFOP,

	CC2520_GPIO_IDX_MAX,
};

struct cc2520_gpio_configuration {
	struct device *dev;
	u32_t pin;
};

struct cc2520_gpio_configuration *cc2520_configure_gpios(void);

#endif	/* ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC2520_H_ */
