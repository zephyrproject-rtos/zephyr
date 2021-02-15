/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H7 CM7 processor
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include "stm32_hsem.h"

#if defined(CONFIG_STM32H7_DUAL_CORE)
static int stm32h7_m4_wakeup(const struct device *arg)
{

	/* HW semaphore and SysCfg Clock enable */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

	if (READ_BIT(SYSCFG->UR1, SYSCFG_UR1_BCM4)) {
		/* CM4 is started at boot in parallel of CM7
		 * but CM4 should set itself into stop mode,
		 * waiting for CM7 clock initialization.
		 */
		int timeout;

		/*
		 * When system initialization is finished, Cortex-M7 will
		 * release Cortex-M4  by means of HSEM notification
		 */

		/*Take HSEM */
		LL_HSEM_1StepLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID);
		/*Release HSEM in order to notify the CPU2(CM4)*/
		LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);

		/* wait until CPU2 wakes up from stop mode */
		timeout = 0xFFFF;
		while ((LL_RCC_D2CK_IsReady() == 0) && ((timeout--) > 0)) {
		}
		if (timeout < 0) {
			return -EIO;
		}
	} else {
		/* CM4 is not started at boot, start it now */
		LL_RCC_ForceCM4Boot();
	}

	return 0;
}
#endif /* CONFIG_STM32H7_DUAL_CORE */

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32h7_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	SCB_EnableICache();

	if (!(SCB->CCR & SCB_CCR_DC_Msk)) {
		SCB_EnableDCache();
	}

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz from HSI */
	SystemCoreClock = 64000000;

	/* Power Configuration */
#ifdef SMPS
	LL_PWR_ConfigSupply(LL_PWR_DIRECT_SMPS_SUPPLY);
#else
	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
#endif
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}

	return 0;
}

SYS_INIT(stm32h7_init, PRE_KERNEL_1, 0);


#if defined(CONFIG_STM32H7_DUAL_CORE)
/* Unlock M4 once system configuration has been done */
SYS_INIT(stm32h7_m4_wakeup, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_STM32H7_DUAL_CORE */
