/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2025 STMicroelectronics
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
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <cortex_m/exception.h>

#if defined(CONFIG_CPU_HAS_NXP_SYSMPU)
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

	int num_regions = ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos);

	for (i = 0; i < num_regions; i++) {
		ARM_MPU_ClrRegion(i);
	}
}
#elif CONFIG_CPU_HAS_NXP_SYSMPU
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
#endif /* CONFIG_CPU_HAS_NXP_SYSMPU */
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

/**
 * @brief Save essential SCB registers into a provided context structure.
 *
 * This function reads the current values of critical System Control Block (SCB)
 * registers that are safe to backup, and stores them into the `context` structure.
 * Access to SCB registers requires atomicity and consistency, so calling code
 * should guarantee that interrupts are disabled.
 *
 * @param context Pointer to an `scb_context` structure where the register
 * values will be stored. Must not be NULL.
 */
void z_arm_save_scb_context(struct scb_context *context)
{
	__ASSERT_NO_MSG(context != NULL);

#if defined(CONFIG_CPU_CORTEX_M_HAS_VTOR)
	context->vtor = SCB->VTOR;
#endif
	context->aircr = SCB->AIRCR;
	context->scr = SCB->SCR;
	context->ccr = SCB->CCR;

	/*
	 * Backup the System Handler Priority Registers.
	 * SCB->SHPR is defined as u8[] or u32[] depending
	 * on the target Cortex-M core, but it can always
	 * be accessed using word-sized reads and writes.
	 * Make u32 pointer using explicit cast to allow
	 * access on all cores without compiler warnings.
	 */
	volatile uint32_t *shpr = (volatile uint32_t *)SCB->SHPR;

	for (int i = 0; i < SHPR_SIZE_W; i++) {
		context->shpr[i] = shpr[i];
	}

	context->shcsr = SCB->SHCSR;
#if defined(CPACR_PRESENT)
	context->cpacr = SCB->CPACR;
#endif /* CPACR_PRESENT */
}

/**
 * @brief Restores essential SCB registers from a provided context structure.
 *
 * This function writes the values from the `context` structure back to the
 * respective System Control Block (SCB) registers. Access to SCB registers
 * requires atomicity and consistency, so calling code should guarantee that
 * interrupts are disabled.
 *
 * @param context Pointer to a `scb_context` structure containing the
 * register values to be restored. Must not be NULL.
 */
void z_arm_restore_scb_context(const struct scb_context *context)
{
	__ASSERT_NO_MSG(context != NULL);

#if defined(CONFIG_CPU_CORTEX_M_HAS_VTOR)
	/* Restore VTOR if present on this CPU */
	SCB->VTOR = context->vtor;
#endif
	/* Restoring AIRCR requires writing VECTKEY along with desired bits.
	 * Mask backed up data to ensure only modifiable bits are restored.
	 */
	SCB->AIRCR = (context->aircr & ~SCB_AIRCR_VECTKEY_Msk) |
		     (AIRCR_VECT_KEY_PERMIT_WRITE << SCB_AIRCR_VECTKEY_Pos);

	SCB->SCR = context->scr;
	SCB->CCR = context->ccr;

	/* Restore System Handler Priority Registers */
	volatile uint32_t *shpr = (volatile uint32_t *)SCB->SHPR;

	for (int i = 0; i < SHPR_SIZE_W; i++) {
		shpr[i] = context->shpr[i];
	}

	/* Restore SHCSR */
	SCB->SHCSR = context->shcsr;

#if defined(CPACR_PRESENT)
	/* Restore CPACR */
	SCB->CPACR = context->cpacr;
#endif /* CPACR_PRESENT */

	/**
	 * Ensure that updates to the SCB are visible by executing a DSB followed by ISB.
	 * This sequence is recommended in the M-profile Architecture Reference Manuals:
	 *   - ARMv6: DDI0419 Issue E - §B2.5 "Barrier support for system correctness"
	 *   - ARMv7: DDI0403 Issue E.e - §A3.7.3 "Memory barriers" (at end of section)
	 *   - ARMv8: DDI0553 Version B.Y - §B7.2.16 "Synchronization requirements [...]"
	 */
	__DSB();
	__ISB();
}
