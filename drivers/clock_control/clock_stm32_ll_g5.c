/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_flash.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_utils.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <stm32_backup_domain.h>

/* Macros to fill up prescaler values */
#define ahb_prescaler(v) CONCAT(LL_RCC_HCLK_PRESCALER_, v)
#define apb1_prescaler(v) CONCAT(LL_RCC_APB1_PRESCALER_, v)
#define apb2_prescaler(v) CONCAT(LL_RCC_APB2_PRESCALER_, v)
#define apb3_prescaler(v) CONCAT(LL_RCC_APB3_PRESCALER_, v)

#define PLL1_ID 1
#define PLL2_ID 2

#define PLL_FRACN_DIVISOR 8192U

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

static uint32_t get_pllsrc_frequency(size_t pll_id)
{
	if ((IS_ENABLED(STM32_PLL_SRC_HSI) && pll_id == PLL1_ID) ||
	    (IS_ENABLED(STM32_PLL2_SRC_HSI) && pll_id == PLL2_ID)) {
		return STM32_HSI_FREQ;
	} else if ((IS_ENABLED(STM32_PLL_SRC_HSE) && pll_id == PLL1_ID) ||
		   (IS_ENABLED(STM32_PLL2_SRC_HSE) && pll_id == PLL2_ID)) {
		return STM32_HSE_FREQ;
	}

	__ASSERT(0, "No PLL source configured");
	return 0;
}

static uint32_t get_startup_frequency(void)
{
	switch (LL_RCC_GetSysClkSource()) {
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:
		return HSI_VALUE;
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:
		return STM32_HSE_FREQ;
	case LL_RCC_SYS_CLKSOURCE_STATUS_PLL1:
		return get_pllsrc_frequency(PLL1_ID);
	default:
		__ASSERT(0, "Unexpected startup frequency source");
		return 0;
	}
}

__unused static uint32_t get_pllout_frequency(uint32_t pllsrc_freq,
					      unsigned int pllm_div,
					      unsigned int plln_mul,
					      unsigned int plln_frac,
					      unsigned int pllout_div)
{
	uint64_t f_vco;

	__ASSERT_NO_MSG(pllm_div && pllout_div);

	f_vco = ((uint64_t)pllsrc_freq *
		 ((uint64_t)plln_mul * PLL_FRACN_DIVISOR + plln_frac)) /
		((uint64_t)pllm_div * PLL_FRACN_DIVISOR);

	return (uint32_t)(f_vco / pllout_div);
}

static uint32_t get_sysclk_frequency(void)
{
#if defined(STM32_SYSCLK_SRC_PLL) && defined(STM32_PLL_M_DIVISOR) && \
	defined(STM32_PLL_N_MULTIPLIER) && defined(STM32_PLL_FRACN_VALUE) && \
	defined(STM32_PLL_P_DIVISOR)
	return get_pllout_frequency(get_pllsrc_frequency(PLL1_ID),
				    STM32_PLL_M_DIVISOR,
				    STM32_PLL_N_MULTIPLIER,
				    STM32_PLL_FRACN_VALUE,
				    STM32_PLL_P_DIVISOR);
#elif defined(STM32_SYSCLK_SRC_HSE)
	return STM32_HSE_FREQ;
#elif defined(STM32_SYSCLK_SRC_HSI)
	return STM32_HSI_FREQ;
#else
	__ASSERT(0, "No SYSCLK source configured");
	return 0;
#endif
}

/** @brief Verifies clock is part of active clock configuration */
int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK3) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI48) && IS_ENABLED(STM32_HSI48_ENABLED))) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	uint32_t enr = pclken->enr;
	uint32_t reg = STM32_DT_CLKSEL_REG_GET(enr);
	uint32_t shift = STM32_DT_CLKSEL_SHIFT_GET(enr);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempt to configure a src clock not available or not valid */
		return err;
	}

	if (pclken->enr == NO_SEL) {
		/* Domain clock is fixed. Nothing to set. Exit */
		return 0;
	}

	stm32_reg_modify_bits((uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) + reg),
			      STM32_DT_CLKSEL_MASK_GET(enr) << shift,
			      STM32_DT_CLKSEL_VAL_GET(enr) << shift);

	return 0;
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Source selection entry: apply it instead of toggling a gate */
		return stm32_clock_control_configure(dev, sub_system, NULL);
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);
	/* Delay after enabling the clock, to allow it to become active */
	temp = sys_read32(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus);
	UNUSED(temp);

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

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus, pclken->enr);

	return 0;
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sys);
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
	case STM32_SRC_PCLK1:
		*rate = apb1_clock;
		break;
	case STM32_CLOCK_BUS_APB2:
	case STM32_SRC_PCLK2:
		*rate = apb2_clock;
		break;
	case STM32_CLOCK_BUS_APB3:
	case STM32_SRC_PCLK3:
		*rate = apb3_clock;
		break;
	case STM32_SRC_SYSCLK:
		*rate = get_sysclk_frequency();
		break;
#if defined(STM32_HSE_ENABLED)
	case STM32_SRC_HSE:
		*rate = STM32_HSE_FREQ;
		break;
#endif
#if defined(STM32_LSE_ENABLED)
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
#endif
#if defined(STM32_LSI_ENABLED)
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
#endif
#if defined(STM32_HSI48_ENABLED)
	case STM32_SRC_HSI48:
		*rate = STM32_HSI48_FREQ;
		break;
#endif
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

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
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

__unused static int get_vco_input_range(uint32_t m_div, uint32_t *range, size_t pll_id)
{
	uint32_t vco_freq = get_pllsrc_frequency(pll_id) / m_div;

	if (MHZ(1) <= vco_freq && vco_freq <= MHZ(2)) {
		*range = LL_RCC_PLLINPUTRANGE_1_2;
	} else if (MHZ(2) < vco_freq && vco_freq <= MHZ(4)) {
		*range = LL_RCC_PLLINPUTRANGE_2_4;
	} else if (MHZ(4) < vco_freq && vco_freq <= MHZ(8)) {
		*range = LL_RCC_PLLINPUTRANGE_4_8;
	} else if (MHZ(8) < vco_freq && vco_freq <= MHZ(16)) {
		*range = LL_RCC_PLLINPUTRANGE_8_16;
	} else {
		return -ERANGE;
	}

	return 0;
}

__unused static int get_vco_output_range(uint32_t pllsrc_freq, uint32_t m_div, uint32_t n_mul,
							  uint32_t *range)
{
	uint32_t vco_freq = (pllsrc_freq / m_div) * n_mul;

	if (MHZ(150) <= vco_freq && vco_freq <= MHZ(420)) {
		*range = LL_RCC_PLLVCORANGE_150_420;
	} else if (MHZ(128) <= vco_freq && vco_freq <= MHZ(560)) {
		*range = LL_RCC_PLLVCORANGE_128_560;
	} else {
		return -ERANGE;
	}

	return 0;
}

static void set_regu_voltage(uint32_t hclk_freq)
{
	if (hclk_freq > MHZ(110)) {
		LL_PWR_SetRegulatorVoltageScaling(LL_PWR_REGU_VOLT_SCALE_0);
	} else if (hclk_freq > MHZ(55)) {
		LL_PWR_SetRegulatorVoltageScaling(LL_PWR_REGU_VOLT_SCALE_1);
	} else {
		LL_PWR_SetRegulatorVoltageScaling(LL_PWR_REGU_VOLT_SCALE_2);
	}

	while (LL_PWR_IsActiveFlag_VOS() == 0U) {
	}
}

static void set_flash_latency(uint32_t hclk_freq)
{
	uint32_t latency;

	if (hclk_freq > MHZ(120)) {
		latency = LL_FLASH_LATENCY_4WS;
	} else if (hclk_freq > MHZ(90)) {
		latency = LL_FLASH_LATENCY_3WS;
	} else if (hclk_freq > MHZ(60)) {
		latency = LL_FLASH_LATENCY_2WS;
	} else if (hclk_freq > MHZ(30)) {
		latency = LL_FLASH_LATENCY_1WS;
	} else {
		latency = LL_FLASH_LATENCY_0WS;
	}

	LL_FLASH_SetLatency(FLASH, latency);
	while (LL_FLASH_GetLatency(FLASH) != latency) {
	}
}

__unused static void clock_switch_to_hsi(void)
{
	if (LL_RCC_HSI_IsReady() != 1U) {
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1U) {
		}
	}

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

	LL_RCC_SetAHBPrescaler(LL_RCC_HCLK_PRESCALER_1);
}

static int set_up_plls(void)
{
#if defined(STM32_PLL_ENABLED) || defined(STM32_PLL2_ENABLED)
	int r;
	uint32_t vco_input_range;
	uint32_t vco_output_range;
#endif

#if defined(STM32_PLL_ENABLED)
	if (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
		clock_switch_to_hsi();
	}

	LL_RCC_PLL1_Disable();
	/* Per RM0514: PLL fields cannot be modified when PLL1ON=1 OR PLL1RDY=1.
	 * Wait for PLL1RDY to deassert. Note: clock_switch_to_hsi() above already
	 * moved SYSCLK off PLL1, so PLL1ON can now be cleared by hardware.
	 */
	while (LL_RCC_PLL1_IsReady() != 0U) {
	}

	if (IS_ENABLED(STM32_PLL_SRC_HSE)) {
		LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL_SRC_HSI)) {
		LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL_M_DIVISOR, &vco_input_range, PLL1_ID);
	if (r < 0) {
		return r;
	}

	r = get_vco_output_range(get_pllsrc_frequency(PLL1_ID), STM32_PLL_M_DIVISOR,
				 STM32_PLL_N_MULTIPLIER, &vco_output_range);
	if (r < 0) {
		return r;
	}

	LL_RCC_PLL1_SetM(STM32_PLL_M_DIVISOR);
	LL_RCC_PLL1_SetInputRange(vco_input_range);
	LL_RCC_PLL1_SetVCORange(vco_output_range);
	LL_RCC_PLL1_SetN(STM32_PLL_N_MULTIPLIER);

	/* FRACN is latched on the 0-to-1 transition of FRACEN. */
	LL_RCC_PLL1_DisableFRACN();
	if (IS_ENABLED(STM32_PLL_FRACN_ENABLED)) {
		LL_RCC_PLL1_SetFRACN(STM32_PLL_FRACN_VALUE);
	} else {
		LL_RCC_PLL1_SetFRACN(0U);
	}
	LL_RCC_PLL1_EnableFRACN();

	if (IS_ENABLED(STM32_PLL_P_ENABLED)) {
		LL_RCC_PLL1_SetP(STM32_PLL_P_DIVISOR);
		LL_RCC_PLL1_EnableOutputP();
	}

	if (IS_ENABLED(STM32_PLL_Q_ENABLED)) {
		LL_RCC_PLL1_SetQ(STM32_PLL_Q_DIVISOR);
		LL_RCC_PLL1_EnableOutputQ();
	}

	if (IS_ENABLED(STM32_PLL_R_ENABLED)) {
		LL_RCC_PLL1_SetR(STM32_PLL_R_DIVISOR);
		LL_RCC_PLL1_EnableOutputR();
	}

	LL_RCC_PLL1_Enable();
	while (LL_RCC_PLL1_IsReady() != 1U) {
	}
#endif

#if defined(STM32_PLL2_ENABLED)
	LL_RCC_PLL2_Disable();
	/* Per RM0514: wait for PLL2RDY=0 before reconfiguring */
	while (LL_RCC_PLL2_IsReady() != 0U) {
	}

	if (IS_ENABLED(STM32_PLL2_SRC_HSE)) {
		LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_HSE);
	} else if (IS_ENABLED(STM32_PLL2_SRC_HSI)) {
		LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_HSI);
	} else {
		return -ENOTSUP;
	}

	r = get_vco_input_range(STM32_PLL2_M_DIVISOR, &vco_input_range, PLL2_ID);
	if (r < 0) {
		return r;
	}

	r = get_vco_output_range(get_pllsrc_frequency(PLL2_ID), STM32_PLL2_M_DIVISOR,
				 STM32_PLL2_N_MULTIPLIER, &vco_output_range);
	if (r < 0) {
		return r;
	}

	LL_RCC_PLL2_SetM(STM32_PLL2_M_DIVISOR);
	LL_RCC_PLL2_SetInputRange(vco_input_range);
	LL_RCC_PLL2_SetVCORange(vco_output_range);
	LL_RCC_PLL2_SetN(STM32_PLL2_N_MULTIPLIER);

	LL_RCC_PLL2_DisableFRACN();
	if (IS_ENABLED(STM32_PLL2_FRACN_ENABLED)) {
		LL_RCC_PLL2_SetFRACN(STM32_PLL2_FRACN_VALUE);
	} else {
		LL_RCC_PLL2_SetFRACN(0U);
	}
	LL_RCC_PLL2_EnableFRACN();

	if (IS_ENABLED(STM32_PLL2_P_ENABLED)) {
		LL_RCC_PLL2_SetP(STM32_PLL2_P_DIVISOR);
		LL_RCC_PLL2_EnableOutputP();
	}

	if (IS_ENABLED(STM32_PLL2_Q_ENABLED)) {
		LL_RCC_PLL2_SetQ(STM32_PLL2_Q_DIVISOR);
		LL_RCC_PLL2_EnableOutputQ();
	}

	if (IS_ENABLED(STM32_PLL2_R_ENABLED)) {
		LL_RCC_PLL2_SetR(STM32_PLL2_R_DIVISOR);
		LL_RCC_PLL2_EnableOutputR();
	}

	LL_RCC_PLL2_Enable();
	while (LL_RCC_PLL2_IsReady() != 1U) {
	}
#else
	LL_RCC_PLL2_SetSource(LL_RCC_PLL2SOURCE_NO_CLK);
#endif

	return 0;
}

static void set_up_fixed_clock_sources(void)
{
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		if (IS_ENABLED(STM32_HSE_BYPASS)) {
			LL_RCC_HSE_EnableBypass();
		} else {
			LL_RCC_HSE_DisableBypass();
		}

		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1U) {
		}
	}

	if (IS_ENABLED(STM32_HSI_ENABLED) ||
	    IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		if (LL_RCC_HSI_IsReady() != 1U) {
			LL_RCC_HSI_Enable();
			while (LL_RCC_HSI_IsReady() != 1U) {
			}
		}
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		stm32_backup_domain_enable_access();
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_RTCCR_LSEDRV_Pos);

		if (IS_ENABLED(STM32_LSE_BYPASS)) {
			LL_RCC_LSE_EnableBypass();
		}

		LL_RCC_LSE_Enable();
		while (!LL_RCC_LSE_IsReady()) {
		}

		stm32_backup_domain_disable_access();
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1U) {
		}
	}

	if (IS_ENABLED(STM32_HSI48_ENABLED)) {
		LL_RCC_HSI48_Enable();
		while (LL_RCC_HSI48_IsReady() != 1U) {
		}
	}
}

int stm32_clock_control_init(const struct device *dev)
{
	uint32_t old_hclk_freq;
	int r;

	ARG_UNUSED(dev);

	old_hclk_freq = LL_RCC_CALC_HCLK_FREQ(get_startup_frequency(), LL_RCC_GetAHBPrescaler());

	set_regu_voltage(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	if (old_hclk_freq < CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		set_flash_latency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	set_up_fixed_clock_sources();

	r = set_up_plls();
	if (r < 0) {
		__ASSERT(0, "PLL setup failed");
		return r;
	}

	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_AHB_PRESCALER));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(STM32_APB3_PRESCALER));

	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSE)) {
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
	} else if (IS_ENABLED(STM32_SYSCLK_SRC_HSI)) {
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
		}
	} else {
		__ASSERT(0, "Invalid SYSCLK source selected");
		return -ENOTSUP;
	}

	if (old_hclk_freq >= CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		set_flash_latency(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		 stm32_clock_control_init,
		 NULL,
		 NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &stm32_clock_control_api);
