/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>

#include <soc_cpuss.h>
#include <soc_factoryregion.h>
#include <soc_fri.h>
#include <soc_memcfg.h>
#include <soc_sysctl.h>

LOG_MODULE_REGISTER(clock_control_mspm0, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define MSPM0_REQUIRES_SYSPLLPARAM2 1
#define MSPM0_CONFIGURE_WAIT_STATES 1
#define MSPM0_HAS_MCLK_DIV2_DIV4    1
#define MSPM0_MCLK_CANNOT_USE_LFCLK 1
#endif

#if defined(CONFIG_SOC_SERIES_MSPM0L) || defined(CONFIG_SOC_SERIES_MSPM0G)
#define MSPM0_HAS_CPUSS 1
#endif

#define MSPM0_CLK_WAIT_TIMEOUT_US 10000

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

#if MSPM0_HAS_MCLK_DIV2_DIV4
#define DT_MCLK_DIV_2 DT_NODELABEL(mclk_div_2)
#define DT_MCLK_DIV_4 DT_NODELABEL(mclk_div_4)

#define MSPM0_MCLK_DIV_2 DT_PROP_OR(DT_MCLK_DIV_2, clk_div, 2)
#define MSPM0_MCLK_DIV_4 DT_PROP_OR(DT_MCLK_DIV_4, clk_div, 4)

BUILD_ASSERT(MSPM0_MCLK_DIV_2 == 1 || MSPM0_MCLK_DIV_2 == 2, "MCLK_DIV_2 divider must be 1 or 2");
BUILD_ASSERT(MSPM0_MCLK_DIV_4 == 1 || MSPM0_MCLK_DIV_4 == 2 || MSPM0_MCLK_DIV_4 == 4,
	     "MCLK_DIV_4 divider must be 1, 2, or 4");
BUILD_ASSERT(!(MSPM0_MCLK_DIV_2 == 2 && MSPM0_MCLK_DIV_4 == 1),
	     "MCLK_DIV_4 divider must be >= MCLK_DIV_2 divider");

#define MSPM0_MCLKDIVCFG_VAL                                                                       \
	((MSPM0_MCLK_DIV_2 == 1) ? ((MSPM0_MCLK_DIV_4 == 1)   ? SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_1  \
				    : (MSPM0_MCLK_DIV_4 == 2) ? SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_2  \
							      : SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_4) \
				 : ((MSPM0_MCLK_DIV_4 == 2) ? SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_2_2    \
							    : SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_2_4))
#else
#define MSPM0_ULPCLK_DIV DT_PROP_OR(DT_ULPCLK, clk_div, 1)
#endif /* MSPM0_HAS_MCLK_DIV2_DIV4 */

#define MSPM0_MCLK_DIV DT_PROP_OR(DT_MCLK, clk_div, 1)

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

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_HFCLK_CLOCKS_CTRL), "HFCLK source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFCLK_IN) ||
		     DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT),
	     "Invalid HFCLK source; must be HFCLK_IN or HFXT");

#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)

#define MSPM0_HFCLK_FREQ      DT_PROP(DT_HFCLK_CLOCKS_CTRL, clock_frequency)
#define MSPM0_HFXT_STARTUP_US DT_PROP_OR(DT_HFXT, ti_xtal_startup_delay_us, 0)
#define MSPM0_HFXT_FREQ_MHZ   (MSPM0_HFCLK_FREQ / MHZ(1))

BUILD_ASSERT(MSPM0_HFXT_FREQ_MHZ >= 4 && MSPM0_HFXT_FREQ_MHZ <= 48,
	     "HFXT frequency out of supported hardware range (4-48 MHz)");

#define MSPM0_HFXT_RANGE                                                                           \
	((MSPM0_HFXT_FREQ_MHZ <= 8)    ? SYSCTL_HFCLKCLKCFG_HFXTRSEL_4_8_MHZ                       \
	 : (MSPM0_HFXT_FREQ_MHZ <= 16) ? SYSCTL_HFCLKCLKCFG_HFXTRSEL_8_16_MHZ                      \
	 : (MSPM0_HFXT_FREQ_MHZ <= 32) ? SYSCTL_HFCLKCLKCFG_HFXTRSEL_16_32_MHZ                     \
				       : SYSCTL_HFCLKCLKCFG_HFXTRSEL_32_48_MHZ)

#endif /* DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT) */

#endif /* DT_HFCLK_OKAY */

/* Low-Frequency clock */
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_LFCLK_CLOCKS_CTRL), "LFCLK source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT) ||
		     DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN) ||
		     DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFOSC),
	     "Invalid LFCLK source; must be LFXT, LFCLK_IN, or LFOSC");

#if DT_LFXT_OKAY
#define MSPM0_LFXT_STARTUP_US DT_PROP_OR(DT_LFXT, ti_xtal_startup_delay_us, 0)
#endif /* DT_LFXT_OKAY */

/* Mid-Frequency Precision Clock */
#if DT_MFPCLK_OKAY
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_MFPCLK_CLOCKS_CTRL), "MFPCLK source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_HFCLK) ||
		     DT_SAME_NODE(DT_MFPCLK_CLOCKS_CTRL, DT_SYSOSC),
	     "Invalid MFPCLK source; must be HFCLK or SYSOSC");
#endif /* DT_MFPCLK_OKAY */

/* High-Speed Clock */
#if DT_HSCLK_OKAY
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_HSCLK_CLOCKS_CTRL), "HSCLK source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_HFCLK) ||
		     DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL) ||
		     DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSOSC),
	     "Invalid HSCLK source; must be HFCLK, SYSPLL, or SYSOSC");
#endif /* DT_HSCLK_OKAY */

/* Main Clock is always present */
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_MCLK_CLOCKS_CTRL), "MCLK source not enabled");
#if defined(MSPM0_MCLK_CANNOT_USE_LFCLK)
BUILD_ASSERT(DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) ||
		     DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC),
	     "Invalid MCLK source; must be HSCLK or SYSOSC");
#else
BUILD_ASSERT(DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) ||
		     DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC) ||
		     DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_LFCLK),
	     "Invalid MCLK source; must be HSCLK, SYSOSC, or LFCLK");
#endif /* defined(MSPM0_MCLK_CANNOT_USE_LFCLK) */

/* CAN Clock */
#if DT_CANCLK_OKAY
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_CANCLK_CLOCKS_CTRL), "CANCLK source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_HFCLK) ||
		     DT_SAME_NODE(DT_CANCLK_CLOCKS_CTRL, DT_SYSPLL),
	     "Invalid CANCLK source; must be HFCLK or SYSPLL");
#endif /* DT_CANCLK_OKAY */

/* System PLL */
#if DT_SYSPLL_OKAY

#define MSPM0_SYSPLL_PDIV      DT_PROP(DT_SYSPLL, p_div)
#define MSPM0_SYSPLL_QDIV      DT_PROP(DT_SYSPLL, q_div)
#define MSPM0_SYSPLL_HAS_CLK2X DT_NODE_HAS_PROP(DT_SYSPLL, clk2x_div)
#define MSPM0_SYSPLL_CLK2X_DIV DT_PROP_OR(DT_SYSPLL, clk2x_div, 1)
#define MSPM0_SYSPLL_HAS_CLK1  DT_NODE_HAS_PROP(DT_SYSPLL, clk1_div)
#define MSPM0_SYSPLL_CLK1_DIV  DT_PROP_OR(DT_SYSPLL, clk1_div, 2)
#define MSPM0_SYSPLL_HAS_CLK0  DT_NODE_HAS_PROP(DT_SYSPLL, clk0_div)
#define MSPM0_SYSPLL_CLK0_DIV  DT_PROP_OR(DT_SYSPLL, clk0_div, 2)

BUILD_ASSERT(MSPM0_SYSPLL_CLK1_DIV != 0 && MSPM0_SYSPLL_CLK1_DIV % 2 == 0,
	     "SYSPLLCLK1 divider can only be a non-zero even number");

BUILD_ASSERT(MSPM0_SYSPLL_CLK0_DIV != 0 && MSPM0_SYSPLL_CLK0_DIV % 2 == 0,
	     "SYSPLLCLK0 divider can only be a non-zero even number");

BUILD_ASSERT(!(MSPM0_SYSPLL_HAS_CLK2X && MSPM0_SYSPLL_HAS_CLK0),
	     "Only CLK2X or CLK0 can be enabled at a time on the SYSPLL");

BUILD_ASSERT(MSPM0_SYSPLL_QDIV > 1, "Divide-by-one is not a valid QDIV option");

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_CLOCKS_CTLR(DT_SYSPLL)), "SYSPLL source not enabled");

BUILD_ASSERT(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_HFCLK) ||
		     DT_SAME_NODE(DT_CLOCKS_CTLR(DT_SYSPLL), DT_SYSOSC),
	     "Invalid SYSPLL source; must be HFCLK or SYSOSC");

#endif /* DT_SYSPLL_OKAY */

struct clock_mspm0_config {
	const struct device *sysctl;
#if defined(MSPM0_HAS_CPUSS)
	const struct device *cpuss;
#endif
#if DT_SYSPLL_OKAY
	const struct device *factoryregion;
#endif
#if defined(MSPM0_CONFIGURE_WAIT_STATES)
	const struct device *fri;
	const struct device *memcfg;
#endif
};

#define MSPM0_CLK_WAIT_RETRIES 100000

static int clock_mspm0_wait_clkstatus(const struct device *sysctl, uint32_t mask, bool set,
				      uint32_t timeout_us)
{
	uint32_t clkstatus;
	int ret;

	if (k_is_pre_kernel()) {
		uint32_t retries = MSPM0_CLK_WAIT_RETRIES;

		do {
			ret = syscon_read_reg(sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);
			if (ret < 0) {
				return ret;
			}

			if (!!(clkstatus & mask) == set) {
				return 0;
			}
		} while (retries--);

	} else {
		uint32_t start_cycles = k_cycle_get_32();
		uint32_t timeout_cycles = k_us_to_cyc_ceil32(timeout_us);

		do {
			ret = syscon_read_reg(sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);
			if (ret < 0) {
				return ret;
			}

			if (!!(clkstatus & mask) == set) {
				return 0;
			}
		} while ((k_cycle_get_32() - start_cycles) < timeout_cycles);
	}

	return -ETIMEDOUT;
}

/* Only 32/4 MHz supported; 16/24 MHz needs board trim we can't source. */
static int clock_mspm0_set_rate(const struct device *dev, clock_control_subsys_t sys,
				clock_control_subsys_rate_t rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	uint32_t freq = *(uint32_t *)rate;
	uint32_t clkstatus;
	int ret;

	if (sys_clock->clk != MSPM0_CLOCK_SYSOSC) {
		return -ENOTSUP;
	}

	ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);
	if (ret < 0) {
		return ret;
	}
	if (clkstatus & (SYSCTL_CLKSTATUS_CURMCLKSEL | SYSCTL_CLKSTATUS_HSCLKMUX)) {
		return -EBUSY;
	}

	switch (freq) {
	case MHZ(32):
		return syscon_update_bits(
			cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET, SYSCTL_SYSOSCCFG_FREQ,
			FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, SYSCTL_SYSOSCCFG_FREQ_BASE));

	case MHZ(4):
		return syscon_update_bits(
			cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET, SYSCTL_SYSOSCCFG_FREQ,
			FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, SYSCTL_SYSOSCCFG_FREQ_4M));

	default:
		return -ENOTSUP;
	}
}

static int clock_mspm0_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		return syscon_update_bits(cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET,
					  SYSCTL_SYSOSCCFG_DISABLE, 0);

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		return syscon_update_bits(cfg->sysctl, SYSCTL_GENCLKEN_OFFSET,
					  SYSCTL_GENCLKEN_MFPCLKEN, SYSCTL_GENCLKEN_MFPCLKEN);
#endif /* DT_MFPCLK_OKAY */

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL: {
		struct mspm0_sys_clock sysosc_subsys = {.clk = MSPM0_CLOCK_SYSOSC};
		uint32_t rate = MHZ(32);
		int ret = clock_mspm0_set_rate(dev, (clock_control_subsys_t)&sysosc_subsys, &rate);

		if (ret < 0) {
			return ret;
		}

		ret = syscon_update_bits(cfg->sysctl, SYSCTL_HSCLKEN_OFFSET,
					 SYSCTL_HSCLKEN_SYSPLLEN, SYSCTL_HSCLKEN_SYSPLLEN);
		if (ret < 0) {
			return ret;
		}
		return clock_mspm0_wait_clkstatus(cfg->sysctl, SYSCTL_CLKSTATUS_SYSPLLGOOD, true,
						  MSPM0_CLK_WAIT_TIMEOUT_US);
	}
#endif /* DT_SYSPLL_OKAY */

	default:
		return -ENOTSUP;
	}
}

static int clock_mspm0_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		return syscon_update_bits(cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET,
					  SYSCTL_SYSOSCCFG_DISABLE, SYSCTL_SYSOSCCFG_DISABLE);

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		return syscon_update_bits(cfg->sysctl, SYSCTL_GENCLKEN_OFFSET,
					  SYSCTL_GENCLKEN_MFPCLKEN, 0);
#endif /* DT_MFPCLK_OKAY */

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL: {
		int ret = syscon_update_bits(cfg->sysctl, SYSCTL_HSCLKEN_OFFSET,
					     SYSCTL_HSCLKEN_SYSPLLEN, 0);

		if (ret < 0) {
			return ret;
		}
		return clock_mspm0_wait_clkstatus(cfg->sysctl, SYSCTL_CLKSTATUS_SYSPLLOFF, true,
						  MSPM0_CLK_WAIT_TIMEOUT_US);
	}
#endif /* DT_SYSPLL_OKAY */

	default:
		return -ENOTSUP;
	}
}

static enum clock_control_status clock_mspm0_get_status(const struct device *dev,
							clock_control_subsys_t sys)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC: {
		uint32_t sysosccfg;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET, &sysosccfg);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		if (sysosccfg & SYSCTL_SYSOSCCFG_DISABLE) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		return CLOCK_CONTROL_STATUS_ON;
	}

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK: {
		uint32_t genclken;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_GENCLKEN_OFFSET, &genclken);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		if (genclken & SYSCTL_GENCLKEN_MFPCLKEN) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_OFF;
	}
#endif /* DT_MFPCLK_OKAY */

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL: {
		uint32_t clkstatus;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_SYSPLLOFF) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_SYSPLLGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
	}
#endif /* DT_SYSPLL_OKAY */

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK: {
		uint32_t clkstatus;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_HFCLKOFF) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_HFCLKGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
	}
#endif /* DT_HFCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK: {
		uint32_t clkstatus;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_HSCLKDEAD) {
			return CLOCK_CONTROL_STATUS_OFF;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_HSCLKGOOD) {
			return CLOCK_CONTROL_STATUS_ON;
		}
		return CLOCK_CONTROL_STATUS_STARTING;
	}
#endif /* DT_HSCLK_OKAY */

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK: {
		struct mspm0_sys_clock src_subsys;

#if DT_SYSPLL_OKAY
		uint32_t genclkcfg;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_GENCLKCFG_OFFSET, &genclkcfg);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		src_subsys.clk = (genclkcfg & SYSCTL_GENCLKCFG_CANCLKSRC) ? MSPM0_CLOCK_SYSPLL
									  : MSPM0_CLOCK_HFCLK;
#else
		src_subsys.clk = MSPM0_CLOCK_HFCLK;
#endif /* DT_SYSPLL_OKAY */
		return clock_mspm0_get_status(dev, (clock_control_subsys_t)&src_subsys);
	}
#endif /* DT_CANCLK_OKAY */

	case MSPM0_CLOCK_LFCLK: {
		uint32_t clkstatus;
		uint32_t mux;

		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return CLOCK_CONTROL_STATUS_UNKNOWN;
		}
		mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, clkstatus);

		switch (mux) {
		case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC:
			if (clkstatus & SYSCTL_CLKSTATUS_LFOSCGOOD) {
				return CLOCK_CONTROL_STATUS_ON;
			}

			return CLOCK_CONTROL_STATUS_OFF;

		case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFXT:
			if (clkstatus & SYSCTL_CLKSTATUS_LFXTGOOD) {
				return CLOCK_CONTROL_STATUS_ON;
			} else if (clkstatus & SYSCTL_CLKSTATUS_LFCLKFAIL) {
				return CLOCK_CONTROL_STATUS_OFF;
			}

			return CLOCK_CONTROL_STATUS_UNKNOWN;

		default:
			if (clkstatus & SYSCTL_CLKSTATUS_LFCLKFAIL) {
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

static int clock_mspm0_sysosc_rate(const struct device *dev, uint32_t *rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t sysosccfg;
	uint32_t freq_field;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET, &sysosccfg);

	if (ret < 0) {
		return ret;
	}

	freq_field = FIELD_GET(SYSCTL_SYSOSCCFG_FREQ, sysosccfg);

	if (freq_field == SYSCTL_SYSOSCCFG_FREQ_4M) {
		*rate = MHZ(4);
		return 0;
	}

#if defined(MSPM_SYSCTL_HAS_SYSOSC_USERTRIM)
	if (freq_field == SYSCTL_SYSOSCCFG_FREQ_USERTRIM) {
		uint32_t sysosctrimuser;

		ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSOSCTRIMUSER_OFFSET, &sysosctrimuser);
		if (ret < 0) {
			return ret;
		}

		if (FIELD_GET(SYSCTL_SYSOSCTRIMUSER_FREQ, sysosctrimuser) ==
		    SYSCTL_SYSOSCTRIMUSER_FREQ_24M) {
			*rate = MHZ(24);
		} else {
			*rate = MHZ(16);
		}
		return 0;
	}
#endif /* MSPM_SYSCTL_HAS_SYSOSC_USERTRIM */

	*rate = MHZ(32);
	return 0;
}

static int clock_mspm0_lfclk_rate(const struct device *dev, uint32_t *rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t clkstatus;
	uint32_t mux;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

	if (ret < 0) {
		return ret;
	}
	mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, clkstatus);

	switch (mux) {
#if DT_LFXT_OKAY
	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFXT:
		*rate = DT_PROP(DT_LFXT, clock_frequency);
		return 0;
#endif /* DT_LFXT_OKAY */

#if DT_LFCLK_IN_OKAY
	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFCLKIN:
		*rate = DT_PROP(DT_LFCLK_IN, clock_frequency);
		return 0;
#endif /* DT_LFCLK_IN_OKAY */

	case SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC:
	default:
		*rate = DT_PROP(DT_LFOSC, clock_frequency);
		return 0;
	}
}

#if DT_HFCLK_OKAY
static int clock_mspm0_hfclk_rate(const struct device *dev, uint32_t *rate)
{
#if DT_HFXT_OKAY && DT_HFCLK_IN_OKAY
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t hsclken;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_HSCLKEN_OFFSET, &hsclken);

	if (ret < 0) {
		return ret;
	}
	if (hsclken & SYSCTL_HSCLKEN_USEEXTHFCLK) {
		*rate = DT_PROP(DT_HFCLK_IN, clock_frequency);
	} else {
		*rate = DT_PROP(DT_HFXT, clock_frequency);
	}
#else
	*rate = MSPM0_HFCLK_FREQ;
#endif
	return 0;
}
#endif /* DT_HFCLK_OKAY */

#if DT_SYSPLL_OKAY
static int clock_mspm0_syspll_ref_rate(const struct device *dev, uint32_t *rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t syspllcfg0;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET, &syspllcfg0);

	if (ret < 0) {
		return ret;
	}

	if (syspllcfg0 & SYSCTL_SYSPLLCFG0_SYSPLLREF) {
#if DT_HFCLK_OKAY
		return clock_mspm0_hfclk_rate(dev, rate);
#else
		return -ENOTSUP;
#endif /* DT_HFCLK_OKAY */
	}

	return clock_mspm0_sysosc_rate(dev, rate);
}

/* fVCO = ref_rate / PDIV * QDIV, from the live syspllcfg1 dividers. */
static int clock_mspm0_syspll_fvco(const struct device *dev, uint32_t *fvco)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t syspllcfg1;
	uint32_t pdiv;
	uint32_t qdiv;
	uint32_t ref_rate;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSPLLCFG1_OFFSET, &syspllcfg1);

	if (ret < 0) {
		return ret;
	}
	pdiv = 1U << FIELD_GET(SYSCTL_SYSPLLCFG1_PDIV, syspllcfg1);
	qdiv = FIELD_GET(SYSCTL_SYSPLLCFG1_QDIV, syspllcfg1) + 1;

	ret = clock_mspm0_syspll_ref_rate(dev, &ref_rate);
	if (ret < 0) {
		return ret;
	}

	*fvco = (ref_rate * qdiv) / pdiv;
	return 0;
}

/* SYSPLL output rate (CLK0/CLK1 = fVCO/RDIVCLKx, CLK2X = 2*fVCO/RDIVCLK2X); -ENODEV if disabled. */
static int clock_mspm0_syspll_output_rate(const struct device *dev, uint32_t clk, uint32_t *rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t syspllcfg0;
	uint32_t fvco;
	uint32_t rdiv;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET, &syspllcfg0);

	if (ret < 0) {
		return ret;
	}

	switch (clk) {
	case MSPM0_CLOCK_SYSPLL_CLK0:
		if (!(syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK0)) {
			return -ENODEV;
		}
		rdiv = (FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK0, syspllcfg0) + 1) * 2;
		break;

	case MSPM0_CLOCK_SYSPLL_CLK1:
		if (!(syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK1)) {
			return -ENODEV;
		}
		rdiv = (FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK1, syspllcfg0) + 1) * 2;
		break;

	case MSPM0_CLOCK_SYSPLL_CLK2X:
		if (!(syspllcfg0 & SYSCTL_SYSPLLCFG0_ENABLECLK2X)) {
			return -ENODEV;
		}
		rdiv = FIELD_GET(SYSCTL_SYSPLLCFG0_RDIVCLK2X, syspllcfg0) + 1;
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
#endif /* DT_SYSPLL_OKAY */

static int clock_mspm0_get_rate(const struct device *dev, clock_control_subsys_t sys,
				uint32_t *rate)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_SYSOSC:
		return clock_mspm0_sysosc_rate(dev, rate);

	case MSPM0_CLOCK_LFCLK:
		return clock_mspm0_lfclk_rate(dev, rate);

	case MSPM0_CLOCK_MCLK: {
		uint32_t clkstatus;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return ret;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_CURMCLKSEL) {
			/* MCLK switched to LFCLK (PM low-power state) */
			ret = clock_mspm0_lfclk_rate(dev, rate);
			if (ret < 0) {
				return ret;
			}
#if DT_HSCLK_OKAY
		} else if (clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX) {
			/* MCLK currently sourced from HSCLK */
			struct mspm0_sys_clock hsclk_subsys = {.clk = MSPM0_CLOCK_HSCLK};

			ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hsclk_subsys,
						   rate);
			if (ret < 0) {
				return ret;
			}
#endif /* DT_HSCLK_OKAY */
		} else {
			/* MCLK switched to SYSOSC */
			ret = clock_mspm0_sysosc_rate(dev, rate);
			if (ret < 0) {
				return ret;
			}
			if (*rate == MHZ(4)) {
				*rate /= MSPM0_MCLK_DIV;
			}
		}
		break;
	}

#if MSPM0_HAS_MCLK_DIV2_DIV4
	case MSPM0_CLOCK_ULPCLK:
	case MSPM0_CLOCK_MCLK_DIV_2:
	case MSPM0_CLOCK_MCLK_DIV_4: {
		struct mspm0_sys_clock mclk_subsys = {.clk = MSPM0_CLOCK_MCLK};
		uint32_t mclk_rate;
		int ret =
			clock_mspm0_get_rate(dev, (clock_control_subsys_t)&mclk_subsys, &mclk_rate);
		if (ret < 0) {
			return ret;
		}

		if (sys_clock->clk == MSPM0_CLOCK_MCLK_DIV_2) {
			*rate = mclk_rate / MSPM0_MCLK_DIV_2;
		} else {
			*rate = mclk_rate / MSPM0_MCLK_DIV_4;
		}

		break;
	}
#else
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
#endif /* MSPM0_HAS_MCLK_DIV2_DIV4 */

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		*rate = MHZ(4);
		break;
#endif /* DT_MFPCLK_OKAY */

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK: {
		uint32_t genclkcfg;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_GENCLKCFG_OFFSET, &genclkcfg);

		if (ret < 0) {
			return ret;
		}
		if (genclkcfg & SYSCTL_GENCLKCFG_CANCLKSRC) {
#if DT_SYSPLL_OKAY
			ret = clock_mspm0_syspll_output_rate(dev, MSPM0_CLOCK_SYSPLL_CLK1, rate);

			if (ret < 0) {
				return ret;
			}
#else
			return -ENOTSUP;
#endif
		} else {
#if DT_HFCLK_OKAY
			struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};

			ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
						   rate);
			if (ret < 0) {
				return ret;
			}
#else
			return -ENOTSUP;
#endif
		}
		break;
	}
#endif /* DT_CANCLK_OKAY */

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK:
		return clock_mspm0_hfclk_rate(dev, rate);
#endif /* DT_HFCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK: {
		uint32_t clkstatus;
		int ret = syscon_read_reg(cfg->sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

		if (ret < 0) {
			return ret;
		}
		if (clkstatus & SYSCTL_CLKSTATUS_CURHSCLKSEL) {
#if DT_HFCLK_OKAY
			/* HSCLK currently sourced from HFCLK */
			struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};

			ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys,
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
			uint32_t syspllcfg0;
			uint32_t clk;

			ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET, &syspllcfg0);
			if (ret < 0) {
				return ret;
			}
			clk = (syspllcfg0 & SYSCTL_SYSPLLCFG0_MCLK2XVCO) ? MSPM0_CLOCK_SYSPLL_CLK2X
									 : MSPM0_CLOCK_SYSPLL_CLK0;
			ret = clock_mspm0_syspll_output_rate(dev, clk, rate);
			if (ret < 0) {
				return ret;
			}
#else
			/* SYSOSC feeds HSCLK when no SYSPLL is present */
			ret = clock_mspm0_sysosc_rate(dev, rate);
			if (ret < 0) {
				return ret;
			}
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
/* Bin floopin_hz into its factory-trimmed range and (re)load syspllparam0/1 accordingly. */
static int clock_mspm0_syspll_load_trim(const struct device *dev, uint32_t floopin_hz)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t param0;
	uint32_t param1;
	int ret;
#if defined(MSPM0_HAS_CPUSS)
	uint32_t old_cpuss_ctl;

	ret = syscon_read_reg(cfg->cpuss, CPUSS_CTL_OFFSET, &old_cpuss_ctl);
	if (ret < 0) {
		return ret;
	}
	ret = syscon_update_bits(cfg->cpuss, CPUSS_CTL_OFFSET, CPUSS_CTL_ICACHE, 0);
	if (ret < 0) {
		return ret;
	}
#endif /* MSPM0_HAS_CPUSS */

#if defined(MSPM0_CONFIGURE_WAIT_STATES)
	ret = syscon_update_bits(
		cfg->fri, FRI_FRDCNTL_OFFSET,
		FRI_FRDCNTL_TRIMENGRRWAIT | FRI_FRDCNTL_RWAIT | FRI_FRDCNTL_WS0_MODE,
		FIELD_PREP(FRI_FRDCNTL_TRIMENGRRWAIT, 2) | FIELD_PREP(FRI_FRDCNTL_RWAIT, 2));
	if (ret < 0) {
		return ret;
	}

	ret = syscon_update_bits(
		cfg->memcfg, MEMCFG_RAM_WS_CONFIG_OFFSET,
		MEMCFG_RAM_WS_CONFIG_ULL_WS_ENABLE | MEMCFG_RAM_WS_CONFIG_GLXMP_0_WS_ENABLE |
			MEMCFG_RAM_WS_CONFIG_GLXMP_1_WS_ENABLE |
			MEMCFG_RAM_WS_CONFIG_GLXMP_2_WS_ENABLE,
		MEMCFG_RAM_WS_CONFIG_ULL_WS_ENABLE | MEMCFG_RAM_WS_CONFIG_GLXMP_0_WS_ENABLE |
			MEMCFG_RAM_WS_CONFIG_GLXMP_1_WS_ENABLE |
			MEMCFG_RAM_WS_CONFIG_GLXMP_2_WS_ENABLE);
	if (ret < 0) {
		return ret;
	}
#endif /* defined(MSPM0_CONFIGURE_WAIT_STATES) */

	if (floopin_hz >= MHZ(4) && floopin_hz < MHZ(8)) {
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP0_4_8_OFFSET, &param0);
		if (ret == 0) {
			ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP1_4_8_OFFSET,
					      &param1);
		}
	} else if (floopin_hz >= MHZ(8) && floopin_hz < MHZ(16)) {
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP0_8_16_OFFSET, &param0);
		if (ret == 0) {
			ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP1_8_16_OFFSET,
					      &param1);
		}
	} else if (floopin_hz >= MHZ(16) && floopin_hz < MHZ(32)) {
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP0_16_32_OFFSET,
				      &param0);
		if (ret == 0) {
			ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP1_16_32_OFFSET,
					      &param1);
		}
	} else if (floopin_hz >= MHZ(32) && floopin_hz <= MHZ(48)) {
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP0_32_48_OFFSET,
				      &param0);
		if (ret == 0) {
			ret = syscon_read_reg(cfg->factoryregion, FACTORY_PLLSTARTUP1_32_48_OFFSET,
					      &param1);
		}
	} else {
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	ret = syscon_write_reg(cfg->sysctl, SYSCTL_SYSPLLPARAM0_OFFSET, param0);
	if (ret < 0) {
		return ret;
	}
	ret = syscon_write_reg(cfg->sysctl, SYSCTL_SYSPLLPARAM1_OFFSET, param1);
	if (ret < 0) {
		return ret;
	}

#if defined(MSPM0_REQUIRES_SYSPLLPARAM2)
	{
		uint32_t syspllparam2;
		uint32_t syspllldoctl;
		uint32_t syspllldoprog;

		ret = syscon_read_reg(cfg->factoryregion, FACTORY_SYSPLLPARAM2_OFFSET,
				      &syspllparam2);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_SYSPLLLDOCTL_OFFSET,
				      &syspllldoctl);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_read_reg(cfg->factoryregion, FACTORY_SYSPLLLDOPROG_OFFSET,
				      &syspllldoprog);
		if (ret < 0) {
			return ret;
		}

		ret = syscon_write_reg(cfg->sysctl, SYSCTL_SYSPLLPARAM2_OFFSET, syspllparam2);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_write_reg(cfg->sysctl, SYSCTL_SYSPLLLDOCTL_OFFSET, syspllldoctl);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_write_reg(cfg->sysctl, SYSCTL_SYSPLLLDOPROG_OFFSET, syspllldoprog);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* MSPM0_REQUIRES_SYSPLLPARAM2 */

#if defined(MSPM0_HAS_CPUSS)
	/* restore CPUSS flash cache control */
	ret = syscon_write_reg(cfg->cpuss, CPUSS_CTL_OFFSET, old_cpuss_ctl);
	if (ret < 0) {
		return ret;
	}
#endif /* MSPM0_HAS_CPUSS */

	return 0;
}

/* Switch SYSPLL reference at runtime, reloading the trim bin for the new fLOOPIN. */
static int clock_mspm0_configure_syspll(const struct device *dev, enum mspm0_clock_source source)
{
	const struct clock_mspm0_config *cfg = dev->config;
	uint32_t syspllcfg1;
	uint32_t pdiv;
	uint32_t ref_rate;
	int ret = syscon_read_reg(cfg->sysctl, SYSCTL_SYSPLLCFG1_OFFSET, &syspllcfg1);

	if (ret < 0) {
		return ret;
	}
	pdiv = 1U << FIELD_GET(SYSCTL_SYSPLLCFG1_PDIV, syspllcfg1);

	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK: {
		struct mspm0_sys_clock hfclk_subsys = {.clk = MSPM0_CLOCK_HFCLK};

		ret = clock_mspm0_get_rate(dev, (clock_control_subsys_t)&hfclk_subsys, &ref_rate);
		if (ret < 0) {
			return ret;
		}

		ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
					 SYSCTL_SYSPLLCFG0_SYSPLLREF, SYSCTL_SYSPLLCFG0_SYSPLLREF);
		if (ret < 0) {
			return ret;
		}
		break;
	}
#endif /* DT_HFCLK_OKAY */

	case MSPM0_CLOCK_SRC_SYSOSC:
		ret = clock_mspm0_sysosc_rate(dev, &ref_rate);
		if (ret < 0) {
			return ret;
		}
		ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
					 SYSCTL_SYSPLLCFG0_SYSPLLREF, 0);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return clock_mspm0_syspll_load_trim(dev, ref_rate / pdiv);
}

static int clock_mspm0_init_syspll(const struct device *dev)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock syspll_subsys = {.clk = MSPM0_CLOCK_SYSPLL};
	int ret;

	/* disable SYSPLL before programming it */
	ret = clock_mspm0_off(dev, (clock_control_subsys_t)&syspll_subsys);
	if (ret < 0) {
		return ret;
	}

	/* set predivider */
	ret = syscon_update_bits(
		cfg->sysctl, SYSCTL_SYSPLLCFG1_OFFSET, SYSCTL_SYSPLLCFG1_PDIV,
		FIELD_PREP(SYSCTL_SYSPLLCFG1_PDIV, SYSCTL_SYSPLLCFG1_PDIV_VAL(MSPM0_SYSPLL_PDIV)));
	if (ret < 0) {
		return ret;
	}

	/* set reference and factory trim via the shared runtime path */
#if DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_HFCLK)
	LOG_DBG("SYSPLL booting from HFCLK");
	ret = clock_mspm0_configure_syspll(dev, MSPM0_CLOCK_SRC_HFCLK);
#else
	LOG_DBG("SYSPLL booting from SYSOSC");
	ret = clock_mspm0_configure_syspll(dev, MSPM0_CLOCK_SRC_SYSOSC);
#endif /* DT_SAME_NODE(DT_SYSPLL_CLOCKS_CTRL, DT_HFCLK) */
	if (ret < 0) {
		return ret;
	}

	/* set QDIV */
	ret = syscon_update_bits(
		cfg->sysctl, SYSCTL_SYSPLLCFG1_OFFSET, SYSCTL_SYSPLLCFG1_QDIV,
		FIELD_PREP(SYSCTL_SYSPLLCFG1_QDIV, SYSCTL_SYSPLLCFG1_QDIV_VAL(MSPM0_SYSPLL_QDIV)));
	if (ret < 0) {
		return ret;
	}

	/* set syspllclk2x divider */
	if (MSPM0_SYSPLL_HAS_CLK2X) {
		ret = syscon_update_bits(
			cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
			SYSCTL_SYSPLLCFG0_RDIVCLK2X | SYSCTL_SYSPLLCFG0_ENABLECLK2X,
			SYSCTL_SYSPLLCFG0_ENABLECLK2X |
				FIELD_PREP(
					SYSCTL_SYSPLLCFG0_RDIVCLK2X,
					SYSCTL_SYSPLLCFG0_RDIVCLK2X_VAL(MSPM0_SYSPLL_CLK2X_DIV)));
	} else {
		ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
					 SYSCTL_SYSPLLCFG0_ENABLECLK2X, 0);
	}
	if (ret < 0) {
		return ret;
	}

	/* set syspllclk1 divider */
	if (MSPM0_SYSPLL_HAS_CLK1) {
		ret = syscon_update_bits(
			cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
			SYSCTL_SYSPLLCFG0_RDIVCLK1 | SYSCTL_SYSPLLCFG0_ENABLECLK1,
			SYSCTL_SYSPLLCFG0_ENABLECLK1 |
				FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK1,
					   SYSCTL_SYSPLLCFG0_RDIVCLK1_VAL(MSPM0_SYSPLL_CLK1_DIV)));
	} else {
		ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
					 SYSCTL_SYSPLLCFG0_ENABLECLK1, 0);
	}
	if (ret < 0) {
		return ret;
	}

	/* set syspllclk0 divider */
	if (MSPM0_SYSPLL_HAS_CLK0) {
		ret = syscon_update_bits(
			cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
			SYSCTL_SYSPLLCFG0_RDIVCLK0 | SYSCTL_SYSPLLCFG0_ENABLECLK0,
			SYSCTL_SYSPLLCFG0_ENABLECLK0 |
				FIELD_PREP(SYSCTL_SYSPLLCFG0_RDIVCLK0,
					   SYSCTL_SYSPLLCFG0_RDIVCLK0_VAL(MSPM0_SYSPLL_CLK0_DIV)));
	} else {
		ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET,
					 SYSCTL_SYSPLLCFG0_ENABLECLK0, 0);
	}
	if (ret < 0) {
		return ret;
	}

#if DT_HSCLK_OKAY && DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) &&                                \
	DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL)
	ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSPLLCFG0_OFFSET, SYSCTL_SYSPLLCFG0_MCLK2XVCO,
				 MSPM0_SYSPLL_HAS_CLK2X ? SYSCTL_SYSPLLCFG0_MCLK2XVCO : 0);
	if (ret < 0) {
		return ret;
	}
#endif

	/* enable SYSPLL */
	return clock_mspm0_on(dev, (clock_control_subsys_t)&syspll_subsys);
}
#endif /* DT_SYSPLL_OKAY */

#if DT_HFCLK_OKAY
static int clock_mspm0_configure_hfclk(const struct device *sysctl, enum mspm0_clock_source source)
{
	uint32_t timeout_us = MSPM0_CLK_WAIT_TIMEOUT_US;
	int ret;

	switch (source) {
#if DT_HFXT_OKAY
	case MSPM0_CLOCK_SRC_HFXT:
		/* disable HFXT */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKEN_OFFSET, SYSCTL_HSCLKEN_HFXTEN, 0);
		if (ret < 0) {
			return ret;
		}

		/* update HFXT frequency range */
		ret = syscon_update_bits(sysctl, SYSCTL_HFCLKCLKCFG_OFFSET,
					 SYSCTL_HFCLKCLKCFG_HFXTRSEL,
					 FIELD_PREP(SYSCTL_HFCLKCLKCFG_HFXTRSEL, MSPM0_HFXT_RANGE));
		if (ret < 0) {
			return ret;
		}

		/* set HFXT startup time */
		ret = syscon_update_bits(sysctl, SYSCTL_HFCLKCLKCFG_OFFSET,
					 SYSCTL_HFCLKCLKCFG_HFXTTIME,
					 SYSCTL_HFCLKCLKCFG_HFXTTIME_VAL(MSPM0_HFXT_STARTUP_US));
		if (ret < 0) {
			return ret;
		}

		/* set HFXT input as HFCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKEN_OFFSET, SYSCTL_HSCLKEN_USEEXTHFCLK,
					 0);
		if (ret < 0) {
			return ret;
		}

		/* enable HFXT */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKEN_OFFSET, SYSCTL_HSCLKEN_HFXTEN,
					 SYSCTL_HSCLKEN_HFXTEN);
		if (ret < 0) {
			return ret;
		}

		/* enable HFXT startup monitor */
		ret = syscon_update_bits(sysctl, SYSCTL_HFCLKCLKCFG_OFFSET,
					 SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK,
					 SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK);
		if (ret < 0) {
			return ret;
		}

		timeout_us = MSPM0_XTAL_WAIT_TIMEOUT_US(MSPM0_HFXT_STARTUP_US);
		break;
#endif /* DT_HFXT_OKAY */

#if DT_HFCLK_IN_OKAY
	case MSPM0_CLOCK_SRC_HFCLK_IN:
		/* set external digital clock input as HFCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKEN_OFFSET, SYSCTL_HSCLKEN_USEEXTHFCLK,
					 SYSCTL_HSCLKEN_USEEXTHFCLK);
		if (ret < 0) {
			return ret;
		}
		break;
#endif /* DT_HFCLK_IN_OKAY */

	default:
		return -ENOTSUP;
	}

	/* wait for HFCLK to stabilize */
	return clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_HFCLKGOOD, true, timeout_us);
}
#endif /* DT_HFCLK_OKAY */

static int clock_mspm0_configure_lfclk(const struct device *sysctl, enum mspm0_clock_source source)
{
	uint32_t clkstatus;
	uint32_t mux;
	int ret = syscon_read_reg(sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

	if (ret < 0) {
		return ret;
	}
	mux = FIELD_GET(SYSCTL_CLKSTATUS_LFCLKMUX, clkstatus);

	switch (source) {
#if DT_LFXT_OKAY
	case MSPM0_CLOCK_SRC_LFXT: {
		/* cannot switch to LFXT if LFCLK_IN is being used */
		if (mux == SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFCLKIN) {
			return -ENOTSUP;
		}

		/* disable low power mode and set drive strength to lowest */
		ret = syscon_update_bits(sysctl, SYSCTL_LFCLKCFG_OFFSET,
					 SYSCTL_LFCLKCFG_LOWCAP | SYSCTL_LFCLKCFG_XT1DRIVE, 0);
		if (ret < 0) {
			return ret;
		}

		/* start LFXT */
		ret = syscon_write_reg(sysctl, SYSCTL_LFXTCTL_OFFSET,
				       FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) |
					       SYSCTL_LFXTCTL_STARTLFXT);
		if (ret < 0) {
			return ret;
		}

		/* wait for LFXT to stabilize */
		ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_LFXTGOOD, true,
						 MSPM0_XTAL_WAIT_TIMEOUT_US(MSPM0_LFXT_STARTUP_US));
		if (ret < 0) {
			return ret;
		}

		/* set LFCLK source as LFXT */
		ret = syscon_write_reg(sysctl, SYSCTL_LFXTCTL_OFFSET,
				       FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL) |
					       SYSCTL_LFXTCTL_SETUSELFXT);
		if (ret < 0) {
			return ret;
		}

		/* wait for lfosc to turn off */
		ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_LFOSCGOOD, false,
						 MSPM0_CLK_WAIT_TIMEOUT_US);
		if (ret < 0) {
			return ret;
		}
		break;
	}
#endif /* DT_LFXT_OKAY */

#if DT_LFCLK_IN_OKAY
	case MSPM0_CLOCK_SRC_LFCLK_IN:
		/* turn off LFXT */
		ret = syscon_write_reg(sysctl, SYSCTL_LFXTCTL_OFFSET,
				       FIELD_PREP(SYSCTL_LFXTCTL_KEY, SYSCTL_LFXTCTL_KEY_VAL));
		if (ret < 0) {
			return ret;
		}

		/* set LFCLK source as LFCLK_IN */
		ret = syscon_write_reg(sysctl, SYSCTL_EXLFCTL_OFFSET,
				       FIELD_PREP(SYSCTL_EXLFCTL_KEY, SYSCTL_EXLFCTL_KEY_VAL) |
					       SYSCTL_EXLFCTL_SETUSEEXLF);
		if (ret < 0) {
			return ret;
		}
		break;
#endif /* DT_LFCLK_IN_OKAY */

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

static int clock_mspm0_configure_mfpclk(const struct device *dev, const struct device *sysctl,
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
		ret = syscon_update_bits(sysctl, SYSCTL_GENCLKCFG_OFFSET,
					 SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV,
					 FIELD_PREP(SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV,
						    SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV_VAL(divider)));
		if (ret < 0) {
			return ret;
		}

		/* set HFCLK as MFPCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_GENCLKCFG_OFFSET,
					 SYSCTL_GENCLKCFG_MFPCLKSRC, SYSCTL_GENCLKCFG_MFPCLKSRC);
		if (ret < 0) {
			return ret;
		}
		break;
	}
#endif /* DT_HFCLK_OKAY */

	case MSPM0_CLOCK_SRC_SYSOSC:
		/* set SYSOSC as MFPCLK source */
		return syscon_update_bits(sysctl, SYSCTL_GENCLKCFG_OFFSET,
					  SYSCTL_GENCLKCFG_MFPCLKSRC, 0);

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* DT_MFPCLK_OKAY */

static int clock_mspm0_configure_mclk(const struct device *sysctl, enum mspm0_clock_source source)
{
	int ret;

#if !MSPM0_MCLK_CANNOT_USE_LFCLK
	ret = syscon_update_bits(sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_USELFCLK, 0);
	if (ret < 0) {
		return ret;
	}

	/* verify that MCLK is not sourced from LFCLK */
	ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_CURMCLKSEL, false,
					 MSPM0_CLK_WAIT_TIMEOUT_US);
	if (ret < 0) {
		return ret;
	}
#endif /* !MSPM0_MCLK_CANNOT_USE_LFCLK */

	ret = syscon_update_bits(sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_USEHSCLK, 0);
	if (ret < 0) {
		return ret;
	}

	/* verify that MCLK is not sourced from HSCLK */
	ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_HSCLKMUX, false,
					 MSPM0_CLK_WAIT_TIMEOUT_US);
	if (ret < 0) {
		return ret;
	}

	switch (source) {
	case MSPM0_CLOCK_SRC_SYSOSC: {
		/* nothing to do, we are already using system oscillator at this point */
		break;
	}

#if !MSPM0_MCLK_CANNOT_USE_LFCLK
	case MSPM0_CLOCK_SRC_LFCLK:
		/* use LFCLK as MCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_USELFCLK,
					 SYSCTL_MCLKCFG_USELFCLK);
		if (ret < 0) {
			return ret;
		}

		/* verify that MCLK is sourced from LFCLK */
		ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_CURMCLKSEL, true,
						 MSPM0_CLK_WAIT_TIMEOUT_US);
		if (ret < 0) {
			return ret;
		}
		break;
#endif

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_SRC_HSCLK:
		ret = syscon_update_bits(sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_USEHSCLK,
					 SYSCTL_MCLKCFG_USEHSCLK);
		if (ret < 0) {
			return ret;
		}

		/* verify that MCLK is sourced from HSCLK */
		ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_HSCLKMUX, true,
						 MSPM0_CLK_WAIT_TIMEOUT_US);
		if (ret < 0) {
			return ret;
		}
		break;
#endif /* DT_HSCLK_OKAY */

	default:
		return -ENOTSUP;
	}

	return 0;
}

#if DT_HSCLK_OKAY
static int clock_mspm0_configure_hsclk(const struct device *sysctl, enum mspm0_clock_source source)
{
	uint32_t clkstatus;
	bool mclk_uses_hsclk;
	int ret = syscon_read_reg(sysctl, SYSCTL_CLKSTATUS_OFFSET, &clkstatus);

	if (ret < 0) {
		return ret;
	}
	mclk_uses_hsclk = !!(clkstatus & SYSCTL_CLKSTATUS_HSCLKMUX);

	if (mclk_uses_hsclk) {
		ret = clock_mspm0_configure_mclk(sysctl, MSPM0_CLOCK_SRC_SYSOSC);

		if (ret < 0) {
			return ret;
		}
	}

	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK:
		/* set HFCLK as HSCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKCFG_OFFSET, SYSCTL_HSCLKCFG_HSCLKSEL,
					 SYSCTL_HSCLKCFG_HSCLKSEL);
		break;
#endif /* DT_HFCLK_OKAY */

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SRC_SYSPLL:
		/* set SYSPLL as HSCLK source */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKCFG_OFFSET, SYSCTL_HSCLKCFG_HSCLKSEL,
					 0);
		break;
#else
	case MSPM0_CLOCK_SRC_SYSOSC:
		/* set SYSOSC as HSCLK source (no SYSPLL present) */
		ret = syscon_update_bits(sysctl, SYSCTL_HSCLKCFG_OFFSET, SYSCTL_HSCLKCFG_HSCLKSEL,
					 0);
		break;
#endif /* DT_SYSPLL_OKAY */

	default:
		ret = -ENOTSUP;
		break;
	}

	/* verify that HSCLK started, unless the source itself was rejected above */
	if (ret == 0) {
		ret = clock_mspm0_wait_clkstatus(sysctl, SYSCTL_CLKSTATUS_HSCLKGOOD, true,
						 MSPM0_CLK_WAIT_TIMEOUT_US);
	}

	/* Restore MCLK to HSCLK regardless of whether the switch above
	 * succeeded -- MCLK was moved off HSCLK to make that switch safe, and
	 * must not be left stuck on SYSOSC just because the new source failed
	 * to come up.
	 */
	if (mclk_uses_hsclk) {
		int restore_ret = clock_mspm0_configure_mclk(sysctl, MSPM0_CLOCK_SRC_HSCLK);

		if (ret == 0) {
			ret = restore_ret;
		}
	}

	return ret;
}
#endif /* DT_HSCLK_OKAY */

static int clock_mspm0_init_mclk(const struct device *dev)
{
	const struct clock_mspm0_config *cfg = dev->config;
	int ret = 0;

#if MSPM0_HAS_MCLK_DIV2_DIV4
	/* configure MCLKDIVCFG (MCLK2 and MCLK4 dividers) */
	ret = syscon_update_bits(cfg->sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_MCLKDIVCFG,
				 FIELD_PREP(SYSCTL_MCLKCFG_MCLKDIVCFG, MSPM0_MCLKDIVCFG_VAL));
	if (ret < 0) {
		return ret;
	}
#else
	/* configure UDIV */
	ret = syscon_update_bits(
		cfg->sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_UDIV,
		FIELD_PREP(SYSCTL_MCLKCFG_UDIV, SYSCTL_MCLKCFG_UDIV_VAL(MSPM0_ULPCLK_DIV)));
	if (ret < 0) {
		return ret;
	}
#endif /* MSPM0_HAS_MCLK_DIV2_DIV4 */

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_SYSOSC) && (DT_SYSOSC_FREQ == 4000000)
	/* configure MDIV */
	ret = syscon_update_bits(
		cfg->sysctl, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_MDIV,
		FIELD_PREP(SYSCTL_MCLKCFG_MDIV, SYSCTL_MCLKCFG_MDIV_VAL(MSPM0_MCLK_DIV)));
	if (ret < 0) {
		return ret;
	}
#endif

#if DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK)
	LOG_DBG("MCLK booting from HSCLK");
	ret = clock_mspm0_configure_mclk(cfg->sysctl, MSPM0_CLOCK_SRC_HSCLK);
#elif DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_LFCLK) && !MSPM0_MCLK_CANNOT_USE_LFCLK
	LOG_DBG("MCLK booting from LFCLK");
	ret = clock_mspm0_configure_mclk(cfg->sysctl, MSPM0_CLOCK_SRC_LFCLK);
#else
	LOG_DBG("MCLK booting from SYSOSC");
	ret = clock_mspm0_configure_mclk(cfg->sysctl, MSPM0_CLOCK_SRC_SYSOSC);
#endif /* DT_SAME_NODE(DT_MCLK_CLOCKS_CTRL, DT_HSCLK) */

	if (ret < 0) {
		return ret;
	}

	return 0;
}

#if DT_CANCLK_OKAY
static int clock_mspm0_configure_canclk(const struct device *sysctl, enum mspm0_clock_source source)
{
	switch (source) {
#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_SRC_HFCLK:
		/* set HFCLK as CANCLK source */
		return syscon_update_bits(sysctl, SYSCTL_GENCLKCFG_OFFSET,
					  SYSCTL_GENCLKCFG_CANCLKSRC, 0);
#endif

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SRC_SYSPLL:
		/* set SYSPLLCLK1 as CANCLK source */
		return syscon_update_bits(sysctl, SYSCTL_GENCLKCFG_OFFSET,
					  SYSCTL_GENCLKCFG_CANCLKSRC, SYSCTL_GENCLKCFG_CANCLKSRC);
#endif

	default:
		return -ENOTSUP;
	}
}
#endif /* DT_CANCLK_OKAY */

static int clock_mspm0_configure(const struct device *dev, clock_control_subsys_t sys, void *data)
{
	const struct clock_mspm0_config *cfg = dev->config;
	struct mspm0_sys_clock *sys_clock = (struct mspm0_sys_clock *)sys;
	enum mspm0_clock_source *source = (enum mspm0_clock_source *)data;
	bool disabled = false;
	int ret;

	if (clock_mspm0_get_status(dev, sys) == CLOCK_CONTROL_STATUS_ON) {
		disabled = clock_mspm0_off(dev, sys) == 0;
	}

	switch (sys_clock->clk) {
	case MSPM0_CLOCK_LFCLK:
		ret = clock_mspm0_configure_lfclk(cfg->sysctl, *source);
		break;

#if DT_SYSPLL_OKAY
	case MSPM0_CLOCK_SYSPLL:
		ret = clock_mspm0_configure_syspll(dev, *source);
		break;
#endif /* DT_SYSPLL_OKAY */

#if DT_HFCLK_OKAY
	case MSPM0_CLOCK_HFCLK:
		ret = clock_mspm0_configure_hfclk(cfg->sysctl, *source);
		break;
#endif /* DT_HFCLK_OKAY */

#if DT_MFPCLK_OKAY
	case MSPM0_CLOCK_MFPCLK:
		ret = clock_mspm0_configure_mfpclk(dev, cfg->sysctl, *source);
		break;
#endif /* DT_MFPCLK_OKAY */

#if DT_HSCLK_OKAY
	case MSPM0_CLOCK_HSCLK:
		ret = clock_mspm0_configure_hsclk(cfg->sysctl, *source);
		break;
#endif /* DT_HSCLK_OKAY */

	case MSPM0_CLOCK_MCLK:
		ret = clock_mspm0_configure_mclk(cfg->sysctl, *source);
		break;

#if DT_CANCLK_OKAY
	case MSPM0_CLOCK_CANCLK:
		ret = clock_mspm0_configure_canclk(cfg->sysctl, *source);
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
	const struct clock_mspm0_config *cfg = dev->config;
	int ret = 0;

	LOG_DBG("initializing MSPM0 clock tree");

	/* set SYSOSCFG frequency */
	ret = syscon_update_bits(cfg->sysctl, SYSCTL_SYSOSCCFG_OFFSET, SYSCTL_SYSOSCCFG_FREQ,
				 FIELD_PREP(SYSCTL_SYSOSCCFG_FREQ, MSPM0_SYSOSC_FREQ));
	if (ret < 0) {
		return ret;
	}

#if DT_HFCLK_OKAY
#if DT_SAME_NODE(DT_HFCLK_CLOCKS_CTRL, DT_HFXT)
	LOG_DBG("HFCLK booting from HFXT");
	ret = clock_mspm0_configure_hfclk(cfg->sysctl, MSPM0_CLOCK_SRC_HFXT);
#else
	LOG_DBG("HFCLK booting from HFCLK_IN");
	ret = clock_mspm0_configure_hfclk(cfg->sysctl, MSPM0_CLOCK_SRC_HFCLK_IN);
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
	ret = clock_mspm0_configure_hsclk(cfg->sysctl, MSPM0_CLOCK_SRC_HFCLK);
#elif DT_SAME_NODE(DT_HSCLK_CLOCKS_CTRL, DT_SYSPLL)
	LOG_DBG("HSCLK booting from SYSPLL");
	ret = clock_mspm0_configure_hsclk(cfg->sysctl, MSPM0_CLOCK_SRC_SYSPLL);
#else
	LOG_DBG("HSCLK booting from SYSOSC");
	ret = clock_mspm0_configure_hsclk(cfg->sysctl, MSPM0_CLOCK_SRC_SYSOSC);
#endif
	if (ret < 0) {
		LOG_ERR("failed to configure HSCLK: %d", ret);
		return ret;
	}
#endif /* DT_HSCLK_OKAY */

#if DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFXT)
	ret = clock_mspm0_configure_lfclk(cfg->sysctl, MSPM0_CLOCK_SRC_LFXT);
#elif DT_SAME_NODE(DT_LFCLK_CLOCKS_CTRL, DT_LFCLK_IN)
	ret = clock_mspm0_configure_lfclk(cfg->sysctl, MSPM0_CLOCK_SRC_LFCLK_IN);
#else
	/* LFCLK is by default sourced by LFOSC, which is not runtime configurable */
	ret = 0;
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
		ret = clock_mspm0_configure_mfpclk(dev, cfg->sysctl, MSPM0_CLOCK_SRC_HFCLK);
#else
		LOG_DBG("MFPCLK booting from SYSOSC");
		ret = clock_mspm0_configure_mfpclk(dev, cfg->sysctl, MSPM0_CLOCK_SRC_SYSOSC);
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
	ret = clock_mspm0_configure_canclk(cfg->sysctl, MSPM0_CLOCK_SRC_SYSPLL);
#else
	LOG_DBG("CANCLK booting from HFCLK");
	ret = clock_mspm0_configure_canclk(cfg->sysctl, MSPM0_CLOCK_SRC_HFCLK);
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

static const struct clock_mspm0_config clock_mspm0_cfg = {
	.sysctl = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(ckm), sysctl)),
#if defined(MSPM0_HAS_CPUSS)
	.cpuss = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(ckm), cpuss)),
#endif
#if DT_SYSPLL_OKAY
	.factoryregion = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(ckm), factoryregion)),
#endif
#if defined(MSPM0_CONFIGURE_WAIT_STATES)
	.fri = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(ckm), fri)),
	.memcfg = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(ckm), memcfg)),
#endif
};

DEVICE_DT_DEFINE(DT_NODELABEL(ckm), &clock_mspm0_init, NULL, NULL, &clock_mspm0_cfg, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mspm0_driver_api);
