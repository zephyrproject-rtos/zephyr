/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H7 CM4 processor
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

#if defined(CONFIG_STM32H7_BOOT_CM4_CM7)
void stm32h7_m4_boot_stop(void)
{
	/*
	 * Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for
	 * Cortex-M7 to perform system initialization (system clock config,
	 * external memory configuration.. )
	 */

	 /* Clear pending events if any */
	 __SEV();
	 __WFE();

	 /* Select the domain Power Down DeepSleep */
	 LL_PWR_SetRegulModeDS(LL_PWR_REGU_DSMODE_MAIN);
	 /* Keep DSTOP mode when D2 domain enters Deepsleep */
	 LL_PWR_CPU_SetD2PowerMode(LL_PWR_CPU_MODE_D2STOP);
	 LL_PWR_CPU2_SetD2PowerMode(LL_PWR_CPU2_MODE_D2STOP);
	 /* Set SLEEPDEEP bit of Cortex System Control Register */
	 LL_LPM_EnableDeepSleep();

	 /* Ensure that all instructions done before entering STOP mode */
	 __DSB();
	 __ISB();
	 /* Request Wait For Event */
	 __WFE();

	 /* Reset SLEEPDEEP bit of Cortex System Control Register,
	  * the following LL API Clear SLEEPDEEP bit of Cortex
	  * System Control Register
	  */
	LL_LPM_EnableSleep();
}
#endif /* CONFIG_STM32H7_BOOT_CM4_CM7 */

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32h7_m4_init(struct device *arg)
{
	u32_t key;

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/*HW semaphore Clock enable*/
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);

#if defined(CONFIG_STM32H7_BOOT_CM4_CM7)
	/* Activate HSEM notification for Cortex-M4*/
	LL_HSEM_EnableIT_C2IER(HSEM, LL_HSEM_MASK_0);

	/* Boot and enter stop mode */
	stm32h7_m4_boot_stop();

	/* Clear HSEM flag */
	LL_HSEM_ClearFlag_C2ICR(HSEM, LL_HSEM_MASK_0);
#endif /* CONFIG_STM32H7_BOOT_CM4_CM7 */

	return 0;
}

SYS_INIT(stm32h7_m4_init, PRE_KERNEL_1, 0);
