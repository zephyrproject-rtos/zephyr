/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_buzzer

#include <zephyr/drivers/buzzer.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buzzer_gpio, CONFIG_BUZZER_LOG_LEVEL);

struct buzzer_gpio_config {
	struct gpio_dt_spec gpio;
};

struct buzzer_gpio_data {
	const struct device *dev;
	struct k_work_delayable stop_work;
	struct k_mutex lock;
	uint8_t volume_percent;
};

static void buzzer_gpio_stop_work(struct k_work *work)
{
	struct k_work_delayable *dw = k_work_delayable_from_work(work);
	struct buzzer_gpio_data *data =
		CONTAINER_OF(dw, struct buzzer_gpio_data, stop_work);
	const struct buzzer_gpio_config *cfg = data->dev->config;
	int ret;

	ret = gpio_pin_set_dt(&cfg->gpio, 0);
	if (ret < 0) {
		LOG_ERR("Failed to silence buzzer (%d)", ret);
	}
}

static void buzzer_gpio_cancel_stop(struct buzzer_gpio_data *data)
{
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&data->stop_work, &sync);
}

static int buzzer_gpio_tone(const struct device *dev, uint32_t freq_hz,
			    uint32_t duration_ms)
{
	const struct buzzer_gpio_config *cfg = dev->config;
	struct buzzer_gpio_data *data = dev->data;
	int ret;
	bool active;

	k_mutex_lock(&data->lock, K_FOREVER);
	buzzer_gpio_cancel_stop(data);
	active = (freq_hz != BUZZER_FREQ_REST) && (data->volume_percent != 0U);

	/*
	 * Active buzzers have no programmatic frequency control. Any
	 * non-zero request turns them on at their built-in resonance;
	 * zero is silence.
	 */
	ret = gpio_pin_set_dt(&cfg->gpio, active ? 1 : 0);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (active && duration_ms != BUZZER_DURATION_FOREVER) {
		ret = k_work_schedule(&data->stop_work, K_MSEC(duration_ms));
		if (ret < 0) {
			int silence_ret = gpio_pin_set_dt(&cfg->gpio, 0);

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

static int buzzer_gpio_set_volume(const struct device *dev, uint8_t percent)
{
	const struct buzzer_gpio_config *cfg = dev->config;
	struct buzzer_gpio_data *data = dev->data;
	int ret;

	if (percent > BUZZER_VOLUME_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (percent == 0U) {
		data->volume_percent = 0U;
		buzzer_gpio_cancel_stop(data);
		ret = gpio_pin_set_dt(&cfg->gpio, 0);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/*
	 * Active buzzers cannot do intermediate volumes; treat any
	 * non-zero request as the single audible level the hardware
	 * supports.
	 */
	data->volume_percent = BUZZER_VOLUME_MAX;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int buzzer_gpio_beep(const struct device *dev, uint32_t duration_ms)
{
	/*
	 * Active buzzers run at a hardware-fixed frequency, so "beep" is
	 * the same as "tone with any non-zero frequency": just turn the
	 * buzzer on for the requested duration.
	 */
	return buzzer_gpio_tone(dev, 1U, duration_ms);
}

static int buzzer_gpio_stop(const struct device *dev)
{
	const struct buzzer_gpio_config *cfg = dev->config;
	struct buzzer_gpio_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	buzzer_gpio_cancel_stop(data);
	ret = gpio_pin_set_dt(&cfg->gpio, 0);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int buzzer_gpio_init(const struct device *dev)
{
	const struct buzzer_gpio_config *cfg = dev->config;
	struct buzzer_gpio_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&cfg->gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;
	data->volume_percent = BUZZER_VOLUME_MAX;
	k_mutex_init(&data->lock);
	k_work_init_delayable(&data->stop_work, buzzer_gpio_stop_work);

	return 0;
}

static DEVICE_API(buzzer, buzzer_gpio_api) = {
	.tone = buzzer_gpio_tone,
	.set_volume = buzzer_gpio_set_volume,
	.beep = buzzer_gpio_beep,
	.stop = buzzer_gpio_stop,
};

#define BUZZER_GPIO_INIT(inst)								\
	static const struct buzzer_gpio_config buzzer_gpio_cfg_##inst = {		\
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),				\
	};										\
	static struct buzzer_gpio_data buzzer_gpio_data_##inst;				\
	DEVICE_DT_INST_DEFINE(inst, buzzer_gpio_init, NULL,				\
			      &buzzer_gpio_data_##inst, &buzzer_gpio_cfg_##inst,	\
			      POST_KERNEL, CONFIG_BUZZER_INIT_PRIORITY,			\
			      &buzzer_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(BUZZER_GPIO_INIT)
