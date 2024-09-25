/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_
#define ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_


#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <adsp-vectors.h>
#include <mem_window.h>

#define L2_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram0)))
#define L2_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram0)))

#define L2_VIRTUAL_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram0virtual)))
#define L2_VIRTUAL_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram0virtual)))

#define LP_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram1)))
#define LP_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram1)))

#define ROM_JUMP_ADDR (LP_SRAM_BASE + 0x10)

/* Linker-usable RAM region */
#define RAM_BASE (L2_SRAM_BASE + CONFIG_HP_SRAM_RESERVE + VECTOR_TBL_SIZE)
#define RAM_SIZE (L2_SRAM_SIZE - CONFIG_HP_SRAM_RESERVE - VECTOR_TBL_SIZE)


/* L3 region (IMR), located in host memory */

#define L3_MEM_BASE_ADDR (DT_REG_ADDR(DT_NODELABEL(imr1)))
#define L3_MEM_SIZE (DT_REG_SIZE(DT_NODELABEL(imr1)))
#define L3_MEM_PAGE_SIZE (DT_PROP(DT_NODELABEL(imr1), block_size))

/* The rimage tool produces two blob addresses we need to find: one is
 * our bootloader code block which starts at its entry point, the
 * other is the "manifest" containing the HP-SRAM data to unpack,
 * which appears 24k earlier in the DMA'd file, and thus in IMR
 * memory.  There's no ability to change this offset, it's a magic
 * number from rimage we simply need to honor.
 */
#define IMR_BOOT_LDR_MANIFEST_OFFSET	0x42000
#define IMR_BOOT_LDR_MANIFEST_SIZE	0x6000
#define IMR_BOOT_LDR_MANIFEST_BASE	(L3_MEM_BASE_ADDR + IMR_BOOT_LDR_MANIFEST_OFFSET)

#define IMR_BOOT_LDR_TEXT_ENTRY_SIZE	0x180
#define IMR_BOOT_LDR_TEXT_ENTRY_BASE	(IMR_BOOT_LDR_MANIFEST_BASE + IMR_BOOT_LDR_MANIFEST_SIZE)

#define IMR_BOOT_LDR_LIT_SIZE		0x40
#define IMR_BOOT_LDR_LIT_BASE		(IMR_BOOT_LDR_TEXT_ENTRY_BASE + \
					IMR_BOOT_LDR_TEXT_ENTRY_SIZE)

#define IMR_BOOT_LDR_TEXT_SIZE		0x1C00
#define IMR_BOOT_LDR_TEXT_BASE		(IMR_BOOT_LDR_LIT_BASE + IMR_BOOT_LDR_LIT_SIZE)

#define IMR_BOOT_LDR_DATA_OFFSET	0x49000
#define IMR_BOOT_LDR_DATA_BASE		(L3_MEM_BASE_ADDR + IMR_BOOT_LDR_DATA_OFFSET)
#define IMR_BOOT_LDR_DATA_SIZE		0xA8000

#define IMR_BOOT_LDR_BSS_OFFSET		0x110000
#define IMR_BOOT_LDR_BSS_BASE		(L3_MEM_BASE_ADDR + IMR_BOOT_LDR_BSS_OFFSET)
#define IMR_BOOT_LDR_BSS_SIZE		0x10000

/* stack to be used at boot, when RAM is not yet powered up */
#define IMR_BOOT_LDR_STACK_BASE		(IMR_BOOT_LDR_BSS_BASE + IMR_BOOT_LDR_BSS_SIZE)
#define IMR_BOOT_LDR_STACK_SIZE		0x1000

/* position of L3 heap, size of L3 heap - till end of the L3 memory */
#define IMR_L3_HEAP_BASE		(IMR_BOOT_LDR_STACK_BASE + IMR_BOOT_LDR_STACK_SIZE)
#define IMR_L3_HEAP_SIZE		(L3_MEM_SIZE - \
					(IMR_L3_HEAP_BASE - L3_MEM_BASE_ADDR))

#define ADSP_L1_CACHE_PREFCTL_VALUE 0x1038

/* L1 init */
#define ADSP_L1CC_ADDR                       (DT_REG_ADDR(DT_NODELABEL(l1ccap)))
#define ADSP_CxL1CCAP_ADDR                   (DT_REG_ADDR(DT_NODELABEL(l1ccap)))
#define ADSP_CxL1CCFG_ADDR                   (DT_REG_ADDR(DT_NODELABEL(l1ccfg)))
#define ADSP_CxL1PCFG_ADDR                   (DT_REG_ADDR(DT_NODELABEL(l1pcfg)))

#if (!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__))

#define ADSP_CxL1CCAP_REG   (*(volatile uint32_t *)(ADSP_CxL1CCAP_ADDR))
#define ADSP_CxL1CCFG_REG   (*(volatile uint32_t *)(ADSP_CxL1CCFG_ADDR))
#define ADSP_CxL1PCFG_REG   (*(volatile uint32_t *)(ADSP_CxL1PCFG_ADDR))

#endif  /* (!defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)) */

/* The number of set associative cache way supported on L1 Data Cache */
#define ADSP_CxL1CCAP_DCMWC ((ADSP_CxL1CCAP_REG >> 16) & 7)
/* The number of set associative cache way supported on L1 Instruction Cache */
#define ADSP_CxL1CCAP_ICMWC ((ADSP_CxL1CCAP_REG >> 20) & 7)

#ifndef _LINKER
/* L2 Local Memory Management */

/* These registers are for the L2 memory control and status. */
#define DFL2MM_REG 0x71d00

struct ace_l2mm {
	uint32_t l2mcap;
	uint32_t l2mpat;
	uint32_t l2mecap;
	uint32_t l2mecs;
	uint32_t l2hsbpmptr;
	uint32_t l2usbpmptr;
	uint32_t l2usbmrpptr;
	uint32_t l2ucmrpptr;
	uint32_t l2ucmrpdptr;
};

#define ACE_L2MM ((volatile struct ace_l2mm *)DFL2MM_REG)

/* DfL2MCAP */
struct ace_l2mcap {
	uint32_t l2hss  : 8;
	uint32_t l2uss  : 4;
	uint32_t l2hsbs : 4;
	uint32_t l2hs2s : 8;
	uint32_t l2usbs : 5;
	uint32_t l2se   : 1;
	uint32_t el2se  : 1;
	uint32_t rsvd32 : 1;
};

#define ACE_L2MCAP ((volatile struct ace_l2mcap *)DFL2MM_REG)

static ALWAYS_INLINE uint32_t ace_hpsram_get_bank_count(void)
{
	return ACE_L2MCAP->l2hss;
}

static ALWAYS_INLINE uint32_t ace_lpsram_get_bank_count(void)
{
	return ACE_L2MCAP->l2uss;
}

struct ace_hpsram_regs {
	/** @brief power gating control */
	uint8_t HSxPGCTL;
	/** @brief retention mode control */
	uint8_t HSxRMCTL;
	uint8_t reserved[2];
	/** @brief power gating status */
	uint8_t HSxPGISTS;
	uint8_t reserved1[3];
};

struct ace_lpsram_regs {
	/** @brief power gating control */
	uint8_t USxPGCTL;
	/** @brief retention mode control */
	uint8_t USxRMCTL;
	uint8_t reserved[2];
	/** @brief power gating status */
	uint8_t USxPGISTS;
	uint8_t reserved1[3];
};
#endif

/* These registers are for the L2 HP SRAM bank power management control and status.*/
#define L2_HSBPM_BASE (DT_REG_ADDR(DT_NODELABEL(hsbpm)))
#define L2_HSBPM_SIZE (DT_REG_SIZE(DT_NODELABEL(hsbpm)))

#define HPSRAM_REGS(block_idx)		((volatile struct ace_hpsram_regs *const) \
	(L2_HSBPM_BASE + L2_HSBPM_SIZE * (block_idx)))

/* These registers are for the L2 LP SRAM bank power management control and status.*/
#define L2_LSBPM_BASE (DT_REG_ADDR(DT_NODELABEL(lsbpm)))
#define L2_LSBPM_SIZE (DT_REG_SIZE(DT_NODELABEL(lsbpm)))

#define LPSRAM_REGS(block_idx)         ((volatile struct ace_lpsram_regs *const) \
	(L2_LSBPM_BASE + L2_LSBPM_SIZE * (block_idx)))

#endif /* ZEPHYR_SOC_INTEL_ADSP_MEMORY_H_ */
