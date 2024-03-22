/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_SOC_ARM_TI_MSPM0_M0G_PINCTRL_SOC_H__
#define __ZEPHYR_SOC_ARM_TI_MSPM0_M0G_PINCTRL_SOC_H__

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/mspm0-pinctrl.h>

#define MSP_GPIO_RESISTOR_PULL_DOWN 16
#define MSP_GPIO_RESISTOR_PULL_UP   17
#define MSP_GPIO_INPUT_ENABLE       18
#define MSP_GPIO_HYSTERESIS_ENABLED 19
#define MSP_GPIO_OPEN_DRAIN_OUTPUT  25
#define MSP_GPIO_INVERSION_ENABLED  26

#define MSP_GPIO_HIGH_DRIVE 20

#define MSP_PINMUX_INIT(node_id) DT_PROP(node_id, pinmux)

#define MSP_PIN_CONTROL_IOMUX_INIT(node_id)                                                        \
	((DT_PROP(node_id, bias_pull_up) << MSP_GPIO_RESISTOR_PULL_UP) |                           \
	 (DT_PROP(node_id, bias_pull_down) << MSP_GPIO_RESISTOR_PULL_DOWN) |                       \
	 (DT_PROP(node_id, drive_open_drain) << MSP_GPIO_OPEN_DRAIN_OUTPUT) |                      \
	 (DT_PROP(node_id, ti_hysteresis_enable) << MSP_GPIO_HYSTERESIS_ENABLED) |                 \
	 (DT_PROP(node_id, ti_invert_enable) << MSP_GPIO_INVERSION_ENABLED) |                      \
	 (DT_PROP(node_id, input_enable) << MSP_GPIO_INPUT_ENABLE))

typedef struct pinctrl_soc_pin {
	/* PINCM register index and pin function */
	uint32_t pinmux;
	/* IOMUX Pin Control Management (direction, inversion, pullups) */
	uint32_t iomux;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{.pinmux = MSP_PINMUX_INIT(DT_PROP_BY_IDX(node_id, prop, idx)),                            \
	 .iomux = MSP_PIN_CONTROL_IOMUX_INIT(DT_PROP_BY_IDX(node_id, prop, idx))},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

#endif /* __ZEPHYR_SOC_ARM_TI_MSPM0_M0G_PINCTRL_SOC_H__ */
