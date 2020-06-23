/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_

#include <soc.h>
#include <kernel.h>

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
/** HW semaphore Complement ID list defined in hw_conf.h from STM32WB
 * and used also for H7 dualcore targets
 */
/** Index of the semaphore used to manage the entry Stop Mode procedure */
#define CFG_HW_ENTRY_STOP_MODE_SEMID        4U
#define CFG_HW_ENTRY_STOP_MODE_MASK_SEMID   (1U << CFG_HW_ENTRY_STOP_MODE_SEMID)

/** Index of the semaphore used to access the RCC and PWR */
#define CFG_HW_RCC_SEMID                    3U

/** Index of the semaphore used to access the FLASH */
#define CFG_HW_FLASH_SEMID                  2U

/** Index of the semaphore used to access the PKA */
#define CFG_HW_PKA_SEMID                    1U

/** Index of the semaphore used to access the RNG */
#define CFG_HW_RNG_SEMID                    0U

/** Index of the semaphore used to access GPIO */
#define CFG_HW_GPIO_SEMID                   5U

/** Index of the semaphore used to access the EXTI */
#define CFG_HW_EXTI_SEMID                   6U

#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
/** HW semaphore from STM32MP1
 * EXTI and GPIO are inherited from STM32MP1 Linux.
 * Other SEMID are not used by linux and must not be used here,
 * but reserved for MPU.
 */
/** Index of the semaphore used to access GPIO */
#define CFG_HW_GPIO_SEMID                   0U

/** Index of the semaphore used to access the EXTI */
#define CFG_HW_EXTI_SEMID                   1U

#else
/** Fake semaphore ID definition for compilation purpose only */
#define CFG_HW_RCC_SEMID                    0U
#define CFG_HW_FLASH_SEMID                  0U
#define CFG_HW_PKA_SEMID                    0U
#define CFG_HW_RNG_SEMID                    0u
#define CFG_HW_GPIO_SEMID                   0U
#define CFG_HW_EXTI_SEMID                   0U

#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE */

/** Hardware Semaphore wait forever value */
#define HSEM_LOCK_WAIT_FOREVER    0xFFFFFFFFU
/** Hardware Semaphore default retry value */
#define HSEM_LOCK_DEFAULT_RETRY       0xFFFFU

/**
 * @brief Lock Hardware Semaphore
 */
static inline void z_stm32_hsem_lock(uint32_t  hsem, uint32_t retry)
{
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE) \
	|| defined(CONFIG_SOC_SERIES_STM32MP1X)

	while (LL_HSEM_1StepLock(HSEM, hsem)) {
		if (retry != HSEM_LOCK_WAIT_FOREVER) {
			retry--;
			if (retry == 0) {
				k_panic();
			}
		}
	}
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE || ... */
}

/**
 * @brief Release Hardware Semaphore
 */
static inline void z_stm32_hsem_unlock(uint32_t  hsem)
{
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE) \
	|| defined(CONFIG_SOC_SERIES_STM32MP1X)
	LL_HSEM_ReleaseLock(HSEM, hsem, 0);
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE || ... */
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_ */
