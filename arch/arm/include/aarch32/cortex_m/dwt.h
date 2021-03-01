/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DWT utility functions for Cortex-M CPUs
 *
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_DWT_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_DWT_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arch/arm/aarch32/cortex_m/cmsis.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_CORTEX_M_DWT)

/* Define DWT LSR masks which are currently not defined by the CMSIS V5.1.2.
 * (LSR register is defined but not its bitfields).
 * Reuse ITM LSR mask as it is the same offset than DWT LSR one.
 */
#if !defined DWT_LSR_Present_Msk
#define DWT_LSR_Present_Msk ITM_LSR_Present_Msk
#endif
#if !defined DWT_LSR_Access_Msk
#define DWT_LSR_Access_Msk ITM_LSR_Access_Msk
#endif

static inline void dwt_access(bool ena)
{
#if defined(CONFIG_CPU_CORTEX_M7)
	/*
	 * In case of Cortex M7, we need to check the optional presence of
	 * Lock Access Register (LAR) which is indicated in Lock Status
	 * Register (LSR). When present, a special access token must be written
	 * to unlock DWT registers.
	 */
	uint32_t lsr = DWT->LSR;

	if ((lsr & DWT_LSR_Present_Msk) != 0) {
		if (ena) {
			if ((lsr & DWT_LSR_Access_Msk) != 0) {
				/* Access is locked. unlock it */
				DWT->LAR = 0xC5ACCE55;
			}
		} else {
			if ((lsr & DWT_LSR_Access_Msk) == 0) {
				/* Access is unlocked. Lock it */
				DWT->LAR = 0;
			}
		}
	}
#else /* CONFIG_CPU_CORTEX_M7 */
	ARG_UNUSED(ena);
#endif /* CONFIG_CPU_CORTEX_M7 */
}

/**
 * @brief Enable DWT
 *
 * This routine enables the DWT unit.
 *
 * @return 0
 */
static inline int z_arm_dwt_init(void)
{
	/* Enable tracing */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	/* Unlock DWT access if any */
	dwt_access(true);

	return 0;
}

/**
 * @brief Initialize and Enable the DWT cycle counter
 *
 * This routine enables the cycle counter and initializes its value to zero.
 *
 * @return 0
 */
static inline int z_arm_dwt_init_cycle_counter(void)
{
	/* Clear and enable the cycle counter */
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/* Assert that the cycle counter is indeed implemented. */
	__ASSERT((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) != 0,
		"DWT implements no cycle counter. "
		"Cannot be used for cycle counting\n");

	return 0;
}

/**
 * @brief Return the current value of the cycle counter
 *
 * This routine returns the current value of the DWT Cycle Counter (DWT.CYCCNT)
 *
 * @return the cycle counter value
 */
static inline uint32_t z_arm_dwt_get_cycles(void)
{
	return DWT->CYCCNT;
}

/**
 * @brief Reset and start the DWT cycle counter
 *
 * This routine starts the cycle counter and resets its value to zero.
 *
 * @return N/A
 */
static inline void z_arm_dwt_cycle_count_start(void)
{
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief Enable the debug monitor handler
 *
 * This routine enables the DebugMonitor handler to service
 * data watchpoint events coming from DWT. The routine sets
 * the DebugMonitor exception priority to highest possible.
 *
 * @return N/A
 */
static inline void z_arm_dwt_enable_debug_monitor(void)
{
	/*
	 * In case the CPU is left in Debug mode, the behavior will be
	 * unpredictable if the DebugMonitor exception is triggered. We
	 * assert that the CPU is in normal mode.
	 */
	__ASSERT((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0,
		"Cannot enable DBM when CPU is in Debug mode\n");

#if defined(CONFIG_ARMV8_M_SE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	/*
	 * By design, the DebugMonitor exception is only employed
	 * for null-pointer dereferencing detection, and enabling
	 * that feature is not supported in Non-Secure builds. So
	 * when enabling the DebugMonitor exception, assert that
	 * it is not targeting the Non Secure domain.
	 */
	__ASSERT((CoreDebug->DEMCR & DCB_DEMCR_SDME_Msk) != 0,
		"DebugMonitor targets Non-Secure\n");
#endif

	/* Set the DebugMonitor handler priority to the higyhest value. */
	NVIC_SetPriority(DebugMonitor_IRQn, _EXC_FAULT_PRIO);

	/* Enable debug monitor exception triggered on debug events */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
}

#endif /* CONFIG_CORTEX_M_DWT */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_DWT_H_ */
