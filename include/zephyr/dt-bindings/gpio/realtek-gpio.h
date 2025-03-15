/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_

#define RTS5912_GPIO_VOLTAGE_POS  11
#define RTS5912_GPIO_VOLTAGE_MASK (3U << RTS5912_GPIO_VOLTAGE_POS)

/** Set pin at the default voltage level */
#define RTS5912_GPIO_VOLTAGE_DEFAULT (0U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define RTS5912_GPIO_VOLTAGE_1V8     (1U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define RTS5912_GPIO_VOLTAGE_3V3     (2U << RTS5912_GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define RTS5912_GPIO_VOLTAGE_5V0     (3U << RTS5912_GPIO_VOLTAGE_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_ */
