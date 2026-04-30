/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_GPIO_H_
#define ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>

/**
 * @brief GPIO pins used to drive the target into / out of bootloader mode.
 *
 * Any pin with @c port == NULL is treated as absent and all operations on
 * it are skipped. This lets a node declare only the pins it actually
 * controls.
 */
struct stm32_bootloader_gpio_cfg {
	struct gpio_dt_spec boot0;
	struct gpio_dt_spec boot1;
	struct gpio_dt_spec nrst;
	uint16_t reset_pulse_ms;
	uint16_t boot_delay_ms;
};

/** Configure declared pins as output-inactive. */
int stm32_bootloader_gpio_init(const struct stm32_bootloader_gpio_cfg *cfg);

/**
 * @brief Drive the target into bootloader mode.
 *
 * BOOT pins are asserted (if declared), NRST is pulsed (if declared), then the caller
 * sleeps for @c boot_delay_ms to give the bootloader time to start.
 */
int stm32_bootloader_gpio_enter(const struct stm32_bootloader_gpio_cfg *cfg);

/**
 * @brief Drive the target out of bootloader mode.
 *
 * BOOT pins are de-asserted (if declared).
 * If @p reset is true and NRST is declared,
 * NRST is pulsed to hard-reset the target.
 */
int stm32_bootloader_gpio_exit(const struct stm32_bootloader_gpio_cfg *cfg, bool reset);

#endif /* ZEPHYR_DRIVERS_MISC_STM32_BOOTLOADER_GPIO_H_ */
