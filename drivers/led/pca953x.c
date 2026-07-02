/*
 * Copyright (c) 2025 Van Petrosyan.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LED driver for the PCA953x I2C LED driver series
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pca953x, CONFIG_LED_LOG_LEVEL);

#define PCA953X_ENGINES 2U

#define LS_LED_BITS     2u /* 2 bits per LED in LS register  */
#define LS_LEDS_PER_REG (8u / LS_LED_BITS)
#define LS_SHIFT(led)   ((led % LS_LEDS_PER_REG) * LS_LED_BITS)
#define LS_MASK(led)    (0x3u << LS_SHIFT(led))

/* Register addresses */
#define PCA953X_INPUT0          0x00 /* read-only pin state              */
/* The following registers are offset by the number of input registers */
#define PCA953X_PSC(config, ch)                   /* ch=0: PSC0; ch=1: PSC1           */           \
	((ch ? 0x02 : 0x00) + config->ninput_reg) /* BLINK0/1 period prescaler        */
#define PCA953X_PWM(config, ch)                   /* ch=0: PWM0; ch=1: PWM1           */           \
	((ch ? 0x03 : 0x01) + config->ninput_reg) /* BLINK0/1 duty                    */
#define PCA953X_LS(config, led)                                                                    \
	(0x04 + config->ninput_reg +                                                               \
	 led / LS_LEDS_PER_REG) /* LED selector LS0-LS3 (2 bits per LED)    */

/* LS register bit fields (6.3.6, Table 10) */
#define LS_FUNC_OFF  0x0 /* high-Z - LED off               */
#define LS_FUNC_ON   0x1 /* output LOW - LED on            */
#define LS_FUNC_PWM0 0x2
#define LS_FUNC_PWM1 0x3

/* Blink period limits derived from PSC range 0 - 255 (6.3.2/6.3.4) */
#define BLINK_MIN_MS 7U    /* (0+1)/152 = 6.58 ms  - ceil   */
#define BLINK_MAX_MS 1685U /* (255+1)/152 = 1.684 s - ceil  */

/* Default PWM frequency when using set_brightness (152 Hz) */
#define PCA953X_DEFAULT_PSC 0x00

struct pca953x_config {
	struct i2c_dt_spec i2c;
	uint8_t nleds;
	uint8_t ninput_reg;
};

struct pca953x_data {
	/* run-time bookkeeping for the two PWM engines */
	uint8_t pwm_val[PCA953X_ENGINES];      /* duty (0-255) programmed into PWMx         */
	uint8_t psc_val[PCA953X_ENGINES];      /* prescaler programmed into PSCx            */
	uint8_t engine_users[PCA953X_ENGINES]; /* bitmask of LEDs using engine 0 / 1        */
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
 * @param config Device config
 * @param led LED index (0-nleds)
 * @param func Desired function (LS_FUNC_*)
 * @return int 0 on success, negative errno on error
 */
static int ls_update(const struct pca953x_config *config, uint8_t led, uint8_t func)
{
	return i2c_reg_update_byte_dt(&config->i2c, PCA953X_LS(config, led), LS_MASK(led),
				      func << LS_SHIFT(led));
}

/**
 * Return engine index (0 - PCA953X_ENGINES-1) that currently drives @p led,
 * or 0xFF if the LED is not routed to any engine (OFF / ON state).
 */
static uint8_t find_engine_for_led(const struct pca953x_data *data, uint8_t led)
{
	for (uint8_t ch = 0; ch < PCA953X_ENGINES; ch++) {
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
static int engine_find_match(const struct pca953x_data *data, uint8_t duty, uint8_t psc,
			     uint8_t *out_ch)
{
	for (uint8_t ch = 0; ch < PCA953X_ENGINES; ch++) {
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
 * @param dev Device structure
 * @param duty Desired duty cycle (0-255)
 * @param psc Desired prescaler value
 * @param[out] out_ch Acquired engine ID (0 or 1)
 * @return int 0 on success, -EBUSY if no engine available
 */
static int engine_acquire(const struct device *dev, uint8_t duty, uint8_t psc, uint8_t *out_ch)
{
	const struct pca953x_config *config = dev->config;
	struct pca953x_data *data = dev->data;
	uint8_t ch;

	/* Check for existing engine with matching parameters */
	for (ch = 0; ch < PCA953X_ENGINES; ch++) {
		if (data->engine_users[ch] && data->pwm_val[ch] == duty &&
		    data->psc_val[ch] == psc) {
			*out_ch = ch;
			return 0;
		}
	}
	/* Find free engine */
	for (ch = 0; ch < PCA953X_ENGINES; ch++) {
		if (data->engine_users[ch] == 0) {
			*out_ch = ch;
			break;
		}
	}

	if (ch == PCA953X_ENGINES) {
		return -EBUSY;
	}

	/* Found a free engine, program it */
	int ret = i2c_reg_write_byte_dt(&config->i2c, PCA953X_PSC(config, ch), psc);

	if (ret == 0) {
		data->psc_val[ch] = psc;
		ret = i2c_reg_write_byte_dt(&config->i2c, PCA953X_PWM(config, ch), duty);
	}
	if (ret) {
		LOG_ERR("Failed to program engine %u: %d", ch, ret);
		return ret;
	}
	data->pwm_val[ch] = duty;

	return 0;
}

/**
 * @brief Bind LED to a PWM engine
 *
 * @param data Driver data
 * @param led LED index (0-nleds)
 * @param ch Engine ID (0 or 1)
 */
static void engine_bind(struct pca953x_data *data, uint8_t led, uint8_t ch)
{
	data->engine_users[ch] |= BIT(led);
}

/**
 * @brief Release LED from its current PWM engine
 *
 * @param data Driver data
 * @param led LED index (0-nleds)
 */
static void engine_release(struct pca953x_data *data, uint8_t led)
{
	uint8_t ch = find_engine_for_led(data, led);

	if (ch < PCA953X_ENGINES) {
		data->engine_users[ch] &= ~BIT(led);
	}
}

/**
 * @brief Change the LEDs PWM settings in place
 *
 * ...or piggy-back on another PWM channel if its settings fit.
 * The LED must be already in a PWM mode (not ON or OFF) and there is
 * no further check if other LEDs are on the modified PWM.
 *
 * @param dev Device structure
 * @param led LED index
 * @param cur Current PWM engine assigned to LED (0-1)
 * @param duty Desired duty cycle
 * @param psc Desired prescaler value
 * @return 0 on success
 */
static int led_change_pwm_settings(const struct device *dev, const uint8_t led, const uint8_t cur,
				   const uint8_t duty, const uint8_t psc)
{
	const struct pca953x_config *config = dev->config;
	struct pca953x_data *data = dev->data;

	uint8_t match;

	/* Can we piggy-back on an existing engine already at (duty, psc)? */
	if (!engine_find_match(data, duty, psc, &match) && match != cur) {
		LOG_DBG("LED%u moves from engine %u to matching engine %u", led, cur, match);
		engine_release(data, led);
		engine_bind(data, led, match);
		return ls_update(config, led, match ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
	}

	/* Otherwise retune in place */
	int ret;

	if (data->psc_val[cur] != psc) {
		ret = i2c_reg_write_byte_dt(&config->i2c, PCA953X_PSC(config, cur), psc);
		if (ret) {
			return ret;
		}
		data->psc_val[cur] = psc;
	}
	if (data->pwm_val[cur] != duty) {
		ret = i2c_reg_write_byte_dt(&config->i2c, PCA953X_PWM(config, cur), duty);
		if (ret) {
			return ret;
		}
		data->pwm_val[cur] = duty;
	}

	return 0;
}

static int pca953x_led_set_brightness(const struct device *dev, uint32_t led, uint8_t percent)
{
	const struct pca953x_config *config = dev->config;
	struct pca953x_data *data = dev->data;
	uint8_t ch;
	int ret;

	if (led >= config->nleds) {
		LOG_ERR("Invalid LED index: %u", led);
		return -EINVAL;
	}

	if (percent == 0) {
		LOG_DBG("LED%u -> OFF", led);
		ret = ls_update(config, led, LS_FUNC_OFF);
		if (ret == 0) {
			engine_release(data, led);
		}
		return ret;
	}
	if (percent == LED_BRIGHTNESS_MAX) {
		LOG_DBG("LED%u -> ON", led);
		ret = ls_update(config, led, LS_FUNC_ON);
		if (ret == 0) {
			engine_release(data, led);
		}
		return ret;
	}

	const uint8_t duty = (percent * UINT8_MAX) / LED_BRIGHTNESS_MAX;
	const uint8_t cur = find_engine_for_led(data, led);

	/* Sole-user fast-path, with reuse-check */
	if (cur < PCA953X_ENGINES && data->engine_users[cur] == BIT(led)) {
		return led_change_pwm_settings(dev, led, cur, duty, PCA953X_DEFAULT_PSC);
	}

	/* Acquire new engine - use default PSC for brightness control */
	ret = engine_acquire(dev, duty, PCA953X_DEFAULT_PSC, &ch);
	if (ret) {
		LOG_WRN("No PWM engine available for LED %u", led);
		return ret;
	}

	/* Bind LED to new engine and update hardware */
	LOG_DBG("LED%u uses engine %u (duty %u)", led, ch, duty);
	engine_release(data, led);
	engine_bind(data, led, ch);
	return ls_update(config, led, ch ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
}

static int pca953x_led_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	const struct pca953x_config *config = dev->config;
	struct pca953x_data *data = dev->data;
	uint8_t ch;

	if (led >= config->nleds) {
		LOG_ERR("Invalid LED index: %u", led);
		return -EINVAL;
	}

	const uint32_t period = delay_on + delay_off;

	if (period < BLINK_MIN_MS || period > BLINK_MAX_MS) {
		LOG_ERR("Invalid blink period: %u ms (min: %u, max: %u)", period, BLINK_MIN_MS,
			BLINK_MAX_MS);
		return -ENOTSUP;
	}

	/* Calculate duty cycle with overflow protection */
	const uint32_t duty32 = (delay_on * 256U) / period;
	const uint8_t duty = CLAMP(duty32, 0, UINT8_MAX);
	const uint8_t psc = ms_to_psc(period);
	const uint8_t cur = find_engine_for_led(data, led);

	/* Sole-user fast-path with reuse-check */
	if (cur < PCA953X_ENGINES && data->engine_users[cur] == BIT(led)) {
		return led_change_pwm_settings(dev, led, cur, duty, psc);
	}

	/* Acquire new engine with desired parameters */
	const int ret = engine_acquire(dev, duty, psc, &ch);

	if (ret) {
		LOG_WRN("No PWM engine available for LED %u blink", led);
		return ret;
	}

	LOG_DBG("LED%u now on engine %u (psc %u duty %u)", led, ch, psc, duty);
	engine_release(data, led);
	engine_bind(data, led, ch);
	return ls_update(config, led, ch ? LS_FUNC_PWM1 : LS_FUNC_PWM0);
}

static int pca953x_led_init_chip(const struct device *dev)
{
	struct pca953x_data *data = dev->data;

	for (uint8_t i = 0; i < PCA953X_ENGINES; i++) {
		data->engine_users[i] = 0;
	}

	/* The Power-On Reset already initializes the registers to their default state
	 * no need to write them here. We'll just reset bookkeeping
	 */

	return 0;
}

static int pca953x_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		return pca953x_led_init_chip(dev);
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int pca953x_led_init(const struct device *dev)
{
	const struct pca953x_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	return pm_device_driver_init(dev, pca953x_pm_action);
}

static DEVICE_API(led, pca953x_led_api) = {
	.blink = pca953x_led_blink,
	.set_brightness = pca953x_led_set_brightness,
};

#define PCA953X_DEVICE(id, dev_name, led_channels)                                                 \
	static const struct pca953x_config dev_name##_##id##_cfg = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(id),                                                   \
		.nleds = led_channels,                                                             \
		.ninput_reg = (led_channels - 1) / 8 + 1};                                         \
	static struct pca953x_data dev_name##_##id##_data;                                         \
	PM_DEVICE_DT_INST_DEFINE(id, pca953x_pm_action);                                           \
	DEVICE_DT_INST_DEFINE(id, &pca953x_led_init, PM_DEVICE_DT_INST_GET(id),                    \
			      &dev_name##_##id##_data, &dev_name##_##id##_cfg, POST_KERNEL,        \
			      CONFIG_LED_INIT_PRIORITY, &pca953x_led_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9530
DT_INST_FOREACH_STATUS_OKAY_VARGS(PCA953X_DEVICE, pca9530, 2)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9531
DT_INST_FOREACH_STATUS_OKAY_VARGS(PCA953X_DEVICE, pca9531, 8)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9532
DT_INST_FOREACH_STATUS_OKAY_VARGS(PCA953X_DEVICE, pca9532, 16)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9533
DT_INST_FOREACH_STATUS_OKAY_VARGS(PCA953X_DEVICE, pca9533, 4)
