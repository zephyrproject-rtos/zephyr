/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RISCV_ESP32C5_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_ESP32C5_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/esp-pinctrl-common.h>

/** @cond INTERNAL_HIDDEN */

/** Type for ESP32 pin. */
typedef struct pinctrl_soc_pin {
	/** Pinmux settings (pin, direction and signal). */
	uint32_t pinmux;
	/** Pincfg settings (bias). */
	uint32_t pincfg;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_ESP32_PINMUX_INIT(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx)

#define Z_PINCTRL_ESP32_PINCFG_INIT(node_id)                                                       \
	(((ESP32_NO_PULL * DT_PROP(node_id, bias_disable)) << ESP32_PIN_BIAS_SHIFT) |              \
	 ((ESP32_PULL_UP * DT_PROP(node_id, bias_pull_up)) << ESP32_PIN_BIAS_SHIFT) |              \
	 ((ESP32_PULL_DOWN * DT_PROP(node_id, bias_pull_down)) << ESP32_PIN_BIAS_SHIFT) |          \
	 ((ESP32_PUSH_PULL * DT_PROP(node_id, drive_push_pull)) << ESP32_PIN_DRV_SHIFT) |          \
	 ((ESP32_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain)) << ESP32_PIN_DRV_SHIFT) |        \
	 ((ESP32_PIN_OUT_HIGH * DT_PROP(node_id, output_high)) << ESP32_PIN_OUT_SHIFT) |           \
	 ((ESP32_PIN_OUT_LOW * DT_PROP(node_id, output_low)) << ESP32_PIN_OUT_SHIFT) |             \
	 ((ESP32_PIN_OUT_EN * DT_PROP(node_id, output_enable)) << ESP32_PIN_EN_DIR_SHIFT) |        \
	 ((ESP32_PIN_IN_EN * DT_PROP(node_id, input_enable)) << ESP32_PIN_EN_DIR_SHIFT))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{.pinmux = Z_PINCTRL_ESP32_PINMUX_INIT(node_id, prop, idx),                                \
	 .pincfg = Z_PINCTRL_ESP32_PINCFG_INIT(node_id)},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#endif /* ZEPHYR_SOC_RISCV_ESP32C5_PINCTRL_SOC_H_ */
