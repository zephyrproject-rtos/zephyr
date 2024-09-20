/*
 * Copyright (c) 2021 Andrés Manelli <am@toroid.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief System module to support early STM32 MCU configuration
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <stm32_ll_system.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>

/**
 * @brief Perform SoC configuration at boot.
 *
 * This should be run early during the boot process but after basic hardware
 * initialization is done.
 *
 * @return 0
 */
static int st_stm32_common_config(void)
{
#ifdef CONFIG_LOG_BACKEND_SWO
	/* Enable SWO trace asynchronous mode */
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_DBGMCU_EnableTraceClock();
#endif
#if !defined(CONFIG_SOC_SERIES_STM32WBX) && defined(DBGMCU_CR_TRACE_IOEN)
	LL_DBGMCU_SetTracePinAssignment(LL_DBGMCU_TRACE_ASYNCH);
#endif
#endif /* CONFIG_LOG_BACKEND_SWO */


#if defined(CONFIG_USE_SEGGER_RTT)
	/* On some STM32 boards, for unclear reason,
	 * RTT feature is working with realtime update only when
	 *   - one of the DMA is clocked.
	 * See https://github.com/zephyrproject-rtos/zephyr/issues/34324
	 */
#if defined(__HAL_RCC_DMA1_CLK_ENABLE)
	__HAL_RCC_DMA1_CLK_ENABLE();
#elif defined(__HAL_RCC_GPDMA1_CLK_ENABLE)
	__HAL_RCC_GPDMA1_CLK_ENABLE();
#endif /* __HAL_RCC_DMA1_CLK_ENABLE */


#endif /* CONFIG_USE_SEGGER_RTT */

	/* On some STM32 boards, for unclear reason,
	 * RTT feature is working with realtime update only when
	 *   - one of the DBGMCU bit STOP/STANDBY/SLEEP is set
	 * See https://github.com/zephyrproject-rtos/zephyr/issues/34324
	 */

	/* Enable DBGMCU clock if it exists */
#if defined(LL_APB1_GRP1_PERIPH_DBGMCU)
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
#elif defined(LL_APB1_GRP2_PERIPH_DBGMCU)
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(LL_APB2_GRP1_PERIPH_DBGMCU)
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#endif /* LL_APB1_GRP1_PERIPH_DBGMCU */

#if defined(CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP)

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_DBGMCU_EnableD1DebugInStopMode();
	LL_DBGMCU_EnableD1DebugInSleepMode();
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
	LL_DBGMCU_EnableDebugInStopMode();
#elif defined(CONFIG_SOC_SERIES_STM32WB0X)
	LL_PWR_EnableDEEPSTOP2();
#else /* all other parts */
	LL_DBGMCU_EnableDBGStopMode();
#endif

#else

/* keeping in mind that debugging draws a lot of power we explcitly disable when not needed */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_DBGMCU_DisableD1DebugInStopMode();
	LL_DBGMCU_DisableD1DebugInSleepMode();
#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
	LL_DBGMCU_DisableDebugInStopMode();
#elif defined(CONFIG_SOC_SERIES_STM32WB0X)
	LL_PWR_DisableDEEPSTOP2();
#else /* all other parts */
	LL_DBGMCU_DisableDBGStopMode();
#endif

#endif /* CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP */

	/* Disable DBGMCU clock if it exists */
#if defined(LL_APB1_GRP1_PERIPH_DBGMCU)
	LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
#elif defined(LL_APB1_GRP2_PERIPH_DBGMCU)
	LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_DBGMCU);
#elif defined(LL_APB2_GRP1_PERIPH_DBGMCU)
	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_DBGMCU);
#endif /* LL_APB1_GRP1_PERIPH_DBGMCU */

	return 0;
}

SYS_INIT(st_stm32_common_config, PRE_KERNEL_1, 1);
