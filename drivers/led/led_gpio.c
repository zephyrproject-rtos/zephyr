/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_leds

/**
 * @file
 * @brief GPIO driven LEDs
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_LOG_LEVEL);

struct led_gpio_config {
	size_t num_leds;
	const struct gpio_dt_spec *led;
};

#ifdef CONFIG_LED_GPIO_BLINK
struct led_gpio_blink_data {
	uint32_t delay_on;
	uint32_t delay_off;
	struct k_timer timer;
};
#endif /* CONFIG_LED_GPIO_BLINK */

struct led_gpio_led_data {
	bool is_on;
#ifdef CONFIG_LED_GPIO_BLINK
	struct led_gpio_blink_data blink;
#endif /* CONFIG_LED_GPIO_BLINK */
};

struct led_gpio_data {
	struct led_gpio_led_data *led;
};

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led_gpio;
	struct led_gpio_data *data = dev->data;

	if ((led >= config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	led_gpio = &config->led[led];
	data->led[led].is_on = value > 0;

	return gpio_pin_set_dt(led_gpio, value > 0);
}

static int led_gpio_on(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 100);
}

static int led_gpio_off(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 0);
}

#ifdef CONFIG_LED_GPIO_BLINK
static int led_gpio_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			  uint32_t delay_off)
{
	const struct led_gpio_config *config = dev->config;
	struct led_gpio_data *data = dev->data;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	data->led[led].blink.delay_on = delay_on;
	data->led[led].blink.delay_off = delay_off;
	if ((delay_on != 0) && (delay_off != 0)) {
		k_timer_start(&data->led[led].blink.timer, K_MSEC(delay_on), K_NO_WAIT);
	} else {
		k_timer_stop(&data->led[led].blink.timer);
	}

	return 0;
}

static void blink_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct led_gpio_config *config = dev->config;
	struct led_gpio_data *data = dev->data;
	struct led_gpio_blink_data *blink_data =
		CONTAINER_OF(timer, struct led_gpio_blink_data, timer);
	struct led_gpio_led_data *led_data =
		CONTAINER_OF(blink_data, struct led_gpio_led_data, blink);
	uint32_t led = ARRAY_INDEX(data->led, led_data);

	if (led >= config->num_leds) {
		return;
	}

	if (led_data->is_on) {
		led_gpio_off(dev, led);
		k_timer_start(timer, K_MSEC(led_data->blink.delay_off), K_NO_WAIT);
	} else {
		led_gpio_on(dev, led);
		k_timer_start(timer, K_MSEC(led_data->blink.delay_on), K_NO_WAIT);
	}
}
#endif /* CONFIG_LED_GPIO_BLINK */

static int led_gpio_init(const struct device *dev)
{
	const struct led_gpio_config *config = dev->config;
#ifdef CONFIG_LED_GPIO_BLINK
	struct led_gpio_data *data = dev->data;
#endif /* CONFIG_LED_GPIO_BLINK */
	int err = 0;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	for (size_t i = 0; (i < config->num_leds) && !err; i++) {
		const struct gpio_dt_spec *led = &config->led[i];

		if (device_is_ready(led->port)) {
			err = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);

			if (err) {
				LOG_ERR("Cannot configure GPIO (err %d)", err);
			}
		} else {
			LOG_ERR("%s: GPIO device not ready", dev->name);
			err = -ENODEV;
		}

#ifdef CONFIG_LED_GPIO_BLINK
		k_timer_init(&data->led[i].blink.timer, blink_timeout, NULL);
		k_timer_user_data_set(&data->led[i].blink.timer, (void *)dev);
#endif /* CONFIG_LED_GPIO_BLINK */
	}

	return err;
}

static const struct led_driver_api led_gpio_api = {
	.on		= led_gpio_on,
	.off		= led_gpio_off,
#ifdef CONFIG_LED_GPIO_BLINK
	.blink		= led_gpio_blink,
#endif /* CONFIG_LED_GPIO_BLINK */
	.set_brightness	= led_gpio_set_brightness,
};

#define LED_GPIO_DEVICE(i)					\
								\
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {		\
	DT_INST_FOREACH_CHILD_SEP_VARGS(i, GPIO_DT_SPEC_GET, (,), gpios) \
};								\
								\
static const struct led_gpio_config led_gpio_config_##i = {	\
	.num_leds	= ARRAY_SIZE(gpio_dt_spec_##i),	\
	.led		= gpio_dt_spec_##i,			\
};								\
								\
static struct led_gpio_led_data led_gpio_led_data_##i[ARRAY_SIZE(gpio_dt_spec_##i)] = {	\
	{.is_on = false},			\
};								\
								\
static struct led_gpio_data led_gpio_data_##i = {	\
	.led = led_gpio_led_data_##i,	\
};								\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
		      &led_gpio_data_##i, &led_gpio_config_##i,		\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
