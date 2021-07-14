/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/gd32_clock_control.h>
#include <gd32e10x_rcu.h>
#include <gd32e10x.h>

#define DT_FREQ_M(x) ((x) * 1000 * 1000) // todo: use dts/common/freq.h

#define RCC_BASE DT_REG_ADDR_BY_IDX(DT_NODELABEL(rcc), 0)
#undef RCU_REG_VAL
#define RCU_REG_VAL(periph)             (REG32(RCC_BASE | periph ))

#define DT_CLOCK_SRC(clk) DT_PHANDLE_BY_IDX(DT_NODELABEL(clk), clocks, 0)

static inline int gd32_clock_reset()
{
	/* reset the RCU clock configuration to the default reset state */
	/* Set IRC8MEN bit */
	RCU_CTL |= RCU_CTL_IRC8MEN;

	/* Reset CFG0 and CFG1 registers */
	RCU_CFG0 = 0x00000000U;
	RCU_CFG1 = 0x00000000U;

	/* Reset HXTALEN, CKMEN, PLLEN, PLL1EN and PLL2EN bits */
	RCU_CTL &= ~(RCU_CTL_PLLEN |RCU_CTL_PLL1EN | RCU_CTL_PLL2EN | RCU_CTL_CKMEN | RCU_CTL_HXTALEN);
	/* disable all interrupts */
	RCU_INT = 0x00ff0000U;

	/* reset HXTALBPS bit */
	RCU_CTL &= ~(RCU_CTL_HXTALBPS);

	return 0;
}

static inline void clock_enable_hsi(void)
{
	uint32_t timeout = 0U;
	uint32_t stab_flag = 0U;

	/* enable IRC8M */
	RCU_CTL |= RCU_CTL_IRC8MEN;

	/* wait until IRC8M is stable or the startup time is longer than IRC8M_STARTUP_TIMEOUT */
	do{
		timeout++;
		stab_flag = (RCU_CTL & RCU_CTL_IRC8MSTB);
	}while((0U == stab_flag) && (IRC8M_STARTUP_TIMEOUT != timeout));

	/* if fail */
	if(0U == (RCU_CTL & RCU_CTL_IRC8MSTB)){
		while(1){
		}
	}
}

static inline void clock_enable_hse(void)
{
	uint32_t timeout = 0U;
	uint32_t stab_flag = 0U;
	/* enable HXTAL */
	RCU_CTL |= RCU_CTL_HXTALEN;

	/* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
	do{
		timeout++;
		stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
	}while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));

	/* if fail */
	if(0U == (RCU_CTL & RCU_CTL_HXTALSTB)){
		while(1){
		}
	}
}

static inline void clock_flash_config(void)
{
#if DT_PROP(DT_NODELABEL(rcc), clock_frequency) <= DT_FREQ_M(30)
	FMC_WS = (FMC_WS & (~FMC_WS_WSCNT)) | FMC_WAIT_STATE_0;
#elif DT_PROP(DT_NODELABEL(rcc), clock_frequency) <= DT_FREQ_M(60)
	FMC_WS = (FMC_WS & (~FMC_WS_WSCNT)) | FMC_WAIT_STATE_1;
#elif DT_PROP(DT_NODELABEL(rcc), clock_frequency) <= DT_FREQ_M(90)
	FMC_WS = (FMC_WS & (~FMC_WS_WSCNT)) | FMC_WAIT_STATE_2;
#elif DT_PROP(DT_NODELABEL(rcc), clock_frequency) <= DT_FREQ_M(120)
	FMC_WS = (FMC_WS & (~FMC_WS_WSCNT)) | FMC_WAIT_STATE_3;
#else
#error "GD32 RCC Freq too High"
#endif // clock freq select
}

static inline void clock_peripherals_config(void)
{
	// set ahb,apb1,apb2
	RCU_CFG0 |= UTIL_CAT(RCU_AHB_CKSYS_DIV, DT_PROP(DT_NODELABEL(rcc), ahb_prescaler));
	RCU_CFG0 |= UTIL_CAT(RCU_APB1_CKAHB_DIV, DT_PROP(DT_NODELABEL(rcc), apb1_prescaler));
	RCU_CFG0 |= UTIL_CAT(RCU_APB2_CKAHB_DIV, DT_PROP(DT_NODELABEL(rcc), apb2_prescaler));
}

// only 120M with hsi/hse is tested.
static inline void clock_pll_config(void)
{
	// set pll src
#if DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(pll), gd_gd32_hse_clock) // hse clk
	RCU_CFG0 |= RCU_PLLSRC_HXTAL_IRC48M;
#elif DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(pll), fixed_clock) // hsi enable
#else
#error "pll src not support."
#endif // DT_NODELABEL(clk_hse) == DT_PHANDLE_BY_IDX(DT_NODELABEL(pll), clocks, 0)

	// set pll MUL
	RCU_CFG0 &= ~(RCU_CFG0_PLLMF | RCU_CFG0_PLLMF_4 );
	RCU_CFG0 |= UTIL_CAT(RCU_PLL_MUL, DT_PROP(DT_NODELABEL(pll), mul));

#if DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(rcc), gd_gd32e103_pll_clock)

#if DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(pll), gd_gd32_hse_clock) // hse clk
	RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PREDV0SEL | RCU_CFG1_PLL1MF | RCU_CFG1_PREDV1 | RCU_CFG1_PREDV0);
	RCU_CFG1 |= RCU_PLLPRESRC_HXTAL;
	RCU_CFG1 |= RCU_PREDV0SRC_CKPLL1;
	RCU_CFG1 |= RCU_PLL1_MUL10;
	RCU_CFG1 |= RCU_PREDV1_DIV2;
	RCU_CFG1 |= RCU_PREDV0_DIV10;
#elif DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(pll), fixed_clock) // hsi enable
	// not need config
#else // not hse/hsi
#error "clock src not support yet."
#endif // hse/hsi switch

#else //DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(rcc), gd_gd32e103_pll_clock)
#error "config rcc not with pll is support yet."
#endif //DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(rcc), gd_gd32e103_pll_clock)

#if DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(pll), gd_gd32_hse_clock) // hse clk
	/* enable PLL1 */
	RCU_CTL |= RCU_CTL_PLL1EN;
	/* wait till PLL1 is ready */
	while((RCU_CTL & RCU_CTL_PLL1STB) == 0U){
	}
#endif// hse clk only

	/* enable PLL */
	RCU_CTL |= RCU_CTL_PLLEN;

	/* wait until PLL is stable */
	while(0U == (RCU_CTL & RCU_CTL_PLLSTB)){
	}
}

static inline void clock_pll_select(void)
{
	/* select PLL as system clock */
	RCU_CFG0 &= ~RCU_CFG0_SCS;
	RCU_CFG0 |= RCU_CKSYSSRC_PLL;

	/* wait until PLL is selected as system clock */
	while(0U == (RCU_CFG0 & RCU_SCSS_PLL)){
	}
}

/**
 * @brief Initialize clocks for the gd32
 *
 * This routine is called to enable and configure the clocks and PLL
 * of the soc on the board. It depends on the board definition.
 * This function is called on the startup and also to restore the config
 * when exiting for low power mode.
 *
 * @param dev clock device struct
 *
 * @return 0
 */
int gd32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	gd32_clock_reset();

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_hse_clock)
	clock_enable_hse();
#else
	clock_enable_hsi();
#endif // DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_hse_clock)

	clock_flash_config();

	clock_peripherals_config();

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32e103_pll_clock)
	clock_pll_config();

#if DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(rcc), gd_gd32e103_pll_clock)
	clock_pll_select();
#endif //DT_NODE_HAS_COMPAT(DT_CLOCK_SRC(rcc), gd_gd32e103_pll_clock)
#endif // DT_HAS_COMPAT_STATUS_OKAY(gd_gd32e103_pll_clock)

	return 0;
}

static inline int gd32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	struct gd32_pclken *pclken = (struct gd32_pclken *)(sub_system);

	switch (pclken->bus)
	{
	case GD32_CLOCK_BUS_AHB1:
    		RCU_REG_VAL(AHBEN_REG_OFFSET) |= pclken->enr;
		break;

	case GD32_CLOCK_BUS_APB1:
    		RCU_REG_VAL(APB1EN_REG_OFFSET) |= pclken->enr;
		break;

	case GD32_CLOCK_BUS_APB2:
    		RCU_REG_VAL(APB2EN_REG_OFFSET) |= pclken->enr;
		break;

	default:
		__ASSERT_NO_MSG(0);
		break;
	}

	return 0;
}

static inline int gd32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	//todo: work later
	return 0;
}

static int gd32_clock_control_get_subsys_rate(const struct device *clock,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	//todo: work later
	return 0;
}

static struct clock_control_driver_api gd32_clock_control_api = {
	.on = gd32_clock_control_on,
	.off = gd32_clock_control_off,
	.get_rate = gd32_clock_control_get_subsys_rate,
};

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    &gd32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_GD32_DEVICE_INIT_PRIORITY,
		    &gd32_clock_control_api);

BUILD_ASSERT(DT_PROP(DT_NODELABEL(rcc), clock_frequency)==CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
		"CPU Freq not same in `kconfig->CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC` & `devicetree->rcc->clock-frequency`");
