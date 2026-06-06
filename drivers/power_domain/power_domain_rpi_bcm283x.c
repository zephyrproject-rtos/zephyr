/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_bcm283x_power

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <rpi_fw.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpi_bcm283x_power, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct rpi_bcm283x_power_config {
	const struct device *fw_dev;
	uint32_t domain_id;
};

struct rpi_bcm283x_power_state {
	uint32_t dev_id;
	uint32_t state;
};

static int rpi_bcm283x_power_on(const struct device *dev)
{
	const struct rpi_bcm283x_power_config *config = dev->config;
	struct rpi_bcm283x_power_state power;
	int ret;

	power.dev_id = config->domain_id;
	power.state = 1;

	ret = rpi_fw_transfer(config->fw_dev, RPI_FW_TAG_SET_POWER_STATE, &power, sizeof(power));
	if (ret < 0) {
		LOG_ERR("Failed to set power state on (%d)", ret);
	}

	LOG_DBG("Power Domain ID: %d, State: %s", power.dev_id, power.state ? "ON" : "OFF");

	return ret;
}

static int rpi_bcm283x_power_off(const struct device *dev)
{
	const struct rpi_bcm283x_power_config *config = dev->config;
	struct rpi_bcm283x_power_state power;
	int ret;

	power.dev_id = config->domain_id;
	power.state = 0;

	ret = rpi_fw_transfer(config->fw_dev, RPI_FW_TAG_SET_POWER_STATE, &power, sizeof(power));
	if (ret < 0) {
		LOG_ERR("Failed to set power state off (%d)", ret);
	}

	LOG_DBG("Power Domain ID: %d, State: %s", power.dev_id, power.state ? "ON" : "OFF");

	return ret;
}

static int rpi_bcm283x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct rpi_bcm283x_power_config *config = dev->config;

	LOG_DBG("PM action %d on domain 0x%08x", action, config->domain_id);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return rpi_bcm283x_power_on(dev);

	case PM_DEVICE_ACTION_SUSPEND:
		return rpi_bcm283x_power_off(dev);

	case PM_DEVICE_ACTION_TURN_ON:
		return 0;

	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int rpi_bcm283x_power_init(const struct device *dev)
{
	const struct rpi_bcm283x_power_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->fw_dev)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	ret = pm_device_driver_init(dev, rpi_bcm283x_pm_action);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime PM: %d", ret);
	}

	return ret;
}

#define RPI_POWER_DOMAIN_INIT(n)                                                                   \
	static const struct rpi_bcm283x_power_config rpi_bcm283x_power_config_##n = {              \
		.fw_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, firmware)),                             \
		.domain_id = DT_INST_PROP(n, power_domain_id),                                     \
	};                                                                                         \
	                                                                                           \
	PM_DEVICE_DT_INST_DEFINE(n, rpi_bcm283x_pm_action);                                        \
	                                                                                           \
	DEVICE_DT_INST_DEFINE(n, rpi_bcm283x_power_init, PM_DEVICE_DT_INST_GET(n), NULL,           \
			      &rpi_bcm283x_power_config_##n, POST_KERNEL,                          \
			      CONFIG_POWER_DOMAIN_RASPBERRYPI_BCM283X_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RPI_POWER_DOMAIN_INIT)
