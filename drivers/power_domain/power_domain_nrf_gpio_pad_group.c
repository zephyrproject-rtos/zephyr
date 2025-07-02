/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_gpio_pad_group

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <hal/nrf_gpio.h>

LOG_MODULE_REGISTER(nrf_gpio_pad_group, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct nrf_port_retain_config {
	NRF_GPIO_Type *regs;
	uint32_t retain_mask;
};

static void nrf_port_retain_driver_turn_off(const struct device *dev)
{
	const struct nrf_port_retain_config *dev_config = dev->config;

	LOG_DBG("%s pads 0x%08x retain %s", dev->name, dev_config->retain_mask, "enable");
	nrf_gpio_port_retain_enable(dev_config->regs, dev_config->retain_mask);
}

static void nrf_port_retain_driver_turn_on(const struct device *dev)
{
	const struct nrf_port_retain_config *dev_config = dev->config;

	LOG_DBG("%s pads 0x%08x retain %s", dev->name, dev_config->retain_mask, "disable");
	nrf_gpio_port_retain_disable(dev_config->regs, dev_config->retain_mask);
}

static int nrf_port_retain_driver_pm_action(const struct device *dev,
					    enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_OFF:
		nrf_port_retain_driver_turn_off(dev);
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		nrf_port_retain_driver_turn_on(dev);
		break;

	default:
		break;
	};

	return 0;
}

static int nrf_port_retain_driver_init(const struct device *dev)
{
	return pm_device_driver_init(dev, nrf_port_retain_driver_pm_action);
}

#define NRF_GPIO_PAD_GROUP_DEFINE(inst)								\
	static const struct nrf_port_retain_config _CONCAT(config, inst) = {			\
		.regs = (NRF_GPIO_Type *)DT_REG_ADDR(DT_INST_PARENT(inst)),			\
		.retain_mask = DT_PROP_OR(inst, retain_mask, UINT32_MAX),			\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, nrf_port_retain_driver_pm_action);			\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		nrf_port_retain_driver_init,							\
		PM_DEVICE_DT_INST_GET(inst),							\
		NULL,										\
		&_CONCAT(config, inst),								\
		PRE_KERNEL_1,									\
		UTIL_INC(CONFIG_GPIO_INIT_PRIORITY),						\
		NULL										\
	);

DT_INST_FOREACH_STATUS_OKAY(NRF_GPIO_PAD_GROUP_DEFINE)
