/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/sys/sys_io.h>

/* Supply the architecture operations used by the driver on native_sim. */
#define arch_curr_cpu() (&_kernel.cpus[0])

static void test_busy_wait(uint32_t usec);
static void test_cache_flush(const void *addr, size_t size);
static uint32_t test_read32(mem_addr_t addr);
static void test_write32(uint32_t value, mem_addr_t addr);
static void test_write64(uint64_t value, mem_addr_t addr);

/* Advance the fake command reader at a deterministic interleaving point. */
#define k_busy_wait                      test_busy_wait
#define arch_dcache_flush_and_invd_range test_cache_flush
#define sys_read32                       test_read32
#define sys_write32                      test_write32
#define sys_write64                      test_write64
#include "../../../../../drivers/interrupt_controller/intc_gicv3_its.c"
#undef sys_write64
#undef sys_write32
#undef sys_read32
#undef arch_dcache_flush_and_invd_range
#undef k_busy_wait

mem_addr_t gic_rdists[CONFIG_MP_MAX_NUM_CPUS];

struct register_behavior {
	uint64_t fixed_mask;
	uint64_t fixed_value;
	uint64_t rejected_bits;
};

struct cache_flush {
	const void *addr;
	size_t size;
	uint32_t order;
};

static struct gicv3_its_data data;
static struct gicv3_its_config config;
static const struct device dev = {.data = &data, .config = &config};
static struct register_behavior baser_behavior[GITS_BASER_NR_REGS];
static struct register_behavior cbaser_behavior;
static struct cache_flush flushes[8];
static size_t flush_count;
static uint32_t publication_order[GITS_BASER_NR_REGS];
static uint32_t enable_order;
static uint32_t io_order;
static uint32_t gicd_typer;
static bool fail_sync;
static bool fail_discard;
static void (*wait_hook)(void);
static uint32_t hook_command;
static uint32_t nested_event;
static int nested_ret;
static int rollback_ret;

static void test_cache_flush(const void *addr, size_t size)
{
	zassert_true(flush_count < ARRAY_SIZE(flushes));
	flushes[flush_count++] = (struct cache_flush){addr, size, ++io_order};
}

static uint32_t test_read32(mem_addr_t addr)
{
	return (addr == GICD_TYPER) ? gicd_typer : sys_read32(addr);
}

static void test_write32(uint32_t value, mem_addr_t addr)
{
	if ((addr == data.base + GITS_CTLR) && (GITS_CTLR_ENABLED_GET(value) != 0U)) {
		enable_order = ++io_order;
	}
	sys_write32(value, addr);
}

static void test_write64(uint64_t value, mem_addr_t addr)
{
	struct register_behavior *behavior = NULL;

	if (addr == data.base + GITS_CBASER) {
		behavior = &cbaser_behavior;
	} else if ((addr >= data.base + GITS_BASER(0)) &&
		   (addr < data.base + GITS_BASER(GITS_BASER_NR_REGS))) {
		size_t index = (addr - data.base - GITS_BASER(0)) / sizeof(uint64_t);
		uint64_t read_only = MASK(GITS_BASER_TYPE) | MASK(GITS_BASER_ENTRY_SIZE);

		behavior = &baser_behavior[index];
		value = (value & ~read_only) | (sys_read64(addr) & read_only);
		if ((MASK_GET(value, GITS_BASER_VALID) != 0U) && (publication_order[index] == 0U)) {
			publication_order[index] = ++io_order;
		}
	}
	if (behavior != NULL) {
		value = (value & ~behavior->fixed_mask) |
			(behavior->fixed_value & behavior->fixed_mask);
		value &= ~behavior->rejected_bits;
	}
	sys_write64(value, addr);
}

static uint32_t flush_order(const void *addr, size_t size)
{
	for (size_t i = 0U; i < flush_count; i++) {
		if ((flushes[i].addr == addr) && (flushes[i].size >= size)) {
			return flushes[i].order;
		}
	}
	zassert_unreachable("Required cache flush was not issued");
	return 0U;
}

static void test_busy_wait(uint32_t usec)
{
	struct its_cmd_block *last = data.cmd_write - 1;
	uint32_t command = MASK_GET(last->raw_cmd[0], GITS_CMD_ID);

	ARG_UNUSED(usec);
	if ((wait_hook != NULL) && (command == hook_command)) {
		void (*hook)(void) = wait_hook;

		wait_hook = NULL;
		hook();
	}

	if (((command == GITS_CMD_ID_SYNC) && fail_sync) ||
	    ((command == GITS_CMD_ID_DISCARD) && fail_discard)) {
		return;
	}

	if (command == GITS_CMD_ID_DISCARD) {
		fail_sync = false;
	}

	sys_write32(sys_read32(data.base + GITS_CWRITER), data.base + GITS_CREADR);
}

static void before(void *fixture)
{
	static struct its_cmd_block commands[ITS_CMD_QUEUE_NR_ENTRIES] __aligned(SIZE_64K);
	static uint64_t registers[SIZE_4K / sizeof(uint64_t)];
	static uint8_t device_table[SIZE_64K] __aligned(SIZE_64K);
	static uint8_t collection_table[SIZE_64K] __aligned(SIZE_64K);
	static uint8_t indirect_pages[CONFIG_GIC_V3_ITS_MAX_DEVICES][SIZE_64K] __aligned(SIZE_64K);
	static uint8_t itts[CONFIG_GIC_V3_ITS_MAX_DEVICES][ITS_ITT_SLOT_SIZE] __aligned(SIZE_256);

	ARG_UNUSED(fixture);
	(void)its_send_invall_cmd;
	memset(&data, 0, sizeof(data));
	memset(registers, 0, sizeof(registers));
	memset(baser_behavior, 0, sizeof(baser_behavior));
	memset(&cbaser_behavior, 0, sizeof(cbaser_behavior));
	memset(publication_order, 0, sizeof(publication_order));
	flush_count = 0U;
	enable_order = 0U;
	io_order = 0U;
	gicd_typer = 15U << 19;
	config = (struct gicv3_its_config){
		.base_addr = (uintptr_t)registers,
		.base_size = sizeof(registers),
		.cmd_queue = commands,
		.cmd_queue_size = sizeof(commands),
		.device_table = device_table,
		.device_table_size = sizeof(device_table),
		.collection_table = collection_table,
		.collection_table_size = sizeof(collection_table),
		.indirect_dev_pages = indirect_pages,
		.indirect_dev_page_count = ARRAY_SIZE(indirect_pages),
		.itts = itts,
		.device_id_bits = 8U,
	};
	data.base = (uintptr_t)registers;
	data.cmd_base = commands;
	data.cmd_write = commands;
	data.device_id_bits = 16U;
	data.lpi_intid_limit = CONFIG_NUM_IRQS;
	data.device_slots[0] = (struct its_device_slot){
		.device_id = 1U,
		.nites = CONFIG_GIC_V3_ITS_MAX_VECTORS,
		.used = true,
	};
	sys_write32(MASK(GITS_CTLR_QUIESCENT), data.base + GITS_CTLR);
	sys_write64(MASK_SET(1U, GITS_TYPER_PTA) | MASK_SET(15U, GITS_TYPER_IDBITS) |
			    MASK_SET(15U, GITS_TYPER_DEVBITS) |
			    MASK_SET(7U, GITS_TYPER_ITT_ENTRY_SIZE),
		    data.base + GITS_TYPER);
	sys_write64(MASK_SET(GITS_BASER_TYPE_DEVICE, GITS_BASER_TYPE) |
			    MASK_SET(7U, GITS_BASER_ENTRY_SIZE),
		    data.base + GITS_BASER(0));
	sys_write64(MASK_SET(GITS_BASER_TYPE_COLLECTION, GITS_BASER_TYPE) |
			    MASK_SET(7U, GITS_BASER_ENTRY_SIZE),
		    data.base + GITS_BASER(1));
	zassert_ok(sys_bitarray_clear_region(&its_lpi_intids, ITS_LPI_BITMAP_BITS, 0U));
	fail_sync = false;
	fail_discard = false;
	wait_hook = NULL;
	nested_event = 0U;
	nested_ret = -EINPROGRESS;
	rollback_ret = -EINPROGRESS;
}

static void remap_during_rollback(void)
{
	rollback_ret = gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U);
}

static void remap_during_mapping(void)
{
	nested_ret = gicv3_its_map_intid(&dev, 1U, nested_event, GIC_LPI_INT_BASE + 1U);
}

static void remap_then_timeout(void)
{
	remap_during_mapping();
	fail_sync = true;
	hook_command = GITS_CMD_ID_DISCARD;
	wait_hook = remap_during_rollback;
}

ZTEST(gicv3_its_reservation, test_in_progress_mapping_cannot_be_reused)
{
	bool reused;

	zassert_ok(its_reserve_event_range(&data, 1U, 0U, 1U, &reused));
	zassert_false(reused);
	zassert_equal(its_reserve_event_range(&data, 1U, 0U, 1U, &reused), -EBUSY,
		      "A remap must not overtake an unfinished mapping or its rollback");
}

ZTEST(gicv3_its_reservation, test_mapping_and_rollback_exclude_remaps)
{
	hook_command = GITS_CMD_ID_MAPTI;
	wait_hook = remap_then_timeout;

	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ETIMEDOUT);
	zassert_equal(nested_ret, -EBUSY, "An unfinished mapping must reject remaps");
	zassert_equal(rollback_ret, -EBUSY, "Rollback must retain exclusive ownership");
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U));
}

ZTEST(gicv3_its_reservation, test_failed_cleanup_keeps_event_busy)
{
	fail_sync = true;
	fail_discard = true;
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ETIMEDOUT);

	fail_sync = false;
	fail_discard = false;
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U), -EBUSY);
}

ZTEST(gicv3_its_reservation, test_failed_remap_keeps_reservation)
{
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
	fail_sync = true;
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U), -ETIMEDOUT);
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 1U), 0U);
	fail_sync = false;
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
}

ZTEST(gicv3_its_reservation, test_disjoint_event_can_be_mapped)
{
	nested_event = 33U;
	hook_command = GITS_CMD_ID_MAPTI;
	wait_hook = remap_during_mapping;
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
	zassert_ok(nested_ret);
}

ZTEST(gicv3_its_reservation, test_batch_mapping_excludes_remaps)
{
	hook_command = GITS_CMD_ID_SYNC;
	wait_hook = remap_during_mapping;
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U), GIC_LPI_INT_BASE);
	zassert_equal(nested_ret, -EBUSY);
	zassert_equal(data.device_slots[0].event_id_busy_bitmap[0], 0U);
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U), 0U);
}

ZTEST(gicv3_its_reservation, test_failed_batch_releases_events)
{
	hook_command = GITS_CMD_ID_MAPTI;
	wait_hook = remap_then_timeout;
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U), 0U);
	zassert_equal(nested_ret, -EBUSY);
	zassert_equal(rollback_ret, -EBUSY);
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U), GIC_LPI_INT_BASE);
}

ZTEST(gicv3_its_reservation, test_baser_nonshareable_fallback_flushes_before_publication)
{
	for (size_t i = 0U; i < 2U; i++) {
		baser_behavior[i].fixed_mask = MASK(GITS_BASER_SHAREABILITY);
	}
	zassert_ok(its_setup_tables(&data, &config));
	zassert_true(data.tables_need_flush);
	zassert_true(flush_order(config.device_table, SIZE_64K) < publication_order[0]);
	zassert_true(flush_order(config.collection_table, SIZE_64K) < publication_order[1]);
	for (size_t i = 0U; i < 2U; i++) {
		uint64_t value = sys_read64(data.base + GITS_BASER(i));

		zassert_equal(MASK_GET(value, GITS_BASER_SHAREABILITY), GIC_BASER_SHARE_NO);
		zassert_equal(MASK_GET(value, GITS_BASER_INNER_CACHE), GIC_BASER_CACHE_NCACHEABLE);
	}
}

ZTEST(gicv3_its_reservation, test_baser_fixed_page_sizes)
{
	const uint32_t pages[] = {
		GITS_BASER_PAGE_SIZE_4K,
		GITS_BASER_PAGE_SIZE_16K,
		GITS_BASER_PAGE_SIZE_64K,
	};

	for (size_t page = 0U; page < ARRAY_SIZE(pages); page++) {
		before(NULL);
		for (size_t i = 0U; i < 2U; i++) {
			baser_behavior[i].fixed_mask = MASK(GITS_BASER_PAGE_SIZE);
			baser_behavior[i].fixed_value = MASK_SET(pages[page], GITS_BASER_PAGE_SIZE);
		}
		zassert_ok(its_setup_tables(&data, &config));
		for (size_t i = 0U; i < 2U; i++) {
			zassert_equal(MASK_GET(sys_read64(data.base + GITS_BASER(i)),
					       GITS_BASER_PAGE_SIZE),
				      pages[page]);
		}
	}
}

ZTEST(gicv3_its_reservation, test_baser_rejected_address_is_not_published)
{
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_ADDR);
	zassert_equal(its_setup_tables(&data, &config), -ENOTSUP);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
}

ZTEST(gicv3_its_reservation, test_baser_rejected_indirection_is_not_published)
{
	uint64_t typer = sys_read64(data.base + GITS_TYPER);

	sys_write64((typer & ~MASK(GITS_TYPER_DEVBITS)) | MASK_SET(16U, GITS_TYPER_DEVBITS),
		    data.base + GITS_TYPER);
	baser_behavior[0].rejected_bits = MASK(GITS_BASER_INDIRECT);
	zassert_equal(its_setup_tables(&data, &config), -ENOTSUP);
	zassert_equal(publication_order[0], 0U);
}

ZTEST(gicv3_its_reservation, test_baser_rejected_valid_invalidates_all_tables)
{
	baser_behavior[1].rejected_bits = MASK(GITS_BASER_VALID);
	zassert_equal(its_setup_tables(&data, &config), -ENOTSUP);
	zassert_true(publication_order[0] > 0U);
	for (size_t i = 0U; i < 2U; i++) {
		zassert_equal(sys_read64(data.base + GITS_BASER(i)) &
				      (MASK(GITS_BASER_VALID) | MASK(GITS_BASER_ADDR)),
			      0U);
	}
}

ZTEST(gicv3_its_reservation, test_baser_capacity_failure_does_not_publish_tables)
{
	config.collection_table_size = SIZE_4K;
	zassert_equal(its_setup_tables(&data, &config), -ENOSPC);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
}

ZTEST(gicv3_its_reservation, test_baser_rejects_cacheable_nonshareable_memory)
{
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_SHAREABILITY) | MASK(GITS_BASER_INNER_CACHE);
	baser_behavior[0].fixed_value = MASK_SET(GIC_BASER_CACHE_RAWAWB, GITS_BASER_INNER_CACHE);
	zassert_equal(its_setup_tables(&data, &config), -ENOTSUP);
	zassert_equal(MASK_GET(sys_read64(data.base + GITS_BASER(0)), GITS_BASER_VALID), 0U);
}

ZTEST(gicv3_its_reservation, test_cbaser_fallback_flushes_before_enable)
{
	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_SHAREABILITY);
	zassert_ok(gicv3_its_init(&dev));
	zassert_true(data.cmd_queue_needs_flush);
	zassert_true(flush_order(config.cmd_queue, ITS_CMD_QUEUE_SIZE) < enable_order);
}

ZTEST(gicv3_its_reservation, test_cbaser_rejects_address_and_valid_bits)
{
	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_ADDR);
	zassert_equal(its_setup_cmd_queue(&dev), -ENOTSUP);
	zassert_equal(MASK_GET(sys_read64(data.base + GITS_CBASER), GITS_CBASER_VALID), 0U);
	cbaser_behavior.fixed_mask = 0U;
	cbaser_behavior.rejected_bits = MASK(GITS_CBASER_VALID);
	zassert_equal(its_setup_cmd_queue(&dev), -ENOTSUP);
}

ZTEST(gicv3_its_reservation, test_lpi_limit_honors_its_idbits)
{
	uint64_t typer = sys_read64(data.base + GITS_TYPER);

	sys_write64((typer & ~MASK(GITS_TYPER_IDBITS)) | MASK_SET(13U, GITS_TYPER_IDBITS),
		    data.base + GITS_TYPER);
	zassert_ok(gicv3_its_init(&dev));
	zassert_equal(data.lpi_id_bits, 14U);
	zassert_equal(data.lpi_intid_limit, 16384U);
	zassert_equal(gicv3_its_alloc_intids(&dev, 8191U), 8192U);
	zassert_equal(gicv3_its_alloc_intid(&dev), 16383U);
	zassert_equal(gicv3_its_alloc_intid(&dev), 0U);
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, 16383U));
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, 16384U), -EINVAL);
}

ZTEST(gicv3_its_reservation, test_lpi_limit_rejects_its_without_lpi_id_space)
{
	uint64_t typer = sys_read64(data.base + GITS_TYPER);

	sys_write64((typer & ~MASK(GITS_TYPER_IDBITS)) | MASK_SET(12U, GITS_TYPER_IDBITS),
		    data.base + GITS_TYPER);
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);
	zassert_equal(enable_order, 0U);
}

ZTEST_SUITE(gicv3_its_reservation, NULL, NULL, before, NULL, NULL);
