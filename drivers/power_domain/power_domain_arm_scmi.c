/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_scmi_power_domain

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/firmware/scmi/power.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(scmi_power_domain, CONFIG_POWER_DOMAIN_LOG_LEVEL);

struct scmi_pd_config {
	uint32_t domain_id;
};

static int scmi_pd_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct scmi_pd_config *cfg = dev->config;
	struct scmi_power_state_config pwr_cfg;
	int ret;

	LOG_INF("attempting PM action %d on domain %d", action, cfg->domain_id);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		pwr_cfg.domain_id = cfg->domain_id;
		pwr_cfg.flags = 0;
		pwr_cfg.power_state = SCMI_POWER_STATE_GENERIC_ON;

		ret = scmi_power_state_set(&pwr_cfg);
		if (ret < 0) {
			return ret;
		}
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);

		pwr_cfg.domain_id = cfg->domain_id;
		pwr_cfg.flags = 0;
		pwr_cfg.power_state = SCMI_POWER_STATE_GENERIC_OFF;

		ret = scmi_power_state_set(&pwr_cfg);
		if (ret < 0) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int scmi_pd_init(const struct device *dev)
{
	return pm_device_driver_init(dev, scmi_pd_pm_action);
}
#define SCMI_PD_DEVICE(inst)							\
	static const struct scmi_pd_config scmi_pd_cfg_##inst = {		\
		.domain_id = DT_INST_REG_ADDR(inst),				\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, scmi_pd_pm_action);			\
	DEVICE_DT_INST_DEFINE(inst, scmi_pd_init,				\
			      PM_DEVICE_DT_INST_GET(inst),			\
			      NULL,						\
			      &scmi_pd_cfg_##inst,				\
			      PRE_KERNEL_2,					\
			      CONFIG_POWER_DOMAIN_ARM_SCMI_INIT_PRIORITY,	\
			      NULL);
DT_INST_FOREACH_STATUS_OKAY(SCMI_PD_DEVICE)
