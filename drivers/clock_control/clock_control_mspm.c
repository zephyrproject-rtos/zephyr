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

#if defined(SOC_DEVICE_FAMILY_MSPM33_C321X)
#define MSPM_REQUIRES_SYSPLLPARAM2
#endif

#if DT_NODE_EXISTS(MSPM_CPUSS_NODE)
#define MSPM_HAS_CPUSS
#endif

#define MSPM_CLK_WAIT_TIMEOUT_US (100)

#define DT_HFCLK    DT_NODELABEL(hfclk)
#define DT_HFCLK_IN DT_NODELABEL(hfclk_in)
#define DT_HFXT     DT_NODELABEL(hfxt)
#define DT_LFCLK    DT_NODELABEL(lfclk)
#define DT_LFCLK_IN DT_NODELABEL(lfclk_in)
#define DT_LFXT     DT_NODELABEL(lfxt)
#define DT_MCLK     DT_NODELABEL(mclk)
#define DT_MFPCLK   DT_NODELABEL(mfpclk)
#define DT_SYSOSC   DT_NODELABEL(sysosc)
#define DT_SYSPLL   DT_NODELABEL(syspll)
#define DT_ULPCLK   DT_NODELABEL(ulpclk)

#define DT_HFCLK_CLOCKS_CTRL  DT_CLOCKS_CTLR(DT_HFCLK)
#define DT_LFCLK_CLOCKS_CTRL  DT_CLOCKS_CTLR(DT_LFCLK)
#define DT_MCLK_CLOCKS_CTRL   DT_CLOCKS_CTLR(DT_MCLK)
#define DT_MFPCLK_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_MFPCLK)
#define DT_SYSPLL_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_SYSPLL)

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HFCLK) || DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSPLL)
#define MSPM_MCLK_USES_HSCLK 1
#endif

#define MSPM_ULPCLK_DIV DT_PROP_OR(DT_ULPCLK, clk_div, 1)

#define MSPM_MCLK_DIV DT_PROP_OR(DT_MCLK, clk_div, 1)

#define MSPM_MFPCLK_DIV DT_PROP_OR(DT_MFPCLK, clk_div, 1)

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

#if DT_NODE_HAS_STATUS_OKAY(DT_HFCLK)

#define MSPM_HFCLK_ENABLED 1
#define MSPM_HFCLK_FREQ    DT_PROP(DT_HFCLK_CLOCKS_CTRL, clock_frequency)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_HFCLK_CLOCKS_CTRL), "HFCLK source not enabled");

#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)

#define MSPM_HFXT_STARTUP_US DT_PROP_OR(DT_HFXT, ti_xtal_startup_delay_us, 0)
#define MSPM_HFXT_FREQ_MHZ   (MSPM_HFCLK_FREQ / MHZ(1))

#if MSPM_HFXT_FREQ_MHZ >= 4 && MSPM_HFXT_FREQ_MHZ <= 8
#define MSPM_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_4_8_MHZ
#elif MSPM_HFXT_FREQ_MHZ > 8 && MSPM_HFXT_FREQ_MHZ <= 16
#define MSPM_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_8_16_MHZ
#elif MSPM_HFXT_FREQ_MHZ > 16 && MSPM_HFXT_FREQ_MHZ <= 32
#define MSPM_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_16_32_MHZ
#elif MSPM_HFXT_FREQ_MHZ > 32 && MSPM_HFXT_FREQ_MHZ <= 48
#define MSPM_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_32_48_MHZ
#else
#error "HFXT frequency out of supported range (4-48 MHz)"
#endif

#endif /* DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT) */
#endif /* DT_NODE_HAS_STATUS(DT_HFCLK, okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(canclk), okay)
#define MSPM_CANCLK_ENABLED 1
#endif

#if DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT) || DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN)
#define MSPM_LFCLK_USES_EXT 1
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_LFCLK_CLOCKS_CTRL), "LFCLK external source not enabled");
#endif

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
	.clk_freq = MSPM_HFCLK_FREQ,
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

static int clock_mspm_get_rate(const struct device *dev, clock_control_subsys_t sys,
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

#if MSPM_HFCLK_ENABLED
static int clock_mspm_config_hfclk(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

	/* disable HFXT */
	soclock->HSCLKEN &= ~SYSCTL_HSCLKEN_HFXTEN;

#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)
	/* update HFXT frequency range */
	soclock->HFCLKCLKCFG = (soclock->HFCLKCLKCFG & ~SYSCTL_HFCLKCLKCFG_HFXTRSEL) |
			       FIELD_PREP(SYSCTL_HFCLKCLKCFG_HFXTRSEL, MSPM_HFXT_RANGE);

	/* set HFXT startup time */
	soclock->HFCLKCLKCFG = (soclock->HFCLKCLKCFG & ~SYSCTL_HFCLKCLKCFG_HFXTTIME) |
			       SYSCTL_HFCLKCLKCFG_HFXTTIME_VAL(MSPM_HFXT_STARTUP_US);

	/* set HFXT input as HFCLK source */
	soclock->HSCLKEN &= ~SYSCTL_HSCLKEN_USEEXTHFCLK;

	/* enable HFXT */
	soclock->HSCLKEN |= SYSCTL_HSCLKEN_HFXTEN;

	/* enable HFXT startup monitor */
	soclock->HFCLKCLKCFG |= SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK;

	/* wait for HFCLK to stabilize */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_HFCLKGOOD) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

#else
	/* set external digital clock input as HFCLK source */
	soclock->HSCLKEN |= SYSCTL_HSCLKEN_USEEXTHFCLK;
#endif

	return 0;
}
#endif

#if MSPM_LFCLK_USES_EXT
static int clock_mspm_config_lfclk(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

#if DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT)
	/* disable low power mode and set drive strength to lowest */
	soclock->LFCLKCFG &= ~(SYSCTL_LFCLKCFG_LOWCAP | SYSCTL_LFCLKCFG_XT1DRIVE);

	/* start LFXT */
	soclock->LFXTCTL =
		FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) | SYSCTL_LFXTCTL_STARTLFXT;

	/* wait for LFXT to stabilize */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_LFXTGOOD) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

	/* set LFCLK source as LFXT */
	soclock->LFXTCTL =
		FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) | SYSCTL_LFXTCTL_SETUSELFXT;

#elif DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN)
	/* set LFCLK source as LFCLK_IN */
	soclock->EXLFCTL =
		FIELD_PREP(SYSCTL_EXLFCTL_KEY, SYSCTL_EXLFCTL_KEY_VAL) | SYSCTL_EXLFCTL_SETUSEEXLF;
#endif

	return 0;
}
#endif /* MSPM_LFCLK_USES_EXT */

static int clock_mspm_config_mclk(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC) && (DT_SYSOSC_FREQ == 4000000)
	/* configure MDIV */
	soclock->MCLKCFG = (soclock->MCLKCFG & ~SYSCTL_MCLKCFG_MDIV) |
			   FIELD_PREP(SYSCTL_MCLKCFG_MDIV, SYSCTL_MCLKCFG_MDIV_VAL(MSPM_MCLK_DIV));
#endif

	/* configure UDIV */
	soclock->MCLKCFG =
		(soclock->MCLKCFG & ~SYSCTL_MCLKCFG_UDIV) |
		FIELD_PREP(SYSCTL_MCLKCFG_UDIV, SYSCTL_MCLKCFG_UDIV_VAL(mspm_ulpclk_cfg.clk_div));

#if MSPM_MCLK_USES_HSCLK

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HFCLK)
	/* set HFCLK as HSCLK source */
	soclock->HSCLKCFG |= SYSCTL_HSCLKCFG_HSCLKSEL;
#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSPLL)
	/* set SYSPLL as HSCLK source */
	soclock->HSCLKCFG &= ~SYSCTL_HSCLKCFG_HSCLKSEL;
#endif

	/* verify that HSCLK started */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_HSCLKGOOD) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

	/* use HSCLK as MCLK source */
	soclock->MCLKCFG |= SYSCTL_MCLKCFG_USEHSCLK;

	/* verify that MCLK is sourced from HSCLK */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_HSCLKMUX) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}

#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_LFCLK)
	/* leave SYSOSCCFG running and leave it at base frequency */
	soclock->SYSOSCCFG =
		(soclock->SYSOSCCFG & ~(SYSCTL_SYSOSCCFG_FREQ | SYSCTL_SYSOSCCFG_DISABLE)) |
		FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, SYSCTL_SYSOSCCFG_FREQ_BASE);

	/* use LFCLK as MCLK source */
	soclock->MCLKCFG |= SYSCTL_MCLKCFG_USELFCLK;

	/* verify that MCLK is sourced from LFCLK */
	if (!WAIT_FOR((soclock->CLKSTATUS & SYSCTL_CLKSTATUS_CURMCLKSEL) != 0,
		      MSPM_CLK_WAIT_TIMEOUT_US, NULL)) {
		return -ETIMEDOUT;
	}
#endif /* MSPM_MCLK_USES_HSCLK */

	return 0;
}

#if MSPM_MFPCLK_ENABLED
static int clock_mspm_config_mfpclk(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

#if DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_HFCLK)
	/* set MFPCLK divider */
	soclock->GENCLKCFG =
		(soclock->GENCLKCFG & ~SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV) |
		FIELD_PREP(SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV,
			   SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV_VAL(mspm_mfpclk_cfg.clk_div));

	/* set HFCLK as MFPCLK source */
	soclock->GENCLKCFG |= SYSCTL_GENCLKCFG_MFPCLKSRC;
#else
	/* set SYSOSC as MFPCLK source */
	soclock->GENCLKCFG &= ~SYSCTL_GENCLKCFG_MFPCLKSRC;
#endif

	/* enable MFPCLKEN */
	soclock->GENCLKEN |= SYSCTL_GENCLKEN_MFPCLKEN;
	return 0;
}
#endif /* MSPM_MFPCLK_ENABLED */

static int clock_mspm_init(const struct device *dev)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->SOCLOCK;

	/* set SYSOSCFG frequency */
	soclock->SYSOSCCFG = (soclock->SYSOSCCFG & ~SYSCTL_SYSOSCCFG_FREQ) |
			     FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, MSPM_SYSOSC_FREQ);

#if MSPM_SYSPLL_ENABLED
	{
		int ret = clock_mspm_config_syspll();

		if (ret < 0) {
			return ret;
		}
	}
#endif

#if MSPM_HFCLK_ENABLED
	{
		int ret = clock_mspm_config_hfclk();

		if (ret < 0) {
			return ret;
		}
	}
#endif

#if MSPM_LFCLK_USES_EXT
	{
		int ret = clock_mspm_config_lfclk();

		if (ret < 0) {
			return ret;
		}
	}
#endif

	{
		int ret = clock_mspm_config_mclk();

		if (ret < 0) {
			return ret;
		}
	}

#if MSPM_MFPCLK_ENABLED
	{
		int ret = clock_mspm_config_mfpclk();

		if (ret < 0) {
			return ret;
		}
	}
#endif /* MSPM_MFPCLK_ENABLED */

	return 0;
}

static DEVICE_API(clock_control, clock_mspm_driver_api) = {
	.on = clock_mspm_on,
	.off = clock_mspm_off,
	.get_rate = clock_mspm_get_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mspm_driver_api);
