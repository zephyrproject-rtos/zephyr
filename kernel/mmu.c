/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Routines for managing virtual address spaces
 */

#include <stdint.h>
#include <kernel_arch_interface.h>
#include <zephyr/spinlock.h>
#include <mmu.h>
#include <zephyr/init.h>
#include <kernel_internal.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/timing/timing.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/*
 * General terminology:
 * - A page frame is a page-sized physical memory region in RAM. It is a
 *   container where a data page may be placed. It is always referred to by
 *   physical address. We have a convention of using uintptr_t for physical
 *   addresses. We instantiate a struct k_mem_page_frame to store metadata for
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
struct k_mem_page_frame k_mem_page_frames[K_MEM_NUM_PAGE_FRAMES];

#ifdef CONFIG_ASSERT
/* Indicator that k_mem_page_frames has been initialized, many of these APIs do
 * not work before POST_KERNEL
 */
bool z_mem_page_frames_initialized;
#endif /* CONFIG_ASSERT */

/* Add colors to page table dumps to indicate mapping type */
#define COLOR_PAGE_FRAMES	1

#if COLOR_PAGE_FRAMES
#define ANSI_DEFAULT "\x1B" "[0m"
#define ANSI_RED     "\x1B" "[1;31m"
#define ANSI_GREEN   "\x1B" "[1;32m"
#define ANSI_YELLOW  "\x1B" "[1;33m"
#define ANSI_BLUE    "\x1B" "[1;34m"
#define ANSI_MAGENTA "\x1B" "[1;35m"
#define ANSI_CYAN    "\x1B" "[1;36m"
#define ANSI_GREY    "\x1B" "[1;90m"

#define COLOR(x)	printk(_CONCAT(ANSI_, x))
#else
#define COLOR(x)	do { } while (false)
#endif /* COLOR_PAGE_FRAMES */

/* LCOV_EXCL_START */
static void page_frame_dump(struct k_mem_page_frame *pf)
{
	if (k_mem_page_frame_is_free(pf)) {
		COLOR(GREY);
		printk("-");
	} else if (k_mem_page_frame_is_reserved(pf)) {
		COLOR(CYAN);
		printk("R");
	} else if (k_mem_page_frame_is_busy(pf)) {
		COLOR(MAGENTA);
		printk("B");
	} else if (k_mem_page_frame_is_pinned(pf)) {
		COLOR(YELLOW);
		printk("P");
	} else if (k_mem_page_frame_is_available(pf)) {
		COLOR(GREY);
		printk(".");
	} else if (k_mem_page_frame_is_mapped(pf)) {
		COLOR(DEFAULT);
		printk("M");
	} else {
		COLOR(RED);
		printk("?");
	}
}

void k_mem_page_frames_dump(void)
{
	int column = 0;

	__ASSERT(z_mem_page_frames_initialized, "%s called too early", __func__);
	printk("Physical memory from 0x%lx to 0x%lx\n",
	       K_MEM_PHYS_RAM_START, K_MEM_PHYS_RAM_END);

	for (int i = 0; i < K_MEM_NUM_PAGE_FRAMES; i++) {
		struct k_mem_page_frame *pf = &k_mem_page_frames[i];

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
/* LCOV_EXCL_STOP */

#define VIRT_FOREACH(_base, _size, _pos) \
	for ((_pos) = (_base); \
	     (_pos) < ((uint8_t *)(_base) + (_size)); (_pos) += CONFIG_MMU_PAGE_SIZE)

#define PHYS_FOREACH(_base, _size, _pos) \
	for ((_pos) = (_base); \
	     (_pos) < ((uintptr_t)(_base) + (_size)); (_pos) += CONFIG_MMU_PAGE_SIZE)


/*
 * Virtual address space management
 *
 * Call all of these functions with z_mm_lock held.
 *
 * Overall virtual memory map: When the kernel starts, it resides in
 * virtual memory in the region K_MEM_KERNEL_VIRT_START to
 * K_MEM_KERNEL_VIRT_END. Unused virtual memory past this, up to the limit
 * noted by CONFIG_KERNEL_VM_SIZE may be used for runtime memory mappings.
 *
 * If CONFIG_ARCH_MAPS_ALL_RAM is set, we do not just map the kernel image,
 * but have a mapping for all RAM in place. This is for special architectural
 * purposes and does not otherwise affect page frame accounting or flags;
 * the only guarantee is that such RAM mapping outside of the Zephyr image
 * won't be disturbed by subsequent memory mapping calls.
 *
 * +--------------+ <- K_MEM_VIRT_RAM_START
 * | Undefined VM | <- May contain ancillary regions like x86_64's locore
 * +--------------+ <- K_MEM_KERNEL_VIRT_START (often == K_MEM_VIRT_RAM_START)
 * | Mapping for  |
 * | main kernel  |
 * | image        |
 * |		  |
 * |		  |
 * +--------------+ <- K_MEM_VM_FREE_START
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
 * | Reserved     | <- special purpose virtual page(s) of size K_MEM_VM_RESERVED
 * +--------------+ <- K_MEM_VIRT_RAM_END
 */

/* Bitmap of virtual addresses where one bit corresponds to one page.
 * This is being used for virt_region_alloc() to figure out which
 * region of virtual addresses can be used for memory mapping.
 *
 * Note that bit #0 is the highest address so that allocation is
 * done in reverse from highest address.
 */
SYS_BITARRAY_DEFINE_STATIC(virt_region_bitmap,
			   CONFIG_KERNEL_VM_SIZE / CONFIG_MMU_PAGE_SIZE);

static bool virt_region_inited;

#define Z_VIRT_REGION_START_ADDR	K_MEM_VM_FREE_START
#define Z_VIRT_REGION_END_ADDR		(K_MEM_VIRT_RAM_END - K_MEM_VM_RESERVED)

static inline uintptr_t virt_from_bitmap_offset(size_t offset, size_t size)
{
	return POINTER_TO_UINT(K_MEM_VIRT_RAM_END)
	       - (offset * CONFIG_MMU_PAGE_SIZE) - size;
}

static inline size_t virt_to_bitmap_offset(void *vaddr, size_t size)
{
	return (POINTER_TO_UINT(K_MEM_VIRT_RAM_END)
		- POINTER_TO_UINT(vaddr) - size) / CONFIG_MMU_PAGE_SIZE;
}

static void virt_region_init(void)
{
	size_t offset, num_bits;

	/* There are regions where we should never map via
	 * k_mem_map() and k_mem_map_phys_bare(). Mark them as
	 * already allocated so they will never be used.
	 */

	if (K_MEM_VM_RESERVED > 0) {
		/* Mark reserved region at end of virtual address space */
		num_bits = K_MEM_VM_RESERVED / CONFIG_MMU_PAGE_SIZE;
		(void)sys_bitarray_set_region(&virt_region_bitmap,
					      num_bits, 0);
	}

	/* Mark all bits up to Z_FREE_VM_START as allocated */
	num_bits = POINTER_TO_UINT(K_MEM_VM_FREE_START)
		   - POINTER_TO_UINT(K_MEM_VIRT_RAM_START);
	offset = virt_to_bitmap_offset(K_MEM_VIRT_RAM_START, num_bits);
	num_bits /= CONFIG_MMU_PAGE_SIZE;
	(void)sys_bitarray_set_region(&virt_region_bitmap,
				      num_bits, offset);

	virt_region_inited = true;
}

static void virt_region_free(void *vaddr, size_t size)
{
	size_t offset, num_bits;
	uint8_t *vaddr_u8 = (uint8_t *)vaddr;

	if (unlikely(!virt_region_inited)) {
		virt_region_init();
	}

#ifndef CONFIG_KERNEL_DIRECT_MAP
	/* Without the need to support K_MEM_DIRECT_MAP, the region must be
	 * able to be represented in the bitmap. So this case is
	 * simple.
	 */

	__ASSERT((vaddr_u8 >= Z_VIRT_REGION_START_ADDR)
		 && ((vaddr_u8 + size - 1) < Z_VIRT_REGION_END_ADDR),
		 "invalid virtual address region %p (%zu)", vaddr_u8, size);
	if (!((vaddr_u8 >= Z_VIRT_REGION_START_ADDR)
	      && ((vaddr_u8 + size - 1) < Z_VIRT_REGION_END_ADDR))) {
		return;
	}

	offset = virt_to_bitmap_offset(vaddr, size);
	num_bits = size / CONFIG_MMU_PAGE_SIZE;
	(void)sys_bitarray_free(&virt_region_bitmap, num_bits, offset);
#else /* !CONFIG_KERNEL_DIRECT_MAP */
	/* With K_MEM_DIRECT_MAP, the region can be outside of the virtual
	 * memory space, wholly within it, or overlap partially.
	 * So additional processing is needed to make sure we only
	 * mark the pages within the bitmap.
	 */
	if (((vaddr_u8 >= Z_VIRT_REGION_START_ADDR) &&
	     (vaddr_u8 < Z_VIRT_REGION_END_ADDR)) ||
	    (((vaddr_u8 + size - 1) >= Z_VIRT_REGION_START_ADDR) &&
	     ((vaddr_u8 + size - 1) < Z_VIRT_REGION_END_ADDR))) {
		uint8_t *adjusted_start = max(vaddr_u8, Z_VIRT_REGION_START_ADDR);
		uint8_t *adjusted_end = min(vaddr_u8 + size,
					    Z_VIRT_REGION_END_ADDR);
		size_t adjusted_sz = adjusted_end - adjusted_start;

		offset = virt_to_bitmap_offset(adjusted_start, adjusted_sz);
		num_bits = adjusted_sz / CONFIG_MMU_PAGE_SIZE;
		(void)sys_bitarray_free(&virt_region_bitmap, num_bits, offset);
	}
#endif /* !CONFIG_KERNEL_DIRECT_MAP */
}

static void *virt_region_alloc(size_t size, size_t align)
{
	uintptr_t dest_addr;
	size_t alloc_size;
	size_t offset;
	size_t num_bits;
	int ret;

	if (unlikely(!virt_region_inited)) {
		virt_region_init();
	}

	/* Possibly request more pages to ensure we can get an aligned virtual address */
	num_bits = (size + align - CONFIG_MMU_PAGE_SIZE) / CONFIG_MMU_PAGE_SIZE;
	alloc_size = num_bits * CONFIG_MMU_PAGE_SIZE;
	ret = sys_bitarray_alloc(&virt_region_bitmap, num_bits, &offset);
	if (ret != 0) {
		LOG_ERR("insufficient virtual address space (requested %zu)",
			size);
		return NULL;
	}

	/* Remember that bit #0 in bitmap corresponds to the highest
	 * virtual address. So here we need to go downwards (backwards?)
	 * to get the starting address of the allocated region.
	 */
	dest_addr = virt_from_bitmap_offset(offset, alloc_size);

	if (alloc_size > size) {
		uintptr_t aligned_dest_addr = ROUND_UP(dest_addr, align);

		/* Here is the memory organization when trying to get an aligned
		 * virtual address:
		 *
		 * +--------------+ <- K_MEM_VIRT_RAM_START
		 * | Undefined VM |
		 * +--------------+ <- K_MEM_KERNEL_VIRT_START (often == K_MEM_VIRT_RAM_START)
		 * | Mapping for  |
		 * | main kernel  |
		 * | image        |
		 * |		  |
		 * |		  |
		 * +--------------+ <- K_MEM_VM_FREE_START
		 * | ...          |
		 * +==============+ <- dest_addr
		 * | Unused       |
		 * |..............| <- aligned_dest_addr
		 * |              |
		 * | Aligned      |
		 * | Mapping      |
		 * |              |
		 * |..............| <- aligned_dest_addr + size
		 * | Unused       |
		 * +==============+ <- offset from K_MEM_VIRT_RAM_END == dest_addr + alloc_size
		 * | ...          |
		 * +--------------+
		 * | Mapping      |
		 * +--------------+
		 * | Reserved     |
		 * +--------------+ <- K_MEM_VIRT_RAM_END
		 */

		/* Free the two unused regions */
		virt_region_free(UINT_TO_POINTER(dest_addr),
				 aligned_dest_addr - dest_addr);
		if (((dest_addr + alloc_size) - (aligned_dest_addr + size)) > 0) {
			virt_region_free(UINT_TO_POINTER(aligned_dest_addr + size),
					 (dest_addr + alloc_size) - (aligned_dest_addr + size));
		}

		dest_addr = aligned_dest_addr;
	}

	/* Need to make sure this does not step into kernel memory */
	if (dest_addr < POINTER_TO_UINT(Z_VIRT_REGION_START_ADDR)) {
		(void)sys_bitarray_free(&virt_region_bitmap, num_bits, offset);
		return NULL;
	}

	return UINT_TO_POINTER(dest_addr);
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
BUILD_ASSERT(SYS_SFLIST_FLAG_BITS >= 1, "MMU free-page list needs one sflist flag bit");

static sys_sflist_t free_page_frame_list;

/* Number of unused and available free page frames.
 * This information may go stale immediately.
 */
static size_t z_free_page_count;

#define PF_ASSERT(pf, expr, fmt, ...) \
	__ASSERT(expr, "page frame 0x%lx: " fmt, k_mem_page_frame_to_phys(pf), \
		 ##__VA_ARGS__)

/* Get an unused page frame. don't care which one, or NULL if there are none */
struct k_mem_page_frame *z_mem_page_frame_get_free(void)
{
	sys_sfnode_t *node;
	struct k_mem_page_frame *pf = NULL;

	node = sys_sflist_get(&free_page_frame_list);
	if (node != NULL) {
		z_free_page_count--;
		pf = CONTAINER_OF(node, struct k_mem_page_frame, node);
		PF_ASSERT(pf, k_mem_page_frame_is_free(pf),
			 "on free list but not free");
		pf->va_and_flags = 0;
	}

	return pf;
}

/* Release a page frame back into the list of free pages */
static void free_page_frame_list_put(struct k_mem_page_frame *pf)
{
	PF_ASSERT(pf, k_mem_page_frame_is_available(pf),
		 "unavailable page put on free list");

	sys_sfnode_init(&pf->node, K_MEM_PAGE_FRAME_FREE);
	sys_sflist_append(&free_page_frame_list, &pf->node);
	z_free_page_count++;
}

static void free_page_frame_list_init(void)
{
	sys_sflist_init(&free_page_frame_list);
}

void z_mem_page_frame_free_locked(struct k_mem_page_frame *pf)
{
	pf->va_and_flags = 0;
	free_page_frame_list_put(pf);
}

/*
 * Memory Mapping
 */

/* Called after the frame is mapped in the arch layer, to update our
 * local ontology (and do some assertions while we're at it)
 */
void z_mem_page_frame_mapped_set(struct k_mem_page_frame *pf, void *addr)
{
	PF_ASSERT(pf, !k_mem_page_frame_is_free(pf),
		  "attempted to map a page frame on the free list");
	PF_ASSERT(pf, !k_mem_page_frame_is_reserved(pf),
		  "attempted to map a reserved page frame");

	/* We do allow multiple mappings for pinned page frames
	 * since we will never need to reverse map them.
	 * This is uncommon, use-cases are for things like the
	 * Zephyr equivalent of VSDOs
	 */
	PF_ASSERT(pf, !k_mem_page_frame_is_mapped(pf) || k_mem_page_frame_is_pinned(pf),
		 "non-pinned and already mapped to %p",
		 k_mem_page_frame_to_virt(pf));

	uintptr_t flags_mask = CONFIG_MMU_PAGE_SIZE - 1;
	uintptr_t va = (uintptr_t)addr & ~flags_mask;

	pf->va_and_flags &= flags_mask;
	pf->va_and_flags |= va | K_MEM_PAGE_FRAME_MAPPED;
}

/* LCOV_EXCL_START */
/* Go through page frames to find the physical address mapped
 * by a virtual address.
 *
 * @param[in]  virt Virtual Address
 * @param[out] phys Physical address mapped to the input virtual address
 *                  if such mapping exists.
 *
 * @retval 0 if mapping is found and valid
 * @retval -EFAULT if virtual address is not mapped
 */
static int virt_to_page_frame(void *virt, uintptr_t *phys)
{
	uintptr_t paddr;
	struct k_mem_page_frame *pf;
	int ret = -EFAULT;

	K_MEM_PAGE_FRAME_FOREACH(paddr, pf) {
		if (k_mem_page_frame_is_mapped(pf)) {
			if (virt == k_mem_page_frame_to_virt(pf)) {
				ret = 0;
				if (phys != NULL) {
					*phys = k_mem_page_frame_to_phys(pf);
				}
				break;
			}
		}
	}

	return ret;
}
/* LCOV_EXCL_STOP */

__weak FUNC_ALIAS(virt_to_page_frame, arch_page_phys_get, int);

/* Allocate a free page frame, and map it to a specified virtual address
 *
 * TODO: Add optional support for copy-on-write mappings to a zero page instead
 * of allocating, in which case page frames will be allocated lazily as
 * the mappings to the zero page get touched. This will avoid expensive
 * page-ins as memory is mapped and physical RAM or backing store storage will
 * not be used if the mapped memory is unused. The cost is an empty physical
 * page of zeroes.
 *
 * @param addr Virtual address to map the page frame to
 * @param flags Mapping flags
 * @param[out] recovered Set to false if a failed mapping could not be undone,
 *                       which means the page may still be mapped and its page
 *                       frame cannot be reused.
 *
 * @retval 0 On success
 * @retval -errno On failure
 */
static int map_anon_page(void *addr, uint32_t flags, bool *recovered)
{
	struct k_mem_page_frame *pf;
	uintptr_t phys;
	bool lock = (flags & K_MEM_MAP_LOCK) != 0U;
	int ret;

	*recovered = true;

	pf = z_mem_page_frame_get_free();
	if (pf == NULL) {
		pf = z_paging_frame_evict_locked();
		if (pf == NULL) {
			return -ENOMEM;
		}
	}

	phys = k_mem_page_frame_to_phys(pf);
	ret = arch_mem_map(addr, phys, CONFIG_MMU_PAGE_SIZE, flags);
	__ASSERT(ret == 0, "%s: arch_mem_map failed %d\n", __func__, ret);
	if (ret != 0) {
#ifdef CONFIG_USERSPACE
		/* The mapping above may have already succeeded in some
		 * memory domains but failing in the middle. So we need to
		 * unmap those first before returning the page frame back
		 * to the list.
		 */
		int unmap_ret = arch_mem_unmap(addr, CONFIG_MMU_PAGE_SIZE);

		__ASSERT(unmap_ret == 0, "%s: cannot undo failed mapping %d\n", __func__,
			 unmap_ret);

		if (unmap_ret != 0) {
			/* The page may still be mapped in some memory domains,
			 * so the page frame cannot be given back to the free
			 * list for reuse.
			 */
			*recovered = false;
			return ret;
		}
#endif

		/* Mapping failed: give the page frame back before bailing out. */
		z_mem_page_frame_free_locked(pf);
		return ret;
	}

	if (lock) {
		k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_PINNED);
	}
	z_mem_page_frame_mapped_set(pf, addr);
	if (!lock) {
		z_paging_eviction_track(pf);
	}

	LOG_DBG("memory mapping anon page %p -> 0x%lx", addr, phys);

	return 0;
}

/**
 * Undo pages previously established by map_anon_page().
 *
 * Only used to roll back a partial k_mem_map_phys_guard() failure
 * via the anonymous memory path, the pages are known to be
 * currently mapped and not paged out.
 *
 * @param virt Start virtual address to unmap pages
 * @param sz Size of memory region to be unmapped
 *
 * @retval 0 On success
 * @retval -errno On failure
 */
static int unmap_anon_pages(void *virt, size_t sz)
{
	uint8_t *pos;
	uintptr_t phys;
	struct k_mem_page_frame *pf;
	int ret = 0;

	VIRT_FOREACH(virt, sz, pos) {
		bool paged_out;

		ret = z_paging_unmap_page_locate(pos, &phys, &paged_out);
		if (paged_out) {
			if (ret != 0) {
				goto unmap_anon_out;
			}

			continue;
		}
		__ASSERT(ret == 0, "%s: cannot unmap an unmapped address %p", __func__, pos);
		if (ret != 0) {
			/* Found an address not mapped. Do not continue. */
			goto unmap_anon_out;
		}

		__ASSERT(k_mem_is_page_frame(phys), "%s: 0x%lx is not a page frame", __func__,
			 phys);
		if (!k_mem_is_page_frame(phys)) {
			/* Physical address has no corresponding page frame
			 * description in the page frame array.
			 * This should not happen. Do not continue.
			 */
			goto unmap_anon_out;
		}

		/* Grab the corresponding page frame from physical address */
		pf = k_mem_phys_to_page_frame(phys);

		__ASSERT(k_mem_page_frame_is_mapped(pf), "%s: 0x%lx is not a mapped page frame",
			 __func__, phys);
		if (!k_mem_page_frame_is_mapped(pf)) {
			/* Page frame is not marked mapped.
			 * This should not happen. Do not continue.
			 */
			goto unmap_anon_out;
		}

		ret = arch_mem_unmap(pos, CONFIG_MMU_PAGE_SIZE);
		__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__, ret);
		if (ret != 0) {
			/* Fail to unmap. Must bail without releasing any resources
			 * as the page is still mapped.
			 */
			goto unmap_anon_out;
		}

		if (!k_mem_page_frame_is_pinned(pf)) {
			z_paging_eviction_untrack(pf);
		}

		/* Put the page frame back into free list */
		z_mem_page_frame_free_locked(pf);
	}

unmap_anon_out:
	return ret;
}

#ifdef CONFIG_DEMAND_MAPPING
/**
 * Undo demand mapped pages previously established by map_anon_page().
 *
 * Only used to roll back a partial k_mem_map_phys_guard() failure
 * via the demand mapped path where pages are mapped with flags
 * K_MEM_MAP_UNPAGED.
 *
 * @param virt Start virtual address to umap pages
 * @param sz Size of memory region to be unmapped
 *
 * @retval true When able to unmap all pages.
 * @retval false When fails to unmap some pages.
 */
static bool undo_demand_mapped_pages(void *virt, size_t sz)
{
	uint8_t *undo_pos;
	bool success = true;
	int ret;

	/* Undo the pages mapped before the failure. */
	VIRT_FOREACH(virt, sz, undo_pos) {
		ret = arch_mem_unmap(undo_pos, CONFIG_MMU_PAGE_SIZE);
		__ASSERT(ret == 0, "%s: cannot undo mapped pages %d", __func__, ret);

		/* Failed to undo one mapped page. Though we still
		 * want to continue unmapping the remaining mapped
		 * pages to reclaim them.
		 */
		if (ret != 0) {
			success = false;
		}
	}

	return success;
}
#endif

void *k_mem_map_phys_guard(uintptr_t phys, size_t size, uint32_t flags, bool is_anon)
{
	uint8_t *dst;
	uint8_t *region;
	size_t total_size;
	int ret;
	k_spinlock_key_t key;
	uint8_t *pos;
	bool uninit = (flags & K_MEM_MAP_UNINIT) != 0U;

	__ASSERT(!is_anon || (is_anon && z_mem_page_frames_initialized), "%s called too early",
		 __func__);
	__ASSERT((flags & K_MEM_CACHE_MASK) == 0U,
		 "%s does not support explicit cache settings", __func__);

	if (((flags & K_MEM_PERM_USER) != 0U) &&
	    ((flags & K_MEM_MAP_UNINIT) != 0U)) {
		LOG_ERR("user access to anonymous uninitialized pages is forbidden");
		return NULL;
	}
	if ((size % CONFIG_MMU_PAGE_SIZE) != 0U) {
		LOG_ERR("unaligned size %zu passed to %s", size, __func__);
		return NULL;
	}
	if (size == 0) {
		LOG_ERR("zero sized memory mapping");
		return NULL;
	}

	/* Need extra for the guard pages (before and after) which we
	 * won't map.
	 */
	if (size_add_overflow(size, CONFIG_MMU_PAGE_SIZE * 2, &total_size)) {
		LOG_ERR("too large size %zu passed to %s", size, __func__);
		return NULL;
	}

	key = k_spin_lock(&z_mm_lock);

	region = virt_region_alloc(total_size, CONFIG_MMU_PAGE_SIZE);
	if (region == NULL) {
		/* Address space has no free region */
		dst = NULL;
		goto out;
	}

	/* Unmap both guard pages to make sure accessing them
	 * will generate fault.
	 *
	 * Note that a failed unmap may have left the guard page mapped in
	 * some memory domains, so the virtual region cannot be freed for
	 * reuse.
	 */
	ret = arch_mem_unmap(region, CONFIG_MMU_PAGE_SIZE);
	__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__, ret);
	if (ret != 0) {
		dst = NULL;
		goto out;
	}

	ret = arch_mem_unmap(region + CONFIG_MMU_PAGE_SIZE + size, CONFIG_MMU_PAGE_SIZE);
	__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__, ret);
	if (ret != 0) {
		dst = NULL;
		goto out;
	}

	/* Skip over the "before" guard page in returned address. */
	dst = region + CONFIG_MMU_PAGE_SIZE;

	if (is_anon) {
		/* Mapping from anonymous memory */
		flags |= K_MEM_CACHE_WB;
#ifdef CONFIG_DEMAND_MAPPING
		if ((flags & K_MEM_MAP_LOCK) == 0) {
			flags |= K_MEM_MAP_UNPAGED;
			VIRT_FOREACH(dst, size, pos) {
				ret = arch_mem_map(pos,
						   uninit ? ARCH_UNPAGED_ANON_UNINIT
							  : ARCH_UNPAGED_ANON_ZERO,
						   CONFIG_MMU_PAGE_SIZE, flags);
				__ASSERT(ret == 0, "%s: arch_mem_map failed %d\n", __func__, ret);
				if (ret != 0) {
					size_t unmap_sz = pos - dst;

					if (IS_ENABLED(CONFIG_USERSPACE)) {
						/* Note that it is unmapping an extra page due to
						 * the possibility that the mapping above has
						 * already updated some memory domains but fail in
						 * the middle. So we need to include the failed
						 * page to unmap those that have been mapped so
						 * far.
						 */
						unmap_sz += CONFIG_MMU_PAGE_SIZE;
					}

					bool success = undo_demand_mapped_pages(dst, unmap_sz);

					dst = NULL;

					if (success) {
						goto fail;
					} else {
						/* Cannot fully recover from mapping
						 * failure. We cannot free the allocated
						 * virtual memory region since some of
						 * the page may still be mapped.
						 */
						goto out;
					}
				}
			}
			LOG_DBG("memory mapping anon pages %p to %p unpaged", dst, pos-1);
			/* skip the memset() below */
			uninit = true;
		} else
#endif
		{
			VIRT_FOREACH(dst, size, pos) {
				bool recovered;

				ret = map_anon_page(pos, flags, &recovered);

				if (ret != 0) {
					int unmap_ret = unmap_anon_pages(dst, pos - dst);

					dst = NULL;

					if ((unmap_ret == 0) && recovered) {
						goto fail;
					} else {
						/* Cannot fully recover from mapping
						 * failure. We cannot free the allocated
						 * virtual memory region since some of
						 * the page may still be mapped.
						 */
						goto out;
					}
				}
			}
		}
	} else {
		/* Mapping known physical memory. */
		ret = arch_mem_map(dst, phys, size, flags);
		__ASSERT(ret == 0, "%s: arch_mem_map failed %d\n", __func__, ret);
		if (ret != 0) {
			/* Clean up any partial mapping left behind. */
			ret = arch_mem_unmap(dst, size);
			__ASSERT(ret == 0, "%s: cannot undo mapped pages %d", __func__, ret);

			dst = NULL;

			if (ret != 0) {
				/* Cannot fully undo all the mappings. There might still be pages
				 * mapped. So we cannot free the allocated virtual memory region.
				 */
				goto out;
			} else {
				goto fail;
			}
		}
	}

out:
	k_spin_unlock(&z_mm_lock, key);

	if (dst != NULL && !uninit) {
		/* If we later implement mappings to a copy-on-write
		 * zero page, won't need this step
		 */
		memset(dst, 0, size);
	}

	return dst;

fail:
	/* Release the whole region, including the guard pages. */
	virt_region_free(region, total_size);

	goto out;
}

void k_mem_unmap_phys_guard(void *addr, size_t size, bool is_anon)
{
	uint8_t *pos;
	k_spinlock_key_t key;
	size_t total_size;
	int ret;

	/* Need space for the "before" guard page */
	__ASSERT_NO_MSG(POINTER_TO_UINT(addr) >= CONFIG_MMU_PAGE_SIZE);

	/* Make sure address range is still valid after accounting
	 * for two guard pages.
	 */
	pos = (uint8_t *)addr - CONFIG_MMU_PAGE_SIZE;
	k_mem_assert_virtual_region(pos, size + (CONFIG_MMU_PAGE_SIZE * 2));

	key = k_spin_lock(&z_mm_lock);

	/* Check if both guard pages are unmapped.
	 * Bail if not, as this is probably a region not mapped
	 * using k_mem_map().
	 */
	pos = addr;
	ret = arch_page_phys_get(pos - CONFIG_MMU_PAGE_SIZE, NULL);
	__ASSERT(ret != 0,
		 "%s: cannot find preceding guard page for (%p, %zu)",
		 __func__, addr, size);
	if (ret == 0) {
		goto out;
	}

	ret = arch_page_phys_get(pos + size, NULL);
	__ASSERT(ret != 0,
		 "%s: cannot find succeeding guard page for (%p, %zu)",
		 __func__, addr, size);
	if (ret == 0) {
		goto out;
	}

	if (is_anon) {
		/* Unmapping anonymous memory */
		ret = unmap_anon_pages(addr, size);
		__ASSERT(ret == 0, "%s: unmap_anon_pages() returned %d", __func__, ret);
		if (ret != 0) {
			/* Fail to unmap. Must bail without releasing any resources
			 * as some pages are still be mapped.
			 */
			goto out;
		}
	} else {
		/*
		 * Unmapping previous mapped memory with specific physical address.
		 *
		 * Note that we don't have to unmap the guard pages, as they should
		 * have been unmapped. We just need to unmapped the in-between
		 * region [addr, (addr + size)).
		 */
		ret = arch_mem_unmap(addr, size);
		__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__, ret);
		if (ret != 0) {
			/* Fail to unmap. Must bail without releasing any resources
			 * as some pages are still be mapped.
			 */
			goto out;
		}
	}

	/* There are guard pages just before and after the mapped
	 * region. So we also need to free them from the bitmap.
	 */
	pos = (uint8_t *)addr - CONFIG_MMU_PAGE_SIZE;
	total_size = size + (CONFIG_MMU_PAGE_SIZE * 2);
	virt_region_free(pos, total_size);

out:
	k_spin_unlock(&z_mm_lock, key);
}

int k_mem_update_flags(void *addr, size_t size, uint32_t flags)
{
	uintptr_t phys;
	k_spinlock_key_t key;
	int ret;

	k_mem_assert_virtual_region(addr, size);

	key = k_spin_lock(&z_mm_lock);

	/*
	 * We can achieve desired result without explicit architecture support
	 * by unmapping and remapping the same physical memory using new flags.
	 */

	ret = arch_page_phys_get(addr, &phys);
	if (ret < 0) {
		goto out;
	}

	/* TODO: detect and handle paged-out memory as well */

	ret = arch_mem_unmap(addr, size);
	__ASSERT(ret == 0, "%s: arch_mem_unmap failed %d\n", __func__, ret);
	if (ret != 0) {
		goto out;
	}

	ret = arch_mem_map(addr, phys, size, flags);
	__ASSERT(ret == 0, "%s: arch_mem_map failed %d\n", __func__, ret);

out:
	k_spin_unlock(&z_mm_lock, key);
	return ret;
}

size_t k_mem_free_get(void)
{
	size_t ret;
	k_spinlock_key_t key;

	__ASSERT(z_mem_page_frames_initialized, "%s called too early", __func__);

	key = k_spin_lock(&z_mm_lock);
	if (z_free_page_count > z_paging_frames_reserved()) {
		ret = z_free_page_count - z_paging_frames_reserved();
	} else {
		ret = 0;
	}
	k_spin_unlock(&z_mm_lock, key);

	return ret * (size_t)CONFIG_MMU_PAGE_SIZE;
}

/* Get the default virtual region alignment, here the default MMU page size
 *
 * @param[in] phys Physical address of region to be mapped, aligned to MMU_PAGE_SIZE
 * @param[in] size Size of region to be mapped, aligned to MMU_PAGE_SIZE
 *
 * @return alignment to apply on the virtual address of this region
 */
static size_t virt_region_align(uintptr_t phys, size_t size)
{
	ARG_UNUSED(phys);
	ARG_UNUSED(size);

	return CONFIG_MMU_PAGE_SIZE;
}

__weak FUNC_ALIAS(virt_region_align, arch_virt_region_align, size_t);

/* This may be called from arch early boot code before z_cstart() is invoked.
 * Data will be copied and BSS zeroed, but this must not rely on any
 * initialization functions being called prior to work correctly.
 */
void k_mem_map_phys_bare(uint8_t **virt_ptr, uintptr_t phys, size_t size, uint32_t flags)
{
	uintptr_t aligned_phys, addr_offset;
	size_t aligned_size, align_boundary;
	k_spinlock_key_t key;
	uint8_t *dest_addr;
	size_t num_bits;
	size_t offset;
	int ret;

#ifndef CONFIG_KERNEL_DIRECT_MAP
	__ASSERT(!(flags & K_MEM_DIRECT_MAP), "The direct-map is not enabled");
#endif /* CONFIG_KERNEL_DIRECT_MAP */
	addr_offset = k_mem_region_align(&aligned_phys, &aligned_size,
					 phys, size,
					 CONFIG_MMU_PAGE_SIZE);
	__ASSERT(aligned_size != 0U, "0-length mapping at 0x%lx", aligned_phys);
	__ASSERT(aligned_size - 1 <= (UINTPTR_MAX - aligned_phys),
		 "wraparound for physical address 0x%lx (size %zu)",
		 aligned_phys, aligned_size);

	align_boundary = arch_virt_region_align(aligned_phys, aligned_size);

	key = k_spin_lock(&z_mm_lock);

	if (IS_ENABLED(CONFIG_KERNEL_DIRECT_MAP) &&
	    (flags & K_MEM_DIRECT_MAP)) {
		dest_addr = (uint8_t *)aligned_phys;

		/* Mark the region of virtual memory bitmap as used
		 * if the region overlaps the virtual memory space.
		 *
		 * Basically if either end of region is within
		 * virtual memory space, we need to mark the bits.
		 */

		if (IN_RANGE(aligned_phys,
			      (uintptr_t)K_MEM_VIRT_RAM_START,
			      (uintptr_t)(K_MEM_VIRT_RAM_END - 1)) ||
		    IN_RANGE(aligned_phys + aligned_size - 1,
			      (uintptr_t)K_MEM_VIRT_RAM_START,
			      (uintptr_t)(K_MEM_VIRT_RAM_END - 1))) {
			uint8_t *adjusted_start = max(dest_addr, K_MEM_VIRT_RAM_START);
			uint8_t *adjusted_end = min(dest_addr + aligned_size,
						    K_MEM_VIRT_RAM_END);
			size_t adjusted_sz = adjusted_end - adjusted_start;

			num_bits = adjusted_sz / CONFIG_MMU_PAGE_SIZE;
			offset = virt_to_bitmap_offset(adjusted_start, adjusted_sz);
			if (sys_bitarray_test_and_set_region(
			    &virt_region_bitmap, num_bits, offset, true)) {
				goto fail;
			}
		}
	} else {
		/* Obtain an appropriately sized chunk of virtual memory */
		dest_addr = virt_region_alloc(aligned_size, align_boundary);
		if (!dest_addr) {
			goto fail;
		}
	}

	/* If this fails there's something amiss with virt_region_get */
	__ASSERT((uintptr_t)dest_addr <
		 ((uintptr_t)dest_addr + (size - 1)),
		 "wraparound for virtual address %p (size %zu)",
		 dest_addr, size);

	LOG_DBG("arch_mem_map(%p, 0x%lx, %zu, %x) offset %lu", (void *)dest_addr,
		aligned_phys, aligned_size, flags, addr_offset);

	ret = arch_mem_map(dest_addr, aligned_phys, aligned_size, flags);
	__ASSERT(ret == 0, "%s: arch_mem_map failed %d\n", __func__, ret);
	if (ret != 0) {
		goto fail;
	}
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

void k_mem_unmap_phys_bare(uint8_t *virt, size_t size)
{
	uintptr_t aligned_virt, addr_offset;
	size_t aligned_size;
	k_spinlock_key_t key;
	int ret;

	addr_offset = k_mem_region_align(&aligned_virt, &aligned_size,
					 POINTER_TO_UINT(virt), size,
					 CONFIG_MMU_PAGE_SIZE);
	__ASSERT(aligned_size != 0U, "0-length mapping at 0x%lx", aligned_virt);
	__ASSERT(aligned_size - 1 <= (UINTPTR_MAX - aligned_virt),
		 "wraparound for virtual address 0x%lx (size %zu)",
		 aligned_virt, aligned_size);

	key = k_spin_lock(&z_mm_lock);

	LOG_DBG("arch_mem_unmap(0x%lx, %zu) offset %lu",
		aligned_virt, aligned_size, addr_offset);

	ret = arch_mem_unmap(UINT_TO_POINTER(aligned_virt), aligned_size);
	__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__, ret);
	if (ret != 0) {
		/* Fail to unmap. We cannot free the virtual memory region
		 * as some pages are still be mapped.
		 */
		goto out;
	}

	virt_region_free(UINT_TO_POINTER(aligned_virt), aligned_size);

out:
	k_spin_unlock(&z_mm_lock, key);
}

/*
 * Miscellaneous
 */

size_t k_mem_region_align(uintptr_t *aligned_addr, size_t *aligned_size,
			  uintptr_t addr, size_t size, size_t align)
{
	size_t addr_offset;

	/* The actual mapped region must be page-aligned. Round down the
	 * physical address and pad the region size appropriately
	 */
	*aligned_addr = ROUND_DOWN(addr, align);
	addr_offset = addr - *aligned_addr;
	*aligned_size = ROUND_UP(size + addr_offset, align);

	return addr_offset;
}

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
static void mark_linker_section_pinned(void *start_addr, void *end_addr,
				       bool pin)
{
	struct k_mem_page_frame *pf;
	uint8_t *addr;

	uintptr_t pinned_start = ROUND_DOWN(POINTER_TO_UINT(start_addr),
					    CONFIG_MMU_PAGE_SIZE);
	uintptr_t pinned_end = ROUND_UP(POINTER_TO_UINT(end_addr),
					CONFIG_MMU_PAGE_SIZE);
	size_t pinned_size = pinned_end - pinned_start;

	VIRT_FOREACH(UINT_TO_POINTER(pinned_start), pinned_size, addr)
	{
		pf = k_mem_phys_to_page_frame(K_MEM_BOOT_VIRT_TO_PHYS(addr));
		z_mem_page_frame_mapped_set(pf, addr);

		if (pin) {
			k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_PINNED);
		} else {
			k_mem_page_frame_clear(pf, K_MEM_PAGE_FRAME_PINNED);
			if (k_mem_page_frame_is_evictable(pf)) {
				z_paging_eviction_track(pf);
			}
		}
	}
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_ONDEMAND_SECTION
static void z_paging_ondemand_section_map(void)
{
	uint8_t *addr;
	size_t size;
	uintptr_t location;
	uint32_t flags;
	int ret;

	size = (uintptr_t)lnkr_ondemand_text_size;
	flags = K_MEM_MAP_UNPAGED | K_MEM_PERM_EXEC | K_MEM_CACHE_WB;
	VIRT_FOREACH(lnkr_ondemand_text_start, size, addr) {
		k_mem_paging_backing_store_location_query(addr, &location);

		ret = arch_mem_map(addr, location, CONFIG_MMU_PAGE_SIZE, flags);
		__ASSERT(ret == 0, "arch_mem_map() returned %d", ret);
		if (ret != 0) {
			k_panic();
		}

		sys_bitarray_set_region(&virt_region_bitmap, 1,
					virt_to_bitmap_offset(addr, CONFIG_MMU_PAGE_SIZE));
	}

	size = (uintptr_t)lnkr_ondemand_rodata_size;
	flags = K_MEM_MAP_UNPAGED | K_MEM_CACHE_WB;
	VIRT_FOREACH(lnkr_ondemand_rodata_start, size, addr) {
		k_mem_paging_backing_store_location_query(addr, &location);

		ret = arch_mem_map(addr, location, CONFIG_MMU_PAGE_SIZE, flags);
		__ASSERT(ret == 0, "arch_mem_map() returned %d", ret);
		if (ret != 0) {
			k_panic();
		}

		sys_bitarray_set_region(&virt_region_bitmap, 1,
					virt_to_bitmap_offset(addr, CONFIG_MMU_PAGE_SIZE));
	}
}
#endif /* CONFIG_LINKER_USE_ONDEMAND_SECTION */

void z_mem_manage_init(void)
{
	uintptr_t phys;
	uint8_t *addr;
	struct k_mem_page_frame *pf;
	k_spinlock_key_t key = k_spin_lock(&z_mm_lock);

	free_page_frame_list_init();

	ARG_UNUSED(addr);

#ifdef CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES
	/* If some page frames are unavailable for use as memory, arch
	 * code will mark K_MEM_PAGE_FRAME_RESERVED in their flags
	 */
	arch_reserved_pages_update();
#endif /* CONFIG_ARCH_HAS_RESERVED_PAGE_FRAMES */

	/* The entire Zephyr image is mapped and pinned at boot. Demand
	 * paging applies only to anonymous mappings created via k_mem_map()
	 * and to memory placed in explicit __ondemand_* linker sections;
	 * the kernel image itself is never a candidate for eviction.
	 */
	VIRT_FOREACH(K_MEM_KERNEL_VIRT_START, K_MEM_KERNEL_VIRT_SIZE, addr)
	{
		pf = k_mem_phys_to_page_frame(K_MEM_BOOT_VIRT_TO_PHYS(addr));
		z_mem_page_frame_mapped_set(pf, addr);
		k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_PINNED);
	}

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
	/* Pin the boot section to prevent it from being swapped out during
	 * boot process. Will be un-pinned once boot process completes.
	 */
	mark_linker_section_pinned(lnkr_boot_start, lnkr_boot_end, true);
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

	/* Any remaining pages that aren't mapped, reserved, or pinned get
	 * added to the free pages list
	 */
	K_MEM_PAGE_FRAME_FOREACH(phys, pf) {
		if (k_mem_page_frame_is_available(pf)) {
			free_page_frame_list_put(pf);
		}
	}
	LOG_DBG("free page frames: %zu", z_free_page_count);

	z_paging_init();

#ifdef CONFIG_LINKER_USE_ONDEMAND_SECTION
	z_paging_ondemand_section_map();
#endif

#ifdef CONFIG_ASSERT
	z_mem_page_frames_initialized = true;
#endif /* CONFIG_ASSERT */

	k_spin_unlock(&z_mm_lock, key);
}

void z_mem_manage_boot_finish(void)
{
#ifdef CONFIG_LINKER_USE_BOOT_SECTION
	/* At the end of boot process, unpin the boot sections
	 * as they don't need to be in memory all the time anymore.
	 */
	mark_linker_section_pinned(lnkr_boot_start, lnkr_boot_end, false);
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */
}
