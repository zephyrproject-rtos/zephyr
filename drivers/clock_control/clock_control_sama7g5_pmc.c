/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sam_pmc

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static int get_pmc_clk(const struct device *dev, clock_control_subsys_t sys,
		       struct device **clk)
{
	const struct sam_clk_cfg *cfg = (const struct sam_clk_cfg *)sys;
	const struct sam_pmc_data *data = dev->data;

	if (cfg == NULL || data == NULL) {
		LOG_ERR("The PMC config and data can not be NULL.");
		return -ENXIO;
	}

	LOG_DBG("Type: %x, Id: %d", cfg->clock_type, cfg->clock_id);

	*clk = sam_pmc_get_clock(cfg, data->pmc);
	if (*clk) {
		return 0;
	}

	LOG_ERR("The PMC clock type is not implemented.");

	return -ENODEV;
}

static int sam_clock_control_on(const struct device *dev,
				clock_control_subsys_t sys)
{
	struct device *clk;
	int ret = get_pmc_clk(dev, sys, &clk);

	if (!ret) {
		return clock_control_on(clk, sys);
	}

	return ret;
}

static int sam_clock_control_off(const struct device *dev,
				 clock_control_subsys_t sys)
{
	struct device *clk;
	int ret = get_pmc_clk(dev, sys, &clk);

	if (!ret) {
		return clock_control_off(clk, sys);
	}

	return ret;
}

static int sam_clock_control_get_rate(const struct device *dev,
				      clock_control_subsys_t sys,
				      uint32_t *rate)
{
	struct device *clk;
	int ret = get_pmc_clk(dev, sys, &clk);

	if (!ret) {
		return clock_control_get_rate(clk, sys, rate);
	}

	return ret;
}

static enum clock_control_status
sam_clock_control_get_status(const struct device *dev,
			     clock_control_subsys_t sys)
{
	struct device *clk;
	int ret = get_pmc_clk(dev, sys, &clk);

	if (!ret) {
		return clock_control_get_status(clk, sys);
	}

	return ret;
}

static int clock_control_sam_pmc_init(const struct device *dev)
{
	sam_pmc_setup(dev);
	return 0;
}

static DEVICE_API(clock_control, sam_clock_control_api) = {
	.on = sam_clock_control_on,
	.off = sam_clock_control_off,
	.get_rate = sam_clock_control_get_rate,
	.get_status = sam_clock_control_get_status,
};

#define PMC_INIT_CFG_SLCK(n, slck)							\
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, slck),					\
	(										\
		.slck = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, slck)),		\
		.slck##_cfg = {								\
			.crystal_osc = DT_INST_CLOCKS_CELL_BY_NAME(n, slck,		\
								   clock_crystal_osc),	\
		},									\
	), ())

#define PMC_INIT_CFG_CLK(n, clk)							\
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, clk),					\
	(										\
		.clk = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_DRV_INST(n), clk)),	\
	), ())

#define PMC_CFG_DEFN(n)									\
	static const struct sam_pmc_cfg pmc##n##_cfg = {				\
		.reg = (uint32_t *)DT_INST_REG_ADDR(n),					\
		PMC_INIT_CFG_SLCK(n, td_slck)						\
		PMC_INIT_CFG_SLCK(n, md_slck)						\
		PMC_INIT_CFG_CLK(n, main_xtal)						\
	}
#define SAM_PMC_DEVICE_INIT(n)								\
static struct sam_pmc_data pmc##n##data;						\
PMC_CFG_DEFN(n);									\
											\
DEVICE_DT_INST_DEFINE(n, clock_control_sam_pmc_init, NULL,				\
		      &pmc##n##data,							\
		      &pmc##n##_cfg,							\
		      PRE_KERNEL_1,							\
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,				\
		      &sam_clock_control_api);						\

DT_INST_FOREACH_STATUS_OKAY(SAM_PMC_DEVICE_INIT)
