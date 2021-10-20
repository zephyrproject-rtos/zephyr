/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZEPHYR_SOC_INTEL_ADSP_MEM
#define _ZEPHYR_SOC_INTEL_ADSP_MEM

#include <devicetree.h>
#include <cavs-vectors.h>

#define L2_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram0)))
#define L2_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram0)))

#define LP_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram1)))
#define LP_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram1)))

/* Linker-usable RAM region */
#define RAM_BASE (L2_SRAM_BASE + CONFIG_HP_SRAM_RESERVE + VECTOR_TBL_SIZE)
#define RAM_SIZE (L2_SRAM_SIZE - CONFIG_HP_SRAM_RESERVE - VECTOR_TBL_SIZE)

/* Host shared memory windows */
#define HP_SRAM_WIN0_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN0_OFFSET)
#define HP_SRAM_WIN0_SIZE 0x2000
#define HP_SRAM_WIN3_BASE (L2_SRAM_BASE + CONFIG_ADSP_WIN3_OFFSET)
#define HP_SRAM_WIN3_SIZE 0x2000

/* IMR memory layout in the bootloader.  Sort of needlessly complex to
 * put all these sections in specific sized regions, it's all the same
 * memory and the bootloader doesn't do any mapping or protection
 * management
 */
#define IMR_BOOT_LDR_MANIFEST_BASE CONFIG_IMR_MANIFEST_ADDR
#define IMR_BOOT_LDR_DATA_BASE     CONFIG_IMR_DATA_ADDR
#define IMR_BOOT_LDR_BSS_BASE      0xb0100000

#define IMR_BOOT_LDR_TEXT_ENTRY_SIZE 0x0120
#define IMR_BOOT_LDR_LIT_SIZE        0x0100
#define IMR_BOOT_LDR_TEXT_SIZE       0x1c00
#define IMR_BOOT_LDR_DATA_SIZE       0x1000
#define BOOT_LDR_STACK_SIZE          0x4000

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V15
# define IMR_BOOT_LDR_BSS_SIZE       0x10000 /* (vestigial typo?) */
#else
# define IMR_BOOT_LDR_BSS_SIZE       0x1000
#endif

#define IMR_BOOT_LDR_TEXT_ENTRY_BASE (IMR_BOOT_LDR_MANIFEST_BASE + 0x6000)
#define IMR_BOOT_LDR_LIT_BASE (IMR_BOOT_LDR_TEXT_ENTRY_BASE + IMR_BOOT_LDR_TEXT_ENTRY_SIZE)
#define IMR_BOOT_LDR_TEXT_BASE (IMR_BOOT_LDR_LIT_BASE + IMR_BOOT_LDR_LIT_SIZE)
#define BOOT_LDR_STACK_BASE (L2_SRAM_BASE + L2_SRAM_SIZE - BOOT_LDR_STACK_SIZE)

/* These are fake section addresses used only in the linker scripts */
#define IDT_BASE		(L2_SRAM_BASE + L2_SRAM_SIZE)
#define IDT_SIZE		0x2000
#define UUID_ENTRY_ELF_BASE	0x1FFFA000
#define UUID_ENTRY_ELF_SIZE	0x6000
#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000
#define EXT_MANIFEST_ELF_BASE	(LOG_ENTRY_ELF_BASE + LOG_ENTRY_ELF_SIZE)
#define EXT_MANIFEST_ELF_SIZE	0x2000000

#endif /* _ZEPHYR_SOC_INTEL_ADSP_MEM */
