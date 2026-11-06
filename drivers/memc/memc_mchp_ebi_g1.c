/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ebi_g1_memc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/memc/mchp_smc_g1.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memc_mchp_ebi_g1, CONFIG_MEMC_LOG_LEVEL);

struct memc_ebi_config {
	struct sam_clk_cfg clk_cfg;
	const struct pinctrl_dev_config *pin_cfg;
	const struct device *smc_dev;
	const struct cs_config *cs_cfg;
	size_t cs_num;
};

static int memc_ebi_init(const struct device *dev)
{
	const struct memc_ebi_config *cfg = dev->config;
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	int ret;

	if (!device_is_ready(pmc)) {
		LOG_ERR("EBI: Power Management Controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(pmc, (clock_control_subsys_t)(uintptr_t)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("EBI: Clock op failed");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(cfg->smc_dev)) {
		LOG_ERR("EBI: smc device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < cfg->cs_num; i++) {
		ret = smc_cs_conf_apply(cfg->smc_dev, &cfg->cs_cfg[i]);
		if (ret) {
			LOG_ERR("EBI: Failed to apply CS %d config, ret=%d", i, ret);
			return ret;
		}
	}

	if (smc_set_mck_cfg(cfg->smc_dev, (clock_control_subsys_t)(uintptr_t)&cfg->clk_cfg)) {
		return -EINVAL;
	}

	return 0;
}

#define CS_CONFIG(node_id)					\
	{							\
		.cs      = DT_PROP_BY_IDX(node_id, reg, 0),	\
		.setup   = SMC_REG_SETUP(node_id),		\
		.pulse   = SMC_REG_PULSE(node_id),		\
		.cycle   = SMC_REG_CYCLE(node_id),		\
		.timings = SMC_REG_TIMINGS(node_id),		\
		.mode    = SMC_REG_MODE(node_id),		\
	},

#define MEMC_EBI_DEFINE(inst)						\
	static const struct cs_config cs_config_##inst[] = {		\
		DT_INST_FOREACH_CHILD(inst, CS_CONFIG)			\
	};								\
	PINCTRL_DT_INST_DEFINE(inst);					\
	static const struct memc_ebi_config ebi_config_##inst = {	\
		.clk_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),		\
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	\
		.smc_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, smc)),	\
		.cs_cfg  = cs_config_##inst,				\
		.cs_num  = ARRAY_SIZE(cs_config_##inst),		\
	};								\
	DEVICE_DT_INST_DEFINE(inst, memc_ebi_init, NULL, NULL,		\
			      &ebi_config_##inst, POST_KERNEL,		\
			      CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_EBI_DEFINE)
