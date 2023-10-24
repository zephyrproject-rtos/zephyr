/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DT_CH56X_PINCTRL_H_
#define ZEPHYR_DT_CH56X_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>
#include <zephyr/dt-bindings/pinctrl/ch5xx-pinctrl-common.h>

#define CH56X_PINMUX(port, pin) CH5XX_PINMUX(port, pin)

#define CH56X_PINMUX_REMAP(port, pin, func, remap)                                                 \
	CH5XX_PINMUX_REMAP(port, pin, CH56X_PINMUX_REMAP_##func, remap)

#define CH56X_PINMUX_REMAP_MII   0
#define CH56X_PINMUX_REMAP_TMR1  1
#define CH56X_PINMUX_REMAP_TMR2  2
#define CH56X_PINMUX_REMAP_UART0 4

#endif /* ZEPHYR_DT_CH56X_PINCTRL_H_ */
