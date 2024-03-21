/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT power_domain_gpio_monitor

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(power_domain_gpio_monitor, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct pd_gpio_monitor_config {
	struct gpio_dt_spec power_good_gpio;
};

struct pd_gpio_monitor_data {
	struct gpio_callback callback;
	const struct device *dev;
	bool is_powered;
};

struct pd_visitor_context {
	const struct device *domain;
	enum pm_device_action action;
};

static int pd_on_domain_visitor(const struct device *dev, void *context)
{
	struct pd_visitor_context *visitor_context = context;

	/* Only run action if the device is on the specified domain */
	if (!dev->pm || (dev->pm_base->domain != visitor_context->domain)) {
		return 0;
	}

	dev->pm->usage = 0;
	(void)pm_device_action_run(dev, visitor_context->action);
	return 0;
}

static void pd_gpio_monitor_callback(const struct device *port,
				    struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct pd_gpio_monitor_data *data = CONTAINER_OF(cb, struct pd_gpio_monitor_data, callback);
	const struct pd_gpio_monitor_config *config = data->dev->config;
	const struct device *dev = data->dev;
	struct pd_visitor_context context = {.domain = dev};
	int rc;

	rc = gpio_pin_get_dt(&config->power_good_gpio);
	if (rc < 0) {
		LOG_WRN("Failed to read gpio logic level");
		return;
	}

	data->is_powered = rc;
	if (rc == 0) {
		context.action = PM_DEVICE_ACTION_SUSPEND;
		(void)device_supported_foreach(dev, pd_on_domain_visitor, &context);
		context.action = PM_DEVICE_ACTION_TURN_OFF;
		(void)device_supported_foreach(dev, pd_on_domain_visitor, &context);
		return;
	}

	pm_device_children_action_run(data->dev, PM_DEVICE_ACTION_TURN_ON, NULL);
}

static int pd_gpio_monitor_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct pd_gpio_monitor_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return -ENOTSUP;
	case PM_DEVICE_ACTION_RESUME:
		if (!data->is_powered) {
			return -EAGAIN;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int pd_gpio_monitor_init(const struct device *dev)
{
	const struct pd_gpio_monitor_config *config = dev->config;
	struct pd_gpio_monitor_data *data = dev->data;
	int rc;

	data->dev = dev;

	if (!gpio_is_ready_dt(&config->power_good_gpio)) {
		LOG_ERR("GPIO port %s is not ready", config->power_good_gpio.port->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&config->power_good_gpio, GPIO_INPUT);
	if (rc) {
		LOG_ERR("Failed to configure GPIO");
		goto unconfigure_pin;
	}

	rc = gpio_pin_interrupt_configure_dt(&config->power_good_gpio, GPIO_INT_EDGE_BOTH);
	if (rc) {
		gpio_pin_configure_dt(&config->power_good_gpio, GPIO_DISCONNECTED);
		LOG_ERR("Failed to configure GPIO interrupt");
		goto unconfigure_interrupt;
	}

	gpio_init_callback(&data->callback, pd_gpio_monitor_callback,
						BIT(config->power_good_gpio.pin));
	rc = gpio_add_callback_dt(&config->power_good_gpio, &data->callback);
	if (rc) {
		LOG_ERR("Failed to add GPIO callback");
		goto remove_callback;
	}

	pm_device_init_suspended(dev);
	return pm_device_runtime_enable(dev);
remove_callback:
	gpio_remove_callback(config->power_good_gpio.port, &data->callback);
unconfigure_interrupt:
	gpio_pin_interrupt_configure_dt(&config->power_good_gpio, GPIO_INT_DISABLE);
unconfigure_pin:
	gpio_pin_configure_dt(&config->power_good_gpio, GPIO_DISCONNECTED);
	return rc;
}

#define POWER_DOMAIN_DEVICE(inst)								\
	static const struct pd_gpio_monitor_config pd_gpio_monitor_config_##inst = {		\
		.power_good_gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),				\
	};											\
	static struct pd_gpio_monitor_data pd_gpio_monitor_data_##inst;				\
	PM_DEVICE_DT_INST_DEFINE(inst, pd_gpio_monitor_pm_action);				\
	DEVICE_DT_INST_DEFINE(inst, pd_gpio_monitor_init,					\
				PM_DEVICE_DT_INST_GET(inst), &pd_gpio_monitor_data_##inst,	\
				&pd_gpio_monitor_config_##inst, POST_KERNEL,			\
				CONFIG_POWER_DOMAIN_GPIO_MONITOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
