/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H7 CM7 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include "stm32_hsem.h"

#include <cmsis_core.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_itcm)) &&                                             \
	!DT_SAME_NODE(DT_CHOSEN(zephyr_itcm), DT_CHOSEN(zephyr_flash))
#define ITCM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_itcm))
#define ITCM_END  (DT_REG_ADDR(DT_CHOSEN(zephyr_itcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_itcm)))

BUILD_ASSERT((ITCM_BASE & __alignof__(uint64_t)) == 0, "ITCM start address must be 64-bit aligned");
BUILD_ASSERT((ITCM_END & __alignof__(uint64_t)) == 0, "ITCM size must be 64-bit aligned");
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm)) &&                                             \
	!DT_SAME_NODE(DT_CHOSEN(zephyr_dtcm), DT_CHOSEN(zephyr_sram))
#define DTCM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm))
#define DTCM_END  (DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm)))

BUILD_ASSERT((DTCM_BASE & __alignof__(uint32_t)) == 0, "DTCM start address must be 32-bit aligned");
BUILD_ASSERT((DTCM_END & __alignof__(uint32_t)) == 0, "DTCM size must be 32-bit aligned");
#endif

#if defined(CONFIG_STM32H7_DUAL_CORE)
static int stm32h7_m4_wakeup(void)
{

	/* HW semaphore and SysCfg Clock enable */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

	if (READ_BIT(SYSCFG->UR1, SYSCFG_UR1_BCM4)) {
		/**
		 * Cortex-M4 has been started by hardware.
		 * Its `soc_early_init_hook()` will stall boot until
		 * a specific HSEM becomes locked, which indicates
		 * that Cortex-M7 has finished initializing the system.
		 * As system initialization is now complete, lock the
		 * HSEM to release CM4 and allow it to continue booting.
		 */
		LL_HSEM_1StepLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID);
	} else if (IS_ENABLED(CONFIG_STM32H7_BOOT_M4_AT_INIT)) {
		/* CM4 is not started at boot, start it now */
		LL_RCC_ForceCM4Boot();
	}

	return 0;
}
#endif /* CONFIG_STM32H7_DUAL_CORE */

void soc_reset_hook(void)
{
	/* ITCM/DTCM are equipped with ECC. On reset, the TCM content
	 * is undefined and reads, unaligned or sub-sized accesses
	 * will trigger an ECC error. Apply the TCM initialization
	 * procedure described in the ST AN5342 "How to use error
	 * correction code (ECC) management for internal memories
	 * protection on STM32 MCUs" Rev. 7, section 3.1.1: write
	 * an arbitrary value (0 in this case) to the entire TCM
	 * using ECC data word-sized writes (as shown in Table 4,
	 * this is 64-bit for ITCM and 32-bit for DTCM).
	 */
#ifdef ITCM_BASE
	volatile uint64_t *itcm_start = (void *)ITCM_BASE;
	volatile uint64_t *itcm_end = (void *)ITCM_END;

	for (volatile uint64_t *p = itcm_start; p < itcm_end; p++) {
		*p = 0;
	}
#endif
#ifdef DTCM_BASE
	volatile uint32_t *dtcm_start = (void *)DTCM_BASE;
	volatile uint32_t *dtcm_end = (void *)DTCM_END;

	for (volatile uint32_t *p = dtcm_start; p < dtcm_end; p++) {
		*p = 0;
	}
#endif
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 64 MHz from HSI */
	SystemCoreClock = 64000000;

	/* Power Configuration */
#if !defined(SMPS) && \
		(defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT))
#error Unsupported configuration: Selected SoC do not support SMPS
#endif
#if defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS)
	LL_PWR_ConfigSupply(LL_PWR_DIRECT_SMPS_SUPPLY);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_EXT_AND_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_EXT);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_EXT);
#elif defined(CONFIG_POWER_SUPPLY_EXTERNAL_SOURCE)
	LL_PWR_ConfigSupply(LL_PWR_EXTERNAL_SOURCE_SUPPLY);
#else
	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
#endif

	/* Errata ES0392 Rev 8:
	 * 2.2.9: Reading from AXI SRAM may lead to data read corruption
	 * Workaround: Set the READ_ISS_OVERRIDE bit in the AXI_TARG7_FN_MOD
	 * register.
	 * Applicable only to RevY (REV_ID 0x1003)
	 */
	if (LL_DBGMCU_GetRevisionID() == 0x1003) {
		MODIFY_REG(GPV->AXI_TARG7_FN_MOD, 0x1, 0x1);
	}
}

#if defined(CONFIG_STM32H7_DUAL_CORE)
/* Unlock M4 once system configuration has been done */
SYS_INIT(stm32h7_m4_wakeup, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_STM32H7_DUAL_CORE */
