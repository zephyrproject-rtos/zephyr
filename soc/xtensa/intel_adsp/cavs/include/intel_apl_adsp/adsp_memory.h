/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_


#include <zephyr/devicetree.h>
#include <adsp-vectors.h>

#define L2_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram0)))
#define L2_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram0)))

#define LP_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram1)))
#define LP_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram1)))

#define ROM_JUMP_ADDR (LP_SRAM_BASE + 0x10)

/* Linker-usable RAM region */
#define RAM_BASE (L2_SRAM_BASE + CONFIG_HP_SRAM_RESERVE + VECTOR_TBL_SIZE)
#define RAM_SIZE (L2_SRAM_SIZE - CONFIG_HP_SRAM_RESERVE - VECTOR_TBL_SIZE)

/* Host shared memory windows */
#define HP_SRAM_WIN0_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN0_OFFSET)
#define HP_SRAM_WIN0_SIZE 0x2000
#define HP_SRAM_WIN2_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN2_OFFSET)
/* window 2 size is variable */
#define HP_SRAM_WIN2_SIZE (CONFIG_ADSP_WIN3_OFFSET - CONFIG_ADSP_WIN2_OFFSET)
#define HP_SRAM_WIN3_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN3_OFFSET)
#define HP_SRAM_WIN3_SIZE 0x2000

/* The rimage tool produces two blob addresses we need to find: one is
 * our bootloader code block which starts at its entry point, the
 * other is the "manifest" containing the HP-SRAM data to unpack,
 * which appears 24k earlier in the DMA'd file, and thus in IMR
 * memory.  There's no ability to change this offset, it's a magic
 * number from rimage we simply need to honor.
 */

#define IMR_BOOT_LDR_DATA_BASE            0xB0002000
#define IMR_BOOT_LDR_MANIFEST_BASE        0xB0004000
#define IMR_BOOT_LDR_TEXT_ENTRY_BASE (IMR_BOOT_LDR_MANIFEST_BASE + 0x6000)

#define ADSP_L1_CACHE_PREFCTL_VALUE 0

#define ADSP_L2CC_ADDR          (DT_REG_ADDR(DT_NODELABEL(l2cc)))
#define ADSP_L2PCFG_ADDR        (ADSP_L2CC_ADDR + 0x08)

#if (!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__))

#define ADSP_L2PCFG_REG (*(volatile uint32_t *)(ADSP_L2PCFG_ADDR))

#endif

#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_ */
