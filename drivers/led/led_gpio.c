/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2026 Siemens AG
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
	const struct device *dev;
	uint32_t led_idx;
	uint32_t delay_on;
	uint32_t delay_off;
	bool is_on;
	struct k_work_delayable work;
};

struct led_gpio_data {
	struct led_gpio_blink_data *blink;
};

static void led_gpio_blink_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_gpio_blink_data *blink =
		CONTAINER_OF(dwork, struct led_gpio_blink_data, work);
	const struct led_gpio_config *config = blink->dev->config;
	const struct gpio_dt_spec *led_gpio = &config->led[blink->led_idx];
	uint32_t next_delay;

	blink->is_on = !blink->is_on;
	gpio_pin_set_dt(led_gpio, blink->is_on);

	next_delay = blink->is_on ? blink->delay_on : blink->delay_off;
	k_work_schedule(&blink->work, K_MSEC(next_delay));
}

static int led_gpio_blink(const struct device *dev, uint32_t led,
			   uint32_t delay_on, uint32_t delay_off)
{
	const struct led_gpio_config *config = dev->config;
	struct led_gpio_data *data = dev->data;
	struct led_gpio_blink_data *blink;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	blink = &data->blink[led];

	/* Cancel any ongoing blink work for this LED */
	k_work_cancel_delayable(&blink->work);

	if (delay_on == 0 && delay_off == 0) {
		/* Stop blinking, turn LED off */
		return gpio_pin_set_dt(&config->led[led], 0);
	}

	blink->dev = dev;
	blink->led_idx = led;
	blink->delay_on = delay_on;
	blink->delay_off = delay_off;
	blink->is_on = true;

	/* Start with LED on */
	gpio_pin_set_dt(&config->led[led], 1);
	k_work_schedule(&blink->work, K_MSEC(delay_on));

	return 0;
}

#endif /* CONFIG_LED_GPIO_BLINK */

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led_gpio;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

#ifdef CONFIG_LED_GPIO_BLINK
	/* cancel ongoing blinking */
	struct led_gpio_data *data = dev->data;

	k_work_cancel_delayable(&data->blink[led].work);
#endif

	led_gpio = &config->led[led];

	return gpio_pin_set_dt(led_gpio, value > 0);
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

#ifdef CONFIG_LED_GPIO_BLINK
	if (!err) {
		struct led_gpio_data *data = dev->data;

		for (size_t i = 0; i < config->num_leds; i++) {
			data->blink[i].dev = dev;
			data->blink[i].led_idx = i;
			k_work_init_delayable(&data->blink[i].work,
					      led_gpio_blink_work_handler);
		}
	}
#endif

	return err;
}

static DEVICE_API(led, led_gpio_api) = {
	.set_brightness	= led_gpio_set_brightness,
#ifdef CONFIG_LED_GPIO_BLINK
	.blink		= led_gpio_blink,
#endif
};

#ifdef CONFIG_LED_GPIO_BLINK

#define LED_GPIO_BLINK_DATA(i)						\
static struct led_gpio_blink_data led_gpio_blink_data_##i[DT_INST_CHILD_NUM(i)]; \
static struct led_gpio_data led_gpio_data_##i = {			\
	.blink = led_gpio_blink_data_##i,				\
};

#define LED_GPIO_DATA(i) &led_gpio_data_##i

#else

#define LED_GPIO_BLINK_DATA(i)
#define LED_GPIO_DATA(i) NULL

#endif /* CONFIG_LED_GPIO_BLINK */

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
LED_GPIO_BLINK_DATA(i)						\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
		      LED_GPIO_DATA(i), &led_gpio_config_##i,	\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
