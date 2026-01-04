/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM64_BCM2711_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM64_BCM2711_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/bcm2711-pinctrl.h>

/**
 * @brief Type to hold a pin's pinctrl configuration.
 */
typedef struct bcm2711_pinctrl_soc_pin {
	/** GPIO pin number (0-57) */
	uint32_t pin;
	/** Function select (0-7: IN, OUT, ALT0-ALT5) */
	uint32_t func;
	/** Pull resistor configuration (0=none, 1=down, 2=up) */
	uint32_t pull;
} pinctrl_soc_pin_t;

/**
 * @brief Get pull configuration from DT node properties
 *
 * @param node_id Node identifier (the group node).
 */
#define Z_PINCTRL_BCM2711_PULL(node_id)                                                            \
	(COND_CODE_1(DT_PROP(node_id, bias_disable), \
		(BCM2711_PULL_NONE), \
	(COND_CODE_1(DT_PROP(node_id, bias_pull_down), \
		(BCM2711_PULL_DOWN), \
	(COND_CODE_1(DT_PROP(node_id, bias_pull_up), \
		(BCM2711_PULL_UP), \
	(BCM2711_PULL_NONE)))))))

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
	 .pull = Z_PINCTRL_BCM2711_PULL(node_id)},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier (the pinctrl state node, e.g., uart0_default).
 * @param prop Property name describing state pins (should be "pinctrl-0").
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_ARM64_BCM2711_PINCTRL_SOC_H_ */
