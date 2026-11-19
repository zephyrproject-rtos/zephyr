/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon kernel linear map
 *
 * Replaces the boot Table map, which is RWX everywhere, with a
 * VM_TRANS_TYPE_LINEAR map that enforces W^X on the kernel image.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <hexagon_vm.h>
#include <hexagon_mmu.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

struct hexagon_size_class {
	uint32_t size_class; /* one of __HVM_LINEAR_SIZE_* */
	uint32_t bytes;
};

static const struct hexagon_size_class hexagon_size_classes[] = {
	{ __HVM_LINEAR_SIZE_16MB, 0x1000000U },
	{ __HVM_LINEAR_SIZE_4MB, 0x400000U },
	{ __HVM_LINEAR_SIZE_1MB, 0x100000U },
	{ __HVM_LINEAR_SIZE_256KB, 0x40000U },
	{ __HVM_LINEAR_SIZE_64KB, 0x10000U },
	{ __HVM_LINEAR_SIZE_16KB, 0x4000U },
	{ __HVM_LINEAR_SIZE_4KB, 0x1000U },
};

/*
 * hexagon_size_classes[] bottoms out at 4KB: HVM's VM_TRANS_TYPE_LINEAR
 * format has no finer granularity. Keep that hardware requirement in
 * sync with the MMU page size used to align regions.
 */
BUILD_ASSERT(CONFIG_MMU_PAGE_SIZE == 0x1000U,
	     "Hexagon linear entries require a 4KB CONFIG_MMU_PAGE_SIZE");

/*
 * Zephyr's own ARCH_STACK_PTR_ALIGN on Hexagon is only 8 bytes, so a
 * thread's stack_info.start is routinely not page-aligned -- round
 * outward to the enclosing page here rather than in
 * hexagon_decompose_region() itself, so that helper's "exact multiple
 * of a size class" invariant stays simple.
 */
void hexagon_align_region_to_page(uintptr_t *addr, size_t *size)
{
	uintptr_t start = ROUND_DOWN(*addr, CONFIG_MMU_PAGE_SIZE);
	uintptr_t end = ROUND_UP(*addr + *size, CONFIG_MMU_PAGE_SIZE);

	*addr = start;
	*size = end - start;
}

/*
 * Greedily decompose [addr, addr+size) into the fewest
 * largest-aligned-size-class-first VM_TRANS_TYPE_LINEAR entries that
 * fit within max_entries, written starting at entries[0]. Returns the
 * number of entries used.
 *
 * Never asserts or panics: a region too irregularly shaped or too
 * large for its budget just loses hardware coverage for the uncovered
 * tail (logged) instead of crashing the kernel. A
 * well-aligned region -- the common case -- needs exactly one entry.
 * Callers must page-align addr/size first (see
 * hexagon_align_region_to_page()): the smallest size class is 4KB, so
 * an unaligned addr can never match any class.
 */
uint32_t hexagon_decompose_region(struct hexagon_linear_entry *entries, uint32_t max_entries,
				  uintptr_t addr, size_t size, uint32_t xwru, uint32_t cache_attr)
{
	uint32_t n = 0;

	while (size > 0) {
		if (n >= max_entries) {
			LOG_ERR("linear map: region %#lx/%#zx exhausted its %u-entry budget",
				(unsigned long)addr, size, max_entries);
			break;
		}

		bool placed = false;

		for (size_t i = 0; i < ARRAY_SIZE(hexagon_size_classes); i++) {
			uint32_t chunk = hexagon_size_classes[i].bytes;

			if (chunk > size || (addr % chunk) != 0) {
				continue;
			}

			hexagon_linear_entry_set(&entries[n], addr, addr,
						 hexagon_size_classes[i].size_class, xwru,
						 cache_attr, 0);
			n++;
			addr += chunk;
			size -= chunk;
			placed = true;
			break;
		}

		if (!placed) {
			LOG_ERR("linear map: region %#lx/%#zx not coverable by any size class",
				(unsigned long)addr, size);
			break;
		}
	}

	return n;
}

/* Matches _setup_page_table's own RAM granule (hvm_event_vectors.S). */
#define HEXAGON_RAM_CHUNK_SIZE 0x400000U

/*
 * Image RAM is mapped one 4KB page per entry, see hexagon_fixed_tail_init().
 * This caps image RAM at 4MB, and a user thread's TLB miss on RAM
 * outside its domain walks linearly through these pages. A page-table
 * (VM_TRANS_TYPE_TABLE) map per domain would lift both limits.
 */
#define HEXAGON_FIXED_TAIL_MAX_RAM_PAGES 1024

/*
 * The gap from image end to the next 4MB boundary needs at most 15
 * entries under greedy decomposition for every 4KB-aligned offset in
 * the chunk. One entry remains as slack.
 */
#define HEXAGON_FIXED_TAIL_POST_IMAGE_ENTRIES 16

/*
 * Text and rodata must be fully covered: an uncovered page faults in
 * kernel mode too. Greedy decomposition uses at most 3 entries per size
 * class on each side of the largest aligned chunk, so 2 * 3 * 6 classes
 * (4KB..4MB) plus one 16MB chunk covers any region below 32MB.
 */
#define HEXAGON_FIXED_TAIL_TEXT_ENTRIES   37
#define HEXAGON_FIXED_TAIL_RODATA_ENTRIES 37

/* The region-size bound the entry budget above is derived from. */
#define HEXAGON_FIXED_TAIL_MAX_REGION_BYTES 0x2000000U

/* The console occupies one 4MB device entry, as in the boot table. */
#if DT_HAS_CHOSEN(zephyr_console)
#define HEXAGON_FIXED_TAIL_CONSOLE_ENTRIES 1
#else
#define HEXAGON_FIXED_TAIL_CONSOLE_ENTRIES 0
#endif

/*
 * The configured H2 window is at most 16MB in 4MB multiples: at most
 * 16MB / 4MB = four entries.
 */
#define HEXAGON_FIXED_TAIL_H2_ENTRIES 4

BUILD_ASSERT((CONFIG_HEXAGON_H2_KERNEL_BASE % 0x400000U) == 0U,
	     "CONFIG_HEXAGON_H2_KERNEL_BASE must be 4MB-aligned");
BUILD_ASSERT((CONFIG_HEXAGON_H2_KERNEL_SIZE % 0x400000U) == 0U &&
	     CONFIG_HEXAGON_H2_KERNEL_SIZE > 0U &&
	     CONFIG_HEXAGON_H2_KERNEL_SIZE <= 0x1000000U,
	     "CONFIG_HEXAGON_H2_KERNEL_SIZE must be a multiple of 4MB, at most 16MB");

/*
 * Text/rodata carve-out entries + image RAM pages + post-image RAM
 * entries + console device + H2-kernel entries + one all-zero terminator.
 */
#define HEXAGON_FIXED_TAIL_ENTRIES                                                               \
	(HEXAGON_FIXED_TAIL_TEXT_ENTRIES + HEXAGON_FIXED_TAIL_RODATA_ENTRIES +                    \
	 HEXAGON_FIXED_TAIL_MAX_RAM_PAGES + HEXAGON_FIXED_TAIL_POST_IMAGE_ENTRIES +              \
	 HEXAGON_FIXED_TAIL_CONSOLE_ENTRIES + HEXAGON_FIXED_TAIL_H2_ENTRIES + 1)

/*
 * Shared tail chained onto hexagon_kernel_map and every memory domain's
 * list (see hexagon_mem_domain_rebuild()). Entries retain the boot table
 * map's addresses and cache attributes.
 *
 * In list order, the tail grants:
 *
 *                     U   R   W   X
 *   Text              1   1   0   1
 *   Rodata            1   1   0   0
 *   Image RAM         0   1   1   0
 *   Remaining RAM     0   1   1   0
 *   Console device    1   1   1   0  (if configured)
 *   H2 kernel image   0   1   1   1
 *
 * U-clear entries still let guest-mode accesses (GSR.UM=0) reach kernel
 * data, ISR stubs, and each thread's priv_stack. Guest mode enforces R/W/X
 * but ignores U. User mode (GSR.UM=1) is gated by U, so RAM access comes
 * from the thread's stack and domain entries ahead of this tail.
 *
 * Text and rodata precede RAM because the walk is first-match-wins:
 * memory domains isolate data partitions while shared code remains
 * readable and executable by every thread. Text uses
 * __text_region_start/end rather than __rom_region_start/end. The latter
 * also spans the reset vector and rodata. Separate R|X and R-only entries
 * keep rodata non-executable. The console keeps U=1 because console output
 * runs at user tier during trap0 dispatch.
 */
struct hexagon_linear_entry hexagon_fixed_tail[HEXAGON_FIXED_TAIL_ENTRIES];

/*
 * Map installed while no user thread's is: image RAM in the largest
 * aligned entries, then a chain to hexagon_fixed_tail for everything
 * else. Nothing earlier in this list can overlap RAM, so the livelock
 * that forces the tail's 4KB pages (see hexagon_fixed_tail_init())
 * cannot happen here, and a kernel thread's RAM miss walks ~20 entries,
 * not ~1100. Reuses the text/rodata budgets' 37 entries, but has no
 * corresponding region-size guard: if decomposition exhausts the budget
 * or cannot cover the remainder, it logs an error and leaves that tail
 * unmapped rather than panicking.
 */
#define HEXAGON_KERNEL_MAP_RAM_ENTRIES 37

struct hexagon_linear_entry hexagon_kernel_map[HEXAGON_KERNEL_MAP_RAM_ENTRIES + 1];

static int hexagon_fixed_tail_init(void)
{
	uint32_t idx = 0;
	uintptr_t region_addr;
	size_t region_size;
	uintptr_t text_start = (uintptr_t)__text_region_start;
	uintptr_t text_end = (uintptr_t)__text_region_end;
	uintptr_t rodata_start = (uintptr_t)__rodata_region_start;
	uintptr_t rodata_end = (uintptr_t)__rodata_region_end;
	uintptr_t ram_start = ROUND_UP(MAX(text_end, rodata_end), CONFIG_MMU_PAGE_SIZE);
	uintptr_t ram_end = ROUND_UP((uintptr_t)_image_ram_end, CONFIG_MMU_PAGE_SIZE);

	if ((ram_end - ram_start) / CONFIG_MMU_PAGE_SIZE > HEXAGON_FIXED_TAIL_MAX_RAM_PAGES) {
		k_panic();
	}

	if (text_end > text_start) {
		region_addr = text_start;
		region_size = text_end - text_start;
		hexagon_align_region_to_page(&region_addr, &region_size);
		if (region_size > HEXAGON_FIXED_TAIL_MAX_REGION_BYTES) {
			k_panic();
		}
		idx += hexagon_decompose_region(
			&hexagon_fixed_tail[idx], HEXAGON_FIXED_TAIL_TEXT_ENTRIES, region_addr,
			region_size, __HVM_LINEAR_R | __HVM_LINEAR_X | __HVM_LINEAR_U,
			__HEXAGON_C_WB_L2);
	}

	/*
	 * Rodata usually starts mid-page right after text, and that page is
	 * already mapped (readable) by the last text entry. Rounding rodata
	 * down onto it again would let a larger rodata entry overlap that
	 * page, which livelocks on TLB miss (see the RAM pages below).
	 */
	if (rodata_start >= text_start) {
		rodata_start = MAX(rodata_start, ROUND_UP(text_end, CONFIG_MMU_PAGE_SIZE));
	}

	if (rodata_end > rodata_start) {
		region_addr = rodata_start;
		region_size = rodata_end - rodata_start;
		hexagon_align_region_to_page(&region_addr, &region_size);
		if (region_size > HEXAGON_FIXED_TAIL_MAX_REGION_BYTES) {
			k_panic();
		}
		idx += hexagon_decompose_region(
			&hexagon_fixed_tail[idx], HEXAGON_FIXED_TAIL_RODATA_ENTRIES, region_addr,
			region_size, __HVM_LINEAR_R | __HVM_LINEAR_U, __HEXAGON_C_WB_L2);
	}

	/*
	 * H2 loads a matched entry at its full size. ctlbw refuses an
	 * overlapping entry for the same ASID without evicting the old one,
	 * so a large RAM entry partially covering an earlier mapping livelocks
	 * when both are touched:
	 *
	 *   earlier mapping:  [----------------------]
	 *   large RAM entry:         [----------------------]  overlap
	 *
	 * With 4KB RAM pages, each page is either contained by the earlier
	 * mapping (which wins the walk) or disjoint from it:
	 *
	 *   earlier mapping:  [----------------------]
	 *   4KB RAM pages:    [4K][4K][4K][4K][4K][4K]
	 */
	for (uintptr_t pa = ram_start; pa < ram_end; pa += CONFIG_MMU_PAGE_SIZE) {
		hexagon_linear_entry_set(&hexagon_fixed_tail[idx], pa, pa, __HVM_LINEAR_SIZE_4KB,
					 __HVM_LINEAR_R | __HVM_LINEAR_W, __HEXAGON_C_WB_L2, 0);
		idx++;
	}

	/* Rest of the boot table's last RAM chunk: no per-thread entries here. */
	idx += hexagon_decompose_region(&hexagon_fixed_tail[idx],
					HEXAGON_FIXED_TAIL_POST_IMAGE_ENTRIES, ram_end,
					ROUND_UP(ram_end, HEXAGON_RAM_CHUNK_SIZE) - ram_end,
					__HVM_LINEAR_R | __HVM_LINEAR_W, __HEXAGON_C_WB_L2);

#if DT_HAS_CHOSEN(zephyr_console)
	uintptr_t console_base = HVM_CONSOLE_MAP_BASE;

	hexagon_linear_entry_set(&hexagon_fixed_tail[idx], console_base, console_base,
				 __HVM_LINEAR_SIZE_4MB,
				 __HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_U, __HEXAGON_C_DEV,
				 __HVM_LINEAR_SHARED);
	idx++;
#endif

	idx += hexagon_decompose_region(&hexagon_fixed_tail[idx], HEXAGON_FIXED_TAIL_H2_ENTRIES,
					CONFIG_HEXAGON_H2_KERNEL_BASE,
					CONFIG_HEXAGON_H2_KERNEL_SIZE,
					__HVM_LINEAR_R | __HVM_LINEAR_W | __HVM_LINEAR_X,
					__HEXAGON_C_WB_L2);

	/* hexagon_fixed_tail[idx] stays all-zero: list terminator. */

	idx = hexagon_decompose_region(hexagon_kernel_map, HEXAGON_KERNEL_MAP_RAM_ENTRIES,
				       ram_start,
				       ROUND_UP(ram_end, HEXAGON_RAM_CHUNK_SIZE) - ram_start,
				       __HVM_LINEAR_R | __HVM_LINEAR_W, __HEXAGON_C_WB_L2);
	hexagon_linear_entry_set_chain(&hexagon_kernel_map[idx], hexagon_fixed_tail);

	/* Replace the boot Table map: it is RWX everywhere. */
	if (hexagon_vm_newmap(hexagon_kernel_map, VM_TRANS_TYPE_LINEAR,
			      VM_TLB_INVALIDATE_TRUE) != 0) {
		k_panic();
	}

	return 0;
}

SYS_INIT(hexagon_fixed_tail_init, PRE_KERNEL_1, 0);
