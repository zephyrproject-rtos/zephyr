/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>

#if defined(CONFIG_ARMV8_M_MAINLINE) && !defined(ICB_CPPWR_SU11_Pos)
/*
 * The CMSIS header for some cores (e.g., Cortex-M33) lacks the definitions
 * for CPPWR even though the registers exists as long as the Main Extension
 * is implemented. Provide definitions ourselves if not found.
 */
#define ICB_CPPWR_SU11_Pos	22U
#define ICB_CPPWR_SU11_Msk	(1UL << ICB_CPPWR_SU11_Pos)
#define ICB_CPPWR_SU10_Pos	20U
#define ICB_CPPWR_SU10_Msk	(1UL << ICB_CPPWR_SU10_Pos)
#endif /* defined(CONFIG_ARMV8_M_MAINLINE) && !defined(ICB_CPPWR_SU11_Pos) */

#if DT_NODE_EXISTS(DT_NODELABEL(pm_s2ram)) &&\
	DT_NODE_HAS_COMPAT(DT_NODELABEL(pm_s2ram), zephyr_memory_region)

/* Linker section name is given by `zephyr,memory-region` property of
 * `zephyr,memory-region` compatible DT node with nodelabel `pm_s2ram`.
 */
#define __s2ram_ctx Z_GENERIC_SECTION(DT_PROP(DT_NODELABEL(pm_s2ram), zephyr_memory_region))
#else
#define __s2ram_ctx __noinit
#endif

/* CPU state preserved across S2RAM */
__s2ram_ctx _cpu_context_t _cpu_context;
__s2ram_ctx struct scb_context _scb_context;
IF_ENABLED(CONFIG_MPU, (__s2ram_ctx struct z_mpu_context_retained _mpu_context;))
IF_ENABLED(CONFIG_FPU, (__s2ram_ctx struct fpu_ctx_full _fpu_context;))

#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
/*
 * When the NVIC is used as interrupt controller, its non-volatile
 * state is saved and restored by the architecture layer.
 *
 * There should be no pending interrupt upon entry in S2RAM so this
 * information is not preserved. (If one becomes pending during the
 * S2RAM entry sequence, it will inhibit entry)
 *
 * Each bit in the 32-bit NVIC_ISERn register holds enable state for
 * one interrupt, whereas each 8-bit NVIC_IPRn register holds priority
 * for a single interrupt.
 *
 * Consume number of interrupts from CONFIG_NUM_IRQS to allow smaller
 * S2RAM context when less IRQs are used than implemented; however,
 * make sure this value is never more than what HW actually supports
 * (according to the core-specific CMSIS header - no runtime check!)
 * Note the trick: since there is one 8-bit `IPR` register per IRQ,
 * `sizeof(NVIC->IPR)` is also the maximum number of IRQs!
 */
#define NUM_IRQS_FOR_NVIC MIN(CONFIG_NUM_IRQS, sizeof(NVIC->IPR))
#define NVIC_ISER_IRQS_PER_REG (BITS_PER_BYTE * sizeof(NVIC->ISER[0]))

__s2ram_ctx uint32_t _nvic_iser[DIV_ROUND_UP(NUM_IRQS_FOR_NVIC, NVIC_ISER_IRQS_PER_REG)];
__s2ram_ctx uint8_t _nvic_ipr[NUM_IRQS_FOR_NVIC];
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

/**
 * These functions are helpers invoked from `pm_s2ram.S`
 * to save/restore CPU state other than general-purpose
 * and special registers (which are handled in assembly)
 */
void z_arm_pm_s2ram_save_scb_mpu_nvic(void)
{
	z_arm_save_scb_context(&_scb_context);
	IF_ENABLED(CONFIG_MPU, (z_arm_save_mpu_context(&_mpu_context);))

#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	for (size_t i = 0; i < ARRAY_SIZE(_nvic_iser); i++) {
		_nvic_iser[i] = NVIC->ISER[i];
	}
	for (size_t i = 0; i < ARRAY_SIZE(_nvic_ipr); i++) {
		_nvic_ipr[i] = NVIC->IPR[i];
	}
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
}

void z_arm_pm_s2ram_restore_scb_mpu_nvic(void)
{
	z_arm_restore_scb_context(&_scb_context);
	IF_ENABLED(CONFIG_MPU, (z_arm_restore_mpu_context(&_mpu_context);))


#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	for (size_t i = 0; i < ARRAY_SIZE(_nvic_iser); i++) {
		NVIC->ISER[i] = _nvic_iser[i];
	}
	for (size_t i = 0; i < ARRAY_SIZE(_nvic_ipr); i++) {
		NVIC->IPR[i] = _nvic_ipr[i];
	}
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
}

void z_arm_pm_s2ram_save_and_disable_fpu(void)
{
#if defined(CONFIG_FPU)
	/* Save context BEFORE disabling the FPU */
	z_arm_save_fp_context(&_fpu_context);

	SCB->CPACR &= ~(CPACR_CP10_Msk | CPACR_CP11_Msk);
#if defined(CONFIG_ARMV8_M_MAINLINE)
	SCnSCB->CPPWR |= (SCnSCB_CPPWR_SU10_Msk | SCnSCB_CPPWR_SU11_Msk);
#endif /* CONFIG_ARMV8_M_MAINLINE */

	__DSB();
	__ISB();
#endif /* CONFIG_FPU */
}

void z_arm_pm_s2ram_enable_and_restore_fpu(void)
{
#if defined(CONFIG_FPU)
#if defined(CONFIG_ARMV8_M_MAINLINE)
	SCnSCB->CPPWR &= ~(SCnSCB_CPPWR_SU10_Msk | SCnSCB_CPPWR_SU11_Msk);
#endif /* CONFIG_ARMV8_M_MAINLINE */
	SCB->CPACR |= ~(CPACR_CP10_Msk | CPACR_CP11_Msk);

	__DSB();
	__ISB();

	z_arm_restore_fp_context(&_fpu_context);
#endif /* CONFIG_FPU */
}

#ifndef CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING
/**
 * S2RAM Marker
 */
static __noinit uint32_t marker;

#define MAGIC (0xDABBAD00)

void pm_s2ram_mark_set(void)
{
	marker = MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	if (marker == MAGIC) {
		marker = 0;

		return true;
	}

	return false;
}

#endif /* CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING */
