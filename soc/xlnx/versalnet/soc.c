/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <cmsis_core.h>
#include "soc.h"

/*
 * Cortex-R52 implementation-defined CP15 register access macros.
 *
 * These registers control TCM regions, ECC, branch prediction,
 * and peripheral port access on the R52 core.
 *
 * Reference: Arm Cortex-R52 TRM (DDI 0501C), Section 4.3
 */

/* IMP_MEMPROTCTLR: p15, 1, c9, c1, 2 - Memory protection control */
#define READ_IMP_MEMPROTCTLR(val)  __asm__ volatile("mrc p15, 1, %0, c9, c1, 2" : "=r"(val))
#define WRITE_IMP_MEMPROTCTLR(val) __asm__ volatile("mcr p15, 1, %0, c9, c1, 2" : : "r"(val))

/* IMP_ATCMREGIONR: p15, 0, c9, c1, 0 - ATCM region register */
#define READ_IMP_ATCMREGIONR(val)  __asm__ volatile("mrc p15, 0, %0, c9, c1, 0" : "=r"(val))
#define WRITE_IMP_ATCMREGIONR(val) __asm__ volatile("mcr p15, 0, %0, c9, c1, 0" : : "r"(val))

/* IMP_BTCMREGIONR: p15, 0, c9, c1, 1 - BTCM region register */
#define READ_IMP_BTCMREGIONR(val)  __asm__ volatile("mrc p15, 0, %0, c9, c1, 1" : "=r"(val))
#define WRITE_IMP_BTCMREGIONR(val) __asm__ volatile("mcr p15, 0, %0, c9, c1, 1" : : "r"(val))

/* IMP_CTCMREGIONR: p15, 0, c9, c1, 2 - CTCM region register */
#define READ_IMP_CTCMREGIONR(val)  __asm__ volatile("mrc p15, 0, %0, c9, c1, 2" : "=r"(val))
#define WRITE_IMP_CTCMREGIONR(val) __asm__ volatile("mcr p15, 0, %0, c9, c1, 2" : : "r"(val))

/* IMP_PERIPHPREGIONR: p15, 0, c15, c0, 0 - Peripheral port region register */
#define READ_IMP_PERIPHPREGIONR(val)  __asm__ volatile("mrc p15, 0, %0, c15, c0, 0" : "=r"(val))
#define WRITE_IMP_PERIPHPREGIONR(val) __asm__ volatile("mcr p15, 0, %0, c15, c0, 0" : : "r"(val))

/* IMP_BPCTLR: p15, 1, c9, c1, 1 - Branch prediction control */
#define READ_IMP_BPCTLR(val)  __asm__ volatile("mrc p15, 1, %0, c9, c1, 1" : "=r"(val))
#define WRITE_IMP_BPCTLR(val) __asm__ volatile("mcr p15, 1, %0, c9, c1, 1" : : "r"(val))

/* IMP_MEMPROTCTLR bit definitions */
#define IMP_MEMPROTCTLR_TCM_ECC_EN BIT(0)

/* TCM region register bit definitions */
#define TCM_REGION_EN   BIT(0)
#define ATCM_BASE_ADDR  0x00000000U /* ATCM at 0x0, 64KB */
#define ATCM_REGION_VAL (ATCM_BASE_ADDR | TCM_REGION_EN)
#define BTCM_BASE_ADDR  0x00020000U /* BTCM at 0x20000, 64KB */
#define BTCM_REGION_VAL (BTCM_BASE_ADDR | TCM_REGION_EN)

/* IMP_PERIPHPREGIONR bit definitions */
#define PERIPHP_EL2_EN     BIT(1)
#define PERIPHP_EL1_EL0_EN BIT(0)

void soc_reset_hook(void)
{
	uint32_t reg;

	/* Clear HIVECS bit to ensure exception vectors are at 0x00000000 */
	reg = __get_SCTLR();
	reg &= ~SCTLR_V_Msk;
	__set_SCTLR(reg);

	/*
	 * Disable TCM ECC before initializing TCM memory.
	 * ECC must be disabled while TCM is being zeroed to avoid
	 * ECC errors on uninitialized memory. It can be re-enabled
	 * after TCM is fully initialized.
	 *
	 * Reference: embeddedsw boot.S - IMP_MEMPROTCTLR handling
	 */
	READ_IMP_MEMPROTCTLR(reg);
	reg &= ~IMP_MEMPROTCTLR_TCM_ECC_EN;
	WRITE_IMP_MEMPROTCTLR(reg);

	/*
	 * Enable ATCM region at 0x00000000.
	 * The ATCM is mapped at the base address for exception vectors
	 * and fast-access code/data.
	 */
	reg = ATCM_REGION_VAL;
	WRITE_IMP_ATCMREGIONR(reg);

	/*
	 * Enable BTCM region at 0x00020000.
	 * The BTCM provides additional tightly-coupled memory
	 * for data or stack usage.
	 */
	reg = BTCM_REGION_VAL;
	WRITE_IMP_BTCMREGIONR(reg);

	/*
	 * Enable low-latency peripheral port access at EL2/EL1/EL0.
	 * This allows GIC and other peripherals to be accessed
	 * through the low-latency port for reduced interrupt latency.
	 *
	 * Reference: embeddedsw boot.S - IMP_PERIPHPREGIONR handling
	 */
	READ_IMP_PERIPHPREGIONR(reg);
	reg |= (PERIPHP_EL2_EN | PERIPHP_EL1_EL0_EN);
	WRITE_IMP_PERIPHPREGIONR(reg);

	__ISB();
}

void soc_early_init_hook(void)
{
	if (IS_ENABLED(CONFIG_ICACHE)) {
		if (!(__get_SCTLR() & SCTLR_I_Msk)) {
			L1C_InvalidateICacheAll();
			__set_SCTLR(__get_SCTLR() | SCTLR_I_Msk);
			barrier_isync_fence_full();
		}
	}

	if (IS_ENABLED(CONFIG_DCACHE)) {
		if (!(__get_SCTLR() & SCTLR_C_Msk)) {
			L1C_InvalidateDCacheAll();
			__set_SCTLR(__get_SCTLR() | SCTLR_C_Msk);
			barrier_dsync_fence_full();
		}
	}
}
