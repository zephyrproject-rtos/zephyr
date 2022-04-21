/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Gigadevice SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_GD32_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_GD32_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef CONFIG_PINCTRL_GD32_AF
#include <dt-bindings/pinctrl/gd32-af.h>
#else
#include <dt-bindings/pinctrl/gd32-afio.h>
#endif /* CONFIG_PINCTRL_GD32_AF */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Type for GD32 pin.
 *
 * Bits (AF model):
 * - 0-12: GD32_PINMUX_AF bit field.
 * - 13-25: Reserved.
 * - 26-31: Pin configuration bit field (@ref GD32_PINCFG).
 *
 * Bits (AFIO model):
 * - 0-19: GD32_PINMUX_AFIO bit field.
 * - 20-25: Reserved.
 * - 26-31: Pin configuration bit field (@ref GD32_PINCFG).
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			       \
	(DT_PROP_BY_IDX(node_id, prop, idx) |				       \
	 ((GD32_PUPD_PULLUP * DT_PROP(node_id, bias_pull_up))		       \
	  << GD32_PUPD_POS) |						       \
	 ((GD32_PUPD_PULLDOWN * DT_PROP(node_id, bias_pull_down))	       \
	  << GD32_PUPD_POS) |						       \
	 ((GD32_OTYPE_OD * DT_PROP(node_id, drive_open_drain))		       \
	  << GD32_OTYPE_POS) |						       \
	 (DT_ENUM_IDX(node_id, slew_rate) << GD32_OSPEED_POS)),

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
 * @name GD32 PUPD (values match the ones in the HAL for AF model).
 * @{
 */

/** No pull-up/down */
#define GD32_PUPD_NONE 0U
/** Pull-up */
#define GD32_PUPD_PULLUP 1U
/** Pull-down */
#define GD32_PUPD_PULLDOWN 2U

/** @} */

/**
 * @name GD32 OTYPE (values match the ones in the HAL for AF model).
 * @{
 */

/** Push-pull */
#define GD32_OTYPE_PP 0U
/** Open-drain */
#define GD32_OTYPE_OD 1U

/** @} */

/**
 * @name GD32 OSPEED (values match the ones in the HAL for AF model, mode minus
 * one for AFIO model).
 * @{
 */

#ifdef CONFIG_PINCTRL_GD32_AF
/** Maximum 2MHz */
#define GD32_OSPEED_2MHZ 0U
#ifdef CONFIG_SOC_SERIES_GD32F3X0
/** Maximum 10MHz */
#define GD32_OSPEED_10MHZ 1U
/** Maximum 50MHz */
#define GD32_OSPEED_50MHZ 3U
#else
/** Maximum 25MHz */
#define GD32_OSPEED_25MHZ 1U
/** Maximum 50MHz */
#define GD32_OSPEED_50MHZ 2U
/** Maximum 200MHz */
#define GD32_OSPEED_200MHZ 3U
#endif /* CONFIG_SOC_SERIES_GD32F3X0 */
#else
/** Maximum 10MHz */
#define GD32_OSPEED_10MHZ 0U
/** Maximum 2MHz */
#define GD32_OSPEED_2MHZ 1U
/** Maximum 50MHz */
#define GD32_OSPEED_50MHZ 2U
/** Maximum speed */
#define GD32_OSPEED_MAX 3U
#endif /* CONFIG_PINCTRL_GD32_AF */

/** @} */

/**
 * @name GD32 pin configuration bit field mask and positions.
 * @anchor GD32_PINCFG
 *
 * Fields:
 *
 * - 31..29: Pull-up/down
 * - 28:     Output type
 * - 27..26: Output speed
 *
 * @{
 */

/** PUPD field mask. */
#define GD32_PUPD_MSK 0x3U
/** PUPD field position. */
#define GD32_PUPD_POS 29U
/** OTYPE field mask. */
#define GD32_OTYPE_MSK 0x1U
/** OTYPE field position. */
#define GD32_OTYPE_POS 28U
/** OSPEED field mask. */
#define GD32_OSPEED_MSK 0x3U
/** OSPEED field position. */
#define GD32_OSPEED_POS 26U

/** @} */

/**
 * Obtain PUPD field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define GD32_PUPD_GET(pincfg) \
	(((pincfg) >> GD32_PUPD_POS) & GD32_PUPD_MSK)

/**
 * Obtain OTYPE field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define GD32_OTYPE_GET(pincfg) \
	(((pincfg) >> GD32_OTYPE_POS) & GD32_OTYPE_MSK)

/**
 * Obtain OSPEED field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define GD32_OSPEED_GET(pincfg) \
	(((pincfg) >> GD32_OSPEED_POS) & GD32_OSPEED_MSK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_GD32_COMMON_H_ */
