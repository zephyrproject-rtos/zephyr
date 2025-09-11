/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3194

/**
 * @file
 * @brief IS31FL3194 LED driver
 *
 * The IS31FL3194 is a 3-channel LED driver that communicates over I2C.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/dt-bindings/led/led.h>

LOG_MODULE_REGISTER(is31fl3194, CONFIG_LED_LOG_LEVEL);

#define IS31FL3194_PROD_ID_REG		0x00
#define IS31FL3194_CONF_REG		0x01
#define IS31FL3194_CURRENT_REG		0x03
#define IS31FL3194_UPDATE_REG		0x40
#define IS31FL3194_RESET_REG		0x4f

#define IS31FL3194_REG_Px_BASE		0x10
#define IS31FL3194_OFFSET_TS_T1_CFG	0x09
#define IS31FL3194_OFFSET_T2_T3_CFG	0x0a
#define IS31FL3194_OFFSET_TP_T4_CFG	0x0b
#define IS31FL3194_OFFSET_CE_CFG	0x0c

#define IS31FL3194_PROD_ID_VAL		0xce
#define IS31FL3194_CONF_ENABLE		0x01
#define IS31FL3194_CONF_RGB		FIELD_PREP(GENMASK(2, 1), 2)
#define IS31FL3194_CONF_SINGLE		FIELD_PREP(GENMASK(2, 1), 0)
#define IS31FL3194_CONF_OUTX_MASK	GENMASK(6, 4)
#define IS31FL3194_UPDATE_VAL		0xc5

#define IS31FL3194_CHANNEL_COUNT 3

#define IS31FL3194_BASE_ADDRESS(led) (IS31FL3194_REG_Px_BASE * (led + 1))
#define IS31FL3194_LED_ADDRESS(led) (IS31FL3194_BASE_ADDRESS(led) + led)

static const uint16_t is31fl3194_timings_ms[] = {
	30, 130, 260, 380, 510, 770, 1040, 1600, 2100, 2600, 3100, 4200, 5200, 6200, 7300, 8300,
};

enum is31fl3194_mode {
	IS31FL3194_MODE_SINGLE,
	IS31FL3194_MODE_RGB,
};

struct is31fl3194_data {
	enum is31fl3194_mode mode;
	uint8_t conf_reg;
};

struct is31fl3194_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *led_infos;
	const uint8_t *current_limits;
	struct gpio_dt_spec gpio_enable;
};

static const struct led_info *is31fl3194_led_to_info(const struct is31fl3194_config *config,
						     uint32_t led)
{
	if (led < config->num_leds) {
		return &config->led_infos[led];
	}

	return NULL;
}

static int is31fl3194_get_info(const struct device *dev,
			       uint32_t led,
			       const struct led_info **info_out)
{
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);

	if (info == NULL) {
		return -EINVAL;
	}

	*info_out = info;
	return 0;
}

static int is31fl3194_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
				const uint8_t *color)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);
	uint8_t address;
	int ret;

	if (info == NULL) {
		return -ENODEV;
	}

	if (data->mode != IS31FL3194_MODE_RGB) {
		return -ENOTSUP;
	}

	if (num_colors != 3) {
		return -EINVAL;
	}

	for (int i = 0; i < 3; i++) {
		uint8_t value;

		switch (info->color_mapping[i]) {
		case LED_COLOR_ID_RED:
			value = color[0];
			break;
		case LED_COLOR_ID_GREEN:
			value = color[1];
			break;
		case LED_COLOR_ID_BLUE:
			value = color[2];
			break;
		default:
			/* unreachable: mapping already tested in is31fl3194_check_config */
			return -EINVAL;
		}

		if (data->conf_reg & IS31FL3194_CONF_OUTX_MASK) {
			/* currently blinking, use pattern register */
			address = IS31FL3194_REG_Px_BASE + i;
		} else {
			address = IS31FL3194_LED_ADDRESS(i);
		}

		ret = i2c_reg_write_byte_dt(&config->bus, address, value);
		if (ret != 0) {
			break;
		}
	}

	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    IS31FL3194_UPDATE_REG,
					    IS31FL3194_UPDATE_VAL);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed: %d", dev->name, ret);
	}

	return ret;
}

static int is31fl3194_blink_one(const struct device *dev, uint32_t led, uint32_t delay_on,
				uint32_t delay_off)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	int ret;
	uint8_t ts = 1;
	uint8_t t1 = 0;
	uint8_t t2 = 1;
	uint8_t t3 = 0;
	uint8_t t4 = 1;
	uint8_t tp = 1;
	uint8_t value;
	uint8_t address;
	uint8_t base;

	if (data->mode == IS31FL3194_MODE_RGB) {
		/* When blinking in pattern mode, only the first P1 is used */
		base = IS31FL3194_REG_Px_BASE;
	} else if (data->mode == IS31FL3194_MODE_SINGLE) {
		base = IS31FL3194_BASE_ADDRESS(led);
	} else {
		return -ENOTSUP;
	}

	for (int j = ARRAY_SIZE(is31fl3194_timings_ms) - 1; j >= 0; j--) {
		if (is31fl3194_timings_ms[j] < delay_on) {
			t2 = j; /* hold (positive pulse) time */
			break;
		}
	}

	for (int j = ARRAY_SIZE(is31fl3194_timings_ms) - 1; j >= 0; j--) {
		if (is31fl3194_timings_ms[j] < delay_off) {
			tp = j; /* off (negative pulse) time*/
			break;
		}
	}

	address = base | IS31FL3194_OFFSET_TS_T1_CFG;
	value = t1 << 4 | ts;
	ret = i2c_reg_write_byte_dt(&config->bus, address, value);
	if (ret != 0) {
		return ret;
	}

	address = base | IS31FL3194_OFFSET_T2_T3_CFG;
	value = t3 << 4 | t2;
	ret = i2c_reg_write_byte_dt(&config->bus, address, value);
	if (ret != 0) {
		return ret;
	}

	address = base | IS31FL3194_OFFSET_TP_T4_CFG;
	value = t4 << 4 | tp;
	ret = i2c_reg_write_byte_dt(&config->bus, address, value);
	if (ret != 0) {
		return ret;
	}

	address = IS31FL3194_UPDATE_REG + led + 1;
	return i2c_reg_write_byte_dt(&config->bus, address, IS31FL3194_UPDATE_VAL);
}

static int is31fl3194_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			    uint32_t delay_off)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);
	uint8_t conf_reg = data->conf_reg;
	int ret = -ENOTSUP;

	if (info == NULL) {
		return -ENODEV;
	}

	/* RGB mode is selected for blinking.
	 * Single mode cannot be used, as the LED channels
	 * blink out of sync after a few hours.
	 */
	conf_reg |= IS31FL3194_CONF_RGB;

	if (data->mode == IS31FL3194_MODE_RGB) {
		/* OUTx to pattern mode for all 3 channels */
		conf_reg |= IS31FL3194_CONF_OUTX_MASK;
	} else if (data->mode == IS31FL3194_MODE_SINGLE) {
		/* OUTx to pattern mode for single channel */
		conf_reg |= LSB_GET(IS31FL3194_CONF_OUTX_MASK) << led;
	} else {
		return -ENOTSUP;
	}

	if (conf_reg != data->conf_reg) {
		ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3194_CONF_REG, conf_reg);
		if (ret != 0) {
			return ret;
		}

		data->conf_reg = conf_reg;
	}

	if (data->mode == IS31FL3194_MODE_RGB) {
		ret = is31fl3194_blink_one(dev, 0, delay_on, delay_off);
	} else if (data->mode == IS31FL3194_MODE_SINGLE) {
		ret = is31fl3194_blink_one(dev, led, delay_on, delay_off);
	}

	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    IS31FL3194_UPDATE_REG,
					    IS31FL3194_UPDATE_VAL);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed: %d", dev->name, ret);
	}

	return ret;
}

static int is31fl3194_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = is31fl3194_led_to_info(config, led);
	uint8_t address;
	int ret = 0;

	if (info == NULL) {
		return -ENODEV;
	}

	if (data->mode != IS31FL3194_MODE_SINGLE) {
		return -ENOTSUP;
	}

	if (data->conf_reg & FIELD_PREP((LSB_GET(IS31FL3194_CONF_OUTX_MASK) << led), 1)) {
		/* currently blinking, use pattern register */
		address = IS31FL3194_REG_Px_BASE + led;
	} else {
		address = IS31FL3194_LED_ADDRESS(led);
	}

	/* Rescale 0..100 to 0..255 */
	value = value * 255 / LED_BRIGHTNESS_MAX;

	ret = i2c_reg_write_byte_dt(&config->bus, address, value);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->bus,
					    IS31FL3194_UPDATE_REG,
					    IS31FL3194_UPDATE_VAL);
	}

	if (ret != 0) {
		LOG_ERR("%s: LED write failed", dev->name);
	}

	return ret;
}

/*
 * Counts red, green, blue channels; returns true if color_id is valid
 * and no more than one channel maps to the same color
 */
static bool is31fl3194_count_colors(const struct device *dev,
				    uint8_t color_id, uint8_t *rgb_counts)
{
	bool ret = false;

	switch (color_id) {
	case LED_COLOR_ID_RED:
		ret = (++rgb_counts[0] == 1);
		break;
	case LED_COLOR_ID_GREEN:
		ret = (++rgb_counts[1] == 1);
		break;
	case LED_COLOR_ID_BLUE:
		ret = (++rgb_counts[2] == 1);
		break;
	}

	if (!ret) {
		LOG_ERR("%s: invalid color %d (duplicate or not RGB)",
			dev->name, color_id);
	}

	return ret;
}

static int is31fl3194_check_config(const struct device *dev)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info;
	uint8_t rgb_counts[3] = { 0 };
	uint8_t i;

	switch (config->num_leds) {
	case 1:
		/* check that it is a three-channel LED */
		info = &config->led_infos[0];

		if (info->num_colors != 3) {
			LOG_ERR("%s: invalid number of colors %d "
				"(must be 3 for RGB LED)",
				dev->name, info->num_colors);
			return -EINVAL;
		}

		for (i = 0; i < 3; i++) {
			if (!is31fl3194_count_colors(dev, info->color_mapping[i], rgb_counts)) {
				return -EINVAL;
			}

		}

		data->mode = IS31FL3194_MODE_RGB;
		break;
	case 3:
		/* check that each LED is single-color */
		for (i = 0; i < 3; i++) {
			info = &config->led_infos[i];

			if (info->num_colors != 1) {
				LOG_ERR("%s: invalid number of colors %d "
					"(must be 1 when defining multiple LEDs)",
					dev->name, info->num_colors);
				return -EINVAL;
			}

			if (!is31fl3194_count_colors(dev, info->color_mapping[0], rgb_counts)) {
				return -EINVAL;
			}
		}

		data->mode = IS31FL3194_MODE_SINGLE;
		break;
	default:
		LOG_ERR("%s: invalid number of LEDs %d (must be 1 or 3)",
			dev->name, config->num_leds);
		return -EINVAL;
	}

	return 0;
}

static int is31fl3194_init(const struct device *dev)
{
	struct is31fl3194_data *data = dev->data;
	const struct is31fl3194_config *config = dev->config;
	const struct led_info *info = NULL;
	int i, ret;
	uint8_t prod_id, band;
	uint8_t current_reg = 0;

	ret = is31fl3194_check_config(dev);
	if (ret != 0) {
		return ret;
	}

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	/* reset unknown state to default */
	ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3194_RESET_REG, IS31FL3194_UPDATE_VAL);
	if (ret != 0) {
		LOG_ERR("Failed to write reset key (%d)", ret);
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, IS31FL3194_PROD_ID_REG, &prod_id);
	if (ret != 0) {
		LOG_ERR("%s: failed to read product ID", dev->name);
		return ret;
	}

	if (prod_id != IS31FL3194_PROD_ID_VAL) {
		LOG_ERR("%s: invalid product ID 0x%02x (expected 0x%02x)", dev->name, prod_id,
			IS31FL3194_PROD_ID_VAL);
		return -ENODEV;
	}

	/* calc current limit register value */
	info = &config->led_infos[0];
	if (info->num_colors == IS31FL3194_CHANNEL_COUNT) {
		/* one RGB LED: set all channels to the same current limit */
		band = (config->current_limits[0] / 10) - 1;
		for (i = 0; i < IS31FL3194_CHANNEL_COUNT; i++) {
			current_reg |= band << (2 * i);
		}
	} else {
		/* single-channel LEDs: independent limits */
		for (i = 0; i < config->num_leds; i++) {
			band = (config->current_limits[i] / 10) - 1;
			current_reg |= band << (2 * i);
		}
	}

	ret = i2c_reg_write_byte_dt(&config->bus, IS31FL3194_CURRENT_REG, current_reg);
	if (ret != 0) {
		LOG_ERR("%s: failed to set current limit", dev->name);
		return ret;
	}

	if (config->gpio_enable.port && gpio_is_ready_dt(&config->gpio_enable)) {
		/* Datasheet required 10uS delay before/after enable before any I2C operation */
		k_usleep(10);
		gpio_pin_configure_dt(&config->gpio_enable, GPIO_OUTPUT_ACTIVE);
		k_usleep(10);
	}

	/* Set enable bit, on subsequent writes to change modes it will always be set */
	data->conf_reg = IS31FL3194_CONF_ENABLE;

	/* enable device */
	return i2c_reg_write_byte_dt(&config->bus, IS31FL3194_CONF_REG, data->conf_reg);
}

static DEVICE_API(led, is31fl3194_led_api) = {
	.set_brightness = is31fl3194_set_brightness,
	.get_info = is31fl3194_get_info,
	.set_color = is31fl3194_set_color,
	.blink = is31fl3194_blink,
};

#define COLOR_MAPPING(led_node_id)						\
	static const uint8_t color_mapping_##led_node_id[] =			\
		DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)							\
	{									\
		.label = DT_PROP(led_node_id, label),				\
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),		\
		.color_mapping = color_mapping_##led_node_id,			\
	},

#define LED_CURRENT(led_node_id)						\
	DT_PROP(led_node_id, current_limit),

#define IS31FL3194_DEFINE(id)							\
										\
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)				\
										\
	static const struct led_info is31fl3194_leds_##id[] =			\
		{ DT_INST_FOREACH_CHILD(id, LED_INFO) };			\
	static const uint8_t is31fl3194_currents_##id[] =			\
		{ DT_INST_FOREACH_CHILD(id, LED_CURRENT) };			\
	BUILD_ASSERT(ARRAY_SIZE(is31fl3194_leds_##id) > 0,			\
		     "No LEDs defined for " #id);				\
										\
	static struct is31fl3194_data is31fl3194_data_##id;			\
	static const struct is31fl3194_config is31fl3194_config_##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),				\
		.num_leds = ARRAY_SIZE(is31fl3194_leds_##id),			\
		.led_infos = is31fl3194_leds_##id,				\
		.current_limits = is31fl3194_currents_##id,			\
		.gpio_enable = GPIO_DT_SPEC_INST_GET_OR(id, enable_gpios, {0}),	\
	};									\
	DEVICE_DT_INST_DEFINE(id, &is31fl3194_init, NULL, &is31fl3194_data_##id,\
			      &is31fl3194_config_##id, POST_KERNEL,             \
			      CONFIG_LED_INIT_PRIORITY, &is31fl3194_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3194_DEFINE)
