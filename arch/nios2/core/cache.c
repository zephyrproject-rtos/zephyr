/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>


/**
 * Flush the entire instruction cache and pipeline.
 *
 * You will need to call this function if the application writes new program
 * text to memory, such as a boot copier or runtime synthesis of code.  If the
 * new text was written with instructions that do not bypass cache memories,
 * this should immediately be followed by an invocation of
 * z_nios2_dcache_flush_all() so that cached instruction data is committed to
 * RAM.
 *
 * See Chapter 9 of the Nios II Gen 2 Software Developer's Handbook for more
 * information on cache considerations.
 */
#if ALT_CPU_ICACHE_SIZE > 0
void z_nios2_icache_flush_all(void)
{
	uint32_t i;

	for (i = 0U; i < ALT_CPU_ICACHE_SIZE; i += ALT_CPU_ICACHE_LINE_SIZE) {
		z_nios2_icache_flush(i);
	}

	/* Get rid of any stale instructions in the pipeline */
	z_nios2_pipeline_flush();
}
#endif

/**
 * Flush the entire data cache.
 *
 * This will be typically needed after writing new program text to memory
 * after flushing the instruction cache.
 *
 * The Nios II does not support hardware cache coherency for multi-master
 * or multi-processor systems and software coherency must be implemented
 * when communicating with shared memory. If support for this is introduced
 * in Zephyr additional APIs for flushing ranges of the data cache will need
 * to be implemented.
 *
 * See Chapter 9 of the Nios II Gen 2 Software Developer's Handbook for more
 * information on cache considerations.
 */
#if ALT_CPU_DCACHE_SIZE > 0
void z_nios2_dcache_flush_all(void)
{
	uint32_t i;

	for (i = 0U; i < ALT_CPU_DCACHE_SIZE; i += ALT_CPU_DCACHE_LINE_SIZE) {
		z_nios2_dcache_flush(i);
	}
}
#endif

/*
 * z_nios2_dcache_flush_no_writeback() is called to flush the data cache for a
 * memory region of length "len" bytes, starting at address "start".
 *
 * Any dirty lines in the data cache are NOT written back to memory.
 * Make sure you really want this behavior.  If you aren't 100% sure,
 * use the z_nios2_dcache_flush() routine instead.
 */
#if ALT_CPU_DCACHE_SIZE > 0
void z_nios2_dcache_flush_no_writeback(void *start, uint32_t len)
{
	uint8_t *i;
	uint8_t *end = ((char *) start) + len;

	for (i = start; i < end; i += ALT_CPU_DCACHE_LINE_SIZE) {
		__asm__ volatile ("initda (%0)" :: "r" (i));
	}

	/*
	 * For an unaligned flush request, we've got one more line left.
	 * Note that this is dependent on ALT_CPU_DCACHE_LINE_SIZE to be a
	 * multiple of 2 (which it always is).
	 */

	if (((uint32_t) start) & (ALT_CPU_DCACHE_LINE_SIZE - 1)) {
		__asm__ volatile ("initda (%0)" :: "r" (i));
	}
}
#endif
