/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_NPM10XX_H_
#define ZEPHYR_DRIVERS_MFD_NPM10XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define NPM10_PIN_NUM 3U

enum mfd_npm10xx_pin_usage {
	NPM10_PIN_USAGE_UNUSED = 0U,
	/* Input/output controlled by the GPIO driver */
	NPM10_PIN_GPIO,
	/* Special outputs controlled by the PMIC */
	NPM10_PIN_IRQ,
	NPM10_PIN_POF,
	NPM10_PIN_WD_WARN,
	NPM10_PIN_WD_RST,
	NPM10_PIN_TIMER_END,
	NPM10_PIN_PWM,
	NPM10_PIN_LED,
	NPM10_PIN_BUCK,
	NPM10_PIN_VBUS_GOOD,
	NPM10_PIN_VBUS_PRESENT,
	/* Long-press reset input */
	NPM10_PIN_LP_RESET,
	/* Regulator control inputs (one pin may have multiple of these) */
	NPM10_PIN_BUCK_EN,
	NPM10_PIN_BUCK_MODE,
	NPM10_PIN_BUCK_VOUT,
	NPM10_PIN_LDSW_EN,
	NPM10_PIN_LDO_EN,

	NPM10_PIN_USAGE_MAX,
};

/**
 * @brief Configure a pin on an nPM10 device with specified usage and flags.
 *
 * This function is the only one in the npm10xx device driver set that may access the GPIO.CONFIGx,
 * GPIO.USAGEx and GPIO.CTRLx registers. The MFD driver itself and all the other npm10xx device
 * drivers must use it when configuring a pin for a specific usage to avoid conflicts. There are a
 * few rules that apply when configuring a pin:
 *
 *   - NPM10_PIN_GPIO (used by the gpio driver) may be used repeatedly to re-configure a given pin
 *   - NPM10_PIN_IRQ..NPM10_PIN_LP_RESET (used during initialisation by a number of drivers) may be
 *     used only once. Consecutive calls on the same pin will result in -EBUSY
 *   - NPM10_PIN_BUCK_EN..NPM10_PIN_LDO_EN (used by the regulator driver initialisation) may be used
 *     multiple times on a single pin to configure different controls, possibly with different
 *     active levels
 *
 * @param dev npm10xx MFD device
 * @param pin Pin index to configure [0..NPM10_PIN_NUM]
 * @param usage Pin usage (see enum mfd_npm10xx_pin_usage)
 * @param flags GPIO flags to apply to the pin's configuration
 * @return 0 on success, negative errno value on failure
 * @retval -EINVAL Invalid pin index or usage
 * @retval -EBUSY The pin has already been configured for a conflicting usage
 * @retval -EIO Any I2C input/output error
 */
int mfd_npm10xx_pin_configure(const struct device *dev, uint8_t pin, uint8_t usage,
			      gpio_flags_t flags);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MFD_NPM10XX_H_ */
