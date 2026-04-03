/*
 * Copyright (c) 2022-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_ESP32_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_ESP32_COMMON_H_

/** @cond INTERNAL_HIDDEN */

/** @brief Get GPIO port index from pin number */
#define ESP32_PORT_IDX(_pin) (((_pin) < 32) ? 0 : 1)

/** @brief Extract pin number from pinmux value */
#define ESP32_PIN_NUM(_mux) (((_mux) >> ESP32_PIN_NUM_SHIFT) & ESP32_PIN_NUM_MASK)

/** @brief Extract input signal index from pinmux value */
#define ESP32_PIN_SIGI(_mux) (((_mux) >> ESP32_PIN_SIGI_SHIFT) & ESP32_PIN_SIGI_MASK)

/** @brief Extract output signal index from pinmux value */
#define ESP32_PIN_SIGO(_mux) (((_mux) >> ESP32_PIN_SIGO_SHIFT) & ESP32_PIN_SIGO_MASK)

/** @brief Extract bias configuration from pin config */
#define ESP32_PIN_BIAS(_cfg) (((_cfg) >> ESP32_PIN_BIAS_SHIFT) & ESP32_PIN_BIAS_MASK)

/** @brief Extract drive configuration from pin config */
#define ESP32_PIN_DRV(_cfg) (((_cfg) >> ESP32_PIN_DRV_SHIFT) & ESP32_PIN_DRV_MASK)

/** @brief Extract output level configuration from pin config */
#define ESP32_PIN_MODE_OUT(_cfg) (((_cfg) >> ESP32_PIN_OUT_SHIFT) & ESP32_PIN_OUT_MASK)

/** @brief Extract input/output enable override configuration */
#define ESP32_PIN_EN_DIR(_cfg) (((_cfg) >> ESP32_PIN_EN_DIR_SHIFT) & ESP32_PIN_EN_DIR_MASK)

/** @brief Extract sleep-hold configuration from pin config */
#define ESP32_PIN_SLEEP_HOLD(_cfg)                                                                 \
	(((_cfg) >> ESP32_PIN_SLEEP_HOLD_SHIFT) & ESP32_PIN_SLEEP_HOLD_MASK)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_ESP32_COMMON_H_ */
