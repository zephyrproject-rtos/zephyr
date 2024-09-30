/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <main/ipc.h>
#include <svc/pm/pm_api.h>
#include <zephyr/dt-bindings/power/imx_scu_rsrc.h>

LOG_MODULE_REGISTER(nxp_scu_pd);

#define DT_DRV_COMPAT nxp_scu_pd

struct scu_pd_data {
	sc_ipc_t handle;
	sc_rsrc_t rsrc;
};

static int scu_pd_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret;
	sc_pm_power_mode_t mode;
	struct scu_pd_data *scu_data = dev->data;

	LOG_DBG("attempting PM action %d on rsrc %d", action, scu_data->rsrc);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		mode = SC_PM_PW_MODE_ON;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		mode = SC_PM_PW_MODE_OFF;
		break;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}

	ret = sc_pm_set_resource_power_mode(scu_data->handle,
					    scu_data->rsrc,
					    mode);
	if (ret != SC_ERR_NONE) {
		LOG_ERR("failed to set rsrc %d power mode to %d",
			scu_data->rsrc, mode);
		return -EIO;
	}

	return 0;
}

static int scu_pd_init(const struct device *dev)
{
	int ret;
	struct scu_pd_data *scu_data = dev->data;

	ret = sc_ipc_open(&scu_data->handle, DT_REG_ADDR(DT_NODELABEL(scu_mu)));
	if (ret != SC_ERR_NONE) {
		return -ENODEV;
	}

	return pm_device_runtime_enable(dev);
}


#define SCU_PD_DEVICE_DEFINE(inst)					\
									\
BUILD_ASSERT(DT_INST_PROP(inst, nxp_resource_id) < IMX_SC_R_LAST,	\
	     "invalid resource ID");					\
									\
static struct scu_pd_data scu_pd_data_##inst = {			\
	.rsrc = DT_INST_PROP(inst, nxp_resource_id),			\
};									\
									\
PM_DEVICE_DT_INST_DEFINE(inst, scu_pd_pm_action);			\
									\
DEVICE_DT_INST_DEFINE(inst, scu_pd_init, PM_DEVICE_DT_INST_GET(inst),	\
		      &scu_pd_data_##inst, NULL, PRE_KERNEL_1,		\
		      CONFIG_POWER_DOMAIN_NXP_SCU_INIT_PRIORITY,	\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(SCU_PD_DEVICE_DEFINE);
