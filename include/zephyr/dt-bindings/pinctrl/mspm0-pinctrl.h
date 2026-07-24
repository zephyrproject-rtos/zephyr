/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSPM0_DT_BINDINGS_PINCTRL_H_
#define _MSPM0_DT_BINDINGS_PINCTRL_H_

/** Pin function: analog (ADC, comparator, DAC) */
#define MSPM_PIN_FUNCTION_ANALOG (0x00000000)
/** Pin function: GPIO */
#define MSPM_PIN_FUNCTION_GPIO   (0x00000001)
/** Pin function: peripheral mux 2 */
#define MSPM_PIN_FUNCTION_2      (0x00000002)
/** Pin function: peripheral mux 3 */
#define MSPM_PIN_FUNCTION_3      (0x00000003)
/** Pin function: peripheral mux 4 */
#define MSPM_PIN_FUNCTION_4      (0x00000004)
/** Pin function: peripheral mux 5 */
#define MSPM_PIN_FUNCTION_5      (0x00000005)
/** Pin function: peripheral mux 6 */
#define MSPM_PIN_FUNCTION_6      (0x00000006)
/** Pin function: peripheral mux 7 */
#define MSPM_PIN_FUNCTION_7      (0x00000007)
/** Pin function: peripheral mux 8 */
#define MSPM_PIN_FUNCTION_8      (0x00000008)
/** Pin function: peripheral mux 9 */
#define MSPM_PIN_FUNCTION_9      (0x00000009)
/** Pin function: peripheral mux 10 */
#define MSPM_PIN_FUNCTION_10     (0x0000000A)
/** Pin function: peripheral mux 11 */
#define MSPM_PIN_FUNCTION_11     (0x0000000B)
/** Pin function: peripheral mux 12 */
#define MSPM_PIN_FUNCTION_12     (0x0000000C)
/** Pin function: peripheral mux 13 */
#define MSPM_PIN_FUNCTION_13     (0x0000000D)
/** Pin function: peripheral mux 14 */
#define MSPM_PIN_FUNCTION_14     (0x0000000E)
/** Pin function: peripheral mux 15 */
#define MSPM_PIN_FUNCTION_15     (0x0000000F)

/* Backward-compatible aliases for existing MSPM0 board/HAL DTS files */
#define MSPM0_PIN_FUNCTION_ANALOG MSPM_PIN_FUNCTION_ANALOG
#define MSPM0_PIN_FUNCTION_GPIO   MSPM_PIN_FUNCTION_GPIO
#define MSPM0_PIN_FUNCTION_2      MSPM_PIN_FUNCTION_2
#define MSPM0_PIN_FUNCTION_3      MSPM_PIN_FUNCTION_3
#define MSPM0_PIN_FUNCTION_4      MSPM_PIN_FUNCTION_4
#define MSPM0_PIN_FUNCTION_5      MSPM_PIN_FUNCTION_5
#define MSPM0_PIN_FUNCTION_6      MSPM_PIN_FUNCTION_6
#define MSPM0_PIN_FUNCTION_7      MSPM_PIN_FUNCTION_7
#define MSPM0_PIN_FUNCTION_8      MSPM_PIN_FUNCTION_8
#define MSPM0_PIN_FUNCTION_9      MSPM_PIN_FUNCTION_9
#define MSPM0_PIN_FUNCTION_10     MSPM_PIN_FUNCTION_10
#define MSPM0_PIN_FUNCTION_11     MSPM_PIN_FUNCTION_11
#define MSPM0_PIN_FUNCTION_12     MSPM_PIN_FUNCTION_12
#define MSPM0_PIN_FUNCTION_13     MSPM_PIN_FUNCTION_13
#define MSPM0_PIN_FUNCTION_14     MSPM_PIN_FUNCTION_14
#define MSPM0_PIN_FUNCTION_15     MSPM_PIN_FUNCTION_15

/*
 * Encode PINCM index (1-based from datasheet) and peripheral function
 * into a 32-bit pinmux value consumed by the MSPM pinctrl driver.
 */
#define MSP_PINMUX(pincm, function) (((pincm - 1) << 0x10) | function)

#endif /* _MSPM0_DT_BINDINGS_PINCTRL_H_ */
