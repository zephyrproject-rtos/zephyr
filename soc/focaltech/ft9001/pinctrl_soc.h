/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FocalTech FT9001 SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_FOCALTECH_FT9001_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_FOCALTECH_FT9001_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/focaltech_ft9001_pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Type for FT9001 pin configuration.
 *
 * Bits:
 * - 0-25: FT9001_PINMUX bit field.
 * - 26-31: Pin configuration bit field (@ref FT9001_PINCFG).
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	(DT_PROP_BY_IDX(node_id, prop, idx) |                                                      \
	 ((FT9001_PUPD_PULLUP * DT_PROP(node_id, bias_pull_up)) << FT9001_PUPD_POS) |              \
	 ((FT9001_PUPD_PULLDOWN * DT_PROP(node_id, bias_pull_down)) << FT9001_PUPD_POS) |          \
	 ((FT9001_OTYPE_OD * DT_PROP(node_id, drive_open_drain)) << FT9001_OTYPE_POS) |            \
	 (DT_ENUM_IDX(node_id, slew_rate) << FT9001_OSPEED_POS)),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

/**
 * @name FT9001 pull-up/down configuration
 * @{
 */
#define FT9001_PUPD_NONE     0U /**< No pull-up/down */
#define FT9001_PUPD_PULLUP   1U /**< Pull-up         */
#define FT9001_PUPD_PULLDOWN 2U /**< Pull-down       */
/** @} */

/**
 * @name FT9001 output type configuration
 * @{
 */
#define FT9001_OTYPE_PP 0U /**< Push-pull  */
#define FT9001_OTYPE_OD 1U /**< Open-drain */
/** @} */

/**
 * @name FT9001 output speed configuration
 * @{
 */
#define FT9001_OSPEED_10MHZ 0U /**< Maximum 10MHz */
#define FT9001_OSPEED_2MHZ  1U /**< Maximum 2MHz  */
#define FT9001_OSPEED_50MHZ 2U /**< Maximum 50MHz */
#define FT9001_OSPEED_MAX   3U /**< Maximum speed */
/** @} */

/**
 * @name FT9001 pin configuration bit field positions and masks
 * @{
 */
#define FT9001_PUPD_POS   29U  /**< PUPD field position   */
#define FT9001_PUPD_MSK   0x3U /**< PUPD field mask       */
#define FT9001_OTYPE_POS  28U  /**< OTYPE field position  */
#define FT9001_OTYPE_MSK  0x1U /**< OTYPE field mask      */
#define FT9001_OSPEED_POS 26U  /**< OSPEED field position */
#define FT9001_OSPEED_MSK 0x3U /**< OSPEED field mask     */
/** @} */

/**
 * @brief Obtain PUPD field from pinctrl_soc_pin_t configuration.
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define FT9001_PUPD_GET(pincfg) (((pincfg) >> FT9001_PUPD_POS) & FT9001_PUPD_MSK)

/**
 * @brief Obtain OTYPE field from pinctrl_soc_pin_t configuration.
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define FT9001_OTYPE_GET(pincfg) (((pincfg) >> FT9001_OTYPE_POS) & FT9001_OTYPE_MSK)

/**
 * @brief Obtain OSPEED field from pinctrl_soc_pin_t configuration.
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define FT9001_OSPEED_GET(pincfg) (((pincfg) >> FT9001_OSPEED_POS) & FT9001_OSPEED_MSK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_FOCALTECH_FT9001_PINCTRL_SOC_H_ */
