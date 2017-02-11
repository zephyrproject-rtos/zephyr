/*
 *
 * Copyright (c) 2017 Linaro Limited.
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

#define _apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) _apb2_prescaler(v)

/**
 * @brief fill in AHB/APB buses configuration structure
 */
static void config_bus_clk_init(LL_UTILS_ClkInitTypeDef *clk_init)
{
	clk_init->AHBCLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_AHB_PRESCALER);
	clk_init->APB1CLKDivider = apb1_prescaler(
					CONFIG_CLOCK_STM32_APB1_PRESCALER);
	clk_init->APB2CLKDivider = apb2_prescaler(
					CONFIG_CLOCK_STM32_APB2_PRESCALER);
}

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
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
#ifdef CONFIG_SOC_SERIES_STM32L4X
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_EnableClock(pclken->enr);
		break;
#ifdef CONFIG_SOC_SERIES_STM32L4X
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X */
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_EnableClock(pclken->enr);
		break;
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
#ifdef CONFIG_SOC_SERIES_STM32L4X
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32L4X */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_DisableClock(pclken->enr);
		break;
#ifdef CONFIG_SOC_SERIES_STM32L4X
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_DisableClock(pclken->enr);
		break;
#endif
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_DisableClock(pclken->enr);
		break;
	}

	return 0;
}


static int stm32_clock_control_get_subsys_rate(struct device *clock,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);
	/*
	 * Get AHB Clock (= SystemCoreClock = SYSCLK/prescaler)
	 * SystemCoreClock is preferred to CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
	 * since it will be updated after clock configuration and hence
	 * more likely to contain actual clock speed
	 */
	uint32_t ahb_clock = SystemCoreClock;
	uint32_t apb1_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_APB1_PRESCALER);
	uint32_t apb2_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_APB2_PRESCALER);

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
#ifdef CONFIG_SOC_SERIES_STM32L4X
	case STM32_CLOCK_BUS_APB1_2:
#endif
		*rate = apb1_clock;
		break;
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
	}

	return 0;
}

static struct clock_control_driver_api stm32_clock_control_api = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
};

static int stm32_clock_control_init(struct device *dev)
{
	LL_UTILS_ClkInitTypeDef s_ClkInitStruct;

	ARG_UNUSED(dev);

	/* configure clock for AHB/APB buses */
	config_bus_clk_init((LL_UTILS_ClkInitTypeDef *)&s_ClkInitStruct);

#ifdef CONFIG_CLOCK_STM32_SYSCLK_SRC_PLL
	LL_UTILS_PLLInitTypeDef s_PLLInitStruct;

	/* configure PLL input settings */
	config_pll_init(&s_PLLInitStruct);

	/* Disable PLL before configuration */
	LL_RCC_PLL_Disable();

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
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);

	/* Set flash latency */
	/* HSI used as SYSCLK, set latency to 0 */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_MSI_Disable();
	LL_RCC_PLL_Disable();

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI

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
	LL_RCC_SetAHBPrescaler(s_ClkInitStruct.AHBCLKDivider);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(__LL_RCC_CALC_HCLK_FREQ(HSI_VALUE,
						  s_ClkInitStruct.AHBCLKDivider));

    /* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);

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
