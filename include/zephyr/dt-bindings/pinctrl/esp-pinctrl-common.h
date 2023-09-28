/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_

#include <zephyr/dt-bindings/dt-util.h>

#define ESP32_PIN_NUM_SHIFT      0U
#define ESP32_PIN_NUM_MASK       0x3FU

/*
 * Definitions used to extract I/O
 * signal indexes used by the GPIO
 * matrix signal routing mechanism
 */
#define ESP32_PIN_SIGI_MASK      0x1FFU
#define ESP32_PIN_SIGI_SHIFT     6U
#define ESP32_PIN_SIGO_MASK      0x1FFU
#define ESP32_PIN_SIGO_SHIFT     15U
#define ESP_SIG_INVAL            ESP32_PIN_SIGI_MASK

#define ESP32_PINMUX(pin, sig_i, sig_o)						\
		(((pin & ESP32_PIN_NUM_MASK) << ESP32_PIN_NUM_SHIFT) |		\
		((sig_i & ESP32_PIN_SIGI_MASK) << ESP32_PIN_SIGI_SHIFT) |	\
		((sig_o & ESP32_PIN_SIGO_MASK) << ESP32_PIN_SIGO_SHIFT))

/*
 * Definitions used to extract pin
 * properties: bias, drive and
 * initial pin level
 */
#define ESP32_PIN_BIAS_SHIFT     0U
#define ESP32_PIN_BIAS_MASK      0x3U
#define ESP32_PIN_DRV_SHIFT      2U
#define ESP32_PIN_DRV_MASK       0x3U
#define ESP32_PIN_OUT_SHIFT      4U
#define ESP32_PIN_OUT_MASK       0x3U
#define ESP32_PIN_EN_DIR_SHIFT   6U
#define ESP32_PIN_EN_DIR_MASK    0x3U

/* Bias definitions */
#define ESP32_NO_PULL            0x1
#define ESP32_PULL_UP            0x2
#define ESP32_PULL_DOWN          0x3

/* Pin drive definitions */
#define ESP32_PUSH_PULL          0x1
#define ESP32_OPEN_DRAIN         0x2

/*
 * An output pin can be initialized
 * to either high or low
 */
#define ESP32_PIN_OUT_HIGH       0x1
#define ESP32_PIN_OUT_LOW        0x2

/*
 * Enable input or output on pin
 * regardless of its direction
 */
#define ESP32_PIN_OUT_EN       0x1
#define ESP32_PIN_IN_EN        0x2

/*
 * These flags are used by the pinctrl
 * driver, based on the DTS properties
 * assigned to a specific pin state
 */
#define ESP32_NO_PULL_FLAG       BIT(0)
#define ESP32_PULL_UP_FLAG       BIT(1)
#define ESP32_PULL_DOWN_FLAG     BIT(2)
#define ESP32_PUSH_PULL_FLAG     BIT(3)
#define ESP32_OPEN_DRAIN_FLAG    BIT(4)
#define ESP32_DIR_INP_FLAG       BIT(5)
#define ESP32_DIR_OUT_FLAG       BIT(6)
#define ESP32_PIN_OUT_HIGH_FLAG  BIT(7)
#define ESP32_PIN_OUT_LOW_FLAG   BIT(8)
#define ESP32_PIN_OUT_EN_FLAG    BIT(9)
#define ESP32_PIN_IN_EN_FLAG     BIT(10)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ESP_PINCTRL_COMMON_H_ */
