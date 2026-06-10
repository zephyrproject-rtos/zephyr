/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pwm_buzzer

#include <zephyr/drivers/buzzer.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buzzer_pwm, CONFIG_BUZZER_LOG_LEVEL);

struct buzzer_pwm_config {
	struct pwm_dt_spec pwm;
};

struct buzzer_pwm_data {
	const struct device *dev;
	struct k_work_delayable stop_work;
	struct k_mutex lock;
	uint8_t volume_percent;
};

#define DEFAULT_VOLUME_PERCENT 50U

static int buzzer_pwm_silence(const struct device *dev)
{
	const struct buzzer_pwm_config *cfg = dev->config;

	if (cfg->pwm.period == 0U) {
		return -EINVAL;
	}

	return pwm_set_dt(&cfg->pwm, cfg->pwm.period, 0U);
}

static void buzzer_pwm_stop_work(struct k_work *work)
{
	struct k_work_delayable *dw = k_work_delayable_from_work(work);
	struct buzzer_pwm_data *data =
		CONTAINER_OF(dw, struct buzzer_pwm_data, stop_work);
	int ret;

	ret = buzzer_pwm_silence(data->dev);
	if (ret < 0) {
		LOG_ERR("Failed to silence buzzer (%d)", ret);
	}
}

static void buzzer_pwm_cancel_stop(struct buzzer_pwm_data *data)
{
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&data->stop_work, &sync);
}

static int buzzer_pwm_tone(const struct device *dev, uint32_t freq_hz,
			   uint32_t duration_ms)
{
	const struct buzzer_pwm_config *cfg = dev->config;
	struct buzzer_pwm_data *data = dev->data;
	uint32_t period_ns;
	uint32_t pulse_ns;
	bool active;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	buzzer_pwm_cancel_stop(data);
	active = (freq_hz != BUZZER_FREQ_REST) && (data->volume_percent != 0U);

	if (active) {
		period_ns = NSEC_PER_SEC / freq_hz;
		/*
		 * For a piezo, perceived volume peaks at 50% duty cycle
		 * and goes to zero at either 0% or 100%. Map the linear
		 * volume percentage to the rising half of the triangular
		 * duty curve so 100% volume gives the loudest output
		 * (50% duty).
		 */
		pulse_ns = (uint32_t)((uint64_t)period_ns *
				      data->volume_percent /
				      (2U * BUZZER_VOLUME_MAX));
		ret = pwm_set_dt(&cfg->pwm, period_ns, pulse_ns);
	} else {
		ret = buzzer_pwm_silence(dev);
	}

	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (active && duration_ms != BUZZER_DURATION_FOREVER) {
		ret = k_work_schedule(&data->stop_work, K_MSEC(duration_ms));
		if (ret < 0) {
			int silence_ret = buzzer_pwm_silence(dev);

			if (silence_ret < 0) {
				LOG_ERR("Failed to silence buzzer (%d)", silence_ret);
			}
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int buzzer_pwm_set_volume(const struct device *dev, uint8_t percent)
{
	struct buzzer_pwm_data *data = dev->data;
	int ret;

	if (percent > BUZZER_VOLUME_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->volume_percent = percent;

	if (percent == 0U) {
		buzzer_pwm_cancel_stop(data);
		ret = buzzer_pwm_silence(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int buzzer_pwm_beep(const struct device *dev, uint32_t duration_ms)
{
	const struct buzzer_pwm_config *cfg = dev->config;
	uint32_t freq_hz;

	if (cfg->pwm.period == 0U) {
		return -ENOTSUP;
	}

	freq_hz = (uint32_t)(NSEC_PER_SEC / cfg->pwm.period);
	return buzzer_pwm_tone(dev, freq_hz, duration_ms);
}

static int buzzer_pwm_stop(const struct device *dev)
{
	struct buzzer_pwm_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	buzzer_pwm_cancel_stop(data);
	ret = buzzer_pwm_silence(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int buzzer_pwm_init(const struct device *dev)
{
	const struct buzzer_pwm_config *cfg = dev->config;
	struct buzzer_pwm_data *data = dev->data;

	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->pwm.dev);
		return -ENODEV;
	}

	data->dev = dev;
	data->volume_percent = DEFAULT_VOLUME_PERCENT;
	k_mutex_init(&data->lock);
	k_work_init_delayable(&data->stop_work, buzzer_pwm_stop_work);

	return buzzer_pwm_silence(dev);
}

static DEVICE_API(buzzer, buzzer_pwm_api) = {
	.tone = buzzer_pwm_tone,
	.set_volume = buzzer_pwm_set_volume,
	.beep = buzzer_pwm_beep,
	.stop = buzzer_pwm_stop,
};

#define BUZZER_PWM_INIT(inst)								\
	static const struct buzzer_pwm_config buzzer_pwm_cfg_##inst = {			\
		.pwm = PWM_DT_SPEC_INST_GET(inst),					\
	};										\
	static struct buzzer_pwm_data buzzer_pwm_data_##inst;				\
	DEVICE_DT_INST_DEFINE(inst, buzzer_pwm_init, NULL,				\
			      &buzzer_pwm_data_##inst, &buzzer_pwm_cfg_##inst,		\
			      POST_KERNEL, CONFIG_BUZZER_INIT_PRIORITY,			\
			      &buzzer_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(BUZZER_PWM_INIT)
