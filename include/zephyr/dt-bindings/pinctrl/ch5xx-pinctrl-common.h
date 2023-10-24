/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DT_CH5XX_PINCTRL_COMMON_H_
#define ZEPHYR_DT_CH5XX_PINCTRL_COMMON_H_

#include <zephyr/dt-bindings/dt-util.h>

#define CH5XX_PINMUX_PORT_S (0)
#define CH5XX_PINMUX_PORT_M BIT_MASK(8)

#define CH5XX_PINMUX_PIN_S (8)
#define CH5XX_PINMUX_PIN_M BIT_MASK(8)

#define CH5XX_PINMUX_REMAP_BIT_S (16)
#define CH5XX_PINMUX_REMAP_BIT_M BIT_MASK(8)

#define CH5XX_PINMUX_REMAP_EN_S (24)
#define CH5XX_PINMUX_REMAP_EN_M BIT_MASK(1)

#define CH5XX_PINMUX(port, pin)                                                                    \
	((((port - 'A') & CH5XX_PINMUX_PORT_M) << CH5XX_PINMUX_PORT_S) |                           \
	 ((pin & CH5XX_PINMUX_PIN_M) << CH5XX_PINMUX_PIN_S))

#define CH5XX_PINMUX_REMAP(port, pin, bit, en)                                                     \
	(CH5XX_PINMUX(port, pin) |                                                                 \
	 ((bit & CH5XX_PINMUX_REMAP_BIT_M) << CH5XX_PINMUX_REMAP_BIT_S) |                          \
	 ((CH5XX_PINMUX_REMAP_EN_##en) << CH5XX_PINMUX_REMAP_EN_S))

#define CH5XX_PINMUX_REMAP_EN_DEFAULT (0)
#define CH5XX_PINMUX_REMAP_EN_REMAP   (1)

#endif /* ZEPHYR_DT_CH5XX_PINCTRL_COMMON_H_ */
