/*
 * Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_

/** @name ti,mspm0-lcd voltage-source property values */
/** @{ */
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
/** @} */

/** @name ti,mspm0-lcd bias-mode property values */
/** @{ */
#define MSPM0_LCD_BIAS_MODE_STATIC 0
#define MSPM0_LCD_BIAS_MODE_1_3    1
#define MSPM0_LCD_BIAS_MODE_1_4    2
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AUXDISPLAY_TI_MSPM0_LCD_H_ */
