/*
 * Copyright (c) 2023, Prevas A/S
 * All rights reserved.
 */

#define DT_DRV_COMPAT rgb_leds

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_rgb, CONFIG_LED_LOG_LEVEL);

struct led_dt_spec {
	const struct device *const dev;
	uint8_t led;
};

struct rgb_led_dt_spec {
	const struct led_dt_spec red;
	const struct led_dt_spec green;
	const struct led_dt_spec blue;
};

struct led_rgb_config {
	int num_leds;
	const struct rgb_led_dt_spec *leds;
};

enum led_color_component {
	RED_COMP,
	GREEN_COMP,
	BLUE_COMP
};

static const uint8_t led_rgb_color_mapping[3] = {LED_COLOR_ID_RED, LED_COLOR_ID_GREEN,
						 LED_COLOR_ID_BLUE};
static const struct led_info led_rgb_info = {
	.index = 0,
	.label = "",
	.num_colors = 3,
	.color_mapping = led_rgb_color_mapping,
};

enum led_blink_state {
	LED_BLINK_STATE_OFF,
	LED_BLINK_STATE_ON
};

struct led_rgb_data {
	uint8_t brightness;
	uint8_t color[3];
	uint8_t output[3];
	enum led_blink_state blink_state;
	uint32_t on_time;
	uint32_t off_time;
	struct k_timer timer;
};

/*****
 * Driver functions
 */

/**
 * @brief Calculates the output for each color component from the color and brightness.
 */
static void led_rgb_calc_output(struct led_rgb_data *data)
{
	/* The output for each color component is in range 0 - 100%.
	 * The color intensity is in range 0 - 0xFF and needs to be calculated into range 0 - 100
	 * and adjusted with the general brightness percentage
	 */
	for (uint8_t comp = 0; comp < ARRAY_SIZE(data->output); comp++) {
		data->output[comp] =
			(uint8_t)((uint32_t)data->brightness * data->color[comp] / 0xFF);
	}
}

/**
 * @brief Sets the brightness of one LED. If not supported, set it on or off.
 */
static int led_rgb_set_led(const struct device *dev, uint32_t led, uint8_t output)
{
	if (led_set_brightness(dev, led, output) != 0) {
		return (output > 0) ? led_on(dev, led) : led_off(dev, led);
	}
	return 0;
}

/**
 * @brief Sets the brightness of all the LEDs to set the color.
 */
static int led_rgb_set_leds(const struct device *dev, uint8_t led)
{
	const struct rgb_led_dt_spec *led_spec = &((struct led_rgb_config *)dev->config)->leds[led];
	struct led_rgb_data *data = &((struct led_rgb_data *)dev->data)[led];

	led_rgb_set_led(led_spec->red.dev, led_spec->red.led, data->output[RED_COMP]);
	led_rgb_set_led(led_spec->green.dev, led_spec->green.led, data->output[GREEN_COMP]);
	led_rgb_set_led(led_spec->blue.dev, led_spec->blue.led, data->output[BLUE_COMP]);

	return 0;
}

/**
 * @brief callback function when blink timer expires
 *
 * @param timer_id pointer to timer instance
 */
static void led_rgb_driver_expiry_function(struct k_timer *timer_id)
{
	const struct device *dev = (const struct device *)k_timer_user_data_get(timer_id);
	const struct led_rgb_config *config = dev->config;
	const struct rgb_led_dt_spec *led_spec;
	struct led_rgb_data *data = dev->data;

	for (uint8_t led = 0; led < config->num_leds; led++) {
		if (timer_id == &data[led].timer) {
			data = &data[led];
			led_spec = &((struct led_rgb_config *)dev->config)->leds[led];
			if (data->blink_state == LED_BLINK_STATE_ON) {
				data->blink_state = LED_BLINK_STATE_OFF;
				led_off(led_spec->red.dev, led_spec->red.led);
				led_off(led_spec->green.dev, led_spec->green.led);
				led_off(led_spec->blue.dev, led_spec->blue.led);

				if (data->off_time != 0) {
					k_timer_start(&data->timer, K_MSEC(data->off_time),
						      K_MSEC(0));
				}
			} else {
				if (data->on_time != 0) {
					data->blink_state = LED_BLINK_STATE_ON;
					led_rgb_set_leds(dev, led);
					k_timer_start(&data->timer, K_MSEC(data->on_time),
						      K_MSEC(0));
				}
			}
			break;
		}
	}
}

/**
 * @brief Stops blinking
 */
static void led_rgb_blink_stop(struct led_rgb_data *data)
{
	k_timer_stop(&data->timer);
	data->on_time = 0;
	data->off_time = 0;
}

int led_rgb_on(const struct device *dev, uint32_t led)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;

	if (led > config->num_leds) {
		return -EINVAL;
	}

	data = &data[led];

	led_rgb_blink_stop(data);
	data->brightness = 100;
	led_rgb_calc_output(data);

	return led_rgb_set_leds(dev, led);
}

int led_rgb_off(const struct device *dev, uint32_t led)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;

	if (led > config->num_leds) {
		return -EINVAL;
	}

	data = &data[led];

	led_rgb_blink_stop(data);
	data->brightness = 0;
	led_rgb_calc_output(data);

	return led_rgb_set_leds(dev, led);
}

int led_rgb_blink(const struct device *dev, uint32_t led, uint32_t delay_on, uint32_t delay_off)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;

	if (led > config->num_leds) {
		return -EINVAL;
	}

	data = &data[led];

	if ((delay_on != data->on_time) || (delay_off != data->off_time)) {
		k_timer_stop(&data->timer);

		data->on_time = delay_on;
		data->off_time = delay_off;

		if (delay_on != 0) {
			data->blink_state = LED_BLINK_STATE_ON;
			k_timer_start(&data->timer, K_MSEC(delay_on), K_MSEC(0));
		} else {
			data->blink_state = LED_BLINK_STATE_OFF;
		}
		return led_rgb_set_leds(dev, led);
	}

	return 0;
}

int led_rgb_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct led_rgb_config *config = dev->config;

	if (led > config->num_leds) {
		return -EINVAL;
	}

	*info = &led_rgb_info;
	return 0;
}

int led_rgb_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;

	if ((led > config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	data = &data[led];

	data->brightness = value;
	led_rgb_calc_output(data);

	return led_rgb_set_leds(dev, led);
}

int led_rgb_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
		      const uint8_t *color)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;

	if ((led > config->num_leds) || (num_colors != 3)) {
		return -EINVAL;
	}

	data = &data[led];

	memcpy(data->color, color, num_colors);
	led_rgb_calc_output(data);

	return led_rgb_set_leds(dev, led);
}

static const struct led_driver_api led_rgb_api = {
	.on = led_rgb_on,
	.off = led_rgb_off,
	.blink = led_rgb_blink,
	.get_info = led_rgb_get_info,
	.set_brightness = led_rgb_set_brightness,
	.set_color = led_rgb_set_color,
};

static int led_rgb_driver_init(const struct device *dev)
{
	const struct led_rgb_config *config = dev->config;
	struct led_rgb_data *data = dev->data;
	const struct rgb_led_dt_spec *led_spec = ((struct led_rgb_config *)dev->config)->leds;

	for (uint8_t led = 0; led < config->num_leds; led++, led_spec++, data++) {
		if ((!device_is_ready(led_spec->red.dev)) ||
		    (!device_is_ready(led_spec->green.dev)) ||
		    (!device_is_ready(led_spec->blue.dev))) {
			LOG_ERR("LED device (%s, %s or %s) not ready", led_spec->red.dev->name,
				led_spec->green.dev->name, led_spec->blue.dev->name);
			return -ENODEV;
		}

		led_rgb_calc_output(data);
		led_rgb_set_leds(dev, led);

		k_timer_init(&data->timer, led_rgb_driver_expiry_function, NULL);
		k_timer_user_data_set(&data->timer, (void *)dev);
	}

	LOG_INF("%s initialized", dev->name);

	return 0;
}

#define LED_RGB_LED_DT_SPEC_GET(node, prop)                                                        \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_PARENT(DT_PHANDLE(node, prop))),                           \
		.led = DT_NODE_CHILD_IDX(DT_PHANDLE(node, prop)),                                  \
	}

#define LED_RGB_DT_SPEC_GET(node)                                                                  \
	{                                                                                          \
		.red = LED_RGB_LED_DT_SPEC_GET(node, red_led),                                     \
		.green = LED_RGB_LED_DT_SPEC_GET(node, green_led),                                 \
		.blue = LED_RGB_LED_DT_SPEC_GET(node, blue_led),                                   \
	}

#define LED_RGB_DATA_INIT(node)                                                                    \
	{                                                                                          \
		.blink_state = LED_BLINK_STATE_OFF, .color = {0xFF, 0xFF, 0xFF},                   \
		.brightness = 100,                                                                 \
	}

#define LED_RGB_INIT(inst)                                                                         \
	static struct led_rgb_data led_rgb_data_##inst[] = {                                       \
		DT_INST_FOREACH_CHILD_SEP(inst, LED_RGB_DATA_INIT, (,))};                          \
                                                                                                   \
	static const struct rgb_led_dt_spec rgb_led_dt_spec_##inst[] = {                           \
		DT_INST_FOREACH_CHILD_SEP(inst, LED_RGB_DT_SPEC_GET, (,))};                        \
                                                                                                   \
	static const struct led_rgb_config led_rgb_config_##inst = {                               \
		.num_leds = ARRAY_SIZE(rgb_led_dt_spec_##inst),                                    \
		.leds = rgb_led_dt_spec_##inst,                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, led_rgb_driver_init, NULL, &led_rgb_data_##inst,               \
			      &led_rgb_config_##inst, POST_KERNEL, CONFIG_LED_RGB_INIT_PRIORITY,   \
			      &led_rgb_api);

DT_INST_FOREACH_STATUS_OKAY(LED_RGB_INIT)
