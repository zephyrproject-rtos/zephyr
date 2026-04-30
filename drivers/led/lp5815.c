/*
 * Copyright (c) 2026 Kristina Kisiuk, Aliensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP5815 LED driver
 *
 * The LP5815 is a 3-channel I2C RGB LED driver from Texas Instruments with
 * autonomous animation engine and instant blinking support. Each channel
 * supports 8-bit PWM dimming and individual 8-bit dot current control.
 * The maximum output current per channel is selectable between 25.5mA and
 * 51mA via the MAX_CURRENT bit.
 *
 * This driver operates the device in manual control mode by default.
 * The blink API uses the autonomous animation engine to produce a
 * hardware-driven square-wave blink on any channel.
 */

#define DT_DRV_COMPAT ti_lp5815

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lp5815, CONFIG_LED_LOG_LEVEL);

/* Register addresses */
#define LP5815_REG_CHIP_EN		0x00
#define LP5815_REG_DEV_CONFIG0		0x01
#define LP5815_REG_DEV_CONFIG1		0x02
#define LP5815_REG_DEV_CONFIG3		0x04
#define LP5815_REG_DEV_CONFIG4		0x05
#define LP5815_REG_ENGINE_CONFIG0	0x06
#define LP5815_REG_ENGINE_CONFIG4	0x0A
#define LP5815_REG_ENGINE_CONFIG5	0x0B
#define LP5815_REG_ENGINE_CONFIG6	0x0C
#define LP5815_REG_UPDATE_CMD		0x0F
#define LP5815_REG_START_CMD		0x10
#define LP5815_REG_STOP_CMD		0x11
#define LP5815_REG_FLAG_CLR		0x13
#define LP5815_REG_OUT0_DC		0x14
#define LP5815_REG_OUT1_DC		0x15
#define LP5815_REG_OUT2_DC		0x16
#define LP5815_REG_OUT0_MANUAL_PWM	0x18
#define LP5815_REG_OUT1_MANUAL_PWM	0x19
#define LP5815_REG_OUT2_MANUAL_PWM	0x1A
#define LP5815_REG_PATTERN0_PAUSE	0x1C
#define LP5815_REG_PATTERN0_PT		0x1D
#define LP5815_REG_PATTERN0_PWM0	0x1E
#define LP5815_REG_PATTERN0_SLOPER1	0x23
#define LP5815_REG_PATTERN0_SLOPER2	0x24

/* CHIP_EN register bits */
#define LP5815_CHIP_EN			BIT(0)
#define LP5815_INSTABLINK_DIS		BIT(1)

/* DEV_CONFIG0 register bits */
#define LP5815_MAX_CURRENT		BIT(0)

/* DEV_CONFIG1 register bits */
#define LP5815_OUT0_EN			BIT(0)
#define LP5815_OUT1_EN			BIT(1)
#define LP5815_OUT2_EN			BIT(2)

/* FLAG_CLR register bits */
#define LP5815_POR_CLR			BIT(0)

/* Command values */
#define LP5815_UPDATE_CMD_VAL		0x55
#define LP5815_START_CMD_VAL		0xFF
#define LP5815_STOP_CMD_VAL		0xAA

/* Pattern repeat infinite */
#define LP5815_PT_INFINITE		0x0F

/* Engine repeat infinite (2-bit field value 3) */
#define LP5815_ENGINE_REPT_INFINITE	3

/* Stride between PATTERNx register blocks (0x1C, 0x25, 0x2E, 0x37) */
#define LP5815_PATTERN_STRIDE		9

#define LP5815_NUM_CHANNELS		3

/* Maximum delay supported by the 4-bit time register (ms) */
#define LP5815_MAX_DELAY_MS		8000

/*
 * Lookup table mapping 4-bit register values to milliseconds.
 * From datasheet Table 7-3.
 */
static const uint16_t lp5815_time_ms[] = {
	0, 50, 100, 150, 200, 250, 300, 350,
	400, 450, 500, 1000, 2000, 4000, 6000, 8000,
};

struct lp5815_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	const struct led_info *leds_info;
	bool max_current;
	uint8_t out_dot_current[LP5815_NUM_CHANNELS];
};

struct lp5815_data {
	/* Bitmask of channels with autonomous animation enabled */
	uint8_t auto_enabled;
};

static uint8_t lp5815_get_start_channel(const struct lp5815_config *config,
					uint32_t led);

static const struct led_info *
lp5815_led_to_info(const struct lp5815_config *config, uint32_t led)
{
	if (led < config->num_leds) {
		return &config->leds_info[led];
	}

	return NULL;
}

static int lp5815_get_info(const struct device *dev, uint32_t led,
			   const struct led_info **info)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);

	if (!led_info) {
		return -ENODEV;
	}

	*info = led_info;

	return 0;
}

/**
 * Find the closest 4-bit time register value for a given duration in ms.
 */
static uint8_t lp5815_ms_to_time_reg(uint32_t ms)
{
	uint8_t best = 0;
	uint32_t best_diff = ms; /* diff for index 0 (0 ms) */

	for (uint8_t i = 1; i < ARRAY_SIZE(lp5815_time_ms); i++) {
		uint32_t diff = (ms >= lp5815_time_ms[i])
			      ? (ms - lp5815_time_ms[i])
			      : (lp5815_time_ms[i] - ms);

		if (diff < best_diff) {
			best_diff = diff;
			best = i;
		}
	}

	return best;
}

/**
 * Stop autonomous animation and return a channel to manual mode.
 */
static int lp5815_stop_auto(const struct device *dev, uint8_t ch)
{
	const struct lp5815_config *config = dev->config;
	struct lp5815_data *data = dev->data;
	int ret;

	if (!(data->auto_enabled & BIT(ch))) {
		return 0;
	}

	/* Stop all engines */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_STOP_CMD,
				    LP5815_STOP_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	/* Return all channels to manual mode */
	ARG_UNUSED(ch);
	data->auto_enabled = 0;
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG3,
				    data->auto_enabled);
	if (ret < 0) {
		return ret;
	}

	/* Apply configuration change */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD,
				    LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int lp5815_set_brightness(const struct device *dev, uint32_t led,
				 uint8_t value)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);
	int ret;
	uint8_t reg;
	uint8_t val;

	if (!led_info) {
		return -ENODEV;
	}

	if (led_info->num_colors != 1) {
		return -ENOTSUP;
	}

	/* If channel is in animation mode, switch back to manual first */
	ret = lp5815_stop_auto(dev, led_info->index);
	if (ret < 0) {
		return ret;
	}

	reg = LP5815_REG_OUT0_MANUAL_PWM + led_info->index;
	val = (value * 0xff) / LED_BRIGHTNESS_MAX;

	return i2c_reg_write_byte_dt(&config->bus, reg, val);
}

static int lp5815_led_blink(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	const struct lp5815_config *config = dev->config;
	struct lp5815_data *data = dev->data;
	const struct led_info *led_info = lp5815_led_to_info(config, led);
	uint8_t start_channel, num_channels, pause_on, pause_off;
	int ret;

	if (!led_info) {
		return -ENODEV;
	}

	start_channel = lp5815_get_start_channel(config, led);
	num_channels = led_info->num_colors;

	if (start_channel + num_channels > LP5815_NUM_CHANNELS) {
		return -EINVAL;
	}

	if (delay_on > LP5815_MAX_DELAY_MS || delay_off > LP5815_MAX_DELAY_MS) {
		return -EINVAL;
	}

	pause_on = lp5815_ms_to_time_reg(delay_on);
	pause_off = lp5815_ms_to_time_reg(delay_off);

	/* Stop any running animation */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_STOP_CMD,
				    LP5815_STOP_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	data->auto_enabled = 0;
	for (uint8_t ch = start_channel; ch < start_channel + num_channels; ch++) {
		uint8_t pat_base = LP5815_REG_PATTERN0_PAUSE +
				  (ch * LP5815_PATTERN_STRIDE);

		/* PATTERNx_PAUSE_TIME: PAUSE_T0[7:4] | PAUSE_T1[3:0] */
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base,
					    (pause_on << 4) | pause_off);
		if (ret < 0) {
			return ret;
		}

		/* PATTERNx_PT: infinite repeats */
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 1,
					    LP5815_PT_INFINITE);
		if (ret < 0) {
			return ret;
		}

		/* PWM0 = 0xFF (ON), PWM1-PWM3 = 0xFF, PWM4 = 0x00 (OFF) */
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 2, 0xFF);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 3, 0xFF);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 4, 0xFF);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 5, 0x00);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 6, 0x00);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 7, 0x00);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->bus, pat_base + 8, 0x00);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->bus,
					    LP5815_REG_ENGINE_CONFIG0 + ch,
					    ch & 0x03);
		if (ret < 0) {
			return ret;
		}

		if (ch < 2) {
			ret = i2c_reg_update_byte_dt(&config->bus,
						     LP5815_REG_ENGINE_CONFIG4,
						     0x0F << (ch * 4),
						     BIT(0) << (ch * 4));
		} else {
			ret = i2c_reg_update_byte_dt(&config->bus,
						     LP5815_REG_ENGINE_CONFIG5,
						     0x0F, BIT(0));
		}
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_update_byte_dt(&config->bus,
					     LP5815_REG_ENGINE_CONFIG6,
					     0x03 << (ch * 2),
					     LP5815_ENGINE_REPT_INFINITE <<
					     (ch * 2));
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_update_byte_dt(&config->bus,
					     LP5815_REG_DEV_CONFIG4,
					     0x03 << (ch * 2),
					     ch << (ch * 2));
		if (ret < 0) {
			return ret;
		}

		data->auto_enabled |= BIT(ch);
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG3,
				    data->auto_enabled);
	if (ret < 0) {
		return ret;
	}

	/* Apply DEV_CONFIG changes */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD,
				    LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	/* Start animation */
	return i2c_reg_write_byte_dt(&config->bus, LP5815_REG_START_CMD,
				     LP5815_START_CMD_VAL);
}

static int lp5815_write_channels(const struct device *dev,
				 uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp5815_config *config = dev->config;
	uint8_t i2c_buf[LP5815_NUM_CHANNELS + 1];
	int ret;

	if (start_channel + num_channels > LP5815_NUM_CHANNELS) {
		return -EINVAL;
	}

	/* Ensure affected channels are in manual mode before writing PWM */
	for (uint8_t i = 0; i < num_channels; i++) {
		ret = lp5815_stop_auto(dev, start_channel + i);
		if (ret < 0) {
			return ret;
		}
	}

	i2c_buf[0] = LP5815_REG_OUT0_MANUAL_PWM + start_channel;
	for (uint8_t i = 0; i < num_channels; i++) {
		i2c_buf[1 + i] = buf[i];
	}

	return i2c_write_dt(&config->bus, i2c_buf, num_channels + 1);
}

static uint8_t lp5815_get_start_channel(const struct lp5815_config *config,
					uint32_t led)
{
	const struct led_info *info = config->leds_info;
	uint8_t channel = 0;

	while (led) {
		channel += info->num_colors;
		led--;
		info++;
	}

	return channel;
}

static int lp5815_set_color(const struct device *dev, uint32_t led,
			    uint8_t num_colors, const uint8_t *color)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *led_info = lp5815_led_to_info(config, led);
	uint8_t start_channel;

	if (!led_info) {
		return -ENODEV;
	}

	if (num_colors != led_info->num_colors) {
		LOG_ERR("Invalid number of colors: got %d, expected %d",
			num_colors, led_info->num_colors);
		return -EINVAL;
	}

	start_channel = lp5815_get_start_channel(config, led);

	return lp5815_write_channels(dev, start_channel, num_colors, color);
}

static int lp5815_init(const struct device *dev)
{
	const struct lp5815_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Validate LED info from devicetree at init time */
	for (uint8_t i = 0; i < config->num_leds; i++) {
		if (config->leds_info[i].index >= LP5815_NUM_CHANNELS) {
			LOG_ERR("LED %d has invalid channel index %d",
				i, config->leds_info[i].index);
			return -EINVAL;
		}
	}

	/* Enable chip and disable instant blinking for I2C control */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_CHIP_EN,
				    LP5815_CHIP_EN | LP5815_INSTABLINK_DIS);
	if (ret < 0) {
		return ret;
	}

	/* Clear POR flag (required after power-on reset) */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_FLAG_CLR,
				    LP5815_POR_CLR);
	if (ret < 0) {
		return ret;
	}

	/* Set maximum current range */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG0,
				    config->max_current ? LP5815_MAX_CURRENT : 0);
	if (ret < 0) {
		return ret;
	}

	/* Enable all 3 output channels */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG1,
				    LP5815_OUT0_EN | LP5815_OUT1_EN |
				    LP5815_OUT2_EN);
	if (ret < 0) {
		return ret;
	}

	/* Apply DEV_CONFIG changes with update command */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD,
				    LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	/* Set per-channel dot currents */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT0_DC,
				    config->out_dot_current[0]);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT1_DC,
				    config->out_dot_current[1]);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_OUT2_DC,
				    config->out_dot_current[2]);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(led, lp5815_led_api) = {
	.get_info	= lp5815_get_info,
	.set_brightness	= lp5815_set_brightness,
	.set_color	= lp5815_set_color,
	.write_channels	= lp5815_write_channels,
	.blink		= lp5815_led_blink,
};

#define LP5815_COLOR_MAPPING(led_node_id)				\
	static const uint8_t color_mapping_##led_node_id[] =		\
		DT_PROP(led_node_id, color_mapping);

#define LP5815_LED_INFO(led_node_id)					\
	{								\
		.label		= DT_PROP(led_node_id, label),		\
		.index		= DT_PROP(led_node_id, index),		\
		.num_colors	= DT_PROP_LEN(led_node_id,		\
					       color_mapping),		\
		.color_mapping	= color_mapping_##led_node_id,		\
	},

#define LP5815_DEFINE(n)						\
	DT_INST_FOREACH_CHILD(n, LP5815_COLOR_MAPPING)			\
									\
	static const struct led_info lp5815_leds_##n[] = {		\
		DT_INST_FOREACH_CHILD(n, LP5815_LED_INFO)		\
	};								\
									\
	static struct lp5815_data lp5815_data_##n;			\
									\
	static const struct lp5815_config lp5815_config_##n = {		\
		.bus		= I2C_DT_SPEC_INST_GET(n),		\
		.num_leds	= ARRAY_SIZE(lp5815_leds_##n),		\
		.leds_info	= lp5815_leds_##n,			\
		.max_current	= DT_INST_PROP(n, max_current_51ma),	\
		.out_dot_current = {					\
			DT_INST_PROP(n, out0_dot_current),		\
			DT_INST_PROP(n, out1_dot_current),		\
			DT_INST_PROP(n, out2_dot_current),		\
		},							\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, lp5815_init, NULL,			\
			      &lp5815_data_##n, &lp5815_config_##n,	\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
			      &lp5815_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP5815_DEFINE)
