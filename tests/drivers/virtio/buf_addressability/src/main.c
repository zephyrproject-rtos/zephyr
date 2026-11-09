/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static uint8_t static_buf[64] __aligned(16);

#if defined(CONFIG_MMU) && !defined(CONFIG_DEMAND_PAGING)
/* Page-aligned so arch_page_phys_get() can be asked about it. */
static uint8_t page_buf[CONFIG_MMU_PAGE_SIZE] __aligned(CONFIG_MMU_PAGE_SIZE);
#endif

#ifdef CONFIG_TEST_VIRTIO_MAPS_ALL_RAM
BUILD_ASSERT(IS_ENABLED(CONFIG_ARCH_MAPS_ALL_RAM),
	     "TEST_VIRTIO_MAPS_ALL_RAM did not drive ARCH_MAPS_ALL_RAM");
#endif

/* The same derivation the helper documents, used only for boundary cases. */
static void __maybe_unused region_bounds(uintptr_t *lower, uintptr_t *upper)
{
#if defined(CONFIG_MMU) && defined(CONFIG_ARCH_MAPS_ALL_RAM)
	*lower = (uintptr_t)z_mapped_start;
	*upper = (uintptr_t)K_MEM_VIRT_ADDR((uintptr_t)DT_CHOSEN_SRAM_ADDR +
					    (uintptr_t)DT_CHOSEN_SRAM_SIZE);
#elif defined(CONFIG_MMU)
	*lower = (uintptr_t)z_mapped_start;
	*upper = (uintptr_t)z_mapped_end;
#elif DT_HAS_CHOSEN(zephyr_sram)
	*lower = (uintptr_t)DT_CHOSEN_SRAM_ADDR;
	*upper = (uintptr_t)DT_CHOSEN_SRAM_ADDR + (uintptr_t)DT_CHOSEN_SRAM_SIZE;
#else
	*lower = 0U;
	*upper = 0U;
#endif
}

ZTEST(buf_addressability, test_null_and_overflow)
{
	zassert_false(virtio_buf_is_in_linear_mapping(NULL, 0), "NULL accepted");
	zassert_false(virtio_buf_is_in_linear_mapping(NULL, 64), "NULL accepted");
	zassert_false(virtio_buf_is_in_linear_mapping((const void *)(UINTPTR_MAX - 3U), 8U),
		      "overflowing range accepted");
	zassert_false(virtio_buf_is_in_linear_mapping((const void *)(UINTPTR_MAX - 64U), 8U),
		      "range outside the window accepted");
}

ZTEST(buf_addressability, test_static_buffer)
{
	bool expected = !IS_ENABLED(CONFIG_DEMAND_PAGING);

	bool in_ram = virtio_buf_is_in_linear_mapping(static_buf, sizeof(static_buf));

	zassert_equal(in_ram, expected, "static RAM buffer mismatch");
	zassert_equal(virtio_buf_is_in_linear_mapping(static_buf, 0), expected,
		      "zero-length static buffer mismatch");
}

ZTEST(buf_addressability, test_current_stack_buffer)
{
	uint8_t local[8] = {0};
	bool expected;

	/*
	 * With mapped stacks the running stack is a runtime mapping outside the
	 * boot window; otherwise it is part of the kernel image RAM.
	 */
	expected = !IS_ENABLED(CONFIG_DEMAND_PAGING) && !IS_ENABLED(CONFIG_THREAD_STACK_MEM_MAPPED);

	zassert_equal(virtio_buf_is_in_linear_mapping(local, sizeof(local)), expected,
		      "thread stack buffer mismatch (stack mapped: %d)",
		      IS_ENABLED(CONFIG_THREAD_STACK_MEM_MAPPED));
	zassert_equal(virtio_buf_is_in_linear_mapping(local, 0), expected,
		      "zero-length thread stack buffer mismatch");
}

ZTEST(buf_addressability, test_heap_buffer)
{
	void *heap = k_malloc(64);

	if (heap == NULL) {
		ztest_test_skip();
	}

	zassert_equal(virtio_buf_is_in_linear_mapping(heap, 64), !IS_ENABLED(CONFIG_DEMAND_PAGING),
		      "heap buffer mismatch");

	k_free(heap);
}

ZTEST(buf_addressability, test_runtime_mapping_rejected)
{
#if defined(CONFIG_MMU) && !defined(CONFIG_DEMAND_PAGING)
	void *mapped = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	uintptr_t lower;
	uintptr_t upper;

	zassert_not_null(mapped, "k_mem_map() failed, no runtime mapping to reject");
	region_bounds(&lower, &upper);

	/* The mapping allocator starts at the end of the boot window. */
	zassert_true((uintptr_t)mapped >= upper, "runtime mapping is inside the boot window");

	zassert_false(virtio_buf_is_in_linear_mapping(mapped, 64), "runtime mapping accepted");
	zassert_false(virtio_buf_is_in_linear_mapping(mapped, 0),
		      "zero-length runtime mapping accepted");
	zassert_false(virtio_buf_is_in_linear_mapping(mapped, CONFIG_MMU_PAGE_SIZE),
		      "whole runtime mapping accepted");

	k_mem_unmap(mapped, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(buf_addressability, test_accepted_translation_matches_page_tables)
{
#if defined(CONFIG_MMU) && !defined(CONFIG_DEMAND_PAGING)
	uintptr_t phys = 0U;

	zassert_true(virtio_buf_is_in_linear_mapping(page_buf, sizeof(page_buf)),
		     "page-aligned buffer should be accepted");
	zassert_ok(arch_page_phys_get(page_buf, &phys),
		   "no page table entry for an accepted buffer");
	zassert_equal(k_mem_phys_addr(page_buf), phys, "translation mismatch");
#else
	ztest_test_skip();
#endif
}

ZTEST(buf_addressability, test_boot_window_boundaries)
{
#if defined(CONFIG_DEMAND_PAGING)
	ztest_test_skip();
#else
	uintptr_t lower;
	uintptr_t upper;

	region_bounds(&lower, &upper);
	zassert_true(upper > lower, "empty accepted region");

	printk("BUFCFG window lower=0x%lx upper=0x%lx\n", (unsigned long)lower,
	       (unsigned long)upper);

	/* The last byte of the window is accepted, one byte more is not. */
	zassert_true(virtio_buf_is_in_linear_mapping((const void *)(upper - 8U), 8U),
		     "range ending at the window end rejected");
	zassert_false(virtio_buf_is_in_linear_mapping((const void *)(upper - 8U), 9U),
		      "range crossing the window end accepted");

	/* Zero-length ranges are strictly inside. */
	zassert_false(virtio_buf_is_in_linear_mapping((const void *)upper, 0),
		      "zero-length at the window end accepted");
	zassert_true(virtio_buf_is_in_linear_mapping((const void *)(lower + 8U), 0),
		     "zero-length inside the window rejected");
	zassert_false(virtio_buf_is_in_linear_mapping((const void *)(lower - 8U), 0),
		      "zero-length below the window accepted");
#endif
}

ZTEST(buf_addressability, test_demand_paging_rejects_everything)
{
#if defined(CONFIG_DEMAND_PAGING)
	uint8_t local[4] = {0};

	zassert_false(virtio_buf_is_in_linear_mapping(static_buf, sizeof(static_buf)));
	zassert_false(virtio_buf_is_in_linear_mapping(static_buf, 0));
	zassert_false(virtio_buf_is_in_linear_mapping(local, sizeof(local)));
	zassert_false(virtio_buf_is_in_linear_mapping(local, 0));
#else
	ztest_test_skip();
#endif
}

static void *buf_addressability_setup(void)
{
	unsigned int vm_size = 0U;

#if defined(CONFIG_KERNEL_VM_SIZE)
	vm_size = CONFIG_KERNEL_VM_SIZE;
#endif

	printk("BUFCFG mmu=%d stack_mapped=%d demand_paging=%d maps_all_ram=%d vm_size=0x%x\n",
	       IS_ENABLED(CONFIG_MMU), IS_ENABLED(CONFIG_THREAD_STACK_MEM_MAPPED),
	       IS_ENABLED(CONFIG_DEMAND_PAGING), IS_ENABLED(CONFIG_ARCH_MAPS_ALL_RAM), vm_size);

	return NULL;
}

ZTEST_SUITE(buf_addressability, NULL, buf_addressability_setup, NULL, NULL, NULL);
