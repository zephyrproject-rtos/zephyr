/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_

/**
 * @defgroup pinctrl_ti_am26x TI AM26x Pin Control Configuration
 * @ingroup pinctrl_interface
 * @{
 */

/**
 * @brief Extract pin mode from configuration (bits 0-3)
 * @param mode Pin mode value (0-15)
 */
#define PIN_MODE(mode) ((mode) & 0xfU)

/** @brief Override IP default to enable input */
#define PIN_FORCE_INPUT_ENABLE   ((0x1U) << 4U)
/** @brief Override IP default to disable input */
#define PIN_FORCE_INPUT_DISABLE  ((0x3U) << 4U)
/** @brief Override IP default to enable output */
#define PIN_FORCE_OUTPUT_ENABLE  ((0x1U) << 6U)
/** @brief Override IP default to disable output */
#define PIN_FORCE_OUTPUT_DISABLE ((0x3U) << 6U)

/** @brief Disable pull-up/pull-down resistor */
#define PIN_PULL_DISABLE ((0x1U) << 8U)
/** @brief Enable pull-up/pull-down resistor */
#define PIN_PULL_ENABLE  ((0x0U) << 8U)
/** @brief Configure pull-up resistor */
#define PIN_PULL_UP      ((0x1U) << 9U)
/** @brief Configure pull-down resistor */
#define PIN_PULL_DOWN    ((0x0U) << 9U)

/** @brief Configure high slew rate */
#define PIN_SLEW_RATE_HIGH ((0x0U) << 10U)
/** @brief Configure low slew rate */
#define PIN_SLEW_RATE_LOW  ((0x1U) << 10U)

/** @brief GPIO pin CPU ownership - R5SS0 core 0 */
#define PIN_GPIO_R5SS0_0 ((0x0U) << 16U)
/** @brief GPIO pin CPU ownership - R5SS0 core 1 */
#define PIN_GPIO_R5SS0_1 ((0x1U) << 16U)

/** @brief Pin qualifier - synchronous mode */
#define PIN_QUAL_SYNC    ((0x0U) << 18U)
/** @brief Pin qualifier - 3-sample qualification */
#define PIN_QUAL_3SAMPLE ((0x1U) << 18U)
/** @brief Pin qualifier - 6-sample qualification */
#define PIN_QUAL_6SAMPLE ((0x2U) << 18U)
/** @brief Pin qualifier - asynchronous mode */
#define PIN_QUAL_ASYNC   ((0x3U) << 18U)

/** @brief Invert pin logic level */
#define PIN_INVERT     ((0x1U) << 20U)
/** @brief Do not invert pin logic level */
#define PIN_NON_INVERT ((0x0U) << 20U)

/**
 * @brief Calculate GPIO pin offset
 * @param num GPIO pin number
 */
#define PIN_GPIO(num) ((num) * 4)

/**
 * @brief Construct an AM26x pinmux configuration value
 * @param gpio_num GPIO pin number
 * @param value Pin configuration value (mode, pull, slew rate, etc.)
 */
#define TI_AM26X_PINMUX(gpio_num, value) (gpio_num) (value)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_TI_AM26X_PINCTRL_H_ */
