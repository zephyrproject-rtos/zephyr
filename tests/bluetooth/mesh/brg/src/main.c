/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/mesh.h>
#include <stdlib.h>

#include "settings.h"
#include "brg_cfg.h"

#define TEST_VECT_SZ (CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX + 1)

static struct test_brg_cfg_row {
	uint8_t direction;
	uint16_t net_idx1;
	uint16_t net_idx2;
	uint16_t addr1;
	uint16_t addr2;
} test_vector[TEST_VECT_SZ];

#define ADDR1_BASE (1)
#define ADDR2_BASE (100)

/**** Helper functions ****/
static void setup(void *f)
{
	/* create test vector */
	for (int i = 0; i < TEST_VECT_SZ; i++) {
		test_vector[i].direction = i < (TEST_VECT_SZ / 2) ? 1 : 2;
		test_vector[i].net_idx1 = (i/8);
		test_vector[i].addr1 = ADDR1_BASE + i;
		test_vector[i].net_idx2 = (i/8) + 16;
		test_vector[i].addr2 = ADDR2_BASE + i;
	}

}

/**** Mocked functions ****/

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
	ztest_check_expected_value(flag);
}

int settings_save_one(const char *name, const void *value, size_t val_len)
{
	ztest_check_expected_data(name, strlen(name));
	ztest_check_expected_value(val_len);
	ztest_check_expected_data(value, val_len);
	return 0;
}

int settings_delete(const char *name)
{
	ztest_check_expected_data(name, strlen(name));
	return 0;
}

/**** Mocked functions - end ****/

static void check_fill_all_bt_entries(void)
{
	int err;

	for (int i = 0; i < TEST_VECT_SZ; i++) {

		if (i < CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX) {
			ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
		}

		err = bt_mesh_brg_cfg_tbl_add(test_vector[i].direction, test_vector[i].net_idx1,
			test_vector[i].net_idx2, test_vector[i].addr1, test_vector[i].addr2);

		if (i != CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX) {
			zassert_equal(err, 0);
		} else {
			zassert_equal(err, -ENOMEM);
		}
	}
}

static void check_delete_all_bt_entries(void)
{
	for (int i = 0; i < TEST_VECT_SZ; i++) {

		if (i < CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX) {
			ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
		}

		bt_mesh_brg_cfg_tbl_remove(test_vector[i].net_idx1, test_vector[i].net_idx2,
			test_vector[i].addr1, test_vector[i].addr2);
	}
}


static void check_bt_mesh_brg_cfg_tbl_reset(void)
{
	int err;

	ztest_expect_data(settings_delete, name, "bt/mesh/brg_en");
	ztest_expect_data(settings_delete, name, "bt/mesh/brg_tbl");
	err = bt_mesh_brg_cfg_tbl_reset();
	zassert_equal(err, 0);
}

/**** Tests ****/

ZTEST_SUITE(bt_mesh_brg_cfg, NULL, NULL, setup, NULL, NULL);

/* Test if basic functionality (add and remove entries) works correctly. */
ZTEST(bt_mesh_brg_cfg, test_basic_functionality_storage)
{
	check_bt_mesh_brg_cfg_tbl_reset();

	/* Test add entries to bridging table. */
	check_fill_all_bt_entries();

	/* Test remove entries from bridging table, and then fill it again. */
	check_delete_all_bt_entries();
	check_fill_all_bt_entries();

	/* Test resetting of the table, and then fill it again. */
	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries();

	/* Test remove entries matching netkey1, and netkey2 */
	uint16_t net_idx1 = test_vector[TEST_VECT_SZ - 1].net_idx1;
	uint16_t net_idx2 = test_vector[TEST_VECT_SZ - 1].net_idx2;
	uint16_t addr1 = BT_MESH_ADDR_UNASSIGNED;
	uint16_t addr2 = BT_MESH_ADDR_UNASSIGNED;

	bt_mesh_brg_cfg_tbl_remove(net_idx1, net_idx2, addr1, addr2);

	const struct bt_mesh_brg_cfg_row *brg_tbl;
	int n = bt_mesh_brg_cfg_tbl_get(&brg_tbl);

	zassert_true(n > 0);

	for (int i = 0; i < n; i++) {
		zassert_true(brg_tbl[i].net_idx1 != net_idx1);
		zassert_true(brg_tbl[i].net_idx2 != net_idx2);
	}

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries();

	/* Test remove entries matching netkey1, and netkey2, and addr1 */
	addr1 = test_vector[TEST_VECT_SZ - 1].addr1;
	n = bt_mesh_brg_cfg_tbl_get(&brg_tbl);

	zassert_true(n > 0);

	for (int i = 0; i < n; i++) {
		zassert_true(brg_tbl[i].net_idx1 != net_idx1);
		zassert_true(brg_tbl[i].net_idx2 != net_idx2);
		zassert_true(brg_tbl[i].addr1 != addr1);
	}

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries();

	/* Test remove entries matching netkey1, and netkey2, and addr2 */
	addr1 = BT_MESH_ADDR_UNASSIGNED;
	addr2 = test_vector[TEST_VECT_SZ - 1].addr2;
	n = bt_mesh_brg_cfg_tbl_get(&brg_tbl);

	zassert_true(n > 0);

	for (int i = 0; i < n; i++) {
		zassert_true(brg_tbl[i].net_idx1 != net_idx1);
		zassert_true(brg_tbl[i].net_idx2 != net_idx2);
		zassert_true(brg_tbl[i].addr2 != addr2);
	}
}

static void pending_store_enable_create_expectations(bool *enable_val,
		int n, const struct bt_mesh_brg_cfg_row *tbl_val)
{
	if (*enable_val) {
		ztest_expect_data(settings_save_one, name, "bt/mesh/brg_en");
		ztest_expect_value(settings_save_one, val_len, 1);
		ztest_expect_data(settings_save_one, value, enable_val);
	} else {
		ztest_expect_data(settings_delete, name, "bt/mesh/brg_en");
	}

	if (n > 0) {
		ztest_expect_data(settings_save_one, name, "bt/mesh/brg_tbl");
		ztest_expect_value(settings_save_one, val_len,
					n * sizeof(struct bt_mesh_brg_cfg_row));
		ztest_expect_data(settings_save_one, value, tbl_val);
	} else {
		ztest_expect_data(settings_delete, name, "bt/mesh/brg_tbl");
	}
}

/* Test if enable flag is stored correctly. */
ZTEST(bt_mesh_brg_cfg, test_brg_cfg_en)
{
	int err;
	int n;
	bool val;
	const struct bt_mesh_brg_cfg_row *tbl;

	check_bt_mesh_brg_cfg_tbl_reset();
	val = bt_mesh_brg_cfg_enable_get();
	n = bt_mesh_brg_cfg_tbl_get(&tbl);
	zassert_equal(val, false, NULL);
	pending_store_enable_create_expectations(&val, n, tbl);
	bt_mesh_brg_cfg_pending_store();


	ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
	err = bt_mesh_brg_cfg_enable_set(true);
	zassert_equal(err, 0, NULL);
	val = bt_mesh_brg_cfg_enable_get();
	n = bt_mesh_brg_cfg_tbl_get(&tbl);
	pending_store_enable_create_expectations(&val, n, tbl);
	bt_mesh_brg_cfg_pending_store();

	zassert_equal(bt_mesh_brg_cfg_enable_get(), true, NULL);
}

/* Test if pending store works correctly by adding one entry to the table. */
ZTEST(bt_mesh_brg_cfg, test_brg_tbl_pending_store)
{
	int n, err;
	bool b_en;
	struct bt_mesh_brg_cfg_row test_vec = {
		.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 1,
		.net_idx2 = 2,
		.addr1 = 3,
		.addr2 = 4,
	};

	check_bt_mesh_brg_cfg_tbl_reset();
	ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
	err = bt_mesh_brg_cfg_tbl_add(test_vec.direction, test_vec.net_idx1,
		test_vec.net_idx2, test_vec.addr1, test_vec.addr2);
	zassert_equal(err, 0);

	const struct bt_mesh_brg_cfg_row *tbl;

	n = bt_mesh_brg_cfg_tbl_get(&tbl);
	b_en = bt_mesh_brg_cfg_enable_get();

	zassert_equal(n, 1);
	zassert_true(tbl);

	pending_store_enable_create_expectations(&b_en, 1, &test_vec);
	bt_mesh_brg_cfg_pending_store();
}

/* Test if invalid entries are not added to the table. */
ZTEST(bt_mesh_brg_cfg, test_tbl_add_invalid_ip)
{
	int err;
	/* Create test vector array of test_brg_cfg_row iteams with invalid values.
	 * Each vector has only one invalid field value, rest all are valid values.
	 */
	const struct test_brg_cfg_row inv_test_vector[] = {
	/* Direction has invalid values */
	{.direction = BT_MESH_BRG_CFG_DIR_PROHIBITED,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 2},
	{.direction = BT_MESH_BRG_CFG_DIR_MAX,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 2},
	/* Out of range netidx values */
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 4096, .net_idx2 = 1, .addr1 = 1, .addr2 = 2},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 4096, .addr1 = 1, .addr2 = 2},
	/* Same netidx values */
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 0, .addr1 = 1, .addr2 = 2},
	/* Same addr values */
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 1},
	/* Invalid address1 value */
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 0, .addr2 = 1},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 0x8000, .addr2 = 1},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 0xC000, .addr2 = 1},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 0xFFFE, .addr2 = 1},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 0xFFFF, .addr2 = 1},
	/* Invalid address2 values */
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0},
	{.direction = BT_MESH_BRG_CFG_DIR_ONEWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0xFFFF},
	{.direction = BT_MESH_BRG_CFG_DIR_TWOWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0x8000},
	{.direction = BT_MESH_BRG_CFG_DIR_TWOWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0xC000},
	{.direction = BT_MESH_BRG_CFG_DIR_TWOWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0xFFFE},
	{.direction = BT_MESH_BRG_CFG_DIR_TWOWAY,
		.net_idx1 = 0, .net_idx2 = 1, .addr1 = 1, .addr2 = 0xFFFF},
	};

	check_bt_mesh_brg_cfg_tbl_reset();

	for (int i = 0; i < ARRAY_SIZE(inv_test_vector); i++) {
		err = bt_mesh_brg_cfg_tbl_add(inv_test_vector[i].direction,
					inv_test_vector[i].net_idx1, inv_test_vector[i].net_idx2,
					inv_test_vector[i].addr1, inv_test_vector[i].addr2);
		zassert_equal(err, -EINVAL, "Test vector index: %zu", i);
	}
}


/* Following are helper functions for the test that checks the iteration logic */
#define NUM_MSGS (10000)

static void print_brg_tbl(void)
{
	const struct bt_mesh_brg_cfg_row *tbl;
	int n = bt_mesh_brg_cfg_tbl_get(&tbl);

	zassert_true(n <= CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX);

	for (int i = 0; i < n; i++) {
		printk("entry: %3d # dir: %d, net_idx1: %3d, addr1: %3d, net_idx2: %3d, addr2: %3d\n",
			i, tbl[i].direction, tbl[i].net_idx1, tbl[i].addr1, tbl[i].net_idx2,
			tbl[i].addr2);
	}
}

static void check_fill_all_bt_entries_reversed(void)
{
	int err;

	for (int i = TEST_VECT_SZ - 2; i >= 0 ; i--) {
		ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
		err = bt_mesh_brg_cfg_tbl_add(test_vector[i].direction, test_vector[i].net_idx1,
			test_vector[i].net_idx2, test_vector[i].addr1, test_vector[i].addr2);
		zassert_equal(err, 0);
	}

	int last = TEST_VECT_SZ - 1;

	err = bt_mesh_brg_cfg_tbl_add(test_vector[last].direction, test_vector[last].net_idx1,
	test_vector[last].net_idx2, test_vector[last].addr1, test_vector[last].addr2);
	zassert_equal(err, -ENOMEM);
}

static struct test_brg_cfg_row test_vector_copy[TEST_VECT_SZ - 1];

static void check_fill_all_bt_entries_randomly(void)
{
	int err;
	int copy_cnt = ARRAY_SIZE(test_vector_copy);

	memcpy(test_vector_copy, test_vector, sizeof(test_vector_copy));

	for (int i = 0; i < copy_cnt; i++) {
		int idx = rand() % copy_cnt;
		struct test_brg_cfg_row tmp = test_vector_copy[i];

		test_vector_copy[i] = test_vector_copy[idx];
		test_vector_copy[idx] = tmp;
	}

	for (int i = 0; i < copy_cnt; i++) {
		ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				BT_MESH_SETTINGS_BRG_PENDING);
		err = bt_mesh_brg_cfg_tbl_add(test_vector_copy[i].direction,
			test_vector_copy[i].net_idx1, test_vector_copy[i].net_idx2,
			test_vector_copy[i].addr1, test_vector_copy[i].addr2);
		zassert_equal(err, 0);
	}

	int last = TEST_VECT_SZ - 1;

	err = bt_mesh_brg_cfg_tbl_add(test_vector[last].direction, test_vector[last].net_idx1,
		test_vector[last].net_idx2, test_vector[last].addr1, test_vector[last].addr2);
	zassert_equal(err, -ENOMEM);
}

static void subnet_relay_cb_check(uint16_t new_net_idx, void *user_data)
{
	int idx = *(int *)user_data;

	zassert_equal(new_net_idx, test_vector[idx].net_idx2);
}

static void subnet_relay_cb_check_rev(uint16_t new_net_idx, void *user_data)
{
	int idx = *(int *)user_data;

	if (test_vector[idx].direction == 2) {
		zassert_equal(new_net_idx, test_vector[idx].net_idx1);
	} else {
		/* Should never assert. Test vector created in setup(). */
		zassert_true(false);
	}
}

static void test_bridging_performance(bool test_one_way)
{
	int idx;
	uint32_t tick1;
	uint32_t ticks = 0;

	for (int i = 0; i < NUM_MSGS; i++) {
		/* randomly pick an entry from the test vector */
		idx = rand() % TEST_VECT_SZ;

		/* check src to dst bridging*/
		const struct bt_mesh_brg_cfg_row *tbl_row = NULL;

		tick1 = k_uptime_ticks();
		bt_mesh_brg_cfg_tbl_foreach_subnet(test_vector[idx].addr1, test_vector[idx].addr2,
			test_vector[idx].net_idx1, subnet_relay_cb_check, &idx);
		ticks += k_uptime_ticks() - tick1;

		if (test_one_way) {
			continue;
		}

		/* check dst to src bridging - for the same test vector src-dst pairs
		 * but now, reverse them and consider packets are arriving on net_idx2
		 */
		tbl_row = NULL;
		tick1 = k_uptime_ticks();
		bt_mesh_brg_cfg_tbl_foreach_subnet(test_vector[idx].addr2, test_vector[idx].addr1,
			test_vector[idx].net_idx2, subnet_relay_cb_check_rev, &idx);
		ticks += k_uptime_ticks() - tick1;
	}
	printk("ticks: %8u  us: %u\n", ticks, k_ticks_to_us_floor32(ticks));
}

/* Test checks iteration logic and performance when run on real devices. */
ZTEST(bt_mesh_brg_cfg, test_zcheck_entry_randomly_sorting)
{
	printk("num msgs: %d\n\n", NUM_MSGS);

	/* Test performance when packets are flowing in one directions */
	/* Fill bridging table in sorted order */
	printk("\n\nPackets going only in one direction (from outside towards the subnet)\n");
	printk("\nBridging table is pre-filled in sorted order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries();
	print_brg_tbl();
	test_bridging_performance(true);

	/* Fill bridging table in reversed order */
	printk("\nBridging table is pre-filled in reversed order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries_reversed();
	print_brg_tbl();
	test_bridging_performance(true);

	/* Fill bridging table in random order */
	printk("\nBridging table is pre-filled in random order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries_randomly();
	print_brg_tbl();
	test_bridging_performance(true);

	/* Test performance when packets are flowing in both directions - use same dataset. */
	printk("\n\nPackets going in both directions (same data set, flip src and dst pairs)\n");
	printk("\nBridging table is pre-filled in sorted order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries();
	print_brg_tbl();
	test_bridging_performance(false);

	/* Fill bridging table in reversed order */
	printk("\nBridging table is pre-filled in reversed order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries_reversed();
	print_brg_tbl();
	test_bridging_performance(false);

	/* Fill bridging table in random order */
	printk("\nBridging table is pre-filled in random order\n");

	check_bt_mesh_brg_cfg_tbl_reset();
	check_fill_all_bt_entries_randomly();
	print_brg_tbl();
	test_bridging_performance(false);
}
