/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/mesh.h>

#include "settings.h"
#include "net.h"
#include "rpl.h"

#define EMPTY_ENTRIES_CNT (CONFIG_BT_MESH_CRPL - ARRAY_SIZE(test_vector))

/* Used for cleaning RPL without checking it. */
static bool skip_delete;

/* Test function that should call bt_mesh_rpl_check(). */
static enum {
	SETTINGS_SAVE_ONE = 1,
	SETTINGS_DELETE
} settings_func;
/* Number of `settings_func` calls before calling bt_mesh_rpl_check() (1 means first call). */
static int settings_func_cnt;
/* Received message context. */
static struct bt_mesh_net_rx recv_msg;

/* We will change test vector during the test as it is convenient to do so. Therefore, we need
 * to keep default values separately.
 */
static const struct test_rpl_entry {
	char *name;
	uint16_t src;
	bool old_iv;
	uint32_t seq;
} test_vector_default[] = {
	{ .name = "bt/mesh/RPL/1",  .src = 0x1,  .old_iv = false, .seq = 10, },
	{ .name = "bt/mesh/RPL/17", .src = 0x17, .old_iv = true,  .seq = 32, },
	{ .name = "bt/mesh/RPL/7c", .src = 0x7c, .old_iv = false, .seq = 20, },
	{ .name = "bt/mesh/RPL/2c", .src = 0x2c, .old_iv = true,  .seq = 5,  },
	{ .name = "bt/mesh/RPL/5a", .src = 0x5a, .old_iv = true,  .seq = 12, },
};
static struct test_rpl_entry test_vector[ARRAY_SIZE(test_vector_default)];

/**** Helper functions ****/

static void prepare_rpl_and_start_reset(void)
{
	/* Add test vector to RPL. */
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_net_rx msg = {
			.local_match = true,
			.ctx.addr = test_vector[i].src,
			.old_iv = test_vector[i].old_iv,
			.seq = test_vector[i].seq,
		};

		ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				   BT_MESH_SETTINGS_RPL_PENDING);
		zassert_false(bt_mesh_rpl_check(&msg, NULL));
	}

	/* settings_save_one() will be triggered for all new entries when
	 * bt_mesh_rpl_pending_store() is called.
	 */
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		ztest_expect_data(settings_save_one, name, test_vector[i].name);
	}
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	/* Check that all added entries are in RPL. */
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_net_rx msg = {
			.local_match = true,
			.ctx.addr = test_vector[i].src,
			.old_iv = test_vector[i].old_iv,
			.seq = test_vector[i].seq,
		};

		zassert_true(bt_mesh_rpl_check(&msg, NULL));
	}

	/* Simulate IVI Update. This should only flip flags. The actual storing will happen
	 * when bt_mesh_rpl_pending_store() is called.
	 */
	ztest_expect_value(bt_mesh_settings_store_schedule, flag, BT_MESH_SETTINGS_RPL_PENDING);
	bt_mesh_rpl_reset();
}

/* Should be called after the reset operation is finished. */
static void check_entries_from_test_vector(void)
{
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		struct bt_mesh_net_rx msg = {
			.local_match = true,
			.ctx.addr = test_vector[i].src,
			/* Entries with old_iv == true should have been deleted. old_iv in entries
			 * is flipped, so to check this we can try to add the removed entries
			 * again. RPL should accept them.
			 */
			.old_iv = !test_vector[i].old_iv,
			.seq = test_vector[i].seq
		};

		/* Removed entries can now be added again. */
		if (test_vector[i].old_iv) {
			ztest_expect_value(bt_mesh_settings_store_schedule, flag,
					   BT_MESH_SETTINGS_RPL_PENDING);
			zassert_false(bt_mesh_rpl_check(&msg, NULL));
		} else {
			zassert_true(bt_mesh_rpl_check(&msg, NULL));
		}
	}
}

static void check_empty_entries(int cnt)
{
	/* Check that RPL has the specified amount of empty entries. */
	for (int i = 0; i < cnt; i++) {
		struct bt_mesh_net_rx msg = {
			.local_match = true,
			.ctx.addr = 0x7fff - i,
			.old_iv = false,
			.seq = i,
		};

		ztest_expect_value(bt_mesh_settings_store_schedule, flag,
				   BT_MESH_SETTINGS_RPL_PENDING);
		zassert_false(bt_mesh_rpl_check(&msg, NULL));
	}

	/* Check that there are no more empty entries in RPL. */
	struct bt_mesh_net_rx msg = {
		.local_match = true,
		.ctx.addr = 0x1024,
		.old_iv = false,
		.seq = 1024,
	};
	zassert_true(bt_mesh_rpl_check(&msg, NULL));
}

static void check_op(int op)
{
	if (settings_func == op) {
		if (settings_func_cnt > 1) {
			settings_func_cnt--;
		} else {
			ztest_expect_value(bt_mesh_settings_store_schedule, flag,
					   BT_MESH_SETTINGS_RPL_PENDING);
			zassert_false(bt_mesh_rpl_check(&recv_msg, NULL));

			settings_func_cnt--;
			settings_func = 0;
		}
	}
}

static void call_rpl_check_on(int func, int cnt, struct test_rpl_entry *entry)
{
	settings_func = func;
	settings_func_cnt = cnt;

	recv_msg.local_match = true;
	recv_msg.ctx.addr = entry->src;
	recv_msg.seq = entry->seq;
	recv_msg.old_iv = entry->old_iv;
}

static void expect_pending_store(void)
{
	/* Entries with old_iv == true should be removed, others should be stored. */
	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		if (test_vector[i].old_iv) {
			ztest_expect_value(settings_delete, name, test_vector[i].name);
		} else {
			ztest_expect_data(settings_save_one, name, test_vector[i].name);
		}
	}
}

static bool is_rpl_check_called(void)
{
	return settings_func_cnt == 0;
}

static void verify_rpl(void)
{
	check_entries_from_test_vector();
	check_empty_entries(EMPTY_ENTRIES_CNT);
}

static void setup(void *f)
{
	/* Restore test vector. */
	memcpy(test_vector, test_vector_default, sizeof(test_vector));

	/* Clear RPL before every test. */
	skip_delete = true;
	ztest_expect_value(bt_mesh_settings_store_schedule, flag, BT_MESH_SETTINGS_RPL_PENDING);
	bt_mesh_rpl_clear();
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	skip_delete = false;

	settings_func = 0;
	settings_func_cnt = 0;
}

/**** Mocked functions ****/

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
	ztest_check_expected_value(flag);
}

void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag)
{
}

int settings_save_one(const char *name, const void *value, size_t val_len)
{
	ztest_check_expected_data(name, strlen(name));

	check_op(SETTINGS_SAVE_ONE);

	return 0;
}

int settings_delete(const char *name)
{
	if (skip_delete) {
		return 0;
	}

	ztest_check_expected_data(name, strlen(name));

	check_op(SETTINGS_DELETE);

	return 0;
}

/**** Tests ****/

ZTEST_SUITE(bt_mesh_rpl_reset, NULL, NULL, setup, NULL, NULL);

/** Test that entries with old_iv == true are removed after the reset operation finished. */
ZTEST(bt_mesh_rpl_reset, test_reset_normal)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a valid entry that was just deleted. The entry should be
 * stored after the reset operation is finished.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_delete_same_entry)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	/* Take the first entry with old_iv == true and simulate msg reception with same src
	 * address and correct IVI after the entry was deleted.
	 */
	struct test_rpl_entry *entry = &test_vector[1];

	zassert_true(entry->old_iv);
	entry->old_iv = false;
	call_rpl_check_on(SETTINGS_DELETE, 1, entry);

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* Call bt_mesh_rpl_pending_store() to store new entry. */
	ztest_expect_data(settings_save_one, name, entry->name);
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a valid entry that was just stored. The entry should be stored
 * after the reset operation is finished.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_save_same_entry)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	/* Take the first entry with old_iv == false and simulate msg reception with same src
	 * address and correct IVI after the entry was stored.
	 */
	struct test_rpl_entry *entry = &test_vector[0];

	zassert_false(entry->old_iv);
	call_rpl_check_on(SETTINGS_SAVE_ONE, 1, entry);

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* Call bt_mesh_rpl_pending_store() to store new entry. */
	ztest_expect_data(settings_save_one, name, entry->name);
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a valid entry that has not yet been deleted. The entry should
 * be stored during the reset operation.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_delete_other_entry)
{
	prepare_rpl_and_start_reset();

	/* Take the non-first entry with old_iv == true and simulate msg reception with same src
	 * address and correct IVI before the entry is deleted.
	 *
	 * Should be done before calling ztest_expect_data() because the expectation changes.
	 */
	struct test_rpl_entry *entry = &test_vector[3];

	zassert_true(entry->old_iv);
	entry->old_iv = false;
	call_rpl_check_on(SETTINGS_DELETE, 1, entry);

	expect_pending_store();

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* The entry should have been deleted in previous bt_mesh_rpl_pending_store() call. Another
	 * call should not do anything.
	 */
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/* Test that RPL accepts and stores a valid entry that has not yet been stored. The entry should be
 * stored during the reset operation.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_save_other_entry)
{
	prepare_rpl_and_start_reset();

	/* Take RPL entry from test vector that has old_iv == false and is not stored yet after
	 * bt_mesh_reset() call and try to store it again. RPL has such entry with flipped old_iv,
	 * so this one can be accepted as is.
	 *
	 * Should be done before calling ztest_expect_data() because the expectation changes.
	 */
	struct test_rpl_entry *entry = &test_vector[2];

	zassert_false(entry->old_iv);
	call_rpl_check_on(SETTINGS_SAVE_ONE, 1, entry);

	expect_pending_store();

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* The entry should have been stored in previous bt_mesh_rpl_pending_store() call. Another
	 * call should not do anything.
	 */
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a valid entry that has been deleted during the reset operation.
 * The entry will be added at the end of RPL, therefore it should be stored during the reset
 * operation.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_delete_deleted_entry)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	/* Take the first entry with old_iv == true, wait until bt_mesh_rpl_pending_store() takes
	 * another entry after that one and simulate msg reception.
	 */
	struct test_rpl_entry *entry = &test_vector[1];

	zassert_true(entry->old_iv);
	entry->old_iv = false;
	call_rpl_check_on(SETTINGS_DELETE, 2, entry);
	/* The entry will be stored during the reset operation as it will be added to the end of
	 * the RPL.
	 */
	ztest_expect_data(settings_save_one, name, entry->name);

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* The new entry should have been stored already. Another bt_mesh_rpl_pending_store() call
	 * should not do anything.
	 */
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a valid entry that has been stored during the reset operation.
 * Since the entry has been already in the list, it should be stored again after the reset
 * operation is finished.
 */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_store_stored_entry)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	/* Take the first entry with old_iv == false, wait until bt_mesh_rpl_pending_store() takes
	 * another entry after that one and simulate msg reception.
	 */
	struct test_rpl_entry *entry = &test_vector[0];

	zassert_false(entry->old_iv);
	entry->old_iv = true;
	entry->seq++;
	call_rpl_check_on(SETTINGS_SAVE_ONE, 2, entry);

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* The entry was updated after bt_mesh_rpl_pending_store() checked it. So it should be
	 * stored again.
	 */
	ztest_expect_data(settings_save_one, name, entry->name);
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	verify_rpl();
}

/** Test that RPL accepts and stores a new entry when the reset operation is not yet finished. */
ZTEST(bt_mesh_rpl_reset, test_rpl_check_on_save_new_entry)
{
	prepare_rpl_and_start_reset();
	expect_pending_store();

	/* Add a new entry to RPL during the reset operation. */
	struct test_rpl_entry entry = {
		.name = "bt/mesh/RPL/2b",
		.src = 43,
		.old_iv = false,
		.seq = 32,
	};
	ztest_expect_data(settings_save_one, name, entry.name);
	call_rpl_check_on(SETTINGS_SAVE_ONE, 1, &entry);

	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
	zassert_true(is_rpl_check_called());

	/* The entry should have been stored in previous bt_mesh_rpl_pending_store() call. Another
	 * call should not do anything.
	 */
	bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);

	check_entries_from_test_vector();
	/* Check that added entry in the RPL. */
	struct bt_mesh_net_rx msg = {
		.local_match = true,
		.ctx.addr = entry.src,
		.old_iv = entry.old_iv,
		.seq = entry.seq
	};
	zassert_true(bt_mesh_rpl_check(&msg, NULL));
	check_empty_entries(EMPTY_ENTRIES_CNT - 1);
}
