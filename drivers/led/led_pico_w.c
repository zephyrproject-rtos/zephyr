/*
 * Copyright (c) 2025 Beechwoods Software, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_w

#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_pico_w, CONFIG_LED_LOG_LEVEL);

#include <airoc_whd_hal_common.h>
whd_interface_t airoc_wifi_get_whd_interface(void);
uint32_t whd_wifi_set_iovar_buffer(whd_interface_t ifp, const char *iovar, void *buffer,
				   uint16_t buffer_length);
static whd_interface_t airoc_if;

#define GPIO_LED_PIN 0x1
#define GPIO_LED_ON  0x1
#define GPIO_LED_OFF 0x0

static int airoc_gpio_set(int gpio_pin, int gpio_state)
{
	uint32_t parameters[] = {gpio_pin, gpio_state};
	uint32_t result;

	LOG_INF("%s: gpio_pin: %d gpio_state: %d", __func__, gpio_pin, gpio_state);

	result = whd_wifi_set_iovar_buffer(airoc_if, "gpioout", (uint8_t *)&parameters,
					   sizeof(parameters));

	if (result != WHD_SUCCESS) {
		LOG_ERR("%s: result: %d", __func__, result);
		return -1;
	}
	return 0;
}

static int led_pico_w_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	if (value > 0) {
		return airoc_gpio_set(GPIO_LED_PIN, GPIO_LED_ON);
	} else {
		return airoc_gpio_set(GPIO_LED_PIN, GPIO_LED_OFF);
	}
}

static DEVICE_API(led, led_pico_w_api) = {
	.set_brightness = led_pico_w_set_brightness,
};

static int led_pico_w_init(const struct device *dev)
{
	int err = 0;

	LOG_INF("%s: initializing", dev->name);

	airoc_if = airoc_wifi_get_whd_interface();
	if (airoc_if == NULL) {
		LOG_ERR("airoc_if is NULL");
		err = -ENODEV;
	}

	LOG_INF("%s: airoc_if: %p", dev->name, airoc_if);

	return err;
}

#define LED_PICO_W_DEFINE(i)                                                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, &led_pico_w_init, NULL, NULL, NULL, POST_KERNEL,                  \
			      CONFIG_LED_INIT_PRIORITY, &led_pico_w_api);

DT_INST_FOREACH_STATUS_OKAY(LED_PICO_W_DEFINE)
