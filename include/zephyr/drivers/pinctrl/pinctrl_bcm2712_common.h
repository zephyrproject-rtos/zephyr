/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_BCM2712_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_BCM2712_COMMON_H_

#include <stdint.h>
#include <zephyr/dt-bindings/pinctrl/bcm2712-pinctrl.h>

#define BRCM_BCM2712_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                    \
	{                                                                                          \
	}

struct brcm_bcm2712_pinctrl_pinconfig {
};

int brcm_bcm2712_pinctrl_configure_pin(const struct brcm_bcm2712_pinctrl_pinconfig *pin);

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_BCM2712_COMMON_H_ */
