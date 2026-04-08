/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_COMMON_POWER_H_
#define ZEPHYR_SOC_ESPRESSIF_COMMON_POWER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configure GPIO pad hold for sleep.
 *
 * Intended to be used by gpio and pinctrl drivers to enable or disable
 * the pad hold feature for a given pin.
 *
 * @param gpio_num GPIO number
 * @param enable   Enable or disable pad hold
 */
void esp32_sleep_gpio_hold_config(uint8_t gpio_num, bool enable);

/**
 * @brief Prepare GPIOs for sleep.
 *
 * This function is executed before entering sleep and performs required
 * operations such as configuring pad hold for selected pins.
 */
void esp32_sleep_gpio_prepare(void);

/**
 * @brief Restore GPIO configuration after sleep.
 *
 * Releases pad hold for pins that were configured during sleep preparation.
 */
void esp32_sleep_gpio_restore(void);

#endif /* ZEPHYR_SOC_ESPRESSIF_COMMON_POWER_H_ */
