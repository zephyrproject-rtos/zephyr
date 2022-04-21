/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_power_domain_psl

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(power_domain_psl, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct npcx_psl_config {
	struct gpio_dt_spec enable;
	/* PSL_IN pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

static int npcx_psl_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct npcx_psl_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_OFF:
		/* Configure detection settings of PSL_IN pads first */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			LOG_ERR("PSL_IN pinctrl setup failed (%d)", ret);
			return ret;
		}

		/**
		 * A transition from 0 to 1 of specific IO (GPIO85) data-out bit
		 * set PSL_OUT to inactive state. Then, it will turn Core Domain
		 * power supply (VCC1) off for better power consumption.
		 */
		gpio_pin_set_dt(&config->enable, 1);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int npcx_psl_init(const struct device *dev)
{
	const struct npcx_psl_config *config = dev->config;

	if (!device_is_ready(config->enable.port)) {
		LOG_ERR("GPIO port %s is not ready", config->enable.port->name);
		return -ENODEV;
	}

	/*
	 * No need to configure GPIO used to turn on/off VCC1 power domain. It
	 * is not a standard GPIO and only effected by setting corresponding bit
	 * of PDOUT reg in the module.
	 */

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct npcx_psl_config power_psl_cfg = {
	.enable = GPIO_DT_SPEC_INST_GET(0, enable_gpios),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

PM_DEVICE_DT_INST_DEFINE(0, npcx_psl_pm_action);
DEVICE_DT_INST_DEFINE(0,
		      npcx_psl_init,
		      PM_DEVICE_DT_INST_GET(0),
		      NULL,
		      &power_psl_cfg,
		      POST_KERNEL,
		      CONFIG_POWER_DOMAIN_INIT_PRIORITY,
		      NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	"only one 'nuvoton_npcx_power_domain_psl' compatible node may be present");
