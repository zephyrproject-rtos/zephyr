/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_

/** Enable input detect */
#define RTS5912_GPIO_INDETEN  BIT(8)
/** Set pin driving current */
#define RTS5912_GPIO_OUTDRV   BIT(9)
/** Set GPIO slew rate */
#define RTS5912_GPIO_SLEWRATE BIT(10)
/** Enable Schmitt-trigger */
#define RTS5912_GPIO_SCHEN    BIT(11)

#define RTS5912_GPIO_VOLTAGE_POS     12
#define RTS5912_GPIO_VOLTAGE_MASK    GENMASK(13, 12)
/** Set pin at the default voltage level */
#define RTS5912_GPIO_VOLTAGE_DEFAULT (0U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define RTS5912_GPIO_VOLTAGE_1V8     (1U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define RTS5912_GPIO_VOLTAGE_3V3     (2U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define RTS5912_GPIO_VOLTAGE_5V0     (3U << RTS5912_GPIO_VOLTAGE_POS)

/** Multi function */
#define RTS5912_GPIO_MFCTRL_POS  14
#define RTS5912_GPIO_MFCTRL_MASK GENMASK(15, 14)
/** 0x00:Function 0 0x01: Function 1 0x02: Function 2 0x03: Function 3 */
#define RTS5912_GPIO_MFCTRL_0    (0U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_1    (1U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_2    (2U << RTS5912_GPIO_MFCTRL_POS)
#define RTS5912_GPIO_MFCTRL_3    (3U << RTS5912_GPIO_MFCTRL_POS)
/** Interrupt Mask since rts5912 does not support GPIO_INT_LEVELS_LOGICAL*/
#define RTS5912_GPIO_INTR_MASK   (1U << 21 | 1U << 22 | 1U << 24 | 1U << 25 | 1U << 26)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_ */
