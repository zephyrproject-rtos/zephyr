/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT can_transceiver_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_transceiver_gpio, CONFIG_CAN_LOG_LEVEL);

/* Does any devicetree instance have an enable-gpios property? */
#define INST_HAS_ENABLE_GPIOS_OR(inst) DT_INST_NODE_HAS_PROP(inst, enable_gpios) ||
#define ANY_INST_HAS_ENABLE_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_ENABLE_GPIOS_OR) 0

/* Does any devicetree instance have a standby-gpios property? */
#define INST_HAS_STANDBY_GPIOS_OR(inst) DT_INST_NODE_HAS_PROP(inst, standby_gpios) ||
#define ANY_INST_HAS_STANDBY_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_STANDBY_GPIOS_OR) 0

struct can_transceiver_gpio_config {
#if ANY_INST_HAS_ENABLE_GPIOS
	struct gpio_dt_spec enable_gpio;
#endif /* ANY_INST_HAS_ENABLE_GPIOS */
#if ANY_INST_HAS_STANDBY_GPIOS
	struct gpio_dt_spec standby_gpio;
#endif /* ANY_INST_HAS_STANDBY_GPIOS */
};

static int can_transceiver_gpio_set_state(const struct device *dev, bool enabled)
{
	const struct can_transceiver_gpio_config *config = dev->config;
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

#if ANY_INST_HAS_STANDBY_GPIOS
	if (config->standby_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->standby_gpio, enabled ? 0 : 1);
		if (err != 0) {
			LOG_ERR("failed to set standby GPIO pin (err %d)", err);
			return -EIO;
		}
	}
#endif /* ANY_INST_HAS_STANDBY_GPIOS */

	return 0;
}

static int can_transceiver_gpio_enable(const struct device *dev, can_mode_t mode)
{
	ARG_UNUSED(mode);

	return can_transceiver_gpio_set_state(dev, true);
}

static int can_transceiver_gpio_disable(const struct device *dev)
{
	return can_transceiver_gpio_set_state(dev, false);
}

static int can_transceiver_gpio_init(const struct device *dev)
{
	const struct can_transceiver_gpio_config *config = dev->config;
	int err;

#if ANY_INST_HAS_ENABLE_GPIOS
	if (config->enable_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->enable_gpio)) {
			LOG_ERR("enable pin GPIO device not ready");
			return -EINVAL;
		}

		/* CAN transceiver is disabled during initialization */
		err = gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure enable GPIO pin (err %d)", err);
			return err;
		}
	}
#endif /* ANY_INST_HAS_ENABLE_GPIOS */

#if ANY_INST_HAS_STANDBY_GPIOS
	if (config->standby_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->standby_gpio)) {
			LOG_ERR("standby pin GPIO device not ready");
			return -EINVAL;
		}

		/* CAN transceiver is put in standby during initialization */
		err = gpio_pin_configure_dt(&config->standby_gpio, GPIO_OUTPUT_ACTIVE);
		if (err != 0) {
			LOG_ERR("failed to configure standby GPIO pin (err %d)", err);
			return err;
		}
	}
#endif /* ANY_INST_HAS_STANDBY_GPIOS */

	return 0;
}

static const struct can_transceiver_driver_api can_transceiver_gpio_driver_api = {
	.enable = can_transceiver_gpio_enable,
	.disable = can_transceiver_gpio_disable,
};

#define CAN_TRANSCEIVER_GPIO_COND(inst, name)				\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, name##_gpios),		\
		   (.name##_gpio = GPIO_DT_SPEC_INST_GET(inst, name##_gpios),))

#define CAN_TRANSCEIVER_GPIO_INIT(inst)					\
	static const struct can_transceiver_gpio_config	can_transceiver_gpio_config_##inst = { \
		CAN_TRANSCEIVER_GPIO_COND(inst, enable)			\
		CAN_TRANSCEIVER_GPIO_COND(inst, standby)		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst, &can_transceiver_gpio_init,		\
			NULL, NULL, &can_transceiver_gpio_config_##inst,\
			POST_KERNEL, CONFIG_CAN_TRANSCEIVER_INIT_PRIORITY, \
			&can_transceiver_gpio_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(CAN_TRANSCEIVER_GPIO_INIT)
