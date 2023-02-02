/*
 * Copyright (c) 2023 Kistler Instrumente AG
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/lp50xx.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp50xx);

#define LP50XX_DEVICE_CONFIG0		0x0
#define   CONFIG0_CHIP_EN		BIT(6)

#define LP50XX_DEVICE_CONFIG1		0x1
#define   CONFIG1_LED_GLOBAL_OFF	BIT(0)
#define   CONFIG1_MAX_CURRENT_OPT	BIT(1)
#define   CONFIG1_PWM_DITHERING_EN	BIT(2)
#define   CONFIG1_AUTO_INCR_EN		BIT(3)
#define   CONFIG1_POWER_SAVE_EN		BIT(4)
#define   CONFIG1_LOG_SCALE_EN		BIT(5)

#define LP50XX_LED_CONFIG0		0x2
#define   CONFIG0_LEDn_BANK_EN(n)	BIT(n)

#define LP503X_LED_CONFIG1		0x3
#define   CONFIG1_LEDn_BANK_EN(n)	BIT(n)

#define LP5009_24_BANK_BRIGHTNESS	0x3
#define LP503X_BANK_BRIGHTNESS		0x4
#define LP5009_24_BANK_A_COLOR		0x4
#define LP503X_BANK_A_COLOR		0x5
#define LP5009_24_BANK_B_COLOR		0x5
#define LP503X_BANK_B_COLOR		0x6
#define LP5009_24_BANK_C_COLOR		0x6
#define LP503X_BANK_C_COLOR		0x7
#define LP5009_24_LED_BRIGHTNESS_BASE	0x7
#define LP503X_LED_BRIGHTNESS_BASE	0x8
#define LP5009_12_OUT_COLOR_BASE	0xb
#define LP5018_24_OUT_COLOR_BASE	0xf
#define LP503X_OUT_COLOR_BASE		0x14
#define LP5009_12_RESET			0x17
#define LP5018_24_RESET			0x27
#define LP503X_RESET			0x38

/* Expose channels starting from the bank registers. */
#define LP503X_CHANNEL_BASE		LP503X_BANK_BRIGHTNESS
#define LP5009_24_CHANNEL_BASE		LP5009_24_BANK_BRIGHTNESS

/* If brightness is not set in DT use default value */
#define LP50XX_DEFAULT_BRIGHTNESS 100

struct lp50xx_config {
	struct i2c_dt_spec bus;
	uint8_t num_leds;
	bool log_scale_en;
	bool max_curr_opt;
	const struct led_info *leds_info;
	struct gpio_dt_spec enable;
	uint8_t brightness_base;
	uint8_t color_base;
	uint8_t channel_base;
	uint8_t max_channels;
	uint8_t max_leds;
	uint8_t reset_reg;
};

struct lp50xx_data {
	const struct device *dev;
	uint8_t *chan_buf;
	uint8_t *brightness;
	uint16_t *norm_value;
	uint8_t *norm_brightness;
	bool *led_is_on;
#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	struct k_mutex lock;
	bool *blink;
	uint32_t *off_ms;
	uint32_t *period;
	uint32_t *counter;
	struct k_timer timer;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_LP50XX_THREAD_STACK_SIZE);
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */
};

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

static int lp50xx_norm_brightness(const struct device *dev, uint32_t led, uint8_t *color)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint16_t sum;
	int calc;

	if (!led_info) {
		return -EINVAL;
	}

	if (color == NULL) { /* readback color from register */
		uint8_t buf[4];

		buf[0] = config->color_base + LP50XX_COLORS_PER_LED * led_info->index;
		i2c_write_read_dt(&config->bus, buf, 1, &buf[1], 3);
		sum = buf[1] + buf[2] + buf[3];
	} else {
		sum = color[0] + color[1] + color[2];
	}

	if (!sum) {
		LOG_ERR("Color value is zero!");
		return -EINVAL;
	}

	calc = data->brightness[led_info->index] * data->norm_value[led_info->index] / sum;
	if (!calc) {
		LOG_ERR("Normed brightness is zero!");
		return -EINVAL;
	} else if (calc > 100) {
		LOG_WRN("Normed brightness %d capped at 100.", calc);
		calc = 100;
	}

	if (data->brightness[led_info->index] != calc) {
		LOG_DBG("%s set from %d to %d", led_info->label,
			data->brightness[led_info->index], calc);
	}

	data->norm_brightness[led_info->index] = (uint8_t)calc;

	return 0;
}

static int lp50xx_set_led(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[2];

	buf[0] = config->brightness_base + led_info->index;
	buf[1] = (value * 0xff) / 100;
	data->led_is_on[led_info->index] = value ? true : false;

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int lp50xx_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	int ret;

	if (!led_info || value > 100) {
		return -EINVAL;
	}

	/* NOTE: Store last set brightness to reuse it at next LED on */
	if (value) {
		data->brightness[led_info->index] = value;
		if (data->norm_value[led_info->index]) {
			ret = lp50xx_norm_brightness(dev, led_info->index, NULL);
			if (ret) {
				return ret;
			}

			value = data->norm_brightness[led_info->index];
		}
	}

	return lp50xx_set_led(dev, led_info->index, value);
}

static int lp50xx_on(const struct device *dev, uint32_t led)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t value;

	if (!led_info) {
		return -EINVAL;
	}

#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	data->blink[led_info->index] = false;
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */

	value = data->norm_value[led_info->index] ? data->norm_brightness[led_info->index] :
						    data->brightness[led_info->index];

	return lp50xx_set_led(dev, led_info->index, value);
}

static int lp50xx_off(const struct device *dev, uint32_t led)
{
#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	struct lp50xx_data *data = dev->data;
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);

	if (!led_info) {
		return -EINVAL;
	}

#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	data->blink[led_info->index] = false;
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */

	return lp50xx_set_led(dev, led_info->index, 0);
}

static int lp50xx_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			    const uint8_t *color)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	uint8_t buf[4];
	int ret;

	if (!led_info || num_colors != led_info->num_colors) {
		return -EINVAL;
	}

	buf[0] = config->color_base + LP50XX_COLORS_PER_LED * led_info->index;
	buf[1] = color[0];
	buf[2] = color[1];
	buf[3] = color[2];

	if (data->norm_value[led_info->index]) {
		ret = lp50xx_norm_brightness(dev, led_info->index, &buf[1]);
		if (ret) {
			return ret;
		}

		if (data->led_is_on[led_info->index]) {
			ret = lp50xx_set_led(dev, led_info->index,
					data->norm_brightness[led_info->index]);
			if (ret) {
				return ret;
			}
		}
	}

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int lp50xx_write_channels(const struct device *dev, uint32_t start_channel,
				 uint32_t num_channels, const uint8_t *buf)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	bool warn_once = false;
	int reg;
	int led;
	int i;

	if (start_channel >= config->max_channels ||
	    start_channel + num_channels > config->max_channels) {
		return -EINVAL;
	}

	/*
	 * Unfortunately this controller don't support commands split into
	 * two I2C messages.
	 */
	data->chan_buf[0] = config->channel_base + start_channel;
	memcpy(data->chan_buf + 1, buf, num_channels);

	/* NOTE: brightness normalization not implemented */
	for (i = start_channel; i < start_channel + num_channels; i++) {
		reg = config->channel_base + i;
		if (reg >= config->brightness_base &&
		    reg < config->brightness_base + config->max_leds) {
			led = reg - config->brightness_base;
			if (data->norm_value[led]) {
				warn_once = true;
			}
		}

		if (reg >= config->color_base &&
		    reg < config->color_base + LP50XX_COLORS_PER_LED * config->max_leds) {
			led = (reg - config->color_base) / 3;
			if (data->norm_value[led]) {
				warn_once = true;
			}
		}
	}

	if (warn_once) {
		LOG_WRN("Channel API: Brightness normalization not supported.");
	}

	return i2c_write_dt(&config->bus, data->chan_buf, num_channels + 1);
}

#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
static int lp50xx_blink(const struct device *dev, uint32_t led,
			   uint32_t delay_on, uint32_t delay_off)
{
	const struct lp50xx_config *config = dev->config;
	const struct led_info *led_info = lp50xx_led_to_info(config, led);
	struct lp50xx_data *data = dev->data;
	int i;

	/* NOTE: fit sum into uint32_t */
	if (delay_on > (0xFFFFFFFF >> 1) || delay_off > (0xFFFFFFFF >> 1)) {
		return -EINVAL;
	}

	if (!led_info) {
		return -EINVAL;
	}

	delay_on /= CONFIG_LP50XX_TIMER_RESOLUTION_MS;
	delay_off /= CONFIG_LP50XX_TIMER_RESOLUTION_MS;

	if (delay_on && !delay_off) {
		LOG_WRN("delay_off = 0 ms, set %s on", led_info->label);
		return lp50xx_on(dev, led);
	}

	if (!delay_on) {
		LOG_WRN("delay_on = 0 ms, set %s off", led_info->label);
		return lp50xx_off(dev, led);
	}

	data->off_ms[led_info->index] = delay_off;
	data->period[led_info->index] = delay_on + delay_off;
	data->blink[led_info->index] = true;
	k_mutex_lock(&data->lock, K_FOREVER);
	/* new LED starts to blink, sync all counters */
	for (i = 0; i < config->num_leds; i++) {
		data->counter[i] = 0;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static inline void lp50xx_handle_blink(struct lp50xx_data *data, int led)
{
	uint8_t value;

	if (!data->counter[led]) {
		lp50xx_set_led(data->dev, led, 0);
	} else if (data->counter[led] == data->off_ms[led]) {
		value = data->norm_value[led] ?
			data->norm_brightness[led] : data->brightness[led];
		lp50xx_set_led(data->dev, led, value);
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->counter[led] += 1;
	data->counter[led] %= data->period[led];
	k_mutex_unlock(&data->lock);
}

static void lp50xx_blink_thread(const struct lp50xx_config *config,
				   struct lp50xx_data *data)
{
	int i;

	while (1) {
		k_timer_status_sync(&data->timer);
		for (i = 0; i < config->num_leds; i++) {
			if (data->blink[i]) {
				lp50xx_handle_blink(data, i);
			}
		}
	}
}
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */

static const struct led_driver_api lp50xx_led_api = {
	.on		= lp50xx_on,
	.off		= lp50xx_off,
	.get_info	= lp50xx_get_info,
	.set_brightness	= lp50xx_set_brightness,
	.set_color	= lp50xx_set_color,
	.write_channels	= lp50xx_write_channels,
#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	.blink		= lp50xx_blink,
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */
};

static int lp50xx_init(const struct device *dev)
{
	const struct lp50xx_config *config = dev->config;
	struct lp50xx_data *data = dev->data;
	uint8_t buf[3];
	int err;
	int i;

	data->dev = dev;

	if (config->enable.port == NULL) {
		LOG_INF("%s: no GPIO enable in use.", dev->name);
	} else {
		if (!device_is_ready(config->enable.port)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->enable, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Could not configure enable GPIO (%d)", err);
			return err;
		}
	}

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	if (config->num_leds > config->max_leds) {
		LOG_ERR("%s: invalid number of LEDs %d (max %d)",
			dev->name, config->num_leds, config->max_leds);
		return -EINVAL;
	}

	/* Reset registers */
	buf[0] = config->reset_reg;
	buf[1] = 0xFF;
	err = i2c_write_dt(&config->bus, buf, 2);
	if (err < 0) {
		return err;
	}

	/* Disable bank control for all LEDs. */
	buf[0] = LP50XX_LED_CONFIG0;
	buf[1] = 0;
	buf[2] = 0;
	err = i2c_write_dt(&config->bus, buf, 3);
	if (err < 0) {
		return err;
	}

	/* Enable LED controller. */
	buf[0] = LP50XX_DEVICE_CONFIG0;
	buf[1] = CONFIG0_CHIP_EN;
	err = i2c_write_dt(&config->bus, buf, 2);
	if (err < 0) {
		return err;
	}

	for (i = 0; i < config->num_leds; i++) {
		LOG_DBG("%s init brightness %3d %%", config->leds_info[i].label,
			data->brightness[i]);
		LOG_DBG("%s norm-value %3d ", config->leds_info[i].label,
			data->norm_value[i]);
		err = lp50xx_off(dev, i);
		if (err < 0) {
			return err;
		}
	}

#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
	k_tid_t tid;

	tid = k_thread_create(&data->thread, data->stack, CONFIG_LP50XX_THREAD_STACK_SIZE,
			      (k_thread_entry_t)lp50xx_blink_thread, (void *)config, (void *)data,
			      NULL, CONFIG_LP50XX_THREAD_PRIO, 0, K_MSEC(1));
	k_thread_name_set(tid, dev->name);
	k_mutex_init(&data->lock);
	k_timer_init(&data->timer, NULL, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_LP50XX_TIMER_RESOLUTION_MS),
		      K_MSEC(CONFIG_LP50XX_TIMER_RESOLUTION_MS));
#endif /* CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK */

	/* Apply configuration. */
	buf[0] = LP50XX_DEVICE_CONFIG1;
	buf[1] = CONFIG1_PWM_DITHERING_EN | CONFIG1_AUTO_INCR_EN | CONFIG1_POWER_SAVE_EN;
	if (config->max_curr_opt) {
		buf[1] |= CONFIG1_MAX_CURRENT_OPT;
	}
	if (config->log_scale_en) {
		buf[1] |= CONFIG1_LOG_SCALE_EN;
	}

	return i2c_write_dt(&config->bus, buf, 2);
}

#define COLOR_MAPPING(led_node_id)					\
static const uint8_t color_mapping_##led_node_id[] =			\
				DT_PROP(led_node_id, color_mapping);

#define LED_INFO(led_node_id)						\
{									\
	.label		= DT_PROP(led_node_id, label),			\
	.index		= DT_PROP(led_node_id, index),			\
	.num_colors	= DT_PROP_LEN(led_node_id, color_mapping),	\
	.color_mapping	= color_mapping_##led_node_id,			\
},

#if CONFIG_LP50XX_ALLOW_SOFTWARE_BLINK
#define LP50XX_DEFINE_DATA_ARRAY(inst)							\
	static bool lp50xx_blink_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};		\
	static uint32_t lp50xx_off_ms_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};	\
	static uint32_t lp50xx_period_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};	\
	static uint32_t lp50xx_counter_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};	\
	static struct lp50xx_data lp50xx_data_##inst = {				\
		.chan_buf		= lp50xx_chan_buf_##inst,			\
		.brightness		= lp50xx_brightness_##inst,			\
		.norm_value		= lp50xx_norm_value_##inst,			\
		.norm_brightness	= lp50xx_norm_brightness_##inst,		\
		.led_is_on		= lp50xx_led_is_on_##inst,			\
		.blink			= lp50xx_blink_##inst,				\
		.off_ms			= lp50xx_off_ms_##inst,				\
		.period			= lp50xx_period_##inst,				\
		.counter		= lp50xx_counter_##inst,			\
	};
#else
#define LP50XX_DEFINE_DATA_ARRAY(inst)						\
	static struct lp50xx_data lp50xx_data_##inst = {			\
		.chan_buf		= lp50xx_chan_buf_##inst,		\
		.brightness		= lp50xx_brightness_##inst,		\
		.norm_value		= lp50xx_norm_value_##inst,		\
		.norm_brightness	= lp50xx_norm_brightness_##inst,	\
		.led_is_on		= lp50xx_led_is_on_##inst,		\
	};
#endif

#define LP50XX_DEVICE(inst, b_base, c_base, ch_base, max_ch, max_l, r_reg)		\
											\
DT_INST_FOREACH_CHILD(inst, COLOR_MAPPING)						\
											\
const struct led_info lp50xx_leds_##inst[] = {						\
	DT_INST_FOREACH_CHILD(inst, LED_INFO)						\
};											\
											\
static uint8_t lp50xx_chan_buf_##inst[max_ch + 1];					\
											\
static struct lp50xx_config lp50xx_config_##inst = {					\
	.bus			= I2C_DT_SPEC_INST_GET(inst),				\
	.num_leds		= ARRAY_SIZE(lp50xx_leds_##inst),			\
	.max_curr_opt		= DT_INST_PROP(inst, max_curr_opt),			\
	.log_scale_en		= DT_INST_PROP(inst, log_scale_en),			\
	.leds_info		= lp50xx_leds_##inst,					\
	.enable			= GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),	\
	.brightness_base	= b_base,						\
	.color_base		= c_base,						\
	.channel_base		= ch_base,						\
	.max_channels		= max_ch,						\
	.max_leds		= max_l,						\
	.reset_reg		= r_reg,						\
};											\
											\
static uint8_t lp50xx_brightness_##inst[] = {						\
	DT_INST_FOREACH_CHILD_SEP_VARGS(inst, DT_PROP_OR, (,), default_brightness,	\
					LP50XX_DEFAULT_BRIGHTNESS)			\
};											\
static uint16_t lp50xx_norm_value_##inst[] = {						\
	DT_INST_FOREACH_CHILD_SEP_VARGS(inst, DT_PROP_OR, (,), norm_value, 0)		\
};											\
static uint8_t lp50xx_norm_brightness_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};	\
static bool lp50xx_led_is_on_##inst[ARRAY_SIZE(lp50xx_leds_##inst)] = {0};		\
LP50XX_DEFINE_DATA_ARRAY(inst)								\
											\
DEVICE_DT_INST_DEFINE(inst, &lp50xx_init, NULL, &lp50xx_data_##inst,			\
		      &lp50xx_config_##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &lp50xx_led_api);

#define LP503X_DEVICE(inst)									\
	LP50XX_DEVICE(inst, LP503X_LED_BRIGHTNESS_BASE, LP503X_OUT_COLOR_BASE,			\
		      LP503X_CHANNEL_BASE, LP503X_NUM_CHANNELS, LP503X_MAX_LEDS,		\
		      LP503X_RESET)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp503x
DT_INST_FOREACH_STATUS_OKAY(LP503X_DEVICE)

#define LP5009_DEVICE(inst)									\
	LP50XX_DEVICE(inst, LP5009_24_LED_BRIGHTNESS_BASE, LP5009_12_OUT_COLOR_BASE,		\
		      LP5009_24_CHANNEL_BASE, LP5009_12_NUM_CHANNELS, LP5009_12_MAX_LEDS,	\
		      LP5009_12_RESET)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5009
DT_INST_FOREACH_STATUS_OKAY(LP5009_DEVICE)

#define LP5012_DEVICE(inst)									\
	LP50XX_DEVICE(inst, LP5009_24_LED_BRIGHTNESS_BASE, LP5009_12_OUT_COLOR_BASE,		\
		      LP5009_24_CHANNEL_BASE, LP5009_12_NUM_CHANNELS, LP5009_12_MAX_LEDS,	\
		      LP5009_12_RESET)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5012
DT_INST_FOREACH_STATUS_OKAY(LP5012_DEVICE)

#define LP5018_DEVICE(inst)									\
	LP50XX_DEVICE(inst, LP5009_24_LED_BRIGHTNESS_BASE, LP5018_24_OUT_COLOR_BASE,		\
		      LP5009_24_CHANNEL_BASE, LP5018_24_NUM_CHANNELS, LP5018_24_MAX_LEDS,	\
		      LP5018_24_RESET)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5018
DT_INST_FOREACH_STATUS_OKAY(LP5018_DEVICE)

#define LP5024_DEVICE(inst)									\
	LP50XX_DEVICE(inst, LP5009_24_LED_BRIGHTNESS_BASE, LP5018_24_OUT_COLOR_BASE,		\
		      LP5009_24_CHANNEL_BASE, LP5018_24_NUM_CHANNELS, LP5018_24_MAX_LEDS,	\
		      LP5018_24_RESET)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5024
DT_INST_FOREACH_STATUS_OKAY(LP5024_DEVICE)
