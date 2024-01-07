/*
 * Copyright (c) 2023 Martin Kiepfer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_AXP192_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_AXP192_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO function type. Only one function can be configured per GPIO.
 */
enum axp192_gpio_func {
	AXP192_GPIO_FUNC_INPUT = BIT(0),
	AXP192_GPIO_FUNC_OUTPUT_OD = BIT(1),
	AXP192_GPIO_FUNC_OUTPUT_LOW = BIT(2),
	AXP192_GPIO_FUNC_LDO = BIT(3),
	AXP192_GPIO_FUNC_ADC = BIT(4),
	AXP192_GPIO_FUNC_PWM = BIT(5),
	AXP192_GPIO_FUNC_FLOAT = BIT(6),
	AXP192_GPIO_FUNC_CHARGE_CTL = BIT(7),
	AXP192_GPIO_FUNC_INVALID
};

/**
 * @brief Check if a given GPIO function value is valid.
 */
#define AXP192_GPIO_FUNC_VALID(func) (func < AXP192_GPIO_FUNC_INVALID)

/**
 * @brief Maximum number of GPIOs supported by AXP192 PMIC.
 */
#define AXP192_GPIO_MAX_NUM 6U

/**
 * @defgroup mdf_interface_axp192 MFD AXP192 interface
 *
 * Pins of AXP192 support several different functions. The mfd interface offers
 * an API to configure and control these different functions.
 *
 * The 6 GPIOS are mapped as follows:
 *  [0]: GPIO0
 *  [1]: GPIO1
 *  [2]: GPIO2
 *  [3]: GPIO3
 *  [4]: GPIO4
 *  [5]: EXTEN
 *
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Request a GPIO pin to be configured to a specific function. GPIO0..5
 * of AXP192 feature various functions (see @ref axp192_gpio_func for details).
 * A GPIO can only be used by one driver instance. Subsequential calls on the
 * same GPIO will overwrite according function.
 *
 * @param dev axp192 mfd device
 * @param client_dev client device the gpio is used in
 * @param gpio GPIO to be configured (0..5)
 * @param func Function to be configured (see @ref axp192_gpio_func for details)
 * @retval 0 on success
 * @retval -EINVAL if an invalid GPIO number is passed
 * @retval -ENOTSUP if the requested function is not supported by the given
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_func_ctrl(const struct device *dev, const struct device *client_dev,
			      uint8_t gpio, enum axp192_gpio_func func);

/**
 * @brief Read out current configuration of a specific GPIO pin.
 *
 * @param dev axp192 mfd device
 * @param gpio GPIO to read configuration from
 * @param func Pointer to store current function configuration in.
 * @return 0 on success
 * @retval -EINVAL if an invalid GPIO number is passed
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_func_get(const struct device *dev, uint8_t gpio, enum axp192_gpio_func *func);

/**
 * @brief Enable pull-down on specified GPIO pin. AXP192 only supports
 * pull-down on GPIO3..5. Pull-ups are not supprted.
 *
 * @param dev axp192 mfd device
 * @param gpio GPIO to control pull-downs
 * @param enable true to enable, false to disable pull-down
 * @retval 0 on success
 * @retval -EINVAL if an invalid argument is given (e.g. invalid GPIO number)
 * @retval -ENOTSUP if pull-down is not supported by the givenn GPIO
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_pd_ctrl(const struct device *dev, uint8_t gpio, bool enable);

/**
 * @brief Read out the current pull-down configuration of a specific GPIO.
 *
 * @param dev axp192 mfd device
 * @param gpio GPIO to control pull-downs
 * @param enabled Pointer to current pull-down configuration (true: pull-down
 * enabled/ false: pull-down disabled)
 * @retval -EINVAL if an invalid argument is given (e.g. invalid GPIO number)
 * @retval -ENOTSUP if pull-down is not supported by the givenn GPIO
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_pd_get(const struct device *dev, uint8_t gpio, bool *enabled);

/**
 * @brief Read GPIO port.
 *
 * @param dev axp192 mfd device
 * @param value Pointer to port value
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_read_port(const struct device *dev, uint8_t *value);

/**
 * @brief Write GPIO port.
 *
 * @param dev axp192 mfd device
 * @param value port value
 * @param mask pin mask within the port
 * @retval 0 on success
 * @retval -errno in case of any bus error
 */
int mfd_axp192_gpio_write_port(const struct device *dev, uint8_t value, uint8_t mask);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_AXP192_H_ */
