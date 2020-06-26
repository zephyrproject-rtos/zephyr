/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifndef __INC_MEMORY_H
#define __INC_MEMORY_H

#include <soc/mailbox.h>

#define PLATFORM_DCACHE_ALIGN	XCHAL_DCACHE_LINESIZE

/** \brief EDF task's default stack size in bytes. */
#define PLATFORM_TASK_DEFAULT_STACK_SIZE	2048

/* Vector and literal addresses and sizes - do not use core-isa.h */
#define MEM_RESET_VECT			0xff2c0000
#define MEM_VECBASE			0xff2c0400
#define MEM_INTLEVEL2_VECT		0xff2c057c
#define MEM_INTLEVEL3_VECT		0xff2c059c
#define MEM_INTLEVEL4_VECT		0xff2c05bc
#define MEM_INTLEVEL5_VECT		0xff2c05dc
#define MEM_INTLEVEL6_VECT		0xff2c05fc
#define MEM_INTLEVEL7_VECT		0xff2c061c
#define MEM_KERNEL_VECT			0xff2c063c
#define MEM_USER_VECT			0xff2c065c
#define MEM_DOUBLEEXC_VECT		0xff2c067c

#define MEM_VECT_LIT_SIZE		0x4
#define MEM_VECT_TEXT_SIZE		0x1c
#define MEM_VECT_SIZE			(MEM_VECT_TEXT_SIZE + MEM_VECT_LIT_SIZE)

#define MEM_RESET_TEXT_SIZE		0x2e0
#define MEM_RESET_LIT_SIZE		0x120
#define MEM_VECBASE_LIT_SIZE		0x178

#define MEM_RO_SIZE			0x8

/* physical DSP addresses */

#define SHIM_BASE	0xFF340000
#define SHIM_SIZE	0x00004000

#define IRAM_BASE	0xFF2C0000
#define IRAM_SIZE	0x00014000

#define DRAM0_BASE	0xFF300000
#define DRAM0_SIZE	0x00028000
#define DRAM0_VBASE	0xC0000000

#define MAILBOX_BASE	0xFF344000
#define MAILBOX_SIZE	0x00001000

#define DMA0_BASE	0xFF298000
#define DMA0_SIZE	0x00004000

#define DMA1_BASE	0xFF29C000
#define DMA1_SIZE	0x00004000

#define DMA2_BASE	0xFF294000
#define DMA2_SIZE	0x00004000

#define SSP0_BASE	0xFF2A0000
#define SSP0_SIZE	0x00001000

#define SSP1_BASE	0xFF2A1000
#define SSP1_SIZE	0x00001000

#define SSP2_BASE	0xFF2A2000
#define SSP2_SIZE	0x00001000

#define SSP3_BASE	0xFF2A4000
#define SSP3_SIZE	0x00001000

#define SSP4_BASE	0xFF2A5000
#define SSP4_SIZE	0x00001000

#define SSP5_BASE	0xFF2A6000
#define SSP5_SIZE	0x00001000

#define UUID_ENTRY_ELF_BASE	0x1FFFA000
#define UUID_ENTRY_ELF_SIZE	0x6000

#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000

/*
 * The Heap and Stack on Baytrail are organised like this :-
 *
 * +--------------------------------------------------------------------------+
 * | Offset              | Region         |  Size                             |
 * +---------------------+----------------+-----------------------------------+
 * | DRAM0_BASE          | RO Data        |  SOF_DATA_SIZE                   |
 * |                     | Data           |                                   |
 * |                     | BSS            |                                   |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_SYSTEM_BASE    | System Heap    |  HEAP_SYSTEM_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_RUNTIME_BASE   | Runtime Heap   |  HEAP_RUNTIME_SIZE                |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_BUFFER_BASE    | Module Buffers |  HEAP_BUFFER_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | STACK_END           | Stack          |  STACK_SIZE                       |
 * +---------------------+----------------+-----------------------------------+
 * | STACK_BASE          |                |                                   |
 * +---------------------+----------------+-----------------------------------+
 */


/* Heap section sizes for module pool */
#define HEAP_RT_COUNT8		0
#define HEAP_RT_COUNT16		32
#define HEAP_RT_COUNT32		32
#define HEAP_RT_COUNT64		32
#define HEAP_RT_COUNT128	32
#define HEAP_RT_COUNT256	32
#define HEAP_RT_COUNT512	2
#define HEAP_RT_COUNT1024	1

/* Heap section sizes for system runtime heap */
#define HEAP_SYS_RT_COUNT64	64
#define HEAP_SYS_RT_COUNT512	8
#define HEAP_SYS_RT_COUNT1024	4

/* Heap configuration */
#define SOF_DATA_SIZE			0x9800

#define HEAP_SYSTEM_BASE		(DRAM0_BASE + SOF_DATA_SIZE)
#define HEAP_SYSTEM_SIZE		0xA800

#define HEAP_SYSTEM_0_BASE		HEAP_SYSTEM_BASE

#define HEAP_SYS_RUNTIME_BASE	(HEAP_SYSTEM_BASE + HEAP_SYSTEM_SIZE)
#define HEAP_SYS_RUNTIME_SIZE \
	(HEAP_SYS_RT_COUNT64 * 64 + HEAP_SYS_RT_COUNT512 * 512 + \
	HEAP_SYS_RT_COUNT1024 * 1024)

#define HEAP_RUNTIME_BASE	(HEAP_SYS_RUNTIME_BASE + HEAP_SYS_RUNTIME_SIZE)
#define HEAP_RUNTIME_SIZE \
	(HEAP_RT_COUNT8 * 8 + HEAP_RT_COUNT16 * 16 + \
	HEAP_RT_COUNT32 * 32 + HEAP_RT_COUNT64 * 64 + \
	HEAP_RT_COUNT128 * 128 + HEAP_RT_COUNT256 * 256 + \
	HEAP_RT_COUNT512 * 512 + HEAP_RT_COUNT1024 * 1024)

#define HEAP_BUFFER_BASE	(HEAP_RUNTIME_BASE + HEAP_RUNTIME_SIZE)
#define HEAP_BUFFER_SIZE \
	(DRAM0_SIZE - HEAP_RUNTIME_SIZE - STACK_TOTAL_SIZE -\
	HEAP_SYS_RUNTIME_SIZE - HEAP_SYSTEM_SIZE - SOF_DATA_SIZE)

#define HEAP_BUFFER_BLOCK_SIZE		0x100
#define HEAP_BUFFER_COUNT	(HEAP_BUFFER_SIZE / HEAP_BUFFER_BLOCK_SIZE)

#define PLATFORM_HEAP_SYSTEM		1 /* one per core */
#define PLATFORM_HEAP_SYSTEM_RUNTIME	1 /* one per core */
#define PLATFORM_HEAP_RUNTIME		1
#define PLATFORM_HEAP_BUFFER		1

#define SRAM_TRACE_BASE			MAILBOX_TRACE_BASE
#define SRAM_TRACE_SIZE			MAILBOX_TRACE_SIZE

/* Stack configuration */
#define STACK_SIZE		0x1000
#define STACK_TOTAL_SIZE	STACK_SIZE
#define STACK_BASE		(DRAM0_BASE + DRAM0_SIZE)
#define STACK_END		(STACK_BASE - STACK_TOTAL_SIZE)


#endif /* __INC_MEMORY_H */
