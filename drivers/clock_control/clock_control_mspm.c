/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm_clock_control.h>

#include <soc_cpuss.h>
#include <soc_factoryregion.h>
#include <soc_sysctl.h>

#include <ti/driverlib/driverlib.h>
#include <string.h>

#if defined(SOC_DEVICE_FAMILY_MSPM33_C321X)
#define MSPM_REQUIRES_SYSPLLPARAM2
#endif

#if DT_NODE_EXISTS(MSPM_CPUSS_NODE)
#define MSPM_HAS_CPUSS
#endif

#define MSPM_CLK_WAIT_TIMEOUT_US (100)

#define DT_HFCLK  DT_NODELABEL(hfclk)
#define DT_SYSOSC DT_NODELABEL(sysosc)
#define DT_SYSPLL DT_NODELABEL(syspll)

#define MSPM_ULPCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div),	\
		(CONCAT(DL_SYSCTL_ULPCLK_DIV_,				\
			DT_PROP(DT_NODELABEL(ulpclk), clk_div))),	\
		(0))

#define MSPM_MCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(mclk), clk_div),		\
		(CONCAT(DL_SYSCTL_MCLK_DIVIDER_,			\
			DT_PROP(DT_NODELABEL(mclk), clk_div))),		\
		(0))

#define MSPM_MFPCLK_DIV COND_CODE_1(					\
		DT_NODE_HAS_PROP(DT_NODELABEL(mfpclk), clk_div),	\
		(CONCAT(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_,		\
			DT_PROP(DT_NODELABEL(mfpclk), clk_div))),	\
		(0))

#define DT_SYSOSC_FREQ	DT_PROP(DT_NODELABEL(sysosc), clock_frequency)
#if DT_SYSOSC_FREQ == 32000000
#define MSPM_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_BASE
#elif DT_SYSOSC_FREQ == 4000000
#define MSPM_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_4M
#else
#error "Set SYSOSC clock frequency not supported"
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mfpclk), okay)
#define MSPM_MFPCLK_ENABLED 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(syspll), okay)
#define MSPM_SYSPLL_ENABLED 1
#endif

#if MSPM_SYSPLL_ENABLED

#define MSPM_SYSPLL_PDIV      DT_PROP(DT_SYSPLL, p_div)
#define MSPM_SYSPLL_QDIV      DT_PROP(DT_SYSPLL, q_div)
#define MSPM_SYSPLL_HAS_CLK2X DT_NODE_HAS_PROP(DT_SYSPLL, clk2x_div)
#define MSPM_SYSPLL_CLK2X_DIV DT_PROP_OR(DT_SYSPLL, clk2x_div, 1)
#define MSPM_SYSPLL_HAS_CLK1  DT_NODE_HAS_PROP(DT_SYSPLL, clk1_div)
#define MSPM_SYSPLL_CLK1_DIV  DT_PROP_OR(DT_SYSPLL, clk1_div, 1)
#define MSPM_SYSPLL_HAS_CLK0  DT_NODE_HAS_PROP(DT_SYSPLL, clk0_div)
#define MSPM_SYSPLL_CLK0_DIV  DT_PROP_OR(DT_SYSPLL, clk0_div, 1)

#if MSPM_SYSPLL_HAS_CLK2X && MSPM_SYSPLL_HAS_CLK0
#error "Only CLK2X or CLK0 can be enabled at a time on the SYSPLL"
#endif

BUILD_ASSERT(IS_POWER_OF_TWO(MSPM_SYSPLL_PDIV) && MSPM_SYSPLL_PDIV <= 8,
	     "SYSPLL PDIV must be 1, 2, 4, or 8");

#if DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_HFCLK)
#define MSPM_SYSPLL_REF_FREQ DT_PROP(DT_HFCLK, clock_frequency)
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_SYSOSC)
#define MSPM_SYSPLL_REF_FREQ DT_SYSOSC_FREQ
#else
#error "Invalid SYSPLL source"
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_CLOCKS_CTLR(DT_SYSPLL)), "SYSPLL source not enabled");

#define MSPM_SYSPLL_LOOPIN_FREQ_MHZ ((MSPM_SYSPLL_REF_FREQ / MSPM_SYSPLL_PDIV) / MHZ(1))

#if MSPM_SYSPLL_LOOPIN_FREQ_MHZ >= 4 && MSPM_SYSPLL_LOOPIN_FREQ_MHZ < 8
#define MSPM_SYSPLL_PARAM0_FIELD PLLSTARTUP0_4_8
#define MSPM_SYSPLL_PARAM1_FIELD PLLSTARTUP1_4_8
#elif MSPM_SYSPLL_LOOPIN_FREQ_MHZ >= 8 && MSPM_SYSPLL_LOOPIN_FREQ_MHZ < 16
#define MSPM_SYSPLL_PARAM0_FIELD PLLSTARTUP0_8_16
#define MSPM_SYSPLL_PARAM1_FIELD PLLSTARTUP1_8_16
#elif MSPM_SYSPLL_LOOPIN_FREQ_MHZ >= 16 && MSPM_SYSPLL_LOOPIN_FREQ_MHZ < 32
#define MSPM_SYSPLL_PARAM0_FIELD PLLSTARTUP0_16_32
#define MSPM_SYSPLL_PARAM1_FIELD PLLSTARTUP1_16_32
#elif MSPM_SYSPLL_LOOPIN_FREQ_MHZ >= 32 && MSPM_SYSPLL_LOOPIN_FREQ_MHZ <= 48
#define MSPM_SYSPLL_PARAM0_FIELD PLLSTARTUP0_32_48
#define MSPM_SYSPLL_PARAM1_FIELD PLLSTARTUP1_32_48
#else
#error "SYSPLL fLOOPIN out of supported range (4-48 MHz)"
#endif

#endif /* MSPM_SYSPLL_ENABLED */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hfxt), okay)
#define MSPM_HFCLK_ENABLED 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(canclk), okay)
#define MSPM_CANCLK_ENABLED 1
#endif

#define DT_MCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mclk))
#define DT_LFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(lfclk))
#define DT_HFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(hfclk))
#define DT_MFPCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mfpclk))
#define DT_SYSPLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(syspll))

struct mspm_clk_cfg {
	uint32_t clk_div;
	uint32_t clk_freq;
};

static struct mspm_clk_cfg mspm_lfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(lfclk), clock_frequency),
};

#if MSPM_CANCLK_ENABLED
static struct mspm_clk_cfg mspm_canclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(canclk), clock_frequency),
};
#endif

static struct mspm_clk_cfg mspm_ulpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(ulpclk), clock_frequency),
	.clk_div = MSPM_ULPCLK_DIV,
};

#if MSPM_MFPCLK_ENABLED
static struct mspm_clk_cfg mspm_mfpclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(mfpclk), clock_frequency),
	.clk_div = MSPM_MFPCLK_DIV,
};
#endif

#if MSPM_HFCLK_ENABLED
static struct mspm_clk_cfg mspm_hfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(hfclk), clock_frequency),
};
#endif

static int clock_mspm_on(const struct device *dev, clock_control_subsys_t sys)
{
	return 0;
}

static int clock_mspm_off(const struct device *dev, clock_control_subsys_t sys)
{
	return 0;
}

static int clock_mspm_get_rate(const struct device *dev,
				clock_control_subsys_t sys,
				uint32_t *rate)
{
	struct mspm_sys_clock *sys_clock = (struct mspm_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM_CLOCK_LFCLK:
		*rate = mspm_lfclk_cfg.clk_freq;
		break;

	case MSPM_CLOCK_ULPCLK:
		*rate = mspm_ulpclk_cfg.clk_freq;
		break;

	case MSPM_CLOCK_MCLK:
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		break;

#if MSPM_MFPCLK_ENABLED
	case MSPM_CLOCK_MFPCLK:
		*rate = mspm_mfpclk_cfg.clk_freq;
		break;
#endif

#if MSPM_CANCLK_ENABLED
	case MSPM_CLOCK_CANCLK:
		*rate = mspm_canclk_cfg.clk_freq;
		break;
#endif

#if MSPM_HFCLK_ENABLED
	case MSPM_CLOCK_HFCLK:
		*rate = mspm_hfclk_cfg.clk_freq;
		break;
#endif

	case MSPM_CLOCK_MFCLK:
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if MSPM_SYSPLL_ENABLED
static void clock_mspm_sypll_params(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;
	volatile struct mspm_factoryregion_regs *factoryregion = MSPM_FACTORY_REGS;

#if defined(MSPM_HAS_CPUSS)
	volatile struct mspm_cpuss_regs *cpuss = MSPM_CPUSS_REGS;
	uint32_t old_cpuss_ctl = cpuss->CTL;

	/* disable flash cache before reading tuning registers */
	cpuss->CTL &= ~CPUSS_CTL_ICACHE;
#endif /* MSPM_HAS_CPUSS */

	/* copy PLL tuning params for actual fLOOPIN range */
	soclock->SYSPLLPARAM0 = factoryregion->MSPM_SYSPLL_PARAM0_FIELD;
	soclock->SYSPLLPARAM1 = factoryregion->MSPM_SYSPLL_PARAM1_FIELD;

#if defined(MSPM_REQUIRES_SYSPLLPARAM2)
	soclock->SYSPLLPARAM2 = factoryregion->SYSPLLPARAM2;
	soclock->SYSPLLLDOCTL = factoryregion->SYSPLLLDOCTL;
	soclock->SYSPLLLDOPROG = factoryregion->SYSPLLLDOPROG;
#endif

#if defined(MSPM_HAS_CPUSS)
	/* restore CPUSS flash cache control */
	cpuss->CTL = old_cpuss_ctl;
#endif /* MSPM_HAS_CPUSS */
}

static int clock_mspm_config_syspll(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

	/* disable SYSPLL */
	soclock->HSCLKEN &= ~SYSCTL_HSCLKEN_SYSPLLEN;

	/* wait for SYSPLL to disable */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_SYSPLLOFF) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

#if DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_HFCLK)
	/* set SYSPLL reference to HFCLK */
	soclock->SYSPLLCFG0 |= SYSCTL_SYSPLLCFG0_SYSPLLREF;
#else
	/* set SYSPLL reference to SYSOSC */
	soclock->SYSPLLCFG0 &= ~SYSCTL_SYSPLLCFG0_SYSPLLREF;
#endif

	/* set predivider */
	soclock->SYSPLLCFG1 =
		(soclock->SYSPLLCFG1 & ~SYSCTL_SYSPLLCFG1_PDIV) |
		FIELD_PREP(SYSCTL_SYSPLLCFG1_PDIV, SYSCTL_SYSPLLCFG1_PDIV_VAL(MSPM_SYSPLL_PDIV));

	/* configure SYSPLL parameters from factory constants */
	clock_mspm_sypll_params();

	/* set QDIV */
	soclock->SYSPLLCFG1 =
		(soclock->SYSPLLCFG1 & ~SYSCTL_SYSPLLCFG1_QDIV) |
		FIELD_PREP(SYSCTL_SYSPLLCFG1_QDIV, SYSCTL_SYSPLLCFG1_QDIV_VAL(MSPM_SYSPLL_QDIV));

	/* set syspllclk2x divider */
	if (MSPM_SYSPLL_HAS_CLK2X) {
		soclock->SYSPLLCFG0 =
			(soclock->SYSPLLCFG0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK2X) |
			SYSCTL_SYSPLLCFG0_ENABLECLK2X |
			FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK2X,
				   SYSCTL_SYSPLLCFG0_RDIV_VAL(MSPM_SYSPLL_CLK2X_DIV));
	} else {
		soclock->SYSPLLCFG0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK2X;
	}

	/* set syspllclk1 divider */
	if (MSPM_SYSPLL_HAS_CLK1) {
		soclock->SYSPLLCFG0 = (soclock->SYSPLLCFG0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK1) |
				      SYSCTL_SYSPLLCFG0_ENABLECLK1 |
				      FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK1,
						 SYSCTL_SYSPLLCFG0_RDIV_VAL(MSPM_SYSPLL_CLK1_DIV));
	} else {
		soclock->SYSPLLCFG0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK1;
	}

	/* set syspllclk0 divider */
	if (MSPM_SYSPLL_HAS_CLK0) {
		soclock->SYSPLLCFG0 = (soclock->SYSPLLCFG0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK0) |
				      SYSCTL_SYSPLLCFG0_ENABLECLK0 |
				      FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK0,
						 SYSCTL_SYSPLLCFG0_RDIV_VAL(MSPM_SYSPLL_CLK0_DIV));
	} else {
		soclock->SYSPLLCFG0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK0;
	}

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSPLL)
	if (MSPM_SYSPLL_HAS_CLK2X) {
		soclock->SYSPLLCFG0 |= SYSCTL_SYSPLLCFG0_MCLK2XVCO;
	} else {
		soclock->SYSPLLCFG0 &= ~SYSCTL_SYSPLLCFG0_MCLK2XVCO;
	}
#endif

	/* enable SYSPLL */
	soclock->HSCLKEN |= SYSCTL_HSCLKEN_SYSPLLEN;

	/* wait for SYSPLL to enable */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_SYSPLLGOOD) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}
#endif

static int clock_mspm_init(const struct device *dev)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

	/* set SYSOSCFG frequency */
	soclock->SYSOSCCFG = (soclock->SYSOSCCFG & ~SYSCTL_SYSOSCCFG_FREQ) |
			     FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, MSPM_SYSOSC_FREQ);

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(sysosc)) && (DT_SYSOSC_FREQ == 4000000)
	DL_SYSCTL_setMCLKDivider(MSPM_MCLK_DIV);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div)
	DL_SYSCTL_setULPCLKDivider(mspm_ulpclk_cfg.clk_div);
#endif

#if MSPM_SYSPLL_ENABLED
	{
		int ret = clock_mspm_config_syspll();

		if (ret < 0) {
			return ret;
		}
	}
#endif

#if MSPM_HFCLK_ENABLED
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

#if MSPM_MFPCLK_ENABLED
#if DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_NODELABEL(hfclk))
	DL_SYSCTL_setHFCLKDividerForMFPCLK(mspm_mfpclk_cfg.clk_div);
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_HFCLK);
#else
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);
#endif
	DL_SYSCTL_enableMFPCLK();
#endif /* MSPM_MFPCLK_ENABLED */

	return 0;
}

static DEVICE_API(clock_control, clock_mspm_driver_api) = {
	.on = clock_mspm_on,
	.off = clock_mspm_off,
	.get_rate = clock_mspm_get_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_mspm_driver_api);
