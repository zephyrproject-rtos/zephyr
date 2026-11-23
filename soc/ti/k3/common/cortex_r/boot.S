/*
 * Copyright (c) 2024-2026 Texas Instruments Incorporated
 *	Andrew Davis <afd@ti.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/cortex_a_r/mpu.h>

#define FULL_ACCESS 0x3
#define FULL_ACCESS_Msk ((FULL_ACCESS << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)

#define ARM_R5_MPU_REGION_SIZE_2GB (0x1eU)
#define REGION_SIZE_2G ((ARM_R5_MPU_REGION_SIZE_2GB << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk)

_ASM_FILE_PROLOGUE

/**
 * DRAM is XN by default. Setup a simple MPU region to allow enable
 * execute before jumping to the real Zephyr start code in DRAM
 */
GTEXT(__boot_spring)
SECTION_SUBSEC_FUNC(boot_section,_start,__boot_spring)

    mov r0, #0 // MPU region 0
    mcr p15, 0, r0, c6, c2, 0

    mov r0, #0x80000000 // Base of DRAM
    mcr p15, 0, r0, c6, c1, 0

    mov r0, #MPU_RASR_TEX_Msk
    orr r0, #FULL_ACCESS_Msk
    mcr p15, 0, r0, c6, c1, 4

    mov r0, #REGION_SIZE_2G // All of DRAM
    orr r0, #MPU_RASR_ENABLE_Msk // Enable region
    mcr p15, 0, r0, c6, c1, 2

    mrc p15, 0, r0, c1, c0, 0 // Read SCTLR
    orr r0, #(1 << 17) // Background MPU fallback enable
    orr r0, #SCTLR_MPU_ENABLE // Enable MPU
    dsb
    mcr p15, 0, r0, c1, c0, 0 // Write SCTLR
    isb

    b __start
