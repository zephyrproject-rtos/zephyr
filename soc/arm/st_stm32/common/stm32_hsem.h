/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_

#include <soc.h>
#include <stm32_ll_hsem.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE)
/** HW semaphore Complement ID list defined in hw_conf.h from STM32WB
 * and used also for H7 dualcore targets
 */
/**
 *  Index of the semaphore used by CPU2 to prevent the CPU1 to either write or
 *  erase data in flash. The CPU1 shall not either write or erase in flash when
 *  this semaphore is taken by the CPU2. When the CPU1 needs to either write or
 *  erase in flash, it shall first get the semaphore and release it just
 *  after writing a raw (64bits data) or erasing one sector.
 *  On v1.4.0 and older CPU2 wireless firmware, this semaphore is unused and
 *  CPU2 is using PES bit. By default, CPU2 is using the PES bit to protect its
 *  timing. The CPU1 may request the CPU2 to use the semaphore instead of the
 *  PES bit by sending the system command SHCI_C2_SetFlashActivityControl()
 */
#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID                    7U

/**
 *  Index of the semaphore used by CPU1 to prevent the CPU2 to either write or
 *  erase data in flash. In order to protect its timing, the CPU1 may get this
 *  semaphore to prevent the  CPU2 to either write or erase in flash
 *  (as this will stall both CPUs)
 *  The PES bit shall not be used as this may stall the CPU2 in some cases.
 */
#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID                    6U

/**
 *  Index of the semaphore used to manage the CLK48 clock configuration
 *  When the USB is required, this semaphore shall be taken before configuring
 *  the CLK48 for USB and should be released after the application switch OFF
 *  the clock when the USB is not used anymore. When using the RNG, it is good
 *  enough to use CFG_HW_RNG_SEMID to control CLK48.
 *  More details in AN5289
 */
#define CFG_HW_CLK48_CONFIG_SEMID                               5U
#define CFG_HW_RCC_CRRCR_CCIPR_SEMID     CFG_HW_CLK48_CONFIG_SEMID

/* Index of the semaphore used to manage the entry Stop Mode procedure */
#define CFG_HW_ENTRY_STOP_MODE_SEMID                            4U
#define CFG_HW_ENTRY_STOP_MODE_MASK_SEMID   (1U << CFG_HW_ENTRY_STOP_MODE_SEMID)

/* Index of the semaphore used to access the RCC */
#define CFG_HW_RCC_SEMID                                        3U

/* Index of the semaphore used to access the FLASH */
#define CFG_HW_FLASH_SEMID                                      2U

/* Index of the semaphore used to access the PKA */
#define CFG_HW_PKA_SEMID                                        1U

/* Index of the semaphore used to access the RNG */
#define CFG_HW_RNG_SEMID                                        0U

/** Index of the semaphore used to access GPIO */
#define CFG_HW_GPIO_SEMID                                       8U

/** Index of the semaphore used to access the EXTI */
#define CFG_HW_EXTI_SEMID                                       9U

/** Index of the semaphore for CPU1 mailbox */
#define CFG_HW_IPM_CPU1_SEMID                                   10U

/** Index of the semaphore for CPU2 mailbox */
#define CFG_HW_IPM_CPU2_SEMID                                   11U

#elif defined(CONFIG_SOC_SERIES_STM32MP1X)
/** HW semaphore from STM32MP1
 * EXTI and GPIO are inherited from STM32MP1 Linux.
 * Other SEMID are not used by linux and must not be used here,
 * but reserved for MPU.
 */
/** Index of the semaphore used to access GPIO */
#define CFG_HW_GPIO_SEMID                     0U

/** Index of the semaphore used to access the EXTI */
#define CFG_HW_EXTI_SEMID                     1U

#else
/** Fake semaphore ID definition for compilation purpose only */
#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID  0U
#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID  0U
#define CFG_HW_CLK48_CONFIG_SEMID             0U
#define CFG_HW_RCC_CRRCR_CCIPR_SEMID          0U
#define CFG_HW_ENTRY_STOP_MODE_SEMID          0U
#define CFG_HW_RCC_SEMID                      0U
#define CFG_HW_FLASH_SEMID                    0U
#define CFG_HW_PKA_SEMID                      0U
#define CFG_HW_RNG_SEMID                      0U
#define CFG_HW_GPIO_SEMID                     0U
#define CFG_HW_EXTI_SEMID                     0U
#define CFG_HW_IPM_CPU1_SEMID                 0U
#define CFG_HW_IPM_CPU2_SEMID                 0U

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
 * @brief Try to lock Hardware Semaphore
 */
static inline int z_stm32_hsem_try_lock(uint32_t hsem)
{
#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE) \
	|| defined(CONFIG_SOC_SERIES_STM32MP1X)

	if (LL_HSEM_1StepLock(HSEM, hsem)) {
		return -EAGAIN;
	}
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE || ... */

	return 0;
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

/**
 * @brief Indicates whether Hardware Semaphore is owned by this core
 */
static inline bool z_stm32_hsem_is_owned(uint32_t hsem)
{
	bool owned = false;

#if defined(CONFIG_SOC_SERIES_STM32WBX) || defined(CONFIG_STM32H7_DUAL_CORE) \
	|| defined(CONFIG_SOC_SERIES_STM32MP1X)

	owned = LL_HSEM_GetCoreId(HSEM, hsem) == LL_HSEM_COREID;
#endif /* CONFIG_SOC_SERIES_STM32WBX || CONFIG_STM32H7_DUAL_CORE || ... */

	return owned;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_HSEM_STM32_HSEM_H_ */
