/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_

#define GPIO_VOLTAGE_POS  11
#define GPIO_VOLTAGE_MASK (3U << GPIO_VOLTAGE_POS)

/** Set pin at the default voltage level */
#define GPIO_VOLTAGE_DEFAULT (0U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 1.8 V */
#define GPIO_VOLTAGE_1P8     (1U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 3.3 V */
#define GPIO_VOLTAGE_3P3     (2U << GPIO_VOLTAGE_POS)
/** Set pin voltage level at 5.0 V */
#define GPIO_VOLTAGE_5P0     (3U << GPIO_VOLTAGE_POS)

#define GPIO_NONE_RESISTOR      0
#define GPIO_PULL_UP_RESISTOR   1
#define GPIO_PULL_DOWN_RESISTOR 2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_REALTEK_GPIO_H_ */
