/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/sys_io.h>

/* Supply the architecture operations used by the driver on native_sim. */
#define arch_curr_cpu() (&_kernel.cpus[0])

static void test_busy_wait(uint32_t usec);
static int test_cache_flush(void *addr, size_t size);
static int test_lpi_free(sys_bitarray_t *bitarray, size_t count, size_t offset);
static uint32_t test_read32(mem_addr_t addr);
static void test_write32(uint32_t value, mem_addr_t addr);
static void test_write64(uint64_t value, mem_addr_t addr);

/* Advance the fake command reader at a deterministic interleaving point. */
#define k_busy_wait                         test_busy_wait
#define sys_cache_data_flush_and_invd_range test_cache_flush
#define sys_bitarray_free                   test_lpi_free
#define sys_read32                          test_read32
#define sys_write32                         test_write32
#define sys_write64                         test_write64
#include "../../../../../drivers/interrupt_controller/intc_gicv3_its.c"
#undef sys_write64
#undef sys_write32
#undef sys_read32
#undef sys_bitarray_free
#undef sys_cache_data_flush_and_invd_range
#undef k_busy_wait

mem_addr_t gic_rdists[CONFIG_MP_MAX_NUM_CPUS];
static unsigned int ready_rdists;

bool gicv3_rdist_lpi_is_ready(unsigned int cpu_id)
{
	return (ready_rdists & BIT(cpu_id)) != 0U;
}

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
static struct device_state state = {.initialized = true};
static const struct device dev = {
	.data = &data,
	.config = &config,
	.state = &state,
	.api = &gicv3_its_api,
	.ops.init = gicv3_its_init,
};
static struct register_behavior baser_behavior[GITS_BASER_NR_REGS];
static struct register_behavior cbaser_behavior;
static struct cache_flush flushes[8];
static size_t flush_count;
static int cache_flush_result;
static uint32_t publication_order[GITS_BASER_NR_REGS];
static uint32_t enable_order;
static uint32_t command_write_order;
static uint32_t io_order;
static uint32_t gicd_typer;
static bool fail_sync;
static bool fail_rollback_sync;
static bool fail_discard;
static bool fail_discard_flush;
static bool fail_mapd;
static int fail_mapc_cpu;
static bool fail_quiesce;
static void (*wait_hook)(void);
static uint32_t hook_command;
static uint32_t nested_event;
static int nested_ret;
static int rollback_ret;
static int rollback_int_ret;
static bool check_lpi_release;
static size_t released_lpis;

static int test_cache_flush(void *addr, size_t size)
{
	zassert_true(flush_count < ARRAY_SIZE(flushes));
	io_order++;
	flushes[flush_count] = (struct cache_flush){addr, size, io_order};
	flush_count++;

	if (fail_discard_flush && (size == sizeof(struct its_cmd_block))) {
		const struct its_cmd_block *cmd = addr;

		if (MASK_GET(cmd->raw_cmd[0], GITS_CMD_ID) == GITS_CMD_ID_DISCARD) {
			return -ENOTSUP;
		}
	}

	return cache_flush_result;
}

static int test_lpi_free(sys_bitarray_t *bitarray, size_t count, size_t offset)
{
	if (check_lpi_release) {
		zassert_equal(data.device_slots[0].event_id_busy_bitmap[0], BIT_MASK(3));
		zassert_equal(released_lpis, 0U);
		released_lpis = count;
	}

	return sys_bitarray_free(bitarray, count, offset);
}

static uint32_t test_read32(mem_addr_t addr)
{
	return (addr == GICD_TYPER) ? gicd_typer : sys_read32(addr);
}

static void test_write32(uint32_t value, mem_addr_t addr)
{
	if ((addr == data.base + GITS_CTLR) && (GITS_CTLR_ENABLED_GET(value) != 0U)) {
		io_order++;
		enable_order = io_order;
		value &= ~MASK(GITS_CTLR_QUIESCENT);
	}
	if (addr == data.base + GITS_CWRITER) {
		io_order++;
		command_write_order = io_order;
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
			io_order++;
			publication_order[index] = io_order;
		}
	}
	if (behavior != NULL) {
		uint32_t ctlr = sys_read32(data.base + GITS_CTLR);

		zassert_equal(GITS_CTLR_ENABLED_GET(ctlr), 0U);
		zassert_equal(GITS_CTLR_QUIESCENT_GET(ctlr), 1U);
		value = (value & ~behavior->fixed_mask) |
			(behavior->fixed_value & behavior->fixed_mask);
		value &= ~behavior->rejected_bits;
	}
	sys_write64(value, addr);
	if (addr == data.base + GITS_CBASER) {
		sys_write32(0U, data.base + GITS_CREADR);
	}
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
	struct its_cmd_block *last;
	uint32_t command;

	if (usec == USEC_PER_MSEC) {
		if (!fail_quiesce) {
			sys_write32(MASK(GITS_CTLR_QUIESCENT), data.base + GITS_CTLR);
		}
		return;
	}
	last = (data.cmd_write == data.cmd_base) ? &data.cmd_base[ITS_CMD_QUEUE_NR_ENTRIES - 1U]
						 : data.cmd_write - 1;
	command = MASK_GET(last->raw_cmd[0], GITS_CMD_ID);

	if ((wait_hook != NULL) && (command == hook_command)) {
		void (*hook)(void) = wait_hook;

		wait_hook = NULL;
		hook();
	}

	if (((command == GITS_CMD_ID_SYNC) && fail_sync) ||
	    ((command == GITS_CMD_ID_MAPD) && fail_mapd &&
	     (MASK_GET(last->raw_cmd[2], GITS_CMD_VALID) != 0U)) ||
	    ((command == GITS_CMD_ID_MAPC) &&
	     ((int)MASK_GET(last->raw_cmd[2], GITS_CMD_ICID) == fail_mapc_cpu)) ||
	    ((command == GITS_CMD_ID_DISCARD) && fail_discard)) {
		return;
	}

	if (command == GITS_CMD_ID_DISCARD) {
		fail_sync = fail_rollback_sync;
	}

	sys_write32(sys_read32(data.base + GITS_CWRITER), data.base + GITS_CREADR);
}

static void before(void *fixture)
{
	static struct its_cmd_block commands[ITS_CMD_QUEUE_NR_ENTRIES] __aligned(SIZE_64K);
	static uint64_t registers[SIZE_4K / sizeof(uint64_t)];
	static uint8_t device_table[3 * SIZE_64K] __aligned(SIZE_64K);
	static uint8_t collection_table[SIZE_64K] __aligned(SIZE_64K);
	static uint8_t itts[CONFIG_GIC_V3_ITS_MAX_DEVICES][ITS_ITT_SLOT_SIZE] __aligned(SIZE_256);

	ARG_UNUSED(fixture);
	(void)its_send_invall_cmd;
	memset(&data, 0, sizeof(data));
	memset(registers, 0, sizeof(registers));
	memset(device_table, 0, sizeof(device_table));
	memset(itts, 0, sizeof(itts));
	memset(baser_behavior, 0, sizeof(baser_behavior));
	memset(&cbaser_behavior, 0, sizeof(cbaser_behavior));
	memset(publication_order, 0, sizeof(publication_order));
	flush_count = 0U;
	cache_flush_result = 0;
	enable_order = 0U;
	command_write_order = 0U;
	io_order = 0U;
	gicd_typer = (15U << 19) | GICD_TYPER_LPIS;
	state = (struct device_state){.initialized = true};
	ready_rdists = BIT(0);
	memset(gic_rdists, 0, sizeof(gic_rdists));
	config = (struct gicv3_its_config){
		.base_addr = (uintptr_t)registers,
		.base_size = sizeof(registers),
		.cmd_queue = commands,
		.cmd_queue_size = sizeof(commands),
		.device_table = device_table,
		.device_table_size = sizeof(device_table),
		.collection_table = collection_table,
		.collection_table_size = sizeof(collection_table),
		.itts = itts,
		.device_id_bits = 8U,
	};
	data.base = (uintptr_t)registers;
	data.cmd_base = commands;
	data.cmd_write = commands;
	data.device_id_bits = 16U;
	data.lpi_intid_limit = CONFIG_NUM_IRQS;
	atomic_set_bit(data.mapped_collections, 0);
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
	fail_rollback_sync = false;
	fail_discard = false;
	fail_discard_flush = false;
	fail_mapd = false;
	fail_mapc_cpu = -1;
	fail_quiesce = false;
	wait_hook = NULL;
	nested_event = 0U;
	nested_ret = -EINPROGRESS;
	rollback_ret = -EINPROGRESS;
	rollback_int_ret = -EINPROGRESS;
	check_lpi_release = false;
	released_lpis = 0U;
}

static void remap_during_rollback(void)
{
	rollback_ret = gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U);
	rollback_int_ret = its_send_int(&dev, 1U, 0U);
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

static void fail_second_mapping(void)
{
	/* This hook runs at the first SYNC. Fail the next one. */
	wait_hook = remap_then_timeout;
}

static void assert_batch_rollback_commands(void)
{
	zassert_equal(data.cmd_posted, 7U);

	/* After two MAPTI/SYNC pairs, discard both events and then synchronize. */
	for (uint32_t i = 0U; i < 2U; i++) {
		const struct its_cmd_block *cmd = &config.cmd_queue[4U + i];

		zassert_equal(MASK_GET(cmd->raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_DISCARD);
		zassert_equal(MASK_GET(cmd->raw_cmd[1], GITS_CMD_EVENTID), i);
	}
	zassert_equal(MASK_GET(config.cmd_queue[6].raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_SYNC);
}

ZTEST(gicv3_its_reservation, test_int_requires_completed_mapping)
{
	zassert_equal(its_send_int(&dev, 1U, 0U), -EINVAL);
	zassert_ok(its_reserve_event_range(&data, 1U, 0U, 1U));
	zassert_equal(its_send_int(&dev, 1U, 0U), -EBUSY);
	zassert_equal(data.cmd_posted, 0U);
}

ZTEST(gicv3_its_reservation, test_mapping_and_rollback_exclude_remaps)
{
	hook_command = GITS_CMD_ID_MAPTI;
	wait_hook = remap_then_timeout;

	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ETIMEDOUT);
	zassert_equal(nested_ret, -EBUSY, "An unfinished mapping must reject remaps");
	zassert_equal(rollback_ret, -EBUSY, "Rollback must retain exclusive ownership");
	zassert_equal(rollback_int_ret, -EBUSY);
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
}

ZTEST(gicv3_its_reservation, test_failed_cleanup_keeps_event_busy)
{
	fail_sync = true;
	fail_discard = true;
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ETIMEDOUT);

	fail_sync = false;
	fail_discard = false;
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U), -EBUSY);
	zassert_equal(its_send_int(&dev, 1U, 0U), -EBUSY);
}

ZTEST(gicv3_its_reservation, test_mapped_event_rejects_repeat_mapping)
{
	unsigned int intid;

	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE));
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE + 1U), -EBUSY);
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 1U, &intid), -EBUSY);
	zassert_equal(data.cmd_posted, 2U);
	zassert_ok(its_send_int(&dev, 1U, 0U));
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
	unsigned int intid;

	hook_command = GITS_CMD_ID_SYNC;
	wait_hook = remap_during_mapping;
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U, &intid), 0);
	zassert_equal(intid, GIC_LPI_INT_BASE);
	zassert_equal(nested_ret, -EBUSY);
	zassert_equal(data.device_slots[0].event_id_busy_bitmap[0], 0U);
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 2U, &intid), -EBUSY);
}

ZTEST(gicv3_its_reservation, test_failed_batch_releases_events)
{
	unsigned int intid = UINT_MAX;

	check_lpi_release = true;
	hook_command = GITS_CMD_ID_SYNC;
	wait_hook = fail_second_mapping;
	zassert_equal(gicv3_its_alloc_map_intids(&dev, 1U, 3U, &intid), -ETIMEDOUT);
	zassert_equal(intid, UINT_MAX);
	assert_batch_rollback_commands();
	zassert_equal(nested_ret, -EBUSY);
	zassert_equal(rollback_ret, -EBUSY);
	zassert_equal(rollback_int_ret, -EBUSY);
	zassert_equal(released_lpis, 3U);
	zassert_ok(gicv3_its_alloc_map_intids(&dev, 1U, 3U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE);
}

ZTEST(gicv3_its_reservation, test_batch_rollback_sync_timeout_retains_mapped_intids)
{
	unsigned int intid = UINT_MAX;

	hook_command = GITS_CMD_ID_SYNC;
	wait_hook = fail_second_mapping;
	fail_rollback_sync = true;
	zassert_equal(its_alloc_map_intids(&dev, 1U, 3U, &intid), -ETIMEDOUT);
	zassert_equal(intid, UINT_MAX);
	assert_batch_rollback_commands();
	zassert_equal(data.device_slots[0].event_id_busy_bitmap[0], BIT_MASK(2));
	zassert_ok(its_alloc_intids(&dev, 1U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE + 2U);
}

ZTEST(gicv3_its_reservation, test_failed_batch_cleanup_preserves_mapping_error)
{
	unsigned int intid = UINT_MAX;

	data.cmd_queue_needs_flush = true;
	fail_sync = true;
	fail_discard_flush = true;
	zassert_equal(its_alloc_map_intids(&dev, 1U, 2U, &intid), -ETIMEDOUT);
	zassert_equal(intid, UINT_MAX);
	zassert_equal(data.device_slots[0].event_id_busy_bitmap[0], BIT(0));
	zassert_ok(its_alloc_intids(&dev, 1U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE + 1U);
}

static void dirty_itt_during_mapd(void)
{
	memset(config.itts[1], 0xff, ITS_ITT_SLOT_SIZE);
}

ZTEST(gicv3_its_reservation, test_mapd_timeout_clears_itt_before_reuse)
{
	static const uint8_t empty_itt[ITS_ITT_SLOT_SIZE];

	ready_rdists = BIT(0) | BIT(1);
	gic_rdists[0] = SIZE_64K;
	gic_rdists[1] = 2U * SIZE_64K;
	data.tables_need_flush = true;
	fail_mapd = true;
	hook_command = GITS_CMD_ID_MAPD;
	wait_hook = dirty_itt_during_mapd;
	zassert_equal(its_setup_deviceid(&dev, 2U, 1U), -ETIMEDOUT);
	zassert_equal(MASK_GET(config.cmd_queue[2].raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_SYNC);
	zassert_equal(MASK_GET(config.cmd_queue[2].raw_cmd[2], GITS_CMD_RDBASE), 1U);
	zassert_mem_equal(config.itts[1], empty_itt, sizeof(empty_itt));
	zassert_true(flush_order(config.itts[1], sizeof(empty_itt)) > 0U);
	zassert_false(data.device_slots[1].used);

	fail_mapd = false;
	zassert_ok(its_setup_deviceid(&dev, 3U, 1U));
}

ZTEST(gicv3_its_reservation, test_mapd_timeout_syncs_with_ready_secondary)
{
	uint64_t rdist_regs[GICR_TYPER / sizeof(uint64_t) + 1U] = {0};
	const unsigned int processor_number = 7U;

	rdist_regs[GICR_TYPER / sizeof(rdist_regs[0])] =
		MASK_SET(processor_number, GICR_TYPER_PROCESSOR_NUMBER);
	gic_rdists[1] = (mem_addr_t)rdist_regs;
	ready_rdists = BIT(1);
	sys_write64(sys_read64(data.base + GITS_TYPER) & ~MASK(GITS_TYPER_PTA),
		    data.base + GITS_TYPER);
	fail_mapd = true;

	zassert_equal(its_setup_deviceid(&dev, 2U, 1U), -ETIMEDOUT);
	zassert_equal(MASK_GET(config.cmd_queue[2].raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_SYNC);
	zassert_equal(MASK_GET(config.cmd_queue[2].raw_cmd[2], GITS_CMD_RDBASE), processor_number);
	zassert_false(data.device_slots[1].used);
}

ZTEST(gicv3_its_reservation, test_mapd_timeout_keeps_itt_without_ready_redistributor)
{
	ready_rdists = 0U;
	fail_mapd = true;
	hook_command = GITS_CMD_ID_MAPD;
	wait_hook = dirty_itt_during_mapd;

	zassert_equal(its_setup_deviceid(&dev, 2U, 1U), -ETIMEDOUT);
	zassert_equal(data.cmd_posted, 2U);
	zassert_true(data.device_slots[1].used);
	zassert_equal(config.itts[1][0], 0xffU);
	zassert_equal(its_setup_deviceid(&dev, 2U, 1U), -ETIMEDOUT);
}

ZTEST(gicv3_its_reservation, test_mapd_timeout_keeps_itt_when_flush_fails)
{
	data.tables_need_flush = true;
	cache_flush_result = -ENOTSUP;
	fail_mapd = true;
	zassert_equal(its_setup_deviceid(&dev, 2U, 1U), -ETIMEDOUT);
	zassert_true(data.device_slots[1].used);
	zassert_equal(its_setup_deviceid(&dev, 3U, 1U), -ENOMEM);
}

ZTEST(gicv3_its_reservation, test_outer_noncacheable_attributes_require_flush)
{
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_OUTER_CACHE);
	baser_behavior[0].fixed_value =
		MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_BASER_OUTER_CACHE);
	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_OUTER_CACHE);
	cbaser_behavior.fixed_value = MASK_SET(GIC_BASER_CACHE_NCACHEABLE, GITS_CBASER_OUTER_CACHE);

	zassert_ok(gicv3_its_init(&dev));
	zassert_true(flush_order(config.device_table, SIZE_64K) < publication_order[0]);
	zassert_true(flush_order(config.cmd_queue, ITS_CMD_QUEUE_SIZE) < enable_order);
}

ZTEST(gicv3_its_reservation, test_flushes_itt_leaf_pointer_and_command_before_mapd)
{
	const struct its_cmd_block *mapd;

	config.device_id_bits = 16U;
	config.device_table_size = 3U * SIZE_4K;
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_SHAREABILITY);
	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_SHAREABILITY);
	zassert_ok(gicv3_its_init(&dev));
	mapd = data.cmd_write;

	/* DeviceID 512 uses L1 entry 1, whose address differs from the table base. */
	zassert_ok(its_setup_deviceid(&dev, 512U, 1U));
	zassert_equal(MASK_GET(mapd->raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_MAPD);
	zassert_true(flush_order(config.itts, CONFIG_GIC_V3_ITS_MAX_DEVICES * ITS_ITT_SLOT_SIZE) <
		     command_write_order);
	zassert_true(flush_order(&data.indirect_dev_l1_table[1], GITS_LVL1_ENTRY_SIZE) <
		     command_write_order);
	zassert_true(flush_order(mapd, sizeof(*mapd)) < command_write_order);
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
		baser_behavior[i].fixed_mask = 0U;
	}
	zassert_ok(its_setup_tables(&data, &config));
	zassert_false(data.tables_need_flush);
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

ZTEST(gicv3_its_reservation, test_baser_falls_back_to_fitting_page_sizes)
{
	config.device_table_size = SIZE_4K;
	config.collection_table_size = SIZE_16K;

	zassert_ok(its_setup_tables(&data, &config));
	zassert_equal(GITS_BASER_PAGE_SIZE_GET(sys_read64(data.base + GITS_BASER(0))),
		      GITS_BASER_PAGE_SIZE_4K);
	zassert_equal(GITS_BASER_PAGE_SIZE_GET(sys_read64(data.base + GITS_BASER(1))),
		      GITS_BASER_PAGE_SIZE_16K);
}

ZTEST(gicv3_its_reservation, test_baser_uses_indirection_when_direct_table_exceeds_page_limit)
{
	static uint8_t table[BIT(16U) * 24U] __aligned(SIZE_64K);
	uint64_t baser;
	uint64_t valid = MASK(GITS_BASER_VALID);

	config.device_table = table;
	config.device_table_size = sizeof(table);
	config.device_id_bits = 16U;
	data.device_slots[0].used = false;
	sys_write64(MASK_SET(GITS_BASER_TYPE_DEVICE, GITS_BASER_TYPE) |
			    MASK_SET(24U - 1U, GITS_BASER_ENTRY_SIZE),
		    data.base + GITS_BASER(0));
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_PAGE_SIZE);
	baser_behavior[0].rejected_bits = MASK(GITS_BASER_INDIRECT);

	zassert_equal(its_setup_tables(&data, &config), -ENOSPC);
	zassert_equal(publication_order[0], 0U);
	baser_behavior[0].rejected_bits = 0U;
	zassert_ok(its_setup_tables(&data, &config));
	baser = sys_read64(data.base + GITS_BASER(0));
	zassert_equal(GITS_BASER_INDIRECT_GET(baser), 1U);
	zassert_equal(MASK_GET(baser, GITS_BASER_SIZE), 0U);

	/* With 24-byte entries, DeviceIDs 128 and 170 lie on different pages. */
	zassert_ok(gicv3_its_init_device_id(&dev, 128U, 1U));
	zassert_ok(gicv3_its_init_device_id(&dev, 170U, 1U));
	zassert_equal(data.indirect_dev_l1_table[0], (uintptr_t)(table + SIZE_4K) | valid);
	zassert_equal(data.indirect_dev_l1_table[1], (uintptr_t)(table + 2U * SIZE_4K) | valid);
}

ZTEST(gicv3_its_reservation, test_indirect_pages_follow_root)
{
	const size_t page_sizes[] = {SIZE_4K, SIZE_16K, SIZE_64K};

	for (size_t i = 0U; i < ARRAY_SIZE(page_sizes); i++) {
		size_t page_size = page_sizes[i];

		before(NULL);
		config.device_id_bits = 16U;
		config.device_table_size = 3U * page_size;
		data.device_slots[0].used = false;
		sys_write64(sys_read64(data.base + GITS_TYPER) | MASK(GITS_TYPER_DEVBITS),
			    data.base + GITS_TYPER);
		baser_behavior[0].fixed_mask =
			MASK(GITS_BASER_PAGE_SIZE) | MASK(GITS_BASER_SHAREABILITY);
		baser_behavior[0].fixed_value = MASK_SET(i, GITS_BASER_PAGE_SIZE);

		config.device_table_size--;
		zassert_equal(its_setup_tables(&data, &config), -ENOSPC);
		zassert_equal(publication_order[0], 0U);
		zassert_equal(publication_order[1], 0U);
		config.device_table_size++;
		zassert_ok(its_setup_tables(&data, &config));
		zassert_true(flush_order(config.device_table, config.device_table_size) <
			     publication_order[0]);
		zassert_equal(MASK_GET(sys_read64(data.base + GITS_BASER(0)), GITS_BASER_SIZE), 0U);
		zassert_ok(gicv3_its_init_device_id(&dev, 0U, 1U));
		zassert_ok(gicv3_its_init_device_id(&dev, UINT16_MAX, 1U));

		size_t last = UINT16_MAX / (page_size / 8U);
		uint64_t valid = MASK(GITS_BASER_VALID);

		zassert_equal(data.indirect_dev_l1_table[0],
			      (uintptr_t)(config.device_table + page_size) | valid);
		zassert_equal(data.indirect_dev_l1_table[last],
			      (uintptr_t)(config.device_table + 2U * page_size) | valid);
	}
}

ZTEST(gicv3_its_reservation, test_exhausted_indirect_pages_can_still_be_shared)
{
	uint64_t first_leaf;

	config.device_id_bits = 16U;
	config.device_table_size = 3U * SIZE_4K;
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_PAGE_SIZE);
	data.device_slots[0].used = false;
	zassert_ok(its_setup_tables(&data, &config));

	/* Failed MAPD frees the device slot, but the published leaf remains reserved. */
	fail_mapd = true;
	zassert_equal(its_setup_deviceid(&dev, 0U, 1U), -ETIMEDOUT);
	zassert_equal(its_setup_deviceid(&dev, 512U, 1U), -ETIMEDOUT);
	zassert_equal(data.indirect_pages_left, 0U);
	zassert_false(data.device_slots[0].used);

	fail_mapd = false;
	zassert_equal(its_setup_deviceid(&dev, 1024U, 1U), -ENOMEM);
	first_leaf = data.indirect_dev_l1_table[0];
	zassert_ok(its_setup_deviceid(&dev, 1U, 1U));
	zassert_equal(data.indirect_dev_l1_table[0], first_leaf);
	zassert_equal(data.indirect_pages_left, 0U);
}

ZTEST(gicv3_its_reservation, test_baser_rejected_address_is_not_published)
{
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_ADDR);
	zassert_equal(its_setup_tables(&data, &config), -ENOTSUP);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
}

ZTEST(gicv3_its_reservation, test_baser_wide_hardware_accepts_direct_table)
{
	static uint8_t table[KB(512)] __aligned(SIZE_64K);
	uint64_t typer = sys_read64(data.base + GITS_TYPER);
	uint64_t baser;

	config.device_table = table;
	config.device_table_size = sizeof(table);
	config.device_id_bits = 16U;
	sys_write64((typer & ~MASK(GITS_TYPER_DEVBITS)) | MASK_SET(16U, GITS_TYPER_DEVBITS),
		    data.base + GITS_TYPER);
	baser_behavior[0].rejected_bits = MASK(GITS_BASER_INDIRECT);
	zassert_ok(its_setup_tables(&data, &config));
	baser = sys_read64(data.base + GITS_BASER(0));
	zassert_equal(GITS_BASER_INDIRECT_GET(baser), 0U);
	zassert_equal(GITS_BASER_PAGE_SIZE_GET(baser), GITS_BASER_PAGE_SIZE_64K);
	zassert_equal(MASK_GET(baser, GITS_BASER_SIZE), 7U);
	zassert_ok(gicv3_its_init_device_id(&dev, UINT16_MAX, 1U));
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
	config.device_id_bits = 16U;
	config.collection_table_size = SIZE_4K;
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_SHAREABILITY);
	baser_behavior[1].fixed_mask = MASK(GITS_BASER_PAGE_SIZE);
	baser_behavior[1].fixed_value = MASK_SET(GITS_BASER_PAGE_SIZE_64K, GITS_BASER_PAGE_SIZE);
	zassert_equal(its_setup_tables(&data, &config), -ENOSPC);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
	zassert_is_null(data.indirect_dev_l1_table);
	zassert_is_null(data.next_indirect_page);
	zassert_equal(data.indirect_pages_left, 0U);
	zassert_false(data.tables_need_flush);
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
	uint32_t writer;

	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_SHAREABILITY);
	zassert_ok(gicv3_its_init(&dev));
	zassert_true(data.cmd_queue_needs_flush);
	zassert_true(flush_order(config.cmd_queue, ITS_CMD_QUEUE_SIZE) < enable_order);

	data.device_slots[0].event_id_bitmap[0] = BIT(0);

	writer = sys_read32(data.base + GITS_CWRITER);
	cache_flush_result = -ENOTSUP;
	zassert_equal(its_send_int(&dev, 1U, 0U), -ENOTSUP);
	zassert_equal(sys_read32(data.base + GITS_CWRITER), writer);
	cache_flush_result = 0;
	zassert_ok(its_send_int(&dev, 1U, 0U));
	zassert_equal(sys_read32(data.base + GITS_CWRITER), writer + sizeof(struct its_cmd_block));
}

static void assert_queue_wait_is_unlocked(void)
{
	zassert_true(arch_cpu_irqs_are_enabled());
}

ZTEST(gicv3_its_reservation, test_queue_full_wait_releases_lock)
{
	const size_t last_entry = ITS_CMD_QUEUE_NR_ENTRIES - 1U;

	data.device_slots[0].event_id_bitmap[0] = BIT(0);

	/* All usable entries are pending, with the reader still at zero. */
	data.cmd_write = &data.cmd_base[last_entry];
	data.cmd_posted = last_entry;
	data.cmd_base[last_entry - 1U].raw_cmd[0] = MASK_SET(GITS_CMD_ID_INT, GITS_CMD_ID);
	sys_write32(last_entry * sizeof(struct its_cmd_block), data.base + GITS_CWRITER);
	hook_command = GITS_CMD_ID_INT;
	wait_hook = assert_queue_wait_is_unlocked;

	zassert_ok(its_send_int(&dev, 1U, 0U));
	zassert_is_null(wait_hook);
	zassert_equal_ptr(data.cmd_write, data.cmd_base);
	zassert_equal(data.cmd_completed, ITS_CMD_QUEUE_NR_ENTRIES);
}

ZTEST(gicv3_its_reservation, test_queue_wrap_survives_cache_flush_failure)
{
	const size_t last_entry = ITS_CMD_QUEUE_NR_ENTRIES - 1U;
	const uint32_t last_offset = last_entry * sizeof(struct its_cmd_block);

	data.device_slots[0].event_id_bitmap[0] = BIT(0);

	for (size_t i = 0U; i < last_entry; i++) {
		zassert_ok(its_send_int(&dev, 1U, 0U));
	}

	data.cmd_queue_needs_flush = true;
	cache_flush_result = -ENOTSUP;
	zassert_equal(its_send_int(&dev, 1U, 0U), -ENOTSUP);
	zassert_equal_ptr(data.cmd_write, &data.cmd_base[last_entry]);
	zassert_equal(data.cmd_posted, last_entry);
	zassert_equal(data.cmd_completed, last_entry);
	zassert_equal(sys_read32(data.base + GITS_CWRITER), last_offset);

	cache_flush_result = 0;
	zassert_ok(its_send_int(&dev, 1U, 0U));
	zassert_equal_ptr(data.cmd_write, data.cmd_base);
	zassert_equal(sys_read32(data.base + GITS_CWRITER), 0U);
	zassert_ok(its_send_int(&dev, 1U, 0U));
	zassert_equal(data.cmd_posted, ITS_CMD_QUEUE_NR_ENTRIES + 1U);
	zassert_equal(data.cmd_completed, data.cmd_posted);
}

static void assert_init_rolled_back(void)
{
	zassert_equal(GITS_CTLR_ENABLED_GET(sys_read32(data.base + GITS_CTLR)), 0U);
	zassert_equal(sys_read64(data.base + GITS_CBASER) &
			      (MASK(GITS_CBASER_VALID) | MASK(GITS_CBASER_ADDR)),
		      0U);
	for (size_t i = 0U; i < 2U; i++) {
		zassert_equal(sys_read64(data.base + GITS_BASER(i)) &
				      (MASK(GITS_BASER_VALID) | MASK(GITS_BASER_ADDR)),
			      0U);
	}
	zassert_is_null(data.cmd_base);
	zassert_is_null(data.cmd_write);
	zassert_equal(data.cmd_posted, 0U);
	zassert_false(data.cmd_queue_needs_flush);
	zassert_equal(data.lpi_intid_limit, 0U);
}

ZTEST(gicv3_its_reservation, test_cbaser_rejects_address_and_valid_bits)
{
	data.lpi_intid_limit = 0U;
	cbaser_behavior.fixed_mask = MASK(GITS_CBASER_ADDR);
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);
	zassert_true(publication_order[0] > 0U);
	zassert_true(publication_order[1] > 0U);
	assert_init_rolled_back();
	cbaser_behavior.fixed_mask = 0U;
	cbaser_behavior.rejected_bits = MASK(GITS_CBASER_VALID);
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);
	assert_init_rolled_back();
	cbaser_behavior.rejected_bits = 0U;
	zassert_ok(gicv3_its_init(&dev));
}

ZTEST(gicv3_its_reservation, test_noncoherent_table_requires_cache_flush)
{
	data.lpi_intid_limit = 0U;
	baser_behavior[0].fixed_mask = MASK(GITS_BASER_SHAREABILITY);
	cache_flush_result = -ENOTSUP;
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(enable_order, 0U);
	assert_init_rolled_back();
}

ZTEST(gicv3_its_reservation, test_mapc_failure_rolls_back_after_quiescence)
{
	data.lpi_intid_limit = 0U;
	fail_mapc_cpu = 0;
	zassert_equal(gicv3_its_init(&dev), -ETIMEDOUT);
	zassert_true(enable_order > 0U);
	assert_init_rolled_back();

	config.collection_table[0] = 0xffU;
	fail_mapc_cpu = -1;
	zassert_ok(gicv3_its_init(&dev));
	zassert_equal(config.collection_table[0], 0U);
}

ZTEST(gicv3_its_reservation, test_mapc_failure_keeps_tables_until_quiescent)
{
	data.lpi_intid_limit = 0U;
	fail_mapc_cpu = 0;
	fail_quiesce = true;
	zassert_equal(gicv3_its_init(&dev), -ETIMEDOUT);
	zassert_equal(GITS_CTLR_ENABLED_GET(sys_read32(data.base + GITS_CTLR)), 0U);
	zassert_equal(MASK_GET(sys_read64(data.base + GITS_CBASER), GITS_CBASER_VALID), 1U);
	zassert_equal(MASK_GET(sys_read64(data.base + GITS_BASER(0)), GITS_BASER_VALID), 1U);
	zassert_equal(data.lpi_intid_limit, 0U);

	sys_write32(MASK(GITS_CTLR_QUIESCENT), data.base + GITS_CTLR);
	fail_quiesce = false;
	baser_behavior[1].rejected_bits = MASK(GITS_BASER_VALID);
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);
	assert_init_rolled_back();
}

ZTEST(gicv3_its_reservation, test_firmware_lpis_do_not_make_cpu_ready)
{
	uint32_t rdist_ctlr = GICR_CTLR_ENABLE_LPIS;

	gic_rdists[0] = (mem_addr_t)&rdist_ctlr;
	ready_rdists = 0U;
	state.initialized = false;
	zassert_equal(device_init(&dev), -ENODEV);
	zassert_false(device_is_ready(&dev));
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
	zassert_equal(enable_order, 0U);
}

ZTEST(gicv3_its_reservation, test_secondary_mapc_failure_rolls_back_initialization)
{
	ready_rdists = BIT(0) | BIT(1);
	gic_rdists[1] = 2U * SIZE_64K;
	data.lpi_intid_limit = 0U;
	state.initialized = false;
	fail_mapc_cpu = 1;
	zassert_equal(device_init(&dev), -ETIMEDOUT);
	zassert_false(device_is_ready(&dev));
	assert_init_rolled_back();
	zassert_false(atomic_test_bit(data.mapped_collections, 0));
	zassert_false(atomic_test_bit(data.mapped_collections, 1));

	fail_mapc_cpu = -1;
	zassert_ok(device_init(&dev));
	zassert_true(device_is_ready(&dev));
	zassert_equal(data.cmd_completed, 2U);
	zassert_true(atomic_test_bit(data.mapped_collections, 0));
	zassert_true(atomic_test_bit(data.mapped_collections, 1));
	zassert_equal(MASK_GET(config.cmd_queue[1].raw_cmd[2], GITS_CMD_RDBASE), 2U);
}

ZTEST(gicv3_its_reservation, test_mapping_requires_ready_redistributor)
{
	unsigned int intid = UINT_MAX;

	ready_rdists = 0U;
	zassert_equal(its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ENODEV);
	zassert_equal(its_alloc_map_intids(&dev, 1U, 1U, &intid), -ENODEV);
	zassert_equal(intid, UINT_MAX);
	zassert_equal(data.cmd_posted, 0U);

	ready_rdists = BIT(0);
	atomic_clear_bit(data.mapped_collections, 0);
	hook_command = GITS_CMD_ID_MAPC;
	wait_hook = remap_during_mapping;
	zassert_ok(its_alloc_map_intids(&dev, 1U, 1U, &intid));
	zassert_equal(nested_ret, -EBUSY);
	zassert_equal(data.cmd_posted, 3U);
	zassert_equal(intid, GIC_LPI_INT_BASE);
	zassert_equal(MASK_GET(config.cmd_queue[0].raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_MAPC);
	zassert_equal(MASK_GET(config.cmd_queue[1].raw_cmd[0], GITS_CMD_ID), GITS_CMD_ID_MAPTI);
}

ZTEST(gicv3_its_reservation, test_lpi_limit_is_independent_of_event_id_bits)
{
	uint64_t typer = sys_read64(data.base + GITS_TYPER);
	unsigned int intid;

	sys_write64((typer & ~MASK(GITS_TYPER_IDBITS)) | MASK_SET(13U, GITS_TYPER_IDBITS),
		    data.base + GITS_TYPER);
	zassert_ok(gicv3_its_init(&dev));
	zassert_equal(data.lpi_intid_limit, CONFIG_NUM_IRQS);
	zassert_ok(gicv3_its_alloc_intids(&dev, 8192U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE);
	zassert_ok(gicv3_its_alloc_intids(&dev, 1U, &intid));
	zassert_equal(intid, 16384U);
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 0U, intid));
	zassert_ok(gicv3_its_alloc_intids(&dev, CONFIG_NUM_IRQS - 16385U, &intid));
	zassert_equal(intid, 16385U);
	zassert_ok(gicv3_its_map_intid(&dev, 1U, 1U, CONFIG_NUM_IRQS - 1U));
	zassert_equal(gicv3_its_alloc_intids(&dev, 1U, &intid), -ENOSPC);
	zassert_equal(gicv3_its_map_intid(&dev, 1U, 2U, CONFIG_NUM_IRQS), -EINVAL);
}

ZTEST(gicv3_its_reservation, test_init_reserves_lpis_beyond_hardware_limit)
{
	const unsigned int usable_lpis = BIT(14) - GIC_LPI_INT_BASE;
	unsigned int intid;

	gicd_typer = (13U << 19) | GICD_TYPER_LPIS;
	zassert_ok(gicv3_its_init(&dev));
	zassert_true(sys_bitarray_is_region_set(&its_lpi_intids, ITS_LPI_BITMAP_BITS - usable_lpis,
						usable_lpis));
	zassert_ok(its_alloc_intids(&dev, usable_lpis - 2U, &intid));
	zassert_equal(its_alloc_intids(&dev, 3U, &intid), -ENOSPC);
	zassert_ok(its_alloc_intids(&dev, 2U, &intid));
	zassert_equal(intid, BIT(14) - 2U);
}

ZTEST(gicv3_its_reservation, test_distributor_num_lpis_limits_allocation)
{
	const unsigned int lpi_count = BIT(14);
	unsigned int intid;

	gicd_typer |= 13U << GICD_TYPER_NUM_LPIS_SHIFT;
	zassert_ok(gicv3_its_init(&dev));
	zassert_equal(data.lpi_intid_limit, GIC_LPI_INT_BASE + lpi_count);
	zassert_ok(its_alloc_intids(&dev, lpi_count, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE);
	zassert_equal(its_alloc_intids(&dev, 1U, &intid), -ENOSPC);
}

ZTEST(gicv3_its_reservation, test_init_rejects_distributor_without_usable_lpis)
{
	gicd_typer &= ~GICD_TYPER_LPIS;
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);

	gicd_typer = (12U << 19) | GICD_TYPER_LPIS;
	zassert_equal(gicv3_its_init(&dev), -ENOTSUP);

	zassert_equal(enable_order, 0U);
	zassert_equal(publication_order[0], 0U);
	zassert_equal(publication_order[1], 0U);
}

ZTEST(gicv3_its_reservation, test_small_event_id_space_limits_device_vectors)
{
	uint64_t typer = sys_read64(data.base + GITS_TYPER);

	sys_write64((typer & ~MASK(GITS_TYPER_IDBITS)) | MASK_SET(4U, GITS_TYPER_IDBITS),
		    data.base + GITS_TYPER);
	zassert_ok(gicv3_its_init(&dev));
	zassert_equal(gicv3_its_init_device_id(&dev, 2U, 33U), -ENOTSUP);
	zassert_ok(gicv3_its_init_device_id(&dev, 2U, 32U));
	zassert_ok(gicv3_its_map_intid(&dev, 2U, 31U, 16384U));
	zassert_equal(gicv3_its_map_intid(&dev, 2U, 32U, 16385U), -EINVAL);
}

static unsigned int legacy_alloc_intid(const struct device *dev)
{
	ARG_UNUSED(dev);

	return GIC_LPI_INT_BASE;
}

TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

ZTEST(gicv3_its_reservation, test_unready_its_does_not_allocate_or_issue_commands)
{
	struct device_state ready_state = {.initialized = true};
	struct device ready_dev = dev;
	unsigned int intid = UINT_MAX;

	ready_dev.state = &ready_state;
	state.initialized = false;
	zassert_equal(its_setup_deviceid(&dev, 1U, 1U), -ENODEV);
	zassert_equal(its_alloc_intids(&dev, 1U, &intid), -ENODEV);
	zassert_equal(its_alloc_intid(&dev), 0);
	zassert_equal(its_alloc_map_intids(&dev, 1U, 1U, &intid), -ENODEV);
	zassert_equal(its_map_intid(&dev, 1U, 0U, GIC_LPI_INT_BASE), -ENODEV);
	zassert_equal(its_send_int(&dev, 1U, 0U), -ENODEV);
	zassert_equal(intid, UINT_MAX);
	zassert_equal(data.cmd_posted, 0U);
	zassert_ok(its_alloc_intids(&ready_dev, 1U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE);
}

ZTEST(gicv3_its_reservation, test_legacy_driver_api)
{
	static DEVICE_API(its, legacy_api) = {.alloc_intid = legacy_alloc_intid};
	const struct device legacy_dev = {.api = &legacy_api};

	zassert_equal(its_alloc_intid(&legacy_dev), GIC_LPI_INT_BASE);
}

ZTEST(gicv3_its_reservation, test_legacy_intid_allocation)
{
	unsigned int intid;

	zassert_equal(its_alloc_intid(&dev), GIC_LPI_INT_BASE);
	zassert_ok(its_alloc_intids(&dev, 1U, &intid));
	zassert_equal(intid, GIC_LPI_INT_BASE + 1U);
	zassert_ok(its_alloc_intids(&dev, CONFIG_NUM_IRQS - GIC_LPI_INT_BASE - 2U, &intid));
	zassert_equal(its_alloc_intid(&dev), 0);
	intid = UINT_MAX;
	zassert_equal(its_alloc_intids(&dev, 1U, &intid), -ENOSPC);
	zassert_equal(intid, UINT_MAX);
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

ZTEST_SUITE(gicv3_its_reservation, NULL, NULL, before, NULL, NULL);
