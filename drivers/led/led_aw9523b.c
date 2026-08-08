/*
 * Copyright (c) 2026 Smartbox Assistive Technology Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw9523b_led

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/aw9523b.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define AW9523B_CTL_IMAX_MASK     0x03
#define AW9523B_RESET_PULSE_WIDTH 20

/* Bizarre mapping of dimming channels */
#define AW9523B_DIM_REG(port, pin)                                                                 \
	(port == 1 ? pin > 3 ? (AW9523B_REG_DIM8 + pin) : (AW9523B_REG_DIM0 + pin)                 \
		   : (AW9523B_REG_DIM4 + pin))

struct led_aw9523b_channel {
	uint8_t port;
	uint8_t pin;
	uint8_t dim_reg;
};

struct led_aw9523b_config {
	const struct device *mfd_dev;
	struct i2c_dt_spec i2c;
	const struct led_aw9523b_channel *channels;
	uint8_t num_leds;
	uint8_t i_max_idx;
};

static int aw9523b_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_aw9523b_config *const config = dev->config;
	const struct led_aw9523b_channel *channel;
	uint8_t pwm;
	int ret;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	channel = &config->channels[led];
	pwm = (uint8_t)(((uint16_t)value * UINT8_MAX) / LED_BRIGHTNESS_MAX);

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);
	ret = i2c_reg_write_byte_dt(&config->i2c, channel->dim_reg, pwm);

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return ret;
}

static int aw9523b_turn_on(const struct device *dev)
{
	const struct led_aw9523b_config *const config = dev->config;
	int ret;

	/* By default all channels are in GPIO mode. Set LED mode based on channel info */
	uint8_t mode[2] = {UINT8_MAX, UINT8_MAX};

	for (uint8_t i = 0U; i < config->num_leds; i++) {
		const struct led_aw9523b_channel *channel = &config->channels[i];

		mode[channel->port] &= (uint8_t)~BIT(channel->pin);
	}

	k_sem_take(aw9523b_get_lock(config->mfd_dev), K_FOREVER);

	/* Set Global Current Limit for LED Driver */
	ret = i2c_reg_update_byte_dt(&config->i2c, AW9523B_REG_CTL, AW9523B_CTL_IMAX_MASK,
				     config->i_max_idx);
	if (ret < 0) {
		k_sem_give(aw9523b_get_lock(config->mfd_dev));
		return ret;
	}

	/* Write LED Mode Channel configuration */
	ret = i2c_reg_write_byte_dt(&config->i2c, AW9523B_REG_MODE0, mode[0]);
	if (ret < 0) {
		k_sem_give(aw9523b_get_lock(config->mfd_dev));
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->i2c, AW9523B_REG_MODE1, mode[1]);

	k_sem_give(aw9523b_get_lock(config->mfd_dev));

	return ret;
}

static int aw9523b_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		return aw9523b_turn_on(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		/* No action needed on turn off */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		/* No action required for suspend - I_q is low */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int led_aw9523b_init(const struct device *dev)
{
	const struct led_aw9523b_config *const config = dev->config;

	if (!device_is_ready(config->mfd_dev) || !device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	if (config->num_leds == 0U) {
		return -ENODEV;
	}

	return pm_device_driver_init(dev, aw9523b_pm_action);
}

static DEVICE_API(led, led_aw9523b_api) = {
	.set_brightness = aw9523b_set_brightness,
};

#define LED_AW9523B_CHANNEL_ENTRY(node_id)                                                         \
	{                                                                                          \
		.port = DT_PROP(node_id, port),                                                    \
		.pin = DT_PROP(node_id, pin),                                                      \
		.dim_reg = AW9523B_DIM_REG(DT_PROP(node_id, port), DT_PROP(node_id, pin)),         \
	}

#define LED_AW9523B_DEFINE(inst)                                                                   \
	static const struct led_aw9523b_channel led_aw9523b_channels##inst[] = {                   \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, LED_AW9523B_CHANNEL_ENTRY, (,))};      \
                                                                                                   \
	static const struct led_aw9523b_config led_aw9523b_config##inst = {                        \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.channels = led_aw9523b_channels##inst,                                            \
		.num_leds = ARRAY_SIZE(led_aw9523b_channels##inst),                                \
		.i_max_idx = DT_INST_ENUM_IDX_OR(inst, i_max_microamp, 0),                         \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, aw9523b_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(inst, led_aw9523b_init, PM_DEVICE_DT_INST_GET(inst), NULL,           \
			      &led_aw9523b_config##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,    \
			      &led_aw9523b_api);

DT_INST_FOREACH_STATUS_OKAY(LED_AW9523B_DEFINE)
