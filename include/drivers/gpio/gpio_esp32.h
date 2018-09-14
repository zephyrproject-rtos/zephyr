/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_

static const char *gpio_esp32_get_gpio_for_pin(int pin)
{
	if (pin < 32) {
#if defined(CONFIG_GPIO_ESP32_0)
		return CONFIG_GPIO_ESP32_0_NAME;
#else
		return NULL;
#endif /* CONFIG_GPIO_ESP32_0 */
	}

#if defined(CONFIG_GPIO_ESP32_1)
	return CONFIG_GPIO_ESP32_1_NAME;
#else
	return NULL;
#endif /* CONFIG_GPIO_ESP32_1 */
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_ */
