/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_


#include <zephyr/devicetree.h>
#include <cavs-vectors.h>

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
#define HP_SRAM_WIN3_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN3_OFFSET)
#define HP_SRAM_WIN3_SIZE 0x2000

/* The rimage tool produces two blob addresses we need to find: one is
 * our bootloader code block which starts at its entry point, the
 * other is the "manifest" containing the HP-SRAM data to unpack,
 * which appears 24k earlier in the DMA'd file, and thus in IMR
 * memory.  There's no ability to change this offset, it's a magic
 * number from rimage we simply need to honor.
 */

#define IMR_BOOT_LDR_DATA_BASE             0xB0039000
#define IMR_BOOT_LDR_MANIFEST_BASE 0xB0032000
#define IMR_BOOT_LDR_TEXT_ENTRY_BASE (IMR_BOOT_LDR_MANIFEST_BASE + 0x6000)

#define ADSP_L1_CACHE_PREFCTL_VALUE 0x1038

/* L1 init */
#define ADSP_L1CC_ADDR                       (0x9F080080)
#define ADSP_CxL1CCAP_ADDR                   (ADSP_L1CC_ADDR + 0x0000)
#define ADSP_CxL1CCFG_ADDR                   (ADSP_L1CC_ADDR + 0x0004)
#define ADSP_CxL1PCFG_ADDR                   (ADSP_L1CC_ADDR + 0x0008)

#if (!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__))

#define ADSP_CxL1CCAP_REG   (*(volatile uint32_t *)(ADSP_CxL1CCAP_ADDR))
#define ADSP_CxL1CCFG_REG   (*(volatile uint32_t *)(ADSP_CxL1CCFG_ADDR))
#define ADSP_CxL1PCFG_REG   (*(volatile uint32_t *)(ADSP_CxL1PCFG_ADDR))

#endif  /* (!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)) */

/* The number of set associative cache way supported on L1 Data Cache */
#define ADSP_CxL1CCAP_DCMWC ((ADSP_CxL1CCAP_REG >> 16) & 7)
/* The number of set associative cache way supported on L1 Instruction Cache */
#define ADSP_CxL1CCAP_ICMWC ((ADSP_CxL1CCAP_REG >> 20) & 7)


#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_ */
