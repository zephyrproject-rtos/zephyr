/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lin_transceiver_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lin/transceiver.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lin_transceiver_gpio, CONFIG_LIN_LOG_LEVEL);

/* Does any devicetree instance have an enable-gpios property? */
#define ANY_INST_HAS_ENABLE_GPIOS DT_ANY_INST_HAS_PROP_STATUS_OKAY(enable_gpios)

/* Does any devicetree instance have a wakeup-gpios property? */
#define ANY_INST_HAS_WAKEUP_GPIOS DT_ANY_INST_HAS_PROP_STATUS_OKAY(wakeup_gpios)

struct lin_transceiver_gpio_config {
#if ANY_INST_HAS_ENABLE_GPIOS
	struct gpio_dt_spec enable_gpio;
#endif /* ANY_INST_HAS_ENABLE_GPIOS */
#if ANY_INST_HAS_WAKEUP_GPIOS
	struct gpio_dt_spec wakeup_gpio;
#endif /* ANY_INST_HAS_WAKEUP_GPIOS */
};

static int lin_transceiver_gpio_set_state(const struct device *dev, bool enabled)
{
	const struct lin_transceiver_gpio_config *config = dev->config;
	int err;

#if ANY_INST_HAS_ENABLE_GPIOS
	if (config->enable_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->enable_gpio, enabled ? 1 : 0);
		if (err != 0) {
			LOG_ERR("failed to set enable GPIO pin (err %d)", err);
			return -EIO;
		}
	}
#endif /* ANY_INST_HAS_ENABLE_GPIOS */

#if ANY_INST_HAS_WAKEUP_GPIOS
	if (config->wakeup_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->wakeup_gpio, enabled ? 1 : 0);
		if (err != 0) {
			LOG_ERR("failed to set wakeup GPIO pin (err %d)", err);
			return -EIO;
		}
	}
#endif /* ANY_INST_HAS_WAKEUP_GPIOS */

	return 0;
}

static int lin_transceiver_gpio_enable(const struct device *dev, uint8_t flags)
{
	ARG_UNUSED(flags);

	return lin_transceiver_gpio_set_state(dev, true);
}

static int lin_transceiver_gpio_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	return lin_transceiver_gpio_set_state(dev, false);
}

static int lin_transceiver_gpio_init(const struct device *dev)
{
	const struct lin_transceiver_gpio_config *config = dev->config;
	int err;

#if ANY_INST_HAS_ENABLE_GPIOS
	if (config->enable_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->enable_gpio)) {
			LOG_ERR("enable pin GPIO device not ready");
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure enable GPIO pin (err %d)", err);
			return err;
		}
	}
#endif /* ANY_INST_HAS_ENABLE_GPIOS */

#if ANY_INST_HAS_WAKEUP_GPIOS
	if (config->wakeup_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->wakeup_gpio)) {
			LOG_ERR("wakeup pin GPIO device not ready");
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->wakeup_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure wakeup GPIO pin (err %d)", err);
			return err;
		}
	}
#endif /* ANY_INST_HAS_WAKEUP_GPIOS */

	return 0;
}

static DEVICE_API(lin_transceiver, lin_transceiver_gpio_driver_api) = {
	.enable = lin_transceiver_gpio_enable,
	.disable = lin_transceiver_gpio_disable,
};

#define LIN_TRANSCEIVER_GPIO_COND(inst, name)                                                      \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, name##_gpios),		                           \
		   (.name##_gpio = GPIO_DT_SPEC_INST_GET(inst, name##_gpios),))

/* clang-format off */

#define LIN_TRANSCEIVER_GPIO_INIT(inst)                                                            \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, enable_gpios) ||                                  \
		     DT_INST_NODE_HAS_PROP(inst, wakeup_gpios),                                    \
		     "Missing GPIO property on " DT_NODE_FULL_NAME(DT_DRV_INST(inst)));            \
                                                                                                   \
	static const struct lin_transceiver_gpio_config lin_transceiver_gpio_config_##inst = {     \
		LIN_TRANSCEIVER_GPIO_COND(inst, enable)                                            \
		LIN_TRANSCEIVER_GPIO_COND(inst, wakeup)                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &lin_transceiver_gpio_init, NULL, NULL,                        \
			      &lin_transceiver_gpio_config_##inst, POST_KERNEL,                    \
			      CONFIG_LIN_TRANSCEIVER_INIT_PRIORITY,                                \
			      &lin_transceiver_gpio_driver_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(LIN_TRANSCEIVER_GPIO_INIT);
