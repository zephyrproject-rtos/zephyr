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

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
size_t sys_cache_line_size;
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

static void dcache_dc_ctrl(uint32_t dcache_en_mask)
{
	if (dcache_available()) {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, dcache_en_mask);
	}
}

void arch_dcache_enable(void)
{
	dcache_dc_ctrl(DC_CTRL_DC_ENABLE);
}

static void arch_dcache_flush(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_dcache_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	unsigned int key;

	if (!dcache_available() || (size == 0U) || line_size == 0U) {
		return;
	}

	end_addr = start_addr + size;

	start_addr = ROUND_DOWN(start_addr, line_size);

	key = arch_irq_lock(); /* --enter critical section-- */

	do {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_FLDL, start_addr);
		__builtin_arc_nop();
		__builtin_arc_nop();
		__builtin_arc_nop();
		/* wait for flush completion */
		do {
			if ((z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) &
			     DC_CTRL_FLUSH_STATUS) == 0) {
				break;
			}
		} while (1);
		start_addr += line_size;
	} while (start_addr < end_addr);

	arch_irq_unlock(key); /* --exit critical section-- */

}

static void arch_dcache_invd(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_dcache_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	unsigned int key;

	if (!dcache_available() || (size == 0U) || line_size == 0U) {
		return;
	}
	end_addr = start_addr + size;
	start_addr = ROUND_DOWN(start_addr, line_size);

	key = arch_irq_lock(); /* -enter critical section- */

	do {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDL, start_addr);
		__builtin_arc_nop();
		__builtin_arc_nop();
		__builtin_arc_nop();
		start_addr += line_size;
	} while (start_addr < end_addr);
	irq_unlock(key); /* -exit critical section- */
}

int arch_dcache_range(void *addr, size_t size, int op)
{
	if (op == K_CACHE_INVD) {
		/*
		 * TODO: On invalidate we can contextually flush by setting the
		 * DC_CTRL_INVALID_FLUSH bit
		 */
		arch_dcache_invd(addr, size);
	} else if (op == K_CACHE_WB) {
		arch_dcache_flush(addr, size);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
static void init_dcache_line_size(void)
{
	uint32_t val;

	val = z_arc_v2_aux_reg_read(_ARC_V2_D_CACHE_BUILD);
	__ASSERT((val&0xff) != 0U, "d-cache is not present");
	val = ((val>>16) & 0xf) + 1;
	val *= 16U;
	sys_cache_line_size = (size_t) val;
}

size_t arch_dcache_line_size_get(void)
{
	return sys_cache_line_size;
}
#endif

static int init_dcache(const struct device *unused)
{
	ARG_UNUSED(unused);

	arch_dcache_enable();

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
	init_dcache_line_size();
#endif

	return 0;
}

SYS_INIT(init_dcache, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
