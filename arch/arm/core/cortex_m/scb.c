/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M System Control Block interface
 *
 *
 * Most of the SCB interface consists of simple bit-flipping methods, and is
 * implemented as inline functions in scb.h. This module thus contains only data
 * definitions and more complex routines, if needed.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/barrier.h>
#include <cmsis_core.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/cache.h>
#include <zephyr/arch/cache.h>

#if defined(CONFIG_CPU_HAS_NXP_MPU)
#include <fsl_sysmpu.h>
#endif

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	NVIC_SystemReset();
}

#if defined(CONFIG_ARM_MPU)
#if defined(CONFIG_CPU_HAS_ARM_MPU)
/**
 *
 * @brief Clear all MPU region configuration
 *
 * This routine clears all ARM MPU region configuration.
 *
 */
void z_arm_clear_arm_mpu_config(void)
{
	int i;

	int num_regions =
		((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos);

	for (i = 0; i < num_regions; i++) {
		ARM_MPU_ClrRegion(i);
	}
}
#elif CONFIG_CPU_HAS_NXP_MPU
void z_arm_clear_arm_mpu_config(void)
{
	int i;

	int num_regions = FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT;

	SYSMPU_Enable(SYSMPU, false);

	/* NXP MPU region 0 is reserved for the debugger */
	for (i = 1; i < num_regions; i++) {
		SYSMPU_RegionEnable(SYSMPU, i, false);
	}
}
#endif /* CONFIG_CPU_HAS_NXP_MPU */
#endif /* CONFIG_ARM_MPU */

#if defined(CONFIG_INIT_ARCH_HW_AT_BOOT)
/**
 *
 * @brief Reset system control blocks and core registers
 *
 * This routine resets Cortex-M system control block
 * components and core registers.
 *
 */
void z_arm_init_arch_hw_at_boot(void)
{
    /* Disable interrupts */
	__disable_irq();

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__set_FAULTMASK(0);
#endif

	/* Initialize System Control Block components */

#if defined(CONFIG_ARM_MPU)
	/* Clear MPU region configuration */
	z_arm_clear_arm_mpu_config();
#endif /* CONFIG_ARM_MPU */

	/* Disable NVIC interrupts */
	for (uint8_t i = 0; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
	}
	/* Clear pending NVIC interrupts */
	for (uint8_t i = 0; i < ARRAY_SIZE(NVIC->ICPR); i++) {
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

#if defined(CONFIG_ARCH_CACHE)
#if defined(CONFIG_DCACHE)
	/* Reset D-Cache settings. If the D-Cache was enabled,
	 * SCB_DisableDCache() takes care of cleaning and invalidating it.
	 * If it was already disabled, just call SCB_InvalidateDCache() to
	 * reset it to a known clean state.
	 */
	if (SCB->CCR & SCB_CCR_DC_Msk) {
		/*
		 * Do not use sys_cache_data_disable at this point, but instead
		 * the architecture specific function. This ensures that the
		 * cache is disabled although CONFIG_CACHE_MANAGEMENT might be
		 * disabled.
		 */
		SCB_DisableDCache();
	} else {
		SCB_InvalidateDCache();
	}
#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)
	/*
	 * Reset I-Cache settings.
	 * Do not use sys_cache_data_disable at this point, but instead
	 * the architecture specific function. This ensures that the
	 * cache is disabled although CONFIG_CACHE_MANAGEMENT might be
	 * disabled.
	 */
	SCB_DisableICache();
#endif /* CONFIG_ICACHE */
#endif /* CONFIG_ARCH_CACHE */

	/* Restore Interrupts */
	__enable_irq();

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}
#endif /* CONFIG_INIT_ARCH_HW_AT_BOOT */
