/*
 * Copyright (c) 2025 Van Petrosyan.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9533

/**
 * @file
 * @brief LED driver for the PCA9533 I2C LED driver (7-bit slave address 0x62)
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca9533, CONFIG_LED_LOG_LEVEL);

#define PCA9533_CHANNELS 4U /* LED0 - LED3 */
#define PCA9533_ENGINES  2U

#define PCA9533_INPUT 0x00 /* read-only pin state            */
#define PCA9533_PSC0  0x01 /* BLINK0 period prescaler        */
#define PCA9533_PWM0  0x02 /* BLINK0 duty                    */
#define PCA9533_PSC1  0x03
#define PCA9533_PWM1  0x04
#define PCA9533_LS0   0x05 /* LED selector (2 bits per LED)  */

/* LS register bit fields (6.3.6, Table 10) */
#define LS_FUNC_OFF  0x0 /* high-Z - LED off               */
#define LS_FUNC_ON   0x1 /* output LOW - LED on            */
#define LS_FUNC_PWM0 0x2
#define LS_FUNC_PWM1 0x3
#define LS_SHIFT(ch) ((ch) * 2) /* 2 bits per LED in LS register */
#define LS_MASK(ch)  (0x3u << LS_SHIFT(ch))

/* Blink period limits derived from PSC range 0 - 255 (6.3.2/6.3.4) */
#define BLINK_MIN_MS 7U    /* (0+1)/152 = 6.58 ms  - ceil   */
#define BLINK_MAX_MS 1685U /* (255+1)/152 = 1.684 s - ceil  */

/* Default PWM frequency when using set_brightness (152 Hz) */
#define PCA9533_DEFAULT_PSC 0x00

struct pca9533_config {
	struct i2c_dt_spec i2c;
};

struct pca9533_data {
	/* run-time bookkeeping for the two PWM engines */
	uint8_t pwm_val[PCA9533_ENGINES];      /* duty (0-255) programmed into PWMx         */
	uint8_t psc_val[PCA9533_ENGINES];      /* prescaler programmed into PSCx            */
	uint8_t engine_users[PCA9533_ENGINES]; /* bitmask of LEDs using engine 0 / 1        */
};

/**
 * @brief Convert period in ms to PSC register value
 *
 * Formula: psc = round(period_ms * 152 / 1000) - 1
 *
 * @param period_ms Blink period in milliseconds
 * @return uint8_t PSC register value (clamped to 0-255)
 */
static uint8_t ms_to_psc(uint32_t period_ms)
{
	int32_t tmp = (period_ms * 152U + 500U) / 1000U;

	return CLAMP(tmp - 1, 0U, UINT8_MAX);
}

/**
 * @brief Update LS bits for one LED (RMW operation)
 *
 * @param i2c I2C device specification
 * @param led LED index (0-3)
 * @param func Desired function (LS_FUNC_*)
 * @return int 0 on success, negative errno on error
 */
static int ls_update(const struct i2c_dt_spec *i2c, uint8_t led, uint8_t func)
{
	return i2c_reg_update_byte_dt(i2c, PCA9533_LS0, LS_MASK(led), func << LS_SHIFT(led));
}

/**
 * Return engine index (0 - PCA9533_ENGINES-1) that currently drives @p led,
 * or 0xFF if the LED is not routed to any engine (OFF / ON state).
 */
static uint8_t find_engine_for_led(const struct pca9533_data *data, uint8_t led)
{
	for (uint8_t ch = 0; ch < PCA9533_ENGINES; ch++) {
		if (data->engine_users[ch] & BIT(led)) {
			return ch;
		}
	}
	return 0xFF;
}

/**
 * @brief Find an in-use engine whose parameters already match (duty, psc)
 *
 * @param data  Driver data
 * @param duty  Desired duty
 * @param psc   Desired prescaler
 * @param[out] out_ch  Matching engine ID
 * @return 0 if found, -ENOENT otherwise
 */
static int engine_find_match(const struct pca9533_data *data, uint8_t duty, uint8_t psc,
			     uint8_t *out_ch)
{
	for (uint8_t ch = 0; ch < PCA9533_ENGINES; ch++) {
		if (data->engine_users[ch] && data->pwm_val[ch] == duty &&
		    data->psc_val[ch] == psc) {
			*out_ch = ch;
			return 0;
		}
	}
	return -ENOENT;
}

/**
 * @brief Claim a PWM engine matching (psc,duty) or find a free one
 *
 * Engine allocation strategy:
 * 1. Reuse engine with exact (duty, psc) if exists
 * 2. Use free engine if available
 * 3. Return -EBUSY if no match
 *
 * @param data Driver data
 * @param duty Desired duty cycle (0-255)
 * @param psc Desired prescaler value
 * @param[out] out_ch Acquired engine ID (0 or 1)
 * @return int 0 on success, -EBUSY if no engine available
 */
static int engine_acquire(struct pca9533_data *data, uint8_t duty, uint8_t psc, uint8_t *out_ch)
{
	/* Check for existing engine with matching parameters */
	for (uint8_t ch = 0; ch < PCA9533_ENGINES; ch++) {
		if (data->engine_users[ch] && data->pwm_val[ch] == duty &&
		    data->psc_val[ch] == psc) {
			*out_ch = ch;
			return 0;
		}
	}
	/* Find free engine */
	for (uint8_t ch = 0; ch < PCA9533_ENGINES; ch++) {
		if (data->engine_users[ch] == 0) {
			*out_ch = ch;
			return 0;
		}
	}

	return -EBUSY;
}

/**
 * @brief Bind LED to a PWM engine
 *
 * @param data Driver data
 * @param led LED index (0-3)
 * @param ch Engine ID (0 or 1)
 */
static void engine_bind(struct pca9533_data *data, uint8_t led, uint8_t ch)
{
	data->engine_users[ch] |= BIT(led);
}

/**
 * @brief Release LED from its current PWM engine
 *
 * @param data Driver data
 * @param led LED index (0-3)
 */
static void engine_release(struct pca9533_data *data, uint8_t led)
{
	uint8_t ch = find_engine_for_led(data, led);

	if (ch < PCA9533_ENGINES) {
		data->engine_users[ch] &= ~BIT(led);
	}
}

static int pca9533_led_set_brightness(const struct device *dev, uint32_t led, uint8_t percent)
{
	const struct pca9533_config *config = dev->config;
	struct pca9533_data *data = dev->data;
	uint8_t cur, duty, ch;
	int ret;

	if (led >= PCA9533_CHANNELS) {
		LOG_ERR("Invalid LED index: %u", led);
		return -EINVAL;
	}

	if (percent == 0) {
		LOG_DBG("LED%u -> OFF", led);
		ret = ls_update(&config->i2c, led, LS_FUNC_OFF);
		if (ret == 0) {
			engine_release(data, led);
		}
		return ret;
	}
	if (percent == LED_BRIGHTNESS_MAX) {
		LOG_DBG("LED%u -> ON", led);
		ret = ls_update(&config->i2c, led, LS_FUNC_ON);
		if (ret == 0) {
			engine_release(data, led);
		}
		return ret;
	}

	duty = (percent * UINT8_MAX) / LED_BRIGHTNESS_MAX;
	cur = find_engine_for_led(data, led);

	/* Sole-user fast-path, with reuse-check */
	if (cur < PCA9533_ENGINES && data->engine_users[cur] == BIT(led)) {
		uint8_t match;

		/* Can we piggy-back on an existing engine already at (duty, default psc)? */
		if (!engine_find_match(data, duty, PCA9533_DEFAULT_PSC, &match) && match != cur) {
			LOG_DBG("LED%u moves from engine %u to matching engine %u", led, cur,
				match);
			engine_release(data, led);
			engine_bind(data, led, match);
			return ls_update(&config->i2c, led, match ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
		}

		/* Otherwise retune in place */
		ret = 0;
		if (data->pwm_val[cur] != duty) {
			LOG_DBG("LED%u retune duty %u on engine %u", led, duty, cur);
			ret = i2c_reg_write_byte_dt(&config->i2c, cur ? PCA9533_PWM1 : PCA9533_PWM0,
						    duty);
			if (ret == 0) {
				data->pwm_val[cur] = duty;
			}
		}
		return ret;
	}

	/* Acquire new engine - use default PSC for brightness control */
	ret = engine_acquire(data, duty, PCA9533_DEFAULT_PSC, &ch);
	if (ret) {
		LOG_WRN("No PWM engine available for LED %u", led);
		return ret;
	}

	/* If engine is new (no users), program its registers */
	if (data->engine_users[ch] == 0) {
		/* Set default period (152 Hz) */
		ret = i2c_reg_write_byte_dt(&config->i2c, ch ? PCA9533_PSC1 : PCA9533_PSC0,
					    PCA9533_DEFAULT_PSC);
		if (ret == 0) {
			ret = i2c_reg_write_byte_dt(&config->i2c, ch ? PCA9533_PWM1 : PCA9533_PWM0,
						    duty);
		}
		if (ret) {
			LOG_ERR("Failed to program engine %u: %d", ch, ret);
			return ret;
		}
		data->psc_val[ch] = PCA9533_DEFAULT_PSC;
		data->pwm_val[ch] = duty;
	}

	/* Bind LED to new engine and update hardware */
	LOG_DBG("LED%u uses engine %u (duty %u)", led, ch, duty);
	engine_release(data, led);
	engine_bind(data, led, ch);
	return ls_update(&config->i2c, led, ch ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
}

static int pca9533_led_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	const struct pca9533_config *config = dev->config;
	struct pca9533_data *data = dev->data;
	int ret;
	uint8_t ch, duty, psc, cur;
	uint32_t period, duty32;

	if (led >= PCA9533_CHANNELS) {
		LOG_ERR("Invalid LED index: %u", led);
		return -EINVAL;
	}

	period = delay_on + delay_off;
	if (period < BLINK_MIN_MS || period > BLINK_MAX_MS) {
		LOG_ERR("Invalid blink period: %u ms (min: %u, max: %u)", period, BLINK_MIN_MS,
			BLINK_MAX_MS);
		return -ENOTSUP;
	}

	/* Calculate duty cycle with overflow protection */
	duty32 = (delay_on * 256U) / period;
	duty = CLAMP(duty32, 0, UINT8_MAX);
	psc = ms_to_psc(period);
	cur = find_engine_for_led(data, led);

	/* Sole-user fast-path with reuse-check */
	if (cur < PCA9533_ENGINES && data->engine_users[cur] == BIT(led)) {
		uint8_t match;

		/* Look for another engine already at the desired (psc,duty) */
		if (!engine_find_match(data, duty, psc, &match) && match != cur) {
			LOG_DBG("LED%u moves from engine %u to matching engine %u (blink)", led,
				cur, match);
			engine_release(data, led);
			engine_bind(data, led, match);
			return ls_update(&config->i2c, led, match ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
		}

		/* Otherwise update this engine in place */
		ret = 0;
		if (data->pwm_val[cur] != duty || data->psc_val[cur] != psc) {
			ret = i2c_reg_write_byte_dt(&config->i2c, cur ? PCA9533_PSC1 : PCA9533_PSC0,
						    psc);
			if (ret == 0) {
				ret = i2c_reg_write_byte_dt(
					&config->i2c, cur ? PCA9533_PWM1 : PCA9533_PWM0, duty);
			}
			if (ret == 0) {
				data->psc_val[cur] = psc;
				data->pwm_val[cur] = duty;
			}
		}
		return ret;
	}

	/* Acquire new engine with desired parameters */
	ret = engine_acquire(data, duty, psc, &ch);
	if (ret) {
		LOG_WRN("No PWM engine available for LED %u blink", led);
		return ret;
	}

	/* If engine is new (no users), program it */
	if (data->engine_users[ch] == 0) {
		ret = i2c_reg_write_byte_dt(&config->i2c, ch ? PCA9533_PSC1 : PCA9533_PSC0, psc);
		if (ret == 0) {
			ret = i2c_reg_write_byte_dt(&config->i2c, ch ? PCA9533_PWM1 : PCA9533_PWM0,
						    duty);
		}
		if (ret) {
			LOG_ERR("Failed to program engine %u: %d", ch, ret);
			return ret;
		}
		data->psc_val[ch] = psc;
		data->pwm_val[ch] = duty;
	}

	LOG_DBG("LED%u now on engine %u (psc %u duty %u)", led, ch, psc, duty);
	engine_release(data, led);
	engine_bind(data, led, ch);
	return ls_update(&config->i2c, led, ch ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
}

static int pca9533_led_init_chip(const struct device *dev)
{
	struct pca9533_data *data = dev->data;

	for (uint8_t i = 0; i < PCA9533_ENGINES; i++) {
		data->engine_users[i] = 0;
	}

	/* The Power-On Reset already initializes the registers to their default state
	 * no need to write them here. We'll just reset bookkeeping
	 */

	return 0;
}

static int pca9533_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		return pca9533_led_init_chip(dev);
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int pca9533_led_init(const struct device *dev)
{
	const struct pca9533_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	return pm_device_driver_init(dev, pca9533_pm_action);
}

static const struct led_driver_api pca9533_led_api = {
	.blink = pca9533_led_blink,
	.set_brightness = pca9533_led_set_brightness,
};

#define PCA9533_DEVICE(id)                                                                         \
	static const struct pca9533_config pca9533_##id##_cfg = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(id),                                                   \
	};                                                                                         \
	static struct pca9533_data pca9533_##id##_data;                                            \
	PM_DEVICE_DT_INST_DEFINE(id, pca9533_pm_action);                                           \
	DEVICE_DT_INST_DEFINE(id, &pca9533_led_init, PM_DEVICE_DT_INST_GET(id),                    \
			      &pca9533_##id##_data, &pca9533_##id##_cfg, POST_KERNEL,              \
			      CONFIG_LED_INIT_PRIORITY, &pca9533_led_api);

DT_INST_FOREACH_STATUS_OKAY(PCA9533_DEVICE)
