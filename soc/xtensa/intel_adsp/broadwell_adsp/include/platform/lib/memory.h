/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifdef __SOF_LIB_MEMORY_H__

#ifndef __PLATFORM_LIB_MEMORY_H__
#define __PLATFORM_LIB_MEMORY_H__

#include <config.h>

#if !defined(__ASSEMBLER__) && !defined(LINKER)

struct sof;

/**
 * \brief Data shared between different cores.
 * Does nothing, since HSW doesn't support SMP.
 */
#define SHARED_DATA

void platform_init_memmap(struct sof *sof);

static inline void *platform_shared_get(void *ptr, int bytes)
{
	return ptr;
}

/**
 * \brief Function for keeping shared data synchronized.
 * It's used after usage of data shared by different cores.
 * Such data is either statically marked with SHARED_DATA
 * or dynamically allocated with SOF_MEM_FLAG_SHARED flag.
 * Does nothing, since HSW doesn't support SMP.
 */
static inline void platform_shared_commit(void *ptr, int bytes) { }

static inline void *platform_rfree_prepare(void *ptr)
{
	return ptr;
}

#endif

/* data cache line alignment */
#define PLATFORM_DCACHE_ALIGN	sizeof(void *)

/** \brief EDF task's default stack size in bytes. */
#define PLATFORM_TASK_DEFAULT_STACK_SIZE	2048

/* physical DSP addresses */

#define SHIM_SIZE	0x00001000

#define IRAM_BASE	0x00000000
#define IRAM_SIZE	0x00050000

#define DRAM0_BASE	0x00400000
#define DRAM0_VBASE	0x00400000

#define MAILBOX_SIZE	(0x00001000 - SOF_VIRTUAL_THREAD_SIZE)
#define DMA0_SIZE	0x00001000
#define DMA1_SIZE	0x00001000
#define SSP0_SIZE	0x00001000
#define SSP1_SIZE	0x00001000

#if CONFIG_BROADWELL
#define DRAM0_SIZE	0x000A0000
#define SHIM_BASE	0xFFFFB000
#define DMA0_BASE	0xFFFFE000
#define DMA1_BASE	0xFFFFF000
#define SSP0_BASE	0xFFFFC000
#define SSP1_BASE	0xFFFFD000

#else	/* HASWELL */
#define DRAM0_SIZE	0x00080000
#define SHIM_BASE	0xFFFE7000
#define DMA0_BASE	0xFFFF0000
#define DMA1_BASE	0xFFFF8000
#define SSP0_BASE	0xFFFE8000
#define SSP1_BASE	0xFFFE9000

#endif

#define UUID_ENTRY_ELF_BASE	0x1FFFA000
#define UUID_ENTRY_ELF_SIZE	0x6000

#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000

/*
 * The Heap and Stack on Haswell/Broadwell are organised like this :-
 *
 * +--------------------------------------------------------------------------+
 * | Offset              | Region         |  Size                             |
 * +---------------------+----------------+-----------------------------------+
 * | DRAM0_BASE          | RO Data        |  SOF_DATA_SIZE                    |
 * |                     | Data           |                                   |
 * |                     | BSS            |                                   |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_SYSTEM_BASE    | System Heap    |  HEAP_SYSTEM_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_RUNTIME_BASE   | Runtime Heap   |  HEAP_RUNTIME_SIZE                |
 * +---------------------+----------------+-----------------------------------+
 * | HEAP_BUFFER_BASE    | Module Buffers |  HEAP_BUFFER_SIZE                 |
 * +---------------------+----------------+-----------------------------------+
 * | MAILBOX_BASE        | Mailbox        |  MAILBOX_SIZE                     |
 * +---------------------+----------------+-----------------------------------+
 * | VIRTUAL_THREAD_BASE | Vthread Ptr    | SOF_VIRTUAL_THREAD_SIZE           |
 * +---------------------+----------------+-----------------------------------+
 * | SOF_STACK_END       | Stack          |  SOF_STACK_SIZE                   |
 * +---------------------+----------------+-----------------------------------+
 * | SOF_STACK_BASE      |                |                                   |
 * +---------------------+----------------+-----------------------------------+
 */


/* Heap section sizes for module pool */
#define HEAP_RT_COUNT8		0
#define HEAP_RT_COUNT16		192
#define HEAP_RT_COUNT32		128
#define HEAP_RT_COUNT64		64
#define HEAP_RT_COUNT128	64
#define HEAP_RT_COUNT256	64
#define HEAP_RT_COUNT512	8
#define HEAP_RT_COUNT1024	4

/* Heap section sizes for system runtime heap */
#define HEAP_SYS_RT_COUNT64	64
#define HEAP_SYS_RT_COUNT512	8
#define HEAP_SYS_RT_COUNT1024	4

/* Heap configuration */
#define SOF_DATA_SIZE			0xD000

#define HEAP_SYSTEM_BASE		(DRAM0_BASE + SOF_DATA_SIZE)
#define HEAP_SYSTEM_SIZE		0x4000

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
	(DRAM0_SIZE - HEAP_RUNTIME_SIZE - SOF_STACK_TOTAL_SIZE -\
	 HEAP_SYS_RUNTIME_SIZE - HEAP_SYSTEM_SIZE - SOF_DATA_SIZE -\
	 SOF_VIRTUAL_THREAD_SIZE - MAILBOX_SIZE)

#define HEAP_BUFFER_BLOCK_SIZE		0x100
#define HEAP_BUFFER_COUNT \
	(HEAP_BUFFER_SIZE / HEAP_BUFFER_BLOCK_SIZE)

#define PLATFORM_HEAP_SYSTEM		1 /* one per core */
#define PLATFORM_HEAP_SYSTEM_RUNTIME	1 /* one per core */
#define PLATFORM_HEAP_RUNTIME		1
#define PLATFORM_HEAP_BUFFER		1

/* Stack configuration */
#define SOF_STACK_SIZE		0x1000
#define SOF_STACK_TOTAL_SIZE	SOF_STACK_SIZE
#define SOF_STACK_BASE		(DRAM0_BASE + DRAM0_SIZE)
#define SOF_STACK_END		(SOF_STACK_BASE - SOF_STACK_TOTAL_SIZE)

/* Virtual threadptr */
#define SOF_VIRTUAL_THREAD_SIZE	0x20
#define SOF_VIRTUAL_THREAD_BASE (SOF_STACK_END - SOF_VIRTUAL_THREAD_SIZE)

#define MAILBOX_BASE			(SOF_VIRTUAL_THREAD_BASE - MAILBOX_SIZE)

/* Vector and literal sizes - not in core-isa.h */
#define SOF_MEM_VECT_LIT_SIZE		0x4
#define SOF_MEM_VECT_TEXT_SIZE		0x1c
#define SOF_MEM_VECT_SIZE		(SOF_MEM_VECT_TEXT_SIZE + \
					SOF_MEM_VECT_LIT_SIZE)

#define SOF_MEM_RESET_TEXT_SIZE	0x2e0
#define SOF_MEM_RESET_LIT_SIZE		0x120
#define SOF_MEM_VECBASE_TEXT_BASE	0x400
#define SOF_MEM_VECBASE_LIT_SIZE	0x178

#define SOF_MEM_RO_SIZE		0x8

#endif /* __PLATFORM_LIB_MEMORY_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/memory.h"

#endif /* __SOF_LIB_MEMORY_H__ */
