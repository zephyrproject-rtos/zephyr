/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_mux

#include <zephyr/drivers/mux.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(gpio_mux, CONFIG_MUX_CONTROL_LOG_LEVEL);

struct gpiomux_config {
	const struct gpio_dt_spec *specs; /* LSB to MSB */
	size_t num_bits; /* number of gpio specs */
};

struct gpiomux_data {
	bool allow_use; /* do not control the mux if the gpios failed to setup */
	uint32_t state;
};

static int gpiomux_init_gpio(const struct device *dev, const struct gpio_dt_spec *spec)
{
	int ret = 0;

	if (!gpio_is_ready_dt(spec)) {
		ret = -ENODEV;
		goto out;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		goto out;
	}

out:
	if (ret) {
		LOG_ERR("ERR %d Could not configure select line %s:%d", ret,
								spec->port->name, spec->pin);
	}
	return ret;
}

static int gpiomux_init_gpios(const struct device *dev)
{
	const struct gpiomux_config *config = dev->config;
	const struct gpio_dt_spec *spec = config->specs;
	int ret = 0;

	for (int i = 0; i < config->num_bits; i++) {
		ret = gpiomux_init_gpio(dev, spec);
		if (ret) {
			return ret;
		}
		spec++;
	};

	return 0;
}

static int gpiomux_configure(const struct device *dev, struct mux_control *mux, uint32_t state)
{
	const struct gpiomux_config *config = dev->config;
	const struct gpio_dt_spec *spec = config->specs;
	struct gpiomux_data *data = dev->data;
	int ret = 0;

	for (int i = 0; i < config->num_bits; i++) {
		uint16_t bit_val = IS_BIT_SET(state, i) ? 1 : 0;

		ret = gpio_pin_set_dt(spec, bit_val);
		if (ret) {
			goto out;
		}
		data->state &= ~BIT(i);
		data->state |= (bit_val == 1 ? BIT(i) : 0);

		spec++;
	}

out:
	if (ret) {
		LOG_ERR("Failed to configure %s", dev->name);
	}

	return ret;
}

static int gpiomux_state_get(const struct device *dev,
			     struct mux_control *control, uint32_t *state)
{
	struct gpiomux_data *data = dev->data;

	return data->state;
}

static int gpiomux_init(const struct device *dev) {
	struct gpiomux_data *data = dev->data;
	int ret = 0;

	ret = gpiomux_init_gpios(dev);
	data->allow_use = ret ? false : true; /* no implicit type conversion */

	return ret;
}

static DEVICE_API(mux_control, gpiomux_api) = {
	.state_get = gpiomux_state_get,
	.configure = gpiomux_configure,
};

#define GPIO_MUX_INIT(n)									\
	const struct gpio_dt_spec gpiomux##n##_specs[] = {					\
		DT_INST_FOREACH_PROP_ELEM_SEP(n, mux_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))	\
	};											\
	const struct gpiomux_config gpiomux_cfg_##n = {						\
		.num_bits = DT_INST_PROP_LEN(n, mux_gpios),					\
		.specs = gpiomux##n##_specs,							\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &gpiomux_init, NULL, NULL,					\
				&gpiomux_cfg_##n,						\
				PRE_KERNEL_2,							\
				CONFIG_MUX_CONTROL_INIT_PRIORITY,				\
				&gpiomux_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MUX_INIT);
