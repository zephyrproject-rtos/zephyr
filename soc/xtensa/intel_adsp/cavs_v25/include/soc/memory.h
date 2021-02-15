/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_MEMORY_H
#define __INC_MEMORY_H

#include <cavs/cpu.h>

/* L2 HP SRAM */
#define HP_RAM_RESERVE_HEADER_SPACE	0x00010000

#define L2_SRAM_BASE			(DT_REG_ADDR(DT_NODELABEL(sram0)))
#define L2_SRAM_SIZE			(DT_REG_SIZE(DT_NODELABEL(sram0)))

#define SRAM_BASE (L2_SRAM_BASE)
#define SRAM_SIZE (L2_SRAM_SIZE)

/* The reset vector address in SRAM and its size */
#define XCHAL_RESET_VECTOR0_PADDR_SRAM		SRAM_BASE
#define MEM_RESET_TEXT_SIZE			0x268
#define MEM_RESET_LIT_SIZE			0x8

/* This is the base address of all the vectors defined in SRAM */
#define XCHAL_VECBASE_RESET_PADDR_SRAM \
	(SRAM_BASE + HP_RAM_RESERVE_HEADER_SPACE)

#define MEM_VECBASE_LIT_SIZE			0x178

/* The addresses of the vectors in SRAM.
 * Only the memerror vector continues to point to its ROM address.
 */
#define XCHAL_INTLEVEL2_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x180)

#define XCHAL_INTLEVEL3_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x1C0)

#define XCHAL_INTLEVEL4_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x200)

#define XCHAL_INTLEVEL5_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x240)

#define XCHAL_INTLEVEL6_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x280)

#define XCHAL_INTLEVEL7_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x2C0)

#define XCHAL_KERNEL_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x300)

#define XCHAL_USER_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x340)

#define XCHAL_DOUBLEEXC_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x3C0)

#define VECTOR_TBL_SIZE				0x0400

/* Vector and literal sizes */
#define MEM_VECT_LIT_SIZE			0x8
#define MEM_VECT_TEXT_SIZE			0x38
#define MEM_VECT_SIZE				(MEM_VECT_TEXT_SIZE +\
						MEM_VECT_LIT_SIZE)

#define MEM_ERROR_TEXT_SIZE			0x180
#define MEM_ERROR_LIT_SIZE			0x8

/* text and data share the same L2 HP SRAM.
 * So, they lie next to each other.
 */
#define RAM_BASE \
	(SRAM_BASE + HP_RAM_RESERVE_HEADER_SPACE + VECTOR_TBL_SIZE)

#define RAM_SIZE \
	(SRAM_SIZE - HP_RAM_RESERVE_HEADER_SPACE - VECTOR_TBL_SIZE)

#define LPSRAM_MASK(x)		0x00000003

/* Location for the intList section which is later used to construct the
 * Interrupt Descriptor Table (IDT). This is a bogus address as this
 * section will be stripped off in the final image.
 */
#define IDT_BASE				(RAM_BASE + RAM_SIZE)

/* size of the Interrupt Descriptor Table (IDT) */
#define IDT_SIZE				0x2000

#define SRAM_BANK_SIZE	(64 * 1024)

/* bootloader */

#define HP_SRAM_BASE	0xbe000000
#define HP_SRAM_SIZE	(30 * SRAM_BANK_SIZE)
#define SOF_STACK_BASE	(HP_SRAM_BASE + HP_SRAM_SIZE)

/* boot loader in IMR */
#define IMR_BOOT_LDR_TEXT_ENTRY_BASE	0xB0038000
#define IMR_BOOT_LDR_TEXT_ENTRY_SIZE	0x120

#define IMR_BOOT_LDR_LIT_BASE \
	(IMR_BOOT_LDR_TEXT_ENTRY_BASE + IMR_BOOT_LDR_TEXT_ENTRY_SIZE)
#define IMR_BOOT_LDR_LIT_SIZE		0x100

#define IMR_BOOT_LDR_TEXT_BASE \
	(IMR_BOOT_LDR_LIT_BASE + IMR_BOOT_LDR_LIT_SIZE)
#define IMR_BOOT_LDR_TEXT_SIZE	0x1c00

#define IMR_BOOT_LDR_DATA_BASE	0xb0039000
#define IMR_BOOT_LDR_DATA_SIZE	0x1000

#define IMR_BOOT_LDR_BSS_BASE	0xb0100000
#define IMR_BOOT_LDR_BSS_SIZE	0x1000

#define BOOT_LDR_STACK_BASE		(HP_SRAM_BASE + HP_SRAM_SIZE - \
					BOOT_LDR_STACK_SIZE)
#define BOOT_LDR_STACK_SIZE		(4 * 0x1000)

/* Manifest base address in IMR - used by boot loader copy procedure. */
#define IMR_BOOT_LDR_MANIFEST_BASE	0xB0032000

/* Manifest size (seems unused). */
#define IMR_BOOT_LDR_MANIFEST_SIZE	0x6000


#define UUID_ENTRY_ELF_BASE	0x1FFFA000
#define UUID_ENTRY_ELF_SIZE	0x6000

#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000

#define EXT_MANIFEST_ELF_BASE	(LOG_ENTRY_ELF_BASE + LOG_ENTRY_ELF_SIZE)
#define EXT_MANIFEST_ELF_SIZE	0x2000000

#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define SRAM_ALIAS_OFFSET	0x20000000


#define uncache_to_cache(address) \
	((__typeof__((address)))((uint32_t)((address)) + SRAM_ALIAS_OFFSET))
#define cache_to_uncache(address) \
	((__typeof__((address)))((uint32_t)((address)) - SRAM_ALIAS_OFFSET))
#define is_uncached(address) \
	(((uint32_t)(address) & SRAM_ALIAS_MASK) == SRAM_ALIAS_BASE)

/* shim */
#define SHIM_BASE		0x00071F00
#define SHIM_SIZE		0x00000100

/* IRQ controller */
#define IRQ_BASE		0x00078800
#define IRQ_SIZE		0x00000200

/* IPC to the host */
#define IPC_HOST_BASE		0x00071E00
#define IPC_HOST_SIZE		0x00000020

/* intra DSP  IPC */
#define IPC_DSP_SIZE		0x00000080
#define IPC_DSP_BASE(x)		(0x00001200 + x * IPC_DSP_SIZE)

/* SRAM window for HOST */
#define HOST_WIN_SIZE		0x00000008
#define HOST_WIN_BASE(x)	(0x00071A00 + x * HOST_WIN_SIZE)

/* HP SRAM windows */

/* window 0 */
#define SRAM_SW_REG_BASE	(HP_SRAM_BASE + 0x4000)
#define SRAM_SW_REG_SIZE	0x1000

#define SRAM_OUTBOX_BASE	(SRAM_SW_REG_BASE + SRAM_SW_REG_SIZE)
#define SRAM_OUTBOX_SIZE	0x1000

#define HP_SRAM_WIN0_BASE	SRAM_SW_REG_BASE
#define HP_SRAM_WIN0_SIZE	(SRAM_SW_REG_SIZE + SRAM_OUTBOX_SIZE)

/* window 1 */
#define SRAM_INBOX_BASE		(SRAM_OUTBOX_BASE + SRAM_OUTBOX_SIZE)
#define SRAM_INBOX_SIZE		0x2000

/* window 2 */
#define SRAM_DEBUG_BASE		(SRAM_INBOX_BASE + SRAM_INBOX_SIZE)
#define SRAM_DEBUG_SIZE		0x800

#define SRAM_EXCEPT_BASE	(SRAM_DEBUG_BASE + SRAM_DEBUG_SIZE)
#define SRAM_EXCEPT_SIZE	0x800

#define SRAM_STREAM_BASE	(SRAM_EXCEPT_BASE + SRAM_EXCEPT_SIZE)
#define SRAM_STREAM_SIZE	0x1000

/* window 3 */
#define SRAM_TRACE_BASE		(SRAM_STREAM_BASE + SRAM_STREAM_SIZE)
#define SRAM_TRACE_SIZE		0x2000

#define HP_SRAM_WIN3_BASE	SRAM_TRACE_BASE
#define HP_SRAM_WIN3_SIZE	SRAM_TRACE_SIZE

#define SOF_TEXT_START	0xbe010400

#define SOF_TEXT_BASE	SOF_TEXT_START

/* SRAM window 0 FW "registers" */
#define SRAM_REG_FW_TRACEP_SLAVE_CORE_BASE      0x14
#define SRAM_REG_FW_END \
	(SRAM_REG_FW_TRACEP_SLAVE_CORE_BASE + \
	 (PLATFORM_CORE_COUNT - 1) * 0x4)

/* Host page size */
#define HOST_PAGE_SIZE		4096

/* low power RAM where DMA buffers are typically placed, used by linker.ld */
#define LP_SRAM_BASE (DT_REG_ADDR(DT_NODELABEL(sram1)))
#define LP_SRAM_SIZE (DT_REG_SIZE(DT_NODELABEL(sram1)))

/* alternate reset vector */
#define LP_SRAM_ALT_RESET_VEC_BASE	LP_SRAM_BASE
#define LP_SRAM_ALT_RESET_VEC_SIZE	0x180

#define LP_SRAM_ALT_RESET_INT_VEC_BASE	(LP_SRAM_ALT_RESET_VEC_BASE + \
					LP_SRAM_ALT_RESET_VEC_SIZE)
#define LP_SRAM_ALT_RESET_INT_VEC_SIZE	0x10

#define LP_SRAM_CODE_BASE		(LP_SRAM_ALT_RESET_INT_VEC_BASE + \
					LP_SRAM_ALT_RESET_INT_VEC_SIZE)
#define LP_SRAM_CODE_SIZE		0x220

#define LP_SRAM_START			(LP_SRAM_CODE_BASE + LP_SRAM_CODE_SIZE)

#endif /* __INC_MEMORY_H */
