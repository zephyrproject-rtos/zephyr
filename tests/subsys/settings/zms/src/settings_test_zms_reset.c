/*
 * Copyright (c) 2026 Classified Cycling BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Reset test for the settings ZMS backend on flash without explicit erase.
 *
 * ZMS keeps each setting as a name, a value and a node of a linked list. The
 * settings backend loads the settings through this linked list. If ZMS loses
 * entries after a reset, the linked list can break. The backend then does
 * not load the settings after the break, although their names are still in ZMS.
 * A save of such a setting is successful, but the backend does not load it.
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/flash_simulator.h>
#include <zephyr/kvss/zms.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

#include <settings_priv.h>
#include <settings/settings_zms.h>
#include "zms_priv.h"

#define AREA_DEV    PARTITION_DEVICE(storage_partition)
#define AREA_OFFSET PARTITION_OFFSET(storage_partition)
#define AREA_SIZE   PARTITION_SIZE(storage_partition)

int settings_backend_init(void);

static struct {
	/* The reset occurs at the first write to this sector */
	int sector;
	bool occurred;
} reset = {.sector = -1};

static struct zms_fs *settings_fs(void)
{
	void *storage = NULL;

	zassert_ok(settings_storage_get(&storage));
	zassert_not_null(storage);
	return storage;
}

static int reset_write_byte(const struct device *dev, off_t offset, uint8_t data)
{
	ARG_UNUSED(dev);

	if (reset.sector >= 0 &&
	    (offset - AREA_OFFSET) / settings_fs()->sector_size == reset.sector) {
		reset.occurred = true;
	}

	/* After the reset, the flash content does not change */
	return reset.occurred ? -EIO : data;
}

static const struct flash_simulator_cb reset_callbacks = {
	.write_byte = reset_write_byte,
};

/*
 * Start again after a reset: let the flash writes continue and initialize the
 * settings backend with a cleared state
 */
static void restart(void)
{
	flash_simulator_set_callbacks(AREA_DEV, NULL);
	reset.sector = -1;
	reset.occurred = false;

	sys_slist_init(&settings_load_srcs);
	settings_save_dst = NULL;
	zassert_ok(settings_backend_init(), "settings backend init failed");
}

static uint32_t write_sector(void)
{
	return settings_fs()->ate_wra >> ADDR_SECT_SHIFT;
}

static uint32_t sector_after(uint32_t sector)
{
	return (sector + 1) % settings_fs()->sector_count;
}

static void change_write_sector(uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		zassert_ok(zms_sector_use_next(settings_fs()));
	}
}

/* Erase the settings area. ZMS did not use any of its sectors before. */
static void erase_settings_area(void)
{
	size_t mem_size;

	memset((uint8_t *)flash_simulator_get_memory(AREA_DEV, &mem_size) + AREA_OFFSET,
	       flash_get_parameters(AREA_DEV)->erase_value, AREA_SIZE);
}

/*
 * Use all sectors one time while ZMS contains no entries. Each garbage
 * collection copies no entries, thus each sector gets a gc_done ATE in its first
 * ATE position. At mount, ZMS looks for a gc_done ATE there when the write
 * sector is empty.
 */
static void use_all_sectors_without_entries(void)
{
	struct flash_pages_info info;
	struct zms_fs fs = {
		.flash_device = AREA_DEV,
		.offset = AREA_OFFSET,
	};

	zassert_ok(flash_get_page_info_by_offs(AREA_DEV, AREA_OFFSET, &info));
	fs.sector_size = info.size * CONFIG_SETTINGS_ZMS_SECTOR_SIZE_MULT;
	fs.sector_count = AREA_SIZE / fs.sector_size;
	zassert_ok(zms_mount(&fs));

	for (uint32_t i = 0; i < fs.sector_count; i++) {
		zassert_ok(zms_sector_use_next(&fs));
	}
}

static int load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
	uint32_t *value = param;

	if (strcmp(key, "b") == 0 && len == sizeof(*value)) {
		zassert_equal(read_cb(cb_arg, value, sizeof(*value)), sizeof(*value));
	}

	return 0;
}

/**
 * @brief Test that the backend loads a setting that you save after a reset.
 *
 * A sector that ZMS used before still has the gc_done ATE of that use. The
 * reset occurs when ZMS changes the write sector, before the garbage
 * collection. A power loss, a watchdog reset or a software reset can cause
 * this. At the next initialization, ZMS erases the oldest sector without
 * a copy of its entries.
 */
ZTEST(settings_zms_reset, test_setting_saved_after_reset_is_loaded)
{
	const uint32_t first = 1;
	const uint32_t second = 2;
	uint32_t loaded = 0;

	if (flash_params_get_erase_cap(flash_get_parameters(AREA_DEV)) & FLASH_ERASE_C_EXPLICIT) {
		ztest_test_skip();
	}

	erase_settings_area();
	use_all_sectors_without_entries();
	restart();

	/* Save setting "test/a". Then make its sector the oldest sector. */
	zassert_ok(settings_save_one("test/a", &first, sizeof(first)));
	change_write_sector(settings_fs()->sector_count - 2);

	/* Save setting "test/b" in the write sector */
	zassert_ok(settings_save_one("test/b", &first, sizeof(first)));

	/* Change the write sector. The reset occurs before the garbage collection. */
	reset.sector = sector_after(write_sector());
	flash_simulator_set_callbacks(AREA_DEV, &reset_callbacks);
	(void)zms_sector_use_next(settings_fs());
	zassert_true(reset.occurred, "no reset");
	restart();

	zassert_ok(settings_save_one("test/b", &second, sizeof(second)));
	restart();

	zassert_ok(settings_load_subtree_direct("test", load_cb, &loaded));
	zassert_equal(loaded, second, "loaded value of test/b is %u, expected %u", loaded, second);
}

/**
 * @brief Test that the backend loads a setting again that is not in the linked list.
 *
 * The test breaks the linked list after setting "test/a", as the recovery of a
 * broken linked list does. Then setting "test/b" is not in the linked list, but
 * its name and its value are still in ZMS. At the next initialization, the
 * backend must add "test/b" to the linked list again.
 */
ZTEST(settings_zms_reset, test_setting_not_in_linked_list_is_loaded)
{
	const uint32_t first = 1;
	const uint32_t second = 2;
	struct settings_hash_linked_list head;
	struct settings_hash_linked_list node_a;
	uint32_t loaded = 0;

	Z_TEST_SKIP_IFNDEF(CONFIG_SETTINGS_ZMS_RELINK_NAMES);

	erase_settings_area();
	restart();

	/* The linked list is: head, "test/a", "test/b" */
	zassert_ok(settings_save_one("test/a", &first, sizeof(first)));
	zassert_ok(settings_save_one("test/b", &first, sizeof(first)));

	/* Break the linked list after "test/a" */
	zassert_equal(zms_read(settings_fs(), ZMS_LL_HEAD_HASH_ID, &head, sizeof(head)),
		      sizeof(head));
	zassert_equal(zms_read(settings_fs(), head.next_hash, &node_a, sizeof(node_a)),
		      sizeof(node_a));
	node_a.next_hash = 0;
	zassert_equal(zms_write(settings_fs(), head.next_hash, &node_a, sizeof(node_a)),
		      sizeof(node_a));
	restart();

	zassert_ok(settings_load_subtree_direct("test", load_cb, &loaded));
	zassert_equal(loaded, first, "loaded value of test/b is %u, expected %u", loaded, first);

	zassert_ok(settings_save_one("test/b", &second, sizeof(second)));
	restart();

	zassert_ok(settings_load_subtree_direct("test", load_cb, &loaded));
	zassert_equal(loaded, second, "loaded value of test/b is %u, expected %u", loaded, second);
}

ZTEST_SUITE(settings_zms_reset, NULL, NULL, NULL, NULL, NULL);
