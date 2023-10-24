/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32V_PINCTRL_SOC_CH5XX_H__
#define __CH32V_PINCTRL_SOC_CH5XX_H__

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/dt-bindings/pinctrl/ch5xx-pinctrl-common.h>

struct pinctrl_soc_pin {
	uint8_t port;
	uint8_t pin;
	uint8_t remap_bit;
	uint8_t remap_en;
	uint32_t flags;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define CH5XX_PINMUX_GET(pinmux, field)                                                            \
	((pinmux >> CH5XX_PINMUX_##field##_S) & CH5XX_PINMUX_##field##_M)

#define CH5XX_FLAGS_INPUT(val)      COND_CODE_1(val, (GPIO_INPUT), (0))
#define CH5XX_FLAGS_OUTPUT(val)     COND_CODE_1(val, (GPIO_OUTPUT), (0))
#define CH5XX_FLAGS_PUSH_PULL(val)  COND_CODE_1(val, (GPIO_PUSH_PULL), (0))
#define CH5XX_FLAGS_OPEN_DRAIN(val) COND_CODE_1(val, (GPIO_OPEN_DRAIN), (0))
#define CH5XX_FLAGS_PULL_UP(val)    COND_CODE_1(val, (GPIO_PULL_UP), (0))
#define CH5XX_FLAGS_PULL_DOWN(val)  COND_CODE_1(val, (GPIO_PULL_DOWN), (0))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{                                                                                          \
		.port = CH5XX_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), PORT),                \
		.pin = CH5XX_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), PIN),                  \
		.remap_bit = CH5XX_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), REMAP_BIT),      \
		.remap_en = CH5XX_PINMUX_GET(DT_PROP_BY_IDX(node_id, prop, idx), REMAP_EN),        \
		.flags = CH5XX_FLAGS_PULL_UP(DT_PROP(node_id, bias_pull_up)) |                     \
			 CH5XX_FLAGS_PULL_DOWN(DT_PROP(node_id, bias_pull_down)) |                 \
			 CH5XX_FLAGS_PUSH_PULL(DT_PROP(node_id, drive_push_pull)) |                \
			 CH5XX_FLAGS_OPEN_DRAIN(DT_PROP(node_id, drive_open_drain)) |              \
			 CH5XX_FLAGS_INPUT(DT_PROP(node_id, input_enable)) |                       \
			 CH5XX_FLAGS_OUTPUT(DT_PROP(node_id, output_enable)) |                     \
			 DT_PROP_OR(node_id, gpio_flags, 0),                                       \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* __CH32V_PINCTRL_SOC_CH5XX_H__ */
