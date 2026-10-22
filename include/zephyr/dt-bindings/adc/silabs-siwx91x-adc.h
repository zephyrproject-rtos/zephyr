/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SILABS_SIWX91X_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SILABS_SIWX91X_ADC_H_

/*
 * These macros define the positive input selection values used in
 * zephyr,input-positive  for the SiWx91x ADC driver to map GPIO
 * pins to ADC channels.
 *
 * Mapping:
 * - ULP GPIO (0-15): Ultra Low Power GPIO pins
 * - HP GPIO (25-30): High Power GPIO pins
 *
 * Usage in device tree:
 * @code{.dts}
 *   zephyr,input-positive = <SIWX91X_ADC_INPUT_ULP4>;  // Maps to ULP4 pin
 * @endcode
 *
 * Note: Pin configuration (pinctrl) must be set separately in device tree
 *       to enable analog mode on the GPIO pin.
 */

#define SIWX91X_ADC_INPUT_ULP0      0
#define SIWX91X_ADC_INPUT_ULP2      1
#define SIWX91X_ADC_INPUT_ULP4      2
#define SIWX91X_ADC_INPUT_ULP6      3
#define SIWX91X_ADC_INPUT_ULP8      4
#define SIWX91X_ADC_INPUT_ULP10     5
#define SIWX91X_ADC_INPUT_HP25      6
#define SIWX91X_ADC_INPUT_HP27      7
#define SIWX91X_ADC_INPUT_HP29      8
#define SIWX91X_ADC_INPUT_ULP1      10
#define SIWX91X_ADC_INPUT_ULP3      11
#define SIWX91X_ADC_INPUT_ULP5      12
#define SIWX91X_ADC_INPUT_ULP11     13
#define SIWX91X_ADC_INPUT_ULP9      14
#define SIWX91X_ADC_INPUT_ULP7      15
#define SIWX91X_ADC_INPUT_HP26      16
#define SIWX91X_ADC_INPUT_HP28      17
#define SIWX91X_ADC_INPUT_HP30      18

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SILABS_SIWX91X_ADC_H_ */
