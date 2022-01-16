/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/dt-bindings/clock/gd32_rcu_f4xx.h>

#include <gd32_rcu.h>
#include <gd32_pmu.h>

/** PMU DT node */
#define GD32_PMU_NODE DT_NODELABEL(pmu)
/** PMU DT clock */
#define GD32_PMU_CLOCK DT_CLOCKS_CELL(GD32_PMU_NODE, id)
/** If high-driver mode is enabled */
#define GD32_PMU_ENABLE_HIGH_DRIVER DT_PROP(GD32_PMU_NODE, high_driver_enable)

/** PLL DT node */
#define GD32_PLL_NODE DT_NODELABEL(pll)
/** IRC16M DT node */
#define GD32_IRC16M_NODE DT_NODELABEL(irc16m)
/** HXTAL DT node */
#define GD32_HXTAL_NODE DT_NODELABEL(hxtal)

/** CK_SYS clock DT node */
#define GD32_CK_SYS_NODE \
	DT_CLOCKS_CTLR(GD32_CLOCK_CONTROLLER_NODE)
/** If CK_SYS clock source is PLL evaluates to 1, else 0 */
#define GD32_CK_SYS_IS_PLL \
	DT_SAME_NODE(GD32_CK_SYS_NODE, GD32_PLL_NODE)
/** If CK_SYS clock source is IRC16M evaluates to 1, else 0 */
#define GD32_CK_SYS_IS_IRC16M \
	DT_SAME_NODE(GD32_CK_SYS_NODE, GD32_IRC16M_NODE)
/** If CK_SYS clock source is HXTAL evaluates to 1, else 0 */
#define GD32_CK_SYS_IS_HXTAL \
	DT_SAME_NODE(GD32_CK_SYS_NODE, GD32_HXTAL_NODE)

/** CK_PLL clock DT node */
#define GD32_CK_PLL_NODE \
	DT_CLOCKS_CTLR(GD32_PLL_NODE)
/** If CK_SYS clock source is PLL evaluates to 1, else 0 */
#define GD32_CK_PLL_IS_HXTAL \
	DT_SAME_NODE(GD32_CK_PLL_NODE, GD32_PLL_NODE)
/** If CK_SYS clock source is IRC16M evaluates to 1, else 0 */
#define GD32_CK_PLL_IS_IRC16M \
	DT_SAME_NODE(GD32_CK_PLL_NODE, GD32_IRC16M_NODE)

/** If PLL is enabled */
#define GD32_PLL_IS_ENABLED DT_NODE_HAS_STATUS(GD32_PLL_NODE, okay)
/** PLL register initial value */
#define GD32_PLL_CFG_INIT 0x24003010U
/** PLL prescaler */
#define GD32_PLL_PRESCALER DT_PROP(GD32_PLL_NODE, prescaler)
/** PLL N multiplier */
#define GD32_PLL_MUL_N DT_PROP(GD32_PLL_NODE, mul_n)
/** PLL P divider */
#define GD32_PLL_DIV_P DT_PROP(GD32_PLL_NODE, div_p)
/** PLL Q divider */
#define GD32_PLL_DIV_Q DT_PROP(GD32_PLL_NODE, div_q)
/** PLL configuration */
#define GD32_PLL_CFG \
	(GD32_PLL_PRESCALER | (GD32_PLL_MUL_N << 6U) | \
	 (((GD32_PLL_DIV_P >> 1U) - 1U) << 16U) | (GD32_PLL_DIV_Q << 24U))

/** If HXTAL is enabled */
#define GD32_HXTAL_IS_ENABLED DT_NODE_HAS_STATUS(GD32_HXTAL_NODE, okay)
/** HXTAL value */
#define GD32_HXTAL_FREQUENCY DT_PROP(GD32_HXTAL_NODE, clock_frequency)
/** If HXTAL bypass should be enabled or not */
#define GD32_HXTAL_BYPASS DT_PROP(GD32_HXTAL_NODE, hxtal_bypass)

/** AHB prescaler */
#define GD32_AHB_PRESCALER \
	DT_ENUM_IDX(GD32_CLOCK_CONTROLLER_NODE, ahb_prescaler)
/** APB1 prescaler */
#define GD32_APB1_PRESCALER \
	DT_ENUM_IDX(GD32_CLOCK_CONTROLLER_NODE, apb1_prescaler)
/** APB2 prescaler */
#define GD32_APB2_PRESCALER \
	DT_ENUM_IDX(GD32_CLOCK_CONTROLLER_NODE, apb2_prescaler)

/** RCU offset (from config field) */
#define GD32_RCU_CFG_OFFSET(cfg) (((cfg) >> 6U) & 0xFFU)
/** RCU configuration bit (from config field) */
#define GD32_RCU_CFG_BIT(cfg) ((cfg) & 0x1FU)
/** RCU register (from config field) */
#define GD32_RCU_CFG_REG(cfg) REG32(RCU + GD32_RCU_CFG_OFFSET(cfg))

/** AHB prescaler exponents */
static const uint8_t ahb_exp[] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U,
				  4U, 6U, 7U, 8U, 9U};
/** APB1 prescaler exponents */
static const uint8_t apb1_exp[] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
/** APB2 prescaler exponents */
static const uint8_t apb2_exp[] = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};

/** Reset RCU to initial state */
static void gd32_reset_rcu_state(void)
{
	/* select internal 16MHz RC oscillator */
	RCU_CTL |= RCU_CTL_IRC16MEN;
	RCU_CFG0 = 0U;
	while ((RCU_CFG0 & RCU_CFG0_SCSS) != RCU_SCSS_IRC16M)
		;

	/* reset HXTAL and PLL settings */
	RCU_CTL &= ~(RCU_CTL_PLLEN | RCU_CTL_HXTALEN | RCU_CTL_CKMEN |
		     RCU_CTL_HXTALBPS);
	RCU_PLL = GD32_PLL_CFG_INIT;

	/* disable all interrupts */
	RCU_INT = 0U;
}

#if GD32_HXTAL_IS_ENABLED
/** Configure and enable HXTAL */
static void gd32_configure_hxtal(void)
{
	if (GD32_HXTAL_BYPASS) {
		RCU_CTL |= RCU_CTL_HXTALBPS;
	}

	/* enable HXTAL and wait until stable */
	RCU_CTL |= RCU_CTL_HXTALEN;
	while ((RCU_CTL & RCU_CTL_HXTALSTB) == 0U)
		;
}
#endif /* GD32_HXTAL_IS_ENABLED */

#if GD32_PLL_IS_ENABLED
/** Configure and enable PLL */
static void gd32_configure_pll(void)
{
	/* configure PSC, N, P, Q and clock source */
	RCU_PLL = GD32_PLL_CFG;

	if (GD32_CK_PLL_IS_HXTAL) {
		RCU_PLL |= RCU_PLLSRC_HXTAL;
	} else {
		RCU_PLL |= RCU_PLLSRC_IRC16M;
	}

	/* enable PLL and wait until stable */
	RCU_CTL |= RCU_CTL_PLLEN;
	while((RCU_CTL & RCU_CTL_PLLSTB) == 0U)
		;
}
#endif /* GD32_PLL_IS_ENABLED */

#ifdef GD32_PMU_ENABLE_HIGH_DRIVER
/** Configure high-driver mode */
static void gd32_enable_high_drive()
{
	PMU_CTL |= PMU_CTL_HDEN;
	while((PMU_CS & PMU_CS_HDRF) == 0U)
		;

	PMU_CTL |= PMU_CTL_HDS;
	while((PMU_CS & PMU_CS_HDSRF) == 0U)
		;
}
#endif /* GD32_PMU_ENABLE_HIGH_DRIVER */

/** Configure CK_SYS clock source */
static void gd32_configure_ck_sys_source()
{
	RCU_CFG0 &= ~RCU_CFG0_SCS;

#if GD32_CK_SYS_IS_PLL
	RCU_CFG0 |= RCU_CKSYSSRC_PLLP;
	while((RCU_CFG0 & RCU_SCSS_PLLP) == 0U)
		;

#if GD32_CK_PLL_IS_HXTAL
	SystemCoreClock = GD32_HXTAL_FREQUENCY;
#else
	SystemCoreClock = 16000000UL;
#endif
	SystemCoreClock = ((SystemCoreClock / GD32_PLL_PRESCALER) *
			   GD32_PLL_MUL_N) / GD32_PLL_DIV_P;
#elif GD32_CK_SYS_IS_HXTAL
	RCU_CFG0 |= RCU_CKSYSSRC_HXTAL;
	while((RCU_CFG0 & RCU_SCSS_HXTAL) == 0U)
		;

	SystemCoreClock = GD32_HXTAL_FREQUENCY;
#elif GD32_CK_SYS_IS_IRC16M
	SystemCoreClock = 16000000UL;
#endif

	SystemCoreClock >>= ahb_exp[GD32_AHB_PRESCALER];
}

static int gd32_clock_control_on(const struct device *dev,
				 clock_control_subsys_t sys)
{
	uint16_t cfg = *(uint16_t *)sys;

	GD32_RCU_CFG_REG(cfg) |= BIT(GD32_RCU_CFG_BIT(cfg));

	return 0;
}

static int gd32_clock_control_off(const struct device *dev,
				  clock_control_subsys_t sys)
{
	uint16_t cfg = *(uint16_t *)sys;

	GD32_RCU_CFG_REG(cfg) &= ~BIT(GD32_RCU_CFG_BIT(cfg));

	return 0;
}

static int gd32_clock_control_get_rate(const struct device *dev,
				       clock_control_subsys_t sys,
				       uint32_t *rate)
{
	uint16_t cfg = *(uint16_t *)sys;

	switch (GD32_RCU_CFG_OFFSET(cfg)) {
	case GD32_AHB1EN_OFFSET:
		__fallthrough;
	case GD32_AHB2EN_OFFSET:
		__fallthrough;
	case GD32_AHB3EN_OFFSET:
		*rate = SystemCoreClock;
	case GD32_APB1EN_OFFSET:
		__fallthrough;
	case GD32_ADDAPB1EN_OFFSET:
		*rate = SystemCoreClock >> apb1_exp[GD32_APB1_PRESCALER];
		break;
	case GD32_APB2EN_OFFSET:
		*rate = SystemCoreClock >> apb2_exp[GD32_APB2_PRESCALER];
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status gd32_clock_control_get_status(
	const struct device *dev, clock_control_subsys_t sys)
{
	uint16_t cfg = *(uint16_t *)sys;

	if ((GD32_RCU_CFG_REG(cfg) & BIT(GD32_RCU_CFG_BIT(cfg))) == 0U) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	return CLOCK_CONTROL_STATUS_ON;
}

static struct clock_control_driver_api gd32_clock_control_api = {
	.on = gd32_clock_control_on,
	.off = gd32_clock_control_off,
	.get_rate = gd32_clock_control_get_rate,
	.get_status = gd32_clock_control_get_status,
};

static int gd32_clock_control_init(const struct device *dev)
{
	gd32_reset_rcu_state();

#if GD32_HXTAL_IS_ENABLED
	gd32_configure_hxtal();
#endif

#if GD32_PMU_ENABLE_HIGH_DRIVER
	/* enable PMU clock */
	GD32_RCU_CFG_REG(GD32_PMU_CLOCK) |= BIT(GD32_RCU_CFG_BIT(GD32_PMU_CLOCK));
	/* select LDO output voltage (PLL must be off) */
	PMU_CTL |= PMU_LDOVS_HIGH;
#endif

	/* configure bus clock prescalers */
	RCU_CFG0 |= CFG0_AHBPSC(GD32_AHB_PRESCALER + 7U) |
		    CFG0_APB1PSC(GD32_APB1_PRESCALER + 3U) |
		    CFG0_APB2PSC(GD32_APB2_PRESCALER + 3U);

#if GD32_PLL_IS_ENABLED
	gd32_configure_pll();
#endif

#ifdef GD32_PMU_ENABLE_HIGH_DRIVER
	gd32_enable_high_drive();
#endif

	gd32_configure_ck_sys_source();

	return 0;
}

DEVICE_DT_DEFINE(GD32_CLOCK_CONTROLLER_NODE, gd32_clock_control_init, NULL,
		 NULL, NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &gd32_clock_control_api);
