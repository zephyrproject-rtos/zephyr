/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <ti/driverlib/driverlib.h>
#include <string.h>

#define ULPCLK_DIV CONCAT(DL_SYSCTL_ULPCLK_DIV_, DT_PROP(DT_NODELABEL(clkmux), uclk_div))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay)
#define MSPM0_PLL_ENABLED 1
static const DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll;
#endif

struct mspm0_clk_cfg {
	bool is_crystal;
	uint32_t xtal_startup_delay;
	uint32_t clk_freq;
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hfclk), okay)
#define MSPM0_HFXT_ENABLED 1
static const struct mspm0_clk_cfg mspm0_cfg_hfclk;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lfxtclk), okay)
#define MSPM0_LFXT_ENABLED 1
static const struct mspm0_clk_cfg mspm0_cfg_lfxtclk;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(clk_out), okay)
#define MSPM0_CLK_OUT_ENABLED 1
struct mspm0_clk_out_cfg {
	const struct pinctrl_dev_config *pinctrl;
	uint32_t source_clk;
};

	PINCTRL_DT_DEFINE(DT_NODELABEL(clk_out));
	struct mspm0_clk_out_cfg clk_out_cfg = {
	.pinctrl = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(clk_out)),
	.source_clk = MSPM0_CLOCK_BUS_ULPCLK,
	};
#endif

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
#if MSPM0_LFXT_ENABLED
		*rate = mspm0_cfg_lfxtclk.clk_freq;
#else
		*rate = 32768;
#endif
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
	struct mspm0_clockSys *clockSys = (struct mspm0_clockSys *)sys;

	if (!rate) {
		return -EINVAL;
	}

	switch (clockSys->bus) {
#if MSPM0_CLK_OUT_ENABLED
	case MSPM0_CLOCK_BUS_CLK_OUT:
		uint32_t clk_reg_value = 0;
		uint8_t divider;
		int *clk_rate = (int *)rate;
		uint32_t ulpclk_rate;

		ulpclk_rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC /
			DT_PROP(DT_NODELABEL(clkmux), uclk_div);

		divider = ulpclk_rate / (*clk_rate);
		divider = (divider / 2) - 1;
		if (divider > 7)
			divider = 7;

		clk_reg_value = divider << 4 |SYSCTL_GENCLKCFG_EXCLKDIVEN_ENABLE;
		DL_SYSCTL_enableExternalClock(DL_SYSCTL_CLK_OUT_SOURCE_ULPCLK, clk_reg_value);
	break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}


static int clock_mspm0_configure(const struct device *dev, clock_control_subsys_t sys, void *data)
{
	return -ENOTSUP;
}

static int clock_mspm0_init(const struct device *dev)
{
	char *mclk_src, *lfclk_src;
	/* setup clocks based on specific rates */
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

#if MSPM0_PLL_ENABLED
	DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&clock_mspm0_cfg_syspll);

	DL_SYSCTL_setULPCLKDivider(ULPCLK_DIV);
	DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
#endif

#if MSPM0_CLK_OUT_ENABLED
	int ret;

	ret = pinctrl_apply_state(clk_out_cfg.pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

#endif

#if MSPM0_HFXT_ENABLED
	uint32_t hf_range;
	if (mspm0_cfg_hfclk.clk_freq >= 4 &&
	    mspm0_cfg_hfclk.clk_freq <= 8 ) {
		hf_range = DL_SYSCTL_HFXT_RANGE_4_8_MHZ;
	} else if (mspm0_cfg_hfclk.clk_freq > 8 &&
		   mspm0_cfg_hfclk.clk_freq <= 16 ) {
		hf_range = DL_SYSCTL_HFXT_RANGE_8_16_MHZ;
	} else if (mspm0_cfg_hfclk.clk_freq > 16 &&
		   mspm0_cfg_hfclk.clk_freq <= 32 ) {
		hf_range = DL_SYSCTL_HFXT_RANGE_16_32_MHZ;
	} else if (mspm0_cfg_hfclk.clk_freq > 32 &&
		   mspm0_cfg_hfclk.clk_freq <= 48 ) {
		hf_range = DL_SYSCTL_HFXT_RANGE_32_48_MHZ;
	} else {
		return -EINVAL;
	}

	/* startup time in 64us resolution */
	DL_SYSCTL_setHFCLKSourceHFXTParams(hf_range,
				 mspm0_cfg_hfclk.xtal_startup_delay / 64,
				 true);
	if (mspm0_cfg_hfclk.is_crystal == false) {
		DL_SYSCTL_setHFCLKSourceHFCLKIN();
	}
#endif

	mclk_src = DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_NODELABEL(clkmux), clock_source, 0));
	if (!mclk_src) {
		return -EINVAL;
	}

	if (!strcmp(mclk_src, "lfosc")) {
		DL_SYSCTL_setMCLKSource(SYSOSC, LFCLK, false);
	} else if (!strcmp(mclk_src, "hfclk")) {
#if MSPM0_HFXT_ENABLED
		DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_HFCLK);
#endif
	}

#if MSPM0_LFXT_ENABLED
	lfclk_src = DT_NODE_FULL_NAME(DT_PHANDLE_BY_IDX(DT_NODELABEL(lfclk), clock_source, 0));
	if (!lfclk_src) {
		return -EINVAL;
	}

	if (!strcmp(lfclk_src, "lfxtclk")) {
		if (mspm0_cfg_lfxtclk.is_crystal == false) {
			DL_SYSCTL_setLFCLKSourceEXLF();
		} else {
			DL_SYSCTL_LFCLKConfig config = {0};

			DL_SYSCTL_setLFCLKSourceLFXT(&config);
		}
	}
#endif

	return 0;
}

static const struct clock_control_driver_api clock_mspm0_driver_api = {
	.on = clock_mspm0_on,
	.off = clock_mspm0_off,
	.get_status = clock_mspm0_get_status,
	.get_rate = clock_mspm0_get_rate,
	.set_rate = clock_mspm0_set_rate,
	.configure = clock_mspm0_configure
};

DEVICE_DT_DEFINE(DT_NODELABEL(clkmux), &clock_mspm0_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mspm0_driver_api);

#if MSPM0_HFXT_ENABLED
static const struct mspm0_clk_cfg mspm0_cfg_hfclk = {
	.is_crystal = DT_NODE_HAS_PROP(DT_NODELABEL(hfclk), ti_xtal),
	.clk_freq = ((DT_PROP(DT_NODELABEL(hfclk), clock_frequency)) / MHZ(1)),
	.xtal_startup_delay = DT_PROP_OR(DT_NODELABEL(hfclk), ti_xtal_startup_delay_us, 0),
};
#endif

#if MSPM0_LFXT_ENABLED
static const struct mspm0_clk_cfg mspm0_cfg_lfxtclk = {
	.is_crystal = DT_NODE_HAS_PROP(DT_NODELABEL(lfxtclk), ti_xtal),
	.clk_freq = DT_PROP(DT_NODELABEL(lfxtclk), clock_frequency),
};
#endif

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
