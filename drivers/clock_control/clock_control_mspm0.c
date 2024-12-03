/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>

#include <ti/driverlib/driverlib.h>

#define ULPCLK_DIV CONCAT(DL_SYSCTL_ULPCLK_DIV_, DT_PROP(DT_NODELABEL(clkmux), uclk_div))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay)
#define MSPM0_PLL_ENABLED 1
#endif

static const DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll;

static int clock_mspm0_on(const struct device *dev, clock_control_subsys_t sys)
{
	return 0;
}

static int clock_mspm0_off(const struct device *dev, clock_control_subsys_t sys)
{
	return 0;
}

static enum clock_control_status clock_mspm0_get_status(const struct device *dev,
							clock_control_subsys_t sys)
{
	return CLOCK_CONTROL_STATUS_UNKNOWN;
}

static int clock_mspm0_get_rate(const struct device *dev, clock_control_subsys_t sys,
				uint32_t *rate)
{
	struct mspm0_clockSys *clockSys = (struct mspm0_clockSys *)sys;
	uint8_t rateNotFound = 0;

	switch (clockSys->bus) {
	case MSPM0_CLOCK_BUS_LFCLK:
		*rate = 32768;
		break;
	case MSPM0_CLOCK_BUS_ULPCLK:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC /
			DT_PROP(DT_NODELABEL(clkmux), uclk_div);
		break;
	case MSPM0_CLOCK_BUS_MCLK:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		break;
	case MSPM0_CLOCK_BUS_MFPCLK:
		*rate = 4000000;
		break;
	case MSPM0_CLOCK_BUS_MFCLK:
	case MSPM0_CLOCK_BUS_CANCLK:
	default:
		rateNotFound = 1;
		break;
	}
	if (rateNotFound == 1) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static int clock_mspm0_set_rate(const struct device *dev, clock_control_subsys_t sys,
				clock_control_subsys_rate_t rate)
{
	return -ENOTSUP;
}

static int clock_mspm0_configure(const struct device *dev, clock_control_subsys_t sys, void *data)
{
	return -ENOTSUP;
}

static int clock_mspm0_init(const struct device *dev)
{
	/* setup clocks based on specific rates */
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

	DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&clock_mspm0_cfg_syspll);

	DL_SYSCTL_setULPCLKDivider(ULPCLK_DIV);
	DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);

	return 0;
}

static const struct clock_control_driver_api clock_mspm0_driver_api = {
	.on = clock_mspm0_on,
	.off = clock_mspm0_off,
	.get_status = clock_mspm0_get_status,
	.get_rate = clock_mspm0_get_rate,
	.set_rate = clock_mspm0_set_rate,
	.configure = clock_mspm0_configure};

DEVICE_DT_DEFINE(DT_NODELABEL(clkmux), &clock_mspm0_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mspm0_driver_api);

#if MSPM0_PLL_ENABLED

/* basic checks of the devicetree to follow */
#if (DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk0_div))
#error "Only CLK2X or CLK0 can be enabled at a time on the PLL"
#endif

#define GENERATE_PLL_STRUCT()                                                                      \
	static const DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll = {        \
		.inputFreq = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,               \
		.rDivClk2x = (DT_PROP_OR(DT_NODELABEL(pll), clk2x_div, 1) - 1),   \
		.rDivClk1 = (DT_PROP_OR(DT_NODELABEL(pll), clk1_div, 1) - 1),     \
		.rDivClk0 = (DT_PROP_OR(DT_NODELABEL(pll), clk0_div, 1) - 1),     \
		.qDiv = (DT_PROP(DT_NODELABEL(pll), q_div) - 1),                  \
		.pDiv = CONCAT(DL_SYSCTL_SYSPLL_PDIV_, DT_PROP(DT_NODELABEL(pll), p_div)), \
		.sysPLLMCLK = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div),  \
			(DL_SYSCTL_SYSPLL_MCLK_CLK2X), (DL_SYSCTL_SYSPLL_MCLK_CLK0)),          \
		.enableCLK2x = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div), \
			(DL_SYSCTL_SYSPLL_CLK2X_ENABLE), (DL_SYSCTL_SYSPLL_CLK2X_DISABLE)),  \
		.enableCLK1 = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk1_div), \
			(DL_SYSCTL_SYSPLL_CLK1_ENABLE), (DL_SYSCTL_SYSPLL_CLK1_DISABLE)),    \
		.enableCLK0 = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk0_div), \
			(DL_SYSCTL_SYSPLL_CLK0_ENABLE), (DL_SYSCTL_SYSPLL_CLK0_DISABLE)),    \
		.sysPLLRef = COND_CODE_1(DT_CLOCKS_CELL(DT_NODELABEL(pll), clocks), \
			(DL_SYSCTL_SYSPLL_REF_HFCLK), (DL_SYSCTL_SYSPLL_REF_SYSOSC)),   \
	};

GENERATE_PLL_STRUCT()

#endif /* MSPM0_PLL_ENABLED */
