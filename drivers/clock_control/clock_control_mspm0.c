/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>

#include <ti/driverlib/driverlib.h>
#include <string.h>

#define MSPM0_ULPCLK_DIV COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div),	\
				     (CONCAT(DL_SYSCTL_ULPCLK_DIV_, DT_PROP(DT_NODELABEL(ulpclk), clk_div))), \
				     (0))

#define MSPM0_MCLK_DIV COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(mclk), clk_div),	\
				   (CONCAT(DL_SYSCTL_MCLK_DIVIDER_, DT_PROP(DT_NODELABEL(mclk), clk_div))), \
				   (0))

#define MSPM0_MFPCLK_DIV COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(mfpclk), clk_div),	\
				     (CONCAT(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_, DT_PROP(DT_NODELABEL(mfpclk), clk_div))), \
				     (0))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay)
#define MSPM0_PLL_ENABLED 1
static const DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll;
#endif

struct mspm0_clk_cfg {
	bool is_crystal;
	uint32_t xtal_startup_delay;
	uint32_t clk_freq;
	uint32_t clk_div;
	uint32_t clk_source;
};

static struct mspm0_clk_cfg mspm0_lfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(lfclk), clock_frequency),
	.clk_source = DT_PROP(DT_NODELABEL(lfclk), clock_source),
};

static struct mspm0_clk_cfg mspm0_ulpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(ulpclk), clock_frequency),
	.clk_div = MSPM0_ULPCLK_DIV,
	.clk_source = DT_PROP(DT_NODELABEL(ulpclk), clock_source),
};

static struct mspm0_clk_cfg mspm0_mclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(mclk), clock_frequency),
	.clk_div = MSPM0_MCLK_DIV,
	.clk_source = DT_PROP(DT_NODELABEL(mclk), clock_source),
};

static struct mspm0_clk_cfg mspm0_mfpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(mfpclk), clock_frequency),
	.clk_div = MSPM0_MFPCLK_DIV,
	.clk_source = DT_PROP(DT_NODELABEL(mfpclk), clock_source),
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hfclk), okay)
#define MSPM0_HFCLK_ENABLED 1
static struct mspm0_clk_cfg mspm0_hfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(hfclk), clock_frequency),
	.is_crystal = DT_NODE_HAS_PROP(DT_NODELABEL(hfclk), ti_xtal),
	.xtal_startup_delay = DT_PROP_OR(DT_NODELABEL(hfclk),
					 ti_xtal_startup_delay_us, 0),
};
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(exlfclk), okay)
#define MSPM0_EXLFCLK_ENABLED 1
static const struct mspm0_clk_cfg mspm0_exlfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(exlfclk), clock_frequency),
	.is_crystal = DT_NODE_HAS_PROP(DT_NODELABEL(exlfclk), ti_xtal),
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

static int clock_mspm0_get_rate(const struct device *dev,
				clock_control_subsys_t sys,
				uint32_t *rate)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->bus) {
	case MSPM0_CLOCK_BUS_LFCLK:
		*rate = mspm0_lfclk_cfg.clk_freq;
		break;

	case MSPM0_CLOCK_BUS_ULPCLK:
		*rate = mspm0_ulpclk_cfg.clk_freq;
		break;

	case MSPM0_CLOCK_BUS_MCLK:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		break;

	case MSPM0_CLOCK_BUS_MFPCLK:
		*rate = mspm0_mfpclk_cfg.clk_freq;
		break;

	case MSPM0_CLOCK_BUS_MFCLK:
	case MSPM0_CLOCK_BUS_CANCLK:
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int clock_mspm0_init(const struct device *dev)
{
	/* setup clocks based on specific rates */
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

#if MSPM0_PLL_ENABLED
	DL_SYSCTL_configSYSPLL(
			(DL_SYSCTL_SYSPLLConfig *)&clock_mspm0_cfg_syspll);

	DL_SYSCTL_setMCLKDivider(mspm0_mclk_cfg.clk_div);
	DL_SYSCTL_setULPCLKDivider(mspm0_ulpclk_cfg.clk_div);
	DL_SYSCTL_setHFCLKDividerForMFPCLK(mspm0_mfpclk_cfg.clk_div);

	if (mspm0_mclk_cfg.clk_source == MSPM0_CLOCK_BUS_SYSPLL0 ||
	    mspm0_mclk_cfg.clk_source == MSPM0_CLOCK_BUS_SYSPLL2X) {
		DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK,
					DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
	}
#endif

#if MSPM0_HFCLK_ENABLED
	uint32_t hf_range;
	uint32_t hfclk_freq_in_mhz = mspm0_hfclk_cfg.clk_freq / MHZ(1);
	if (hfclk_freq_in_mhz >= 4 &&
	    hfclk_freq_in_mhz <= 8) {
		hf_range = DL_SYSCTL_HFXT_RANGE_4_8_MHZ;
	} else if (hfclk_freq_in_mhz > 8 &&
		   hfclk_freq_in_mhz <= 16) {
		hf_range = DL_SYSCTL_HFXT_RANGE_8_16_MHZ;
	} else if (hfclk_freq_in_mhz > 16 &&
		   hfclk_freq_in_mhz <= 32) {
		hf_range = DL_SYSCTL_HFXT_RANGE_16_32_MHZ;
	} else if (hfclk_freq_in_mhz > 32 &&
		   hfclk_freq_in_mhz <= 48) {
		hf_range = DL_SYSCTL_HFXT_RANGE_32_48_MHZ;
	} else {
		return -EINVAL;
	}

	if (mspm0_hfclk_cfg.is_crystal == false) {
		DL_SYSCTL_setHFCLKSourceHFCLKIN();
	} else {
		/* startup time in 64us resolution */
		DL_SYSCTL_setHFCLKSourceHFXTParams(hf_range,
					mspm0_hfclk_cfg.xtal_startup_delay / 64,
					true);
	}

	if (mspm0_mclk_cfg.clk_source == MSPM0_CLOCK_BUS_HFCLK) {
		DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK,
					DL_SYSCTL_HSCLK_SOURCE_HFCLK);
	}
#endif

#if MSPM0_EXLFCLK_ENABLED
	if (mspm0_lfclk_cfg.clk_source == MSPM0_CLOCK_BUS_EXLFCLK) {
		if (mspm0_exlfclk_cfg.is_crystal == false) {
			DL_SYSCTL_setLFCLKSourceEXLF();
		} else {
			DL_SYSCTL_LFCLKConfig config = {0};

			DL_SYSCTL_setLFCLKSourceLFXT(&config);
		}
	}
#endif

	if (mspm0_mclk_cfg.clk_source == MSPM0_CLOCK_BUS_LFCLK) {
		DL_SYSCTL_setMCLKSource(SYSOSC, LFCLK, false);
	}

	return 0;
}

static const struct clock_control_driver_api clock_mspm0_driver_api = {
	.on = clock_mspm0_on,
	.off = clock_mspm0_off,
	.get_rate = clock_mspm0_get_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm0_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_mspm0_driver_api);

#if MSPM0_PLL_ENABLED

/* basic checks of the devicetree to follow */
#if (DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk0_div))
#error "Only CLK2X or CLK0 can be enabled at a time on the PLL"
#endif

#define GENERATE_PLL_STRUCT()									\
	static const DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll = {				\
		.inputFreq = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,				\
		.rDivClk2x = (DT_PROP_OR(DT_NODELABEL(pll), clk2x_div, 1) - 1),			\
		.rDivClk1 = (DT_PROP_OR(DT_NODELABEL(pll), clk1_div, 1) - 1),			\
		.rDivClk0 = (DT_PROP_OR(DT_NODELABEL(pll), clk0_div, 1) - 1),			\
		.qDiv = (DT_PROP(DT_NODELABEL(pll), q_div) - 1),				\
		.pDiv = CONCAT(DL_SYSCTL_SYSPLL_PDIV_, DT_PROP(DT_NODELABEL(pll), p_div)),	\
		.sysPLLMCLK = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div),	\
			(DL_SYSCTL_SYSPLL_MCLK_CLK2X), (DL_SYSCTL_SYSPLL_MCLK_CLK0)),		\
		.enableCLK2x = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk2x_div),	\
			(DL_SYSCTL_SYSPLL_CLK2X_ENABLE), (DL_SYSCTL_SYSPLL_CLK2X_DISABLE)),	\
		.enableCLK1 = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk1_div),	\
			(DL_SYSCTL_SYSPLL_CLK1_ENABLE), (DL_SYSCTL_SYSPLL_CLK1_DISABLE)),	\
		.enableCLK0 = COND_CODE_1(DT_NODE_HAS_PROP(DT_NODELABEL(pll), clk0_div),	\
			(DL_SYSCTL_SYSPLL_CLK0_ENABLE), (DL_SYSCTL_SYSPLL_CLK0_DISABLE)),	\
		.sysPLLRef = COND_CODE_1(DT_CLOCKS_CELL(DT_NODELABEL(pll), clocks),		\
			(DL_SYSCTL_SYSPLL_REF_HFCLK), (DL_SYSCTL_SYSPLL_REF_SYSOSC)),		\
	};

GENERATE_PLL_STRUCT()

#endif /* MSPM0_PLL_ENABLED */
