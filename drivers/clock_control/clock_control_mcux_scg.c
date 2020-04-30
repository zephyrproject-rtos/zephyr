/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on clock_control_mcux_sim.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_scg

#include <drivers/clock_control.h>
#include <dt-bindings/clock/kinetis_scg.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control_scg);

static int mcux_scg_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_scg_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_scg_get_rate(const struct device *dev,
			     clock_control_subsys_t sub_system,
			     uint32_t *rate)
{
	clock_name_t clock_name;

	switch ((uint32_t) sub_system) {
	case KINETIS_SCG_CORESYS_CLK:
		clock_name = kCLOCK_CoreSysClk;
		break;
	case KINETIS_SCG_BUS_CLK:
		clock_name = kCLOCK_BusClk;
		break;
	case KINETIS_SCG_FLEXBUS_CLK:
		clock_name = kCLOCK_FlexBusClk;
		break;
	case KINETIS_SCG_FLASH_CLK:
		clock_name = kCLOCK_FlashClk;
		break;
	case KINETIS_SCG_SOSC_CLK:
		clock_name = kCLOCK_ScgSysOscClk;
		break;
	case KINETIS_SCG_SIRC_CLK:
		clock_name = kCLOCK_ScgSircClk;
		break;
	case KINETIS_SCG_FIRC_CLK:
		clock_name = kCLOCK_ScgFircClk;
		break;
	case KINETIS_SCG_SPLL_CLK:
		clock_name = kCLOCK_ScgSysPllClk;
		break;
	case KINETIS_SCG_SOSC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSysOscAsyncDiv1Clk;
		break;
	case KINETIS_SCG_SOSC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSysOscAsyncDiv2Clk;
		break;
	case KINETIS_SCG_SIRC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSircAsyncDiv1Clk;
		break;
	case KINETIS_SCG_SIRC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSircAsyncDiv2Clk;
		break;
	case KINETIS_SCG_FIRC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgFircAsyncDiv1Clk;
		break;
	case KINETIS_SCG_FIRC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgFircAsyncDiv2Clk;
		break;
	case KINETIS_SCG_SPLL_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSysPllAsyncDiv1Clk;
		break;
	case KINETIS_SCG_SPLL_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSysPllAsyncDiv2Clk;
		break;
	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
	}

	*rate = CLOCK_GetFreq(clock_name);
	return 0;
}

static int mcux_scg_init(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, clkout_source)
	CLOCK_SetClkOutSel(DT_INST_PROP(0, clkout_source));
#endif

	return 0;
}

static const struct clock_control_driver_api mcux_scg_driver_api = {
	.on = mcux_scg_on,
	.off = mcux_scg_off,
	.get_rate = mcux_scg_get_rate,
};

DEVICE_AND_API_INIT(mcux_scg, DT_INST_LABEL(0),
		    &mcux_scg_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_scg_driver_api);
