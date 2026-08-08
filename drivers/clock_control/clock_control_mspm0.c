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

#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define MSPM0_REQUIRES_SYSPLLPARAM2
#define MSPM0_MCLK_CANNOT_USE_LFCLK 1
#endif

#if DT_NODE_EXISTS(MSPM_CPUSS_NODE)
#define MSPM0_HAS_CPUSS 1
#endif

#define MSPM0_CLK_WAIT_TIMEOUT_US 1000

/* Crystal startup is not instant; wait up to 4x its rated startup delay. */
#define MSPM0_XTAL_WAIT_TIMEOUT_US(startup_us) ((startup_us) * 4)

#define DT_SYSOSC DT_NODELABEL(sysosc)

#define DT_SYSOSC_FREQ DT_PROP(DT_SYSOSC, clock_frequency)
#if DT_SYSOSC_FREQ == 32000000
#define MSPM0_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_BASE
#elif DT_SYSOSC_FREQ == 4000000
#define MSPM0_SYSOSC_FREQ SYSCTL_SYSOSCCFG_FREQ_4M
#else
#error "Set SYSOSC clock frequency not supported"
#endif

#define DT_HFCLK    DT_NODELABEL(hfclk)
#define DT_HFCLK_IN DT_NODELABEL(hfclk_in)
#define DT_HFXT     DT_NODELABEL(hfxt)
#define DT_HSCLK    DT_NODELABEL(hsclk)
#define DT_LFCLK    DT_NODELABEL(lfclk)
#define DT_LFCLK_IN DT_NODELABEL(lfclk_in)
#define DT_LFOSC    DT_NODELABEL(lfosc)
#define DT_LFXT     DT_NODELABEL(lfxt)
#define DT_CANCLK   DT_NODELABEL(canclk)
#define DT_MCLK     DT_NODELABEL(mclk)
#define DT_MFPCLK   DT_NODELABEL(mfpclk)
#define DT_SYSPLL   DT_NODELABEL(syspll)
#define DT_ULPCLK   DT_NODELABEL(ulpclk)

#define MSPM0_ULPCLK_DIV DT_PROP_OR(DT_ULPCLK, clk_div, 1)
#define MSPM0_MCLK_DIV   DT_PROP_OR(DT_MCLK, clk_div, 1)

#define DT_CANCLK_OKAY   DT_NODE_HAS_STATUS_OKAY(DT_CANCLK)
#define DT_HFCLK_IN_OKAY DT_NODE_HAS_STATUS_OKAY(DT_HFCLK_IN)
#define DT_HFCLK_OKAY    DT_NODE_HAS_STATUS_OKAY(DT_HFCLK)
#define DT_HFXT_OKAY     DT_NODE_HAS_STATUS_OKAY(DT_HFXT)
#define DT_HSCLK_OKAY    DT_NODE_HAS_STATUS_OKAY(DT_HSCLK)
#define DT_LFCLK_IN_OKAY DT_NODE_HAS_STATUS_OKAY(DT_LFCLK_IN)
#define DT_LFOSC_OKAY    DT_NODE_HAS_STATUS_OKAY(DT_LFOSC)
#define DT_LFXT_OKAY     DT_NODE_HAS_STATUS_OKAY(DT_LFXT)
#define DT_MFPCLK_OKAY   DT_NODE_HAS_STATUS_OKAY(DT_MFPCLK)
#define DT_SYSPLL_OKAY   DT_NODE_HAS_STATUS_OKAY(DT_SYSPLL)

#define DT_CANCLK_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_CANCLK)
#define DT_MCLK_CLOCKS_CTRL   DT_CLOCKS_CTLR(DT_MCLK)
#define DT_LFCLK_CLOCKS_CTRL  DT_CLOCKS_CTLR(DT_LFCLK)
#define DT_HFCLK_CLOCKS_CTRL  DT_CLOCKS_CTLR(DT_HFCLK)
#define DT_HSCLK_CLOCKS_CTRL  DT_CLOCKS_CTLR(DT_HSCLK)
#define DT_MFPCLK_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_MFPCLK)
#define DT_SYSPLL_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_SYSPLL)

/* High Frequency Clock */
#if DT_HFCLK_OKAY

#define MSPM0_HFCLK_FREQ DT_PROP(DT_HFCLK_CLOCKS_CTRL, clock_frequency)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_HFCLK_CLOCKS_CTRL), "HFCLK source not enabled");

#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFCLK_IN)
/* High frequency digital input */
#elif DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)

#define MSPM0_HFXT_STARTUP_US DT_PROP_OR(DT_HFXT, ti_xtal_startup_delay_us, 0)
#define MSPM0_HFXT_FREQ_MHZ   (MSPM0_HFCLK_FREQ / MHZ(1))

#if MSPM0_HFXT_FREQ_MHZ >= 4 && MSPM0_HFXT_FREQ_MHZ <= 8
#define MSPM0_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_4_8_MHZ
#elif MSPM0_HFXT_FREQ_MHZ > 8 && MSPM0_HFXT_FREQ_MHZ <= 16
#define MSPM0_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_8_16_MHZ
#elif MSPM0_HFXT_FREQ_MHZ > 16 && MSPM0_HFXT_FREQ_MHZ <= 32
#define MSPM0_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_16_32_MHZ
#elif MSPM0_HFXT_FREQ_MHZ > 32 && MSPM0_HFXT_FREQ_MHZ <= 48
#define MSPM0_HFXT_RANGE SYSCTL_HFCLKCLKCFG_HFXTRSEL_32_48_MHZ
#else
#error "HFXT frequency out of supported range (4-48 MHz)"
#endif

#else
#error "Invalid HFCLK source"
#endif

#endif /* DT_HFCLK_OKAY */

/* Low-Frequency clock */
#if !DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT) &&                                                \
	!DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN) &&                                        \
	!DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFOSC)
#error "Invalid LFCLK source"
#endif

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_LFCLK_CLOCKS_CTRL), "LFCLK source not enabled");

#if DT_LFXT_OKAY
#define MSPM0_LFXT_STARTUP_US DT_PROP_OR(DT_LFXT, ti_xtal_startup_delay_us, 0)
#endif /* DT_LFXT_OKAY */

/* Mid-Frequency Precision Clock */
#if DT_MFPCLK_OKAY
#if !DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_HFCLK) &&                                              \
	!DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_SYSOSC)
#error "Invalid MFPCLK source"
#endif
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_MFPCLK_CLOCKS_CTRL), "MFPCLK source not enabled");
#endif /* DT_MFPCLK_OKAY */

/* High-Speed Clock */
#if DT_HSCLK_OKAY
#if !DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_HFCLK) &&                                               \
	!DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL) &&                                          \
	!DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSOSC)
#error "Invalid HSCLK source"
#endif
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_HSCLK_CLOCKS_CTRL), "HSCLK source not enabled");
#endif /* DT_HSCLK_OKAY */

/* Main Clock is always present */
#if !DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) &&                                                \
	!DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC) &&                                           \
	(defined(MSPM0_MCLK_CANNOT_USE_LFCLK) || !DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_LFCLK))
#error "Invalid MCLK source"
#endif
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_MCLK_CLOCKS_CTRL), "MCLK source not enabled");

/* CAN Clock */
#if DT_CANCLK_OKAY
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_CANCLK_CLOCKS_CTRL), "CANCLK source not enabled");
#if !DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_HFCLK) &&                                              \
	!DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_SYSPLL)
#error "Invalid CANCLK source"
#endif
#endif /* DT_CANCLK_OKAY */

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

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		soclock->genclken |= SYSCTL_GENCLKEN_MFPCLKEN;
		return 0;
#endif /* DT_MFPCLK_OKAY */

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

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		soclock->genclken &= ~SYSCTL_GENCLKEN_MFPCLKEN;
		return 0;
#endif /* DT_MFPCLK_OKAY */

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

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		if (soclock->genclken & SYSCTL_GENCLKEN_MFPCLKEN) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
#endif /* DT_MFPCLK_OKAY */

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

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK:
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_HFCLKOFF) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_HFCLKGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
#endif /* DT_HFCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK:
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKDEAD) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
#endif /* DT_HSCLK_OKAY */

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK: {
		struct mspm0_sys_clock src_subsys;

#if DT_SYSPLL_OKAY
		src_subsys.clk = (soclock->genclkcfg & SYSCTL_GENCLKCFG_CANCLKSRC)
					 ? MSPM0_CLOCK_SYSPLL
					 : MSPM0_CLOCK_HFCLK;
#else
		src_subsys.clk = MSPM0_CLOCK_HFCLK;
#endif /* DT_SYSPLL_OKAY */
		return clock_mspm0_get_status(dev, (clock_control_subsys_t)&src_subsys);
	}
#endif /* DT_CANCLK_OKAY */

	case MSPM0_CLOCK_LFCLK: {
		uint32_t mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, soclock->clkstatus);

		switch (mux) {
		case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC:
			if (soclock->clkstatus & SYSCTL_CLKSTATUS_LFOSCGOOD) {
				return CLOCK_CONTROL_STATUS_ON;
			}
			return CLOCK_CONTROL_STATUS_UNKNOWN;

		case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFXT:
			if (soclock->clkstatus & SYSCTL_CLKSTATUS_LFXTGOOD) {
				return CLOCK_CONTROL_STATUS_ON;
			} else if (soclock->clkstatus & SYSCTL_CLKSTATUS_LFCLKFAIL) {
				return CLOCK_CONTROL_STATUS_OFF;
			}

			return CLOCK_CONTROL_STATUS_UNKNOWN;

		default:
			if (soclock->clkstatus & SYSCTL_CLKSTATUS_LFCLKFAIL) {
				return CLOCK_CONTROL_STATUS_OFF;
			}
			/* LFCLK_IN: no hardware readiness bit for an external input */
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
	}

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

static uint32_t clock_mspm0_lfclk_rate(void)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	uint32_t mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, soclock->clkstatus);

	switch (mux) {
#if DT_LFXT_OKAY
	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFXT:
		return DT_PROP(DT_LFXT, clock_frequency);
#endif /* DT_LFXT_OKAY */

#if DT_LFCLK_IN_OKAY
	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFCLKIN:
		return DT_PROP(DT_LFCLK_IN, clock_frequency);
#endif /* DT_LFCLK_IN_OKAY */

	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC:
	default:
		return DT_PROP(DT_LFOSC, clock_frequency);
	}
}

static int clock_mspm0_get_rate(const struct device *dev, clock_control_subsys_t sys,
				uint32_t *rate)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		*rate = clock_mspm0_sysosc_rate();
		break;

	case MSPM0_CLOCK_LFCLK:
		*rate = clock_mspm0_lfclk_rate();
		break;

	case MSPM0_CLOCK_MCLK: {
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_CURMCLKSEL) {
			/* MCLK switched to LFCLK (PM low-power state) */
			*rate = clock_mspm0_lfclk_rate();
#if DT_HSCLK_OKAY
		} else if (soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX) {
			/* MCLK currently sourced from HSCLK */
			struct mspm0_sys_clock hsclk_subsys = {.clk = MSPM0_CLOCK_HSCLK};
			int ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hsclk_subsys,
						       rate);

			if (ret < 0) {
				return ret;
			}
#endif /* DT_HSCLK_OKAY */
		} else {
			/* MCLK switched to SYSOSC */
			*rate = clock_mspm0_sysosc_rate();
			if (*rate == MHZ(4)) {
				*rate /= MSPM0_MCLK_DIV;
			}
		}
		break;
	}

	case MSPM0_CLOCK_ULPCLK: {
		uint32_t mclk_rate;
		struct mspm0_sys_clock mclk_subsys = {.clk = MSPM0_CLOCK_MCLK};
		int ret =
			clock_mspm0_get_rate(dev, (clock_control_subsys_t)&mclk_subsys, &mclk_rate);

		if (ret < 0) {
			return ret;
		}
		*rate = mclk_rate / MSPM0_ULPCLK_DIV;
		break;
	}

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		*rate = MHZ(4);
		break;
#endif /* DT_MFPCLK_OKAY */

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK:
		if (soclock->genclkcfg & SYSCTL_GENCLKCFG_CANCLKSRC) {
#if DT_SYSPLL_OKAY
			int ret =
				clock_mspm0_syspll_output_rate(dev, MSPM0_CLOCK_SYSPLL_CLK1, rate);

			if (ret < 0) {
				return ret;
			}
#else
			return -ENOTSUP;
#endif
		} else {
#if DT_HFCLK_OKAY
			struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};
			int ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
						       rate);

			if (ret < 0) {
				return ret;
			}
#else
			return -ENOTSUP;
#endif
		}
		break;
#endif /* DT_CANCLK_OKAY */

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK: {
#if DT_HFXT_OKAY && DT_HFCLK_IN_OKAY
		if (soclock->hsclken & SYSCTL_HSCLKEN_USEEXTHFCLK) {
			*rate = DT_PROP(DT_HFCLK_IN, clock_frequency);
		} else {
			*rate = DT_PROP(DT_HFXT, clock_frequency);
		}
#else
		*rate = MSPM0_HFCLK_FREQ;
#endif
		break;
	}
#endif /* DT_HFCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK: {
		if (soclock->clkstatus & SYSCTL_CLKSTATUS_CURHSCLKSEL) {
#if DT_HFCLK_OKAY
			/* HSCLK currently sourced from HFCLK */
			struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};
			int ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
						       rate);
			if (ret < 0) {
				return ret;
			}
#else
			return -ENOTSUP;
#endif /* DT_HFCLK_OKAY */
		} else {
#if DT_SYSPLL_OKAY
			/* HSCLK is fed by SYSPLL CLK2X (2x-VCO path) or CLK0 otherwise. */
			uint32_t clk = (soclock->syspllcfg0 & SYSCTL_SYSPLLCFG0_MCLK2XVCO)
					       ? MSPM0_CLOCK_SYSPLL_CLK2X
					       : MSPM0_CLOCK_SYSPLL_CLK0;
			int ret = clock_mspm0_syspll_output_rate(dev, clk, rate);

			if (ret < 0) {
				return ret;
			}
#else
			/* SYSOSC feeds HSCLK when no SYSPLL is present */
			*rate = clock_mspm0_sysosc_rate();
#endif /* DT_SYSPLL_OKAY */
		}
		break;
	}
#endif /* DT_HSCLK_OKAY */

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
#if DT_HFCLK_OKAY
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};

		return clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys, rate);
#else
		return -ENOTSUP;
#endif /* DT_HFCLK_OKAY */
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
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK: {
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};
		int ret =
			clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys, &ref_rate);

		if (ret < 0) {
			return ret;
		}
		soclock->syspllcfg0 |= SYSCTL_SYSPLLCFG0_SYSPLLREF;
		break;
	}
#endif /* DT_HFCLK_OKAY */

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
		soclock->syspllcfg0 =
			(soclock->syspllcfg0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK1) |
			SYSCTL_SYSPLLCFG0_ENABLECLK1 |
			FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK1,
				   SYSCTL_SYSPLLCFG0_RDIVCLK1_VAL(MSPM0_SYSPLL_CLK1_DIV));
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK1;
	}

	/* set syspllclk0 divider */
	if (MSPM0_SYSPLL_HAS_CLK0) {
		soclock->syspllcfg0 =
			(soclock->syspllcfg0 & ~SYSCTL_SYSPLLCFG0_RDIVCLK0) |
			SYSCTL_SYSPLLCFG0_ENABLECLK0 |
			FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK0,
				   SYSCTL_SYSPLLCFG0_RDIVCLK0_VAL(MSPM0_SYSPLL_CLK0_DIV));
	} else {
		soclock->syspllcfg0 &= ~SYSCTL_SYSPLLCFG0_ENABLECLK0;
	}

#if DT_HSCLK_OKAY && DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) &&                                \
	DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL)
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

#if DT_HFCLK_OKAY
static int clock_mspm0_configure_hfclk(volatile struct mspm_sysctl_soclock_regs *soclock,
				       enum mspm0_clock_source source)
{
	uint32_t timeout_us = MSPM0_CLK_WAIT_TIMEOUT_US;

	switch (source) {
#if DT_HFXT_OKAY
	case MSPM0_CLOCK_SRC_HFXT: {
		/* disable HFXT */
		soclock->hsclken &= ~SYSCTL_HSCLKEN_HFXTEN;

		/* update HFXT frequency range */
		soclock->hfclkclkcfg = (soclock->hfclkclkcfg & ~SYSCTL_HFCLKCLKCFG_HFXTRSEL) |
				       FIELD_PREP(SYSCTL_HFCLKCLKCFG_HFXTRSEL, MSPM0_HFXT_RANGE);

		/* set HFXT startup time */
		soclock->hfclkclkcfg = (soclock->hfclkclkcfg & ~SYSCTL_HFCLKCLKCFG_HFXTTIME) |
				       SYSCTL_HFCLKCLKCFG_HFXTTIME_VAL(MSPM0_HFXT_STARTUP_US);

		/* set HFXT input as HFCLK source */
		soclock->hsclken &= ~SYSCTL_HSCLKEN_USEEXTHFCLK;

		/* enable HFXT */
		soclock->hsclken |= SYSCTL_HSCLKEN_HFXTEN;

		/* enable HFXT startup monitor */
		soclock->hfclkclkcfg |= SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK;

		timeout_us = MSPM0_XTAL_WAIT_TIMEOUT_US(MSPM0_HFXT_STARTUP_US);
		break;
	}
#endif /* DT_HFXT_OKAY */

#if DT_HFCLK_IN_OKAY
	case MSPM0_CLOCK_SRC_HFCLK_IN: {
		/* set external digital clock input as HFCLK source */
		soclock->hsclken |= SYSCTL_HSCLKEN_USEEXTHFCLK;

		break;
	}
#endif /* DT_HFCLK_IN_OKAY */

	default:
		return -ENOTSUP;
	}

	/* wait for HFCLK to stabilize */
	if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_HFCLKGOOD) != 0, timeout_us, NULL)) {
		LOG_ERR("timed out waiting for HFCLK to stabilize");
		return -ETIMEDOUT;
	}

	return 0;
}
#endif /* DT_HFCLK_OKAY */

static int clock_mspm0_configure_lfclk(volatile struct mspm_sysctl_soclock_regs *soclock,
				       enum mspm0_clock_source source)
{
	uint32_t mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, soclock->clkstatus);

	switch (source) {
#if DT_LFXT_OKAY
	case MSPM0_CLOCK_SRC_LFXT: {
		/* cannot switch to LFXT if LFCLK_IN is being used */
		if (mux == SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFCLKIN) {
			return -ENOTSUP;
		}

		/* disable low power mode and set drive strength to lowest */
		soclock->lfclkcfg &= ~(SYSCTL_LFCLKCFG_LOWCAP | SYSCTL_LFCLKCFG_XT1DRIVE);

		/* start LFXT */
		soclock->lfxtctl = FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) |
				   SYSCTL_LFXTCTL_STARTLFXT;

		/* wait for LFXT to stabilize */
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_LFXTGOOD) != 0,
			      MSPM0_XTAL_WAIT_TIMEOUT_US(MSPM0_LFXT_STARTUP_US), NULL)) {
			LOG_ERR("timed out waiting for LFXT to stabilize");
			return -ETIMEDOUT;
		}

		/* set LFCLK source as LFXT */
		soclock->lfxtctl = FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) |
				   SYSCTL_LFXTCTL_SETUSELFXT;

		/* wait for lfosc to turn off */
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_LFOSCGOOD) == 0,
			      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
			LOG_ERR("timed out waiting for LFOSC to turn off");
			return -ETIMEDOUT;
		}
		break;
	}
#endif /* DT_LFXT_OKAY */

#if DT_LFCLK_IN_OKAY
	case MSPM0_CLOCK_SRC_LFCLK_IN:
		/* turn off LFXT */
		soclock->lfxtctl = FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL);

		/* set LFCLK source as LFCLK_IN */
		soclock->exlfctl = FIELD_PREP(SYSCTL_EXLFCTL_KEY, SYSCTL_EXLFCTL_KEY_VAL) |
				   SYSCTL_EXLFCTL_SETUSEEXLF;
		break;
#endif /* DT_LFCLK_IN_OKAY */

#if DT_LFOSC_OKAY
	case MSPM0_CLOCK_SRC_LFOSC:
		/* Once LFCLK has switched to LFXT/LFCLK_IN, the mux is latched
		 * and cannot be switched back to LFOSC in software -- only a
		 * BOR or POR resets it.
		 */
		if (mux != SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC) {
			LOG_WRN("LFCLK cannot be switched back to LFOSC once moved off it; do a "
				"BOR/POR to enable LFOSC as source");
		}
		break;
#endif /* DT_LFOSC_OKAY */

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if DT_MFPCLK_OKAY

#if DT_HFCLK_OKAY
/* Compute HFCLK4MFPCLKDIV (1-16) so hfclk_rate/divider == 4 MHz. */
static int clock_mspm0_mfpclk_hfclk_div(uint32_t hfclk_rate, uint32_t *divider)
{
	if (hfclk_rate == 0 || hfclk_rate % MHZ(4) != 0) {
		return -EINVAL;
	}

	*divider = hfclk_rate / MHZ(4);

	if (*divider < 1 || *divider > 16) {
		return -EINVAL;
	}

	return 0;
}
#endif /* DT_HFCLK_OKAY */

static int clock_mspm0_configure_mfpclk(const struct device *dev,
					volatile struct mspm_sysctl_soclock_regs *soclock,
					enum mspm0_clock_source source)
{
	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK: {
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};
		uint32_t hfclk_rate;
		uint32_t divider;
		int ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
					       &hfclk_rate);

		if (ret < 0) {
			return ret;
		}

		ret = clock_mspm0_mfpclk_hfclk_div(hfclk_rate, &divider);
		if (ret < 0) {
			return ret;
		}

		/* set MFPCLK divider so that HFCLK / divider == 4 MHz */
		soclock->genclkcfg = (soclock->genclkcfg & ~SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV) |
				     FIELD_PREP(SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV,
						SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV_VAL(divider));

		/* set HFCLK as MFPCLK source */
		soclock->genclkcfg |= SYSCTL_GENCLKCFG_MFPCLKSRC;
		break;
	}
#endif /* DT_HFCLK_OKAY */

	case MSPM0_CLOCK_SRC_SYSOSC:
		/* set SYSOSC as MFPCLK source */
		soclock->genclkcfg &= ~SYSCTL_GENCLKCFG_MFPCLKSRC;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* DT_MFPCLK_OKAY */

static int clock_mspm0_configure_mclk(volatile struct mspm_sysctl_soclock_regs *soclock,
				      enum mspm0_clock_source source)
{
	soclock->mclkcfg &= ~SYSCTL_MCLKCFG_USELFCLK;

	/* verify that MCLK is not sourced from LFCLK */
	if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_CURMCLKSEL) == 0,
		      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
		LOG_ERR("timed out waiting for MCLK to leave LFCLK");
		return -ETIMEDOUT;
	}

	soclock->mclkcfg &= ~SYSCTL_MCLKCFG_USEHSCLK;

	/* verify that MCLK is not sourced from HSCLK */
	if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX) == 0,
		      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
		LOG_ERR("timed out waiting for MCLK to leave HSCLK");
		return -ETIMEDOUT;
	}

	switch (source) {
	case MSPM0_CLOCK_SRC_SYSOSC: {
		/* nothing to do, we are already using system oscillator at this point */
		break;
	}

#if !MSPM0_MCLK_CANNOT_USE_LFCLK
	case MSPM0_CLOCK_SRC_LFCLK:
		/* use LFCLK as MCLK source */
		soclock->mclkcfg |= SYSCTL_MCLKCFG_USELFCLK;

		/* verify that MCLK is sourced from LFCLK */
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_CURMCLKSEL) != 0,
			      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
			LOG_ERR("timed out waiting for MCLK to switch to LFCLK");
			return -ETIMEDOUT;
		}
		break;
#endif

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_SRC_HSCLK:
		soclock->mclkcfg |= SYSCTL_MCLKCFG_USEHSCLK;

		/* verify that MCLK is sourced from HSCLK */
		if (!WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX) != 0,
			      MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
			LOG_ERR("timed out waiting for MCLK to switch to HSCLK");
			return -ETIMEDOUT;
		}
		break;
#endif /* DT_HSCLK_OKAY */

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if DT_HSCLK_OKAY
static int clock_mspm0_configure_hsclk(volatile struct mspm_sysctl_soclock_regs *soclock,
				       enum mspm0_clock_source source)
{
	bool mclk_uses_hsclk = !!(soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX);
	int ret = 0;

	if (mclk_uses_hsclk) {
		ret = clock_mspm0_configure_mclk(soclock, MSPM0_CLOCK_SRC_SYSOSC);

		if (ret < 0) {
			return ret;
		}
	}

	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK:
		/* set HFCLK as HSCLK source */
		soclock->hsclkcfg |= SYSCTL_HSCLKCFG_HSCLKSEL;
		break;
#endif /* DT_HFCLK_OKAY */

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SRC_SYSPLL:
		/* set SYSPLL as HSCLK source */
		soclock->hsclkcfg &= ~SYSCTL_HSCLKCFG_HSCLKSEL;
		break;
#else
	case MSPM0_CLOCK_SRC_SYSOSC:
		/* set SYSOSC as HSCLK source (no SYSPLL present) */
		soclock->hsclkcfg &= ~SYSCTL_HSCLKCFG_HSCLKSEL;
		break;
#endif /* DT_SYSPLL_OKAY */

	default:
		ret = -ENOTSUP;
		break;
	}

	/* verify that HSCLK started, unless the source itself was rejected above */
	if (ret == 0 && !WAIT_FOR((soclock->clkstatus & SYSCTL_CLKSTATUS_HSCLKGOOD) != 0,
				   MSPM0_CLK_WAIT_TIMEOUT_US, NULL)) {
		LOG_ERR("timed out waiting for HSCLK to start");
		ret = -ETIMEDOUT;
	}

	/* Restore MCLK to HSCLK regardless of whether the switch above
	 * succeeded -- MCLK was moved off HSCLK to make that switch safe, and
	 * must not be left stuck on SYSOSC just because the new source failed
	 * to come up.
	 */
	if (mclk_uses_hsclk) {
		int restore_ret = clock_mspm0_configure_mclk(soclock, MSPM0_CLOCK_SRC_HSCLK);

		if (ret == 0) {
			ret = restore_ret;
		}
	}

	return ret;
}
#endif /* DT_HSCLK_OKAY */

static int clock_mspm0_init_mclk(const struct device *dev)
{
	volatile struct mspm_sysctl_regs *regs = MSPM_SYSCTL_REGS;
	volatile struct mspm_sysctl_soclock_regs *soclock = &regs->soclock;
	int ret = 0;

	/* configure UDIV */
	soclock->mclkcfg =
		(soclock->mclkcfg & ~SYSCTL_MCLKCFG_UDIV) |
		FIELD_PREP(SYSCTL_MCLKCFG_UDIV, SYSCTL_MCLKCFG_UDIV_VAL(MSPM0_ULPCLK_DIV));

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC) && (DT_SYSOSC_FREQ == 4000000)
	/* configure MDIV */
	soclock->mclkcfg = (soclock->mclkcfg & ~SYSCTL_MCLKCFG_MDIV) |
			   FIELD_PREP(SYSCTL_MCLKCFG_MDIV, SYSCTL_MCLKCFG_MDIV_VAL(MSPM0_MCLK_DIV));
#endif

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK)
	LOG_DBG("MCLK booting from HSCLK");
	ret = clock_mspm0_configure_mclk(soclock, MSPM0_CLOCK_SRC_HSCLK);
#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_LFCLK) && !MSPM0_MCLK_CANNOT_USE_LFCLK
	LOG_DBG("MCLK booting from LFCLK");
	ret = clock_mspm0_configure_mclk(soclock, MSPM0_CLOCK_SRC_LFCLK);
#else
	/* validated at top of file: must be SYSOSC */
	LOG_DBG("MCLK booting from SYSOSC");
	ret = clock_mspm0_configure_mclk(soclock, MSPM0_CLOCK_SRC_SYSOSC);
#endif /* DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) */

	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if DT_CANCLK_OKAY
static int clock_mspm0_configure_canclk(volatile struct mspm_sysctl_soclock_regs *soclock,
					enum mspm0_clock_source source)
{
	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK:
		/* set HFCLK as CANCLK source */
		soclock->genclkcfg &= ~SYSCTL_GENCLKCFG_CANCLKSRC;
		break;
#endif

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SRC_SYSPLL:
		/* set SYSPLLCLK1 as CANCLK source */
		soclock->genclkcfg |= SYSCTL_GENCLKCFG_CANCLKSRC;
		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* DT_CANCLK_OKAY */

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
	case MSPM0_CLOCK_LFCLK:
		ret = clock_mspm0_configure_lfclk(soclock, *source);
		break;

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL:
		ret = clock_mspm0_configure_syspll(dev, soclock, *source);
		break;
#endif /* DT_SYSPLL_OKAY */

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK:
		ret = clock_mspm0_configure_hfclk(soclock, *source);
		break;
#endif /* DT_HFCLK_OKAY */

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		ret = clock_mspm0_configure_mfpclk(dev, soclock, *source);
		break;
#endif /* DT_MFPCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK:
		ret = clock_mspm0_configure_hsclk(soclock, *source);
		break;
#endif /* DT_HSCLK_OKAY */

	case MSPM0_CLOCK_MCLK:
		ret = clock_mspm0_configure_mclk(soclock, *source);
		break;

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK:
		ret = clock_mspm0_configure_canclk(soclock, *source);
		break;
#endif /* DT_CANCLK_OKAY */

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

#if DT_HFCLK_OKAY
#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)
	LOG_DBG("HFCLK booting from HFXT");
	ret = clock_mspm0_configure_hfclk(soclock, MSPM0_CLOCK_SRC_HFXT);
#else
	/* validated at top of file: must be HFCLK_IN */
	LOG_DBG("HFCLK booting from HFCLK_IN");
	ret = clock_mspm0_configure_hfclk(soclock, MSPM0_CLOCK_SRC_HFCLK_IN);
#endif
	if (ret < 0) {
		LOG_ERR("failed to configure HFCLK: %d", ret);
		return ret;
	}
#endif /* DT_HFCLK_OKAY */

#if DT_SYSPLL_OKAY
	ret = clock_mspm0_init_syspll(dev);
	if (ret < 0) {
		LOG_ERR("failed to init SYSPLL: %d", ret);
		return ret;
	}
#endif /* DT_SYSPLL_OKAY */

#if DT_HSCLK_OKAY
#if DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_HFCLK)
	LOG_DBG("HSCLK booting from HFCLK");
	ret = clock_mspm0_configure_hsclk(soclock, MSPM0_CLOCK_SRC_HFCLK);
#elif DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL)
	LOG_DBG("HSCLK booting from SYSPLL");
	ret = clock_mspm0_configure_hsclk(soclock, MSPM0_CLOCK_SRC_SYSPLL);
#else
	/* validated at top of file: must be SYSOSC */
	LOG_DBG("HSCLK booting from SYSOSC");
	ret = clock_mspm0_configure_hsclk(soclock, MSPM0_CLOCK_SRC_SYSOSC);
#endif
	if (ret < 0) {
		LOG_ERR("failed to configure HSCLK: %d", ret);
		return ret;
	}
#endif /* DT_HSCLK_OKAY */

#if DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT)
	LOG_DBG("LFCLK booting from LFXT");
	ret = clock_mspm0_configure_lfclk(soclock, MSPM0_CLOCK_SRC_LFXT);
#elif DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN)
	LOG_DBG("LFCLK booting from LFCLK_IN");
	ret = clock_mspm0_configure_lfclk(soclock, MSPM0_CLOCK_SRC_LFCLK_IN);
#else
	/* validated at top of file: must be LFOSC */
	LOG_DBG("LFCLK booting from LFOSC");
	ret = clock_mspm0_configure_lfclk(soclock, MSPM0_CLOCK_SRC_LFOSC);
#endif
	if (ret < 0) {
		LOG_ERR("failed to configure LFCLK: %d", ret);
		return ret;
	}

	ret = clock_mspm0_init_mclk(dev);
	if (ret < 0) {
		LOG_ERR("failed to init MCLK: %d", ret);
		return ret;
	}

#if DT_MFPCLK_OKAY
	{
		struct mspm0_sys_clock mfpclk_subsys = {.clk = MSPM0_CLOCK_MFPCLK};

#if DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_HFCLK)
		LOG_DBG("MFPCLK booting from HFCLK");
		ret = clock_mspm0_configure_mfpclk(dev, soclock, MSPM0_CLOCK_SRC_HFCLK);
#else
		/* validated at top of file: must be SYSOSC */
		LOG_DBG("MFPCLK booting from SYSOSC");
		ret = clock_mspm0_configure_mfpclk(dev, soclock, MSPM0_CLOCK_SRC_SYSOSC);
#endif /* DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_HFCLK) */
		if (ret < 0) {
			LOG_ERR("failed to configure MFPCLK: %d", ret);
			return ret;
		}

		ret = clock_mspm0_on(dev, (clock_control_subsys_t)&mfpclk_subsys);
		if (ret < 0) {
			LOG_ERR("failed to enable MFPCLK: %d", ret);
			return ret;
		}
	}
#endif /* DT_MFPCLK_OKAY */

#if DT_CANCLK_OKAY
#if DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_SYSPLL)
	LOG_DBG("CANCLK booting from SYSPLL");
	ret = clock_mspm0_configure_canclk(soclock, MSPM0_CLOCK_SRC_SYSPLL);
#else
	/* validated at top of file: must be HFCLK */
	LOG_DBG("CANCLK booting from HFCLK");
	ret = clock_mspm0_configure_canclk(soclock, MSPM0_CLOCK_SRC_HFCLK);
#endif /* DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_SYSPLL) */
	if (ret < 0) {
		LOG_ERR("failed to configure CANCLK: %d", ret);
		return ret;
	}
#endif /* DT_CANCLK_OKAY */

	LOG_DBG("MSPM0 clock tree initialized");

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

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm0_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mspm0_driver_api);
