/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT power_domain_gpio

#include <kernel.h>
#include <drivers/gpio.h>
#include <pm/device.h>
#include <pm/device_runtime.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(power_domain_gpio, LOG_LEVEL_INF);

struct pd_gpio_config {
	struct gpio_dt_spec enable;
	uint32_t startup_delay_us;
	uint32_t off_on_delay_us;
};

struct pd_gpio_data {
	k_timeout_t last_boot;
};

const char *actions[] = {
	[PM_DEVICE_ACTION_RESUME] = "RESUME",
	[PM_DEVICE_ACTION_SUSPEND] = "SUSPEND",
	[PM_DEVICE_ACTION_TURN_ON] = "TURN ON",
	[PM_DEVICE_ACTION_TURN_OFF] = "TURN OFF"
};


static int pd_gpio_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct pd_gpio_config *cfg = dev->config;
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Switch power on */
		gpio_pin_set_dt(&cfg->enable, 1);
		LOG_DBG("%s is now ON", dev->name);
		/* Notify supported devices they are now powered */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Notify supported devices power is going down */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		/* Switch power off */
		gpio_pin_set_dt(&cfg->enable, 0);
		LOG_DBG("%s is now OFF and powered", dev->name);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/* Actively control the enable pin now that the device is powered */
		gpio_pin_configure_dt(&cfg->enable, GPIO_OUTPUT_INACTIVE);
		LOG_DBG("%s is OFF and powered", dev->name);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/* Let the enable pin float while device is not powered */
		gpio_pin_configure_dt(&cfg->enable, GPIO_DISCONNECTED);
		LOG_DBG("%s is OFF and not powered", dev->name);
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int pd_gpio_init(const struct device *dev)
{
	const struct pd_gpio_config *cfg = dev->config;
	int rc;

	if (!device_is_ready(cfg->enable.port)) {
		LOG_ERR("GPIO port %s is not ready", cfg->enable.port->name);
		return -ENODEV;
	}

	if (pm_device_on_power_domain(dev)) {
		/* Device is unpowered */
		pm_device_runtime_init_off(dev);
		rc = gpio_pin_configure_dt(&cfg->enable, GPIO_DISCONNECTED);
	} else {
		pm_device_runtime_init_suspended(dev);
		rc = gpio_pin_configure_dt(&cfg->enable, GPIO_OUTPUT_INACTIVE);
	}

	return rc;
}

#define POWER_DOMAIN_DEVICE(id)						   \
	static const struct pd_gpio_config pd_gpio_##id##_cfg = {	   \
		.enable = GPIO_DT_SPEC_INST_GET(id, enable_gpios),	   \
		.startup_delay_us = DT_INST_PROP(id, startup_delay_us),	   \
		.off_on_delay_us = DT_INST_PROP(id, off_on_delay_us),	   \
	};								   \
	static struct pd_gpio_data pd_gpio_##id##_data;			   \
	PM_DEVICE_DT_INST_DEFINE(id, pd_gpio_pm_action);		   \
	DEVICE_DT_INST_DEFINE(id, pd_gpio_init, PM_DEVICE_DT_INST_GET(id), \
			      &pd_gpio_##id##_data, &pd_gpio_##id##_cfg,   \
			      POST_KERNEL, 75,				   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
