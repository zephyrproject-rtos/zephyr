/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_REALTEK_RTS5912_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_REALTEK_RTS5912_PINCTRL_H_

#include <zephyr/dt-bindings/dt-util.h>

#define REALTEK_RTS5912_GPIO_INOUT     BIT(0) /* IN/OUT : 0 input 1 output */
#define REALTEK_RTS5912_GPIO_PINON     BIT(1) /* Input_detect : 1 enable 0 disable */
#define REALTEK_RTS5912_GPIO_VOLT      BIT(2) /* Pin Volt : 1 1.8V 0 3.3V */
#define REALTEK_RTS5912_FUNC0          0      /* GPIO mode */
#define REALTEK_RTS5912_FUNC1          BIT(8) /* Function mode use BIT0~2 */
#define REALTEK_RTS5912_FUNC2          BIT(9)
#define REALTEK_RTS5912_FUNC3          ((BIT(8)) | (BIT(9)))
#define REALTEK_RTS5912_FUNC4          BIT(10)
#define REALTEK_RTS5912_GPIO_DRIVING   BIT(11) /* DRIVING : 0 low  1 high  */
#define REALTEK_RTS5912_GPIO_SLEW      BIT(12) /* SLEW : 0 fast  1 slow  */
#define REALTEK_RTS5912_GPIO_PULLDN    BIT(13) /* PULL DOWN : 0 disable 1 ebable */
#define REALTEK_RTS5912_GPIO_PULLUP    BIT(14) /* PULL UP : 0 disable 1 ebable */
#define REALTEK_RTS5912_GPIO_SCHMITTER BIT(15) /* Schmitter : 1 ebable 0 disable */
#define REALTEK_RTS5912_GPIO_TYPE      BIT(16) /* TYPE : 0 PP 1 OD */
#define REALTEK_RTS5912_GPIO_HILOW     BIT(17) /* OUTPUT : 0 low 1 high */

#define REALTEK_RTS5912_INPUT_OUTPUT_POS    0
#define REALTEK_RTS5912_INPUT_DETECTION_POS 1
#define REALTEK_RTS5912_VOLTAGE_POS         2
#define REALTEK_RTS5912_DRV_STR_POS         11
#define REALTEK_RTS5912_SLEW_RATE_POS       12
#define REALTEK_RTS5912_PD_POS              13
#define REALTEK_RTS5912_PU_POS              14
#define REALTEK_RTS5912_SCHMITTER_POS       15
#define REALTEK_RTS5912_TYPE_POS            16
#define REALTEK_RTS5912_HIGH_LOW_POS        17

#define REALTEK_RTS5912_GPIO_HIGH_POS 18
#define REALTEK_RTS5912_GPIO_HIGH_MSK 0x3f
#define REALTEK_RTS5912_GPIO_LOW_POS  3
#define REALTEK_RTS5912_GPIO_LOW_MSK  0x1f

#define REALTEK_RTS5912_PINMUX(n, f)                                                               \
	(((((n) >> 5) & REALTEK_RTS5912_GPIO_HIGH_MSK) << REALTEK_RTS5912_GPIO_HIGH_POS) |         \
	 (((n) & REALTEK_RTS5912_GPIO_LOW_MSK) << REALTEK_RTS5912_GPIO_LOW_POS) | (f))

#define REALTEK_RTS5912_PINMUX_GET_GPIO_PIN(n)                                                     \
	(((((n) >> REALTEK_RTS5912_GPIO_LOW_POS) & REALTEK_RTS5912_GPIO_LOW_MSK)) |                \
	 (((((n) >> REALTEK_RTS5912_GPIO_HIGH_POS) & REALTEK_RTS5912_GPIO_HIGH_MSK)) << 5))

#define PURE_PINMUX_MASK                   (GENMASK(31, 24) | GENMASK(17, 8) | GENMASK(2, 0))
#define REALTEK_RTS5912_GET_PURE_PINMUX(n) (n & PURE_PINMUX_MASK)

#define STD_GPIO_FUNC1                                                                             \
	((REALTEK_RTS5912_FUNC1) | (REALTEK_RTS5912_GPIO_PINON) | (REALTEK_RTS5912_GPIO_SCHMITTER))
#define STD_GPIO_FUNC2                                                                             \
	((REALTEK_RTS5912_FUNC2) | (REALTEK_RTS5912_GPIO_PINON) | (REALTEK_RTS5912_GPIO_SCHMITTER))
#define STD_GPIO_FUNC3                                                                             \
	((REALTEK_RTS5912_FUNC3) | (REALTEK_RTS5912_GPIO_PINON) | (REALTEK_RTS5912_GPIO_SCHMITTER))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_REALTEK_RTS5912_PINCTRL_H_ */
