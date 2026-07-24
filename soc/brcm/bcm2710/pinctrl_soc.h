/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The BCM283x GPIO/pinctrl block is register-compatible across the
 * BCM2710 and BCM2711, so the brcm,bcm2711-pinctrl driver and its
 * dt-bindings are reused unchanged.
 */

#ifndef ZEPHYR_SOC_ARM64_BCM2710_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM64_BCM2710_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/bcm2711-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
typedef struct bcm2710_pinctrl_soc_pin {
	/** GPIO pin number (0-57) */
	uint8_t pin;
	/** Function select (0-7: IN, OUT, ALT0-ALT5) */
	uint8_t func;
	/** Pull resistor configuration (0=none, 1=up, 2=down) */
	uint8_t pull;
} pinctrl_soc_pin_t;

/**
 * @brief Get pull configuration from DT node properties.
 *
 * @param node_id Node identifier (the group node).
 */
#define BCM2710_GET_PULL(node_id)                                                                  \
	COND_CASE_1(DT_PROP(node_id, bias_disable), (BCM2711_PULL_NONE),                           \
		    DT_PROP(node_id, bias_pull_up), (BCM2711_PULL_UP),                             \
		    DT_PROP(node_id, bias_pull_down), (BCM2711_PULL_DOWN), (BCM2711_PULL_NONE))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier (the group node).
 * @param prop Property name (should be "pinmux").
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{.pin = BCM2711_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),                               \
	 .func = BCM2711_GET_FUNC(DT_PROP_BY_IDX(node_id, prop, idx)),                             \
	 .pull = BCM2710_GET_PULL(node_id)},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier (the pinctrl state node).
 * @param prop Property name describing state pins (should be "pinctrl-0").
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_ARM64_BCM2710_PINCTRL_SOC_H_ */
