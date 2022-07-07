/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Testing specific helpers for pinctrl driver.
 */

#ifndef ZEPHYR_TESTS_DRIVERS_PINCTRL_API_SRC_PINCTRL_SOC_H_
#define ZEPHYR_TESTS_DRIVERS_PINCTRL_API_SRC_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

/**
 * @name Test pin configuration bit field positions and masks.
 *
 * Bits 31, 30: Pull config.
 * Bits 29..0: Pin.
 *
 * @{
 */

/** Position of the pull field. */
#define TEST_PULL_POS 30U
/** Mask of the pull field. */
#define TEST_PULL_MSK 0x3U
/** Position of the pin field. */
#define TEST_PIN_POS 0U
/** Mask for the pin field. */
#define TEST_PIN_MSK 0x3FFFFFFFU

/** @} */

/**
 * @name Test pinctrl pull-up/down.
 * @{
 */

/** Pull-up disabled. */
#define TEST_PULL_DISABLE 0U
/** Pull-down enabled. */
#define TEST_PULL_DOWN 1U
/** Pull-up enabled. */
#define TEST_PULL_UP 2U

/** @} */

/**
 * @brief Utility macro to obtain pin pull configuration.
 *
 * @param pincfg Pin configuration bit field.
 */
#define TEST_GET_PULL(pincfg) (((pincfg) >> TEST_PULL_POS) & TEST_PULL_MSK)

/**
 * @brief Utility macro to obtain port and pin combination.
 *
 * @param pincfg Pin configuration bit field.
 */
#define TEST_GET_PIN(pincfg) (((pincfg) >> TEST_PIN_POS) & TEST_PIN_MSK)

/** @cond INTERNAL_HIDDEN */

/** Test pin type */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			       \
	(((DT_PROP_BY_IDX(node_id, prop, idx) << TEST_PIN_POS)		       \
	  & TEST_PIN_MSK) |						       \
	 ((TEST_PULL_UP * DT_PROP(node_id, bias_pull_up))		       \
	  << TEST_PULL_POS) |						       \
	 ((TEST_PULL_DOWN * DT_PROP(node_id, bias_pull_down))		       \
	  << TEST_PULL_POS)						       \
	),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PROP_BY_IDX(node_id, prop, 0),	       \
				DT_FOREACH_PROP_ELEM, pins,		       \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#endif /* ZEPHYR_TESTS_DRIVERS_PINCTRL_API_SRC_PINCTRL_SOC_H_ */
