/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Routines for managing virtual address spaces
 */

#include <stdint.h>
#include <kernel_arch_interface.h>
#include <spinlock.h>
#include <mmu.h>
#include <init.h>
#include <kernel_internal.h>
#include <linker/linker-defs.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/*
 * General terminology:
 * - A page frame is a page-sized physical memory region in RAM. It is a
 *   container where a data page may be placed. It is always referred to by
 *   physical address. We have a convention of using uintptr_t for physical
 *   addresses. We instantiate a struct z_page_frame to store metadata for
 *   every page frame.
 *
 * - A data page is a page-sized region of data. It may exist in a page frame,
 *   or be paged out to some backing store. Its location can always be looked
 *   up in the CPU's page tables (or equivalent) by virtual address.
 *   The data type will always be void * or in some cases uint8_t * when we
 *   want to do pointer arithmetic.
 */

/* Spinlock to protect any globals in this file and serialize page table
 * updates in arch code
 */
struct k_spinlock z_mm_lock;

/*
 * General page frame management
 */

/* Database of all RAM page frames */
struct z_page_frame z_page_frames[Z_NUM_PAGE_FRAMES];

#if __ASSERT_ON
/* Indicator that z_page_frames has been initialized, many of these APIs do
 * not work before POST_KERNEL
 */
static bool page_frames_initialized;
#endif

/* Add colors to page table dumps to indicate mapping type */
#define COLOR_PAGE_FRAMES	1

#if COLOR_PAGE_FRAMES
#define ANSI_DEFAULT "\x1B[0m"
#define ANSI_RED     "\x1B[1;31m"
#define ANSI_GREEN   "\x1B[1;32m"
#define ANSI_YELLOW  "\x1B[1;33m"
#define ANSI_BLUE    "\x1B[1;34m"
#define ANSI_MAGENTA "\x1B[1;35m"
#define ANSI_CYAN    "\x1B[1;36m"
#define ANSI_GREY    "\x1B[1;90m"

#define COLOR(x)	printk(_CONCAT(ANSI_, x))
#else
#define COLOR(x)	do { } while (0)
#endif

static void page_frame_dump(struct z_page_frame *pf)
{
	if (z_page_frame_is_reserved(pf)) {
		COLOR(CYAN);
		printk("R");
	} else if (z_page_frame_is_busy(pf)) {
		COLOR(MAGENTA);
		printk("B");
	} else if (z_page_frame_is_pinned(pf)) {
		COLOR(YELLOW);
		printk("P");
	} else if (z_page_frame_is_available(pf)) {
		COLOR(GREY);
		printk(".");
	} else if (z_page_frame_is_mapped(pf)) {
		COLOR(DEFAULT);
		printk("M");
	} else {
		COLOR(RED);
		printk("?");
	}
}

void z_page_frames_dump(void)
{
	int column = 0;

	__ASSERT(page_frames_initialized, "%s called too early", __func__);
	printk("Physical memory from 0x%lx to 0x%lx\n",
	       Z_PHYS_RAM_START, Z_PHYS_RAM_END);

	for (int i = 0; i < Z_NUM_PAGE_FRAMES; i++) {
		struct z_page_frame *pf = &z_page_frames[i];

		page_frame_dump(pf);

		column++;
		if (column == 64) {
			column = 0;
			printk("\n");
		}
	}

	COLOR(DEFAULT);
	if (column != 0) {
		printk("\n");
	}
}

#define VIRT_FOREACH(_base, _size, _pos) \
	for (_pos = _base; \
	     _pos < ((uint8_t *)_base + _size); _pos += CONFIG_MMU_PAGE_SIZE)

#define PHYS_FOREACH(_base, _size, _pos) \
	for (_pos = _base; \
	     _pos < ((uintptr_t)_base + _size); _pos += CONFIG_MMU_PAGE_SIZE)


/*
 * Virtual address space management
 *
 * Call all of these functions with z_mm_lock held.
 *
 * Overall virtual memory map: When the kernel starts, it resides in
 * virtual memory in the region Z_KERNEL_VIRT_START to
 * Z_KERNEL_VIRT_END. Unused virtual memory past this, up to the limit
 * noted by CONFIG_KERNEL_VM_SIZE may be used for runtime memory mappings.
 *
 * If CONFIG_ARCH_MAPS_ALL_RAM is set, we do not just map the kernel image,
 * but have a mapping for all RAM in place. This is for special architectural
 * purposes and does not otherwise affect page frame accounting or flags;
 * the only guarantee is that such RAM mapping outside of the Zephyr image
 * won't be disturbed by subsequent memory mapping calls.
 *
 * +--------------+ <- Z_VIRT_RAM_START
 * | Undefined VM | <- May contain ancillary regions like x86_64's locore
 * +--------------+ <- Z_KERNEL_VIRT_START (often == Z_VIRT_RAM_START)
 * | Mapping for  |
 * | main kernel  |
 * | image        |
 * |		  |
 * |		  |
 * +--------------+ <- Z_FREE_VM_START
 * |              |
 * | Unused,      |
 * | Available VM |
 * |              |
 * |..............| <- mapping_pos (grows downward as more mappings are made)
 * | Mapping      |
 * +--------------+
 * | Mapping      |
 * +--------------+
 * | ...          |
 * +--------------+
 * | Mapping      |
 * +--------------+ <- mappings start here
 * | Reserved     | <- special purpose virtual page(s) of size Z_VM_RESERVED
 * +--------------+ <- Z_VIRT_RAM_END
 *
 * At the moment we just have one downward-growing area for mappings.
 * There is currently no support for un-mapping memory, see #28900.
 */
static uint8_t *mapping_pos = Z_VIRT_RAM_END - Z_VM_RESERVED;

/* Get a chunk of virtual memory and mark it as being in-use.
 *
 * This may be called from arch early boot code before z_cstart() is invoked.
 * Data will be copied and BSS zeroed, but this must not rely on any
 * initialization functions being called prior to work correctly.
 */
static void *virt_region_get(size_t size)
{
	uint8_t *dest_addr;

	if ((mapping_pos - size) < Z_FREE_VM_START) {
		LOG_ERR("insufficient virtual address space (requested %zu)",
			size);
		return NULL;
	}

	mapping_pos -= size;
	dest_addr = mapping_pos;

	return dest_addr;
}

/*
 * Free page frames management
 *
 * Call all of these functions with z_mm_lock held.
 */

/* Linked list of unused and available page frames.
 *
 * TODO: This is very simple and treats all free page frames as being equal.
 * However, there are use-cases to consolidate free pages such that entire
 * SRAM banks can be switched off to save power, and so obtaining free pages
 * may require a more complex ontology which prefers page frames in RAM banks
 * which are still active.
 *
 * This implies in the future there may be multiple slists managing physical
 * pages. Each page frame will still just have one snode link.
 */
static sys_slist_t free_page_frame_list;

/* Number of unused and available free page frames */
size_t z_free_page_count;

#define PF_ASSERT(pf, expr, fmt, ...) \
	__ASSERT(expr, "page frame 0x%lx: " fmt, z_page_frame_to_phys(pf), \
		 ##__VA_ARGS__)

/* Get an unused page frame. don't care which one, or NULL if there are none */
static struct z_page_frame *free_page_frame_list_get(void)
{
	sys_snode_t *node;
	struct z_page_frame *pf = NULL;

	node = sys_slist_get(&free_page_frame_list);
	if (node != NULL) {
		z_free_page_count--;
		pf = CONTAINER_OF(node, struct z_page_frame, node);
		PF_ASSERT(pf, z_page_frame_is_available(pf),
			 "unavailable but somehow on free list");
	}

	return pf;
}

/* Release a page frame back into the list of free pages */
static void free_page_frame_list_put(struct z_page_frame *pf)
{
	PF_ASSERT(pf, z_page_frame_is_available(pf),
		 "unavailable page put on free list");
	sys_slist_append(&free_page_frame_list, &pf->node);
	z_free_page_count++;
}

static void free_page_frame_list_init(void)
{
	sys_slist_init(&free_page_frame_list);
}

/*
 * Memory Mapping
 */

/* Called after the frame is mapped in the arch layer, to update our
 * local ontology (and do some assertions while we're at it)
 */
static void frame_mapped_set(struct z_page_frame *pf, void *addr)
{
	PF_ASSERT(pf, !z_page_frame_is_reserved(pf),
		  "attempted to map a reserved page frame");

	/* We do allow multiple mappings for pinned page frames
	 * since we will never need to reverse map them.
	 * This is uncommon, use-cases are for things like the
	 * Zephyr equivalent of VSDOs
	 */
	PF_ASSERT(pf, !z_page_frame_is_mapped(pf) || z_page_frame_is_pinned(pf),
		 "non-pinned and already mapped to %p", pf->addr);

	pf->flags |= Z_PAGE_FRAME_MAPPED;
	pf->addr = addr;
}

#ifdef CONFIG_DEMAND_PAGING
static int page_frame_prepare_locked(struct z_page_frame *pf, bool *dirty_ptr,
				     bool page_in, uintptr_t *location_ptr);
#endif /* CONFIG_DEMAND_PAGING */

/* Allocate a free page frame, and map it to a specified virtual address
 *
 * TODO: Add optional support for copy-on-write mappings to a zero page instead
 * of allocating, in which case page frames will be allocated lazily as
 * the mappings to the zero page get touched. This will avoid expensive
 * page-ins as memory is mapped and physical RAM or backing store storage will
 * not be used if the mapped memory is unused. The cost is an empty physical
 * page of zeroes.
 */
static int map_anon_page(void *addr, uint32_t flags)
{
	struct z_page_frame *pf;
	uintptr_t phys;
	bool lock = (flags & K_MEM_MAP_LOCK) != 0;
	bool uninit = (flags & K_MEM_MAP_UNINIT) != 0;

	pf = free_page_frame_list_get();
	if (pf == NULL) {
#ifdef CONFIG_DEMAND_PAGING
		uintptr_t location;
		bool dirty;
		int ret;

		pf = z_eviction_select(&dirty);
		__ASSERT(pf != NULL, "failed to get a page frame");
		LOG_DBG("evicting %p at 0x%lx", pf->addr,
			z_page_frame_to_phys(pf));
		ret = page_frame_prepare_locked(pf, &dirty, false, &location);
		if (ret != 0) {
			return -ENOMEM;
		}
		if (dirty) {
			z_backing_store_page_out(location);
		}
		pf->flags = 0;
#else
		return -ENOMEM;
#endif /* CONFIG_DEMAND_PAGING */
	}

	phys = z_page_frame_to_phys(pf);
	arch_mem_map(addr, phys, CONFIG_MMU_PAGE_SIZE, flags | K_MEM_CACHE_WB);

	if (lock) {
		pf->flags |= Z_PAGE_FRAME_PINNED;
	}
	frame_mapped_set(pf, addr);

	LOG_DBG("memory mapping anon page %p -> 0x%lx", addr, phys);

	if (!uninit) {
		/* If we later implement mappings to a copy-on-write
		 * zero page, won't need this step
		 */
		memset(addr, 0, CONFIG_MMU_PAGE_SIZE);
	}

	return 0;
}

void *k_mem_map(size_t size, uint32_t flags)
{
	uint8_t *dst;
	size_t total_size = size;
	int ret;
	k_spinlock_key_t key;
	bool guard = (flags & K_MEM_MAP_GUARD) != 0;
	uint8_t *pos;

	__ASSERT(!(((flags & K_MEM_PERM_USER) != 0) &&
		   ((flags & K_MEM_MAP_UNINIT) != 0)),
		 "user access to anonymous uninitialized pages is forbidden");
	__ASSERT(size % CONFIG_MMU_PAGE_SIZE == 0,
		 "unaligned size %zu passed to %s", size, __func__);
	__ASSERT(size != 0, "zero sized memory mapping");
	__ASSERT(page_frames_initialized, "%s called too early", __func__);
	__ASSERT((flags & K_MEM_CACHE_MASK) == 0,
		 "%s does not support explicit cache settings", __func__);

	key = k_spin_lock(&z_mm_lock);

	if (guard) {
		/* Need extra virtual page for the guard which we
		 * won't map
		 */
		total_size += CONFIG_MMU_PAGE_SIZE;
	}

	dst = virt_region_get(total_size);
	if (dst == NULL) {
		/* Address space has no free region */
		goto out;
	}
	if (guard) {
		/* Skip over the guard page in returned address. */
		dst += CONFIG_MMU_PAGE_SIZE;
	}

	VIRT_FOREACH(dst, size, pos) {
		ret = map_anon_page(pos, flags);

		if (ret != 0) {
			/* TODO: call k_mem_unmap(dst, pos - dst)  when
			 * implmented in #28990 and release any guard virtual
			 * page as well.
			 */
			dst = NULL;
			goto out;
		}
	}
out:
	k_spin_unlock(&z_mm_lock, key);
	return dst;
}

size_t k_mem_free_get(void)
{
	size_t ret;
	k_spinlock_key_t key;

	__ASSERT(page_frames_initialized, "%s called too early", __func__);

	key = k_spin_lock(&z_mm_lock);
	ret = z_free_page_count;
	k_spin_unlock(&z_mm_lock, key);

	return ret * CONFIG_MMU_PAGE_SIZE;
}

/* This may be called from arch early boot code before z_cstart() is invoked.
 * Data will be copied and BSS zeroed, but this must not rely on any
 * initialization functions being called prior to work correctly.
 */
void z_phys_map(uint8_t **virt_ptr, uintptr_t phys, size_t size, uint32_t flags)
{
	uintptr_t aligned_phys, addr_offset;
	size_t aligned_size;
	k_spinlock_key_t key;
	uint8_t *dest_addr;

	addr_offset = k_mem_region_align(&aligned_phys, &aligned_size,
					 phys, size,
					 CONFIG_MMU_PAGE_SIZE);
	__ASSERT(aligned_size != 0, "0-length mapping at 0x%lx", aligned_phys);
	__ASSERT(aligned_phys < (aligned_phys + (aligned_size - 1)),
		 "wraparound for physical address 0x%lx (size %zu)",
		 aligned_phys, aligned_size);

	key = k_spin_lock(&z_mm_lock);
	/* Obtain an appropriately sized chunk of virtual memory */
	dest_addr = virt_region_get(aligned_size);
	if (!dest_addr) {
		goto fail;
	}

	/* If this fails there's something amiss with virt_region_get */
	__ASSERT((uintptr_t)dest_addr <
		 ((uintptr_t)dest_addr + (size - 1)),
		 "wraparound for virtual address %p (size %zu)",
		 dest_addr, size);

	LOG_DBG("arch_mem_map(%p, 0x%lx, %zu, %x) offset %lu", dest_addr,
		aligned_phys, aligned_size, flags, addr_offset);

	arch_mem_map(dest_addr, aligned_phys, aligned_size, flags);
	k_spin_unlock(&z_mm_lock, key);

	*virt_ptr = dest_addr + addr_offset;
	return;
fail:
	/* May re-visit this in the future, but for now running out of
	 * virtual address space or failing the arch_mem_map() call is
	 * an unrecoverable situation.
	 *
	 * Other problems not related to resource exhaustion we leave as
	 * assertions since they are clearly programming mistakes.
	 */
	LOG_ERR("memory mapping 0x%lx (size %zu, flags 0x%x) failed",
		phys, size, flags);
	k_panic();
}

/*
 * Miscellaneous
 */

size_t k_mem_region_align(uintptr_t *aligned_phys, size_t *aligned_size,
			  uintptr_t phys_addr, size_t size, size_t align)
{
	size_t addr_offset;

	/* The actual mapped region must be page-aligned. Round down the
	 * physical address and pad the region size appropriately
	 */
	*aligned_phys = ROUND_DOWN(phys_addr, align);
	addr_offset = phys_addr - *aligned_phys;
	*aligned_size = ROUND_UP(size + addr_offset, align);

	return addr_offset;
}


#ifdef CONFIG_USERSPACE
void z_kernel_map_fixup(void)
{
	/* XXX: Gperf kernel object data created at build time will not have
	 * visibility in zephyr_prebuilt.elf. There is a possibility that this
	 * data would not be memory-mapped if it shifts z_mapped_end between
	 * builds. Ensure this area is mapped.
	 *
	 * A third build phase for page tables would solve this.
	 */
	uint8_t *kobject_page_begin =
		(uint8_t *)ROUND_DOWN((uintptr_t)&z_kobject_data_begin,
				      CONFIG_MMU_PAGE_SIZE);
	size_t kobject_size = (size_t)(Z_KERNEL_VIRT_END - kobject_page_begin);

	if (kobject_size != 0) {
		arch_mem_map(kobject_page_begin,
			     Z_BOOT_VIRT_TO_PHYS(kobject_page_begin),
			     kobject_size, K_MEM_PERM_RW | K_MEM_CACHE_WB);
	}
}
#endif /* CONFIG_USERSPACE */

void z_mem_manage_init(void)
{
	uintptr_t phys;
	uint8_t *addr;
	struct z_page_frame *pf;
	k_spinlock_key_t key = k_spin_lock(&z_mm_lock);

	free_page_frame_list_init();

#ifdef CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES
	/* If some page frames are unavailable for use as memory, arch
	 * code will mark Z_PAGE_FRAME_RESERVED in their flags
	 */
	arch_reserved_pages_update();
#endif /* CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES */

	/* All pages composing the Zephyr image are mapped at boot in a
	 * predictable way. This can change at runtime.
	 */
	VIRT_FOREACH(Z_KERNEL_VIRT_START, Z_KERNEL_VIRT_SIZE, addr)
	{
		pf = z_phys_to_page_frame(Z_BOOT_VIRT_TO_PHYS(addr));
		frame_mapped_set(pf, addr);

		/* TODO: for now we pin the whole Zephyr image. Demand paging
		 * currently tested with anonymously-mapped pages which are not
		 * pinned.
		 *
		 * We will need to setup linker regions for a subset of kernel
		 * code/data pages which are pinned in memory and
		 * may not be evicted. This will contain critical CPU data
		 * structures, and any code used to perform page fault
		 * handling, page-ins, etc.
		 */
		pf->flags |= Z_PAGE_FRAME_PINNED;
	}

	/* Any remaining pages that aren't mapped, reserved, or pinned get
	 * added to the free pages list
	 */
	Z_PAGE_FRAME_FOREACH(phys, pf) {
		if (z_page_frame_is_available(pf)) {
			free_page_frame_list_put(pf);
		}
	}
	LOG_DBG("free page frames: %zu", z_free_page_count);

#ifdef CONFIG_DEMAND_PAGING
	z_backing_store_init();
	z_eviction_init();
#endif
#if __ASSERT_ON
	page_frames_initialized = true;
#endif
	k_spin_unlock(&z_mm_lock, key);
}

#ifdef CONFIG_DEMAND_PAGING
static unsigned long z_num_pagefaults;

/* Current implementation relies on interrupt locking to any prevent page table
 * access, which falls over if other CPUs are active. Addressing this is not
 * as simple as using spinlocks as regular memory reads/writes constitute
 * "access" in this sense.
 *
 * Current needs for demand paging are on uniprocessor systems.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP));

static void virt_region_foreach(void *addr, size_t size,
				void (*func)(void *))
{
	z_mem_assert_virtual_region(addr, size);

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		func((uint8_t *)addr + offset);
	}
}

static void page_frame_free_locked(struct z_page_frame *pf)
{
	pf->flags = 0;
	free_page_frame_list_put(pf);
}

/*
 * Perform some preparatory steps before paging out. The provided page frame
 * must be evicted to the backing store immediately after this is called
 * with a call to z_backing_store_page_out() if it contains a data page.
 *
 * - Map page frame to scratch area if requested. This always is true if we're
 *   doing a page fault, but is only set on manual evictions if the page is
 *   dirty.
 * - If mapped:
 *    - obtain backing store location and populate location parameter
 *    - Update page tables with location
 * - Mark page frame as busy
 *
 * Returns -ENOMEM if the backing store is full
 */
static int page_frame_prepare_locked(struct z_page_frame *pf, bool *dirty_ptr,
				     bool page_fault, uintptr_t *location_ptr)
{
	uintptr_t phys;
	int ret;
	bool dirty = *dirty_ptr;

	phys = z_page_frame_to_phys(pf);
	__ASSERT(!z_page_frame_is_pinned(pf), "page frame 0x%lx is pinned",
		 phys);

	/* If the backing store doesn't have a copy of the page, even if it
	 * wasn't modified, treat as dirty. This can happen for a few
	 * reasons:
	 * 1) Page has never been swapped out before, and the backing store
	 *    wasn't pre-populated with this data page.
	 * 2) Page was swapped out before, but the page contents were not
	 *    preserved after swapping back in.
	 * 3) Page contents were preserved when swapped back in, but were later
	 *    evicted from the backing store to make room for other evicted
	 *    pages.
	 */
	if (z_page_frame_is_mapped(pf)) {
		dirty = dirty || !z_page_frame_is_backed(pf);
	}

	if (dirty || page_fault) {
		arch_mem_scratch(phys);
	}

	if (z_page_frame_is_mapped(pf)) {
		ret = z_backing_store_location_get(pf, location_ptr,
						   page_fault);
		if (ret != 0) {
			LOG_ERR("out of backing store memory");
			return -ENOMEM;
		}
		arch_mem_page_out(pf->addr, *location_ptr);
	} else {
		/* Shouldn't happen unless this function is mis-used */
		__ASSERT(!dirty, "un-mapped page determined to be dirty");
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	/* Mark as busy so that z_page_frame_is_evictable() returns false */
	__ASSERT(!z_page_frame_is_busy(pf), "page frame 0x%lx is already busy",
		 phys);
	pf->flags |= Z_PAGE_FRAME_BUSY;
#endif
	/* Update dirty parameter, since we set to true if it wasn't backed
	 * even if otherwise clean
	 */
	*dirty_ptr = dirty;

	return 0;
}

static int do_mem_evict(void *addr)
{
	bool dirty;
	struct z_page_frame *pf;
	uintptr_t location;
	int key, ret;
	uintptr_t flags, phys;

#if CONFIG_DEMAND_PAGING_ALLOW_IRQ
	__ASSERT(!k_is_in_isr(),
		 "%s is unavailable in ISRs with CONFIG_DEMAND_PAGING_ALLOW_IRQ",
		 __func__);
	k_sched_lock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	key = irq_lock();
	flags = arch_page_info_get(addr, &phys, false);
	__ASSERT((flags & ARCH_DATA_PAGE_NOT_MAPPED) == 0,
		 "address %p isn't mapped", addr);
	if ((flags & ARCH_DATA_PAGE_LOADED) == 0) {
		/* Un-mapped or already evicted. Nothing to do */
		ret = 0;
		goto out;
	}

	dirty = (flags & ARCH_DATA_PAGE_DIRTY) != 0;
	pf = z_phys_to_page_frame(phys);
	__ASSERT(pf->addr == addr, "page frame address mismatch");
	ret = page_frame_prepare_locked(pf, &dirty, false, &location);
	if (ret != 0) {
		goto out;
	}

	__ASSERT(ret == 0, "failed to prepare page frame");
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	irq_unlock(key);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		z_backing_store_page_out(location);
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = irq_lock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	page_frame_free_locked(pf);
out:
	irq_unlock(key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_sched_unlock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	return ret;
}

int k_mem_page_out(void *addr, size_t size)
{
	__ASSERT(page_frames_initialized, "%s called on %p too early", __func__,
		 addr);
	z_mem_assert_virtual_region(addr, size);

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		void *pos = (uint8_t *)addr + offset;
		int ret;

		ret = do_mem_evict(pos);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int z_page_frame_evict(uintptr_t phys)
{
	int key, ret;
	struct z_page_frame *pf;
	bool dirty;
	uintptr_t flags;
	uintptr_t location;

	__ASSERT(page_frames_initialized, "%s called on 0x%lx too early",
		 __func__, phys);

	/* Implementation is similar to do_page_fault() except there is no
	 * data page to page-in, see comments in that function.
	 */

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	__ASSERT(!k_is_in_isr(),
		 "%s is unavailable in ISRs with CONFIG_DEMAND_PAGING_ALLOW_IRQ",
		 __func__);
	k_sched_lock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	key = irq_lock();
	pf = z_phys_to_page_frame(phys);
	if (!z_page_frame_is_mapped(pf)) {
		/* Nothing to do, free page */
		ret = 0;
		goto out;
	}
	flags = arch_page_info_get(pf->addr, NULL, false);
	/* Shouldn't ever happen */
	__ASSERT((flags & ARCH_DATA_PAGE_LOADED) != 0, "data page not loaded");
	dirty = (flags & ARCH_DATA_PAGE_DIRTY) != 0;
	ret = page_frame_prepare_locked(pf, &dirty, false, &location);
	if (ret != 0) {
		goto out;
	}

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	irq_unlock(key);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		z_backing_store_page_out(location);
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = irq_lock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	page_frame_free_locked(pf);
out:
	irq_unlock(key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_sched_unlock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	return ret;
}

static bool do_page_fault(void *addr, bool pin)
{
	struct z_page_frame *pf;
	int key, ret;
	uintptr_t page_in_location, page_out_location;
	enum arch_page_location status;
	bool result;
	bool dirty = false;

	__ASSERT(page_frames_initialized, "page fault at %p happened too early",
		 addr);

	LOG_DBG("page fault at %p", addr);

	/*
	 * TODO: Add performance accounting:
	 * - Number of pagefaults
	 *   * gathered on a per-thread basis:
	 *     . Pagefaults with IRQs locked in faulting thread (bad)
	 *     . Pagefaults with IRQs unlocked in faulting thread
	 *   * Pagefaults in ISRs (if allowed)
	 * - z_eviction_select() metrics
	 *   * Clean vs dirty page eviction counts
	 *   * execution time histogram
	 *   * periodic timer execution time histogram (if implemented)
	 * - z_backing_store_page_out() execution time histogram
	 * - z_backing_store_page_in() execution time histogram
	 */

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	/* We lock the scheduler so that other threads are never scheduled
	 * during the page-in/out operation.
	 *
	 * We do however re-enable interrupts during the page-in/page-out
	 * operation iff interrupts were enabled when the exception was taken;
	 * in this configuration page faults in an ISR are a bug; all their
	 * code/data must be pinned.
	 *
	 * If interrupts were disabled when the exception was taken, the
	 * arch code is responsible for keeping them that way when entering
	 * this function.
	 *
	 * If this is not enabled, then interrupts are always locked for the
	 * entire operation. This is far worse for system interrupt latency
	 * but requires less pinned pages and ISRs may also take page faults.
	 *
	 * Support for allowing z_backing_store_page_out() and
	 * z_backing_store_page_in() to also sleep and allow other threads to
	 * run (such as in the case where the transfer is async DMA) is not
	 * implemented. Even if limited to thread context, arbitrary memory
	 * access triggering exceptions that put a thread to sleep on a
	 * contended page fault operation will break scheduling assumptions of
	 * cooperative threads or threads that implement crticial sections with
	 * spinlocks or disabling IRQs.
	 */
	k_sched_lock();
	__ASSERT(!k_is_in_isr(), "ISR page faults are forbidden");
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */

	key = irq_lock();
	status = arch_page_location_get(addr, &page_in_location);
	if (status == ARCH_PAGE_LOCATION_BAD) {
		/* Return false to treat as a fatal error */
		result = false;
		goto out;
	}
	result = true;
	if (status == ARCH_PAGE_LOCATION_PAGED_IN) {
		if (pin) {
			/* It's a physical memory address */
			uintptr_t phys = page_in_location;

			pf = z_phys_to_page_frame(phys);
			pf->flags |= Z_PAGE_FRAME_PINNED;
		}
		/* We raced before locking IRQs, re-try */
		goto out;
	}
	__ASSERT(status == ARCH_PAGE_LOCATION_PAGED_OUT,
		 "unexpected status value %d", status);

	pf = free_page_frame_list_get();
	if (pf == NULL) {
		/* Need to evict a page frame */
		pf = z_eviction_select(&dirty);
		__ASSERT(pf != NULL, "failed to get a page frame");
		LOG_DBG("evicting %p at 0x%lx", pf->addr,
			z_page_frame_to_phys(pf));
	}
	ret = page_frame_prepare_locked(pf, &dirty, true, &page_out_location);
	__ASSERT(ret == 0, "failed to prepare page frame");

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	irq_unlock(key);
	/* Interrupts are now unlocked if they were not locked when we entered
	 * this function, and we may service ISRs. The scheduler is still
	 * locked.
	 */
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		z_backing_store_page_out(page_out_location);
	}
	z_backing_store_page_in(page_in_location);

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = irq_lock();
	pf->flags &= ~Z_PAGE_FRAME_BUSY;
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (pin) {
		pf->flags |= Z_PAGE_FRAME_PINNED;
	}
	pf->flags |= Z_PAGE_FRAME_MAPPED;
	pf->addr = addr;
	arch_mem_page_in(addr, z_page_frame_to_phys(pf));
	z_backing_store_page_finalize(pf, page_in_location);
out:
	irq_unlock(key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_sched_unlock();
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */

	return result;
}

static void do_page_in(void *addr)
{
	bool ret;

	ret = do_page_fault(addr, false);
	__ASSERT(ret, "unmapped memory address %p", addr);
	(void)ret;
}

void k_mem_page_in(void *addr, size_t size)
{
	__ASSERT(!IS_ENABLED(CONFIG_DEMAND_PAGING_ALLOW_IRQ) || !k_is_in_isr(),
		 "%s may not be called in ISRs if CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled",
		 __func__);
	virt_region_foreach(addr, size, do_page_in);
}

static void do_mem_pin(void *addr)
{
	bool ret;

	ret = do_page_fault(addr, true);
	__ASSERT(ret, "unmapped memory address %p", addr);
	(void)ret;
}

void k_mem_pin(void *addr, size_t size)
{
	__ASSERT(!IS_ENABLED(CONFIG_DEMAND_PAGING_ALLOW_IRQ) || !k_is_in_isr(),
		 "%s may not be called in ISRs if CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled",
		 __func__);
	virt_region_foreach(addr, size, do_mem_pin);
}

bool z_page_fault(void *addr)
{
	bool ret;

	ret = do_page_fault(addr, false);
	if (ret) {
		/* Wasn't an error, increment page fault count */
		int key;

		key = irq_lock();
		z_num_pagefaults++;
		irq_unlock(key);
	}
	return ret;
}

unsigned long z_num_pagefaults_get(void)
{
	unsigned long ret;
	int key;

	key = irq_lock();
	ret = z_num_pagefaults;
	irq_unlock(key);

	return ret;
}

static void do_mem_unpin(void *addr)
{
	struct z_page_frame *pf;
	int key;
	uintptr_t flags, phys;

	key = irq_lock();
	flags = arch_page_info_get(addr, &phys, false);
	__ASSERT((flags & ARCH_DATA_PAGE_NOT_MAPPED) == 0,
		 "invalid data page at %p", addr);
	if ((flags & ARCH_DATA_PAGE_LOADED) != 0) {
		pf = z_phys_to_page_frame(phys);
		pf->flags &= ~Z_PAGE_FRAME_PINNED;
	}
	irq_unlock(key);
}

void k_mem_unpin(void *addr, size_t size)
{
	__ASSERT(page_frames_initialized, "%s called on %p too early", __func__,
		 addr);
	virt_region_foreach(addr, size, do_mem_unpin);
}
#endif /* CONFIG_DEMAND_PAGING */
