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
#include <arch/cpu.h>
#include <cortex_m/exc.h>

#if defined(CONFIG_STM32H7_DUAL_CORE)
static int stm32h7_m4_wakeup(struct device *arg)
{

	/*HW semaphore Clock enable*/
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);

	if (IS_ENABLED(CONFIG_STM32H7_BOOT_CM4_CM7)) {
		int timeout;

		/*
		 * When system initialization is finished, Cortex-M7 will
		 * release Cortex-M4  by means of HSEM notification
		 */

		/*Take HSEM */
		LL_HSEM_1StepLock(HSEM, LL_HSEM_ID_0);
		/*Release HSEM in order to notify the CPU2(CM4)*/
		LL_HSEM_ReleaseLock(HSEM, LL_HSEM_ID_0, 0);

		/* wait until CPU2 wakes up from stop mode */
		timeout = 0xFFFF;
		while ((LL_RCC_D2CK_IsReady() == 0) && ((timeout--) > 0)) {
		}
		if (timeout < 0) {
			return -EIO;
		}
	} else if (IS_ENABLED(CONFIG_STM32H7_BOOT_CM7_CM4GATED)) {
		/* Start CM4 */
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
static int stm32h7_init(struct device *arg)
{
	u32_t key;

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

	return 0;
}

SYS_INIT(stm32h7_init, PRE_KERNEL_1, 0);


#if defined(CONFIG_STM32H7_DUAL_CORE)
/* Unlock M4 once system configuration has been done */
SYS_INIT(stm32h7_m4_wakeup, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_STM32H7_DUAL_CORE */
