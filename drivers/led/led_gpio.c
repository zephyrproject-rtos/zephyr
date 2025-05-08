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
#define MAX_GPIO_LEDS 32
struct led_gpio_data {
	const struct led_gpio_config *config;
	struct k_timer *blink_timer;
	uint32_t *delay_on;
	uint32_t *delay_off;
	uint32_t blink_state;
};

static void led_gpio_timer_cb(struct k_timer *timer_id)
{
	struct led_gpio_data *data = k_timer_user_data_get(timer_id);
	const struct led_gpio_config *config = data->config;
	int led = timer_id - data->blink_timer;
	const struct gpio_dt_spec *led_gpio = &config->led[led];

	data->blink_state ^= 1 << led;
	gpio_pin_set_dt(led_gpio, data->blink_state & (1 << led));
	if (data->blink_state & (1 << led)) {
		k_timer_start(&data->blink_timer[led], K_MSEC(data->delay_on[led]), K_NO_WAIT);
	} else {
		k_timer_start(&data->blink_timer[led], K_MSEC(data->delay_off[led]), K_NO_WAIT);
	}
}

static int led_gpio_blink(const struct device *dev, uint32_t led,
			  uint32_t delay_on, uint32_t delay_off)
{
	struct led_gpio_data *data = dev->data;

	data->blink_state = 1 << led;
	data->delay_on[led] = delay_on;
	data->delay_off[led] = delay_off;

	k_timer_start(&data->blink_timer[led], K_MSEC(delay_on), K_NO_WAIT);

	return 0;
}

static void led_gpio_stop_blink(const struct device *dev, uint32_t led)
{
	struct led_gpio_data *data = dev->data;

	k_timer_stop(&data->blink_timer[led]);
}

static void led_gpio_blink_init(const struct device *dev, uint32_t led)
{
	struct led_gpio_data *data = dev->data;

	k_timer_init(&data->blink_timer[led], led_gpio_timer_cb, NULL);
	k_timer_user_data_set(&data->blink_timer[led], (void *)data);
}
#else
static void led_gpio_stop_blink(const struct device *dev, uint32_t led) {}
static void led_gpio_blink_init(const struct device *dev, uint32_t led) {}
#endif /* CONFIG_LED_GPIO_BLINK */

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{

	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led_gpio;

	if ((led >= config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	led_gpio = &config->led[led];

	return gpio_pin_set_dt(led_gpio, value > 0);
}

static int led_gpio_on(const struct device *dev, uint32_t led)
{
	led_gpio_stop_blink(dev, led);
	return led_gpio_set_brightness(dev, led, 100);
}

static int led_gpio_off(const struct device *dev, uint32_t led)
{
	led_gpio_stop_blink(dev, led);
	return led_gpio_set_brightness(dev, led, 0);
}

static int led_gpio_init(const struct device *dev)
{
	const struct led_gpio_config *config = dev->config;
	int err = 0;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	for (size_t i = 0; (i < config->num_leds) && !err; i++) {
		const struct gpio_dt_spec *led = &config->led[i];

		led_gpio_blink_init(dev, i);
		if (device_is_ready(led->port)) {
			err = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);

			if (err) {
				LOG_ERR("Cannot configure GPIO (err %d)", err);
			}
		} else {
			LOG_ERR("%s: GPIO device not ready", dev->name);
			err = -ENODEV;
		}
	}

	return err;
}

static DEVICE_API(led, led_gpio_api) = {
	.on		= led_gpio_on,
	.off		= led_gpio_off,
	.set_brightness	= led_gpio_set_brightness,
#ifdef CONFIG_LED_GPIO_BLINK
	.blink		= led_gpio_blink,
#endif
};

#define LED_GPIO_BLINK_DATA(i, count)				\
static struct k_timer blink_timers_##i[count];                  \
static uint32_t delay_on_##i[count];	                        \
static uint32_t delay_off_##i[count];	                        \
static struct led_gpio_data led_gpio_data_##i = {		\
	.config		= &led_gpio_config_##i,			\
	.blink_timer	= blink_timers_##i,			\
	.delay_on	= delay_on_##i,				\
	.delay_off	= delay_off_##i,			\
};								\
BUILD_ASSERT(ARRAY_SIZE(gpio_dt_spec_##i) <= MAX_GPIO_LEDS);

#define LED_GPIO_CONFIG(i)					\
								\
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {		\
	DT_INST_FOREACH_CHILD_SEP_VARGS(i, GPIO_DT_SPEC_GET, (,), gpios) \
};								\
								\
static const struct led_gpio_config led_gpio_config_##i = {	\
	.num_leds	= ARRAY_SIZE(gpio_dt_spec_##i),		\
	.led		= gpio_dt_spec_##i,			\
};								\


#ifdef CONFIG_LED_GPIO_BLINK
#define LED_GPIO_DEVICE(i)					\
LED_GPIO_CONFIG(i);						\
LED_GPIO_BLINK_DATA(i, ARRAY_SIZE(gpio_dt_spec_##i));		\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
		      &led_gpio_data_##i, &led_gpio_config_##i,	\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);
#else /* CONFIG_LED_GPIO_BLINK */
#define LED_GPIO_DEVICE(i)					\
LED_GPIO_CONFIG(i);						\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
		      NULL, &led_gpio_config_##i,		\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);
#endif /* CONFIG_LED_GPIO_BLINK */


DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
