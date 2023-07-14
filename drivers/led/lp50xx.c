/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP50xx LED controller
 */

#define DT_DRV_COMPAT ti_lp50xx

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/lp50xx.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp50xx);

#define LP50XX_DEVICE_CONFIG0 0
#define CONFIG0_CHIP_EN       BIT(6)

#define LP50XX_DEVICE_CONFIG1    0x01
#define CONFIG1_LED_GLOBAL_OFF   BIT(0)
#define CONFIG1_MAX_CURRENT_OPT  BIT(1)
#define CONFIG1_PWM_DITHERING_EN BIT(2)
#define CONFIG1_AUTO_INCR_EN     BIT(3)
#define CONFIG1_POWER_SAVE_EN    BIT(4)
#define CONFIG1_LOG_SCALE_EN     BIT(5)

#define LP50XX_LED_CONFIG0   0x02
#define CONFIG0_LED0_BANK_EN BIT(0)
#define CONFIG0_LED1_BANK_EN BIT(1)
#define CONFIG0_LED2_BANK_EN BIT(2)
#define CONFIG0_LED3_BANK_EN BIT(3)
#define CONFIG0_LED4_BANK_EN BIT(4)
#define CONFIG0_LED5_BANK_EN BIT(5)
#define CONFIG0_LED6_BANK_EN BIT(6)
#define CONFIG0_LED7_BANK_EN BIT(7)

/* LED_CONFIG1 Register is only present on LP503x */
#define LP503X_LED_CONFIG1    0x03
#define CONFIG1_LED8_BANK_EN  BIT(0)
#define CONFIG1_LED9_BANK_EN  BIT(1)
#define CONFIG1_LED10_BANK_EN BIT(2)
#define CONFIG1_LED11_BANK_EN BIT(3)

struct lp50xx_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec enable_gpio;
	uint8_t num_leds;
	bool log_scale_en;
	bool max_curr_opt;
	const struct led_info *leds_info;
	enum lp50xx_chips chip;
};

struct lp50xx_data {
	uint8_t *chan_buf;
	union {
		struct {
			uint8_t reserved0: 6;
			uint8_t chip_enable: 1;
			uint8_t reserved1: 1;
		};
		uint8_t dev_config0;
	};
	union {
		struct {
			uint8_t led_global_off: 1;
			uint8_t max_current_option: 1;
			uint8_t pwm_dithering_en: 1;
			uint8_t auto_incr_en: 1;
			uint8_t power_save_en: 1;
			uint8_t log_scale_en: 1;
			uint8_t reserved2: 2;
		};
		uint8_t dev_config1;
	};
};

int _lp50xx_update_config(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	uint8_t buf[3];

	buf[0] = LP50XX_DEVICE_CONFIG0;
	buf[1] = data->dev_config0;
	buf[2] = data->dev_config1;
	return i2c_write_dt(&config->bus, buf, 3);
}

static const struct led_info *lp50xx_led_to_info(const struct lp50xx_config *config, uint32_t led)
{
	int i;

	for (i = 0; i < config->num_leds; i++) {
		if (config->leds_info[i].index == led) {
			return &config->leds_info[i];
		}
	}
	return NULL;
}

static int lp50xx_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int lp50xx_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[2];

	if (!led_info || value > 100) {
		return -EINVAL;
	}

	buf[0] = LP50XX_LED_BRIGHT_CHAN_BASE(config->chip) + led_info->index;
	buf[1] = (value * 0xff) / 100;

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int lp50xx_on(const struct device *dev, uint32_t led)
{
	return lp50xx_set_brightness(dev, led, 100);
}

static int lp50xx_off(const struct device *dev, uint32_t led)
{
	return lp50xx_set_brightness(dev, led, 0);
}

static int lp50xx_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			    const uint8_t *color)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[4];
	uint8_t base;

	if (!led_info || num_colors != led_info->num_colors) {
		return -EINVAL;
	}

	base = LP50XX_LED_COL_CHAN_BASE(config->chip);
	buf[0] = base + 3 * led_info->index;
	buf[1] = color[0];
	buf[2] = color[1];
	buf[3] = color[2];

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int lp50xx_write_channels(const struct device *dev, uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	uint8_t max = LP50XX_NUM_CHANNELS(config->chip);

	if (start_channel + num_channels - 1 > max) {
		return -EINVAL;
	}

	data->chan_buf[0] = start_channel;
	memcpy(data->chan_buf + 1, buf, num_channels);

	return i2c_write_dt(&config->bus, data->chan_buf, num_channels + 1);
}

static int lp50xx_init(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	uint8_t buf[3];
	int err;

	if (config->enable_gpio.port != NULL && device_is_ready(config->enable_gpio.port)) {
		gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_HIGH);
		k_msleep(2);
	}
	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}
	if (config->num_leds > LP50XX_MAX_LEDS(config->chip)) {
		LOG_ERR("%s: invalid number of LEDs %d (max %d)", dev->name, config->num_leds,
			LP50XX_MAX_LEDS(config->chip));
		return -EINVAL;
	}

	/*
	 * Since the status of the LP50xx controller is unknown when entering
	 * this function, reset the controller and bring it into a defined state.
	 */

	buf[0] = LP50XX_RESET_CHAN(config->chip);
	buf[1] = 0xF;
	buf[2] = 0xF;
	err = i2c_write_dt(&config->bus, buf, 3);
	if (err < 0) {
		LOG_ERR("I2C write failed (%d)", err);
		return err;
	}

	/* Enable LED controller. */
	buf[0] = LP50XX_DEVICE_CONFIG0;
	buf[1] = data->dev_config0;
	err = i2c_write_dt(&config->bus, buf, 2);
	if (err < 0) {
		LOG_ERR("I2C write failed (%d)", err);
		return err;
	}

	/* Apply configuration. */
	buf[0] = LP50XX_DEVICE_CONFIG1;
	buf[1] = data->dev_config1;
	return i2c_write_dt(&config->bus, buf, 2);
}

enum lp50xx_chips lp50xx_get_chip(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;

	return config->chip;
}

int lp50xx_set_bank_mode(const struct device *dev, uint32_t leds)
{
	const struct lp50xx_config *config = dev->config;

	uint8_t num_channels = (config->chip >= LP5030) ? 2 : 1;
	uint8_t buf[2];

	buf[0] = (uint8_t)(leds & 0x000000FF);
	if (num_channels == 2) {
		buf[1] = (uint8_t)((leds >> 8) & 0x000000FF);
	}

	return lp50xx_write_channels(dev, LP50XX_LED_CONFIG0, num_channels, buf);
}

int lp50xx_set_bank_color(const struct device *dev, const uint8_t color[3])
{
	const struct lp50xx_config *config = dev->config;
	uint8_t chan = LP50XX_BANK_A_COLOR(config->chip);

	return lp50xx_write_channels(dev, chan, 3, color);
}

int lp50xx_set_bank_brightness(const struct device *dev, uint8_t value)
{
	const struct lp50xx_config *config = dev->config;
	uint8_t chan = LP50XX_BANK_BRIGHT_CHAN(config->chip);

	return lp50xx_write_channels(dev, chan, 1, &value);
}

int lp50xx_set_chip_en(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->chip_enable = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_log_scale_en(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->log_scale_en = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_power_save_en(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->power_save_en = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_auto_incr_en(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->auto_incr_en = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_pwm_dithering_en(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->pwm_dithering_en = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_max_current_option(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->max_current_option = !!enable;
	return _lp50xx_update_config(dev);
}

int lp50xx_set_led_global_off(const struct device *dev, uint8_t enable)
{
	struct lp50xx_data *data = dev->data;

	data->led_global_off = !!enable;
	return _lp50xx_update_config(dev);
}

static const struct led_driver_api lp50xx_led_api = {
	.on = lp50xx_on,
	.off = lp50xx_off,
	.get_info = lp50xx_get_info,
	.set_brightness = lp50xx_set_brightness,
	.set_color = lp50xx_set_color,
	.write_channels = lp50xx_write_channels,
};

#define LP50XX_CHIP_VERSION(id)                                                                    \
	(DT_NODE_HAS_COMPAT(DT_DRV_INST(id), ti_lp5036)   ? LP5036                                 \
	 : DT_NODE_HAS_COMPAT(DT_DRV_INST(id), ti_lp5030) ? LP5030                                 \
	 : DT_NODE_HAS_COMPAT(DT_DRV_INST(id), ti_lp5024) ? LP5024                                 \
	 : DT_NODE_HAS_COMPAT(DT_DRV_INST(id), ti_lp5018) ? LP5018                                 \
	 : DT_NODE_HAS_COMPAT(DT_DRV_INST(id), ti_lp5012) ? LP5012                                 \
							  : LP5009)

#define COLOR_MAPPING(led_node_id)                                                                 \
	const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)                                                                      \
	{                                                                                          \
		.label = DT_PROP(led_node_id, label),                                              \
		.index = DT_PROP(led_node_id, index),                                              \
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),                             \
		.color_mapping = color_mapping_##led_node_id,                                      \
	},

#define LP50xx_DEVICE(id)                                                                          \
	DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)                                                   \
                                                                                                   \
	const struct led_info lp50xx_leds_##id[] = {DT_INST_FOREACH_CHILD(id, LED_INFO)};          \
	static uint8_t lp50xx_chan_buf_##id[LP50XX_NUM_CHANNELS(LP50XX_CHIP_VERSION(id)) + 1];     \
                                                                                                   \
	static const struct lp50xx_config lp50xx_config_##id = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(id),                                                   \
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(id, enable_gpios, {0}),                    \
		.num_leds = ARRAY_SIZE(lp50xx_leds_##id),                                          \
		.max_curr_opt = DT_INST_PROP(id, max_curr_opt),                                    \
		.log_scale_en = DT_INST_PROP(id, log_scale_en),                                    \
		.leds_info = lp50xx_leds_##id,                                                     \
		.chip = LP50XX_CHIP_VERSION(id),                                                   \
	};                                                                                         \
                                                                                                   \
	static struct lp50xx_data lp50xx_data_##id = {                                             \
		.chan_buf = lp50xx_chan_buf_##id,                                                  \
		.chip_enable = 1,                                                                  \
		.pwm_dithering_en = 1,                                                             \
		.auto_incr_en = 1,                                                                 \
		.power_save_en = 1,                                                                \
		.max_current_option = DT_INST_PROP(id, max_curr_opt),                              \
		.log_scale_en = DT_INST_PROP(id, log_scale_en),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &lp50xx_init, NULL, &lp50xx_data_##id, &lp50xx_config_##id,      \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &lp50xx_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP50xx_DEVICE)
