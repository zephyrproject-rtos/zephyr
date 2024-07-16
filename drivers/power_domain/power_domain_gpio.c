/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT power_domain_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power_domain_gpio, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct pd_gpio_config {
	struct gpio_dt_spec enable;
	const struct gpio_dt_spec *followers;
	uint32_t num_followers;
	uint32_t startup_delay_us;
	uint32_t off_on_delay_us;
};

struct pd_gpio_data {
	k_timeout_t next_boot;
};

struct pd_visitor_context {
	const struct device *domain;
	enum pm_device_action action;
};

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN

static int pd_on_domain_visitor(const struct device *dev, void *context)
{
	struct pd_visitor_context *visitor_context = context;

	/* Only run action if the device is on the specified domain */
	if (!dev->pm || (dev->pm_base->domain != visitor_context->domain)) {
		return 0;
	}

	(void)pm_device_action_run(dev, visitor_context->action);
	return 0;
}

#endif

static int pd_gpio_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	struct pd_visitor_context context = {.domain = dev};
#endif
	const struct pd_gpio_config *cfg = dev->config;
	struct pd_gpio_data *data = dev->data;
	const struct gpio_dt_spec *gpio;
	int64_t next_boot_ticks;
	int rc = 0;

	/* Validate that blocking API's can be used */
	if (!k_can_yield()) {
		LOG_ERR("Blocking actions cannot run in this context");
		return -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Wait until we can boot again */
		k_sleep(data->next_boot);
		/* Switch power on */
		gpio_pin_set_dt(&cfg->enable, 1);
		/* Enable all follower GPIOs */
		for (int i = 0; i < cfg->num_followers; i++) {
			/* Set pin back to physical high value */
			gpio = &cfg->followers[i];
			gpio_pin_set_raw(gpio->port, gpio->pin, 1);
			LOG_DBG("%s:%02d active", cfg->followers[i].port->name,
				cfg->followers[i].pin);
		}
		LOG_INF("%s is now ON", dev->name);
		/* Wait for domain to come up */
		k_sleep(K_USEC(cfg->startup_delay_us));
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
		/* Notify devices on the domain they are now powered */
		context.action = PM_DEVICE_ACTION_TURN_ON;
		(void)device_supported_foreach(dev, pd_on_domain_visitor, &context);
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
		/* Notify devices on the domain that power is going down */
		context.action = PM_DEVICE_ACTION_TURN_OFF;
		(void)device_supported_foreach(dev, pd_on_domain_visitor, &context);
#endif
		/* Disable all follower GPIOs */
		for (int i = 0; i < cfg->num_followers; i++) {
			/* Set pin to physical low value */
			gpio = &cfg->followers[i];
			gpio_pin_set_raw(gpio->port, gpio->pin, 0);
			LOG_DBG("%s:%02d inactive", cfg->followers[i].port->name,
				cfg->followers[i].pin);
		}
		/* Switch power off */
		gpio_pin_set_dt(&cfg->enable, 0);
		LOG_INF("%s is now OFF", dev->name);
		/* Store next time we can boot */
		next_boot_ticks = k_uptime_ticks() + k_us_to_ticks_ceil32(cfg->off_on_delay_us);
		data->next_boot = K_TIMEOUT_ABS_TICKS(next_boot_ticks);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/* Ensure follower GPIOs are disabled */
		for (int i = 0; i < cfg->num_followers; i++) {
			/* Set pin to physical low value */
			gpio = &cfg->followers[i];
			gpio_pin_set_raw(gpio->port, gpio->pin, 0);
			LOG_DBG("%s:%02d inactive", cfg->followers[i].port->name,
				cfg->followers[i].pin);
		}
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
	struct pd_gpio_data *data = dev->data;

	if (!gpio_is_ready_dt(&cfg->enable)) {
		LOG_ERR("GPIO port %s is not ready", cfg->enable.port->name);
		return -ENODEV;
	}
	/* We can't know how long the domain has been off for before boot */
	data->next_boot = K_TIMEOUT_ABS_US(cfg->off_on_delay_us);

	/* Boot according to state */
	return pm_device_driver_init(dev, pd_gpio_pm_action);
}

#define _INST_RAW_FOLLOWERS(id)					\
	DT_INST_FOREACH_PROP_ELEM_SEP(id, raw_follower_gpios,	\
		GPIO_DT_SPEC_GET_BY_IDX, (,))

/* Generate array of raw follower GPIOs */
#define RAW_FOLLOWERS(id, name)							\
	static const struct gpio_dt_spec name[] = {				\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(id, raw_follower_gpios),	\
			    (_INST_RAW_FOLLOWERS(id)), ())			\
	}

#define POWER_DOMAIN_DEVICE(id)							\
	RAW_FOLLOWERS(id, followers##id);					\
	static const struct pd_gpio_config pd_gpio_##id##_cfg = {		\
		.enable = GPIO_DT_SPEC_INST_GET(id, enable_gpios),		\
		.followers = followers##id,					\
		.num_followers = ARRAY_SIZE(followers##id),			\
		.startup_delay_us = DT_INST_PROP(id, startup_delay_us),		\
		.off_on_delay_us = DT_INST_PROP(id, off_on_delay_us),		\
	};									\
	static struct pd_gpio_data pd_gpio_##id##_data;				\
	PM_DEVICE_DT_INST_DEFINE(id, pd_gpio_pm_action);			\
	DEVICE_DT_INST_DEFINE(id, pd_gpio_init, PM_DEVICE_DT_INST_GET(id),	\
			      &pd_gpio_##id##_data, &pd_gpio_##id##_cfg,	\
			      POST_KERNEL, CONFIG_POWER_DOMAIN_GPIO_INIT_PRIORITY,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(POWER_DOMAIN_DEVICE)
