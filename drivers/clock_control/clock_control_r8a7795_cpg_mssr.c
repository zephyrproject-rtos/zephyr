/*
 * Copyright (c) 2022 IoT.bzh
 *
 * r8a7795 Clock Pulse Generator / Module Standby and Software Reset
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_r8a7795_cpg_mssr

#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/dt-bindings/clock/renesas_cpg_mssr.h>
#include <zephyr/dt-bindings/clock/r8a7795_cpg_mssr.h>
#include "clock_control_renesas_cpg_mssr.h"

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_rcar);

struct r8a7795_cpg_mssr_config {
	mm_reg_t base_address;
};

int r8a7795_cpg_core_clock_endisable(uint32_t base_address, uint32_t module,
				     uint32_t rate, bool enable)
{
	uint32_t divider;
	unsigned int key;
	int ret = 0;

	/* Only support CANFD core clock at the moment */
	if (module != R8A7795_CLK_CANFD) {
		return -EINVAL;
	}

	key = irq_lock();

	if (enable) {
		if (rate > 0) {
			if ((CANFDCKCR_PARENT_CLK_RATE % rate) != 0) {
				LOG_ERR("Can not generate %u from CANFD parent clock", rate);
				ret = -EINVAL;
				goto unlock;
			}

			divider = (CANFDCKCR_PARENT_CLK_RATE / rate) - 1;
			if (divider > CANFDCKCR_DIVIDER_MASK) {
				LOG_ERR("Can not generate %u from CANFD parent clock", rate);
				ret = -EINVAL;
				goto unlock;
			}

			rcar_cpg_write(base_address, CANFDCKCR, divider);
		} else {
			LOG_ERR("Can not enable a clock at %u Hz", rate);
			ret = -EINVAL;
		}
	} else {
		rcar_cpg_write(base_address, CANFDCKCR, CANFDCKCR_CKSTP);
	}

unlock:
	irq_unlock(key);
	return ret;
}

int r8a7795_cpg_mssr_start_stop(const struct device *dev, clock_control_subsys_t sys, bool enable)
{
	const struct r8a7795_cpg_mssr_config *config = dev->config;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	int ret = -EINVAL;

	if (clk->domain == CPG_MOD) {
		ret = rcar_cpg_mstp_clock_endisable(config->base_address, clk->module, enable);
	} else if (clk->domain == CPG_CORE) {
		ret = r8a7795_cpg_core_clock_endisable(config->base_address, clk->module, clk->rate,
						       enable);
	}

	return ret;
}

static int r8a7795_cpg_mssr_start(const struct device *dev,
				  clock_control_subsys_t sys)
{
	return r8a7795_cpg_mssr_start_stop(dev, sys, true);
}

static int r8a7795_cpg_mssr_stop(const struct device *dev,
				 clock_control_subsys_t sys)
{
	return r8a7795_cpg_mssr_start_stop(dev, sys, false);
}

static int r8a7795_cpg_get_rate(const struct device *dev,
				clock_control_subsys_t sys,
				uint32_t *rate)
{
	const struct r8a7795_cpg_mssr_config *config = dev->config;
	struct rcar_cpg_clk *clk = (struct rcar_cpg_clk *)sys;
	uint32_t val;
	int ret = 0;

	if (clk->domain != CPG_CORE) {
		return -ENOTSUP;
	}

	switch (clk->module) {
	case R8A7795_CLK_CANFD:
		val = sys_read32(config->base_address + CANFDCKCR);
		if (val & CANFDCKCR_CKSTP) {
			*rate = 0;
		} else {
			val &= CANFDCKCR_DIVIDER_MASK;
			*rate = CANFDCKCR_PARENT_CLK_RATE / (val + 1);
		}
		break;
	case R8A7795_CLK_S3D4:
		*rate = S3D4_CLK_RATE;
		break;
	case R8A7795_CLK_S0D12:
		*rate = S0D12_CLK_RATE;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int r8a7795_cpg_mssr_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct clock_control_driver_api r8a7795_cpg_mssr_api = {
	.on = r8a7795_cpg_mssr_start,
	.off = r8a7795_cpg_mssr_stop,
	.get_rate = r8a7795_cpg_get_rate,
};

#define R8A7795_MSSR_INIT(inst)							  \
	static struct r8a7795_cpg_mssr_config r8a7795_cpg_mssr##inst##_config = { \
		.base_address = DT_INST_REG_ADDR(inst)				  \
	};									  \
										  \
	DEVICE_DT_INST_DEFINE(inst,						  \
			      &r8a7795_cpg_mssr_init,				  \
			      NULL,						  \
			      NULL, &r8a7795_cpg_mssr##inst##_config,		  \
			      PRE_KERNEL_1,					  \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		  \
			      &r8a7795_cpg_mssr_api);

DT_INST_FOREACH_STATUS_OKAY(R8A7795_MSSR_INIT)
