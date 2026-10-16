/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright (c) 2026 sevenlab engineering GmbH
 */

#define DT_DRV_COMPAT kinetic_ktd2052

/**
 * @file
 * @brief LED driver for the Kinetic KTD2052
 *
 * The KTD2052 sinks the current of four RGB LED modules. Each of its twelve channels has an
 * 8 bit current register of 0.125 mA per step; the driver scales the LED API's 0 to 255 to the
 * rated 0 to 24 mA. Channel n is colour (n % 3) of RGB module (n / 3). With child nodes,
 * LED n of the LED API is the n-th child and its index selects the RGB module; without child
 * nodes, LED n is module n.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ktd2052, CONFIG_LED_LOG_LEVEL);

#define KTD2052_REG_ID      0x00
#define KTD2052_REG_CONTROL 0x02
#define KTD2052_REG_I_RED_1 0x03

#define KTD2052_ID             0xA6
#define KTD2052_CONTROL_NORMAL 0x80

#define KTD2052_LEDS        4
#define KTD2052_COLORS      3
#define KTD2052_CHANNELS    (KTD2052_LEDS * KTD2052_COLORS)
#define KTD2052_MAX_CURRENT 0xC0 /* 24 mA */

struct ktd2052_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *leds_info;
};

struct ktd2052_data {
	uint8_t color[KTD2052_LEDS][KTD2052_COLORS];
	uint8_t brightness[KTD2052_LEDS];
};

static uint8_t ktd2052_current(uint32_t value)
{
	return (uint8_t)((value * KTD2052_MAX_CURRENT) / 255U);
}

static int ktd2052_module(const struct ktd2052_config *config, uint32_t led)
{
	if (config->num_leds == 0) {
		return (led < KTD2052_LEDS) ? (int)led : -EINVAL;
	}

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	return config->leds_info[led].index;
}

static int ktd2052_update_led(const struct device *dev, uint32_t led)
{
	const struct ktd2052_config *config = dev->config;
	struct ktd2052_data *data = dev->data;
	uint8_t currents[KTD2052_COLORS];

	for (size_t i = 0; i < KTD2052_COLORS; i++) {
		currents[i] = ktd2052_current((data->color[led][i] * data->brightness[led]) /
					      LED_BRIGHTNESS_MAX);
	}

	return i2c_burst_write_dt(&config->bus, KTD2052_REG_I_RED_1 + (led * KTD2052_COLORS),
				  currents, sizeof(currents));
}

static int ktd2052_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct ktd2052_data *data = dev->data;
	int module = ktd2052_module(dev->config, led);

	if (module < 0) {
		return module;
	}

	data->brightness[module] = value;

	return ktd2052_update_led(dev, module);
}

static int ktd2052_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			     const uint8_t *color)
{
	struct ktd2052_data *data = dev->data;
	int module = ktd2052_module(dev->config, led);

	if (module < 0 || num_colors != KTD2052_COLORS) {
		return -EINVAL;
	}

	memcpy(data->color[module], color, KTD2052_COLORS);

	return ktd2052_update_led(dev, module);
}

static int ktd2052_write_channels(const struct device *dev, uint32_t start_channel,
				  uint32_t num_channels, const uint8_t *buf)
{
	const struct ktd2052_config *config = dev->config;
	struct ktd2052_data *data = dev->data;
	uint8_t currents[KTD2052_CHANNELS];
	int ret;

	if (start_channel >= KTD2052_CHANNELS || num_channels > KTD2052_CHANNELS - start_channel) {
		return -EINVAL;
	}

	for (size_t i = 0; i < num_channels; i++) {
		currents[i] = ktd2052_current(buf[i]);
	}

	/* all channels in a single bus transaction */
	ret = i2c_burst_write_dt(&config->bus, KTD2052_REG_I_RED_1 + start_channel, currents,
				 num_channels);
	if (ret < 0) {
		return ret;
	}

	/* keep the cache in step, so that a later set_brightness() or set_color() builds on this */
	for (size_t i = 0; i < num_channels; i++) {
		uint32_t module = (start_channel + i) / KTD2052_COLORS;
		uint8_t *color = data->color[module];
		uint8_t brightness = data->brightness[module];

		if (brightness != LED_BRIGHTNESS_MAX) {
			for (size_t c = 0; c < KTD2052_COLORS; c++) {
				color[c] = (color[c] * brightness) / LED_BRIGHTNESS_MAX;
			}
			data->brightness[module] = LED_BRIGHTNESS_MAX;
		}

		color[(start_channel + i) % KTD2052_COLORS] = buf[i];
	}

	return 0;
}

static int ktd2052_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct ktd2052_config *config = dev->config;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	*info = &config->leds_info[led];

	return 0;
}

static int ktd2052_init(const struct device *dev)
{
	const struct ktd2052_config *config = dev->config;
	struct ktd2052_data *data = dev->data;
	const uint8_t off[KTD2052_CHANNELS] = {0};
	uint8_t id;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C bus %s not ready", dev->name, config->bus.bus->name);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, KTD2052_REG_ID, &id);
	if (ret < 0) {
		LOG_ERR("%s: no KTD2052 at 0x%02x", dev->name, config->bus.addr);
		return ret;
	}

	if (id != KTD2052_ID) {
		LOG_ERR("%s: unexpected chip id 0x%02x", dev->name, id);
		return -ENODEV;
	}

	for (size_t i = 0; i < config->num_leds; i++) {
		if (config->leds_info[i].index >= KTD2052_LEDS ||
		    config->leds_info[i].num_colors != KTD2052_COLORS) {
			LOG_ERR("%s: LED %u must have index 0 to 3 and three colors", dev->name,
				config->leds_info[i].index);
			return -EINVAL;
		}
	}

	/* white at zero brightness until told otherwise */
	memset(data->color, 0xFF, sizeof(data->color));

	ret = i2c_burst_write_dt(&config->bus, KTD2052_REG_I_RED_1, off, sizeof(off));
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, KTD2052_REG_CONTROL, KTD2052_CONTROL_NORMAL);
}

static DEVICE_API(led, ktd2052_api) = {
	.set_brightness = ktd2052_set_brightness,
	.set_color = ktd2052_set_color,
	.write_channels = ktd2052_write_channels,
	.get_info = ktd2052_get_info,
};

#define COLOR_MAPPING(led_node_id)                                                                 \
	static const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)                                                                      \
	{                                                                                          \
		.label = DT_PROP(led_node_id, label),                                              \
		.index = DT_PROP(led_node_id, index),                                              \
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),                             \
		.color_mapping = color_mapping_##led_node_id,                                      \
	},

#define KTD2052_DEFINE(id)                                                                         \
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)                                                   \
                                                                                                   \
	static const struct led_info ktd2052_leds_##id[] = {DT_INST_FOREACH_CHILD(id, LED_INFO)};  \
                                                                                                   \
	static const struct ktd2052_config ktd2052_config_##id = {                                 \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
		.num_leds = ARRAY_SIZE(ktd2052_leds_##id),                                         \
		.leds_info = ktd2052_leds_##id,                                                    \
	};                                                                                         \
                                                                                                   \
	static struct ktd2052_data ktd2052_data_##id;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, ktd2052_init, NULL, &ktd2052_data_##id, &ktd2052_config_##id,    \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &ktd2052_api);

DT_INST_FOREACH_STATUS_OKAY(KTD2052_DEFINE)
