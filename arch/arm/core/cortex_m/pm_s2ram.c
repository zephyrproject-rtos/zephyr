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

/**
 * These functions are helpers invoked from `pm_s2ram.S`
 * to save/restore CPU state other than general-purpose
 * and special registers (which are handled in assembly)
 */
void z_arm_pm_s2ram_save_scb_mpu(void)
{
	z_arm_save_scb_context(&_scb_context);
	IF_ENABLED(CONFIG_MPU, (z_arm_save_mpu_context(&_mpu_context);))
}

void z_arm_pm_s2ram_restore_scb_mpu(void)
{
	z_arm_restore_scb_context(&_scb_context);
	IF_ENABLED(CONFIG_MPU, (z_arm_restore_mpu_context(&_mpu_context);))
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
	uint32_t old_control;

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
