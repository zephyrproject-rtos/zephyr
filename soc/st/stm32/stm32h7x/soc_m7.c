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
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <stm32_hsem.h>

#include <cmsis_core.h>

#define PWR_NODE DT_INST(0, st_stm32h7_pwr)

#if defined(CONFIG_STM32H7_DUAL_CORE)
static int stm32h7_m4_wakeup(void)
{

	/* HW semaphore and SysCfg Clock enable */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);

	if (stm32_reg_read_bits(&SYSCFG->UR1, SYSCFG_UR1_BCM4) != 0) {
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

/* Helpers to simplify following #if chain */
#define SELECTED_PSU(_x) DT_ENUM_HAS_VALUE(PWR_NODE, power_supply, _x)
#define SMPS_CONFIGURABLE_SUPPLY(_target)			\
	CONCAT(LL_PWR_SMPS_,					\
	       DT_STRING_TOKEN(PWR_NODE, smps_output_voltage),	\
	       _SUPPLIES_, _target)

#if SELECTED_PSU(ldo)
#define SELECTED_POWER_SUPPLY LL_PWR_LDO_SUPPLY
#elif SELECTED_PSU(external_source)
#define SELECTED_POWER_SUPPLY LL_PWR_EXTERNAL_SOURCE_SUPPLY
#elif SELECTED_PSU(smps_direct)
#define SELECTED_POWER_SUPPLY LL_PWR_DIRECT_SMPS_SUPPLY
#elif !DT_NODE_HAS_PROP(PWR_NODE, smps_output_voltage)
/**
 * All property values that follow require "smps-output-voltage".
 * Assert it once as part of the #if chain, and provide a dummy
 * value for SELECTED_POWER_SUPPLY, to reduce noise in compiler
 * error log.
 */
#error Invalid power supply configuration: \
'smps-output-voltage' property is required but missing
#define SELECTED_POWER_SUPPLY 0
#elif SELECTED_PSU(smps_ldo)
#define SELECTED_POWER_SUPPLY SMPS_CONFIGURABLE_SUPPLY(LDO)
#elif SELECTED_PSU(smps_ext_ldo)
#define SELECTED_POWER_SUPPLY SMPS_CONFIGURABLE_SUPPLY(EXT_AND_LDO)
#elif SELECTED_PSU(smps_ext_bypass)
#define SELECTED_POWER_SUPPLY SMPS_CONFIGURABLE_SUPPLY(EXT)
#endif

#if !defined(SMPS)
/*
 * The SoC header file defines SMPS if the feature is available.
 * If the product has no SMPS, only the LDO supply configuration
 * (and *possibly* Bypass, depending on product!) is valid: make
 * sure nothing blatantly invalid has been selected.
 */
BUILD_ASSERT(SELECTED_PSU(ldo) || SELECTED_PSU(external_source),
	     "Invalid power supply configuration: "
	     "SoC does not have SMPS step-down converter");
#endif /* SMPS */

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
	LL_PWR_ConfigSupply(SELECTED_POWER_SUPPLY);

	/* Errata ES0392 Rev 8:
	 * 2.2.9: Reading from AXI SRAM may lead to data read corruption
	 * Workaround: Set the READ_ISS_OVERRIDE bit in the AXI_TARG7_FN_MOD
	 * register.
	 * Applicable only to RevY (REV_ID 0x1003)
	 */
	if (LL_DBGMCU_GetRevisionID() == 0x1003) {
		stm32_reg_set_bits(&GPV->AXI_TARG7_FN_MOD, 0x1);
	}
}

#if defined(CONFIG_STM32H7_DUAL_CORE)
/* Unlock M4 once system configuration has been done */
SYS_INIT(stm32h7_m4_wakeup, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_STM32H7_DUAL_CORE */
