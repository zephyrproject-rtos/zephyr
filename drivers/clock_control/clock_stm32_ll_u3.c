/*
 * Copyright (c) 2025 STMicroelectronics
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
#define z_ahb_prescaler(v) LL_RCC_HCLK_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) z_ahb_prescaler(v)

#define z_apb1_prescaler(v) LL_RCC_APB1_HCLK_DIV_ ## v
#define apb1_prescaler(v) z_apb1_prescaler(v)

#define z_apb2_prescaler(v) LL_RCC_APB2_HCLK_DIV_ ## v
#define apb2_prescaler(v) z_apb2_prescaler(v)

#define z_apb3_prescaler(v) LL_RCC_APB3_HCLK_DIV_ ## v
#define apb3_prescaler(v) z_apb3_prescaler(v)

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

static uint32_t get_msis_frequency(void)
{
	uint32_t frequency = MSIRC1_VALUE;

	if (LL_RCC_MSIS_GetClockSource() == LL_RCC_MSIS_CLOCK_SOURCE_RC0)
	{
	frequency = MSIRC0_VALUE;
	}
	switch (LL_RCC_MSIS_GetClockDivision())
	{
	case LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_2:
	frequency /= 2U;
	break;
	case LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_4:
	frequency /= 4U;
	break;
	case LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_8:
	frequency /= 8U;
	break;
	case LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_1:
	break;
	default:
	return 0;
	}

	return frequency;
}

static uint32_t get_startup_frequency(void)
{
	switch (LL_RCC_GetSysClkSource()) {
	case LL_RCC_SYS_CLKSOURCE_STATUS_MSIS:
		return get_msis_frequency();
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSI16:
		return STM32_HSI_FREQ;
	case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:
		return STM32_HSE_FREQ;
	default:
		__ASSERT(0, "Unexpected startup freq");
		return 0;
	}
}

static uint32_t get_sysclk_frequency(void)
{
#if defined(STM32_SYSCLK_SRC_MSIS)
	return get_msis_frequency();
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
int enabled_clock(uint32_t src_clk)
{
	if ((src_clk == STM32_SRC_SYSCLK) ||
	    (src_clk == STM32_SRC_HCLK) ||
	    (src_clk == STM32_SRC_PCLK1) ||
	    (src_clk == STM32_SRC_PCLK2) ||
	    (src_clk == STM32_SRC_PCLK3) ||
	    ((src_clk == STM32_SRC_HSE) && IS_ENABLED(STM32_HSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI16) && IS_ENABLED(STM32_HSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_HSI48) && IS_ENABLED(STM32_HSI48_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSE) && IS_ENABLED(STM32_LSE_ENABLED)) ||
	    ((src_clk == STM32_SRC_LSI) && IS_ENABLED(STM32_LSI_ENABLED)) ||
	    ((src_clk == STM32_SRC_MSIS) && IS_ENABLED(STM32_MSIS_ENABLED)) ||
	    ((src_clk == STM32_SRC_MSIK) && IS_ENABLED(STM32_MSIK_ENABLED))
	) {
		return 0;
	}

	return -ENOTSUP;
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	volatile int temp;

	ARG_UNUSED(dev);

	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempt to toggle a wrong periph clock bit */
		return -ENOTSUP;
	}

	sys_set_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
		     pclken->enr);
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

	sys_clear_bits(DT_REG_ADDR(DT_NODELABEL(rcc)) + pclken->bus,
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
	case STM32_CLOCK_BUS_AHB1_2:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB2_2:
	case STM32_SRC_HCLK:
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
#if defined(STM32_HSI_ENABLED)
	case STM32_SRC_HSI16:
		*rate = STM32_HSI_FREQ;
		break;
#endif /* STM32_HSI_ENABLED */
#if defined(STM32_MSIS_ENABLED)
	case STM32_SRC_MSIS:
		*rate = get_msis_frequency();
		break;
#endif /* STM32_MSIS_ENABLED */
#if defined(STM32_MSIK_ENABLED)
	case STM32_SRC_MSIK:
#ifndef CONFIG_SOC_SERIES_STM32U3X /* TBD */
		*rate = __LL_RCC_CALC_MSIK_FREQ(LL_RCC_MSIRANGESEL_RUN,
				STM32_MSIK_RANGE << RCC_ICSCR1_MSIKRANGE_Pos);
#endif
		break;
#endif /* STM32_MSIK_ENABLED */
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

static void set_regu_voltage(uint32_t hclk_freq)
{
	if (hclk_freq < MHZ(48)) {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
	} else {
		LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	}
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
		/* Enable HSI if not enabled */
		if (LL_RCC_HSI_IsReady() != 1) {
			/* Enable HSI */
			LL_RCC_HSI_Enable();
			while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
			}
		}
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
		/* Enable the power interface clock */

		LL_AHB1_GRP2_EnableClock(LL_AHB1_GRP2_PERIPH_PWR);

		if (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Enable write access to Backup domain */
			LL_PWR_EnableBkUpAccess();
			while (!LL_PWR_IsEnabledBkUpAccess()) {
				/* Wait for Backup domain access */
			}
		}

		/* Configure driving capability */
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

		/* Enable LSESYS additionally */
		LL_RCC_LSE_EnablePropagation();
		/* Wait till LSESYS is ready */
		while (!LL_RCC_LSE_IsPropagationReady()) {
		}

		LL_PWR_DisableBkUpAccess();
	}

	if (IS_ENABLED(STM32_MSIS_ENABLED)) {
		/* Set MSIS Range */
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
		while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
		{
		}
		if(STM32_MSIS_RANGE >= 0 && STM32_MSIS_RANGE <= 3)
		{
			LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
		}
		else if(STM32_MSIS_RANGE >= 4 && STM32_MSIS_RANGE <= 7)
		{
			LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
		}

		/* Enable Epod Booster */
		LL_AHB1_GRP2_EnableClock(LL_AHB1_GRP2_PERIPH_PWR);
		LL_RCC_SetEPODBoosterClkSource(LL_RCC_EPODBOOSTCLKSRCE_MSIS);
		LL_RCC_SetEPODBoosterClkPrescaler(LL_RCC_EPODBOOSTCLKPRESCAL_1);
		LL_PWR_EnableEPODBooster();
		while(LL_PWR_IsActiveFlag_BOOST() != 1)
		{
		}

		/* Activate MSIS as source */
		__HAL_RCC_MSIS_ENABLE();
		if(STM32_MSIS_RANGE >= 0 && STM32_MSIS_RANGE <= 3)
		{
			LL_RCC_MSIS_SetClockSource(LL_RCC_MSIS_CLOCK_SOURCE_RC0);
			switch (STM32_MSIS_RANGE)
			{
			case 0:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_1);
			break;
			case 1:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_2);
			break;
			case 2:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_4);
			break;
			case 3:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_8);
			break;
			default:
			break;
			}
		}
		else if(STM32_MSIS_RANGE >= 4 && STM32_MSIS_RANGE <= 7)
		{
			LL_RCC_MSIS_SetClockSource(LL_RCC_MSIS_CLOCK_SOURCE_RC1);
			switch (STM32_MSIS_RANGE)
			{
			case 4:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_1);
			break;
			case 5:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_2);
			break;
			case 6:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_4);
			break;
			case 7:
			LL_RCC_MSIS_SetClockDivision(LL_RCC_MSIS_CLOCK_SOURCE_RC_DIV_8);
			break;
			default:
			break;
			}

		}
		LL_RCC_MSI_SetMSIxClockRange();
		/* For stm32u3 LSE or HSE must be enabled and ready before
		* selecting this oscillator as MSIRC0 input clock.*/
		/* Enable MSIS */
		LL_RCC_MSIS_Enable();
		while (!LL_RCC_MSIS_IsReady()) {
		}
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSIS);

		/* Wait till System clock is ready */
		while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_MSIS)
		{
		}

		LL_RCC_SetAHBPrescaler(LL_RCC_HCLK_SYSCLK_DIV_1);
		LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_HCLK_DIV_1);
		LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_HCLK_DIV_1);
		LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_HCLK_DIV_1);

		__ASSERT(STM32_LSE_ENABLED,
			"MSIS Hardware auto calibration needs LSE clock activation");
		if(STM32_MSIS_RANGE >= 0 && STM32_MSIS_RANGE <= 3)
		{
			LL_RCC_MSI_RC0_SetPLLInputClk(LL_RCC_MSIPLL0SEL_LSE);
			LL_RCC_MSI_RC0_PLLmode_Enable();
			while (!LL_RCC_MSI_RC0_PLLmode_IsEnabled()) {
			}
		}
		else if(STM32_MSIS_RANGE >= 4 && STM32_MSIS_RANGE <= 7)
		{
			LL_RCC_MSI_RC1_SetPLLInputClk(LL_RCC_MSIPLL1SEL_LSE);
			LL_RCC_MSI_RC1_PLLmode_Enable();
			while (!LL_RCC_MSI_RC1_PLLmode_IsEnabled()) {
			}
		}


	}

	if (IS_ENABLED(STM32_MSIK_ENABLED)) {
		/* Set MSIK Range TBD*/
		LL_RCC_MSI_SetMSIxClockRange();

		if (IS_ENABLED(STM32_MSIS_ENABLED)) {
			__ASSERT((STM32_MSIK_PLL_MODE == STM32_MSIS_PLL_MODE),
				"Please check MSIS/MSIK config consistency");
		}

		/* Enable MSIK */
		LL_RCC_MSIK_Enable();

		/* Wait till MSIK is ready */
		while (LL_RCC_MSIK_IsReady() != 1) {
		}
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		if (!LL_AHB1_GRP2_IsEnabledClock(LL_AHB1_GRP2_PERIPH_PWR)) {
		/* Enable the power interface clock */
		LL_AHB1_GRP2_EnableClock(LL_AHB1_GRP2_PERIPH_PWR);
	}

		if (!LL_PWR_IsEnabledBkUpAccess()) {
			/* Enable write access to Backup domain */
			LL_PWR_EnableBkUpAccess();
			while (!LL_PWR_IsEnabledBkUpAccess()) {
				/* Wait for Backup domain access */
			}
		}

		/* Enable LSI oscillator */
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}

		LL_PWR_DisableBkUpAccess();
	}

	if (IS_ENABLED(STM32_HSI48_ENABLED)) {
		LL_RCC_HSI48_Enable();
		while (LL_RCC_HSI48_IsReady() != 1) {
		}
	}
}

int stm32_clock_control_init(const struct device *dev)
{
	uint32_t old_hclk_freq;

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

	/* Set peripheral buses prescalers */
	LL_RCC_SetAHBPrescaler(ahb_prescaler(STM32_AHB_PRESCALER));
	LL_RCC_SetAPB1Prescaler(apb1_prescaler(STM32_APB1_PRESCALER));
	LL_RCC_SetAPB2Prescaler(apb2_prescaler(STM32_APB2_PRESCALER));
	LL_RCC_SetAPB3Prescaler(apb3_prescaler(STM32_APB3_PRESCALER));

#if STM32_SYSCLK_SRC_HSE
		/* Set HSE as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
		}
#elif STM32_SYSCLK_SRC_MSIS
		/* Set MSIS as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSIS);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSIS) {
		}
#elif STM32_SYSCLK_SRC_HSI
		/* Set HSI as SYSCLCK source */
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI16);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI16) {
		}
#else
		return -ENOTSUP;
#endif

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
		    stm32_clock_control_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &stm32_clock_control_api);
