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

#define SRAM_BANK_SIZE		(64 * 1024)
#define EBB_SEG_SIZE	32
#define HPSRAM_EBB_COUNT (DT_REG_SIZE(DT_NODELABEL(sram0)) / SRAM_BANK_SIZE)

/* div_round_up, but zephyr version is not defined for assembler where this is also used */
#define HPSRAM_SEGMENTS (HPSRAM_EBB_COUNT + EBB_SEG_SIZE - 1) / EBB_SEG_SIZE
#define HPSRAM_MEMMASK(idx) ((1ULL << (HPSRAM_EBB_COUNT - EBB_SEG_SIZE * idx)) - 1)

/* L3 region (IMR), located in host memory */
#define L3_MEM_BASE_ADDR (DT_REG_ADDR(DT_NODELABEL(imr1)))

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

#ifndef _ASMLANGUAGE
/* L2 Local Memory Management */

struct cavs_hpsram_regs {
	/** @brief power gating control */
	uint32_t HSxPGCTL;
	/** @brief retention mode control */
	uint32_t HSxRMCTL;
	/** @brief power gating status */
	uint32_t HSxPGISTS;
};

struct cavs_lpsram_regs {
	/** @brief power gating control */
	uint32_t USxPGCTL;
	/** @brief retention mode control */
	uint32_t USxRMCTL;
	/** @brief power gating status */
	uint32_t USxPGISTS;
};
#endif /* _ASMLANGUAGE */

/* These registers are for the L2 HP SRAM bank power management control and status.*/
#define L2_HSBPM_BASE (DT_REG_ADDR(DT_NODELABEL(hsbpm)))
#define L2_HSBPM_SIZE (DT_REG_SIZE(DT_NODELABEL(hsbpm)))

#define HPSRAM_REGS(block_idx)		((volatile struct cavs_hpsram_regs *const) \
	(L2_HSBPM_BASE + L2_HSBPM_SIZE * (block_idx)))

/* These registers are for the L2 LP SRAM bank power management control and status.*/
#define L2_LSBPM_BASE (DT_REG_ADDR(DT_NODELABEL(lsbpm)))
#define L2_LSBPM_SIZE (DT_REG_SIZE(DT_NODELABEL(lsbpm)))

#define LPSRAM_REGS(block_idx)		((volatile struct cavs_lpsram_regs *const) \
	(L2_LSBPM_BASE + L2_LSBPM_SIZE * (block_idx)))

#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_ */
