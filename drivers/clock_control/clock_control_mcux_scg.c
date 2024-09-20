/*
 * Copyright (c) 2019-2021 Vestas Wind Systems A/S
 * Copyright 2024 NXP
 *
 * Based on clock_control_mcux_sim.c, which is:
 * Copyright (c) 2017, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_scg

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/kinetis_scg.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_scg);

#define MCUX_SCG_CLOCK_NODE(name) DT_INST_CHILD(0, name)

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
#if !(defined(CONFIG_SOC_MKE17Z7) || defined(CONFIG_SOC_MKE17Z9))
	case KINETIS_SCG_FLEXBUS_CLK:
		clock_name = kCLOCK_FlexBusClk;
		break;
#endif
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
#if (defined(FSL_FEATURE_SCG_HAS_SPLL) && FSL_FEATURE_SCG_HAS_SPLL)
	case KINETIS_SCG_SPLL_CLK:
		clock_name = kCLOCK_ScgSysPllClk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_SPLL) && FSL_FEATURE_SCG_HAS_SPLL) */
#if (defined(FSL_FEATURE_SCG_HAS_LPFLL) && FSL_FEATURE_SCG_HAS_LPFLL)
	case KINETIS_SCG_SPLL_CLK:
		clock_name = kCLOCK_ScgLpFllClk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_LPFLL) && FSL_FEATURE_SCG_HAS_LPFLL) */
#if (defined(FSL_FEATURE_SCG_HAS_SOSCDIV1) && FSL_FEATURE_SCG_HAS_SOSCDIV1)
	case KINETIS_SCG_SOSC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSysOscAsyncDiv1Clk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_SOSCDIV1) && FSL_FEATURE_SCG_HAS_SOSCDIV1) */
	case KINETIS_SCG_SOSC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSysOscAsyncDiv2Clk;
		break;
#if (defined(FSL_FEATURE_SCG_HAS_SIRCDIV1) && FSL_FEATURE_SCG_HAS_SIRCDIV1)
	case KINETIS_SCG_SIRC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSircAsyncDiv1Clk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_SIRCDIV1) && FSL_FEATURE_SCG_HAS_SIRCDIV1) */
	case KINETIS_SCG_SIRC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSircAsyncDiv2Clk;
		break;
#if (defined(FSL_FEATURE_FSL_FEATURE_SCG_HAS_FIRCDIV1) && FSL_FEATURE_SCG_HAS_FIRCDIV1)
	case KINETIS_SCG_FIRC_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgFircAsyncDiv1Clk;
		break;
#endif /* (defined(FSL_FEATURE_FSL_FEATURE_SCG_HAS_FIRCDIV1) && FSL_FEATURE_SCG_HAS_FIRCDIV1) */
	case KINETIS_SCG_FIRC_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgFircAsyncDiv2Clk;
		break;
#if (defined(FSL_FEATURE_SCG_HAS_SPLLDIV1) && FSL_FEATURE_SCG_HAS_SPLLDIV1)
	case KINETIS_SCG_SPLL_ASYNC_DIV1_CLK:
		clock_name = kCLOCK_ScgSysPllAsyncDiv1Clk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_SPLLDIV1) && FSL_FEATURE_SCG_HAS_SPLLDIV1) */
#if (defined(FSL_FEATURE_SCG_HAS_SPLL) && FSL_FEATURE_SCG_HAS_SPLL)
	case KINETIS_SCG_SPLL_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSysPllAsyncDiv2Clk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_SPLL) && FSL_FEATURE_SCG_HAS_SPLL) */
#if (defined(FSL_FEATURE_SCG_HAS_FLLDIV1) && FSL_FEATURE_SCG_HAS_FLLDIV1)
	case KINETIS_SCG_LPFLL_ASYNC_DIV2_CLK:
		clock_name = kCLOCK_ScgSysLPFllAsyncDiv2Clk;
		break;
#endif /* (defined(FSL_FEATURE_SCG_HAS_FLLDIV1) && FSL_FEATURE_SCG_HAS_FLLDIV1) */


	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
	}

	*rate = CLOCK_GetFreq(clock_name);
	return 0;
}

static int mcux_scg_init(const struct device *dev)
{
#if DT_NODE_HAS_STATUS_OKAY(MCUX_SCG_CLOCK_NODE(clkout_clk))
#if DT_SAME_NODE(DT_CLOCKS_CTLR(MCUX_SCG_CLOCK_NODE(clkout_clk)), MCUX_SCG_CLOCK_NODE(slow_clk))
	CLOCK_SetClkOutSel(kClockClkoutSelScgSlow);
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(MCUX_SCG_CLOCK_NODE(clkout_clk)), MCUX_SCG_CLOCK_NODE(sosc_clk))
	CLOCK_SetClkOutSel(kClockClkoutSelSysOsc);
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(MCUX_SCG_CLOCK_NODE(clkout_clk)), MCUX_SCG_CLOCK_NODE(sirc_clk))
	CLOCK_SetClkOutSel(kClockClkoutSelSirc);
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(MCUX_SCG_CLOCK_NODE(clkout_clk)), MCUX_SCG_CLOCK_NODE(firc_clk))
	CLOCK_SetClkOutSel(kClockClkoutSelFirc);
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(MCUX_SCG_CLOCK_NODE(clkout_clk)), MCUX_SCG_CLOCK_NODE(spll_clk))
	CLOCK_SetClkOutSel(kClockClkoutSelSysPll);
#else
#error Unsupported SCG clkout clock source
#endif
#endif /* DT_NODE_HAS_STATUS_OKAY(MCUX_SCG_CLOCK_NODE(clkout_clk)) */

	return 0;
}

static const struct clock_control_driver_api mcux_scg_driver_api = {
	.on = mcux_scg_on,
	.off = mcux_scg_off,
	.get_rate = mcux_scg_get_rate,
};

DEVICE_DT_INST_DEFINE(0,
		    mcux_scg_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &mcux_scg_driver_api);
