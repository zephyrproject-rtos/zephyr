/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * ABOV Semiconductor SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ABOV_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ABOV_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/abov32-pinctrl.h>

/* ABOV HLL PCU type definitions */
#include <type/pcu_type.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Type for ABOV pin configuration bitfield.
 *
 * Bit allocation:
 * - 0..3:   Pin number (0..15)
 * - 4..7:   Port ID (0=PA, 1=PB, 2=PC, etc.)
 * - 8..11:  Alternate function number (0..10)
 * - 12..15: Reserved
 * - 16..17: Pull-up/pull-down configuration (@ref ABOV_PUPD)
 * - 18:     Output type configuration (@ref ABOV_OTYPE)
 * - 19..31: Reserved
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @name ABOV pin configuration field bitfield mask and positions
 * @{
 */

/** Pull-up/down field position */
#define ABOV_PUPD_POS     16U
/** Pull-up/down field mask */
#define ABOV_PUPD_MSK     0x3U

/** Output type field position */
#define ABOV_OTYPE_POS    18U
/** Output type field mask */
#define ABOV_OTYPE_MSK    0x1U

/** @} */

/**
 * @name ABOV Pull-up/pull-down values (mapped to PCU_PUPD_e in HLL)
 * @anchor ABOV_PUPD
 * @{
 */

/** Pull-up/pull-down disabled */
#define ABOV_PUPD_NONE    0U
/** Pull-up enabled */
#define ABOV_PUPD_PULLUP  1U
/** Pull-down enabled */
#define ABOV_PUPD_PULLDN  2U

/** @} */

/**
 * @name ABOV Output type values
 * @anchor ABOV_OTYPE
 * @{
 */

/** Push-pull output */
#define ABOV_OTYPE_PP     0U
/** Open-drain output */
#define ABOV_OTYPE_OD     1U

/** @} */

/**
 * @brief Utility macro to initialize each pin in pinctrl_soc_pin_t format.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			       \
	(DT_PROP_BY_IDX(node_id, prop, idx) |				       \
	 ((ABOV_PUPD_PULLUP * DT_PROP(node_id, bias_pull_up)) << ABOV_PUPD_POS) | \
	 ((ABOV_PUPD_PULLDN * DT_PROP(node_id, bias_pull_down)) << ABOV_PUPD_POS) | \
	 ((ABOV_OTYPE_OD * DT_PROP(node_id, drive_open_drain)) << ABOV_OTYPE_POS)),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM, pinmux,		       \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

/**
 * @brief Obtain Port ID field from pinctrl_soc_pin_t configuration.
 *
 * @param pin pinctrl_soc_pin_t bitfield value.
 * @return Port ID as PCU_ID_e type.
 */
#define ABOV_PORT_GET(pin)  ((PCU_ID_e)(((pin) >> ABOV_PORT_POS) & ABOV_PORT_MSK))

/**
 * @brief Obtain Pin number field from pinctrl_soc_pin_t configuration.
 *
 * @param pin pinctrl_soc_pin_t bitfield value.
 * @return Pin number as PCU_PIN_ID_e type.
 */
#define ABOV_PIN_GET(pin)   ((PCU_PIN_ID_e)(((pin) >> ABOV_PIN_POS) & ABOV_PIN_MSK))

/**
 * @brief Obtain Alternate Function field from pinctrl_soc_pin_t configuration.
 *
 * @param pin pinctrl_soc_pin_t bitfield value.
 * @return Alternate Function as PCU_ALT_e type.
 */
#define ABOV_ALT_GET(pin)   ((PCU_ALT_e)(((pin) >> ABOV_ALT_POS) & ABOV_ALT_MSK))

/**
 * @brief Obtain Pull-up/down field from pinctrl_soc_pin_t configuration.
 *
 * @param pin pinctrl_soc_pin_t bitfield value.
 * @return Pull configuration as PCU_PUPD_e type.
 */
#define ABOV_PUPD_GET(pin)  ((PCU_PUPD_e)(((pin) >> ABOV_PUPD_POS) & ABOV_PUPD_MSK))

/**
 * @brief Obtain Output Type field from pinctrl_soc_pin_t configuration.
 *
 * @param pin pinctrl_soc_pin_t bitfield value.
 * @return 0 for Push-Pull, 1 for Open-Drain.
 */
#define ABOV_OTYPE_GET(pin) (((pin) >> ABOV_OTYPE_POS) & ABOV_OTYPE_MSK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ABOV_COMMON_PINCTRL_SOC_H_ */
