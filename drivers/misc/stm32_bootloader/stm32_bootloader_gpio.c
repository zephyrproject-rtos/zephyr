/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 bootloader GPIO control.
 *
 * Drives the BOOT0, BOOT1 and NRST pins used to enter and exit the
 * STM32 system bootloader. Any pin whose @c port is NULL is treated as
 * absent and silently skipped, so call sites do not need to branch on
 * optional pins.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "stm32_bootloader_gpio.h"

LOG_MODULE_REGISTER(stm32_sys_bl, CONFIG_STM32_BOOTLOADER_LOG_LEVEL);

/**
 * @brief Test whether a GPIO is declared in the configuration.
 *
 * @param gpio_dt_spec_ptr Pointer to a @c gpio_dt_spec.
 * @return @c true if the pin has a non-NULL port, @c false otherwise.
 */
#define PIN_PRESENT(gpio_dt_spec_ptr) ((gpio_dt_spec_ptr)->port != NULL)

/**
 * @brief Configure a pin as output-inactive, skipping absent pins.
 *
 * Expands to a statement expression that yields an @c int errno value so
 * it can be used as @c ret = PIN_CONFIGURE(...).
 *
 * @param p    Pointer to a @c gpio_dt_spec.
 * @param name Human-readable pin name, used for log messages.
 * @return 0 on success or if the pin is absent, negative errno otherwise.
 */
#define PIN_CONFIGURE(p, name)                                                                     \
	({                                                                                         \
		int _ret_ = 0;                                                                     \
		if (PIN_PRESENT(p)) {                                                              \
			if (!gpio_is_ready_dt(p)) {                                                \
				LOG_ERR("%s GPIO not ready", name);                                \
				_ret_ = -ENODEV;                                                   \
			} else {                                                                   \
				_ret_ = gpio_pin_configure_dt(p, GPIO_OUTPUT_INACTIVE);            \
				if (_ret_ < 0) {                                                   \
					LOG_ERR("Failed to configure %s GPIO: %d", name, _ret_);   \
				}                                                                  \
			}                                                                          \
		}                                                                                  \
		_ret_;                                                                             \
	})

/**
 * @brief Drive a pin to @p value, skipping absent pins.
 *
 * Expands to a statement expression that yields an @c int errno value so
 * it can be used as @c ret = PIN_SET(...).
 *
 * @param p     Pointer to a @c gpio_dt_spec.
 * @param value Logical level to drive (int 0 or 1).
 * @param name  Human-readable pin name, used for log messages.
 * @return 0 on success or if the pin is absent, negative errno otherwise.
 */
#define PIN_SET(p, value, name)                                                                    \
	({                                                                                         \
		int _ret_ = 0;                                                                     \
		if (PIN_PRESENT(p)) {                                                              \
			_ret_ = gpio_pin_set_dt(p, value);                                         \
			if (_ret_ < 0) {                                                           \
				LOG_ERR("Failed to set %s GPIO: %d", name, _ret_);                 \
			}                                                                          \
		}                                                                                  \
		_ret_;                                                                             \
	})

int stm32_bootloader_gpio_init(const struct stm32_bootloader_gpio_cfg *cfg)
{
	int ret;

	ret = PIN_CONFIGURE(&cfg->boot0, "BOOT0");
	if (ret < 0) {
		return ret;
	}
	ret = PIN_CONFIGURE(&cfg->boot1, "BOOT1");
	if (ret < 0) {
		return ret;
	}
	return PIN_CONFIGURE(&cfg->nrst, "NRST");
}

int stm32_bootloader_gpio_enter(const struct stm32_bootloader_gpio_cfg *cfg)
{
	int ret;

	ret = PIN_SET(&cfg->boot0, 1, "BOOT0");
	if (ret < 0) {
		return ret;
	}
	ret = PIN_SET(&cfg->boot1, 1, "BOOT1");
	if (ret < 0) {
		return ret;
	}

	if (PIN_PRESENT(&cfg->nrst)) {
		ret = PIN_SET(&cfg->nrst, 1, "NRST");
		if (ret < 0) {
			return ret;
		}
		k_msleep(cfg->reset_pulse_ms);
		ret = PIN_SET(&cfg->nrst, 0, "NRST");
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->boot_delay_ms) {
		k_msleep(cfg->boot_delay_ms);
	}
	return 0;
}

int stm32_bootloader_gpio_exit(const struct stm32_bootloader_gpio_cfg *cfg, bool reset)
{
	int ret;

	(void)PIN_SET(&cfg->boot0, 0, "BOOT0");
	(void)PIN_SET(&cfg->boot1, 0, "BOOT1");

	if (reset && PIN_PRESENT(&cfg->nrst)) {
		ret = PIN_SET(&cfg->nrst, 1, "NRST");
		if (ret < 0) {
			return ret;
		}
		k_msleep(cfg->reset_pulse_ms);
		ret = PIN_SET(&cfg->nrst, 0, "NRST");
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}
