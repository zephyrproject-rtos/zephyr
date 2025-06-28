/* cache.c - d-cache support for ARC CPUs */

/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
 * Copyright (c) 2025 GSI Technology, All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief d-cache manipulation
 *
 * This module contains functions for manipulation of the d-cache.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/cache.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arc/v2/aux_regs.h>
#include <kernel_internal.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/init.h>
#include <stdbool.h>

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
size_t sys_cache_line_size;
#endif

#define DC_CTRL_DC_ENABLE            0x0   /* enable d-cache */
#define DC_CTRL_DC_DISABLE           0x1   /* disable d-cache */
#define DC_CTRL_INVALID_ONLY         0x0   /* invalid d-cache only */
#define DC_CTRL_INVALID_FLUSH        0x40  /* invalid and flush d-cache */
#define DC_CTRL_ENABLE_FLUSH_LOCKED  0x80  /* locked d-cache can be flushed */
#define DC_CTRL_DISABLE_FLUSH_LOCKED 0x0   /* locked d-cache cannot be flushed */
#define DC_CTRL_FLUSH_STATUS         0x100 /* flush status */
#define DC_CTRL_DIRECT_ACCESS        0x0   /* direct access mode  */
#define DC_CTRL_INDIRECT_ACCESS      0x20  /* indirect access mode */
#define DC_CTRL_OP_SUCCEEDED         0x4   /* d-cache operation succeeded */
#define DC_CTRL_INVALIDATE_MODE      0x40  /* d-cache invalidate mode bit */
#define DC_CTRL_REGION_OP            0xe00 /* d-cache region operation */

#define MMU_BUILD_PHYSICAL_ADDR_EXTENSION 0x1000 /* physical address extension enable mask */

#define SLC_CTRL_DISABLE         0x1   /* SLC disable */
#define SLC_CTRL_INVALIDATE_MODE 0x40  /* SLC invalidate mode  */
#define SLC_CTRL_BUSY_STATUS     0x100 /* SLC busy status */
#define SLC_CTRL_REGION_OP       0xe00 /* SLC region operation */

#if defined(CONFIG_ARC_SLC)
/*
 * spinlock is used for SLC access because depending on HW configuration, the SLC might be shared
 * between the cores, and in this case, only one core is allowed to access the SLC register
 * interface at a time.
 */
static struct k_spinlock slc_lock;
#endif

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

static bool pae_exists(void)
{
	uint32_t bcr = z_arc_v2_aux_reg_read(_ARC_V2_MMU_BUILD);

	return 1 == FIELD_GET(MMU_BUILD_PHYSICAL_ADDR_EXTENSION, bcr);
}

#if defined(CONFIG_ARC_SLC)
static void slc_enable(void)
{
	uint32_t val = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);

	val &= ~SLC_CTRL_DISABLE;
	z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, val);
}

static void slc_high_addr_init(void)
{
	z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_END1, 0);
	z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_START1, 0);
}

static void slc_flush_region(void *start_addr_ptr, size_t size)
{
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;

	K_SPINLOCK(&slc_lock) {
		ctrl = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);

		ctrl &= ~SLC_CTRL_REGION_OP;

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, ctrl);

		/*
		 * END needs to be setup before START (latter triggers the operation)
		 * END can't be same as START, so add (l2_line_sz - 1) to sz
		 */
		end_addr = start_addr + size + CONFIG_ARC_SLC_LINE_SIZE - 1;

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_END, end_addr);
		/* Align start address to cache line size, see STAR 5103816 */
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_START,
				       start_addr & ~(CONFIG_ARC_SLC_LINE_SIZE - 1));

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}

static void slc_invalidate_region(void *start_addr_ptr, size_t size)
{
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;

	K_SPINLOCK(&slc_lock) {
		ctrl = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);

		ctrl &= ~SLC_CTRL_INVALIDATE_MODE;

		ctrl &= ~SLC_CTRL_REGION_OP;
		ctrl |= FIELD_PREP(SLC_CTRL_REGION_OP, 0x1);

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, ctrl);

		/*
		 * END needs to be setup before START (latter triggers the operation)
		 * END can't be same as START, so add (l2_line_sz - 1) to sz
		 */
		end_addr = start_addr + size + CONFIG_ARC_SLC_LINE_SIZE - 1;

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_END, end_addr);
		/* Align start address to cache line size, see STAR 5103816 */
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_START,
				       start_addr & ~(CONFIG_ARC_SLC_LINE_SIZE - 1));

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}

static void slc_flush_and_invalidate_region(void *start_addr_ptr, size_t size)
{
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;

	K_SPINLOCK(&slc_lock) {
		ctrl = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);

		ctrl |= SLC_CTRL_INVALIDATE_MODE;

		ctrl &= ~SLC_CTRL_REGION_OP;
		ctrl |= FIELD_PREP(SLC_CTRL_REGION_OP, 0x1);

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, ctrl);

		/*
		 * END needs to be setup before START (latter triggers the operation)
		 * END can't be same as START, so add (l2_line_sz - 1) to sz
		 */
		end_addr = start_addr + size + CONFIG_ARC_SLC_LINE_SIZE - 1;

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_END, end_addr);
		/* Align start address to cache line size, see STAR 5103816 */
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_RGN_START,
				       start_addr & ~(CONFIG_ARC_SLC_LINE_SIZE - 1));

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}

static void slc_flush_all(void)
{
	K_SPINLOCK(&slc_lock) {
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_FLUSH, 0x1);

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}

static void slc_invalidate_all(void)
{
	uint32_t ctrl;

	K_SPINLOCK(&slc_lock) {
		ctrl = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		ctrl &= ~SLC_CTRL_INVALIDATE_MODE;
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, ctrl);

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_INVALIDATE, 0x1);

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}

static void slc_flush_and_invalidate_all(void)
{
	uint32_t ctrl;

	K_SPINLOCK(&slc_lock) {
		ctrl = z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		ctrl |= SLC_CTRL_INVALIDATE_MODE;
		z_arc_v2_aux_reg_write(_ARC_V2_SLC_CTRL, ctrl);

		z_arc_v2_aux_reg_write(_ARC_V2_SLC_INVALIDATE, 0x1);

		/* Make sure "busy" bit reports correct status, see STAR 9001165532 */
		z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL);
		while (z_arc_v2_aux_reg_read(_ARC_V2_SLC_CTRL) & SLC_CTRL_BUSY_STATUS) {
			/* Do Nothing */
		}
	}
}
#endif /* CONFIG_ARC_SLC */

void arch_dcache_enable(void)
{
	dcache_dc_ctrl(DC_CTRL_DC_ENABLE);

#if defined(CONFIG_ARC_SLC)
	slc_enable();
#endif
}

void arch_dcache_disable(void)
{
	/* nothing */
}

#if defined(CONFIG_ARC_DCACHE_REGION_OPERATIONS)
static void dcache_high_addr_init(void)
{
	z_arc_v2_aux_reg_write(_ARC_V2_DC_PTAG_HI, 0);
}

static void dcache_flush_region(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;
	unsigned int key;

	key = arch_irq_lock(); /* --enter critical section-- */

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);

	ctrl &= ~DC_CTRL_REGION_OP;

	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	end_addr = start_addr + size + line_size - 1;

	z_arc_v2_aux_reg_write(_ARC_V2_DC_ENDR, end_addr);
	z_arc_v2_aux_reg_write(_ARC_V2_DC_STARTR, start_addr);

	while (z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) & DC_CTRL_FLUSH_STATUS) {
		/* Do nothing */
	}

	arch_irq_unlock(key); /* --exit critical section-- */
}

static void dcache_invalidate_region(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;
	unsigned int key;

	key = arch_irq_lock(); /* --enter critical section-- */

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);

	ctrl &= ~DC_CTRL_INVALIDATE_MODE;

	ctrl &= ~DC_CTRL_REGION_OP;
	ctrl |= FIELD_PREP(DC_CTRL_REGION_OP, 0x1);

	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	end_addr = start_addr + size + line_size - 1;

	z_arc_v2_aux_reg_write(_ARC_V2_DC_ENDR, end_addr);
	z_arc_v2_aux_reg_write(_ARC_V2_DC_STARTR, start_addr);

	arch_irq_unlock(key); /* --exit critical section-- */
}

static void dcache_flush_and_invalidate_region(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	uint32_t ctrl;
	unsigned int key;

	key = arch_irq_lock(); /* --enter critical section-- */

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);

	ctrl |= DC_CTRL_INVALIDATE_MODE;

	ctrl &= ~DC_CTRL_REGION_OP;
	ctrl |= FIELD_PREP(DC_CTRL_REGION_OP, 0x1);

	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	end_addr = start_addr + size + line_size - 1;

	z_arc_v2_aux_reg_write(_ARC_V2_DC_ENDR, end_addr);
	z_arc_v2_aux_reg_write(_ARC_V2_DC_STARTR, start_addr);

	while (z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) & DC_CTRL_FLUSH_STATUS) {
		/* Do nothing */
	}

	arch_irq_unlock(key); /* --exit critical section-- */
}

#else /* CONFIG_ARC_DCACHE_REGION_OPERATIONS */

static void dcache_flush_lines(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	unsigned int key;

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

static void dcache_invalidate_lines(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	unsigned int key;
	uint32_t ctrl;

	end_addr = start_addr + size;
	start_addr = ROUND_DOWN(start_addr, line_size);

	key = arch_irq_lock(); /* -enter critical section- */

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);
	ctrl &= ~DC_CTRL_INVALIDATE_MODE;
	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	do {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDL, start_addr);
		__builtin_arc_nop();
		__builtin_arc_nop();
		__builtin_arc_nop();
		start_addr += line_size;
	} while (start_addr < end_addr);
	irq_unlock(key); /* -exit critical section- */
}

static void dcache_flush_and_invalidate_lines(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();
	uintptr_t start_addr = (uintptr_t)start_addr_ptr;
	uintptr_t end_addr;
	unsigned int key;
	uint32_t ctrl;

	end_addr = start_addr + size;
	start_addr = ROUND_DOWN(start_addr, line_size);

	key = arch_irq_lock(); /* -enter critical section- */

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);
	ctrl |= DC_CTRL_INVALIDATE_MODE;
	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	do {
		z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDL, start_addr);
		__builtin_arc_nop();
		__builtin_arc_nop();
		__builtin_arc_nop();
		start_addr += line_size;
	} while (start_addr < end_addr);
	irq_unlock(key); /* -exit critical section- */
}

#endif /* CONFIG_ARC_DCACHE_REGION_OPERATIONS */

int arch_dcache_flush_range(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (!dcache_available() || (size == 0U) || line_size == 0U) {
		return -ENOTSUP;
	}

#if defined(CONFIG_ARC_DCACHE_REGION_OPERATIONS)
	dcache_flush_region(start_addr_ptr, size);
#else
	dcache_flush_lines(start_addr_ptr, size);
#endif

#if defined(CONFIG_ARC_SLC)
	slc_flush_region(start_addr_ptr, size);
#endif

	return 0;
}

int arch_dcache_invd_range(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (!dcache_available() || (size == 0U) || line_size == 0U) {
		return -ENOTSUP;
	}

#if defined(CONFIG_ARC_DCACHE_REGION_OPERATIONS)
	dcache_invalidate_region(start_addr_ptr, size);
#else
	dcache_invalidate_lines(start_addr_ptr, size);
#endif

#if defined(CONFIG_ARC_SLC)
	slc_invalidate_region(start_addr_ptr, size);
#endif

	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr_ptr, size_t size)
{
	size_t line_size = sys_cache_data_line_size_get();

	if (!dcache_available() || (size == 0U) || line_size == 0U) {
		return -ENOTSUP;
	}

#if defined(CONFIG_ARC_DCACHE_REGION_OPERATIONS)
	dcache_flush_and_invalidate_region(start_addr_ptr, size);
#else
	dcache_flush_and_invalidate_lines(start_addr_ptr, size);
#endif

#if defined(CONFIG_ARC_SLC)
	slc_flush_and_invalidate_region(start_addr_ptr, size);
#endif

	return 0;
}

int arch_dcache_flush_all(void)
{
	size_t line_size = sys_cache_data_line_size_get();
	unsigned int key;

	if (!dcache_available() || line_size == 0U) {
		return -ENOTSUP;
	}

	key = irq_lock();

	z_arc_v2_aux_reg_write(_ARC_V2_DC_FLSH, 0x1);

	while (z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) & DC_CTRL_FLUSH_STATUS) {
		/* Do nothing */
	}

	irq_unlock(key);

#if defined(CONFIG_ARC_SLC)
	slc_flush_all();
#endif

	return 0;
}

int arch_dcache_invd_all(void)
{
	size_t line_size = sys_cache_data_line_size_get();
	unsigned int key;
	uint32_t ctrl;

	if (!dcache_available() || line_size == 0U) {
		return -ENOTSUP;
	}

	key = irq_lock();

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);
	ctrl &= ~DC_CTRL_INVALIDATE_MODE;
	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDC, 0x1);

	irq_unlock(key);

#if defined(CONFIG_ARC_SLC)
	slc_invalidate_all();
#endif

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	size_t line_size = sys_cache_data_line_size_get();
	unsigned int key;
	uint32_t ctrl;

	if (!dcache_available() || line_size == 0U) {
		return -ENOTSUP;
	}

	key = irq_lock();

	ctrl = z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL);
	ctrl |= DC_CTRL_INVALIDATE_MODE;
	z_arc_v2_aux_reg_write(_ARC_V2_DC_CTRL, ctrl);

	z_arc_v2_aux_reg_write(_ARC_V2_DC_IVDC, 0x1);

	while (z_arc_v2_aux_reg_read(_ARC_V2_DC_CTRL) & DC_CTRL_FLUSH_STATUS) {
		/* Do nothing */
	}

	irq_unlock(key);

#if defined(CONFIG_ARC_SLC)
	slc_flush_and_invalidate_all();
#endif

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

void arch_icache_enable(void)
{
	/* nothing */
}

void arch_icache_disable(void)
{
	/* nothing */
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_invd_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int arch_icache_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

static int init_dcache(void)
{
	sys_cache_data_enable();

#if defined(CONFIG_DCACHE_LINE_SIZE_DETECT)
	init_dcache_line_size();
#endif

	/*
	 * Init high address registers to 0 if PAE exists, cache operations for 40 bit addresses not
	 * implemented
	 */
	if (pae_exists()) {
#if defined(CONFIG_ARC_DCACHE_REGION_OPERATIONS)
		dcache_high_addr_init();
#endif
#if defined(CONFIG_ARC_SLC)
		slc_high_addr_init();
#endif
	}

	return 0;
}


void arch_cache_init(void)
{
	init_dcache();
}
