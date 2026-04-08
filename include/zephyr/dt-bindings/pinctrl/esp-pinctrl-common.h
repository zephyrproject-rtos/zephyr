/*
 * Copyright (c) 2022-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_

#include <zephyr/dt-bindings/dt-util.h>

/** @brief GPIO pin number field */
#define ESP32_PIN_NUM_SHIFT 0U
#define ESP32_PIN_NUM_MASK  0x3FU

/**
 * @brief Definitions used to extract I/O signal indexes.
 *
 * These fields encode the GPIO matrix signal routing mechanism,
 * allowing internal peripheral signals to be connected to GPIO pins.
 */
#define ESP32_PIN_SIGI_MASK  0x1FFU
#define ESP32_PIN_SIGI_SHIFT 6U
#define ESP32_PIN_SIGO_MASK  0x1FFU
#define ESP32_PIN_SIGO_SHIFT 15U
#define ESP_SIG_INVAL        ESP32_PIN_SIGI_MASK

/**
 * @brief Construct ESP32 pinmux value from pin number and GPIO matrix signals
 *
 * @param pin    GPIO pin number
 * @param sig_i  Input signal index
 * @param sig_o  Output signal index
 */
#define ESP32_PINMUX(pin, sig_i, sig_o)                                                            \
	(((pin & ESP32_PIN_NUM_MASK) << ESP32_PIN_NUM_SHIFT) |                                     \
	 ((sig_i & ESP32_PIN_SIGI_MASK) << ESP32_PIN_SIGI_SHIFT) |                                 \
	 ((sig_o & ESP32_PIN_SIGO_MASK) << ESP32_PIN_SIGO_SHIFT))

/**
 * @brief ESP32 pin configuration bit field.
 *
 * This field encodes pin configuration options used by the ESP32
 * pinctrl driver.
 *
 * Fields:
 *
 * - bias         [ 0 : 1 ]
 * - drive        [ 2 : 3 ]
 * - output level [ 4 : 5 ]
 * - input/output enable override [ 6 : 7 ]
 * - sleep hold   [ 8 ]
 *
 * These values are combined into a single integer and later
 * translated into GPIO configuration flags by the driver.
 */

/**
 * @name ESP32 pin configuration bit field layout
 * @{
 */
#define ESP32_PIN_BIAS_SHIFT       0U
#define ESP32_PIN_BIAS_MASK        0x3U
#define ESP32_PIN_DRV_SHIFT        2U
#define ESP32_PIN_DRV_MASK         0x3U
#define ESP32_PIN_OUT_SHIFT        4U
#define ESP32_PIN_OUT_MASK         0x3U
#define ESP32_PIN_EN_DIR_SHIFT     6U
#define ESP32_PIN_EN_DIR_MASK      0x3U
#define ESP32_PIN_SLEEP_HOLD_SHIFT 8U   /**< Sleep hold bit */
#define ESP32_PIN_SLEEP_HOLD_MASK  0x1U /**< Sleep hold mask */
/** @} */

/**
 * @name ESP32 pin bias configuration values
 * @{
 */
#define ESP32_NO_PULL   0x1
#define ESP32_PULL_UP   0x2
#define ESP32_PULL_DOWN 0x3
/** @} */

/**
 * @name ESP32 pin drive configuration values
 * @{
 */
#define ESP32_PUSH_PULL  0x1
#define ESP32_OPEN_DRAIN 0x2
/** @} */

/**
 * @name ESP32 pin output level values
 * @{
 */
#define ESP32_PIN_OUT_HIGH 0x1
#define ESP32_PIN_OUT_LOW  0x2
/** @} */

/**
 * @name ESP32 input/output enable override values
 * @{
 */
#define ESP32_PIN_OUT_EN 0x1
#define ESP32_PIN_IN_EN  0x2
/** @} */

/**
 * @name ESP32 sleep hold configuration
 * @{
 */
#define ESP32_PIN_SLEEP_HOLD_EN 0x1 /**< Sleep hold enable */
/** @} */

/**
 * @brief Internal flags used by the ESP32 pinctrl driver.
 *
 * These flags are derived from the pin configuration bit field
 * and control the GPIO driver behavior.
 *
 * @name ESP32 pinctrl internal flags
 * @{
 */
#define ESP32_NO_PULL_FLAG      BIT(0)
#define ESP32_PULL_UP_FLAG      BIT(1)
#define ESP32_PULL_DOWN_FLAG    BIT(2)
#define ESP32_PUSH_PULL_FLAG    BIT(3)
#define ESP32_OPEN_DRAIN_FLAG   BIT(4)
#define ESP32_DIR_INP_FLAG      BIT(5)
#define ESP32_DIR_OUT_FLAG      BIT(6)
#define ESP32_PIN_OUT_HIGH_FLAG BIT(7)
#define ESP32_PIN_OUT_LOW_FLAG  BIT(8)
#define ESP32_PIN_OUT_EN_FLAG   BIT(9)
#define ESP32_PIN_IN_EN_FLAG    BIT(10)
#define ESP32_SLEEP_HOLD_FLAG   BIT(11) /**< Sleep hold enable bit */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_ */
