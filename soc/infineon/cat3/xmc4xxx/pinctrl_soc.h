/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_INFINEON_XMC_4XXX_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_INFINEON_XMC_4XXX_PINCTRL_SOC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/xmc4xxx-pinctrl.h>

typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node, pr, idx)                                                    \
	(DT_PROP_BY_PHANDLE_IDX(node, pr, idx, pinmux) |                                           \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, bias_pull_down) << XMC4XXX_PULL_DOWN_POS |          \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, bias_pull_up) << XMC4XXX_PULL_UP_POS |              \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, drive_push_pull) << XMC4XXX_PUSH_PULL_POS |         \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, drive_open_drain) << XMC4XXX_OPEN_DRAIN_POS |       \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, output_high) << XMC4XXX_OUT_HIGH_POS |              \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, output_low) << XMC4XXX_OUT_LOW_POS |                \
	 DT_PROP_BY_PHANDLE_IDX(node, pr, idx, invert_input) << XMC4XXX_INV_INPUT_POS |            \
	 DT_ENUM_IDX(DT_PHANDLE_BY_IDX(node, pr, idx), drive_strength) << XMC4XXX_DRIVE_POS |      \
	 DT_ENUM_IDX(DT_PHANDLE_BY_IDX(node, pr, idx), hwctrl) << XMC4XXX_HWCTRL_POS),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif /* ZEPHYR_SOC_ARM_INFINEON_XMC_4XXX_PINCTRL_SOC_H_ */
