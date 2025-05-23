/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <stm32_ll_system.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

/* Macros to fill up prescaler values */
#define z_ic_src_pll(v) LL_RCC_ICCLKSOURCE_PLL ## v
#define ic_src_pll(v) z_ic_src_pll(v)

#define z_hsi_divider(v) LL_RCC_HSI_DIV_ ## v
#define hsi_divider(v) z_hsi_divider(v)

#define z_ahb_prescaler(v) LL_RCC_AHB_DIV_ ## v
#define ahb_prescaler(v) z_ahb_prescaler(v)

#define z_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) z_apb1_prescaler(v)

#define z_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) z_apb2_prescaler(v)

#define z_apb4_prescaler(v) LL_RCC_APB4_DIV_ ## v
#define apb4_prescaler(v) z_apb4_prescaler(v)

#define z_apb5_prescaler(v) LL_RCC_APB5_DIV_ ## v
#define apb5_prescaler(v) z_apb5_prescaler(v)

#define z_timg_prescaler(v) LL_RCC_TIM_PRESCALER_ ## v
#define timg_prescaler(v) z_timg_prescaler(v)

#define PLL1_ID		1
#define PLL2_ID		2
#define PLL3_ID		3
#define PLL4_ID		4


static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

__unused
/** @brief returns the pll source frequency of given pll_id */
static uint32_t get_pllsrc_frequency(int pll_id)
{
	if ((IS_ENABLED(STM32_PLL_SRC_HSI) && pll_id == PLL1_ID) ||
	    (IS_ENABLED(STM32_PLL2_SRC_HSI) && pll_id == PLL2_ID) ||
	    (IS_ENABLED(STM32_PLL3_SRC_HSI) && pll_id == PLL3_ID) ||
	    (IS_ENABLED(STM32_PLL4_SRC_HSI) && pll_id == PLL4_ID)) {
		return STM32_HSI_FREQ;
	} else if ((IS_ENABLED(STM32_PLL_SRC_HSE) && pll_id == PLL1_ID) ||
		   (IS_ENABLED(STM32_PLL2_SRC_HSE) && pll_id == PLL2_ID) ||
		   (IS_ENABLED(STM32_PLL3_SRC_HSE) && pll_id == PLL3_ID) ||
		   (IS_ENABLED(STM32_PLL4_SRC_HSE) && pll_id == PLL4_ID)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "No PLL Source configured");
	return 0;
}

static uint32_t get_pllout_frequency(int pll_id)
{
	uint32_t pllsrc_freq = get_pllsrc_frequency(pll_id);
	int pllm_div;
	int plln_mul;
	int pllout_div1;
	int pllout_div2;

	switch (pll_id) {
#if defined(STM32_PLL1_ENABLED)
	case PLL1_ID:
		pllm_div = STM32_PLL1_M_DIVISOR;
		plln_mul = STM32_PLL1_N_MULTIPLIER;
		pllout_div1 = STM32_PLL1_P1_DIVISOR;
		pllout_div2 = STM32_PLL1_P2_DIVISOR;
		break;
#endif /* STM32_PLL1_ENABLED */
#if defined(STM32_PLL2_ENABLED)
	case PLL2_ID:
		pllm_div = STM32_PLL2_M_DIVISOR;
		plln_mul = STM32_PLL2_N_MULTIPLIER;
		pllout_div1 = STM32_PLL2_P1_DIVISOR;
		pllout_div2 = STM32_PLL2_P2_DIVISOR;
		break;
#endif /* STM32_PLL2_ENABLED */
#if defined(STM32_PLL3_ENABLED)
	case PLL3_ID:
		pllm_div = STM32_PLL3_M_DIVISOR;
		plln_mul = STM32_PLL3_N_MULTIPLIER;
		pllout_div1 = STM32_PLL3_P1_DIVISOR;
		pllout_div2 = STM32_PLL3_P2_DIVISOR;
		break;
#endif /* STM32_PLL3_ENABLED */
#if defined(STM32_PLL4_ENABLED)
	case PLL4_ID:
		pllm_div = STM32_PLL4_M_DIVISOR;
		plln_mul = STM32_PLL4_N_MULTIPLIER;
		pllout_div1 = STM32_PLL4_P1_DIVISOR;
		pllout_div2 = STM32_PLL4_P2_DIVISOR;
		break;
#endif /* STM32_PLL4_ENABLED */
	default:
		__ASSERT(0, "No PLL configured");
		return 0;
	}

	__ASSERT_NO_MSG(pllm_div && pllout_div1 && pllout_div2);

	return (pllsrc_freq / pllm_div) * plln_mul / (pllout_div1 * pllout_div2);
}

__unused uint32_t get_icout_frequency(uint32_t icsrc, int div)
{
	if (icsrc == LL_RCC_ICCLKSOURCE_PLL1) {
		return get_pllout_frequency(PLL1_ID) / div;
	} else if (icsrc == LL_RCC_ICCLKSOURCE_PLL2) {
		return get_pllout_frequency(PLL2_ID) / div;
	} else if (icsrc == LL_RCC_ICCLKSOURCE_PLL3) {
		return get_pllout_frequency(PLL3_ID) / div;
	} else if (icsrc == LL_RCC_ICCLKSOURCE_PLL4) {
		return get_pllout_frequency(PLL4_ID) / div;
	}

	__ASSERT(0, "No IC Source configured");
	return 0;
}

static uint32_t get_sysclk_frequency(void)
{
#if defined(STM32_SYSCLK_SRC_HSE)
	return STM32_HSE_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSI)
	return STM32_HSI_FREQ;
#elif defined(STM32_SYSCLK_SRC_IC2)
	return get_icout_frequency(LL_RCC_IC2_GetSource(), STM32_IC2_DIV);
#else
	__ASSERT(0, "No SYSCLK Source configured");
	return 0;
#endif

}


/** @brief Verifies clock is part of active clock configuration */
static int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_HCLK1) ||
	    (src_clk == STM32_SRC_HCLK2) ||
	    (src_clk == STM32_SRC_HCLK3) ||
	    (src_clk == STM32_SRC_HCLK4) ||
	    (src_clk == STM32_SRC_HCLK5) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK4) ||
	    (src_clk == STM32_SRC_PCLK5) ||
	    (src_clk == STM32_SRC_TIMG) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI_DIV) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1) && IS_ENABLED(STM32_PLL1_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2) && IS_ENABLED(STM32_PLL2_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3) && IS_ENABLED(STM32_PLL3_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL4) && IS_ENABLED(STM32_PLL4_ENABLED)) ||
	    ((src_clk == STM32_SRC_CKPER) && IS_ENABLED(STM32_CKPER_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC1) && IS_ENABLED(STM32_IC1_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC2) && IS_ENABLED(STM32_IC2_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC3) && IS_ENABLED(STM32_IC3_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC4) && IS_ENABLED(STM32_IC4_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC5) && IS_ENABLED(STM32_IC5_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC6) && IS_ENABLED(STM32_IC6_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC7) && IS_ENABLED(STM32_IC7_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC8) && IS_ENABLED(STM32_IC8_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC9) && IS_ENABLED(STM32_IC9_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC10) && IS_ENABLED(STM32_IC10_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC11) && IS_ENABLED(STM32_IC11_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC12) && IS_ENABLED(STM32_IC12_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC13) && IS_ENABLED(STM32_IC13_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC14) && IS_ENABLED(STM32_IC14_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC15) && IS_ENABLED(STM32_IC15_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC16) && IS_ENABLED(STM32_IC16_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC17) && IS_ENABLED(STM32_IC17_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC18) && IS_ENABLED(STM32_IC18_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC19) && IS_ENABLED(STM32_IC19_ENABLED)) ||
	    ((src_clk == STM32_SRC_IC20) && IS_ENABLED(STM32_IC20_ENABLED))) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	/* Set Run clock */
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		     pclken->enr);

	/* Set Low Power clock */
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus + STM32_CLOCK_LP_BUS_SHIFT,
		     pclken->enr);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	/* Clear Run clock */
	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		       pclken->enr);

	/* Clear Low Power clock */
	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus + STM32_CLOCK_LP_BUS_SHIFT,
		       pclken->enr);

	return 0;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		       STM32_DT_CLKSEL_MASK_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));
	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_DT_CLKSEL_REG_GET(pclken->enr),
		     STM32_DT_CLKSEL_VAL_GET(pclken->enr) <<
			STM32_DT_CLKSEL_SHIFT_GET(pclken->enr));

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sys);

	uint32_t sys_clock = get_sysclk_frequency();
	uint32_t ahb_clock = get_bus_clock(sys_clock, STM32_AHB_PRESCALER);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_SRC_SYSCLK:
		*rate = get_sysclk_frequency();
		break;
	case STM32_SRC_HCLK1:
	case STM32_SRC_HCLK2:
	case STM32_SRC_HCLK3:
	case STM32_SRC_HCLK4:
	case STM32_SRC_HCLK5:
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB3:
	case STM32_CLOCK_BUS_AHB4:
	case STM32_CLOCK_BUS_AHB5:
		*rate = ahb_clock;
		break;
	case STM32_SRC_PCLK1:
	case STM32_CLOCK_BUS_APB1:
	case STM32_CLOCK_BUS_APB1_2:
		*rate = get_bus_clock(ahb_clock, STM32_APB1_PRESCALER);
		break;
	case STM32_SRC_PCLK2:
	case STM32_CLOCK_BUS_APB2:
		*rate = get_bus_clock(ahb_clock, STM32_APB2_PRESCALER);
		break;
	case STM32_SRC_PCLK4:
	case STM32_CLOCK_BUS_APB4:
	case STM32_CLOCK_BUS_APB4_2:
		*rate = get_bus_clock(ahb_clock, STM32_APB4_PRESCALER);
		break;
	case STM32_SRC_PCLK5:
	case STM32_CLOCK_BUS_APB5:
		*rate = get_bus_clock(ahb_clock, STM32_APB5_PRESCALER);
		break;
#if defined(STM32_LSE_ENABLED)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif /* STM32_LSE_ENABLED */
#if defined(STM32_LSI_ENABLED)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif /* STM32_LSI_ENABLED */
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif /* STM32_HSE_ENABLED */
#if defined(STM32_HSI_ENABLED)
	case STM32_SRC_HSI:
		*rate = STM32_HSI_FREQ;
		break;
	case STM32_SRC_HSI_DIV:
		*rate = STM32_HSI_FREQ / STM32_HSI_DIVISOR;
		break;
#endif /* STM32_HSI_ENABLED */
	case STM32_SRC_PLL1:
		*rate = get_pllout_frequency(PLL1_ID);
		break;
	case STM32_SRC_PLL2:
		*rate = get_pllout_frequency(PLL2_ID);
		break;
	case STM32_SRC_PLL3:
		*rate = get_pllout_frequency(PLL3_ID);
		break;
	case STM32_SRC_PLL4:
		*rate = get_pllout_frequency(PLL4_ID);
		break;
#if defined(STM32_CKPER_ENABLED)
	case STM32_SRC_CKPER:
		*rate = LL_RCC_GetCLKPClockFreq(LL_RCC_CLKP_CLKSOURCE);
		break;
#endif /* STM32_CKPER_ENABLED */
#if defined(STM32_IC1_ENABLED)
	case STM32_SRC_IC1:
		*rate = get_icout_frequency(LL_RCC_IC1_GetSource(), STM32_IC1_DIV);
		break;
#endif /* STM32_IC1_ENABLED */
#if defined(STM32_IC2_ENABLED)
	case STM32_SRC_IC2:
		*rate = get_icout_frequency(LL_RCC_IC2_GetSource(), STM32_IC2_DIV);
		break;
#endif /* STM32_IC2_ENABLED */
#if defined(STM32_IC3_ENABLED)
	case STM32_SRC_IC3:
		*rate = get_icout_frequency(LL_RCC_IC3_GetSource(), STM32_IC3_DIV);
		break;
#endif /* STM32_IC3_ENABLED */
#if defined(STM32_IC4_ENABLED)
	case STM32_SRC_IC4:
		*rate = get_icout_frequency(LL_RCC_IC4_GetSource(), STM32_IC4_DIV);
		break;
#endif /* STM32_IC4_ENABLED */
#if defined(STM32_IC5_ENABLED)
	case STM32_SRC_IC5:
		*rate = get_icout_frequency(LL_RCC_IC5_GetSource(), STM32_IC5_DIV);
		break;
#endif /* STM32_IC5_ENABLED */
#if defined(STM32_IC6_ENABLED)
	case STM32_SRC_IC6:
		*rate = get_icout_frequency(LL_RCC_IC6_GetSource(), STM32_IC6_DIV);
		break;
#endif /* STM32_IC6_ENABLED */
#if defined(STM32_IC7_ENABLED)
	case STM32_SRC_IC7:
		*rate = get_icout_frequency(LL_RCC_IC7_GetSource(), STM32_IC7_DIV);
		break;
#endif /* STM32_IC7_ENABLED */
#if defined(STM32_IC8_ENABLED)
	case STM32_SRC_IC8:
		*rate = get_icout_frequency(LL_RCC_IC8_GetSource(), STM32_IC8_DIV);
		break;
#endif /* STM32_IC8_ENABLED */
#if defined(STM32_IC9_ENABLED)
	case STM32_SRC_IC9:
		*rate = get_icout_frequency(LL_RCC_IC9_GetSource(), STM32_IC9_DIV);
		break;
#endif /* STM32_IC9_ENABLED */
#if defined(STM32_IC10_ENABLED)
	case STM32_SRC_IC10:
		*rate = get_icout_frequency(LL_RCC_IC10_GetSource(), STM32_IC10_DIV);
		break;
#endif /* STM32_IC10_ENABLED */
#if defined(STM32_IC11_ENABLED)
	case STM32_SRC_IC11:
		*rate = get_icout_frequency(LL_RCC_IC11_GetSource(), STM32_IC11_DIV);
		break;
#endif /* STM32_IC11_ENABLED */
#if defined(STM32_IC12_ENABLED)
	case STM32_SRC_IC12:
		*rate = get_icout_frequency(LL_RCC_IC12_GetSource(), STM32_IC12_DIV);
		break;
#endif /* STM32_IC12_ENABLED */
#if defined(STM32_IC13_ENABLED)
	case STM32_SRC_IC13:
		*rate = get_icout_frequency(LL_RCC_IC13_GetSource(), STM32_IC13_DIV);
		break;
#endif /* STM32_IC13_ENABLED */
#if defined(STM32_IC14_ENABLED)
	case STM32_SRC_IC14:
		*rate = get_icout_frequency(LL_RCC_IC14_GetSource(), STM32_IC14_DIV);
		break;
#endif /* STM32_IC14_ENABLED */
#if defined(STM32_IC15_ENABLED)
	case STM32_SRC_IC15:
		*rate = get_icout_frequency(LL_RCC_IC15_GetSource(), STM32_IC15_DIV);
		break;
#endif /* STM32_IC15_ENABLED */
#if defined(STM32_IC16_ENABLED)
	case STM32_SRC_IC16:
		*rate = get_icout_frequency(LL_RCC_IC16_GetSource(), STM32_IC16_DIV);
		break;
#endif /* STM32_IC16_ENABLED */
#if defined(STM32_IC17_ENABLED)
	case STM32_SRC_IC17:
		*rate = get_icout_frequency(LL_RCC_IC17_GetSource(), STM32_IC17_DIV);
		break;
#endif /* STM32_IC17_ENABLED */
#if defined(STM32_IC18_ENABLED)
	case STM32_SRC_IC18:
		*rate = get_icout_frequency(LL_RCC_IC18_GetSource(), STM32_IC18_DIV);
		break;
#endif /* STM32_IC18_ENABLED */
#if defined(STM32_IC19_ENABLED)
	case STM32_SRC_IC19:
		*rate = get_icout_frequency(LL_RCC_IC19_GetSource(), STM32_IC19_DIV);
		break;
#endif /* STM32_IC19_ENABLED */
#if defined(STM32_IC20_ENABLED)
	case STM32_SRC_IC20:
		*rate = get_icout_frequency(LL_RCC_IC20_GetSource(), STM32_IC20_DIV);
		break;
#endif /* STM32_IC20_ENABLED */
	case STM32_SRC_TIMG:
		*rate = sys_clock / STM32_TIMG_PRESCALER;
		break;
	default:
		return -ENOTSUP;
	}

	if (pclken->div) {
		*rate /= (pclken->div + 1);
	}

	return 0;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == true) {
		/* Gated clocks */
		if ((sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus) & pclken->enr)
		    == pclken->enr) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else {
		/* Domain clock sources */
		if (enabled_clock(pclken->bus) == 0) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.get_status = stm32_clock_control_get_status,
	.configure = stm32_clock_control_configure,
};

/*
 * Unconditionally switch the system clock source to HSI.
 */
__unused
static void stm32_clock_switch_to_hsi(void)
{
	/* Enable HSI if not enabled */
	if (LL_RCC_HSI_IsReady() != 1) {
		/* Enable HSI */
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
		}
	}

	/* Set HSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

	LL_RCC_SetCpuClkSource(LL_RCC_CPU_CLKSOURCE_HSI);
	while (LL_RCC_GetCpuClkSource() != LL_RCC_CPU_CLKSOURCE_STATUS_HSI) {
	}
}

static int set_up_ics(void)
{
#if defined(STM32_IC1_ENABLED)
	LL_RCC_IC1_SetSource(ic_src_pll(STM32_IC1_PLL_SRC));
	LL_RCC_IC1_SetDivider(STM32_IC1_DIV);
	LL_RCC_IC1_Enable();
#endif

#if defined(STM32_IC2_ENABLED)
	LL_RCC_IC2_SetSource(ic_src_pll(STM32_IC2_PLL_SRC));
	LL_RCC_IC2_SetDivider(STM32_IC2_DIV);
	LL_RCC_IC2_Enable();
#endif

#if defined(STM32_IC3_ENABLED)
	LL_RCC_IC3_SetSource(ic_src_pll(STM32_IC3_PLL_SRC));
	LL_RCC_IC3_SetDivider(STM32_IC3_DIV);
	LL_RCC_IC3_Enable();
#endif

#if defined(STM32_IC4_ENABLED)
	LL_RCC_IC4_SetSource(ic_src_pll(STM32_IC4_PLL_SRC));
	LL_RCC_IC4_SetDivider(STM32_IC4_DIV);
	LL_RCC_IC4_Enable();
#endif

#if defined(STM32_IC5_ENABLED)
	LL_RCC_IC5_SetSource(ic_src_pll(STM32_IC5_PLL_SRC));
	LL_RCC_IC5_SetDivider(STM32_IC5_DIV);
	LL_RCC_IC5_Enable();
#endif

#if defined(STM32_IC6_ENABLED)
	LL_RCC_IC6_SetSource(ic_src_pll(STM32_IC6_PLL_SRC));
	LL_RCC_IC6_SetDivider(STM32_IC6_DIV);
	LL_RCC_IC6_Enable();
#endif

#if defined(STM32_IC7_ENABLED)
	LL_RCC_IC7_SetSource(ic_src_pll(STM32_IC7_PLL_SRC));
	LL_RCC_IC7_SetDivider(STM32_IC7_DIV);
	LL_RCC_IC7_Enable();
#endif

#if defined(STM32_IC8_ENABLED)
	LL_RCC_IC8_SetSource(ic_src_pll(STM32_IC8_PLL_SRC));
	LL_RCC_IC8_SetDivider(STM32_IC8_DIV);
	LL_RCC_IC8_Enable();
#endif

#if defined(STM32_IC9_ENABLED)
	LL_RCC_IC9_SetSource(ic_src_pll(STM32_IC9_PLL_SRC));
	LL_RCC_IC9_SetDivider(STM32_IC9_DIV);
	LL_RCC_IC9_Enable();
#endif

#if defined(STM32_IC10_ENABLED)
	LL_RCC_IC10_SetSource(ic_src_pll(STM32_IC10_PLL_SRC));
	LL_RCC_IC10_SetDivider(STM32_IC10_DIV);
	LL_RCC_IC10_Enable();
#endif

#if defined(STM32_IC11_ENABLED)
	LL_RCC_IC11_SetSource(ic_src_pll(STM32_IC11_PLL_SRC));
	LL_RCC_IC11_SetDivider(STM32_IC11_DIV);
	LL_RCC_IC11_Enable();
#endif

#if defined(STM32_IC12_ENABLED)
	LL_RCC_IC12_SetSource(ic_src_pll(STM32_IC12_PLL_SRC));
	LL_RCC_IC12_SetDivider(STM32_IC12_DIV);
	LL_RCC_IC12_Enable();
#endif

#if defined(STM32_IC13_ENABLED)
	LL_RCC_IC13_SetSource(ic_src_pll(STM32_IC13_PLL_SRC));
	LL_RCC_IC13_SetDivider(STM32_IC13_DIV);
	LL_RCC_IC13_Enable();
#endif

#if defined(STM32_IC14_ENABLED)
	LL_RCC_IC14_SetSource(ic_src_pll(STM32_IC14_PLL_SRC));
	LL_RCC_IC14_SetDivider(STM32_IC14_DIV);
	LL_RCC_IC14_Enable();
#endif

#if defined(STM32_IC15_ENABLED)
	LL_RCC_IC15_SetSource(ic_src_pll(STM32_IC15_PLL_SRC));
	LL_RCC_IC15_SetDivider(STM32_IC15_DIV);
	LL_RCC_IC15_Enable();
#endif

#if defined(STM32_IC16_ENABLED)
	LL_RCC_IC16_SetSource(ic_src_pll(STM32_IC16_PLL_SRC));
	LL_RCC_IC16_SetDivider(STM32_IC16_DIV);
	LL_RCC_IC16_Enable();
#endif

#if defined(STM32_IC17_ENABLED)
	LL_RCC_IC17_SetSource(ic_src_pll(STM32_IC17_PLL_SRC));
	LL_RCC_IC17_SetDivider(STM32_IC17_DIV);
	LL_RCC_IC17_Enable();
#endif

#if defined(STM32_IC18_ENABLED)
	LL_RCC_IC18_SetSource(ic_src_pll(STM32_IC18_PLL_SRC));
	LL_RCC_IC18_SetDivider(STM32_IC18_DIV);
	LL_RCC_IC18_Enable();
#endif

#if defined(STM32_IC19_ENABLED)
	LL_RCC_IC19_SetSource(ic_src_pll(STM32_IC19_PLL_SRC));
	LL_RCC_IC19_SetDivider(STM32_IC19_DIV);
	LL_RCC_IC19_Enable();
#endif

#if defined(STM32_IC20_ENABLED)
	LL_RCC_IC20_SetSource(ic_src_pll(STM32_IC20_PLL_SRC));
	LL_RCC_IC20_SetDivider(STM32_IC20_DIV);
	LL_RCC_IC20_Enable();
#endif

	return 0;
}

static int set_up_plls(void)
{
#if defined(STM32_PLL1_ENABLED)
	/* TODO: Do not switch systematically on HSI if not needed */
	stm32_clock_switch_to_hsi();

	LL_RCC_PLL1_Disable();

	/* Configure PLL source : Can be HSE, HSI, MSI */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLLSOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL_SRC_MSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLLSOURCE_MSI);
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLLSOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	/* Disable PLL1 modulation spread-spectrum */
	LL_RCC_PLL1_DisableModulationSpreadSpectrum();

	/* Disable bypass to use the PLL VCO */
	if (LL_RCC_PLL1_IsEnabledBypass()) {
		LL_RCC_PLL1_DisableBypass();
	}

	/* Configure PLL */
	LL_RCC_PLL1_SetM(STM32_PLL1_M_DIVISOR);
	LL_RCC_PLL1_SetN(STM32_PLL1_N_MULTIPLIER);
	LL_RCC_PLL1_SetP1(STM32_PLL1_P1_DIVISOR);
	LL_RCC_PLL1_SetP2(STM32_PLL1_P2_DIVISOR);

	/* Disable fractional mode */
	LL_RCC_PLL1_SetFRACN(0);
	LL_RCC_PLL1_DisableFractionalModulationSpreadSpectrum();

	LL_RCC_PLL1_AssertModulationSpreadSpectrumReset();

	/* Enable post division */
	if (!LL_RCC_PLL1P_IsEnabled()) {
		LL_RCC_PLL1P_Enable();
	}

	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1U) {
	}
#endif /* STM32_PLL1_ENABLED */

#if defined(STM32_PLL2_ENABLED)
	LL_RCC_PLL2_Disable();

	/* Configure PLL source : Can be HSE, HSI, MSI */
	if (IS_ENABLED(STM32_PLL2_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL2_SetSource(LL_RCC_PLLSOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL2_SRC_MSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL2_SetSource(LL_RCC_PLLSOURCE_MSI);
	} else if (IS_ENABLED(STM32_PLL2_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL2_SetSource(LL_RCC_PLLSOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	/* Disable PLL2 modulation spread-spectrum */
	LL_RCC_PLL2_DisableModulationSpreadSpectrum();

	/* Disable bypass to use the PLL VCO */
	if (LL_RCC_PLL2_IsEnabledBypass()) {
		LL_RCC_PLL2_DisableBypass();
	}

	/* Configure PLL */
	LL_RCC_PLL2_SetM(STM32_PLL2_M_DIVISOR);
	LL_RCC_PLL2_SetN(STM32_PLL2_N_MULTIPLIER);
	LL_RCC_PLL2_SetP1(STM32_PLL2_P1_DIVISOR);
	LL_RCC_PLL2_SetP2(STM32_PLL2_P2_DIVISOR);

	/* Disable fractional mode */
	LL_RCC_PLL2_SetFRACN(0);
	LL_RCC_PLL2_DisableFractionalModulationSpreadSpectrum();

	LL_RCC_PLL2_AssertModulationSpreadSpectrumReset();

	/* Enable post division */
	if (!LL_RCC_PLL2P_IsEnabled()) {
		LL_RCC_PLL2P_Enable();
	}

	LL_RCC_PLL2_Enable();
	while (LL_RCC_PLL2_IsReady() != 1U) {
	}
#endif

#if defined(STM32_PLL3_ENABLED)
	LL_RCC_PLL3_Disable();

	/* Configure PLL source : Can be HSE, HSI, MSIS */
	if (IS_ENABLED(STM32_PLL3_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL3_SetSource(LL_RCC_PLLSOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL3_SRC_MSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL3_SetSource(LL_RCC_PLLSOURCE_MSI);
	} else if (IS_ENABLED(STM32_PLL3_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL3_SetSource(LL_RCC_PLLSOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	/* Disable PLL3 modulation spread-spectrum */
	LL_RCC_PLL3_DisableModulationSpreadSpectrum();

	/* Disable bypass to use the PLL VCO */
	if (LL_RCC_PLL3_IsEnabledBypass()) {
		LL_RCC_PLL3_DisableBypass();
	}

	/* Configure PLL */
	LL_RCC_PLL3_SetM(STM32_PLL3_M_DIVISOR);
	LL_RCC_PLL3_SetN(STM32_PLL3_N_MULTIPLIER);
	LL_RCC_PLL3_SetP1(STM32_PLL3_P1_DIVISOR);
	LL_RCC_PLL3_SetP2(STM32_PLL3_P2_DIVISOR);

	/* Disable fractional mode */
	LL_RCC_PLL3_SetFRACN(0);
	LL_RCC_PLL3_DisableFractionalModulationSpreadSpectrum();

	LL_RCC_PLL3_AssertModulationSpreadSpectrumReset();

	/* Enable post division */
	if (!LL_RCC_PLL3P_IsEnabled()) {
		LL_RCC_PLL3P_Enable();
	}

	LL_RCC_PLL3_Enable();
	while (LL_RCC_PLL3_IsReady() != 1U) {
	}
#endif

#if defined(STM32_PLL4_ENABLED)
	LL_RCC_PLL4_Disable();

	/* Configure PLL source : Can be HSE, HSI, MSIS */
	if (IS_ENABLED(STM32_PLL4_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL4_SetSource(LL_RCC_PLLSOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL4_SRC_MSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL4_SetSource(LL_RCC_PLLSOURCE_MSI);
	} else if (IS_ENABLED(STM32_PLL4_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL4_SetSource(LL_RCC_PLLSOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	/* Disable PLL4 modulation spread-spectrum */
	LL_RCC_PLL4_DisableModulationSpreadSpectrum();

	/* Disable bypass to use the PLL VCO */
	if (LL_RCC_PLL4_IsEnabledBypass()) {
		LL_RCC_PLL4_DisableBypass();
	}

	/* Configure PLL */
	LL_RCC_PLL4_SetM(STM32_PLL4_M_DIVISOR);
	LL_RCC_PLL4_SetN(STM32_PLL4_N_MULTIPLIER);
	LL_RCC_PLL4_SetP1(STM32_PLL4_P1_DIVISOR);
	LL_RCC_PLL4_SetP2(STM32_PLL4_P2_DIVISOR);

	/* Disable fractional mode */
	LL_RCC_PLL4_SetFRACN(0);
	LL_RCC_PLL4_DisableFractionalModulationSpreadSpectrum();

	LL_RCC_PLL4_AssertModulationSpreadSpectrumReset();

	/* Enable post division */
	if (!LL_RCC_PLL4P_IsEnabled()) {
		LL_RCC_PLL4P_Enable();
	}

	LL_RCC_PLL4_Enable();
	while (LL_RCC_PLL4_IsReady() != 1U) {
	}
#endif

	return 0;
}

static void set_up_fixed_clock_sources(void)
{
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		/* Check if need to enable HSE bypass feature or not */
		if (IS_ENABLED(STM32_HSE_BYPASS)) {
			LL_RCC_HSE_EnableBypass();
		} else {
			LL_RCC_HSE_DisableBypass();
		}

		if (IS_ENABLED(STM32_HSE_DIV2)) {
			LL_RCC_HSE_SelectHSEDiv2AsDiv2Clock();
		} else {
			LL_RCC_HSE_SelectHSEAsDiv2Clock();
		}

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}
	}

	if (IS_ENABLED(STM32_HSI_ENABLED)) {
		/* Enable HSI oscillator */
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
		}
		/* HSI divider configuration */
		LL_RCC_HSI_SetDivider(hsi_divider(STM32_HSI_DIVISOR));
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/* Enable the power interface clock */
		LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

		if (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Enable write access to Backup domain */
			LL_PWR_EnableBkUpAccess();
			while (!LL_PWR_IsEnabledBkUpAccess()) {
				/* Wait for Backup domain access */
			}
		}

		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_LSECFGR_LSEDRV_Pos);

		if (IS_ENABLED(STM32_LSE_BYPASS)) {
			/* Configure LSE bypass */
			LL_RCC_LSE_EnableBypass();
		}

		/* Enable LSE Oscillator */
		LL_RCC_LSE_Enable();
		/* Wait for LSE ready */
		while (!LL_RCC_LSE_IsReady()) {
		}

		LL_PWR_DisableBkUpAccess();
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		/* Enable LSI oscillator */
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}
	}
}

int stm32_clock_control_init(const struct device *dev)
{
	int r = 0;

	ARG_UNUSED(dev);

	/* For now, enable clocks (including low_power ones) of all RAM */
	uint32_t all_ram = LL_MEM_AXISRAM1 | LL_MEM_AXISRAM2 | LL_MEM_AXISRAM3 | LL_MEM_AXISRAM4 |
			   LL_MEM_AXISRAM5 | LL_MEM_AXISRAM6 | LL_MEM_AHBSRAM1 | LL_MEM_AHBSRAM2 |
			   LL_MEM_BKPSRAM | LL_MEM_FLEXRAM | LL_MEM_CACHEAXIRAM | LL_MEM_VENCRAM;
	LL_MEM_EnableClock(all_ram);
	LL_MEM_EnableClockLowPower(all_ram);

	/* Set up individual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set up PLLs */
	r = set_up_plls();
	if (r < 0) {
		return r;
	}

	/* Preset the prescalers prior to chosing SYSCLK */
	/* Prevents APB clock to go over limits */
	/* Set buses (AHB, APB1, APB2, APB4 & APB5) prescalers */
	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_AHB_PRESCALER));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB4Prescaler(apb4_prescaler(STM32_APB4_PRESCALER));
	LL_RCC_SetAPB5Prescaler(apb5_prescaler(STM32_APB5_PRESCALER));

	/* Set TIMG presclar */
	LL_RCC_SetTIMPrescaler(timg_prescaler(STM32_TIMG_PRESCALER));

	if (IS_ENABLED(STM32_CKPER_ENABLED)) {
		LL_MISC_EnableClock(LL_PER);
		LL_MISC_EnableClockLowPower(LL_PER);
		while (LL_MISC_IsEnabledClock(LL_PER) != 1) {
		}
	}

	/* Set up ICs */
	r = set_up_ics();
	if (r < 0) {
		return r;
	}

	/* Set up sys clock */
	if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		/* Set sysclk source to HSE */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() !=
					LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		/* Set sysclk source to HSI */
		stm32_clock_switch_to_hsi();
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_IC2)) {
		/* Set sysclk source to IC2 */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_IC2_IC6_IC11);
		while (LL_RCC_GetSysClkSource() !=
					LL_RCC_SYS_CLKSOURCE_STATUS_IC2_IC6_IC11) {
		}
	} else {
		return -ENOTSUP;
	}

	/* Update CMSIS variable */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return r;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    &stm32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &stm32_clock_control_api);
