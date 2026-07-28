/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/logging/log.h>

#include <soc_cpuss.h>
#include <soc_factoryregion.h>
#include <soc_sysctl.h>

LOG_MODULE_REGISTER(clock_control_mspm0, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <ti/driverlib/driverlib.h>
#include <string.h>

#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define MSPM0_REQUIRES_SYSPLLPARAM2
#endif

#if DT_NODE_EXISTS(MSPM_CPUSS_NODE)
#define MSPM0_HAS_CPUSS
#endif

#define MSPM0_CLK_WAIT_TIMEOUT_US 1000

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

#define DT_SYSOSC DT_NODELABEL(sysosc)

#define DT_SYSOSC_FREQ DT_PROP(DT_SYSOSC, clock_frequency)
#if DT_SYSOSC_FREQ == 32000000
#define MSPM0_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_BASE
#elif DT_SYSOSC_FREQ == 4000000
#define MSPM0_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_4M
#else
#error "Set SYSOSC clock frequency not supported"
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mfpclk), okay)
#define MSPM0_MFPCLK_ENABLED 1
#endif

#define DT_HFCLK  DT_NODELABEL(hfclk)
#define DT_SYSPLL DT_NODELABEL(syspll)

#define DT_SYSPLL_OKAY DT_NODE_HAS_STATUS_OKAY(DT_SYSPLL)

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hfxt), okay)
#define MSPM0_HFCLK_ENABLED 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(canclk), okay)
#define MSPM0_CANCLK_ENABLED 1
#endif

#define DT_MCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mclk))
#define DT_LFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(lfclk))
#define DT_HFCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(hfclk))
#define DT_MFPCLK_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_NODELABEL(mfpclk))
#define DT_SYSPLL_CLOCKS_CTRL	DT_CLOCKS_CTLR(DT_SYSPLL)

/* System PLL */
#if DT_SYSPLL_OKAY

#define MSPM0_SYSPLL_PDIV      DT_PROP(DT_SYSPLL, p_div)
#define MSPM0_SYSPLL_QDIV      DT_PROP(DT_SYSPLL, q_div)
#define MSPM0_SYSPLL_HAS_CLK2X DT_NODE_HAS_PROP(DT_SYSPLL, clk2x_div)
#define MSPM0_SYSPLL_CLK2X_DIV DT_PROP_OR(DT_SYSPLL, clk2x_div, 1)
#define MSPM0_SYSPLL_HAS_CLK1  DT_NODE_HAS_PROP(DT_SYSPLL, clk1_div)
#define MSPM0_SYSPLL_CLK1_DIV  DT_PROP_OR(DT_SYSPLL, clk1_div, 1)
#define MSPM0_SYSPLL_HAS_CLK0  DT_NODE_HAS_PROP(DT_SYSPLL, clk0_div)
#define MSPM0_SYSPLL_CLK0_DIV  DT_PROP_OR(DT_SYSPLL, clk0_div, 1)

#if MSPM0_SYSPLL_HAS_CLK2X && MSPM0_SYSPLL_HAS_CLK0
#error "Only CLK2X or CLK0 can be enabled at a time on the SYSPLL"
#endif

#if MSPM0_SYSPLL_QDIV <= 1
#error "Divide-by-one is not a valid QDIV option"
#endif

#if !DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_HFCLK) &&                                          \
	!DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_SYSOSC)
#error "Invalid SYSPLL source"
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_CLOCKS_CTLR(DT_SYSPLL)), "SYSPLL source not enabled");

#endif /* DT_SYSPLL_OKAY */

#if DT_SYSPLL_OKAY
static int clock_mspm0_syspll_output_rate(const struct device *dev, uint32_t clk, uint32_t *rate);
#endif /* DT_SYSPLL_OKAY */

struct mspm0_clk_cfg {
	uint32_t clk_div;
	uint32_t clk_freq;
};

static struct mspm0_clk_cfg mspm0_lfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(lfclk), clock_frequency),
};

#if MSPM0_CANCLK_ENABLED
static struct mspm0_clk_cfg mspm0_canclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(canclk), clock_frequency),
};
#endif

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

#if MSPM0_HFCLK_ENABLED
static struct mspm0_clk_cfg mspm0_hfclk_cfg = {
	.clk_freq = DT_PROP(DT_NODELABEL(hfclk), clock_frequency),
};
#endif


/* Only 32/4 MHz supported; 16/24 MHz needs board trim we can't source. */
static int clock_mspm0_set_rate(const struct device *dev, clock_control_subsys_t sys,
				clock_control_subsys_rate_t rate)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	uint32_t freq = *(uint32_t *)rate;

	if (sys_clock->clk != MSPM0_CLOCK_SYSOSC) {
		return -ENOTSUP;
	}

	if (soclock->clkstatus & (SYSCTL_CLKSTATUS_CURMCLKSEL | SYSCTL_CLKSTATUS_HSCLKMUX)) {
		return -EBUSY;
	}

	switch (freq) {
	case MHZ(32):
		soclock->sysosccfg = (soclock->sysosccfg & ~SYSCTL_SYSOSCCFG_FREQ) |
				     FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, SYSCTL_SYSOSCCFG_FREQ_BASE);
		break;

	case MHZ(4):
		soclock->sysosccfg = (soclock->sysosccfg & ~SYSCTL_SYSOSCCFG_FREQ) |
				     FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, SYSCTL_SYSOSCCFG_FREQ_4M);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int clock_mspm0_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		soclock->sysosccfg &= ~SYSCTL_SYSOSCCFG_DISABLE;
		return 0;

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL: {
		struct mspm0_sys_clock sysosc_subsys = {.clk = MSPM0_CLOCK_SYSOSC};
		uint32_t rate = MHZ(32);
		int ret = clock_mspm0_set_rate(dev, &sysosc_subsys, &rate);

		if (ret < 0) {
			return ret;
		}

		soclock->hsclken |= SYSCTL_HSCLKEN_SYSPLLEN;
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_SYSPLLGOOD) != 0,
			      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
			LOG_ERR("timed out waiting for SYSPLL to start");
			return -ETIMEDOUT;
		}
		return 0;
	}
#endif /* DT_SYSPLL_OKAY */

	default:
		return -ENOTSUP;
	}
}

static int clock_mspm0_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		soclock->sysosccfg |= SYSCTL_SYSOSCCFG_DISABLE;
		return 0;

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL: {
		soclock->hsclken &= ~SYSCTL_HSCLKEN_SYSPLLEN;
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_SYSPLLOFF) != 0,
			      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
			LOG_ERR("timed out waiting for SYSPLL to stop");
			return -ETIMEDOUT;
		}
		return 0;
	}
#endif /* DT_SYSPLL_OKAY */

	default:
		return -ENOTSUP;
	}
}

static enum clock_control_status clock_mspm0_get_status(const struct device *dev,
							clock_control_subsys_t sys)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		if (soclock->sysosccfg & SYSCTL_SYSOSCCFG_DISABLE) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		return CLOCK_CONTROL_STATUS_ON;

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL:
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_SYSPLLOFF) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_SYSPLLGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
#endif /* DT_SYSPLL_OKAY */

	default:
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

static uint32_t clock_mspm0_sysosc_rate(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	uint32_t freq_field = FIELD_GET(SYSCTL_SYSOSCCFG_FREQ, soclock->sysosccfg);

	if (freq_field == SYSCTL_SYSOSCCFG_FREQ_4M) {
		return MHZ(4);
	}

#if defined(MSPM_SYSCTL_HAS_SYSOSC_USERTRIM)
	if (freq_field == SYSCTL_SYSOSCCFG_FREQ_USERTRIM) {
		if (FIELD_GET(SYSCTL_SYSOSCTRIMUSER_FREQ, soclock->sysosctrimuser) ==
		    SYSCTL_SYSOSCTRIMUSER_FREQ_24M) {
			return MHZ(24);
		}

		return MHZ(16);
	}
#endif /* MSPM_SYSCTL_HAS_SYSOSC_USERTRIM */

	return MHZ(32);
}

static int clock_mspm0_get_rate(const struct device *dev,
				clock_control_subsys_t sys,
				uint32_t *rate)
{
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		*rate = clock_mspm0_sysosc_rate();
		break;

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

#if MSPM0_CANCLK_ENABLED
	case MSPM0_CLOCK_CANCLK:
		*rate = mspm0_canclk_cfg.clk_freq;
		break;
#endif

#if MSPM0_HFCLK_ENABLED
	case MSPM0_CLOCK_HFCLK:
		*rate = mspm0_hfclk_cfg.clk_freq;
		break;
#endif

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL_CLK0:
	case MSPM0_CLOCK_SYSPLL_CLK1:
	case MSPM0_CLOCK_SYSPLL_CLK2X: {
		int ret = clock_mspm0_syspll_output_rate(dev, sys_clock->clk, rate);

		if (ret < 0) {
			return ret;
		}
		break;
	}
#endif /* DT_SYSPLL_OKAY */

	case MSPM0_CLOCK_MFCLK:
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if DT_SYSPLL_OKAY
static int clock_mspm0_syspll_ref_rate(const struct device *dev, uint32_t *rate)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;

	if (soclock->syspllcfg0 & SYSCTL_SYSPLLCFG0_SYSPLLREF) {
#if MSPM0_HFCLK_ENABLED
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};

		return clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys, rate);
#else
		return -ENOTSUP;
#endif /* MSPM0_HFCLK_ENABLED */
	}

	*rate = clock_mspm0_sysosc_rate();
	return 0;
}

/* fVCO = ref_rate / PDIV * QDIV, from the live syspllcfg1 dividers. */
static int clock_mspm0_syspll_fvco(const struct device *dev, uint32_t *fvco)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	uint32_t pdiv = 1U << FIELD_GET(SYSCTL_SYSPLLCFG1_PDIV, soclock->syspllcfg1);
	uint32_t qdiv = FIELD_GET(SYSCTL_SYSPLLCFG1_QDIV, soclock->syspllcfg1) + 1;
	uint32_t ref_rate;
	int ret = clock_mspm0_syspll_ref_rate(dev, &ref_rate);

	if (ret < 0) {
		return ret;
	}

	*fvco = (ref_rate / pdiv) * qdiv;
	return 0;
}

/* SYSPLL output rate (CLK0/CLK1 = fVCO/RDIVCLKx, CLK2X = 2*fVCO/RDIVCLK2X); -ENODEV if disabled. */
static int clock_mspm0_syspll_output_rate(const struct device *dev, uint32_t clk, uint32_t *rate)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	uint32_t fvco;
	uint32_t rdiv;
	int ret;

	switch (clk) {
	case MSPM0_CLOCK_SYSPLL_CLK0:
		if (!(soclock->syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK0)) {
			return -ENODEV;
		}
		rdiv = (FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK0, soclock->syspllcfg0) + 1) * 2;
		break;

	case MSPM0_CLOCK_SYSPLL_CLK1:
		if (!(soclock->syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK1)) {
			return -ENODEV;
		}
		rdiv = (FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK1, soclock->syspllcfg0) + 1) * 2;
		break;

	case MSPM0_CLOCK_SYSPLL_CLK2X:
		if (!(soclock->syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK2X)) {
			return -ENODEV;
		}
		rdiv = FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK2X, soclock->syspllcfg0) + 1;
		break;

	default:
		return -EINVAL;
	}

	ret = clock_mspm0_syspll_fvco(dev, &fvco);
	if (ret < 0) {
		return ret;
	}

	*rate = (clk == MSPM0_CLOCK_SYSPLL_CLK2X) ? ((2 * fvco) / rdiv) : (fvco / rdiv);
	return 0;
}

/* Bin floopin_hz into its factory-trimmed range and (re)load syspllparam0/1 accordingly. */
static int clock_mspm0_syspll_load_trim(uint32_t floopin_hz)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	volatile struct mspm_factoryregion_regs *factoryregion = MSPM_FACTORY_REGS;
	uint32_t param0;
	uint32_t param1;
#if defined(MSPM0_HAS_CPUSS)
	volatile struct mspm_cpuss_regs *cpuss = MSPM_CPUSS_REGS;
	uint32_t old_cpuss_ctl = cpuss->ctl;

	cpuss->ctl &= ~CPUSS_CTL_ICACHE;
#endif /* MSPM0_HAS_CPUSS */

	if (floopin_hz >= MHZ(4) && floopin_hz < MHZ(8)) {
		param0 = factoryregion->pllstartup0_4_8;
		param1 = factoryregion->pllstartup1_4_8;
	} else if (floopin_hz >= MHZ(8) && floopin_hz < MHZ(16)) {
		param0 = factoryregion->pllstartup0_8_16;
		param1 = factoryregion->pllstartup1_8_16;
	} else if (floopin_hz >= MHZ(16) && floopin_hz < MHZ(32)) {
		param0 = factoryregion->pllstartup0_16_32;
		param1 = factoryregion->pllstartup1_16_32;
	} else if (floopin_hz >= MHZ(32) && floopin_hz <= MHZ(48)) {
		param0 = factoryregion->pllstartup0_32_48;
		param1 = factoryregion->pllstartup1_32_48;
	} else {
		return -EINVAL;
	}

	soclock->syspllparam0 = param0;
	soclock->syspllparam1 = param1;

#if defined(MSPM0_REQUIRES_SYSPLLPARAM2)
	soclock->syspllparam2 = factoryregion->syspllparam2;
	soclock->syspllldoctl = factoryregion->syspllldoctl;
	soclock->syspllldoprog = factoryregion->syspllldoprog;
#endif /* MSPM0_REQUIRES_SYSPLLPARAM2 */

#if defined(MSPM0_HAS_CPUSS)
	/* restore CPUSS flash cache control */
	cpuss->ctl = old_cpuss_ctl;
#endif /* MSPM0_HAS_CPUSS */

	return 0;
}

/* Switch SYSPLL reference at runtime, reloading the trim bin for the new fLOOPIN. */
static int clock_mspm0_configure_syspll(const struct device *dev,
					volatile struct mspm_sysctl_soclock_regs *soclock,
					enum mspm0_clock_source source)
{
	uint32_t pdiv = 1U << FIELD_GET(SYSCTL_SYSPLLCFG1_PDIV, soclock->syspllcfg1);
	uint32_t ref_rate;

	switch (source) {
#if MSPM0_HFCLK_ENABLED
	case MSPM0_CLOCK_SRC_HFCLK: {
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};
		int ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
					       &ref_rate);

		if (ret < 0) {
			return ret;
		}

		soclock->syspllcfg0 |= SYSCTL_SYSPLLCFG0_SYSPLLREF;
		break;
	}
#endif /* MSPM0_HFCLK_ENABLED */

	case MSPM0_CLOCK_SRC_SYSOSC:
		ref_rate = clock_mspm0_sysosc_rate();
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_SYSPLLREF;
		break;

	default:
		return -ENOTSUP;
	}

	return clock_mspm0_syspll_load_trim(ref_rate / pdiv);
}

static int clock_mspm0_init_syspll(const struct device *dev)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	struct mspm0_sys_clock syspll_subsys = {.clk = MSPM0_CLOCK_SYSPLL};
	int ret;

	/* disable SYSPLL before programming it */
	ret = clock_mspm0_off(dev, (clock_control_subsys_t)&syspll_subsys);
	if (ret < 0) {
		return ret;
	}

	/* set predivider */
	soclock->syspllcfg1 =
		(soclock->syspllcfg1 & ~SYSCTL_SYSPLLCFG1_PDIV) |
		FIELD_PREP(SYSCTL_SYSPLLCFG1_PDIV, SYSCTL_SYSPLLCFG1_PDIV_VAL(MSPM0_SYSPLL_PDIV));

	/* set reference and factory trim via the shared runtime path */
#if DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_HFCLK)
	LOG_DBG("SYSPLL booting from HFCLK");
	ret = clock_mspm0_configure_syspll(dev, soclock, MSPM0_CLOCK_SRC_HFCLK);
#else
	LOG_DBG("SYSPLL booting from SYSOSC");
	ret = clock_mspm0_configure_syspll(dev, soclock, MSPM0_CLOCK_SRC_SYSOSC);
#endif /* DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_HFCLK) */
	if (ret < 0) {
		return ret;
	}

	/* set QDIV */
	soclock->syspllcfg1 =
		(soclock->syspllcfg1 & ~SYSCTL_SYSPLLCFG1_QDIV) |
		FIELD_PREP(SYSCTL_SYSPLLCFG1_QDIV, SYSCTL_SYSPLLCFG1_QDIV_VAL(MSPM0_SYSPLL_QDIV));

	/* set syspllclk2x divider */
	if (MSPM0_SYSPLL_HAS_CLK2X) {
		soclock->syspllcfg0 =
			(soclock->syspllcfg0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK2X) |
			SYSCTL_SYSPLLCFG0_ENABLECLK2X |
			FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK2X,
				   SYSCTL_SYSPLLCFG0_RDIVCLK2X_VAL(MSPM0_SYSPLL_CLK2X_DIV));
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK2X;
	}

	/* set syspllclk1 divider */
	if (MSPM0_SYSPLL_HAS_CLK1) {
		soclock->syspllcfg0 = (soclock->syspllcfg0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK1) |
				      SYSCTL_SYSPLLCFG0_ENABLECLK1 |
				      FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK1,
						 SYSCTL_SYSPLLCFG0_RDIVCLK1_VAL(
							 MSPM0_SYSPLL_CLK1_DIV));
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK1;
	}

	/* set syspllclk0 divider */
	if (MSPM0_SYSPLL_HAS_CLK0) {
		soclock->syspllcfg0 = (soclock->syspllcfg0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK0) |
				      SYSCTL_SYSPLLCFG0_ENABLECLK0 |
				      FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK0,
						 SYSCTL_SYSPLLCFG0_RDIVCLK0_VAL(
							 MSPM0_SYSPLL_CLK0_DIV));
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK0;
	}

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSPLL)
	if (MSPM0_SYSPLL_HAS_CLK2X) {
		soclock->syspllcfg0 |= SYSCTL_SYSPLLCFG0_MCLK2XVCO;
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_MCLK2XVCO;
	}
#endif

	/* enable SYSPLL */
	return clock_mspm0_on(dev, (clock_control_subsys_t)&syspll_subsys);
}
#endif /* DT_SYSPLL_OKAY */

static int clock_mspm0_configure(const struct device *dev, clock_control_subsys_t sys, void *data)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	enum mspm0_clock_source *source = (enum mspm0_clock_source *)data;
	bool disabled = false;
	int ret;

	if (clock_mspm0_get_status(dev, sys) == CLOCK_CONTROL_STATUS_ON) {
		disabled = clock_mspm0_off(dev, sys) == 0;
	}

	switch (sys_clock->clk) {
#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL:
		ret = clock_mspm0_configure_syspll(dev, soclock, *source);
		break;
#endif /* DT_SYSPLL_OKAY */

	default:
		ret = -ENOTSUP;
	}

	/* restore the clock to whatever state it was in before this call */
	if (disabled) {
		if (ret < 0) {
			clock_mspm0_on(dev, sys);
		} else {
			return clock_mspm0_on(dev, sys);
		}
	}

	return ret;
}

static int clock_mspm0_init(const struct device *dev)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	int ret = 0;

	LOG_DBG("initializing MSPM0 clock tree");

	/* set SYSOSCFG frequency */
	soclock->sysosccfg = (soclock->sysosccfg & ~SYSCTL_SYSOSCCFG_FREQ) |
			     FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, MSPM0_SYSOSC_FREQ);

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_NODELABEL(sysosc)) && (DT_SYSOSC_FREQ == 4000000)
	DL_SYSCTL_setMCLKDivider(MSPM0_MCLK_DIV);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(ulpclk), clk_div)
	DL_SYSCTL_setULPCLKDivider(mspm0_ulpclk_cfg.clk_div);
#endif

#if DT_SYSPLL_OKAY
	ret = clock_mspm0_init_syspll(dev);
	if (ret < 0) {
		LOG_ERR("failed to init SYSPLL: %d", ret);
		return ret;
	}
#endif /* DT_SYSPLL_OKAY */

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
	.get_status = clock_mspm0_get_status,
	.get_rate = clock_mspm0_get_rate,
	.set_rate = clock_mspm0_set_rate,
	.configure = clock_mspm0_configure,
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm0_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_mspm0_driver_api);
