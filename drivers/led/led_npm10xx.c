/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_led

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_npm10xx, CONFIG_LED_LOG_LEVEL);

/* GPIO Register Offsets */
#define NPM10_GPIO_CONFIG0 0xA0U
#define NPM10_GPIO_USAGE0  0xA3U

/* LED Driver Register Offsets */
#define NPM10_LEDDRV_MODE0   0x90U
#define NPM10_LEDDRV_DUTY0   0x93U
#define NPM10_LEDDRV_TIMEON  0x96U
#define NPM10_LEDDRV_TIMEOFF 0x97U
#define NPM10_LEDDRV_TASKS   0x98U
#define NPM10_LEDDRV_CONFIG  0x99U
#define NPM10_LEDDRV_STATUS  0x9AU

/* GPIO USAGEx (0xA3–0xA5) */
#define GPIO_USAGE_SEL_LED (7U << 0)

/* LED MODEx (0x90–0x92) */
#define LEDDRV_MODE_SEL_OFF    0U
#define LEDDRV_MODE_SEL_ON     1U
#define LEDDRV_MODE_SEL_CHARGE 2U
#define LEDDRV_MODE_SEL_ERROR  3U
#define LEDDRV_MODE_SEL_VBUS   4U
#define LEDDRV_MODE_SEL_PWM    5U
#define LEDDRV_MODE_SEL_BLINK  6U

/* LED TASKS (0x98) */
#define LEDDRV_TASKS_BLINK BIT(0)

/* LED CONFIG (0x99) */
#define LEDDRV_CONFIG_ILED0        BIT(0)
#define LEDDRV_CONFIG_ILED1        BIT(1)
#define LEDDRV_CONFIG_ILED2        BIT(2)
#define LEDDRV_CONFIG_RGB          BIT(3)
#define LEDDRV_CONFIG_LOOP         BIT(4)
#define LEDDRV_CONFIG_DOUBLE       BIT(5)
#define LEDDRV_CONFIG_PWMFREQ_MASK (BIT_MASK(2) << 6)

#define NPM10_LED_MAX_NUM 3U

enum led_npm10xx_mode {
	MODE_HOST = 0U,
	MODE_CHARGE,
	MODE_ERROR,
	MODE_VBUS,
};

enum led_npm10xx_blink_mode {
	BLINK_ONCE = 0U,
	BLINK_TWICE,
	BLINK_LOOP,
};

enum led_npm10xx_state {
	LED_OFF,
	LED_ON,
	LED_PWM,
	LED_BLINK,
	LED_AUTO,
};

struct led_npm10xx_node {
	struct led_info info;
	enum led_npm10xx_mode mode;
	uint8_t current;
};

struct led_npm10xx_config {
	struct i2c_dt_spec i2c;
	enum led_npm10xx_blink_mode blink_mode;
	const struct led_npm10xx_node *leds;
	uint8_t leds_num;
	uint8_t pwm_freq;
	bool phase_shift;
};

struct led_npm10xx_data {
	enum led_npm10xx_state *led_state;
};

static const struct linear_range delay_range = LINEAR_RANGE_INIT(8, 8, 0, 255);

/* Could do PWM with max/zero duty, but using on/off states leads to lower power consumption */
static int led_npm10xx_on_off(const struct device *dev, uint32_t led, bool on)
{
	int ret;
	const struct led_npm10xx_config *config = dev->config;
	struct led_npm10xx_data *data = dev->data;

	if (data->led_state[led] == (on ? LED_ON : LED_OFF)) {
		return 0;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LEDDRV_MODE0 + config->leds[led].info.index,
				    on ? LEDDRV_MODE_SEL_ON : LEDDRV_MODE_SEL_OFF);
	if (ret == 0) {
		data->led_state[led] = on ? LED_ON : LED_OFF;
	}

	return ret;
}

static int led_npm10xx_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	int ret;
	uint16_t time_on, time_off;
	uint8_t buf[3];
	const struct led_npm10xx_config *config = dev->config;
	struct led_npm10xx_data *data = dev->data;

	if (led >= config->leds_num) {
		LOG_ERR("LED idx %d out of range [0, %d]", led, config->leds_num - 1);
		return -EINVAL;
	}

	if (config->leds[led].mode != MODE_HOST) {
		LOG_ERR("LED idx %d is not configured to be controlled by host", led);
		return -EINVAL;
	}

	/* special case - stop blinking */
	if (delay_on == 0U) {
		return led_npm10xx_on_off(dev, led, false);
	} else if (delay_off == 0U) {
		/* keep the brightness */
		ret = i2c_reg_write_byte_dt(&config->i2c,
					    NPM10_LEDDRV_MODE0 + config->leds[led].info.index,
					    LEDDRV_MODE_SEL_PWM);
		if (ret == 0) {
			data->led_state[led] = LED_PWM;
		}

		return ret;
	}

	ret = linear_range_get_index(&delay_range, delay_on, &time_on);
	if (ret < 0) {
		LOG_ERR("delay_on (%d) out of range [8,2048]", delay_on);
		return ret;
	}

	ret = linear_range_get_index(&delay_range, delay_off, &time_off);
	if (ret < 0) {
		LOG_ERR("delay_off (%d) out of range [8,2048]", delay_off);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LEDDRV_MODE0 + config->leds[led].info.index,
				    LEDDRV_MODE_SEL_BLINK);
	if (ret < 0) {
		return ret;
	}

	data->led_state[led] = LED_BLINK;

	buf[0] = (uint8_t)time_on;
	buf[1] = (uint8_t)time_off;
	buf[2] = LEDDRV_TASKS_BLINK;

	return i2c_burst_write_dt(&config->i2c, NPM10_LEDDRV_TIMEON, buf, sizeof(buf));
}

static int led_npm10xx_get_info(const struct device *dev, uint32_t led,
				const struct led_info **info)
{
	const struct led_npm10xx_config *config = dev->config;

	if (led >= config->leds_num) {
		LOG_ERR("LED idx %d out of range [0, %d]", led, config->leds_num - 1);
		return -EINVAL;
	}

	*info = &config->leds[led].info;

	return 0;
}

static int led_npm10xx_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	int ret;
	const struct led_npm10xx_config *config = dev->config;
	struct led_npm10xx_data *data = dev->data;

	if (led >= config->leds_num) {
		LOG_ERR("LED idx %d out of range [0, %d]", led, config->leds_num - 1);
		return -EINVAL;
	}

	if (config->leds[led].mode == MODE_ERROR) {
		LOG_ERR("LED mode \"error\" has no brightness control");
		return -EINVAL;
	}

	if (config->leds[led].mode == MODE_HOST && data->led_state[led] != LED_BLINK &&
	    (value == 0 || (value == LED_BRIGHTNESS_MAX && !config->phase_shift))) {
		return led_npm10xx_on_off(dev, led, value == LED_BRIGHTNESS_MAX);
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_LEDDRV_DUTY0 + config->leds[led].info.index,
				    (uint8_t)((uint16_t)value * 255 / LED_BRIGHTNESS_MAX));
	if (ret < 0) {
		return ret;
	}

	if (data->led_state[led] == LED_OFF || data->led_state[led] == LED_ON) {
		ret = i2c_reg_write_byte_dt(&config->i2c,
					    NPM10_LEDDRV_MODE0 + config->leds[led].info.index,
					    LEDDRV_MODE_SEL_PWM);
		if (ret == 0) {
			data->led_state[led] = LED_PWM;
		}
	}

	return ret;
}

static int led_npm10xx_init(const struct device *dev)
{
	int ret;
	uint8_t mode;
	uint8_t cfg = 0U;
	const struct led_npm10xx_config *config = dev->config;
	struct led_npm10xx_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < config->leds_num; i++) {
		/* writing 0 to config disconnects input/output buffers and pulls */
		ret = i2c_reg_write_byte_dt(&config->i2c,
					    NPM10_GPIO_CONFIG0 + config->leds[i].info.index, 0U);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c,
					    NPM10_GPIO_USAGE0 + config->leds[i].info.index,
					    GPIO_USAGE_SEL_LED);
		if (ret < 0) {
			return ret;
		}

		/* current is part of the global config */
		cfg |= config->leds[i].current << config->leds[i].info.index;

		if (config->leds[i].mode == MODE_HOST) {
			mode = LEDDRV_MODE_SEL_OFF;
			data->led_state[i] = LED_OFF;
		} else {
			mode = config->leds[i].mode + 1;
			data->led_state[i] = LED_AUTO;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c,
					    NPM10_LEDDRV_MODE0 + config->leds[i].info.index, mode);
		if (ret < 0) {
			return ret;
		}
	}

	cfg |= FIELD_PREP(LEDDRV_CONFIG_RGB, config->phase_shift) |
	       FIELD_PREP(LEDDRV_CONFIG_LOOP, config->blink_mode == BLINK_LOOP) |
	       FIELD_PREP(LEDDRV_CONFIG_DOUBLE, config->blink_mode == BLINK_TWICE) |
	       FIELD_PREP(LEDDRV_CONFIG_PWMFREQ_MASK, config->pwm_freq);

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_LEDDRV_CONFIG, cfg);
}

static DEVICE_API(led, led_npm10xx_api) = {
	.set_brightness = led_npm10xx_set_brightness,
	.blink = led_npm10xx_blink,
	.get_info = led_npm10xx_get_info,
};

#define LED_NPM10XX_NODE_DEFINE(led_node)                                                          \
	{                                                                                          \
		.info =                                                                            \
			{                                                                          \
				.label = DT_PROP_OR(led_node, label, ""),                          \
				.index = DT_PROP(led_node, index),                                 \
			},                                                                         \
		.mode = DT_ENUM_IDX(led_node, led_mode),                                           \
		.current = DT_ENUM_IDX(led_node, drive_current_ma),                                \
	},

#define LED_NPM10XX_DEFINE(n)                                                                      \
	BUILD_ASSERT(DT_INST_CHILD_NUM(n) <= NPM10_LED_MAX_NUM, "Too many LEDs (max 3 expected)"); \
                                                                                                   \
	static enum led_npm10xx_state led_npm10xx_states##n[DT_INST_CHILD_NUM(n)];                 \
	static struct led_npm10xx_data led_npm10xx_data##n = {led_npm10xx_states##n};              \
                                                                                                   \
	const static struct led_npm10xx_node led_npm10xx_nodes##n[DT_INST_CHILD_NUM(n)] = {        \
		DT_INST_FOREACH_CHILD(n, LED_NPM10XX_NODE_DEFINE)};                                \
	static const struct led_npm10xx_config led_npm10xx_config##n = {                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.pwm_freq = DT_INST_ENUM_IDX(n, pwm_freq_hz),                                      \
		.blink_mode = DT_INST_ENUM_IDX(n, blink_mode),                                     \
		.leds = led_npm10xx_nodes##n,                                                      \
		.leds_num = DT_INST_CHILD_NUM(n),                                                  \
		.phase_shift = DT_INST_PROP(n, phase_shift_enable),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_npm10xx_init, NULL, &led_npm10xx_data##n,                    \
			      &led_npm10xx_config##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &led_npm10xx_api);

DT_INST_FOREACH_STATUS_OKAY(LED_NPM10XX_DEFINE)
