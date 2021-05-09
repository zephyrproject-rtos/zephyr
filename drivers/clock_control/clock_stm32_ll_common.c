/*
 *
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <stm32_ll_utils.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "clock_stm32_ll_common.h"

/* Macros to fill up prescaler values */
#define z_ahb_prescaler(v) LL_RCC_SYSCLK_DIV_ ## v
#define ahb_prescaler(v) z_ahb_prescaler(v)

#define z_apb1_prescaler(v) LL_RCC_APB1_DIV_ ## v
#define apb1_prescaler(v) z_apb1_prescaler(v)

#ifndef CONFIG_SOC_SERIES_STM32F0X
#define z_apb2_prescaler(v) LL_RCC_APB2_DIV_ ## v
#define apb2_prescaler(v) z_apb2_prescaler(v)
#endif /* CONFIG_SOC_SERIES_STM32F0X  */

#define z_mco1_prescaler(v) LL_RCC_MCO1_DIV_ ## v
#define mco1_prescaler(v) z_mco1_prescaler(v)

#define z_mco2_prescaler(v) LL_RCC_MCO2_DIV_ ## v
#define mco2_prescaler(v) z_mco2_prescaler(v)

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_SOC_SERIES_STM32WLX)
#define __LL_RCC_CALC_HCLK_FREQ __LL_RCC_CALC_HCLK1_FREQ
#endif /* CONFIG_SOC_SERIES_STM32F0X */

#if CONFIG_CLOCK_STM32_AHB_PRESCALER > 1
/*
 * AHB prescaler allows to set a HCLK frequency (feeding cortex systick)
 * lower than SYSCLK frequency (actual core frequency).
 * Though, zephyr doesn't make a difference today between these two clocks.
 * So, changing this prescaler is not allowed until it is made possible to
 * use them independently in zephyr clock subsystem.
 */
#error "AHB prescaler can't be higher than 1"
#endif

/**
 * @brief fill in AHB/APB buses configuration structure
 */
static void config_bus_clk_init(LL_UTILS_ClkInitTypeDef *clk_init)
{
#if defined(CONFIG_SOC_SERIES_STM32WBX)
	clk_init->CPU1CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_CPU1_PRESCALER);
	clk_init->CPU2CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_CPU2_PRESCALER);
	clk_init->AHB4CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_AHB4_PRESCALER);
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
	clk_init->CPU1CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_CPU1_PRESCALER);
	clk_init->CPU2CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_CPU2_PRESCALER);
	clk_init->AHB3CLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_AHB3_PRESCALER);
#else
	clk_init->AHBCLKDivider = ahb_prescaler(
					CONFIG_CLOCK_STM32_AHB_PRESCALER);
#endif /* CONFIG_SOC_SERIES_STM32WBX */

	clk_init->APB1CLKDivider = apb1_prescaler(
					CONFIG_CLOCK_STM32_APB1_PRESCALER);

#if !defined (CONFIG_SOC_SERIES_STM32F0X) && \
	!defined (CONFIG_SOC_SERIES_STM32G0X)
	clk_init->APB2CLKDivider = apb2_prescaler(
					CONFIG_CLOCK_STM32_APB2_PRESCALER);
#endif
}

static uint32_t get_bus_clock(uint32_t clock, uint32_t prescaler)
{
	return clock / prescaler;
}

static inline int stm32_clock_control_on(const struct device *dev,
					 clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_EnableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) && defined(RCC_AHB2_SUPPORT) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) && defined(RCC_AHB3_SUPPORT) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_EnableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
#if !defined(CONFIG_SOC_SERIES_STM32F0X)
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_EnableClock(pclken->enr);
		break;
#endif
#if defined (CONFIG_SOC_SERIES_STM32L0X) || \
	defined (CONFIG_SOC_SERIES_STM32G0X)
	case STM32_CLOCK_BUS_IOP:
		LL_IOP_GRP1_EnableClock(pclken->enr);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}


static inline int stm32_clock_control_off(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)(sub_system);

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
		LL_AHB1_GRP1_DisableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) && defined(RCC_AHB2_SUPPORT) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_AHB2:
		LL_AHB2_GRP1_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) && defined(RCC_AHB3_SUPPORT) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_AHB3:
		LL_AHB3_GRP1_EnableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
	case STM32_CLOCK_BUS_APB1:
		LL_APB1_GRP1_DisableClock(pclken->enr);
		break;
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_APB1_2:
		LL_APB1_GRP2_DisableClock(pclken->enr);
		break;
#endif /* CONFIG_SOC_SERIES_STM32_* */
#if !defined(CONFIG_SOC_SERIES_STM32F0X)
	case STM32_CLOCK_BUS_APB2:
		LL_APB2_GRP1_DisableClock(pclken->enr);
		break;
#endif
#if defined (CONFIG_SOC_SERIES_STM32L0X) || \
	defined (CONFIG_SOC_SERIES_STM32G0X)
	case STM32_CLOCK_BUS_IOP:
		LL_IOP_GRP1_DisableClock(pclken->enr);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}


static int stm32_clock_control_get_subsys_rate(const struct device *clock,
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
#if !defined (CONFIG_SOC_SERIES_STM32F0X) && \
	!defined (CONFIG_SOC_SERIES_STM32G0X)
	uint32_t apb2_clock = get_bus_clock(ahb_clock,
				CONFIG_CLOCK_STM32_APB2_PRESCALER);
#endif

	ARG_UNUSED(clock);

	switch (pclken->bus) {
	case STM32_CLOCK_BUS_AHB1:
	case STM32_CLOCK_BUS_AHB2:
	case STM32_CLOCK_BUS_AHB3:
#if defined (CONFIG_SOC_SERIES_STM32L0X) || \
	defined (CONFIG_SOC_SERIES_STM32G0X)
	case STM32_CLOCK_BUS_IOP:
#endif
		*rate = ahb_clock;
		break;
	case STM32_CLOCK_BUS_APB1:
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	case STM32_CLOCK_BUS_APB1_2:
#endif /* CONFIG_SOC_SERIES_STM32_* */
#if defined(CONFIG_SOC_SERIES_STM32G0X)
	case STM32_CLOCK_BUS_APB2:
		/*
		 * STM32G0x only has one APB, but two reset/clock enable
		 * registers for peripherals, so return the APB1 clock rate here
		 */
#endif /* CONFIG_SOC_SERIES_STM32G0X */
		*rate = apb1_clock;
		break;
#if !defined (CONFIG_SOC_SERIES_STM32F0X) && \
	!defined (CONFIG_SOC_SERIES_STM32G0X)
	case STM32_CLOCK_BUS_APB2:
		*rate = apb2_clock;
		break;
#endif
	default:
		return -ENOTSUP;
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
static void stm32_clock_switch_to_hsi(uint32_t ahb_prescaler)
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

/*
 * MCO configure doesn't active requested clock source,
 * so please make sure the clock source was enabled.
 */
static inline void stm32_clock_control_mco_init(void)
{
#ifndef CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK
	LL_RCC_ConfigMCO(MCO1_SOURCE,
			 mco1_prescaler(CONFIG_CLOCK_STM32_MCO1_DIV));
#endif /* CONFIG_CLOCK_STM32_MCO1_SRC_NOCLOCK */

#ifndef CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK
	LL_RCC_ConfigMCO(MCO2_SOURCE,
			 mco2_prescaler(CONFIG_CLOCK_STM32_MCO2_DIV));
#endif /* CONFIG_CLOCK_STM32_MCO2_SRC_NOCLOCK */
}


/**
 * @brief Initialize clocks for the stm32
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
int stm32_clock_control_init(const struct device *dev)
{
	LL_UTILS_ClkInitTypeDef s_ClkInitStruct;
	uint32_t hclk_prescaler;
#if defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE) || \
	defined(CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI)
	uint32_t old_hclk_freq;
	uint32_t new_hclk_freq;
#endif

	ARG_UNUSED(dev);

	/* configure clock for AHB/APB buses */
	config_bus_clk_init((LL_UTILS_ClkInitTypeDef *)&s_ClkInitStruct);

	/* update local hclk prescaler variable */
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_SOC_SERIES_STM32WLX)
	hclk_prescaler = s_ClkInitStruct.CPU1CLKDivider;
#else
	hclk_prescaler = s_ClkInitStruct.AHBCLKDivider;
#endif /* CONFIG_SOC_SERIES_STM32WBX */

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

#ifdef CONFIG_SOC_SERIES_STM32F7X
	 /* Assuming we stay on Power Scale default value: Power Scale 1 */
	 if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC > 180000000) {
		 LL_PWR_EnableOverDriveMode();
		 while (LL_PWR_IsActiveFlag_OD() != 1) {
		 /* Wait for OverDrive mode ready */
		 }
		 LL_PWR_EnableOverDriveSwitching();
		 while (LL_PWR_IsActiveFlag_ODSW() != 1) {
		 /* Wait for OverDrive switch ready */
		 }
	 }
#endif

#ifdef CONFIG_CLOCK_STM32_PLL_Q_DIVISOR
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ,
		   CONFIG_CLOCK_STM32_PLL_Q_DIVISOR
		   << RCC_PLLCFGR_PLLQ_Pos);
#endif /* CONFIG_CLOCK_STM32_PLL_Q_DIVISOR */

#ifdef CONFIG_CLOCK_STM32_PLL_SRC_MSI

	/* Set MSI Range */
	LL_RCC_MSI_EnableRangeSelection();
	LL_RCC_MSI_SetRange(CONFIG_CLOCK_STM32_MSI_RANGE
							<< RCC_CR_MSIRANGE_Pos);
	LL_RCC_MSI_SetCalibTrimming(0);

#ifdef CONFIG_CLOCK_STM32_MSI_PLL_MODE
	/* Enable MSI hardware auto calibration */
	LL_RCC_MSI_EnablePLLMode();
#endif

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
	int hse_bypass;

	if (IS_ENABLED(CONFIG_CLOCK_STM32_HSE_BYPASS)) {
		hse_bypass = LL_UTILS_HSEBYPASS_ON;
	} else {
		hse_bypass = LL_UTILS_HSEBYPASS_OFF;
	}

	/* Switch to PLL with HSE as clock source */
#ifdef CONFIG_SOC_SERIES_STM32WLX
	LL_RCC_HSE_EnableTcxo();
#endif
	LL_PLL_ConfigSystemClock_HSE(
#if !defined(CONFIG_SOC_SERIES_STM32WBX) && !defined(CONFIG_SOC_SERIES_STM32WLX)
		CONFIG_CLOCK_STM32_HSE_CLOCK,
#endif
#ifndef CONFIG_SOC_SERIES_STM32WLX
		hse_bypass,
#endif
		&s_PLLInitStruct,
		&s_ClkInitStruct);

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_MSI_Disable();

#endif /* CONFIG_CLOCK_STM32_PLL_SRC_* */

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_HSE

	old_hclk_freq = HAL_RCC_GetHCLKFreq();

	/* Calculate new SystemCoreClock variable based on HSE freq */
	new_hclk_freq = __LL_RCC_CALC_HCLK_FREQ(CONFIG_CLOCK_STM32_HSE_CLOCK,
						hclk_prescaler);
#if defined(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
	__ASSERT(new_hclk_freq == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			 "Config mismatch HCLK frequency %u %u",
			 CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, new_hclk_freq);
#endif

	/* If freq increases, set flash latency before any clock setting */
	if (new_hclk_freq > old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

	/* Enable HSE if not enabled */
	if (LL_RCC_HSE_IsReady() != 1) {
		/* Check if need to enable HSE bypass feature or not */
		if (IS_ENABLED(CONFIG_CLOCK_STM32_HSE_BYPASS)) {
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

	/* Set HSE as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
	LL_RCC_SetAHBPrescaler(hclk_prescaler);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE) {
	}

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(new_hclk_freq);

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
#if !defined (CONFIG_SOC_SERIES_STM32F0X) && \
	!defined (CONFIG_SOC_SERIES_STM32G0X)
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);
#endif

	/* If freq not increased, set flash latency after all clock setting */
	if (new_hclk_freq <= old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

	/* Disable other clocks */
	LL_RCC_HSI_Disable();
	LL_RCC_MSI_Disable();
	LL_RCC_PLL_Disable();

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_MSI

	old_hclk_freq = HAL_RCC_GetHCLKFreq();

	/* Calculate new SystemCoreClock variable with MSI freq */
	/* MSI freq is defined from RUN range selection */
	new_hclk_freq =
		__LL_RCC_CALC_HCLK_FREQ(
			__LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGESEL_RUN,
			CONFIG_CLOCK_STM32_MSI_RANGE << RCC_CR_MSIRANGE_Pos),
			hclk_prescaler);

#if defined(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
	__ASSERT(new_hclk_freq == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			 "Config mismatch HCLK frequency %u %u",
			 CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, new_hclk_freq);
#endif

	/* If freq increases, set flash latency before any clock setting */
	if (new_hclk_freq > old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

	/* Set MSI Range */
	LL_RCC_MSI_EnableRangeSelection();
	LL_RCC_MSI_SetRange(CONFIG_CLOCK_STM32_MSI_RANGE << RCC_CR_MSIRANGE_Pos);

#if defined(CONFIG_CLOCK_STM32_MSI_PLL_MODE)
	/* Enable MSI hardware auto calibration */
	LL_RCC_MSI_EnablePLLMode();
#endif

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
	LL_RCC_SetAHBPrescaler(hclk_prescaler);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {
	}

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(new_hclk_freq);

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);
#ifdef CONFIG_SOC_SERIES_STM32WBX
	/* Set C2 AHB & AHB4 prescalers */
	LL_C2_RCC_SetAHBPrescaler(s_ClkInitStruct->CPU2CLKDivider);
	LL_RCC_SetAHB4Prescaler(s_ClkInitStruct->AHB4CLKDivider);
#endif /* CONFIG_SOC_SERIES_STM32WBX */
#ifdef CONFIG_SOC_SERIES_STM32WLX
	/* Set C2 AHB & AHB3 prescalers */
	LL_C2_RCC_SetAHBPrescaler(s_ClkInitStruct->CPU2CLKDivider);
	LL_RCC_SetAHB3Prescaler(s_ClkInitStruct->AHB3CLKDivider);
#endif /* CONFIG_SOC_SERIES_STM32WLX */
	/* If freq not increased, set flash latency after all clock setting */
	if (new_hclk_freq <= old_hclk_freq) {
		LL_SetFlashLatency(new_hclk_freq);
	}

	/* Disable other clocks */
	LL_RCC_HSE_Disable();
	LL_RCC_HSI_Disable();
	LL_RCC_PLL_Disable();

#elif CONFIG_CLOCK_STM32_SYSCLK_SRC_HSI

	stm32_clock_switch_to_hsi(hclk_prescaler);

	/* Update SystemCoreClock variable */
	LL_SetSystemCoreClock(__LL_RCC_CALC_HCLK_FREQ(HSI_VALUE,
						      hclk_prescaler));

	/* Set APB1 & APB2 prescaler*/
	LL_RCC_SetAPB1Prescaler(s_ClkInitStruct.APB1CLKDivider);
#if !defined (CONFIG_SOC_SERIES_STM32F0X) && \
	!defined (CONFIG_SOC_SERIES_STM32G0X)
	LL_RCC_SetAPB2Prescaler(s_ClkInitStruct.APB2CLKDivider);
#endif /* CONFIG_SOC_SERIES_STM32F0X && CONFIG_SOC_SERIES_STM32G0X */

	/* Set flash latency */
	/* HSI used as SYSCLK, set latency to 0 */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

	/* Disable other clocks */
	LL_RCC_HSE_Disable();
	LL_RCC_MSI_Disable();
	LL_RCC_PLL_Disable();

#endif /* CONFIG_CLOCK_STM32_SYSCLK_SRC_... */

	/* configure MCO1/MCO2 based on Kconfig */
	stm32_clock_control_mco_init();

	return 0;
}

/**
 * @brief RCC device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcc),
		    &stm32_clock_control_init,
		    device_pm_control_nop,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_STM32_DEVICE_INIT_PRIORITY,
		    &stm32_clock_control_api);
