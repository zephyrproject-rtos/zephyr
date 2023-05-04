/*
 *
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2022 Thomas Stranger
 * Copyright (c) 2023 STMicroelectronics
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
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/* Macros to fill up prescaler values */
#define z_hsi_divider(v) LL_RCC_HSI_DIV_ ## v
#define hsi_divider(v) z_hsi_divider(v)

#define z_ahb_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) z_ahb_prescaler(v)

#define z_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) z_apb1_prescaler(v)

#define z_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) z_apb2_prescaler(v)

#define z_apb3_prescaler(v) LL_RCC_APB3_DIV_ ## v
#define apb3_prescaler(v) z_apb3_prescaler(v)

#define PLL1_ID		1
#define PLL2_ID		2
#define PLL3_ID		3

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

__unused
/** @brief returns the pll source frequency of given pll_id */
static uint32_t get_pllsrc_frequency(size_t pll_id)
{

	if ((IS_ENABLED(STM32_PLL_SRC_HSI) && pll_id == PLL1_ID) ||
	    (IS_ENABLED(STM32_PLL2_SRC_HSI) && pll_id == PLL2_ID) ||
	    (IS_ENABLED(STM32_PLL3_SRC_HSI) && pll_id == PLL3_ID)) {
		return STM32_HSI_FREQ;
	} else if ((IS_ENABLED(STM32_PLL_SRC_HSE) && pll_id == PLL1_ID) ||
		   (IS_ENABLED(STM32_PLL2_SRC_HSE) && pll_id == PLL2_ID) ||
		   (IS_ENABLED(STM32_PLL3_SRC_HSE) && pll_id == PLL3_ID)) {
		return STM32_HSE_FREQ;
	} else if ((IS_ENABLED(STM32_PLL_SRC_CSI) && pll_id == PLL1_ID) ||
		   (IS_ENABLED(STM32_PLL2_SRC_CSI) && pll_id == PLL2_ID) ||
		   (IS_ENABLED(STM32_PLL3_SRC_CSI) && pll_id == PLL3_ID)) {
		return STM32_CSI_FREQ;
	}

	__ASSERT(0, "No PLL Source configured");
	return 0;
}

static uint32_t get_startup_frequency(void)
{
	switch (LL_RCC_GetSysClkSource()) {
	case LL_RCC_SYS_CLKSOURCE_STATUS_CSI:
		return STM32_CSI_FREQ;
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:
		return STM32_HSI_FREQ;
	default:
		__ASSERT(0, "Unexpected startup freq");
		return 0;
	}
}

__unused
static uint32_t get_pllout_frequency(uint32_t pllsrc_freq,
					    int pllm_div,
					    int plln_mul,
					    int pllout_div)
{
	__ASSERT_NO_MSG(pllm_div && pllout_div);

	return (pllsrc_freq / pllm_div) * plln_mul / pllout_div;
}

static uint32_t get_sysclk_frequency(void)
{
#if defined(STM32_SYSCLK_SRC_PLL)
	return get_pllout_frequency(get_pllsrc_frequency(PLL1_ID),
					STM32_PLL_M_DIVISOR,
					STM32_PLL_N_MULTIPLIER,
					STM32_PLL_R_DIVISOR);
#elif defined(STM32_SYSCLK_SRC_CSI)
	return STM32_CSI_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSE)
	return STM32_HSE_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSI)
	return STM32_HSI_FREQ;
#else
	__ASSERT(0, "No SYSCLK Source configured");
	return 0;
#endif

}

/** @brief Verifies clock is part of active clock configuration */
static int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI48) && IS_ENABLED(STM32_HSI48_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_CSI) && IS_ENABLED(STM32_CSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_P) && IS_ENABLED(STM32_PLL_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_Q) && IS_ENABLED(STM32_PLL_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL1_R) && IS_ENABLED(STM32_PLL_R_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_P) && IS_ENABLED(STM32_PLL2_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_Q) && IS_ENABLED(STM32_PLL2_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL2_R) && IS_ENABLED(STM32_PLL2_R_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_P) && IS_ENABLED(STM32_PLL3_P_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_Q) && IS_ENABLED(STM32_PLL3_Q_ENABLED)) ||
	    ((src_clk == STM32_SRC_PLL3_R) && IS_ENABLED(STM32_PLL3_R_ENABLED))) {
		return 0;
	}

	return -ENOTSUP;
}

static inline int stm32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
		/* Attemp to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		     pclken->enr);

	return 0;
}

static inline int stm32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX) == 0) {
		/* Attemp to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		       pclken->enr);

	return 0;
}

static inline int stm32_clock_control_configure(const struct device *dev,
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

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + STM32_CLOCK_REG_GET(pclken->enr),
		     STM32_CLOCK_VAL_GET(pclken->enr) << STM32_CLOCK_SHIFT_GET(pclken->enr));

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sys);

	/*
	 * Get AHB Clock (= SystemCoreClock = SYSCLK/prescaler)
	 * SystemCoreClock is preferred to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	 * since it will be updated after clock configuration and hence
	 * more likely to contain actual clock speed
	 */
	uint32_t ahb_clock = SystemCoreClock;
	uint32_t apb1_clock = get_bus_clock(ahb_clock, STM32_APB1_PRESCALER);
	uint32_t apb2_clock = get_bus_clock(ahb_clock, STM32_APB2_PRESCALER);
	uint32_t apb3_clock = get_bus_clock(ahb_clock, STM32_APB3_PRESCALER);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB4:
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
	case STM32_CLOCK_BUS_APB1_2:
		*rate = apb1_clock;
		break;
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
	case STM32_CLOCK_BUS_APB3:
		*rate = apb3_clock;
		break;
	case STM32_SRC_SYSCLK:
		*rate = get_sysclk_frequency();
		break;
#if defined(STM32_HSI_ENABLED)
	case STM32_SRC_HSI:
		*rate = STM32_HSI_FREQ;
		break;
#endif /* STM32_HSI_ENABLED */
#if defined(STM32_CSI_ENABLED)
	case STM32_SRC_CSI:
		*rate = STM32_CSI_FREQ;
		break;
#endif /* STM32_MSIS_ENABLED */
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif /* STM32_HSE_ENABLED */
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
#if defined(STM32_HSI48_ENABLED)
	case STM32_SRC_HSI48:
		*rate = STM32_HSI48_FREQ;
		break;
#endif /* STM32_HSI48_ENABLED */
#if defined(STM32_PLL_ENABLED)
	case STM32_SRC_PLL1_P:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL1_ID),
					      STM32_PLL_M_DIVISOR,
					      STM32_PLL_N_MULTIPLIER,
					      STM32_PLL_P_DIVISOR);
		break;
	case STM32_SRC_PLL1_Q:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL1_ID),
					      STM32_PLL_M_DIVISOR,
					      STM32_PLL_N_MULTIPLIER,
					      STM32_PLL_Q_DIVISOR);
		break;
	case STM32_SRC_PLL1_R:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL1_ID),
					      STM32_PLL_M_DIVISOR,
					      STM32_PLL_N_MULTIPLIER,
					      STM32_PLL_R_DIVISOR);
		break;
#endif /* STM32_PLL_ENABLED */
#if defined(STM32_PLL2_ENABLED)
	case STM32_SRC_PLL2_P:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL2_ID),
					      STM32_PLL2_M_DIVISOR,
					      STM32_PLL2_N_MULTIPLIER,
					      STM32_PLL2_P_DIVISOR);
		break;
	case STM32_SRC_PLL2_Q:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL2_ID),
					      STM32_PLL2_M_DIVISOR,
					      STM32_PLL2_N_MULTIPLIER,
					      STM32_PLL2_Q_DIVISOR);
		break;
	case STM32_SRC_PLL2_R:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL2_ID),
					      STM32_PLL2_M_DIVISOR,
					      STM32_PLL2_N_MULTIPLIER,
					      STM32_PLL2_R_DIVISOR);
		break;
#endif /* STM32_PLL2_ENABLED */
#if defined(STM32_PLL3_ENABLED)
	case STM32_SRC_PLL3_P:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL3_ID),
					      STM32_PLL3_M_DIVISOR,
					      STM32_PLL3_N_MULTIPLIER,
					      STM32_PLL3_P_DIVISOR);
		break;
	case STM32_SRC_PLL3_Q:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL3_ID),
					      STM32_PLL3_M_DIVISOR,
					      STM32_PLL3_N_MULTIPLIER,
					      STM32_PLL3_Q_DIVISOR);
		break;
	case STM32_SRC_PLL3_R:
		*rate = get_pllout_frequency(get_pllsrc_frequency(PLL3_ID),
					      STM32_PLL3_M_DIVISOR,
					      STM32_PLL3_N_MULTIPLIER,
					      STM32_PLL3_R_DIVISOR);
		break;
#endif /* STM32_PLL3_ENABLED */
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.configure = stm32_clock_control_configure,
};

__unused
static int get_vco_input_range(uint32_t m_div, uint32_t *range, size_t pll_id)
{
	uint32_t vco_freq;

	vco_freq = get_pllsrc_frequency(pll_id) / m_div;

	if (MHZ(4) <= vco_freq && vco_freq <= MHZ(8)) {
		*range = LL_RCC_PLLINPUTRANGE_4_8;
	} else if (MHZ(8) < vco_freq && vco_freq <= MHZ(16)) {
		*range = LL_RCC_PLLINPUTRANGE_8_16;
	} else {
		return -ERANGE;
	}

	return 0;
}


__unused
static uint32_t get_vco_output_range(uint32_t vco_input_range)
{
	if (vco_input_range == LL_RCC_PLLINPUTRANGE_1_2) {
		return LL_RCC_PLLVCORANGE_MEDIUM;
	}

	return LL_RCC_PLLVCORANGE_WIDE;
}

static void set_regu_voltage(uint32_t hclk_freq)
{
	if (hclk_freq < MHZ(80)) {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);
	} else if (hclk_freq < MHZ(130)) {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
	} else if (hclk_freq < MHZ(180)) {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	} else {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
	}
	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
}

__unused
static void clock_switch_to_hsi(void)
{
	/* Enable HSI if not enabled */
	if (LL_RCC_HSI_IsReady() != 1) {
		/* Enable HSI */
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
		/* Wait for HSI ready */
		}
	}

	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

	/* Set HSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}
}

__unused
static int set_up_plls(void)
{
#if defined(STM32_PLL_ENABLED) || defined(STM32_PLL2_ENABLED) || \
	defined(STM32_PLL3_ENABLED)
	int r;
	uint32_t vco_input_range;
	uint32_t vco_output_range;
#endif

#if defined(STM32_PLL_ENABLED)
	/*
	 * Switch to HSI and disable the PLL before configuration.
	 * (Switching to HSI makes sure we have a SYSCLK source in
	 * case we're currently running from the PLL we're about to
	 * turn off and reconfigure.)
	 */
	if (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
		clock_switch_to_hsi();
	}

	LL_RCC_PLL1_Disable();

	/* Configure PLL source : Can be HSE, HSI, MSIS */
	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL_SRC_CSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_CSI);
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		/* Main PLL configuration and activation */
		LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL_M_DIVISOR, &vco_input_range, PLL1_ID);
	if (r < 0) {
		return r;
	}

	vco_output_range = get_vco_output_range(vco_input_range);

	LL_RCC_PLL1_SetM(STM32_PLL_M_DIVISOR);

	/* Set VCO Input before enabling the PLL, depends on the freq of the PLL1 */
	LL_RCC_PLL1_SetVCOInputRange(vco_input_range);
	/* Select VCO freq range before enabling the PLL, depends on the freq of the PLL1 */
	LL_RCC_PLL1_SetVCOOutputRange(vco_output_range);

	LL_RCC_PLL1_SetN(STM32_PLL_N_MULTIPLIER);

	LL_RCC_PLL1FRACN_Disable();

	if (IS_ENABLED(STM32_PLL_P_ENABLED)) {
		LL_RCC_PLL1_SetP(STM32_PLL_P_DIVISOR);
		LL_RCC_PLL1P_Enable();
	}

	if (IS_ENABLED(STM32_PLL_Q_ENABLED)) {
		LL_RCC_PLL1_SetQ(STM32_PLL_Q_DIVISOR);
		LL_RCC_PLL1Q_Enable();
	}

	if (IS_ENABLED(STM32_PLL_R_ENABLED)) {
		LL_RCC_PLL1_SetR(STM32_PLL_R_DIVISOR);
		LL_RCC_PLL1R_Enable();
	}

	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1U) {
	}
#else
	/* Init PLL source to None */
	LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_NONE);
#endif /* STM32_PLL_ENABLED */

#if defined(STM32_PLL2_ENABLED)
	/* Configure PLL2 source */
	if (IS_ENABLED(STM32_PLL2_SRC_HSE)) {
		LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL2_SRC_CSI)) {
		LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_CSI);
	} else if (IS_ENABLED(STM32_PLL2_SRC_HSI)) {
		LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL2_M_DIVISOR, &vco_input_range, PLL2_ID);
	if (r < 0) {
		return r;
	}

	vco_output_range = get_vco_output_range(vco_input_range);

	LL_RCC_PLL2_SetM(STM32_PLL2_M_DIVISOR);

	/* Set VCO Input before enabling the PLL, depends on the freq of the PLL2 */
	LL_RCC_PLL2_SetVCOInputRange(vco_input_range);
	/* Select VCO freq range before enabling the PLL, depends on the freq of the PLL2 */
	LL_RCC_PLL2_SetVCOOutputRange(vco_output_range);

	LL_RCC_PLL2_SetN(STM32_PLL2_N_MULTIPLIER);

	LL_RCC_PLL2FRACN_Disable();

	if (IS_ENABLED(STM32_PLL2_P_ENABLED)) {
		LL_RCC_PLL2_SetP(STM32_PLL2_P_DIVISOR);
		LL_RCC_PLL2P_Enable();
	}

	if (IS_ENABLED(STM32_PLL2_Q_ENABLED)) {
		LL_RCC_PLL2_SetQ(STM32_PLL2_Q_DIVISOR);
		LL_RCC_PLL2Q_Enable();
	}

	if (IS_ENABLED(STM32_PLL2_R_ENABLED)) {
		LL_RCC_PLL2_SetR(STM32_PLL2_R_DIVISOR);
		LL_RCC_PLL2R_Enable();
	}

	LL_RCC_PLL2_Enable();
	while (LL_RCC_PLL2_IsReady() != 1U) {
	}
#else
	/* Init PLL2 source to None */
	LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_NONE);
#endif /* STM32_PLL2_ENABLED */

#if defined(RCC_CR_PLL3ON)
#if defined(STM32_PLL3_ENABLED)
	/* Configure PLL3 source */
	if (IS_ENABLED(STM32_PLL3_SRC_HSE)) {
		LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL3_SRC_CSI)) {
		LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_CSI);
	} else if (IS_ENABLED(STM32_PLL3_SRC_HSI)) {
		LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL3_M_DIVISOR, &vco_input_range, PLL3_ID);
	if (r < 0) {
		return r;
	}

	vco_output_range = get_vco_output_range(vco_input_range);

	LL_RCC_PLL3_SetM(STM32_PLL3_M_DIVISOR);

	/* Set VCO Input before enabling the PLL, depends on the freq of the PLL3 */
	LL_RCC_PLL3_SetVCOInputRange(vco_input_range);
	/* Select VCO freq range before enabling the PLL, depends on the freq of the PLL3 */
	LL_RCC_PLL3_SetVCOOutputRange(vco_output_range);

	LL_RCC_PLL3_SetN(STM32_PLL3_N_MULTIPLIER);

	LL_RCC_PLL3FRACN_Disable();

	if (IS_ENABLED(STM32_PLL3_P_ENABLED)) {
		LL_RCC_PLL3_SetP(STM32_PLL3_P_DIVISOR);
		LL_RCC_PLL3P_Enable();
	}

	if (IS_ENABLED(STM32_PLL3_Q_ENABLED)) {
		LL_RCC_PLL3_SetQ(STM32_PLL3_Q_DIVISOR);
		LL_RCC_PLL3Q_Enable();
	}

	if (IS_ENABLED(STM32_PLL3_R_ENABLED)) {
		LL_RCC_PLL3_SetR(STM32_PLL3_R_DIVISOR);
		LL_RCC_PLL3R_Enable();
	}

	LL_RCC_PLL3_Enable();
	while (LL_RCC_PLL3_IsReady() != 1U) {
	}
#else
	/* Init PLL3 source to None */
	LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_NONE);
#endif /* STM32_PLL3_ENABLED */
#endif /* (RCC_CR_PLL3ON) */

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

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}
	}

	if (IS_ENABLED(STM32_HSI_ENABLED)) {
		if (IS_ENABLED(STM32_PLL_SRC_HSI) ||
			IS_ENABLED(STM32_PLL2_SRC_HSI) || IS_ENABLED(STM32_PLL3_SRC_HSI)) {
			/* HSI calibration */
			LL_RCC_HSI_SetCalibTrimming(RCC_HSICALIBRATION_DEFAULT);
		}
		/* Enable HSI if not enabled */
		if (LL_RCC_HSI_IsReady() != 1) {
			/* Enable HSI */
			LL_RCC_HSI_Enable();
			while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
			}
		}
		/* HSI divider configuration */
		LL_RCC_HSI_SetDivider(hsi_divider(STM32_HSI_DIVISOR));
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		if (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Enable write access to Backup domain */
			LL_PWR_EnableBkUpAccess();
			while (!LL_PWR_IsEnabledBkUpAccess()) {
				/* Wait for Backup domain access */
			}
		}

		/* Configure driving capability before enabling the LSE oscillator */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_BDCR_LSEDRV_Pos);

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

	if (IS_ENABLED(STM32_CSI_ENABLED)) {
		if (IS_ENABLED(STM32_PLL_SRC_CSI) ||
			IS_ENABLED(STM32_PLL2_SRC_CSI) || IS_ENABLED(STM32_PLL3_SRC_CSI)) {
			/* CSI calibration */
			LL_RCC_CSI_SetCalibTrimming(RCC_CSICALIBRATION_DEFAULT);
		}

		/* Enable CSI */
		LL_RCC_CSI_Enable();

		/* Wait till CSI is ready */
		while (LL_RCC_CSI_IsReady() != 1) {
		}
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		/* Enable LSI oscillator */
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}
	}

	if (IS_ENABLED(STM32_HSI48_ENABLED)) {
		LL_RCC_HSI48_Enable();
		while (LL_RCC_HSI48_IsReady() != 1) {
		}
	}

}

int stm32_clock_control_init(const struct device *dev)
{
	uint32_t old_hclk_freq = 0;
	int r = 0;

	ARG_UNUSED(dev);

	/* Current hclk value */
	old_hclk_freq = __LL_RCC_CALC_HCLK_FREQ(get_startup_frequency(), LL_RCC_GetAHBPrescaler());

	/* Set voltage regulator to comply with targeted system frequency */
	set_regu_voltage(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* Set flash latency */
	/* If freq increases, set flash latency before any clock setting */
	if (old_hclk_freq < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	/* Set up individual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set up PLLs */
	r = set_up_plls();
	if (r < 0) {
		return r;
	}

	/* Set peripheral busses prescalers */
	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_AHB_PRESCALER));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(STM32_APB3_PRESCALER));

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		/* Set PLL1 as System Clock Source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		/* Set HSE as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_CSI)) {
		/* Set CSI as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_CSI);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_CSI) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		/* Set HSI as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
		}
	} else {
		return -ENOTSUP;
	}

	/* Set FLASH latency */
	/* If freq not increased, set flash latency after all clock setting */
	if (old_hclk_freq >= CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LL_SetFlashLatency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	/* Update CMSIS variable */
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return 0;
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
