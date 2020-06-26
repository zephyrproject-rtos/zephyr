/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Bartosz Kokoszko <bartoszx.kokoszko@linux.intel.com>
 */

#ifndef __CAVS_MEMORY_H__
#define __CAVS_MEMORY_H__

#include <adsp/cache.h>
#if !defined(__ASSEMBLER__) && !defined(LINKER)
#include <stdint.h>
#include <cavs/cpu.h>
#endif

#define DCACHE_LINE_SIZE	XCHAL_DCACHE_LINESIZE

/* data cache line alignment */
#define PLATFORM_DCACHE_ALIGN	DCACHE_LINE_SIZE

#define SRAM_BANK_SIZE			(64 * 1024)

#define EBB_BANKS_IN_SEGMENT		32

#define EBB_SEGMENT_SIZE		EBB_BANKS_IN_SEGMENT

#if CONFIG_LP_MEMORY_BANKS
#define PLATFORM_LPSRAM_EBB_COUNT	CONFIG_LP_MEMORY_BANKS
#else
#define PLATFORM_LPSRAM_EBB_COUNT	0
#endif

#define PLATFORM_HPSRAM_EBB_COUNT	CONFIG_HP_MEMORY_BANKS

#define MAX_MEMORY_SEGMENTS		PLATFORM_HPSRAM_SEGMENTS

#if CONFIG_LP_MEMORY_BANKS
#define LP_SRAM_SIZE \
	(CONFIG_LP_MEMORY_BANKS * SRAM_BANK_SIZE)
#else
#define LP_SRAM_SIZE 0
#endif

#define HP_SRAM_SIZE \
	(CONFIG_HP_MEMORY_BANKS * SRAM_BANK_SIZE)

#define PLATFORM_HPSRAM_SEGMENTS	((PLATFORM_HPSRAM_EBB_COUNT \
	+ EBB_BANKS_IN_SEGMENT - 1) / EBB_BANKS_IN_SEGMENT)

#if defined(__ASSEMBLER__)
#define LPSRAM_MASK(ignored)	((1 << PLATFORM_LPSRAM_EBB_COUNT) - 1)

#define HPSRAM_MASK(seg_idx)	((1 << (PLATFORM_HPSRAM_EBB_COUNT \
	- EBB_BANKS_IN_SEGMENT * seg_idx)) - 1)
#else
#define LPSRAM_MASK(ignored)	((1ULL << PLATFORM_LPSRAM_EBB_COUNT) - 1)

#define HPSRAM_MASK(seg_idx)	((1ULL << (PLATFORM_HPSRAM_EBB_COUNT \
	- EBB_BANKS_IN_SEGMENT * seg_idx)) - 1)
#endif

#define LPSRAM_SIZE (PLATFORM_LPSRAM_EBB_COUNT * SRAM_BANK_SIZE)

#define HEAP_BUF_ALIGNMENT		PLATFORM_DCACHE_ALIGN

/** \brief EDF task's default stack size in bytes. */
#define PLATFORM_TASK_DEFAULT_STACK_SIZE	0x1000

#if !defined(__ASSEMBLER__) && !defined(LINKER)

/**
 * \brief Data shared between different cores.
 * Placed into dedicated section, which should be accessed through
 * uncached memory region. SMP platforms without uncache can simply
 * align to cache line size instead.
 */
#if PLATFORM_CORE_COUNT > 1 && !defined(UNIT_TEST)
#define SHARED_DATA	__section(".shared_data")
#else
#define SHARED_DATA
#endif

#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define SRAM_ALIAS_OFFSET	0x20000000

#endif

/**
 * FIXME check that correct core count is used
 */
#include <cavs/cpu.h>
/* SOF Core S configuration */
#define SRAM_BANK_SIZE		(64 * 1024)

/* low power sequencer */
#define LPS_RESTORE_VECTOR_OFFSET 0x1000
#define LPS_RESTORE_VECTOR_SIZE 0x800
#define LPS_RESTORE_VECTOR_ADDR (LP_SRAM_BASE + LPS_RESTORE_VECTOR_OFFSET)

#endif /* __CAVS_MEMORY_H__ */
