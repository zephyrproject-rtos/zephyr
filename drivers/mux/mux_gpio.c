/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO-based MUX driver. The state value is encoded as a binary pattern
 * across the mux-gpios lines, LSB first (bit N -> mux-gpios[N]).
 *
 * Single-channel by design: one controller node drives exactly one
 * select bus, so there is no addressing. The binding pins
 * #mux-control-cells = const: 0 and #mux-state-cells = const: 1 to
 * reflect this; mux-controls reference is just <&mux0> and mux-states
 * reference is <&mux0 state>.
 */

#define DT_DRV_COMPAT gpio_mux

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mux_gpio, CONFIG_MUX_LOG_LEVEL);

struct mux_gpio_config {
	const struct gpio_dt_spec *gpios;
	const struct gpio_dt_spec *en_gpios;
	uint8_t num_gpios;
	uint8_t num_en_gpios;
};

struct mux_gpio_data {
	uint32_t state;
	bool connected;
};

static int mux_gpio_set(const struct device *dev,
			const struct mux_control *control,
			uint32_t state)
{
	const struct mux_gpio_config *cfg = dev->config;
	struct mux_gpio_data *data = dev->data;

	ARG_UNUSED(control);

	if (state >= (1U << cfg->num_gpios)) {
		LOG_ERR("state 0x%x exceeds %u-bit GPIO bus", state, cfg->num_gpios);
		return -EINVAL;
	}

	/* Drive the select lines to the requested route first. */
	for (uint8_t i = 0; i < cfg->num_gpios; i++) {
		uint32_t bit_val = (state >> i) & 0x1U;
		int ret = gpio_pin_set_dt(&cfg->gpios[i], bit_val);

		if (ret < 0) {
			LOG_ERR("gpio_pin_set_dt(%u) failed: %d", i, ret);
			return ret;
		}

		WRITE_BIT(data->state, i, bit_val);
	}

	/* Only after the route is in place, assert the enable lines (connect).
	 * This avoids momentarily connecting to a stale route when reconnecting
	 * after a disconnect. The loop is empty when there are no enable lines.
	 */
	for (uint8_t i = 0; i < cfg->num_en_gpios; i++) {
		int ret = gpio_pin_set_dt(&cfg->en_gpios[i], 1);

		if (ret < 0) {
			LOG_ERR("enable gpio_pin_set_dt(%u) failed: %d", i, ret);
			return ret;
		}
	}

	if (cfg->num_en_gpios > 0) {
		data->connected = true;
	}

	return 0;
}

static int mux_gpio_get_state(const struct device *dev,
			      const struct mux_control *control,
			      uint32_t *state)
{
	struct mux_gpio_data *data = dev->data;

	ARG_UNUSED(control);

	*state = data->state;

	return 0;
}

static int mux_gpio_disconnect(const struct device *dev,
			       const struct mux_control *control)
{
	const struct mux_gpio_config *cfg = dev->config;
	struct mux_gpio_data *data = dev->data;

	ARG_UNUSED(control);

	if (cfg->num_en_gpios == 0) {
		return -ENOSYS;
	}

	/* De-assert the enable lines to physically open the mux. The select
	 * lines (and data->state) are preserved so the next set() reconnects.
	 */
	for (uint8_t i = 0; i < cfg->num_en_gpios; i++) {
		int ret = gpio_pin_set_dt(&cfg->en_gpios[i], 0);

		if (ret < 0) {
			LOG_ERR("disable gpio_pin_set_dt(%u) failed: %d", i, ret);
			return ret;
		}
	}

	data->connected = false;

	return 0;
}

static DEVICE_API(mux_control, mux_gpio_driver_api) = {
	.set = mux_gpio_set,
	.get_state = mux_gpio_get_state,
	.disconnect = mux_gpio_disconnect,
};

static int mux_gpio_init(const struct device *dev)
{
	const struct mux_gpio_config *cfg = dev->config;

	for (uint8_t i = 0; i < cfg->num_gpios; i++) {
		const struct gpio_dt_spec *gpio = &cfg->gpios[i];
		int ret;

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("gpio[%u] %s not ready", i, gpio->port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("gpio_pin_configure_dt(%u) failed: %d", i, ret);
			return ret;
		}
	}

	/* Enable lines start de-asserted: the mux powers up disconnected and
	 * the first mux_control_set() connects it.
	 */
	for (uint8_t i = 0; i < cfg->num_en_gpios; i++) {
		const struct gpio_dt_spec *gpio = &cfg->en_gpios[i];
		int ret;

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("enable gpio[%u] %s not ready", i, gpio->port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("enable gpio_pin_configure_dt(%u) failed: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

#define MUX_GPIO_INIT(n)                                                                \
	static const struct gpio_dt_spec mux_gpio_gpios_##n[] = {                       \
		DT_INST_FOREACH_PROP_ELEM_SEP(n, mux_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))\
	};                                                                              \
	BUILD_ASSERT(ARRAY_SIZE(mux_gpio_gpios_##n) > 0,                                \
		     "gpio-mux node " #n " must declare at least one mux-gpio");        \
	BUILD_ASSERT(ARRAY_SIZE(mux_gpio_gpios_##n) <= 32,                              \
		     "gpio-mux node " #n " supports at most 32 mux-gpios");             \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, enable_gpios),                              \
		(static const struct gpio_dt_spec mux_gpio_en_gpios_##n[] = {           \
			DT_INST_FOREACH_PROP_ELEM_SEP(n, enable_gpios,                 \
						      GPIO_DT_SPEC_GET_BY_IDX, (,))       \
		};))                                                                    \
	static const struct mux_gpio_config mux_gpio_cfg_##n = {                        \
		.gpios = mux_gpio_gpios_##n,                                            \
		.num_gpios = ARRAY_SIZE(mux_gpio_gpios_##n),                            \
		.en_gpios = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, enable_gpios),         \
					(mux_gpio_en_gpios_##n), (NULL)),              \
		.num_en_gpios = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, enable_gpios),     \
					    (ARRAY_SIZE(mux_gpio_en_gpios_##n)), (0)),     \
	};                                                                              \
	static struct mux_gpio_data mux_gpio_data_##n;                                  \
	DEVICE_DT_INST_DEFINE(n, mux_gpio_init, NULL,                                   \
			      &mux_gpio_data_##n, &mux_gpio_cfg_##n,                    \
			      POST_KERNEL, CONFIG_MUX_INIT_PRIORITY,                    \
			      &mux_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_GPIO_INIT)
