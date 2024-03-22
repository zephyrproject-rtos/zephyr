/*
 * Copyright (c) 2022, Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Atmel SAM SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_SAM_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_SAM_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <dt-bindings/pinctrl/atmel_sam_pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Type for SAM pin.
 *
 * Bits:
 * -  0-15: SAM pinmux bit field (@ref SAM_PINMUX).
 * - 16-21: Pin flags bit field (@ref SAM_PINFLAGS).
 * - 22-31: Reserved.
 */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#if defined(CONFIG_SOC_FAMILY_ATMEL_SAM)
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)				\
	((DT_PROP_BY_IDX(node_id, prop, idx)   << SAM_PINCTRL_PINMUX_POS)	\
	 | (DT_PROP(node_id, bias_pull_up)     << SAM_PINCTRL_PULLUP_POS)	\
	 | (DT_PROP(node_id, bias_pull_down)   << SAM_PINCTRL_PULLDOWN_POS)	\
	 | (DT_PROP(node_id, drive_open_drain) << SAM_PINCTRL_OPENDRAIN_POS)	\
	),
#else /* CONFIG_SOC_FAMILY_ATMEL_SAM0 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)				  \
	((DT_PROP_BY_IDX(node_id, prop, idx)     << SAM_PINCTRL_PINMUX_POS)	  \
	 | (DT_PROP(node_id, bias_pull_up)       << SAM_PINCTRL_PULLUP_POS)	  \
	 | (DT_PROP(node_id, bias_pull_down)     << SAM_PINCTRL_PULLDOWN_POS)	  \
	 | (DT_PROP(node_id, input_enable)       << SAM_PINCTRL_INPUTENABLE_POS)  \
	 | (DT_PROP(node_id, output_enable)      << SAM_PINCTRL_OUTPUTENABLE_POS) \
	 | (DT_ENUM_IDX(node_id, drive_strength) << SAM_PINCTRL_DRIVESTRENGTH_POS)\
	),
#endif

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),			\
				DT_FOREACH_PROP_ELEM, pinmux,			\
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */


/**
 * @brief Pin flags/attributes
 * @anchor SAM_PINFLAGS
 *
 * @{
 */

#define SAM_PINCTRL_FLAGS_DEFAULT       (0U)
#define SAM_PINCTRL_FLAGS_POS           (0U)
#define SAM_PINCTRL_FLAGS_MASK          (0x3F << SAM_PINCTRL_FLAGS_POS)
#define SAM_PINCTRL_FLAG_MASK           (1U)
#define SAM_PINCTRL_PULLUP_POS          (SAM_PINCTRL_FLAGS_POS)
#define SAM_PINCTRL_PULLUP              (1U << SAM_PINCTRL_PULLUP_POS)
#define SAM_PINCTRL_PULLDOWN_POS        (SAM_PINCTRL_PULLUP_POS + 1U)
#define SAM_PINCTRL_PULLDOWN            (1U << SAM_PINCTRL_PULLDOWN_POS)
#define SAM_PINCTRL_OPENDRAIN_POS       (SAM_PINCTRL_PULLDOWN_POS + 1U)
#define SAM_PINCTRL_OPENDRAIN           (1U << SAM_PINCTRL_OPENDRAIN_POS)
#define SAM_PINCTRL_INPUTENABLE_POS     (SAM_PINCTRL_OPENDRAIN_POS + 1U)
#define SAM_PINCTRL_INPUTENABLE         (1U << SAM_PINCTRL_INPUTENABLE_POS)
#define SAM_PINCTRL_OUTPUTENABLE_POS    (SAM_PINCTRL_INPUTENABLE_POS + 1U)
#define SAM_PINCTRL_OUTPUTENABLE        (1U << SAM_PINCTRL_OUTPUTENABLE_POS)
#define SAM_PINCTRL_DRIVESTRENGTH_POS   (SAM_PINCTRL_OUTPUTENABLE_POS + 1U)
#define SAM_PINCTRL_DRIVESTRENGTH       (1U << SAM_PINCTRL_DRIVESTRENGTH_POS)

/** @} */

/**
 * Obtain Flag value from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 * @param pos    attribute/flags bit position (@ref SAM_PINFLAGS).
 */
#define SAM_PINCTRL_FLAG_GET(pincfg, pos) \
	(((pincfg) >> pos) & SAM_PINCTRL_FLAG_MASK)

#define SAM_PINCTRL_FLAGS_GET(pincfg) \
	(((pincfg) >> SAM_PINCTRL_FLAGS_POS) & SAM_PINCTRL_FLAGS_MASK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_SOC_SAM_COMMON_H_ */
