/*
 * Copyright (c) 2024 Maxjta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Artery SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_ARTERY_AT32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_ARTERY_AT32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <dt-bindings/pinctrl/at32_iomux.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Type for AT32 pin.
 *
 * Bits (mux model):
 * - 0-12: AT32_PIN_MUX bit field.
 * - 13-25: Reserved.
 * - 26-31: Pin configuration bit field (@ref AT32_PINCFG).
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
	 ((AT32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << AT32_PUPD_POS) |                  \
	 ((AT32_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << AT32_PUPD_POS) |              \
	 ((AT32_OUTPUT_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << AT32_OMODE_POS) |                \
	 (DT_ENUM_IDX(node_id, slew_rate) << AT32_DRIVE_POS)),

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,    \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}
/** @endcond */

/**
 * @name AT32 PULL (GPIO pull-up/pull-donw).
 * @{
 */

/** No pull-up/down */
#define AT32_PULL_NONE 0U
/** Pull-up */
#define AT32_PULL_UP   1U
/** Pull-down */
#define AT32_PULL_DOWN 2U

/** @} */

/**
 * @name AT32 OMODE (GPIO output mode).
 * @{
 */

/** Push-pull */
#define AT32_OUTPUT_PUSH_PULL  0U
/** Open-drain */
#define AT32_OUTPUT_OPEN_DRAIN 1U

/** @} */

/**
 * @name AT32 ODRVR (GPIO output drive capability).
 * @{
 */

 /** GPIO stronger sourcing/sinking strength */
#define AT32_DRIVE_STRENGTH_STRONGER  1U
 /** GPIO moderate sourcing/sinking strength */
#define AT32_DRIVE_STRENGTH_MODERATE  2U

/** @} */

/**
 * @name AT32 pin configuration bit field mask and positions.
 * @anchor AT32_PINCFG
 *
 * Fields:
 *
 * - 31..29: Pull-up/down
 * - 28:     Output type
 * - 27..26: Output drive capability
 *
 * @{
 */

/** PUPD field mask. */
#define AT32_PUPD_MSK 0x3U
/** PUPD field position. */
#define AT32_PUPD_POS 29U
/** OMODE field mask. */
#define AT32_OMODE_MSK 0x1U
/** OMODE field position. */
#define AT32_OMODE_POS 28U
/** DRIVE field mask. */
#define AT32_DRIVE_MSK 0x3U
/** DRIVE field position. */
#define AT32_DRIVE_POS 26U

/** @} */

/**
 * Obtain PUPD field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define AT32_PUPD_GET(pincfg) (((pincfg) >> AT32_PUPD_POS) & AT32_PUPD_MSK)

/**
 * Obtain OMODE field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define AT32_OMODE_GET(pincfg) (((pincfg) >> AT32_OMODE_POS) & AT32_OMODE_MSK)

/**
 * Obtain DRIVE field from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define AT32_DRIVE_GET(pincfg) (((pincfg) >> AT32_DRIVE_POS) & AT32_DRIVE_MSK)


#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_SOC_ARM_ARTERY_AT32_COMMON_PINCTRL_SOC_H_ */
