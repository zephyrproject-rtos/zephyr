/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_rz_cpg.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT renesas_rz_cpg

static int clock_control_renesas_rz_on(const struct device *dev, clock_control_subsys_t sys)
{

	struct clock_control_rz_subsys_cfg *subsys_clk = (struct clock_control_rz_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}

	switch (subsys_clk->ip) {
	case FSP_IP_GTM:
		R_BSP_MODULE_START(FSP_IP_GTM, subsys_clk->channel);
		break;
	case FSP_IP_GPT:
		R_BSP_MODULE_START(FSP_IP_GPT, subsys_clk->channel);
		break;
	case FSP_IP_POEG:
		R_BSP_MODULE_START(FSP_IP_POEG, subsys_clk->channel);
		break;
	case FSP_IP_SCIF:
		R_BSP_MODULE_START(FSP_IP_SCIF, subsys_clk->channel);
		break;
	case FSP_IP_RIIC:
		R_BSP_MODULE_START(FSP_IP_RIIC, subsys_clk->channel);
		break;
	case FSP_IP_RSPI:
		R_BSP_MODULE_START(FSP_IP_RSPI, subsys_clk->channel);
		break;
	case FSP_IP_MHU:
		R_BSP_MODULE_START(FSP_IP_MHU, subsys_clk->channel);
		break;
	case FSP_IP_DMAC:
		R_BSP_MODULE_START(FSP_IP_DMAC, subsys_clk->channel);
		break;
	case FSP_IP_SSI:
		R_BSP_MODULE_START(FSP_IP_SSI, subsys_clk->channel);
		break;
	case FSP_IP_CANFD:
		R_BSP_MODULE_START(FSP_IP_CANFD, subsys_clk->channel);
		break;
	case FSP_IP_ADC:
		R_BSP_MODULE_START(FSP_IP_ADC, subsys_clk->channel);
		break;
	case FSP_IP_TSU:
		R_BSP_MODULE_START(FSP_IP_TSU, subsys_clk->channel);
		break;
	case FSP_IP_WDT:
		R_BSP_MODULE_START(FSP_IP_WDT, subsys_clk->channel);
		break;
	case FSP_IP_SCI:
		R_BSP_MODULE_START(FSP_IP_SCI, subsys_clk->channel);
		break;
	case FSP_IP_XSPI:
		R_BSP_MODULE_START(FSP_IP_XSPI, subsys_clk->channel);
		break;
	default:
		return -EINVAL; /* Invalid FSP IP Module */
	}

	return 0;
}

static int clock_control_renesas_rz_off(const struct device *dev, clock_control_subsys_t sys)
{

	struct clock_control_rz_subsys_cfg *subsys_clk = (struct clock_control_rz_subsys_cfg *)sys;

	if (!dev || !sys) {
		return -EINVAL;
	}

	switch (subsys_clk->ip) {
	case FSP_IP_GTM:
		R_BSP_MODULE_STOP(FSP_IP_GTM, subsys_clk->channel);
		break;
	case FSP_IP_GPT:
		R_BSP_MODULE_STOP(FSP_IP_GPT, subsys_clk->channel);
		break;
	case FSP_IP_POEG:
		R_BSP_MODULE_STOP(FSP_IP_POEG, subsys_clk->channel);
		break;
	case FSP_IP_SCIF:
		R_BSP_MODULE_STOP(FSP_IP_SCIF, subsys_clk->channel);
		break;
	case FSP_IP_RIIC:
		R_BSP_MODULE_STOP(FSP_IP_RIIC, subsys_clk->channel);
		break;
	case FSP_IP_RSPI:
		R_BSP_MODULE_STOP(FSP_IP_RSPI, subsys_clk->channel);
		break;
	case FSP_IP_MHU:
		R_BSP_MODULE_STOP(FSP_IP_MHU, subsys_clk->channel);
		break;
	case FSP_IP_DMAC:
		R_BSP_MODULE_STOP(FSP_IP_DMAC, subsys_clk->channel);
		break;
	case FSP_IP_SSI:
		R_BSP_MODULE_STOP(FSP_IP_SSI, subsys_clk->channel);
		break;
	case FSP_IP_CANFD:
		R_BSP_MODULE_STOP(FSP_IP_CANFD, subsys_clk->channel);
		break;
	case FSP_IP_ADC:
		R_BSP_MODULE_STOP(FSP_IP_ADC, subsys_clk->channel);
		break;
	case FSP_IP_TSU:
		R_BSP_MODULE_STOP(FSP_IP_TSU, subsys_clk->channel);
		break;
	case FSP_IP_WDT:
		R_BSP_MODULE_STOP(FSP_IP_WDT, subsys_clk->channel);
		break;
	case FSP_IP_SCI:
		R_BSP_MODULE_STOP(FSP_IP_SCI, subsys_clk->channel);
		break;
	case FSP_IP_XSPI:
		R_BSP_MODULE_STOP(FSP_IP_XSPI, subsys_clk->channel);
		break;
	default:
		return -EINVAL; /* Invalid */
	}
	return 0;
}

static int clock_control_renesas_rz_get_rate(const struct device *dev, clock_control_subsys_t sys,
					     uint32_t *rate)
{
	struct clock_control_rz_subsys_cfg *subsys_clk = (struct clock_control_rz_subsys_cfg *)sys;

	if (!dev || !sys || !rate) {
		return -EINVAL;
	}

	uint32_t clock_source = R_FSP_SystemClockHzGet(subsys_clk->clk_src);
	*rate = clock_source;
	return 0;
}

static DEVICE_API(clock_control, rz_clock_control_driver_api) = {
	.on = clock_control_renesas_rz_on,
	.off = clock_control_renesas_rz_off,
	.get_rate = clock_control_renesas_rz_get_rate,
};

static int clock_control_rz_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

DEVICE_DT_INST_DEFINE(0, clock_control_rz_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &rz_clock_control_driver_api);
