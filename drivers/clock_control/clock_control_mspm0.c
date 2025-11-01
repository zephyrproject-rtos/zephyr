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

#define MSPM0_ULPCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div),	\
		(CONCAT(DL_SYSCTL_ULPCLK_DIV_,				\
			DT_PROP(DT_NODELABEL(ulpclk), clk_div))),	\
		(0))

#define MSPM0_MCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(mclk), clk_div),		\
		(CONCAT(DL_SYSCTL_MCLK_DIVIDER_,			\
			DT_PROP(DT_NODELABEL(mclk), clk_div))),		\
		(0))

#define MSPM0_MFPCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(mfpclk), clk_div),	\
		(CONCAT(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_,		\
			DT_PROP(DT_NODELABEL(mfpclk), clk_div))),	\
		(0))

#define DT_SYSOSC_FREQ	DT_PROP(DT_NODELABEL(sysosc), clock_frequency)
#if DT_SYSOSC_FREQ == 32000000
#define SYSOSC_FREQ	DL_SYSCTL_SYSOSC_FREQ_BASE
#elif DT_SYSOSC_FREQ == 4000000
#define SYSOSC_FREQ	DL_SYSCTL_SYSOSC_FREQ_4M
#else
#error "Set SYSOSC clock frequency not supported"
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mfpclk), okay)
#define MSPM0_MFPCLK_ENABLED 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(syspll), okay)
#define MSPM0_SYSPLL_ENABLED 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hfxt), okay)
#define MSPM0_HFCLK_ENABLED 1
#endif

#define DT_MCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mclk))
#define DT_LFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(lfclk))
#define DT_HFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(hfclk))
#define DT_MFPCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mfpclk))
#define DT_SYSPLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(syspll))

struct mspm0_clk_cfg {
	uint32_t clk_div;
	uint32_t clk_freq;
};

static struct mspm0_clk_cfg mspm0_lfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(lfclk), clock_frequency),
};

static struct mspm0_clk_cfg mspm0_ulpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(ulpclk), clock_frequency),
	.clk_div = MSPM0_ULPCLK_DIV,
};

#if MSPM0_MFPCLK_ENABLED
static struct mspm0_clk_cfg mspm0_mfpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(mfpclk), clock_frequency),
	.clk_div = MSPM0_MFPCLK_DIV,
};
#endif

#if MSPM0_SYSPLL_ENABLED
/* basic checks of the devicetree to follow */
#if (DT_NODE_HAS_PROP(DT_NODELABEL(syspll), clk2x_div) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(syspll), clk0_div))
#error "Only CLK2X or CLK0 can be enabled at a time on the SYSPLL"
#endif

static DL_SYSCTL_SYSPLLConfig clock_mspm0_cfg_syspll = {
	.inputFreq = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,
	.sysPLLMCLK = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
	.sysPLLRef = DL_SYSCTL_SYSPLL_REF_SYSOSC,
	.rDivClk2x = (DT_PROP_OR(DT_NODELABEL(syspll), clk2x_div, 1) - 1),
	.rDivClk1 = (DT_PROP_OR(DT_NODELABEL(syspll), clk1_div, 1) - 1),
	.rDivClk0 = (DT_PROP_OR(DT_NODELABEL(syspll), clk0_div, 1) - 1),
	.qDiv = (DT_PROP(DT_NODELABEL(syspll), q_div) - 1),
	.pDiv = CONCAT(DL_SYSCTL_SYSPLL_PDIV_,
		       DT_PROP(DT_NODELABEL(syspll), p_div)),
	.enableCLK2x = COND_CODE_1(
		DT_NODE_HAS_PROP(DT_NODELABEL(syspll), clk2x_div),
		(DL_SYSCTL_SYSPLL_CLK2X_ENABLE),
		(DL_SYSCTL_SYSPLL_CLK2X_DISABLE)),
	.enableCLK1 = COND_CODE_1(
		DT_NODE_HAS_PROP(DT_NODELABEL(syspll), clk1_div),
		(DL_SYSCTL_SYSPLL_CLK1_ENABLE),
		(DL_SYSCTL_SYSPLL_CLK1_DISABLE)),
	.enableCLK0 = COND_CODE_1(
		DT_NODE_HAS_PROP(DT_NODELABEL(syspll), clk0_div),
		(DL_SYSCTL_SYSPLL_CLK0_ENABLE),
		(DL_SYSCTL_SYSPLL_CLK0_DISABLE)),
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

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_LFCLK:
		*rate = mspm0_lfclk_cfg.clk_freq;
		break;

	case MSPM0_CLOCK_ULPCLK:
		*rate = mspm0_ulpclk_cfg.clk_freq;
		break;

	case MSPM0_CLOCK_MCLK:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		break;

#if MSPM0_MFPCLK_ENABLED
	case MSPM0_CLOCK_MFPCLK:
		*rate = mspm0_mfpclk_cfg.clk_freq;
		break;
#endif

	case MSPM0_CLOCK_MFCLK:
	case MSPM0_CLOCK_CANCLK:
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int clock_mspm0_init(const struct device *dev)
{
	/* setup clocks based on specific rates */
	DL_SYSCTL_setSYSOSCFreq(SYSOSC_FREQ);

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(sysosc)) && (DT_SYSOSC_FREQ == 4000000)
	DL_SYSCTL_setMCLKDivider(MSPM0_MCLK_DIV);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div)
	DL_SYSCTL_setULPCLKDivider(mspm0_ulpclk_cfg.clk_div);
#endif

#if MSPM0_SYSPLL_ENABLED
#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(syspll))
	clock_mspm0_cfg_syspll.sysPLLMCLK = DL_SYSCTL_SYSPLL_MCLK_CLK0;
#endif
#if DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_NODELABEL(hfclk))
	clock_mspm0_cfg_syspll.sysPLLRef = DL_SYSCTL_SYSPLL_REF_HFCLK;
#endif
	DL_SYSCTL_configSYSPLL(
			(DL_SYSCTL_SYSPLLConfig *)&clock_mspm0_cfg_syspll);
#endif

#if MSPM0_HFCLK_ENABLED
#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_NODELABEL(hfxt))
	uint32_t hf_range;
	uint32_t hfxt_freq = DT_PROP(DT_NODELABEL(hfxt),
				     clock_frequency)  / MHZ(1);
	uint32_t xtal_startup_delay = DT_PROP_OR(DT_NODELABEL(hfxt),
						 ti_xtal_startup_delay_us, 0);

	if (hfxt_freq >= 4 &&
	    hfxt_freq <= 8) {
		hf_range = DL_SYSCTL_HFXT_RANGE_4_8_MHZ;
	} else if (hfxt_freq > 8 &&
		   hfxt_freq <= 16) {
		hf_range = DL_SYSCTL_HFXT_RANGE_8_16_MHZ;
	} else if (hfxt_freq > 16 &&
		   hfxt_freq <= 32) {
		hf_range = DL_SYSCTL_HFXT_RANGE_16_32_MHZ;
	} else if (hfxt_freq > 32 &&
		   hfxt_freq <= 48) {
		hf_range = DL_SYSCTL_HFXT_RANGE_32_48_MHZ;
	} else {
		return -EINVAL;
	}

	/* startup time in 64us resolution */
	DL_SYSCTL_setHFCLKSourceHFXTParams(hf_range,
					   xtal_startup_delay / 64,
					   true);
#else
	DL_SYSCTL_setHFCLKSourceHFCLKIN();
#endif
#endif

#if DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_NODELABEL(lfxt))
	DL_SYSCTL_LFCLKConfig config = {0};

	DL_SYSCTL_setLFCLKSourceLFXT(&config);
#elif DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_NODELABEL(lfdig_in))
	DL_SYSCTL_setLFCLKSourceEXLF();

#endif

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(hfclk))
	DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK,
				DL_SYSCTL_HSCLK_SOURCE_HFCLK);

#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(syspll))
	DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK,
				DL_SYSCTL_HSCLK_SOURCE_SYSPLL);

#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(lfclk))
	DL_SYSCTL_setMCLKSource(SYSOSC, LFCLK, false);

#endif /* DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(hfclk)) */

#if MSPM0_MFPCLK_ENABLED
#if DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_NODELABEL(hfclk))
	DL_SYSCTL_setHFCLKDividerForMFPCLK(mspm0_mfpclk_cfg.clk_div);
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_HFCLK);
#else
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);
#endif
	DL_SYSCTL_enableMFPCLK();
#endif /* MSPM0_MFPCLK_ENABLED */

	return 0;
}

static DEVICE_API(clock_control, clock_mspm0_driver_api) = {
	.on = clock_mspm0_on,
	.off = clock_mspm0_off,
	.get_rate = clock_mspm0_get_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm0_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_mspm0_driver_api);
