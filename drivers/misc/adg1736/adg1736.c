/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/adg1736/adg1736.h>

LOG_MODULE_REGISTER(adg1736, CONFIG_ADG1736_LOG_LEVEL);

struct adg1736_config {
	struct gpio_dt_spec gpios[2];
	struct gpio_dt_spec en_gpio;
	bool has_en;
};

struct adg1736_data {
	struct k_mutex lock;
};

int adg1736_set_switch_state(const struct device *dev,
			     enum adg1736_switch sw,
			     enum adg1736_state state)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	if (sw > ADG1736_SW2) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&cfg->gpios[sw],
			      state == ADG1736_CONNECT_A ? 1 : 0);
	if (ret) {
		LOG_ERR("Failed to set SW%d: %d", sw + 1, ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1736_get_switch_state(const struct device *dev,
			     enum adg1736_switch sw,
			     enum adg1736_state *state)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	if (sw > ADG1736_SW2 || !state) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_get_dt(&cfg->gpios[sw]);
	if (ret < 0) {
		LOG_ERR("Failed to get SW%d: %d", sw + 1, ret);
		goto unlock;
	}

	*state = ret ? ADG1736_CONNECT_A : ADG1736_CONNECT_B;
	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1736_enable(const struct device *dev)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	if (!cfg->has_en) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&cfg->en_gpio, 1);
	if (ret) {
		LOG_ERR("Failed to enable: %d", ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1736_disable(const struct device *dev)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	if (!cfg->has_en) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&cfg->en_gpio, 0);
	if (ret) {
		LOG_ERR("Failed to disable: %d", ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1736_reset_switches(const struct device *dev)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < 2; i++) {
		ret = gpio_pin_set_dt(&cfg->gpios[i], 0);
		if (ret) {
			LOG_ERR("Failed to reset SW%d: %d", i + 1, ret);
			goto unlock;
		}
	}

	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adg1736_init(const struct device *dev)
{
	const struct adg1736_config *cfg = dev->config;
	struct adg1736_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	for (int i = 0; i < 2; i++) {
		if (!gpio_is_ready_dt(&cfg->gpios[i])) {
			LOG_ERR("GPIO %d not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->gpios[i],
					    GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure GPIO %d: %d", i, ret);
			return ret;
		}
	}

	if (cfg->has_en) {
		if (!gpio_is_ready_dt(&cfg->en_gpio)) {
			LOG_ERR("EN GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->en_gpio,
					    GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure EN GPIO: %d", ret);
			return ret;
		}
	}

	ret = adg1736_reset_switches(dev);
	if (ret) {
		LOG_ERR("Failed to reset switches: %d", ret);
		return ret;
	}

	LOG_INF("ADG1736 initialized");

	return 0;
}

#define ADG1736_HAS_EN(inst) DT_INST_NODE_HAS_PROP(inst, en_gpios)

#define ADG1736_DEFINE(inst)						\
	static struct adg1736_data adg1736_data_##inst;			\
	static const struct adg1736_config adg1736_config_##inst = {	\
		.gpios = {						\
			GPIO_DT_SPEC_INST_GET(inst, in1_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in2_gpios),		\
		},							\
		.en_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),\
		.has_en = ADG1736_HAS_EN(inst),				\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      adg1736_init,				\
			      NULL,					\
			      &adg1736_data_##inst,			\
			      &adg1736_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_ADG1736_INIT_PRIORITY,		\
			      NULL);

/* ADG736 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adg736
DT_INST_FOREACH_STATUS_OKAY(ADG1736_DEFINE)

/* ADG1736 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adg1736
DT_INST_FOREACH_STATUS_OKAY(ADG1736_DEFINE)
