/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <gd32_pmu.h>
#include <gd32_rcu.h>

#define GD32_RCU_NODE    DT_NODELABEL(rcu)
#define GD32_PLL_NODE    DT_NODELABEL(pll)
#define GD32_HXTAL_NODE  DT_NODELABEL(hxtal)
#define GD32_IRC16M_NODE DT_NODELABEL(irc16m)

#define GD32_HXTAL_FREQ DT_PROP_OR(GD32_HXTAL_NODE, clock_frequency, 0)

/* System clock source from the rcu clocks phandle, IRC16M when absent */
#if DT_NODE_HAS_PROP(GD32_RCU_NODE, clocks)
#if DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR(GD32_RCU_NODE), gd_gd32_pll_clock)
#define GD32_CKSYS_IS_PLL 1
#elif DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR(GD32_RCU_NODE), gd_gd32_hxtal_clock)
#define GD32_CKSYS_IS_HXTAL 1
#define GD32_USES_HXTAL     1
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(GD32_RCU_NODE), GD32_IRC16M_NODE)
#define GD32_USES_IRC16M 1
#else
#error "rcu clocks must reference pll, hxtal or irc16m"
#endif
#endif

#if defined(GD32_CKSYS_IS_PLL)
#define GD32_CKSYS_SRC  RCU_CKSYSSRC_PLLP
#define GD32_CKSYS_SCSS RCU_SCSS_PLLP
#elif defined(GD32_CKSYS_IS_HXTAL)
#define GD32_CKSYS_SRC  RCU_CKSYSSRC_HXTAL
#define GD32_CKSYS_SCSS RCU_SCSS_HXTAL
#else
#define GD32_CKSYS_SRC  RCU_CKSYSSRC_IRC16M
#define GD32_CKSYS_SCSS RCU_SCSS_IRC16M
#endif

/* PLL input source from the pll clocks phandle */
#if defined(GD32_CKSYS_IS_PLL)
#if DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR(GD32_PLL_NODE), gd_gd32_hxtal_clock)
#define GD32_PLL_SRC     RCU_PLLSRC_HXTAL
#define GD32_PLL_IN_FREQ GD32_HXTAL_FREQ
#define GD32_USES_HXTAL  1
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(GD32_PLL_NODE), GD32_IRC16M_NODE)
#define GD32_PLL_SRC     RCU_PLLSRC_IRC16M
#define GD32_PLL_IN_FREQ IRC16M_VALUE
#define GD32_USES_IRC16M 1
#else
#error "pll clocks must reference hxtal or irc16m"
#endif
#endif

/* SYSCLK as programmed below, f(PLLP) = f(in) / PSC * N / P */
#if defined(GD32_CKSYS_IS_PLL)
/* Multiply before dividing so a non-integer VCO input does not lose precision */
#define GD32_PLL_VCO_FREQ                                                                          \
	((uint64_t)GD32_PLL_IN_FREQ * DT_PROP(GD32_PLL_NODE, pll_n) /                              \
	 DT_PROP(GD32_PLL_NODE, pll_psc))
#define GD32_CKSYS_FREQ (GD32_PLL_VCO_FREQ / DT_PROP(GD32_PLL_NODE, pll_p))
#elif defined(GD32_CKSYS_IS_HXTAL)
#define GD32_CKSYS_FREQ GD32_HXTAL_FREQ
#else
#define GD32_CKSYS_FREQ IRC16M_VALUE
#endif

#if defined(GD32_CKSYS_IS_PLL)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GD32_PLL_NODE),
	     "pll node must be enabled when it drives the system clock");
/* VCO operating ranges from the GD32F527 user manual (RCU_PLL) */
BUILD_ASSERT(IN_RANGE(GD32_PLL_IN_FREQ, MHZ(1) * DT_PROP(GD32_PLL_NODE, pll_psc),
		      MHZ(2) * DT_PROP(GD32_PLL_NODE, pll_psc)),
	     "PLL VCO input must be within 1-2 MHz");
BUILD_ASSERT(IN_RANGE(GD32_PLL_VCO_FREQ, MHZ(100), MHZ(500)),
	     "PLL VCO output must be within 100-500 MHz");
#endif

#if defined(GD32_USES_HXTAL)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GD32_HXTAL_NODE),
	     "hxtal node must be enabled when it feeds the clock tree");
#endif

#if defined(GD32_USES_IRC16M)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GD32_IRC16M_NODE),
	     "irc16m node must be enabled when it feeds the clock tree");
#endif

BUILD_ASSERT(GD32_CKSYS_FREQ <= MHZ(200), "system clock exceeds 200 MHz");

/* The kernel and clock_control_gd32 use the rcu clock-frequency as HCLK */
BUILD_ASSERT(GD32_CKSYS_FREQ / DT_PROP(GD32_RCU_NODE, ahb_prescaler) ==
		     DT_PROP(GD32_RCU_NODE, clock_frequency),
	     "rcu clock-frequency does not match the configured clock tree");

/* Bus limits from the GD32F527 datasheet */
BUILD_ASSERT(DT_PROP(GD32_RCU_NODE, clock_frequency) / DT_PROP(GD32_RCU_NODE, apb1_prescaler) <=
		     MHZ(50),
	     "APB1 clock exceeds 50 MHz");
BUILD_ASSERT(DT_PROP(GD32_RCU_NODE, clock_frequency) / DT_PROP(GD32_RCU_NODE, apb2_prescaler) <=
		     MHZ(100),
	     "APB2 clock exceeds 100 MHz");

struct gd32_pll_config {
#if defined(GD32_CKSYS_IS_PLL)
	uint32_t pll_psc;
	uint32_t pll_n;
	uint32_t pll_p;
	uint32_t pll_q;
#endif
	uint16_t ahb_psc;
	uint16_t apb1_psc;
	uint16_t apb2_psc;
};

/* Map divider value to HAL AHB prescaler encoding. */
static uint32_t gd32_ahb_psc_to_reg(uint16_t psc)
{
	switch (psc) {
	case 1U:
		return RCU_AHB_CKSYS_DIV1;
	case 2U:
		return RCU_AHB_CKSYS_DIV2;
	case 4U:
		return RCU_AHB_CKSYS_DIV4;
	case 8U:
		return RCU_AHB_CKSYS_DIV8;
	case 16U:
		return RCU_AHB_CKSYS_DIV16;
	case 64U:
		return RCU_AHB_CKSYS_DIV64;
	case 128U:
		return RCU_AHB_CKSYS_DIV128;
	case 256U:
		return RCU_AHB_CKSYS_DIV256;
	case 512U:
		return RCU_AHB_CKSYS_DIV512;
	default:
		return UINT32_MAX; /* invalid, not a valid encoding */
	}
}

/* Map divider value to HAL APB1 prescaler encoding. */
static uint32_t gd32_apb1_psc_to_reg(uint16_t psc)
{
	switch (psc) {
	case 1U:
		return RCU_APB1_CKAHB_DIV1;
	case 2U:
		return RCU_APB1_CKAHB_DIV2;
	case 4U:
		return RCU_APB1_CKAHB_DIV4;
	case 8U:
		return RCU_APB1_CKAHB_DIV8;
	case 16U:
		return RCU_APB1_CKAHB_DIV16;
	default:
		return UINT32_MAX; /* invalid, not a valid encoding */
	}
}

/* Map divider value to HAL APB2 prescaler encoding. */
static uint32_t gd32_apb2_psc_to_reg(uint16_t psc)
{
	switch (psc) {
	case 1U:
		return RCU_APB2_CKAHB_DIV1;
	case 2U:
		return RCU_APB2_CKAHB_DIV2;
	case 4U:
		return RCU_APB2_CKAHB_DIV4;
	case 8U:
		return RCU_APB2_CKAHB_DIV8;
	case 16U:
		return RCU_APB2_CKAHB_DIV16;
	default:
		return UINT32_MAX; /* invalid, not a valid encoding */
	}
}

static int gd32_pll_init(const struct device *dev)
{
	const struct gd32_pll_config *config = dev->config;
	const uint32_t ahb_psc = gd32_ahb_psc_to_reg(config->ahb_psc);
	const uint32_t apb1_psc = gd32_apb1_psc_to_reg(config->apb1_psc);
	const uint32_t apb2_psc = gd32_apb2_psc_to_reg(config->apb2_psc);

	if ((ahb_psc == UINT32_MAX) || (apb1_psc == UINT32_MAX) || (apb2_psc == UINT32_MAX)) {
		return -EINVAL;
	}

	/* No timeout: failures cannot be reported this early, so hang for debug */
#if defined(GD32_USES_HXTAL)
	/* An external clock on the HXTAL pin needs bypass before enabling */
	if (DT_PROP(GD32_HXTAL_NODE, hxtal_bypass)) {
		RCU_CTL |= RCU_CTL_HXTALBPS;
	}

	RCU_CTL |= RCU_CTL_HXTALEN;
	while ((RCU_CTL & RCU_CTL_HXTALSTB) == 0U) {
	}
#endif

	/* Enable the PMU clock and configure the LDO output voltage. */
	rcu_periph_clock_enable(RCU_PMU);
	PMU_CTL |= PMU_CTL_LDOVS;

	/* Configure AHB, APB1 and APB2 prescalers. */
	rcu_ahb_clock_config(ahb_psc);
	rcu_apb1_clock_config(apb1_psc);
	rcu_apb2_clock_config(apb2_psc);

#if defined(GD32_CKSYS_IS_PLL)
	/* Configure the main PLL. */
	if (rcu_pll_config(GD32_PLL_SRC, config->pll_psc, config->pll_n, config->pll_p,
			   config->pll_q) != SUCCESS) {
		return -EINVAL;
	}

	RCU_CTL |= RCU_CTL_PLLEN;
	while ((RCU_CTL & RCU_CTL_PLLSTB) == 0U) {
	}

	/* High-driver mode is needed for SYSCLK up to 200 MHz */
	PMU_CTL |= PMU_CTL_HDEN;
	while ((PMU_CS & PMU_CS_HDRF) == 0U) {
	}

	PMU_CTL |= PMU_CTL_HDS;
	while ((PMU_CS & PMU_CS_HDSRF) == 0U) {
	}
#endif

	rcu_system_clock_source_config(GD32_CKSYS_SRC);
	while (GD32_CKSYS_SCSS != (RCU_CFG0 & RCU_CFG0_SCSS)) {
	}

	/* Update SystemCoreClock from the actual SYSCLK/AHB configuration. */
	SystemCoreClockUpdate();

	return 0;
}

static const struct gd32_pll_config gd32_pll_config = {
#if defined(GD32_CKSYS_IS_PLL)
	.pll_psc = DT_PROP(GD32_PLL_NODE, pll_psc),
	.pll_n = DT_PROP(GD32_PLL_NODE, pll_n),
	.pll_p = DT_PROP(GD32_PLL_NODE, pll_p),
	.pll_q = DT_PROP(GD32_PLL_NODE, pll_q),
#endif
	.ahb_psc = DT_PROP(GD32_RCU_NODE, ahb_prescaler),
	.apb1_psc = DT_PROP(GD32_RCU_NODE, apb1_prescaler),
	.apb2_psc = DT_PROP(GD32_RCU_NODE, apb2_prescaler),
};

DEVICE_DT_DEFINE(DT_NODELABEL(rcu), gd32_pll_init, NULL, NULL, &gd32_pll_config, PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);
