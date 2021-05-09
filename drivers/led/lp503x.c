/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lp503x

/**
 * @file
 * @brief LP503x LED controller
 */

#include <drivers/i2c.h>
#include <drivers/led.h>
#include <drivers/led/lp503x.h>
#include <device.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lp503x);

#define LP503X_DEVICE_CONFIG0		0
#define   CONFIG0_CHIP_EN		BIT(6)

#define LP503X_DEVICE_CONFIG1		0x1
#define   CONFIG1_LED_GLOBAL_OFF	BIT(0)
#define   CONFIG1_MAX_CURRENT_OPT	BIT(1)
#define   CONFIG1_PWM_DITHERING_EN	BIT(2)
#define   CONFIG1_AUTO_INCR_EN		BIT(3)
#define   CONFIG1_POWER_SAVE_EN		BIT(4)
#define   CONFIG1_LOG_SCALE_EN		BIT(5)

#define LP503X_LED_CONFIG0		0x2
#define   CONFIG0_LED0_BANK_EN		BIT(0)
#define   CONFIG0_LED1_BANK_EN		BIT(1)
#define   CONFIG0_LED2_BANK_EN		BIT(2)
#define   CONFIG0_LED3_BANK_EN		BIT(3)
#define   CONFIG0_LED4_BANK_EN		BIT(4)
#define   CONFIG0_LED5_BANK_EN		BIT(5)
#define   CONFIG0_LED6_BANK_EN		BIT(6)
#define   CONFIG0_LED7_BANK_EN		BIT(7)

#define LP503X_LED_CONFIG1		0x3
#define   CONFIG1_LED8_BANK_EN		BIT(0)
#define   CONFIG1_LED9_BANK_EN		BIT(1)
#define   CONFIG1_LED10_BANK_EN		BIT(2)
#define   CONFIG1_LED11_BANK_EN		BIT(3)

#define LP503X_BANK_BRIGHTNESS		0x4
#define LP503X_BANK_A_COLOR		0x5
#define LP503X_BANK_B_COLOR		0x6
#define LP503X_BANK_C_COLOR		0x7

#define LP503X_LED_BRIGHTNESS_BASE	0x8
#define LP503X_OUT_COLOR_BASE		0x14

/* Expose channels starting from the bank registers. */
#define LP503X_CHANNEL_BASE		LP503X_BANK_BRIGHTNESS

#define DEV_CFG(dev)	((const struct lp503x_config *) ((dev)->config))
#define DEV_DATA(dev)	((struct lp503x_data *) ((dev)->data))

struct lp503x_config {
	char *i2c_bus_label;
	uint8_t i2c_addr;
	uint8_t num_leds;
	bool log_scale_en;
	bool max_curr_opt;
	const struct led_info *leds_info;
};

struct lp503x_data {
	const struct device *i2c;
	uint8_t *chan_buf;
};

static const struct led_info *
lp503x_led_to_info(const struct lp503x_config *config, uint32_t led)
{
	int i;

	for (i = 0; i < config->num_leds; i++) {
		if (config->leds_info[i].index == led) {
			return &config->leds_info[i];
		}
	}
	return NULL;
}

static int lp503x_get_info(const struct device *dev, uint32_t led,
			   const struct led_info **info)
{
	const struct lp503x_config *config = DEV_CFG(dev);
	const struct led_info *led_info = lp503x_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int lp503x_set_brightness(const struct device *dev,
				 uint32_t led, uint8_t value)
{
	const struct lp503x_config *config = DEV_CFG(dev);
	struct lp503x_data *data = DEV_DATA(dev);
	const struct led_info *led_info = lp503x_led_to_info(config, led);
	uint8_t buf[2];

	if (!led_info || value > 100) {
		return -EINVAL;
	}

	buf[0] = LP503X_LED_BRIGHTNESS_BASE + led_info->index;
	buf[1] = (value * 0xff) / 100;

	return i2c_write(data->i2c, buf, sizeof(buf), config->i2c_addr);
}

static int lp503x_on(const struct device *dev, uint32_t led)
{
	return lp503x_set_brightness(dev, led, 100);
}

static int lp503x_off(const struct device *dev, uint32_t led)
{
	return lp503x_set_brightness(dev, led, 0);
}

static int lp503x_set_color(const struct device *dev, uint32_t led,
			    uint8_t num_colors, const uint8_t *color)
{
	const struct lp503x_config *config = DEV_CFG(dev);
	struct lp503x_data *data = DEV_DATA(dev);
	const struct led_info *led_info = lp503x_led_to_info(config, led);
	uint8_t buf[4];

	if (!led_info || num_colors != led_info->num_colors) {
		return -EINVAL;
	}

	buf[0] = LP503X_OUT_COLOR_BASE + 3 * led_info->index;
	buf[1] = color[0];
	buf[2] = color[1];
	buf[3] = color[2];

	return i2c_write(data->i2c, buf, sizeof(buf), config->i2c_addr);
}

static int lp503x_write_channels(const struct device *dev,
				 uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp503x_config *config = DEV_CFG(dev);
	struct lp503x_data *data = DEV_DATA(dev);

	if (start_channel >= LP503X_NUM_CHANNELS ||
	    start_channel + num_channels > LP503X_NUM_CHANNELS) {
		return -EINVAL;
	}

	/*
	 * Unfortunately this controller don't support commands split into
	 * two I2C messages.
	 */
	data->chan_buf[0] = LP503X_CHANNEL_BASE + start_channel;
	memcpy(data->chan_buf + 1, buf, num_channels);

	return i2c_write(data->i2c, data->chan_buf,
			 num_channels + 1, config->i2c_addr);
}

static int lp503x_init(const struct device *dev)
{
	const struct lp503x_config *config = DEV_CFG(dev);
	struct lp503x_data *data = DEV_DATA(dev);
	uint8_t buf[3];
	int err;

	data->i2c = device_get_binding(config->i2c_bus_label);
	if (data->i2c == NULL) {
		LOG_ERR("%s: device %s not found",
			dev->name, config->i2c_bus_label);
		return -ENODEV;
	}
	if (config->num_leds > LP503X_MAX_LEDS) {
		LOG_ERR("%s: invalid number of LEDs %d (max %d)",
			dev->name, config->num_leds, LP503X_MAX_LEDS);
		return -EINVAL;
	}

	/*
	 * Since the status of the LP503x controller is unknown when entering
	 * this function, and since there is no way to reset it, then the whole
	 * configuration must be applied.
	 */

	/* Disable bank control for all LEDs. */
	buf[0] = LP503X_LED_CONFIG0;
	buf[1] = 0;
	buf[2] = 0;
	err = i2c_write(data->i2c, buf, 3, config->i2c_addr);
	if (err < 0) {
		return err;
	}

	/* Enable LED controller. */
	buf[0] = LP503X_DEVICE_CONFIG0;
	buf[1] = CONFIG0_CHIP_EN;
	err = i2c_write(data->i2c, buf, 2, config->i2c_addr);
	if (err < 0) {
		return err;
	}

	/* Apply configuration. */
	buf[0] = LP503X_DEVICE_CONFIG1;
	buf[1] = CONFIG1_PWM_DITHERING_EN | CONFIG1_AUTO_INCR_EN
		| CONFIG1_POWER_SAVE_EN;
	if (config->max_curr_opt) {
		buf[1] |= CONFIG1_MAX_CURRENT_OPT;
	}
	if (config->log_scale_en) {
		buf[1] |= CONFIG1_LOG_SCALE_EN;
	}

	return i2c_write(data->i2c, buf, 2, config->i2c_addr);
}

static const struct led_driver_api lp503x_led_api = {
	.on		= lp503x_on,
	.off		= lp503x_off,
	.get_info	= lp503x_get_info,
	.set_brightness	= lp503x_set_brightness,
	.set_color	= lp503x_set_color,
	.write_channels	= lp503x_write_channels,
};

#define COLOR_MAPPING(led_node_id)				\
const uint8_t color_mapping_##led_node_id[] =			\
		DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)					\
{								\
	.label		= DT_LABEL(led_node_id),		\
	.index		= DT_PROP(led_node_id, index),		\
	.num_colors	=					\
		DT_PROP_LEN(led_node_id, color_mapping),	\
	.color_mapping	= color_mapping_##led_node_id,		\
},

#define LP503x_DEVICE(id)					\
								\
DT_INST_FOREACH_CHILD(id, COLOR_MAPPING)			\
								\
const struct led_info lp503x_leds_##id[] = {			\
	DT_INST_FOREACH_CHILD(id, LED_INFO)			\
};								\
								\
static uint8_t lp503x_chan_buf_##id[LP503X_NUM_CHANNELS + 1];	\
								\
static struct lp503x_config lp503x_config_##id = {		\
	.i2c_bus_label	= DT_INST_BUS_LABEL(id),		\
	.i2c_addr	= DT_INST_REG_ADDR(id),			\
	.num_leds	= ARRAY_SIZE(lp503x_leds_##id),		\
	.max_curr_opt	= DT_INST_PROP(id, max_curr_opt),	\
	.log_scale_en	= DT_INST_PROP(id, log_scale_en),	\
	.leds_info	= lp503x_leds_##id,			\
};								\
								\
static struct lp503x_data lp503x_data_##id = {			\
	.chan_buf	= lp503x_chan_buf_##id,			\
};								\
								\
DEVICE_DT_INST_DEFINE(id,					\
		    &lp503x_init,				\
		    device_pm_control_nop,			\
		    &lp503x_data_##id,				\
		    &lp503x_config_##id,			\
		    POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		    &lp503x_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP503x_DEVICE)
