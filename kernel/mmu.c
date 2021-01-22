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
 * virtual memory in the region Z_BOOT_KERNEL_VIRT_START to
 * Z_BOOT_KERNEL_VIRT_END. Unused virtual memory past this, up to the limit
 * noted by CONFIG_KERNEL_VM_SIZE may be used for runtime memory mappings.
 *
 * +--------------+ <- Z_VIRT_ADDR_START
 * | Undefined VM | <- May contain ancillary regions like x86_64's locore
 * +--------------+ <- Z_BOOT_KERNEL_VIRT_START (often == Z_VIRT_ADDR_START)
 * | Mapping for  |
 * | main kernel  |
 * | image        |
 * |		  |
 * |		  |
 * +--------------+ <- Z_BOOT_KERNEL_VIRT_END
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

	if ((mapping_pos - size) < Z_KERNEL_VIRT_END) {
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

/* Allocate a free page frame, and map it to a specified virtual address
 *
 * TODO: Add optional support for copy-on-write mappings to a zero page instead
 * of allocating, in which case page frames will be allocated lazily as
 * the mappings to the zero page get touched.
 */
static int map_anon_page(void *addr, uint32_t flags)
{
	struct z_page_frame *pf;
	uintptr_t phys;
	bool lock = (flags & K_MEM_MAP_LOCK) != 0;

	pf = free_page_frame_list_get();
	if (pf == NULL) {
		return -ENOMEM;
	}

	phys = z_page_frame_to_phys(pf);
	arch_mem_map(addr, phys, CONFIG_MMU_PAGE_SIZE, flags | K_MEM_CACHE_WB);

	if (lock) {
		pf->flags |= Z_PAGE_FRAME_PINNED;
	}
	frame_mapped_set(pf, addr);

	return 0;
}

void *k_mem_map(size_t size, uint32_t flags)
{;
	uint8_t *dst;
	size_t total_size = size;
	int ret;
	k_spinlock_key_t key;
	bool uninit = (flags & K_MEM_MAP_UNINIT) != 0;
	bool guard = (flags & K_MEM_MAP_GUARD) != 0;
	uint8_t *pos;

	__ASSERT(!(((flags & K_MEM_PERM_USER) != 0) && uninit),
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

	if (!uninit) {
		/* If we later implement mappings to a copy-on-write zero
		 * page, won't need this step
		 */
		memset(dst, 0, size);
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

#define VM_OFFSET	 ((CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET) - \
			  CONFIG_SRAM_BASE_ADDRESS)

/* Only applies to boot RAM mappings within the Zephyr image that have never
 * been remapped or paged out. Never use this unless you know exactly what you
 * are doing.
 */
#define BOOT_VIRT_TO_PHYS(virt) ((uintptr_t)(((uint8_t *)virt) + VM_OFFSET))

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
			     BOOT_VIRT_TO_PHYS(kobject_page_begin),
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
		frame_mapped_set(z_phys_to_page_frame(BOOT_VIRT_TO_PHYS(addr)),
				 addr);
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
#if __ASSERT_ON
	page_frames_initialized = true;
#endif
	k_spin_unlock(&z_mm_lock, key);
}
