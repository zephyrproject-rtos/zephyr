/*
 * Copyright 2023 NXP
 *
 * Based on clock_control_mcux_scg.c, which is:
 * Copyright (c) 2019-2021 Vestas Wind Systems A/S
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_scg_k4

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/scg_k4.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_scg);

#define MCUX_SCG_CLOCK_NODE(name) DT_INST_CHILD(0, name)

static int mcux_scg_k4_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_scg_k4_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_scg_k4_get_rate(const struct device *dev, clock_control_subsys_t sub_system,
				uint32_t *rate)
{
	clock_name_t clock_name;

	switch ((uint32_t)sub_system) {
	case SCG_K4_CORESYS_CLK:
		clock_name = kCLOCK_CoreSysClk;
		break;
	case SCG_K4_SLOW_CLK:
		clock_name = kCLOCK_SlowClk;
		break;
	case SCG_K4_PLAT_CLK:
		clock_name = kCLOCK_PlatClk;
		break;
	case SCG_K4_SYS_CLK:
		clock_name = kCLOCK_SysClk;
		break;
	case SCG_K4_BUS_CLK:
		clock_name = kCLOCK_BusClk;
		break;
	case SCG_K4_SYSOSC_CLK:
		clock_name = kCLOCK_ScgSysOscClk;
		break;
	case SCG_K4_SIRC_CLK:
		clock_name = kCLOCK_ScgSircClk;
		break;
	case SCG_K4_FIRC_CLK:
		clock_name = kCLOCK_ScgFircClk;
		break;
	case SCG_K4_RTCOSC_CLK:
		clock_name = kCLOCK_RtcOscClk;
		break;
	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
	}

	*rate = CLOCK_GetFreq(clock_name);
	return 0;
}

static DEVICE_API(clock_control, mcux_scg_driver_api) = {
	.on = mcux_scg_k4_on,
	.off = mcux_scg_k4_off,
	.get_rate = mcux_scg_k4_get_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mcux_scg_driver_api);
