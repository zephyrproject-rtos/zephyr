/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Axonne Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Testing specific helpers for pinctrl hogs.
 */

#ifndef ZEPHYR_TESTS_DRIVERS_PINCTRL_HOGS_SRC_PINCTRL_SOC_H_
#define ZEPHYR_TESTS_DRIVERS_PINCTRL_HOGS_SRC_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

/** @cond INTERNAL_HIDDEN */

/** Test pin type. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

/**
 * @brief Utility macro to initialize pins from one state phandle.
 *
 * @param node_id Consumer node identifier.
 * @param prop Property containing state phandles.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PINS_INIT_BY_IDX(node_id, prop, idx)                                       \
	DT_FOREACH_CHILD_VARGS(DT_PHANDLE_BY_IDX(node_id, prop, idx), DT_FOREACH_PROP_ELEM, pins,  \
			       Z_PINCTRL_STATE_PIN_INIT)

/**
 * @brief Utility macro to initialize state pins referenced by a property.
 *
 * @param node_id Consumer node identifier.
 * @param prop Property containing state phandles.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, Z_PINCTRL_STATE_PINS_INIT_BY_IDX, ())}

/** @endcond */

#endif /* ZEPHYR_TESTS_DRIVERS_PINCTRL_HOGS_SRC_PINCTRL_SOC_H_ */
