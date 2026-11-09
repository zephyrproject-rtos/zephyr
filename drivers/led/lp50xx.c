/*
 * Copyright (c) 2020 Seagate Technology LLC
 * Copyright (c) 2022 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP50xx LED controller
 */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/lp50xx.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp50xx, CONFIG_LED_LOG_LEVEL);

/*
 * Number of supported RGB led modules per chipset.
 *
 * For each module, there are 4 associated registers:
 *   - 1 brightness register
 *   - 3 color registers (RGB)
 *
 * A chipset can have more modules than leds. In this case, the
 * associated registers will simply be inactive.
 */
#define LP5012_NUM_MODULES		4
#define LP5024_NUM_MODULES		8
#define LP5036_NUM_MODULES		12

/* Maximum number of channels */
#define LP50XX_MAX_CHANNELS(nmodules)	\
	((LP50XX_COLORS_PER_LED + 1) * ((nmodules) + 1))

#define LP50XX_DISABLE_DELAY_US		3
#define LP50XX_ENABLE_DELAY_US		500

/* Base registers */
#define LP50XX_DEVICE_CONFIG0		0x00
#define LP50XX_DEVICE_CONFIG1		0x01
#define LP50XX_LED_CONFIG0		0x02

#define LP50XX_BANK_BASE(nmodules)		\
	(0x03 + (((nmodules) - 1) / 8))

#define LP50XX_LED0_BRIGHTNESS(nmodules)	\
	((LP50XX_BANK_BASE(nmodules)) + 4)

#define LP50XX_OUT0_COLOR(nmodules)		\
	(LP50XX_LED0_BRIGHTNESS(nmodules) + (nmodules))

#define LP50XX_RESET(nmodules)			\
	(LP50XX_OUT0_COLOR(nmodules) + LP50XX_COLORS_PER_LED * (nmodules))

/* AL5887 specific registers */
#define AL5887_LED_GLOBAL_DIMMING	0x66

/* Register values */
#define CONFIG0_CHIP_EN			BIT(6)

#define CONFIG1_LED_GLOBAL_OFF		BIT(0)
#define CONFIG1_MAX_CURRENT_OPT		BIT(1)
#define CONFIG1_PWM_DITHERING_EN	BIT(2)
#define CONFIG1_AUTO_INCR_EN		BIT(3)
#define CONFIG1_POWER_SAVE_EN		BIT(4)
#define CONFIG1_LOG_SCALE_EN		BIT(5)
#define CONFIG1_PHASE_SHIFT_EN		BIT(7)

#define RESET_SW			0xFF

enum lp50xx_chip {
	LP50XX_CHIP_TI,
	LP50XX_CHIP_AL5887,
};

struct lp50xx_config {
	struct i2c_dt_spec bus;
	const struct gpio_dt_spec gpio_enable;
	uint8_t num_modules;
	uint8_t max_leds;
	uint8_t num_leds;
	uint8_t config1;
	uint8_t chip; /* enum lp50xx_chip */
	uint8_t global_dimming;
	const struct led_info *leds_info;
};

struct lp50xx_data {
	uint8_t *chan_buf;
};

static inline bool lp50xx_is_al5887(const struct lp50xx_config *config)
{
	return DT_HAS_COMPAT_STATUS_OKAY(diodes_al5887) &&
	       (config->chip == LP50XX_CHIP_AL5887);
}

static const struct led_info *lp50xx_led_to_info(
			const struct lp50xx_config *config, uint32_t led)
{
	if (led < config->num_leds) {
		return &config->leds_info[led];
	}

	return NULL;
}

static int lp50xx_get_info(const struct device *dev, uint32_t led,
			   const struct led_info **info)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

	*info = led_info;

	return 0;
}

static int lp50xx_set_brightness(const struct device *dev,
				 uint32_t led, uint8_t value)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[2];

	if (!led_info) {
		return -ENODEV;
	}

	buf[0] = LP50XX_LED0_BRIGHTNESS(config->num_modules) + led_info->index;
	buf[1] = (value * 0xff) / LED_BRIGHTNESS_MAX;

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int lp50xx_set_color(const struct device *dev, uint32_t led,
			    uint8_t num_colors, const uint8_t *color)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[LP50XX_COLORS_PER_LED + 1];
	uint8_t i;

	if (!led_info) {
		return -ENODEV;
	}

	if (num_colors != led_info->num_colors) {
		LOG_ERR("%s: invalid number of colors: got=%d, expected=%d",
			dev->name,
			num_colors,
			led_info->num_colors);
		return -EINVAL;
	}

	buf[0] = LP50XX_OUT0_COLOR(config->num_modules);
	buf[0] += LP50XX_COLORS_PER_LED * led_info->index;

	for (i = 0; i < led_info->num_colors; i++) {
		buf[1 + i] = color[i];
	}

	return i2c_write_dt(&config->bus, buf, led_info->num_colors + 1);
}

static int lp50xx_write_channels(const struct device *dev,
				 uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	uint8_t base_channel, end_channel, max_channels;

	base_channel = LP50XX_BANK_BASE(config->num_modules);
	end_channel = base_channel + start_channel + num_channels;
	max_channels = base_channel + LP50XX_MAX_CHANNELS(config->num_modules);

	if (end_channel > max_channels) {
		return -EINVAL;
	}

	/*
	 * Unfortunately this controller doesn't support commands split into
	 * two I2C messages.
	 */
	data->chan_buf[0] = base_channel + start_channel;
	memcpy(data->chan_buf + 1, buf, num_channels);

	return i2c_write_dt(&config->bus, data->chan_buf, num_channels + 1);
}

static int lp50xx_reset(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;
	uint8_t buf[2];
	int err;

	/* Software reset */
	buf[0] = LP50XX_RESET(config->num_modules);
	buf[1] = RESET_SW;
	err = i2c_write_dt(&config->bus, buf, 2);
	if (err < 0) {
		return err;
	}

	/* After reset, apply configuration since all registers are reset. */
	buf[0] = LP50XX_DEVICE_CONFIG1;
	buf[1] = config->config1;
	err = i2c_write_dt(&config->bus, buf, 2);
	if (err < 0) {
		return err;
	}

	if (lp50xx_is_al5887(config)) {
		buf[0] = AL5887_LED_GLOBAL_DIMMING;
		buf[1] = config->global_dimming;
		err = i2c_write_dt(&config->bus, buf, 2);
	}

	return err;
}

static int lp50xx_hw_enable(const struct device *dev, bool enable)
{
	const struct lp50xx_config *config = dev->config;
	int err;

	if (config->gpio_enable.port == NULL) {
		/* Nothing to do */
		return 0;
	}

	err = gpio_pin_set_dt(&config->gpio_enable, enable);
	if (err < 0) {
		LOG_ERR("%s: failed to set enable gpio", dev->name);
		return err;
	}

	k_usleep(enable ? LP50XX_ENABLE_DELAY_US : LP50XX_DISABLE_DELAY_US);

	return 0;
}

static int lp50xx_enable(const struct device *dev, bool enable)
{
	const struct lp50xx_config *config = dev->config;
	uint8_t value = enable ? CONFIG0_CHIP_EN : 0;

	return i2c_reg_update_byte_dt(&config->bus,
				      LP50XX_DEVICE_CONFIG0,
				      CONFIG0_CHIP_EN,
				      value);
}

static int lp50xx_init(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;
	uint8_t led;
	int err;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: I2C device not ready", dev->name);
		return -ENODEV;
	}

	/* Check LED configuration found in DT */
	if (config->num_leds > config->max_leds) {
		LOG_ERR("%s: invalid number of LEDs %d (max %d)",
			dev->name,
			config->num_leds,
			config->max_leds);
		return -EINVAL;
	}
	for (led = 0; led < config->num_leds; led++) {
		const struct led_info *led_info =
			lp50xx_led_to_info(config, led);

		if (led_info->num_colors > LP50XX_COLORS_PER_LED) {
			LOG_ERR("%s: LED %d: invalid number of colors (max %d)",
				dev->name, led, LP50XX_COLORS_PER_LED);
			return -EINVAL;
		}
	}

	/* Configure GPIO if present */
	if (config->gpio_enable.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_enable)) {
			LOG_ERR("%s: enable gpio is not ready", dev->name);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->gpio_enable,
					    GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("%s: failed to initialize enable gpio",
				dev->name);
			return err;
		}
	}

	/* Enable hardware */
	err = lp50xx_hw_enable(dev, true);
	if (err < 0) {
		LOG_ERR("%s: failed to enable hardware", dev->name);
		return err;
	}

	/* Reset device */
	err = lp50xx_reset(dev);
	if (err < 0) {
		LOG_ERR("%s: failed to reset", dev->name);
		return err;
	}

	/* Enable device */
	err = lp50xx_enable(dev, true);
	if (err < 0) {
		LOG_ERR("%s: failed to enable", dev->name);
		return err;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lp50xx_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return lp50xx_enable(dev, false);
	case PM_DEVICE_ACTION_RESUME:
		return lp50xx_enable(dev, true);
	case PM_DEVICE_ACTION_TURN_ON:
		return lp50xx_init(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return lp50xx_hw_enable(dev, false);
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(led, lp50xx_led_api) = {
	.get_info	= lp50xx_get_info,
	.set_brightness	= lp50xx_set_brightness,
	.set_color	= lp50xx_set_color,
	.write_channels	= lp50xx_write_channels,
};

#define COLOR_MAPPING(led_node_id)						\
	const uint8_t color_mapping_##led_node_id[] =				\
		DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)							\
	{									\
		.label		= DT_PROP(led_node_id, label),			\
		.index		= DT_PROP(led_node_id, index),			\
		.num_colors	=						\
			DT_PROP_LEN(led_node_id, color_mapping),		\
		.color_mapping	= color_mapping_##led_node_id,			\
	},

#define LP50XX_CONFIG1(n)							\
	(CONFIG1_PWM_DITHERING_EN | CONFIG1_AUTO_INCR_EN |			\
	 CONFIG1_POWER_SAVE_EN |						\
	 (DT_INST_PROP(n, max_curr_opt) ? CONFIG1_MAX_CURRENT_OPT : 0) |	\
	 (DT_INST_PROP(n, log_scale_en) ? CONFIG1_LOG_SCALE_EN : 0))

#define LP50XX_TI_CONFIG(n)							\
	.chip = LP50XX_CHIP_TI,							\
	.config1 = LP50XX_CONFIG1(n),

#define AL5887_CONFIG(n)							\
	.chip = LP50XX_CHIP_AL5887,						\
	.config1 = LP50XX_CONFIG1(n) |						\
		   (DT_INST_PROP(n, phase_shift_disable) ?			\
		    0 : CONFIG1_PHASE_SHIFT_EN),				\
	.global_dimming = DT_INST_PROP(n, global_dimming),

#define LP50XX_DEVICE(n, id, nmodules, nleds, chip_config)			\
	DT_INST_FOREACH_CHILD(n, COLOR_MAPPING)					\
										\
	static const struct led_info id##_leds_##n[] = {			\
		DT_INST_FOREACH_CHILD(n, LED_INFO)				\
	};									\
										\
	static const struct lp50xx_config id##_config_##n = {		\
		.bus			= I2C_DT_SPEC_INST_GET(n),		\
		.gpio_enable		=					\
			GPIO_DT_SPEC_INST_GET_OR(n, enable_gpios, {0}),		\
		.num_modules		= nmodules,				\
		.max_leds		= nleds,				\
		.num_leds		= ARRAY_SIZE(id##_leds_##n),		\
		.leds_info		= id##_leds_##n,			\
		chip_config(n)							\
	};									\
										\
	static uint8_t id##_chan_buf_##n[LP50XX_MAX_CHANNELS(nmodules) + 1];	\
										\
	static struct lp50xx_data id##_data_##n = {				\
		.chan_buf	= id##_chan_buf_##n,				\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(n, lp50xx_pm_action);				\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      lp50xx_init,					\
			      PM_DEVICE_DT_INST_GET(n),				\
			      &id##_data_##n,					\
			      &id##_config_##n,					\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
			      &lp50xx_led_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5009
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5009, LP5012_NUM_MODULES,
				  LP5009_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5012
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5012, LP5012_NUM_MODULES,
				  LP5012_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5018
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5018, LP5024_NUM_MODULES,
				  LP5018_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5024
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5024, LP5024_NUM_MODULES,
				  LP5024_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5030
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5030, LP5036_NUM_MODULES,
				  LP5030_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5036
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, lp5036, LP5036_NUM_MODULES,
				  LP5036_MAX_LEDS, LP50XX_TI_CONFIG)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT diodes_al5887
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP50XX_DEVICE, al5887, LP5036_NUM_MODULES,
				  AL5887_MAX_LEDS, AL5887_CONFIG)
