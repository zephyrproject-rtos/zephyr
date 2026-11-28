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
 * an autonomous animation engine. Each output has an 8-bit PWM value and an
 * 8-bit dot current, and the maximum current of all outputs is selectable
 * between 25.5 mA and 51 mA.
 *
 * An output is either in manual mode, where it shows its PWM register, or
 * in autonomous mode, where one of the animation engines drives it. The
 * driver uses the autonomous mode for led_blink() only: output N follows
 * engine N, which plays pattern N forever. A blinking LED goes between off
 * and full PWM; the levels written meanwhile show when the blinking ends.
 *
 * The device starts and stops all its engines together. Starting or ending
 * the blinking of one LED therefore restarts the blink period of the other
 * LEDs that are blinking.
 */

#define DT_DRV_COMPAT ti_lp5815

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(lp5815, CONFIG_LED_LOG_LEVEL);

#define LP5815_NUM_CHANNELS 3

#define LP5815_REG_CHIP_EN         0x00
#define LP5815_REG_DEV_CONFIG0     0x01
#define LP5815_REG_DEV_CONFIG1     0x02
#define LP5815_REG_DEV_CONFIG3     0x04
#define LP5815_REG_DEV_CONFIG4     0x05
#define LP5815_REG_ENGINE_CONFIG0  0x06
#define LP5815_REG_ENGINE_CONFIG4  0x0A
#define LP5815_REG_ENGINE_CONFIG5  0x0B
#define LP5815_REG_ENGINE_CONFIG6  0x0C
#define LP5815_REG_UPDATE_CMD      0x0F
#define LP5815_REG_START_CMD       0x10
#define LP5815_REG_STOP_CMD        0x11
#define LP5815_REG_FLAG_CLR        0x13
#define LP5815_REG_OUT0_DC         0x14
#define LP5815_REG_OUT0_MANUAL_PWM 0x18
#define LP5815_REG_PATTERN0        0x1C

/* DEV_CONFIG0 to ENGINE_CONFIG6 are written in one transfer */
#define LP5815_CONFIG_SIZE    (LP5815_REG_ENGINE_CONFIG6 - LP5815_REG_DEV_CONFIG0 + 1)
#define LP5815_CONFIG_AT(reg) ((reg) - LP5815_REG_DEV_CONFIG0)

/* CHIP_EN. With instant blinking left possible, the device may ignore the commands. */
#define LP5815_CHIP_EN        BIT(0)
#define LP5815_INSTABLINK_DIS BIT(1)

/* DEV_CONFIG0 */
#define LP5815_MAX_CURRENT_51MA BIT(0)

/* DEV_CONFIG1 */
#define LP5815_OUT_EN_ALL BIT_MASK(LP5815_NUM_CHANNELS)

/* DEV_CONFIG4: the engine each output follows, two bits for each output */
#define LP5815_OUT_ENGINE(out, engine) ((engine) << ((out) * 2))

/* ENGINE_CONFIG0 to ENGINE_CONFIG3, one for each engine: the pattern of its first order */
#define LP5815_ORDER0_PATTERN(pattern) (pattern)

/* ENGINE_CONFIG4 (engines 0 and 1) and ENGINE_CONFIG5 (engines 2 and 3): the orders in use */
#define LP5815_ORDER0_EN(engine) BIT(((engine) % 2) * 4)

/* ENGINE_CONFIG6: how many times each engine repeats, two bits for each engine */
#define LP5815_ENGINE_REPEAT_FOREVER(engine) (3 << ((engine) * 2))

/* FLAG_CLR */
#define LP5815_POR_CLR BIT(0)

#define LP5815_UPDATE_CMD_VAL 0x55
#define LP5815_START_CMD_VAL  0xFF
#define LP5815_STOP_CMD_VAL   0xAA

/* The registers of a pattern, in address order */
enum lp5815_pattern_reg {
	LP5815_PATTERN_PAUSE_TIME,
	LP5815_PATTERN_REPEAT,
	LP5815_PATTERN_PWM0,
	LP5815_PATTERN_PWM1,
	LP5815_PATTERN_PWM2,
	LP5815_PATTERN_PWM3,
	LP5815_PATTERN_PWM4,
	LP5815_PATTERN_SLOPER_TIME1, /* PWM0 to PWM1 in the low half, PWM1 to PWM2 in the high */
	LP5815_PATTERN_SLOPER_TIME2, /* PWM2 to PWM3 in the low half, PWM3 to PWM4 in the high */
	LP5815_PATTERN_SIZE,
};

#define LP5815_PATTERN_REPEAT_FOREVER 0x0F

/* The times a pattern step can take, by register value (datasheet table 7-3) */
static const uint16_t lp5815_time_ms[] = {
	0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 1000, 2000, 4000, 6000, 8000,
};

struct lp5815_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	/* The index of an LED is its first output; its colors take the next outputs */
	const struct led_info *leds_info;
	bool max_current_51ma;
	uint8_t dot_current[LP5815_NUM_CHANNELS];
};

struct lp5815_data {
	/* Serializes the changes of the engines, which the device stops and starts together */
	struct k_mutex lock;
	/* The outputs in autonomous mode, one bit each */
	uint8_t auto_en;
};

static const struct led_info *lp5815_led_to_info(const struct lp5815_config *config, uint32_t led)
{
	if (led >= config->num_leds) {
		return NULL;
	}

	return &config->leds_info[led];
}

/* The outputs of an LED, one bit each */
static uint8_t lp5815_led_outputs(const struct led_info *info)
{
	return BIT_MASK(info->num_colors) << info->index;
}

static int lp5815_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *found = lp5815_led_to_info(config, led);

	if (found == NULL) {
		return -ENODEV;
	}

	*info = found;

	return 0;
}

static int lp5815_write_channels(const struct device *dev, uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp5815_config *config = dev->config;

	if (start_channel >= LP5815_NUM_CHANNELS ||
	    num_channels > LP5815_NUM_CHANNELS - start_channel) {
		return -EINVAL;
	}

	/* An output in autonomous mode keeps blinking and shows the value afterwards */
	return i2c_burst_write_dt(&config->bus, LP5815_REG_OUT0_MANUAL_PWM + start_channel, buf,
				  num_channels);
}

static int lp5815_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *info = lp5815_led_to_info(config, led);
	uint8_t pwm;

	if (info == NULL) {
		return -ENODEV;
	}

	if (info->num_colors != 1) {
		return -ENOTSUP;
	}

	pwm = value * UINT8_MAX / LED_BRIGHTNESS_MAX;

	return lp5815_write_channels(dev, info->index, 1, &pwm);
}

static int lp5815_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			    const uint8_t *color)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *info = lp5815_led_to_info(config, led);

	if (info == NULL) {
		return -ENODEV;
	}

	if (num_colors != info->num_colors) {
		return -EINVAL;
	}

	return lp5815_write_channels(dev, info->index, num_colors, color);
}

/*
 * With the engines stopped: put the outputs of the LED in autonomous mode with @p pattern
 * on their engines, or in manual mode if @p pattern is NULL.
 */
static int lp5815_write_engines(const struct device *dev, const struct led_info *info,
				const uint8_t *pattern)
{
	const struct lp5815_config *config = dev->config;
	struct lp5815_data *data = dev->data;
	uint8_t auto_en = data->auto_en & ~lp5815_led_outputs(info);
	int ret;

	if (pattern != NULL) {
		auto_en |= lp5815_led_outputs(info);
	}

	for (uint8_t i = 0; pattern != NULL && i < info->num_colors; i++) {
		uint8_t reg = LP5815_REG_PATTERN0 + (info->index + i) * LP5815_PATTERN_SIZE;

		ret = i2c_burst_write_dt(&config->bus, reg, pattern, LP5815_PATTERN_SIZE);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_DEV_CONFIG3, auto_en);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD, LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	data->auto_en = auto_en;

	return 0;
}

/*
 * Start the blinking of an LED with @p pattern, or end it if @p pattern is NULL. The device
 * refuses pattern writes while an engine runs, so all engines stop for the change, and they
 * start again even if the change fails.
 */
static int lp5815_change_engines(const struct device *dev, const struct led_info *info,
				 const uint8_t *pattern)
{
	const struct lp5815_config *config = dev->config;
	struct lp5815_data *data = dev->data;
	int ret;
	int start_ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_STOP_CMD, LP5815_STOP_CMD_VAL);
	if (ret == 0) {
		ret = lp5815_write_engines(dev, info, pattern);
	}

	if (data->auto_en != 0) {
		start_ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_START_CMD,
						  LP5815_START_CMD_VAL);
	}

	k_mutex_unlock(&data->lock);

	return ret != 0 ? ret : start_ret;
}

/* The register value of a time the device supports exactly, or -EINVAL */
static int lp5815_time_to_reg(uint32_t ms)
{
	ARRAY_FOR_EACH(lp5815_time_ms, i) {
		if (lp5815_time_ms[i] == ms) {
			return i;
		}
	}

	return -EINVAL;
}

static int lp5815_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			uint32_t delay_off)
{
	const struct lp5815_config *config = dev->config;
	const struct led_info *info = lp5815_led_to_info(config, led);
	int time_on = lp5815_time_to_reg(delay_on);
	int time_off = lp5815_time_to_reg(delay_off);
	/*
	 * A pattern goes from PWM0 to PWM4 in four steps. Two of them are flat and last
	 * delay_on and delay_off, the two between them take no time (datasheet table 8-2).
	 */
	uint8_t pattern[LP5815_PATTERN_SIZE] = {
		[LP5815_PATTERN_REPEAT] = LP5815_PATTERN_REPEAT_FOREVER,
		[LP5815_PATTERN_PWM0] = UINT8_MAX,
		[LP5815_PATTERN_PWM1] = UINT8_MAX,
	};

	if (info == NULL) {
		return -ENODEV;
	}

	/*
	 * An LED that is never on, or never off, does not blink: back to manual mode, where
	 * it shows its levels at once. An LED that is never on gets its levels cleared first.
	 */
	if (delay_on == 0) {
		static const uint8_t off[LP5815_NUM_CHANNELS];
		int ret = lp5815_write_channels(dev, info->index, info->num_colors, off);

		if (ret < 0) {
			return ret;
		}
	}

	if (delay_on == 0 || delay_off == 0) {
		return lp5815_change_engines(dev, info, NULL);
	}

	if (time_on < 0 || time_off < 0) {
		return -EINVAL;
	}

	pattern[LP5815_PATTERN_SLOPER_TIME1] = time_on;
	pattern[LP5815_PATTERN_SLOPER_TIME2] = time_off;

	return lp5815_change_engines(dev, info, pattern);
}

/* Every LED needs its own outputs, inside the three the device has */
static int lp5815_check_leds(const struct device *dev)
{
	const struct lp5815_config *config = dev->config;
	uint8_t used = 0;

	for (uint8_t i = 0; i < config->num_leds; i++) {
		const struct led_info *info = &config->leds_info[i];

		if (info->num_colors == 0 || info->index >= LP5815_NUM_CHANNELS ||
		    info->num_colors > LP5815_NUM_CHANNELS - info->index) {
			LOG_ERR("%s: LED %u does not fit in outputs 0 to %u", dev->name, i,
				LP5815_NUM_CHANNELS - 1);
			return -EINVAL;
		}

		if ((used & lp5815_led_outputs(info)) != 0) {
			LOG_ERR("%s: LED %u shares an output with another LED", dev->name, i);
			return -EINVAL;
		}

		used |= lp5815_led_outputs(info);
	}

	return 0;
}

static int lp5815_configure(const struct device *dev)
{
	const struct lp5815_config *config = dev->config;
	/* All outputs enabled and in manual mode; output N on engine N playing pattern N */
	const uint8_t dev_config[LP5815_CONFIG_SIZE] = {
		[LP5815_CONFIG_AT(LP5815_REG_DEV_CONFIG0)] =
			config->max_current_51ma ? LP5815_MAX_CURRENT_51MA : 0,
		[LP5815_CONFIG_AT(LP5815_REG_DEV_CONFIG1)] = LP5815_OUT_EN_ALL,
		[LP5815_CONFIG_AT(LP5815_REG_DEV_CONFIG4)] = LP5815_OUT_ENGINE(0, 0) |
							     LP5815_OUT_ENGINE(1, 1) |
							     LP5815_OUT_ENGINE(2, 2),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG0) + 0] = LP5815_ORDER0_PATTERN(0),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG0) + 1] = LP5815_ORDER0_PATTERN(1),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG0) + 2] = LP5815_ORDER0_PATTERN(2),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG4)] = LP5815_ORDER0_EN(0) |
								LP5815_ORDER0_EN(1),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG5)] = LP5815_ORDER0_EN(2),
		[LP5815_CONFIG_AT(LP5815_REG_ENGINE_CONFIG6)] = LP5815_ENGINE_REPEAT_FOREVER(0) |
								LP5815_ENGINE_REPEAT_FOREVER(1) |
								LP5815_ENGINE_REPEAT_FOREVER(2),
	};
	static const uint8_t off[LP5815_NUM_CHANNELS];
	int ret;

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_CHIP_EN,
				    LP5815_CHIP_EN | LP5815_INSTABLINK_DIS);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_FLAG_CLR, LP5815_POR_CLR);
	if (ret < 0) {
		return ret;
	}

	/*
	 * After a reboot without a power cycle the device is as the application left it:
	 * an engine still running would lock the engine registers, and the outputs still
	 * show their last values.
	 */
	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_STOP_CMD, LP5815_STOP_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_burst_write_dt(&config->bus, LP5815_REG_OUT0_MANUAL_PWM, off, sizeof(off));
	if (ret < 0) {
		return ret;
	}

	ret = i2c_burst_write_dt(&config->bus, LP5815_REG_DEV_CONFIG0, dev_config,
				 sizeof(dev_config));
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, LP5815_REG_UPDATE_CMD, LP5815_UPDATE_CMD_VAL);
	if (ret < 0) {
		return ret;
	}

	return i2c_burst_write_dt(&config->bus, LP5815_REG_OUT0_DC, config->dot_current,
				  sizeof(config->dot_current));
}

static int lp5815_init(const struct device *dev)
{
	const struct lp5815_config *config = dev->config;
	struct lp5815_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C bus not ready", dev->name);
		return -ENODEV;
	}

	ret = lp5815_check_leds(dev);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&data->lock);

	ret = lp5815_configure(dev);
	if (ret < 0) {
		LOG_ERR("%s: failed to configure the device: %d", dev->name, ret);
	}

	return ret;
}

static DEVICE_API(led, lp5815_led_api) = {
	.blink = lp5815_blink,
	.get_info = lp5815_get_info,
	.set_brightness = lp5815_set_brightness,
	.set_color = lp5815_set_color,
	.write_channels = lp5815_write_channels,
};

#define LP5815_COLOR_MAPPING(led_node_id)                                                         \
	static const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define LP5815_LED_INFO(led_node_id)                                                              \
	{                                                                                         \
		.label = DT_PROP(led_node_id, label),                                             \
		.index = DT_PROP(led_node_id, index),                                             \
		.num_colors = DT_PROP_LEN(led_node_id, color_mapping),                            \
		.color_mapping = color_mapping_##led_node_id,                                     \
	},

#define LP5815_DEFINE(n)                                                                          \
	DT_INST_FOREACH_CHILD(n, LP5815_COLOR_MAPPING)                                            \
                                                                                                  \
	static const struct led_info lp5815_leds_##n[] = {                                        \
		DT_INST_FOREACH_CHILD(n, LP5815_LED_INFO)                                         \
	};                                                                                        \
                                                                                                  \
	static struct lp5815_data lp5815_data_##n;                                                \
                                                                                                  \
	static const struct lp5815_config lp5815_config_##n = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                   \
		.num_leds = ARRAY_SIZE(lp5815_leds_##n),                                          \
		.leds_info = lp5815_leds_##n,                                                     \
		.max_current_51ma = DT_INST_PROP(n, max_current_51ma),                            \
		.dot_current = DT_INST_PROP(n, dot_current),                                      \
	};                                                                                        \
                                                                                                  \
	DEVICE_DT_INST_DEFINE(n, lp5815_init, NULL, &lp5815_data_##n, &lp5815_config_##n,         \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &lp5815_led_api);

DT_INST_FOREACH_STATUS_OKAY(LP5815_DEFINE)
