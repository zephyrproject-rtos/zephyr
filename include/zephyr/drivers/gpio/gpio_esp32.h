/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_

static const char *gpio_esp32_get_gpio_for_pin(int pin)
{
	if (pin < 32) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
		return DT_LABEL(DT_INST(0, espressif_esp32_gpio));
#else
		return NULL;
#endif
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	return DT_LABEL(DT_INST(1, espressif_esp32_gpio));
#else
	return NULL;
#endif
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ESP32_H_ */
