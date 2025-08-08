/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * HC32 SoC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_ARM_XHSC_HC32_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_XHSC_HC32_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/hc32-pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type for HC32 pin. */
typedef uint32_t pinctrl_soc_pin_t;

/**
 * @brief Utility macro to initialize pinmux field.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_HC32_PINCFG(node_id)				       \
	(((HC32_NO_PULL * DT_PROP(node_id, bias_disable)) << HC32_PUR_SHIFT) | \
	 ((HC32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << HC32_PUR_SHIFT) | \
	 ((HC32_PUSH_PULL * DT_PROP(node_id, drive_push_pull)) << HC32_NOD_SHIFT) | \
	 ((HC32_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << HC32_NOD_SHIFT) | \
	 ((HC32_OUTPUT_LOW * DT_PROP(node_id, output_low)) << HC32_OTYPE_SHIFT) | \
	 ((HC32_OUTPUT_HIGH * DT_PROP(node_id, output_high)) << HC32_OTYPE_SHIFT) | \
	 ((HC32_INPUT_ENABLE * DT_PROP(node_id, input_enable)) << HC32_DIREN_SHIFT) | \
	 ((HC32_OUTPUT_ENABLE * DT_PROP(node_id, output_enable)) << HC32_DIREN_SHIFT) | \
	 (DT_ENUM_IDX(node_id, drive_strength) << HC32_STRENGTH_SHIFT))

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)			       \
	DT_PROP_BY_IDX(node_id, prop, idx) | Z_PINCTRL_HC32_PINCFG(node_id),

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		       \
				DT_FOREACH_PROP_ELEM, pinmux,		       \
				Z_PINCTRL_STATE_PIN_INIT)}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_XHSC_HC32_COMMON_PINCTRL_SOC_H_ */
