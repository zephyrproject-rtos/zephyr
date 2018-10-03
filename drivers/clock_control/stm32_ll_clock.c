/*
 *
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_registers.h>
#include <clock_control.h>
#include <misc/util.h>
#include <clock_control/stm32_clock_control.h>
#include "stm32_ll_clock.h"

/* Macros to fill up prescaler values */
#define _ahb_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) _ahb_prescaler(v)

#define _apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) _apb1_prescaler(v)

#ifndef CONFIG_SOC_SERIES_STM32F0X
#define _apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) _apb2_prescaler(v)
#endif /* CONFIG_SOC_SERIES_STM32F0X  */

/**
 * @brief fill in AHB/APB buses configuration structure
 */
static void config_bus_clk_init(LL_UTILS_ClkInitTypeDef *clk_init)
{
	clk_init->AHBCLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_AHB_PRESCALER);
	clk_init->APB1CLKDivider = apb1_prescaler(
					CONFIG_CLOCK_STM32_APB1_PRESCALER);
#ifndef CONFIG_SOC_SERIES_STM32F0X
	clk_init->APB2CLKDivider = apb2_prescaler(
					CONFIG_CLOCK_STM32_APB2_PRESCALER);
#endif /* CONFIG_SOC_SERIES_STM32F0X  */
}

static u32_t get_bus_clock(u32_t clock, u32_t prescaler)
{
	return clock / prescaler;
}

static inline int stm32_clock_control_on(struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_EnableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X)
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X || CONFIG_SOC_SERIES_STM32F4X ||
		CONFIG_SOC_SERIES_STM32F7X */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_EnableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X || CONFIG_SOC_SERIES_STM32F0X */
#ifndef CONFIG_SOC_SERIES_STM32F0X
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32F0X */
#ifdef CONFIG_SOC_SERIES_STM32L0X
	case STM32_CLOCK_BUS_IOP:
		LL_IOP_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L0X */
	}

	return 0;
}


static inline int stm32_clock_control_off(struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_DisableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X)
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X || CONFIG_SOC_SERIES_STM32F4X ||
		CONFIG_SOC_SERIES_STM32F7X */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_DisableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X || CONFIG_SOC_SERIES_STM32F0X */
#ifndef CONFIG_SOC_SERIES_STM32F0X
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32F0X */
#ifdef CONFIG_SOC_SERIES_STM32L0X
	case STM32_CLOCK_BUS_IOP:
		LL_IOP_GRP1_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L0X */
	}

	return 0;
}


static int stm32_clock_control_get_subsys_rate(struct device *clock,
						clock_control_subsys_t sub_system,
						u32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	/*
	 * Get AHB Clock (= SystemCoreClock = SYSCLK/prescaler)
	 * SystemCoreClock is preferred to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	 * since it will be updated after clock configuration and hence
	 * more likely to contain actual clock speed
	 */
	u32_t ahb_clock = SystemCoreClock;
	u32_t apb1_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_APB1_PRESCALER);
#ifndef CONFIG_SOC_SERIES_STM32F0X
	u32_t apb2_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_APB2_PRESCALER);
#endif /* CONFIG_SOC_SERIES_STM32F0X */

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
#ifdef CONFIG_SOC_SERIES_STM32L0X
	case STM32_CLOCK_BUS_IOP:
#endif /* CONFIG_SOC_SERIES_STM32L0X */
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32F0X)
	case STM32_CLOCK_BUS_APB1_2:
#endif /* CONFIG_SOC_SERIES_STM32L4X || CONFIG_SOC_SERIES_STM32F0X  */
		*rate = apb1_clock;
		break;
#ifndef CONFIG_SOC_SERIES_STM32F0X
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
#endif /* CONFIG_SOC_SERIES_STM32F0X */
	}

	return 0;
}

static struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

/*
 * Unconditionally switch the system clock source to HSI.
 */
__unused
static void stm32_clock_switch_to_hsi(u32_t ahb_prescaler)
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
	LL_RCC_SetAHBPrescaler(ahb_prescaler);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}
}

static int stm32_clock_control_init(struct device *dev)
{
	LL_UTILS_ClkInitTypeDef s_ClkInitStruct;

	ARG_UNUSED(dev);

	/* configure clock for AHB/APB buses */
	config_bus_clk_init((LL_UTILS_ClkInitTypeDef *)&s_ClkInitStruct);

	/* Some clocks would be activated by default */
	config_enable_default_clocks();

#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL
	LL_UTILS_PLLInitTypeDef s_PLLInitStruct;

	/* configure PLL input settings */
	config_pll_init(&s_PLLInitStruct);

	/*
	 * Switch to HSI and disable the PLL before configuration.
	 * (Switching to HSI makes sure we have a SYSCLK source in
	 * case we're currently running from the PLL we're about to
	 * turn off and reconfigure.)
	 *
	 * Don't use s_ClkInitStruct.AHBCLKDivider as the AHB
	 * prescaler here. In this configuration, that's the value to
	 * use when the SYSCLK source is the PLL, not HSI.
	 */
	stm32_clock_switch_to_hsi(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_PLL_Disable();

#ifdef CONFIG_CLOCK_STM32_PLL_Q_DIVISOR
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ,
		   CONFIG_CLOCK_STM32_PLL_Q_DIVISOR
		   << POSITION_VAL(RCC_PLLCFGR_PLLQ));
#endif /* CONFIG_CLOCK_STM32_PLL_Q_DIVISOR */

#ifdef CONFIG_CLOCK_STM32_PLL_SRC_MSI
	/* Switch to PLL with MSI as clock source */
	LL_PLL_ConfigSystemClock_MSI(&s_PLLInitStruct, &s_ClkInitStruct);

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_HSE_Disable();

#elif CONFIG_CLOCK_STM32_PLL_SRC_HSI
	/* Switch to PLL with HSI as clock source */
	LL_PLL_ConfigSystemClock_HSI(&s_PLLInitStruct, &s_ClkInitStruct);

	/* Disable other clocks */
	LL_RCC_HSE_Disable();
	LL_RCC_MSI_Disable();

#elif CONFIG_CLOCK_STM32_PLL_SRC_HSE
	int hse_bypass = LL_UTILS_HSEBYPASS_OFF;

#ifdef CONFIG_CLOCK_STM32_HSE_BYPASS
	hse_bypass = LL_UTILS_HSEBYPASS_ON;
#endif /* CONFIG_CLOCK_STM32_HSE_BYPASS */

	/* Switch to PLL with HSE as clock source */
	LL_PLL_ConfigSystemClock_HSE(CONFIG_CLOCK_STM32_HSE_CLOCK, hse_bypass,
							&s_PLLInitStruct,
							&s_ClkInitStruct);

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_MSI_Disable();

#endif /* CONFIG_CLOCK_STM32_PLL_SRC_... */

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE

	/* Enable HSE if not enabled */
	if (LL_RCC_HSE_IsReady() != 1) {
		/* Check if need to enable HSE bypass feature or not */
#ifdef CONFIG_CLOCK_STM32_HSE_BYPASS
		LL_RCC_HSE_EnableBypass();
#else
		LL_RCC_HSE_DisableBypass();
#endif /* CONFIG_CLOCK_STM32_HSE_BYPASS */

		/* Enable HSE */
		LL_RCC_HSE_Enable();
		while (LL_RCC_HSE_IsReady() != 1) {
		/* Wait for HSE ready */
		}
	}

	/* Set HSE as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
	LL_RCC_SetAHBPrescaler(s_ClkInitStruct.AHBCLKDivider);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
	}

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(__LL_RCC_CALC_HCLK_FREQ(
					      CONFIG_CLOCK_STM32_HSE_CLOCK,
					      s_ClkInitStruct.AHBCLKDivider));

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
#ifndef CONFIG_SOC_SERIES_STM32F0X
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);
#endif /* CONFIG_SOC_SERIES_STM32F0X */

	/* Set flash latency */
	/* HSI used as SYSCLK, set latency to 0 */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_MSI_Disable();
	LL_RCC_PLL_Disable();

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI

	/* Set MSI Range */
	LL_RCC_MSI_EnableRangeSelection();
	LL_RCC_MSI_SetRange(CONFIG_CLOCK_STM32_MSI_RANGE << RCC_CR_MSIRANGE_Pos);

	/* Enable MSI if not enabled */
	if (LL_RCC_MSI_IsReady() != 1) {
		/* Enable MSI */
		LL_RCC_MSI_Enable();
		while (LL_RCC_MSI_IsReady() != 1) {
		/* Wait for HSI ready */
		}
	}

	/* Set MSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
	LL_RCC_SetAHBPrescaler(s_ClkInitStruct.AHBCLKDivider);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {
	}

	/* Update SystemCoreClock variable with MSI freq */
	/* MSI freq is defined from RUN range selection */
	LL_SetSystemCoreClock(__LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGESEL_RUN,
						     LL_RCC_MSI_GetRange()));

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);

	/* Set flash latency */
	/* MSI used as SYSCLK (16MHz), set latency to 0 */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	/* Disable other clocks */
	LL_RCC_HSE_Disable();
	LL_RCC_HSI_Disable();
	LL_RCC_PLL_Disable();

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI

	stm32_clock_switch_to_hsi(s_ClkInitStruct.AHBCLKDivider);

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(__LL_RCC_CALC_HCLK_FREQ(HSI_VALUE,
						s_ClkInitStruct.AHBCLKDivider));

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
#ifndef CONFIG_SOC_SERIES_STM32F0X
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);
#endif /* CONFIG_SOC_SERIES_STM32F0X */

	/* Set flash latency */
	/* HSI used as SYSCLK, set latency to 0 */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	/* Disable other clocks */
	LL_RCC_HSE_Disable();
	LL_RCC_MSI_Disable();
	LL_RCC_PLL_Disable();

#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_... */

	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_AND_API_INIT(rcc_stm32, STM32_CLOCK_CONTROL_NAME,
		    &stm32_clock_control_init,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32_DEVICE_INIT_PRIORITY,
		    &stm32_clock_control_api);
