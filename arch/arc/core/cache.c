/* cache.c - d-cache support for ARC CPUs */

/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief d-cache manipulation
 *
 * This module contains functions for manipulation of the d-cache.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <toolchain.h>
#include <cache.h>
#include <linker/linker-defs.h>
#include <arch/arc/v2/aux_regs.h>
#include <kernel_internal.h>
#include <sys/__assert.h>
#include <init.h>
#include <stdbool.h>

#if (CONFIG_CACHE_LINE_SIZE == 0) && !defined(CONFIG_CACHE_LINE_SIZE_DETECT)
#error Cannot use this implementation with a cache line size of 0
#endif

#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
#define DCACHE_LINE_SIZE sys_cache_line_size
#else
#define DCACHE_LINE_SIZE CONFIG_CACHE_LINE_SIZE
#endif

#define DC_CTRL_DC_ENABLE            0x0  /* enable d-cache */
#define DC_CTRL_DC_DISABLE           0x1  /* disable d-cache */
#define DC_CTRL_INVALID_ONLY         0x0  /* invalid d-cache only */
#define DC_CTRL_INVALID_FLUSH        0x40 /* invalid and flush d-cache */
#define DC_CTRL_ENABLE_FLUSH_LOCKED  0x80 /* locked d-cache can be flushed */
#define DC_CTRL_DISABLE_FLUSH_LOCKED 0x0  /* locked d-cache cannot be flushed */
#define DC_CTRL_FLUSH_STATUS         0x100/* flush status */
#define DC_CTRL_DIRECT_ACCESS        0x0  /* direct access mode  */
#define DC_CTRL_INDIRECT_ACCESS      0x20 /* indirect access mode */
#define DC_CTRL_OP_SUCCEEDED         0x4  /* d-cache operation succeeded */


static bool dcache_available(void)
{
	unsigned long val = z_arc_v2_aux_reg_read(_ARC_V2_D_CACHE_BUILD);

	val &= 0xff; /* extract version */
	return (val == 0) ? false : true;
}

static void dcache_dc_ctrl(u32_t dcache_en_mask)
{
	if (dcache_available()) {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, dcache_en_mask);
	}
}

static void dcache_enable(void)
{
	dcache_dc_ctrl(DC_CTRL_DC_ENABLE);
}


/**
 *
 * @brief Flush multiple d-cache lines to memory
 *
 * No alignment is required for either <start_addr> or <size>, but since
 * dcache_flush_mlines() iterates on the d-cache lines, a cache line
 * alignment for both is optimal.
 *
 * The d-cache line size is specified either via the CONFIG_CACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param start_addr the pointer to start the multi-line flush
 * @param size the number of bytes that are to be flushed
 *
 * @return N/A
 */
static void dcache_flush_mlines(u32_t start_addr, u32_t size)
{
	u32_t end_addr;
	unsigned int key;

	if (!dcache_available() || (size == 0U)) {
		return;
	}

	end_addr = start_addr + size - 1;
	start_addr &= (u32_t)(~(DCACHE_LINE_SIZE - 1));

	key = irq_lock(); /* --enter critical section-- */

	do {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_FLDL, start_addr);
		__asm__ volatile("nop_s");
		__asm__ volatile("nop_s");
		__asm__ volatile("nop_s");
		/* wait for flush completion */
		do {
			if ((z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) &
			     DC_CTRL_FLUSH_STATUS) == 0) {
				break;
			}
		} while (1);
		start_addr += DCACHE_LINE_SIZE;
	} while (start_addr <= end_addr);

	irq_unlock(key); /* --exit critical section-- */

}


/**
 *
 * @brief Flush d-cache lines to main memory
 *
 * No alignment is required for either <virt> or <size>, but since
 * sys_cache_flush() iterates on the d-cache lines, a d-cache line alignment for
 * both is optimal.
 *
 * The d-cache line size is specified either via the CONFIG_CACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param start_addr the pointer to start the multi-line flush
 * @param size the number of bytes that are to be flushed
 *
 * @return N/A
 */

void sys_cache_flush(vaddr_t start_addr, size_t size)
{
	dcache_flush_mlines((u32_t)start_addr, (u32_t)size);
}


#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
size_t sys_cache_line_size;
static void init_dcache_line_size(void)
{
	u32_t val;

	val = z_arc_v2_aux_reg_read(_ARC_V2_D_CACHE_BUILD);
	__ASSERT((val&0xff) != 0U, "d-cache is not present");
	val = ((val>>16) & 0xf) + 1;
	val *= 16U;
	sys_cache_line_size = (size_t) val;
}
#endif

static int init_dcache(struct device *unused)
{
	ARG_UNUSED(unused);

	dcache_enable();

#if defined(CONFIG_CACHE_LINE_SIZE_DETECT)
	init_dcache_line_size();
#endif

	return 0;
}

SYS_INIT(init_dcache, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
