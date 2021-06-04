/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pwm_leds

/**
 * @file
 * @brief PWM driven LEDs
 */

#include <drivers/led.h>
#include <drivers/pwm.h>
#include <device.h>
#include <zephyr.h>
#include <sys/math_extras.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(led_pwm, CONFIG_LED_LOG_LEVEL);

#define DEV_CFG(dev)	((const struct led_pwm_config *) ((dev)->config))
#define DEV_DATA(dev)	((struct led_pwm_data *) ((dev)->data))

struct led_pwm {
	const struct device *dev;
	uint32_t channel;
	uint32_t period;
	pwm_flags_t flags;
};

struct led_pwm_config {
	int num_leds;
	const struct led_pwm *led;
};

struct led_pwm_data {
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

static int led_pwm_blink(const struct device *dev, uint32_t led,
			 uint32_t delay_on, uint32_t delay_off)
{
	const struct led_pwm_config *config = DEV_CFG(dev);
	const struct led_pwm *led_pwm;
	uint32_t period_usec, pulse_usec;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	/*
	 * Convert delays (in ms) into PWM period and pulse (in us) and check
	 * for overflows.
	 */
	if (u32_add_overflow(delay_on, delay_off, &period_usec) ||
	    u32_mul_overflow(period_usec, 1000, &period_usec) ||
	    u32_mul_overflow(delay_on, 1000, &pulse_usec)) {
		return -EINVAL;
	}

	led_pwm = &config->led[led];

	return pwm_pin_set_usec(led_pwm->dev, led_pwm->channel,
				period_usec, pulse_usec, led_pwm->flags);
}

static int led_pwm_set_brightness(const struct device *dev,
				  uint32_t led, uint8_t value)
{
	const struct led_pwm_config *config = DEV_CFG(dev);
	const struct led_pwm *led_pwm;
	uint32_t pulse;

	if (led >= config->num_leds || value > 100) {
		return -EINVAL;
	}

	led_pwm = &config->led[led];

	pulse = led_pwm->period * value / 100;

	return pwm_pin_set_cycles(led_pwm->dev, led_pwm->channel,
				  led_pwm->period, pulse, led_pwm->flags);
}

static int led_pwm_on(const struct device *dev, uint32_t led)
{
	return led_pwm_set_brightness(dev, led, 100);
}

static int led_pwm_off(const struct device *dev, uint32_t led)
{
	return led_pwm_set_brightness(dev, led, 0);
}

static int led_pwm_init(const struct device *dev)
{
	const struct led_pwm_config *config = DEV_CFG(dev);
	int i;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)",
			dev->name);
		return -ENODEV;
	}

	for (i = 0; i < config->num_leds; i++) {
		const struct led_pwm *led = &config->led[i];

		if (!device_is_ready(led->dev)) {
			LOG_ERR("%s: pwm device not ready", dev->name);
			return -ENODEV;
		}
	}

#ifdef CONFIG_PM_DEVICE
	struct led_pwm_data *data = DEV_DATA(dev);

	data->pm_state = PM_DEVICE_STATE_ACTIVE;
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int led_pwm_pm_get_state(const struct device *dev,
				enum pm_device_state *state)
{
	struct led_pwm_data *data = DEV_DATA(dev);

	unsigned int key = irq_lock();
	*state = data->pm_state;
	irq_unlock(key);

	return 0;
}

static int led_pwm_pm_set_state(const struct device *dev,
				enum pm_device_state new_state)
{
	const struct led_pwm_config *config = DEV_CFG(dev);
	struct led_pwm_data *data = DEV_DATA(dev);
	enum pm_device_state old_state;
	unsigned int key;

	key = irq_lock();
	old_state = data->pm_state;
	irq_unlock(key);

	if (old_state == new_state) {
		/* leave unchanged */
		return 0;
	}

	/* switch all underlying PWM devices to the new state */
	for (size_t i = 0; i < config->num_leds; i++) {
		const struct led_pwm *led_pwm = &config->led[i];

		LOG_DBG("Switching PWM %p to state %" PRIu32, led_pwm->dev, new_state);
		int err = pm_device_state_set(led_pwm->dev, new_state);

		if (err) {
			LOG_ERR("Cannot switch PWM %p power state", led_pwm->dev);
		}
	}

	/* record the new state */
	key = irq_lock();
	data->pm_state = new_state;
	irq_unlock(key);

	return 0;
}

static int led_pwm_pm_control(const struct device *dev, uint32_t ctrl_command,
			      enum pm_device_state *state)
{
	int err;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_GET:
		err = led_pwm_pm_get_state(dev, state);
		break;

	case PM_DEVICE_STATE_SET:
		err = led_pwm_pm_set_state(dev, *state);
		break;

	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

#endif /* CONFIG_PM_DEVICE */

static const struct led_driver_api led_pwm_api = {
	.on		= led_pwm_on,
	.off		= led_pwm_off,
	.blink		= led_pwm_blink,
	.set_brightness	= led_pwm_set_brightness,
};

#define LED_PWM(led_node_id)						\
{									\
	.dev		= DEVICE_DT_GET(DT_PWMS_CTLR(led_node_id)),	\
	.channel	= DT_PWMS_CHANNEL(led_node_id),			\
	.period		= DT_PHA_OR(led_node_id, pwms, period, 100),	\
	.flags		= DT_PHA_OR(led_node_id, pwms, flags,		\
				    PWM_POLARITY_NORMAL),		\
},

#define LED_PWM_DEVICE(id)					\
								\
static const struct led_pwm led_pwm_##id[] = {			\
	DT_INST_FOREACH_CHILD(id, LED_PWM)			\
};								\
								\
static const struct led_pwm_config led_pwm_config_##id = {	\
	.num_leds	= ARRAY_SIZE(led_pwm_##id),		\
	.led		= led_pwm_##id,				\
};								\
								\
static struct led_pwm_data led_pwm_data_##id;			\
								\
DEVICE_DT_INST_DEFINE(id, &led_pwm_init, led_pwm_pm_control,	\
		      &led_pwm_data_##id, &led_pwm_config_##id,	\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(LED_PWM_DEVICE)
