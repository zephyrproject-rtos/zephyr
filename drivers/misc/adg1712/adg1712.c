/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/adg1712/adg1712.h>

LOG_MODULE_REGISTER(adg1712, CONFIG_ADG1712_LOG_LEVEL);

#define ADG1712_SWITCH_MSK GENMASK(3, 0)

struct adg1712_config {
	struct gpio_dt_spec gpios[4];
};

struct adg1712_data {
	struct k_mutex lock;
	uint8_t switch_states;
};

int adg1712_set_switch_state(const struct device *dev,
			     enum adg1712_switch sw,
			     enum adg1712_state state)
{
	const struct adg1712_config *cfg = dev->config;
	struct adg1712_data *data = dev->data;
	int ret;

	if (sw > ADG1712_SW4) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_set_dt(&cfg->gpios[sw],
			      state == ADG1712_ENABLE ? 1 : 0);
	if (ret) {
		goto unlock;
	}

	if (state == ADG1712_ENABLE) {
		data->switch_states |= BIT(sw);
	} else {
		data->switch_states &= ~BIT(sw);
	}

	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1712_get_switch_state(const struct device *dev,
			     enum adg1712_switch sw,
			     enum adg1712_state *state)
{
	const struct adg1712_config *cfg = dev->config;
	struct adg1712_data *data = dev->data;
	int ret;

	if (sw > ADG1712_SW4 || !state) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gpio_pin_get_dt(&cfg->gpios[sw]);
	if (ret < 0) {
		goto unlock;
	}

	*state = ret ? ADG1712_ENABLE : ADG1712_DISABLE;
	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1712_set_switches(const struct device *dev, uint8_t switch_mask)
{
	const struct adg1712_config *cfg = dev->config;
	struct adg1712_data *data = dev->data;
	int ret;

	switch_mask &= ADG1712_SWITCH_MSK;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < 4; i++) {
		ret = gpio_pin_set_dt(&cfg->gpios[i],
				      (switch_mask & BIT(i)) ? 1 : 0);
		if (ret) {
			goto unlock;
		}
	}

	data->switch_states = switch_mask;
	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1712_get_switches(const struct device *dev, uint8_t *switch_mask)
{
	const struct adg1712_config *cfg = dev->config;
	struct adg1712_data *data = dev->data;
	int ret;

	if (!switch_mask) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	*switch_mask = 0;
	for (int i = 0; i < 4; i++) {
		ret = gpio_pin_get_dt(&cfg->gpios[i]);
		if (ret < 0) {
			goto unlock;
		}

		if (ret) {
			*switch_mask |= BIT(i);
		}
	}

	data->switch_states = *switch_mask;
	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int adg1712_reset_switches(const struct device *dev)
{
	return adg1712_set_switches(dev, 0x00);
}

static int adg1712_init(const struct device *dev)
{
	const struct adg1712_config *cfg = dev->config;
	struct adg1712_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	data->switch_states = 0;

	for (int i = 0; i < 4; i++) {
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

	ret = adg1712_reset_switches(dev);
	if (ret) {
		LOG_ERR("Failed to reset switches: %d", ret);
		return ret;
	}

	LOG_INF("ADG1712 initialized");

	return 0;
}

#define ADG1712_DEFINE(inst)						\
	static struct adg1712_data adg1712_data_##inst;			\
	static const struct adg1712_config adg1712_config_##inst = {	\
		.gpios = {						\
			GPIO_DT_SPEC_INST_GET(inst, in1_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in2_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in3_gpios),		\
			GPIO_DT_SPEC_INST_GET(inst, in4_gpios),		\
		},							\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
			      adg1712_init,				\
			      NULL,					\
			      &adg1712_data_##inst,			\
			      &adg1712_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_ADG1712_INIT_PRIORITY,		\
			      NULL);

/* ADG1712 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adg1712
DT_INST_FOREACH_STATUS_OKAY(ADG1712_DEFINE)

/* ADG2712 (pin-compatible variant) */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT adi_adg2712
DT_INST_FOREACH_STATUS_OKAY(ADG1712_DEFINE)
