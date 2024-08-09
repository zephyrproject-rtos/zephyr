/*
 * Copyright 2021, 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_ADSP_MEMORY_H_
#define ZEPHYR_SOC_NXP_ADSP_MEMORY_H_

#define PLATFORM_CORE_COUNT 1

/** Id of master DSP core */
#define PLATFORM_PRIMARY_CORE_ID	0

#define IRAM_RESERVE_HEADER_SPACE	0x400

/*#define IRAM_BASE    0x3B6F8000*/
#define IRAM_BASE      0x60600000
#define IRAM_SIZE      0x800

#define IRAM_SIZE	0x800

/*#define SDRAM0_BASE	0x92400000*/
/*#define SDRAM0_SIZE	0x800000*/
#define SDRAM0_BASE    0x60601000
#define SDRAM0_SIZE	   0xFF000

/*#define SDRAM1_BASE	0x92C00000*/
/*#define SDRAM1_SIZE	0x800000*/
#define SDRAM1_BASE    0x60700000
#define SDRAM1_SIZE    0x100000

/* The reset vector address in SRAM and its size */
#define MEM_RESET_TEXT_SIZE			0x2E0
#define MEM_RESET_LIT_SIZE			0x120

/* This is the base address of all the vectors defined in IRAM */
#define XCHAL_VECBASE_RESET_PADDR_IRAM \
		(IRAM_BASE + IRAM_RESERVE_HEADER_SPACE)

#define MEM_VECBASE_LIT_SIZE			0x178

/*
 * EXCEPTIONS and VECTORS
 */
/*#define XCHAL_RESET_VECTOR0_PADDR_IRAM	0x3B6F8000*/
#define XCHAL_RESET_VECTOR0_PADDR_IRAM 0x6060000

/* Vector and literal sizes */
#define MEM_VECT_LIT_SIZE			0x4
#define MEM_VECT_TEXT_SIZE			0x1C
#define MEM_VECT_SIZE				(MEM_VECT_TEXT_SIZE +\
									 MEM_VECT_LIT_SIZE)

/* The addresses of the vectors.
 * Only the mem_error vector continues to point to its ROM address.
 */
#define XCHAL_INTLEVEL2_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x17C)

#define XCHAL_INTLEVEL3_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x19C)

#define XCHAL_INTLEVEL4_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x1BC)

#define XCHAL_INTLEVEL5_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x1DC)

#define XCHAL_KERNEL_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x1FC)

#define XCHAL_USER_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x21C)

#define XCHAL_DOUBLEEXC_VECTOR_PADDR_IRAM \
	(XCHAL_VECBASE_RESET_PADDR_IRAM + 0x23C)

/* Location for the intList section which is later used to construct the
 * Interrupt Descriptor Table (IDT). This is a bogus address as this
 * section will be stripped off in the final image.
 */
#define IDT_BASE	(IRAM_BASE + IRAM_SIZE)

/* size of the Interrupt Descriptor Table (IDT) */
#define IDT_SIZE	0x2000

/* physical DSP addresses */
/*#define IRAM_BASE	0x3B6F8000*/
#define IRAM_BASE   0x60600000
#define IRAM_SIZE	0x800

/*#define DRAM0_BASE	0x3B6E8000*/
#define DRAM0_BASE	0x70800000
#define DRAM0_SIZE		0x8000

/*#define DRAM1_BASE	0x3B6F0000*/
#define DRAM1_BASE	0x80608000
#define DRAM1_SIZE		0x8000

/*#define SDRAM0_BASE	0x92400000*/
#define SDRAM0_BASE	0x60601000
/*#define SDRAM0_SIZE	0x800000*/
#define SDRAM0_SIZE		0xFF000

/*#define SDRAM1_BASE	0x92C00000*/
#define SDRAM1_BASE     0x60700000
/*#define SDRAM1_SIZE	0x800000*/
#define SDRAM1_SIZE     0x100000

#define XSHAL_MU2_SIDEB_BYPASS_PADDR 0x30E70000
#define MU_BASE		XSHAL_MU2_SIDEB_BYPASS_PADDR

#define SDMA2_BASE	0x30E10000
#define SDMA2_SIZE	0x10000

#define SDMA3_BASE	0x30E00000
#define SDMA3_SIZE	0x10000

#define SAI_1_BASE	0x30C10000
#define SAI_1_SIZE	0x00010000

#define SAI_3_BASE	0x30C30000
#define SAI_3_SIZE	0x00010000
#define UUID_ENTRY_ELF_BASE	0x1FFFA000
#define UUID_ENTRY_ELF_SIZE	0x6000

#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000

#define EXT_MANIFEST_ELF_BASE	(LOG_ENTRY_ELF_BASE + LOG_ENTRY_ELF_SIZE)
#define EXT_MANIFEST_ELF_SIZE	0x2000000

/*
 * The Heap and Stack on i.MX8 are organized like this :-
 *
 * +--------------------------------------------------------------------------+
 * | Offset              | Region         |  Size                             |
 * +---------------------+----------------+-----------------------------------+
 * | SDRAM_BASE          | RO Data        |  SOF_DATA_SIZE                    |
 * |                     | Data           |                                   |
 * |                     | BSS            |                                   |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_SYSTEM_BASE    | System Heap    |  HEAP_SYSTEM_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_RUNTIME_BASE   | Runtime Heap   |  HEAP_RUNTIME_SIZE                |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_BUFFER_BASE    | Module Buffers |  HEAP_BUFFER_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | SOF_STACK_END       | Stack          |  SOF_STACK_SIZE                   |
 * +---------------------+----------------+-----------------------------------+
 * | SOF_STACK_BASE      |                |                                   |
 * +---------------------+----------------+-----------------------------------+
 */

#define SRAM_OUTBOX_BASE	SDRAM1_BASE
#define SRAM_OUTBOX_SIZE	0x1000
#define SRAM_OUTBOX_OFFSET	0

#define SRAM_INBOX_BASE		(SRAM_OUTBOX_BASE + SRAM_OUTBOX_SIZE)
#define SRAM_INBOX_SIZE		0x1000
#define SRAM_INBOX_OFFSET	SRAM_OUTBOX_SIZE

#define SRAM_DEBUG_BASE		(SRAM_INBOX_BASE + SRAM_INBOX_SIZE)
#define SRAM_DEBUG_SIZE		0x800
#define SRAM_DEBUG_OFFSET	(SRAM_INBOX_OFFSET + SRAM_INBOX_SIZE)

#define SRAM_EXCEPT_BASE	(SRAM_DEBUG_BASE + SRAM_DEBUG_SIZE)
#define SRAM_EXCEPT_SIZE	0x800
#define SRAM_EXCEPT_OFFSET	(SRAM_DEBUG_OFFSET + SRAM_DEBUG_SIZE)

#define SRAM_STREAM_BASE	(SRAM_EXCEPT_BASE + SRAM_EXCEPT_SIZE)
#define SRAM_STREAM_SIZE	0x1000
#define SRAM_STREAM_OFFSET	(SRAM_EXCEPT_OFFSET + SRAM_EXCEPT_SIZE)

#define SRAM_TRACE_BASE		(SRAM_STREAM_BASE + SRAM_STREAM_SIZE)
#define SRAM_TRACE_SIZE		0x1000
#define SRAM_TRACE_OFFSET	(SRAM_STREAM_OFFSET + SRAM_STREAM_SIZE)

#define SOF_MAILBOX_SIZE	(SRAM_INBOX_SIZE + SRAM_OUTBOX_SIZE \
				 + SRAM_DEBUG_SIZE + SRAM_EXCEPT_SIZE \
				 + SRAM_STREAM_SIZE + SRAM_TRACE_SIZE)

#endif /* ZEPHYR_SOC_NXP_ADSP_MEMORY_H_ */
