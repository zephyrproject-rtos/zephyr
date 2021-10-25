/*
 * Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Bouffalo Lab SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_RISCV_BFLB_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_BFLB_COMMON_PINCTRL_SOC_H_

#include <devicetree.h>
#include <zephyr/types.h>
#include <dt-bindings/pinctrl/bflb-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef struct pinctrl_soc_pin {
	uint16_t fun;
	uint16_t cfg;
	uint8_t  pin;
	uint8_t  flags;
} pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize fun field in #pinctrl_soc_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_BFLB_FUN_INIT(node_id, prop, idx)				\
	(									\
	 DT_PHA_BY_IDX(node_id, prop, idx, fun) |				\
	 (BFLB_FUN_MODE_INPUT * DT_PROP_OR(node_id, input_enable, 0)) |		\
	 (BFLB_FUN_MODE_OUTPUT * DT_PROP_OR(node_id, output_enable, 0))		\
	)

/**
 * @brief Utility macro to initialize flags field in #pinctrl_soc_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_BFLB_CFG_INIT(node_id)					\
	(									\
	 (BFLB_GPIO_MODE_PULL_UP * DT_PROP_OR(node_id, bias_pull_up, 0)) |	\
	 (BFLB_GPIO_MODE_PULL_DOWN * DT_PROP_OR(node_id, bias_pull_down, 0)) |	\
	 (BFLB_GPIO_INP_SMT_EN * DT_PROP_OR(node_id, input_schmitt_enable, 0)) |\
	 (DT_PROP_OR(node_id, drive_strength, 0) << BFLB_GPIO_DRV_STR_POS)	\
	)

/**
 * @brief Utility macro to initialize pin field in #pinctrl_soc_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_BFLB_PIN_INIT(node_id, prop, idx)				\
	DT_PHA_BY_IDX(node_id, prop, idx, pin)

/**
 * @brief Utility macro to initialize flags field in #pinctrl_soc_pin_t.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_BFLB_FLAGS_INIT(node_id, prop, idx)				\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, idx),			\
		    (DT_PROP_BY_IDX(node_id, prop, idx)), (0))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)				\
	{ .fun   = Z_PINCTRL_BFLB_FUN_INIT(node_id, prop, idx),			\
	  .cfg   = Z_PINCTRL_BFLB_CFG_INIT(node_id),				\
	  .pin   = Z_PINCTRL_BFLB_PIN_INIT(node_id, prop, idx),			\
	  .flags = Z_PINCTRL_BFLB_FLAGS_INIT(node_id, bflb_signals, idx),	\
	},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PROP_BY_IDX(node_id, prop, 0),	       \
				DT_FOREACH_PROP_ELEM, bflb_pins,	       \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_BFLB_COMMON_PINCTRL_SOC_H_ */
