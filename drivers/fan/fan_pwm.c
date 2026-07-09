/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fan_pwm

#include <zephyr/drivers/fan.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_FAN_PWM_TACHOMETER
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>
#endif

LOG_MODULE_REGISTER(fan_pwm, CONFIG_FAN_LOG_LEVEL);

/* Interval between tachometer samples when polling, in microseconds. */
#define FAN_PWM_TACH_POLL_INTERVAL_US 200U

struct fan_pwm_config {
	struct pwm_dt_spec pwm;
#ifdef CONFIG_FAN_PWM_TACHOMETER
	struct gpio_dt_spec tach;
	uint32_t pulses_per_rev;
#endif
};

struct fan_pwm_data {
	struct k_mutex lock;
	uint8_t speed_percent;
#ifdef CONFIG_FAN_PWM_TACHOMETER_INTERRUPT
	struct gpio_callback tach_cb;
	atomic_t pulse_count;
#endif
};

static int fan_pwm_set_speed(const struct device *dev, uint8_t percent)
{
	const struct fan_pwm_config *cfg = dev->config;
	struct fan_pwm_data *data = dev->data;
	uint32_t pulse_ns;
	int ret;

	if (percent > FAN_SPEED_MAX) {
		return -EINVAL;
	}

	pulse_ns = (uint32_t)((uint64_t)cfg->pwm.period * percent / FAN_SPEED_MAX);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = pwm_set_dt(&cfg->pwm, cfg->pwm.period, pulse_ns);
	if (ret == 0) {
		data->speed_percent = percent;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int fan_pwm_get_speed(const struct device *dev, uint8_t *percent)
{
	struct fan_pwm_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	*percent = data->speed_percent;
	k_mutex_unlock(&data->lock);

	return 0;
}

#ifdef CONFIG_FAN_PWM_TACHOMETER

static uint32_t fan_pwm_pulses_to_rpm(const struct fan_pwm_config *cfg, uint32_t pulses)
{
	uint64_t numerator = (uint64_t)pulses * 60U * MSEC_PER_SEC;
	uint64_t denominator = (uint64_t)CONFIG_FAN_PWM_TACHOMETER_MEASURE_MS *
			       cfg->pulses_per_rev;

	return (uint32_t)(numerator / denominator);
}

#ifdef CONFIG_FAN_PWM_TACHOMETER_INTERRUPT

static void fan_pwm_tach_handler(const struct device *port, struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{
	struct fan_pwm_data *data = CONTAINER_OF(cb, struct fan_pwm_data, tach_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	atomic_inc(&data->pulse_count);
}

static int fan_pwm_get_rpm(const struct device *dev, uint32_t *rpm)
{
	const struct fan_pwm_config *cfg = dev->config;
	struct fan_pwm_data *data = dev->data;
	uint32_t pulses;

	if (cfg->tach.port == NULL) {
		return -ENOSYS;
	}

	atomic_set(&data->pulse_count, 0);
	k_sleep(K_MSEC(CONFIG_FAN_PWM_TACHOMETER_MEASURE_MS));
	pulses = (uint32_t)atomic_get(&data->pulse_count);

	*rpm = fan_pwm_pulses_to_rpm(cfg, pulses);

	return 0;
}

#else /* CONFIG_FAN_PWM_TACHOMETER_POLL */

static int fan_pwm_get_rpm(const struct device *dev, uint32_t *rpm)
{
	const struct fan_pwm_config *cfg = dev->config;
	uint32_t pulses = 0U;
	int64_t end;
	int prev;

	if (cfg->tach.port == NULL) {
		return -ENOSYS;
	}

	prev = gpio_pin_get_dt(&cfg->tach);
	if (prev < 0) {
		return prev;
	}

	end = k_uptime_get() + CONFIG_FAN_PWM_TACHOMETER_MEASURE_MS;
	while (k_uptime_get() < end) {
		int cur = gpio_pin_get_dt(&cfg->tach);

		if (cur < 0) {
			return cur;
		}

		if ((prev == 0) && (cur == 1)) {
			pulses++;
		}

		prev = cur;
		k_busy_wait(FAN_PWM_TACH_POLL_INTERVAL_US);
	}

	*rpm = fan_pwm_pulses_to_rpm(cfg, pulses);

	return 0;
}

#endif /* CONFIG_FAN_PWM_TACHOMETER_INTERRUPT */

static int fan_pwm_tach_init(const struct device *dev)
{
	const struct fan_pwm_config *cfg = dev->config;
	int ret;

	if (cfg->tach.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->tach)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->tach.port);
		return -ENODEV;
	}

	if (cfg->pulses_per_rev == 0U) {
		LOG_ERR("pulses-per-revolution must be non-zero");
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->tach, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure tachometer GPIO (%d)", ret);
		return ret;
	}

#ifdef CONFIG_FAN_PWM_TACHOMETER_INTERRUPT
	struct fan_pwm_data *data = dev->data;

	gpio_init_callback(&data->tach_cb, fan_pwm_tach_handler, BIT(cfg->tach.pin));

	ret = gpio_add_callback_dt(&cfg->tach, &data->tach_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add tachometer callback (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->tach, GPIO_INT_EDGE_RISING);
	if (ret < 0) {
		LOG_ERR("Failed to configure tachometer interrupt (%d)", ret);
		return ret;
	}
#endif /* CONFIG_FAN_PWM_TACHOMETER_INTERRUPT */

	return 0;
}

#endif /* CONFIG_FAN_PWM_TACHOMETER */

static int fan_pwm_init(const struct device *dev)
{
	const struct fan_pwm_config *cfg = dev->config;
	struct fan_pwm_data *data = dev->data;

	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->pwm.dev);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	data->speed_percent = CONFIG_FAN_PWM_INITIAL_DUTY_CYCLE;

#ifdef CONFIG_FAN_PWM_TACHOMETER
	int ret = fan_pwm_tach_init(dev);

	if (ret < 0) {
		return ret;
	}
#endif

	return fan_pwm_set_speed(dev, CONFIG_FAN_PWM_INITIAL_DUTY_CYCLE);
}

static DEVICE_API(fan, fan_pwm_api) = {
	.set_speed = fan_pwm_set_speed,
	.get_speed = fan_pwm_get_speed,
#ifdef CONFIG_FAN_PWM_TACHOMETER
	.get_rpm = fan_pwm_get_rpm,
#endif
};

#define FAN_PWM_TACH_INIT(inst)								\
	.tach = GPIO_DT_SPEC_INST_GET_OR(inst, tach_gpios, {0}),				\
	.pulses_per_rev = DT_INST_PROP(inst, pulses_per_revolution),

#define FAN_PWM_INIT(inst)								\
	static const struct fan_pwm_config fan_pwm_cfg_##inst = {			\
		.pwm = PWM_DT_SPEC_INST_GET(inst),					\
		IF_ENABLED(CONFIG_FAN_PWM_TACHOMETER, (FAN_PWM_TACH_INIT(inst)))		\
	};										\
	static struct fan_pwm_data fan_pwm_data_##inst;					\
	DEVICE_DT_INST_DEFINE(inst, fan_pwm_init, NULL,					\
			      &fan_pwm_data_##inst, &fan_pwm_cfg_##inst,		\
			      POST_KERNEL, CONFIG_FAN_INIT_PRIORITY,			\
			      &fan_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(FAN_PWM_INIT)
