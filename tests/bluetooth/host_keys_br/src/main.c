/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/keys.h"
#include "host/settings.h"

static const bt_addr_t peers[] = {
	{{1, 2, 3, 4, 5, 6}},
	{{2, 3, 4, 5, 6, 7}},
	{{3, 4, 5, 6, 7, 8}},
};

static struct bt_conn conns[2];
static bt_addr_le_t deleted_addr;
static unsigned int delete_count;

void __wrap_bt_conn_foreach(enum bt_conn_type type,
			   void (*func)(struct bt_conn *conn, void *data), void *data)
{
	for (size_t i = 0; i < ARRAY_SIZE(conns); i++) {
		if (conns[i].type & type) {
			func(&conns[i], data);
		}
	}
}

int __wrap_bt_settings_delete_link_key(const bt_addr_le_t *addr)
{
	bt_addr_le_copy(&deleted_addr, addr);
	delete_count++;
	return 0;
}

static void reset_pool(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(conns, 0, sizeof(conns));
	bt_keys_link_key_clear_addr(NULL);
	delete_count = 0;
	memset(&deleted_addr, 0, sizeof(deleted_addr));
}

static void *setup(void)
{
#if defined(CONFIG_BT_SETTINGS)
	zassert_ok(settings_subsys_init());
#endif
	return NULL;
}

static void fill_pool(struct bt_keys_link_key *keys[2])
{
	for (size_t i = 0; i < 2; i++) {
		keys[i] = bt_keys_get_link_key(&peers[i]);
		zassert_not_null(keys[i]);
		memset(keys[i]->val, i + 1, sizeof(keys[i]->val));
		keys[i]->enc_key_size = 16;
	}
	zassert_equal(delete_count, 0);
}

static void connect_peer(size_t index, struct bt_keys_link_key *key)
{
	conns[index].type = BT_CONN_TYPE_BR;
	conns[index].state = BT_CONN_CONNECTED;
	bt_addr_copy(&conns[index].br.dst, &key->addr);
	conns[index].br.link_key = key;
}

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
static void check_deleted_peer(size_t index)
{
	zassert_equal(delete_count, IS_ENABLED(CONFIG_BT_SETTINGS) ? 1 : 0);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		zassert_equal(deleted_addr.type, BT_ADDR_LE_PUBLIC);
		zassert_true(bt_addr_eq(&deleted_addr.a, &peers[index]));
	}
}
#endif

ZTEST_SUITE(keys_br, NULL, setup, reset_pool, NULL, NULL);

ZTEST(keys_br, test_existing_key_when_full)
{
	struct bt_keys_link_key *keys[2];

	fill_pool(keys);
	connect_peer(0, keys[0]);
	connect_peer(1, keys[1]);

	zassert_equal_ptr(bt_keys_get_link_key(&peers[0]), keys[0]);
	zassert_equal(delete_count, 0);
}

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
static void check_in_use_oldest(size_t index, bt_conn_state_t state)
{
	struct bt_keys_link_key *keys[2];
	struct bt_keys_link_key saved;

	fill_pool(keys);
	keys[index]->aging_counter = 0;
	keys[1 - index]->aging_counter = 1;
	connect_peer(index, keys[index]);
	conns[index].state = state;
	memcpy(&saved, keys[index], sizeof(saved));

	zassert_equal_ptr(bt_keys_get_link_key(&peers[2]), keys[1 - index]);
	zassert_equal_ptr(conns[index].br.link_key, keys[index]);
	zassert_mem_equal(keys[index], &saved, sizeof(saved));
	zassert_not_null(bt_keys_find_link_key(&peers[index]));
	zassert_is_null(bt_keys_find_link_key(&peers[1 - index]));
	check_deleted_peer(1 - index);
}

ZTEST(keys_br, test_in_use_first_slot)
{
	check_in_use_oldest(0, BT_CONN_CONNECTED);
}

ZTEST(keys_br, test_in_use_second_slot)
{
	check_in_use_oldest(1, BT_CONN_CONNECTED);
}

ZTEST(keys_br, test_disconnecting_key_is_preserved)
{
	check_in_use_oldest(0, BT_CONN_DISCONNECTING);
}

ZTEST(keys_br, test_disconnect_complete_key_is_preserved)
{
	check_in_use_oldest(1, BT_CONN_DISCONNECT_COMPLETE);
}

ZTEST(keys_br, test_connected_without_key_pointer)
{
	struct bt_keys_link_key *keys[2];
	struct bt_keys_link_key saved;

	fill_pool(keys);
	connect_peer(0, keys[0]);
	conns[0].br.link_key = NULL;
	memcpy(&saved, keys[0], sizeof(saved));

	zassert_equal_ptr(bt_keys_get_link_key(&peers[2]), keys[1]);
	zassert_mem_equal(keys[0], &saved, sizeof(saved));
	check_deleted_peer(1);
}

static void check_all_in_use_returns_null(bt_conn_state_t first_state, bt_conn_state_t second_state)
{
	struct bt_keys_link_key *keys[2];
	struct bt_keys_link_key saved[2];

	fill_pool(keys);
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		connect_peer(i, keys[i]);
		memcpy(&saved[i], keys[i], sizeof(saved[i]));
	}
	conns[0].state = first_state;
	conns[1].state = second_state;

	zassert_is_null(bt_keys_get_link_key(&peers[2]));
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		zassert_equal_ptr(conns[i].br.link_key, keys[i]);
		zassert_mem_equal(keys[i], &saved[i], sizeof(saved[i]));
	}
	zassert_equal(delete_count, 0);
}

ZTEST(keys_br, test_all_in_use_returns_null)
{
	check_all_in_use_returns_null(BT_CONN_CONNECTED, BT_CONN_CONNECTED);
}

ZTEST(keys_br, test_all_pending_disconnect_returns_null)
{
	check_all_in_use_returns_null(BT_CONN_DISCONNECTING, BT_CONN_DISCONNECT_COMPLETE);
}

ZTEST(keys_br, test_oldest_unused_key)
{
	struct bt_keys_link_key *keys[2];

	fill_pool(keys);
	keys[0]->aging_counter = 2;
	keys[1]->aging_counter = 1;

	zassert_equal_ptr(bt_keys_get_link_key(&peers[2]), keys[1]);
	zassert_not_null(bt_keys_find_link_key(&peers[0]));
	zassert_is_null(bt_keys_find_link_key(&peers[1]));
	check_deleted_peer(1);
}

ZTEST(keys_br, test_disconnected_reference_does_not_pin_key)
{
	struct bt_keys_link_key *keys[2];

	fill_pool(keys);
	connect_peer(0, keys[0]);
	conns[0].state = BT_CONN_DISCONNECTED;

	zassert_equal_ptr(bt_keys_get_link_key(&peers[2]), keys[0]);
	zassert_is_null(bt_keys_find_link_key(&peers[0]));
	check_deleted_peer(0);
}
#else
ZTEST(keys_br, test_full_pool_without_overwrite)
{
	struct bt_keys_link_key *keys[2];
	struct bt_keys_link_key saved[2];

	fill_pool(keys);
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		memcpy(&saved[i], keys[i], sizeof(saved[i]));
	}

	zassert_is_null(bt_keys_get_link_key(&peers[2]));
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		zassert_mem_equal(keys[i], &saved[i], sizeof(saved[i]));
	}
	zassert_equal(delete_count, 0);
}
#endif

#if defined(CONFIG_BT_SETTINGS)
ZTEST(keys_br, test_restore_without_available_slot)
{
	struct bt_keys_link_key *keys[2];
	struct bt_keys_link_key saved[2];
	struct bt_keys_link_key stored = {0};
	bt_addr_le_t addr = {.type = BT_ADDR_LE_PUBLIC};
	char name[BT_SETTINGS_KEY_MAX];

	fill_pool(keys);
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		connect_peer(i, keys[i]);
		memcpy(&saved[i], keys[i], sizeof(saved[i]));
	}
	bt_addr_copy(&addr.a, &peers[2]);
	zassert_ok(bt_settings_encode_key(name, sizeof(name), "link_key", &addr, NULL));

	zassert_equal(settings_runtime_set(name, stored.storage_start,
					   BT_KEYS_LINK_KEY_STORAGE_LEN), -ENOMEM);
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		zassert_mem_equal(keys[i], &saved[i], sizeof(saved[i]));
	}
	zassert_equal(delete_count, 0);
}
#endif
