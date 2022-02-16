/*
 * Copyright (c) 2020-2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_cpg_mssr

#include <errno.h>
#include <soc.h>
#include <clock_soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/rcar_clock_control.h>
#include <dt-bindings/clock/renesas/renesas-cpg-mssr.h>

struct rcar_mssr_config {
	uint32_t base_address;
};

static void cpg_write(const struct rcar_mssr_config *config,
		      uint32_t reg, uint32_t val)
{
	sys_write32(~val, config->base_address + CPGWPR);
	sys_write32(val, config->base_address + reg);
	/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
	k_sleep(K_USEC(35));
}

static int cpg_reset(const struct rcar_mssr_config *config,
		     uint32_t reg, uint32_t bit)
{
	uint16_t offset;
	int ret;

	ret = soc_cpg_get_srcr_offset(reg, &offset);
	if (ret < 0) {
		return ret;
	}

	cpg_write(config, offset, BIT(bit));
	cpg_write(config, SRSTCLR(reg), BIT(bit));

	return 0;
}

static int cpg_mstp_clock_endisable(const struct device *dev,
				    uint32_t module, bool enable)
{
	const struct rcar_mssr_config *config = dev->config;
	uint32_t reg = module / 100;
	uint32_t bit = module % 100;
	uint32_t bitmask = BIT(bit);
	uint32_t reg_val;
	uint16_t offset;
	unsigned int key;
	int ret = 0;

	if (bit > 32) {
		return -EINVAL;
	}

	ret = soc_cpg_get_mstpcr_offset(reg, &offset);
	if (ret < 0) {
		return ret;
	}

	key = irq_lock();

	reg_val = sys_read32(config->base_address + offset);
	if (enable) {
		reg_val &= ~bitmask;
	} else {
		reg_val |= bitmask;
	}

	sys_write32(reg_val, config->base_address + offset);
	if (!enable) {
		ret = cpg_reset(config, reg, bit);
	}

	irq_unlock(key);

	return ret;
}

static int cpg_mssr_blocking_start(const struct device *dev,
				   clock_control_subsys_t sys)
{
	const struct rcar_mssr_config *config = dev->config;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	int ret = -EINVAL;

	if (clk->domain == CPG_MOD) {
		ret = cpg_mstp_clock_endisable(dev, clk->module, true);
	} else if (clk->domain == CPG_CORE) {
		ret = soc_cpg_core_clock_endisable(config->base_address, clk->module,
						   clk->rate, true);
	}

	return ret;
}

static int cpg_mssr_stop(const struct device *dev,
			 clock_control_subsys_t sys)
{
	const struct rcar_mssr_config *config = dev->config;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	int ret = -EINVAL;

	if (clk->domain == CPG_MOD) {
		ret = cpg_mstp_clock_endisable(dev, clk->module, false);
	} else if (clk->domain == CPG_CORE) {
		ret = soc_cpg_core_clock_endisable(config->base_address, clk->module,
						   clk->rate, false);
	}

	return ret;
}

static int cpg_get_rate(const struct device *dev,
			clock_control_subsys_t sys,
			uint32_t *rate)
{
	const struct rcar_mssr_config *config = dev->config;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;

	if (clk->domain != CPG_CORE) {
		return -ENOTSUP;
	}

	return soc_cpg_get_rate(config->base_address, clk->module, rate);
}

static int rcar_cpg_mssr_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api rcar_cpg_mssr_api = {
	.on = cpg_mssr_blocking_start,
	.off = cpg_mssr_stop,
	.get_rate = cpg_get_rate,
};

#define RCAR_MSSR_INIT(inst)					    \
	static struct rcar_mssr_config rcar_mssr##inst##_config = { \
		.base_address = DT_INST_REG_ADDR(inst)		    \
	};							    \
								    \
	DEVICE_DT_INST_DEFINE(inst,				    \
			      &rcar_cpg_mssr_init,		    \
			      NULL,				    \
			      NULL, &rcar_mssr##inst##_config,	    \
			      PRE_KERNEL_1,			    \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,   \
			      &rcar_cpg_mssr_api);

DT_INST_FOREACH_STATUS_OKAY(RCAR_MSSR_INIT)
