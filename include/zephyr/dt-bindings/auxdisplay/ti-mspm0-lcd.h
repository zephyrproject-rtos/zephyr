/*
 * Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_

/** ti,mspm0-lcd voltage-source property values */
/** External reference on R33, external resistor ladder */
#define MSPM0_LCD_VOLTAGE_SOURCE_EXT_REF_EXT_DIV     0
/** External reference on R33, internal resistor ladder */
#define MSPM0_LCD_VOLTAGE_SOURCE_EXT_REF_INT_DIV     1
/** AVDD reference, internal resistor ladder */
#define MSPM0_LCD_VOLTAGE_SOURCE_AVDD_INT_DIV        2
/** Charge pump with external supply on R33/R13 */
#define MSPM0_LCD_VOLTAGE_SOURCE_CHARGE_PUMP_EXT     3
/** Charge pump with internal VREF on R13 */
#define MSPM0_LCD_VOLTAGE_SOURCE_CHARGE_PUMP_INT_REF 4
/** AVDD on R33, external resistor ladder */
#define MSPM0_LCD_VOLTAGE_SOURCE_AVDD_EXT_DIV        5
/** Charge pump using AVDD as supply */
#define MSPM0_LCD_VOLTAGE_SOURCE_CHARGE_PUMP_AVDD    6

/** ti,mspm0-lcd bias-mode property values */
#define MSPM0_LCD_BIAS_MODE_STATIC 0
#define MSPM0_LCD_BIAS_MODE_1_3    1
#define MSPM0_LCD_BIAS_MODE_1_4    2

/** ti,mspm0-lcd mux-mode property values */
/** Mux mode static */
#define MSPM0_LCD_MUX_MODE_STATIC 0
/** Mux mode 2-mux */
#define MSPM0_LCD_MUX_MODE_2      1
/** Mux mode 3-mux */
#define MSPM0_LCD_MUX_MODE_3      2
/** Mux mode 4-mux */
#define MSPM0_LCD_MUX_MODE_4      3
/** Mux mode 5-mux */
#define MSPM0_LCD_MUX_MODE_5      4
/** Mux mode 6-mux */
#define MSPM0_LCD_MUX_MODE_6      5
/** Mux mode 7-mux */
#define MSPM0_LCD_MUX_MODE_7      6
/** Mux mode 8-mux */
#define MSPM0_LCD_MUX_MODE_8      7

/** ti,mspm0-lcd blink-rate property values */
/** Divide LCD blinking frequency by 2 */
#define MSPM0_LCD_BLINK_RATE_DIV_2   0
/** Divide LCD blinking frequency by 4 */
#define MSPM0_LCD_BLINK_RATE_DIV_4   1
/** Divide LCD blinking frequency by 8 */
#define MSPM0_LCD_BLINK_RATE_DIV_8   2
/** Divide LCD blinking frequency by 16 */
#define MSPM0_LCD_BLINK_RATE_DIV_16  3
/** Divide LCD blinking frequency by 32 */
#define MSPM0_LCD_BLINK_RATE_DIV_32  4
/** Divide LCD blinking frequency by 64 */
#define MSPM0_LCD_BLINK_RATE_DIV_64  5
/** Divide LCD blinking frequency by 128 */
#define MSPM0_LCD_BLINK_RATE_DIV_128 6
/** Divide LCD blinking frequency by 256 */
#define MSPM0_LCD_BLINK_RATE_DIV_256 7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_ */
