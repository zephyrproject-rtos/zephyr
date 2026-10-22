/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_spc_bandgap_buffer

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/pm/device.h>

#define NXP_SPC_BANDGAP_BUFFER_DISABLED SPC_ACTIVE_CFG_BGMODE(1U)
#define NXP_SPC_BANDGAP_BUFFER_ENABLED  SPC_ACTIVE_CFG_BGMODE(2U)

struct regulator_nxp_spc_bandgap_config {
	struct regulator_common_config common;
	SPC_Type *base;
};

struct regulator_nxp_spc_bandgap_data {
	struct regulator_common_data common;
#if defined(CONFIG_PM_DEVICE)
	bool restore_buffer;
#endif
};

static int regulator_nxp_spc_bandgap_enable(const struct device *dev)
{
	const struct regulator_nxp_spc_bandgap_config *config = dev->config;
	uint32_t value;

	value = config->base->ACTIVE_CFG;
	value &= ~SPC_ACTIVE_CFG_BGMODE_MASK;
	value |= NXP_SPC_BANDGAP_BUFFER_ENABLED;
	config->base->ACTIVE_CFG = value;

	return 0;
}

static int regulator_nxp_spc_bandgap_disable(const struct device *dev)
{
	const struct regulator_nxp_spc_bandgap_config *config = dev->config;
	uint32_t value;

	value = config->base->ACTIVE_CFG;
	value &= ~SPC_ACTIVE_CFG_BGMODE_MASK;
	value |= NXP_SPC_BANDGAP_BUFFER_DISABLED;
	config->base->ACTIVE_CFG = value;

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int regulator_nxp_spc_bandgap_pm_action(const struct device *dev,
					       enum pm_device_action action)
{
	const struct regulator_nxp_spc_bandgap_config *config = dev->config;
	struct regulator_nxp_spc_bandgap_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		data->restore_buffer =
			((config->base->ACTIVE_CFG & SPC_ACTIVE_CFG_BGMODE_MASK) ==
			 NXP_SPC_BANDGAP_BUFFER_ENABLED);
		if (data->restore_buffer) {
			return regulator_nxp_spc_bandgap_disable(dev);
		}
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		if (data->restore_buffer) {
			return regulator_nxp_spc_bandgap_enable(dev);
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif

static int regulator_nxp_spc_bandgap_init(const struct device *dev)
{
	const struct regulator_nxp_spc_bandgap_config *config = dev->config;
	const bool is_enabled =
		((config->base->ACTIVE_CFG & SPC_ACTIVE_CFG_BGMODE_MASK) ==
		 NXP_SPC_BANDGAP_BUFFER_ENABLED);

	regulator_common_data_init(dev);

	return regulator_common_init(dev, is_enabled);
}

static DEVICE_API(regulator, regulator_nxp_spc_bandgap_api) = {
	.enable = regulator_nxp_spc_bandgap_enable,
	.disable = regulator_nxp_spc_bandgap_disable,
};

#define REGULATOR_NXP_SPC_BANDGAP_DEFINE(inst)							\
	static struct regulator_nxp_spc_bandgap_data data_##inst;				\
												\
	static const struct regulator_nxp_spc_bandgap_config config_##inst = {			\
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),				\
		.base = (SPC_Type *)DT_REG_ADDR(DT_PARENT(DT_DRV_INST(inst))),			\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, regulator_nxp_spc_bandgap_pm_action);			\
											\
	DEVICE_DT_INST_DEFINE(inst, regulator_nxp_spc_bandgap_init,				\
			      PM_DEVICE_DT_INST_GET(inst), &data_##inst, &config_##inst,	\
			      POST_KERNEL, CONFIG_REGULATOR_NXP_SPC_BANDGAP_INIT_PRIORITY,	\
			      &regulator_nxp_spc_bandgap_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NXP_SPC_BANDGAP_DEFINE)
