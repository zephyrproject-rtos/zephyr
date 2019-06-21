/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_

static const char *gpio_esp32_get_gpio_for_pin(int pin)
{
	if (pin < 32) {
#if defined(CONFIG_GPIO_ESP32_0)
		return DT_INST_0_ESPRESSIF_ESP32_GPIO_LABEL;
#else
		return NULL;
#endif /* CONFIG_GPIO_ESP32_0 */
	}

#if defined(CONFIG_GPIO_ESP32_1)
	return DT_INST_1_ESPRESSIF_ESP32_GPIO_LABEL;
#else
	return NULL;
#endif /* CONFIG_GPIO_ESP32_1 */
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_ */
