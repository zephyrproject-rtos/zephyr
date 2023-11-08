/*
 * Copyright (c) 2022-2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/has.h>

#include "../../subsys/bluetooth/audio/has_internal.h"

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(has_client_test, LOG_LEVEL_DBG);

#define PRESET_RECORD(_i, _) _record_##_i
#define PRESET_RECORD_DEFINE(_i, _) \
	static char _preset_name_##_i[BT_HAS_PRESET_NAME_MAX]; \
	static struct bt_has_preset_record PRESET_RECORD(_i, _) = { .name = _preset_name_##_i }
#define PRESET_RECORD_ARRAY_DEFINE(_name, _size) \
	LISTIFY(_size, PRESET_RECORD_DEFINE, (;)); \
	static struct bt_has_preset_record *_name[] = { LISTIFY(_size, &PRESET_RECORD, (,)) }

extern enum bst_result_t bst_result;

extern const char *test_preset_name_1;
extern const char *test_preset_name_3;
extern const char *test_preset_name_5;
extern const uint8_t test_preset_index_1;
extern const uint8_t test_preset_index_3;
extern const uint8_t test_preset_index_5;

CREATE_FLAG(g_has_connected);
CREATE_FLAG(g_has_connected_err);
CREATE_FLAG(g_has_disconnected);
CREATE_FLAG(g_has_unbound);
CREATE_FLAG(g_preset_switched);
CREATE_FLAG(g_cmd_complete);
CREATE_FLAG(g_preset_list_updated);
PRESET_RECORD_ARRAY_DEFINE(preset_list, 10);
static volatile uint8_t g_active_index;
static volatile struct bt_has_client *default_client;

static void service_connected_cb(struct bt_has_client *client, int err)
{
	if (err) {
		LOG_DBG("Failed to connect HAS (err %d)", err);
		SET_FLAG(g_has_connected_err);
		return;
	}

	default_client = client;

	LOG_DBG("HAS connected");
	SET_FLAG(g_has_connected);
}

static void service_disconnected_cb(struct bt_has_client *client)
{
	__ASSERT_NO_MSG(default_client == client);

	LOG_DBG("HAS disconnected");
	SET_FLAG(g_has_disconnected);
}

static void service_unbound_cb(struct bt_has_client *client, int err)
{
	__ASSERT(default_client == client, "pointer mismatch %p != %p", default_client, client);

	if (err != 0) {
		LOG_DBG("Failed to unbind HAS (err %d)", err);
		return;
	}

	default_client = NULL;

	LOG_DBG("HAS unbound");
	SET_FLAG(g_has_unbound);
}

static void preset_switch_cb(struct bt_has_client *client, uint8_t index)
{
	LOG_DBG("Active preset index %d", index);

	SET_FLAG(g_preset_switched);
	g_active_index = index;
}

static void presets_list_add(const struct bt_has_preset_record *record)
{
	struct bt_has_preset_record *entry = NULL;

	LOG_DBG("index 0x%02x prop 0x%02x name %s",
		record->index, record->properties, record->name);

	for (size_t i = 0; i < ARRAY_SIZE(preset_list); i++) {
		if (preset_list[i]->index == record->index) {
			entry = preset_list[i];
			break;
		}

		if (entry == NULL && preset_list[i]->index == BT_HAS_PRESET_INDEX_NONE) {
			entry = preset_list[i];
		}
	}

	__ASSERT_NO_MSG(entry != NULL);

	entry->index = record->index;
	entry->properties = record->properties;
	utf8_lcpy((void *)entry->name, record->name, BT_HAS_PRESET_NAME_MAX);
}

static void presets_list_delete(uint8_t start_index, uint8_t end_index)
{
	LOG_DBG("start_index 0x%02x end_index 0x%02x", start_index, end_index);

	for (size_t i = 0; i < ARRAY_SIZE(preset_list); i++) {
		if (preset_list[i]->index > start_index && preset_list[i]->index < end_index) {
			preset_list[i]->index = BT_HAS_PRESET_INDEX_NONE;
		}
	}
}

static struct bt_has_preset_record *preset_record_lookup_index(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(preset_list); i++) {
		if (preset_list[i]->index == index) {
			return preset_list[i];
		}
	}

	return NULL;
}

static void preset_read_rsp_cb(struct bt_has_client *client,
			       const struct bt_has_preset_record *record, bool is_last)
{
	presets_list_add(record);

	if (is_last) {
		SET_FLAG(g_preset_list_updated);
	}
}

static void preset_update_cb(struct bt_has_client *client, uint8_t index_prev,
			     const struct bt_has_preset_record *record, bool is_last)
{
	presets_list_delete(index_prev, record->index);
	presets_list_add(record);

	if (is_last) {
		SET_FLAG(g_preset_list_updated);
	}
}

static void preset_deleted_cb(struct bt_has_client *client, uint8_t index, bool is_last)
{
	presets_list_delete(index, index);

	if (is_last) {
		SET_FLAG(g_preset_list_updated);
	}
}

static void preset_availability_cb(struct bt_has_client *client, uint8_t index, bool available,
				   bool is_last)
{
	/* TODO */

	ARG_UNUSED(preset_list);

	if (is_last) {
		SET_FLAG(g_preset_list_updated);
	}
}

static void cmd_status_cb(struct bt_has_client *client, uint8_t err)
{
	if (err == 0) {
		SET_FLAG(g_cmd_complete);
	}
}

static const struct bt_has_client_cb has_cb = {
	.connected = service_connected_cb,
	.disconnected = service_disconnected_cb,
	.unbound = service_unbound_cb,
	.preset_switch = preset_switch_cb,
	.preset_read_rsp = preset_read_rsp_cb,
	.preset_update = preset_update_cb,
	.preset_deleted = preset_deleted_cb,
	.preset_availability = preset_availability_cb,
	.cmd_status = cmd_status_cb,
};

static void expect_cmd_complete(void)
{
	WAIT_FOR_FLAG(g_cmd_complete);
	UNSET_FLAG(g_cmd_complete);
}

static void expect_preset_switched(void)
{
	WAIT_FOR_FLAG(g_preset_switched);
	UNSET_FLAG(g_preset_switched);
}

static void expect_preset_list_updated(void)
{
	WAIT_FOR_FLAG(g_preset_list_updated);
	UNSET_FLAG(g_preset_list_updated);
}

static bool test_preset_switch(struct bt_has_client *client, uint8_t index)
{
	int err;

	err = bt_has_client_cmd_preset_set(client, index, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	expect_preset_switched();
	expect_cmd_complete();

	return g_active_index == index;
}

static bool test_preset_next(struct bt_has_client *client, uint8_t active_index_expected)
{
	int err;

	err = bt_has_client_cmd_preset_next(client, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	expect_preset_switched();
	expect_cmd_complete();

	return g_active_index == active_index_expected;
}

static bool test_preset_prev(struct bt_has_client *client, uint8_t active_index_expected)
{
	int err;

	err = bt_has_client_cmd_preset_prev(client, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	expect_preset_switched();
	expect_cmd_complete();

	return g_active_index == active_index_expected;
}

static bool test_preset_write(struct bt_has_client *client, uint8_t index, const char *name)
{
	struct bt_has_preset_record *record;
	int err;

	err = bt_has_client_cmd_preset_write(client, index, name);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	expect_cmd_complete();
	expect_preset_list_updated();

	record = preset_record_lookup_index(index);
	__ASSERT_NO_MSG(record != NULL);

	return strcmp(record->name, name) == 0;
}

static void expect_service_connected(struct bt_has_client *client)
{
	WAIT_FOR_FLAG(g_has_connected);
	UNSET_FLAG(g_has_connected);
}

static void expect_service_disconnected(struct bt_has_client *client)
{
	WAIT_FOR_FLAG(g_has_disconnected);
	UNSET_FLAG(g_has_disconnected);
}

static void expect_service_unbound(struct bt_has_client *client)
{
	WAIT_FOR_FLAG(g_has_unbound);
	UNSET_FLAG(g_has_unbound);
}

static void test_main(void)
{
	struct bt_has_preset_record *record;
	struct bt_has_client *client;
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_has_client_init(&has_cb);
	if (err < 0) {
		FAIL("Failed to register HAS client (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);

	err = bt_has_client_bind(default_conn, &client);
	if (err < 0) {
		FAIL("Failed to connect HAS (err %d)\n", err);
		return;
	}

	expect_service_connected(client);
	expect_preset_switched();

	err = bt_has_client_cmd_presets_read(client, BT_HAS_PRESET_INDEX_FIRST,
					     BT_HAS_PRESET_INDEX_LAST);
	if (err < 0) {
		FAIL("Failed to read presets (err %d)\n", err);
		return;
	}

	expect_preset_list_updated();

	record = preset_record_lookup_index(test_preset_index_1);
	__ASSERT_NO_MSG(record != NULL);
	__ASSERT_NO_MSG(strcmp(record->name, test_preset_name_1) == 0);

	record = preset_record_lookup_index(test_preset_index_5);
	__ASSERT_NO_MSG(record != NULL);
	__ASSERT_NO_MSG(strcmp(record->name, test_preset_name_5) == 0);

	LOG_DBG("Switch to 1");
	if (!test_preset_switch(client, test_preset_index_1)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_1);
		return;
	}

	LOG_DBG("Switch to 5");
	if (!test_preset_switch(client, test_preset_index_5)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_5);
		return;
	}

	LOG_DBG("Set next");
	if (!test_preset_next(client, test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	LOG_DBG("Set next");
	if (!test_preset_next(client, test_preset_index_5)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_5);
		return;
	}

	LOG_DBG("Set next");
	if (!test_preset_next(client, test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	LOG_DBG("Set previous");
	if (!test_preset_prev(client, test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	LOG_DBG("Set previous");
	if (!test_preset_prev(client, test_preset_index_1)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_1);
		return;
	}

	LOG_DBG("Set previous");
	if (!test_preset_prev(client, test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	LOG_DBG("Write preset name");
	if (!test_preset_write(client, test_preset_index_5, "Vacuum")) {
		FAIL("Failed to write preset name %d\n", test_preset_index_5);
		return;
	}

	err = bt_has_client_unbind(client);
	if (err < 0) {
		FAIL("Failed to disconnect HAS (err %d)\n", err);
		return;
	}

	expect_service_disconnected(client);
	expect_service_unbound(client);

	PASS("%s\n", __func__);
}

static void test_client_offline_behavior(void)
{
	struct bt_has_preset_record *record;
	struct bt_has_client *client;
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_has_client_init(&has_cb);
	if (err < 0) {
		FAIL("Failed to register HAS client (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);

	LOG_DBG("Bind HAS");

	err = bt_has_client_bind(default_conn, &client);
	if (err < 0) {
		FAIL("Failed to connect HAS (err %d)\n", err);
		return;
	}

	__ASSERT_NO_MSG(client != NULL);

	expect_service_connected(client);
	expect_preset_switched();

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(err == 0);

	expect_service_disconnected(client);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started");

	expect_service_connected(client);
	expect_preset_switched();
	expect_preset_list_updated();

	record = preset_record_lookup_index(test_preset_index_1);
	__ASSERT_NO_MSG(record != NULL);
	__ASSERT_NO_MSG(strcmp(record->name, test_preset_name_1) == 0);

	record = preset_record_lookup_index(test_preset_index_3);
	__ASSERT_NO_MSG(record != NULL);
	__ASSERT_NO_MSG(strcmp(record->name, test_preset_name_3) == 0);

	record = preset_record_lookup_index(test_preset_index_5);
	__ASSERT_NO_MSG(record != NULL);
	__ASSERT_NO_MSG(strcmp(record->name, test_preset_name_5) == 0);

	PASS("%s\n", __func__);
}

static void expect_client_connect_failed(void)
{
	WAIT_FOR_FLAG(g_has_connected_err);
	UNSET_FLAG(g_has_connected_err);
}

static void test_client_connect_preamble(struct bt_has_client **client)
{
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(err == 0);

	LOG_DBG("Bluetooth initialized");

	err = bt_has_client_init(&has_cb);
	if (err < 0) {
		FAIL("Failed to register HAS client (err %d)\n", err);
		return;
	}

	LOG_DBG("HAS initialized");

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	__ASSERT_NO_MSG(err == 0);

	LOG_DBG("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);

	LOG_DBG("Connect HAS");

	err = bt_has_client_bind(default_conn, client);
	if (err < 0) {
		FAIL("Failed to connect HAS (err %d)\n", err);
		return;
	}
}

static void test_client_connect_err(void)
{
	struct bt_has_client *client;

	test_client_connect_preamble(&client);

	expect_client_connect_failed();

	PASS("%s\n", __func__);
}

static void test_client_bond_deleted_acl_connected(void)
{
	struct bt_conn_info info = { 0 };
	struct bt_has_client *client;
	int err;

	test_client_connect_preamble(&client);

	expect_service_connected(client);

	err = bt_conn_get_info(default_conn, &info);
	__ASSERT_NO_MSG(err == 0);

	LOG_DBG("Remove bond");

	err = bt_unpair(info.id, info.le.dst);
	__ASSERT_NO_MSG(err == 0);

	expect_service_disconnected(client);
	expect_service_unbound(client);

	PASS("%s\n", __func__);
}

static void test_client_bond_deleted_acl_disconnected(void)
{
	struct bt_conn_info info = { 0 };
	struct bt_has_client *client;
	int err;

	test_client_connect_preamble(&client);

	expect_service_connected(client);

	err = bt_conn_get_info(default_conn, &info);
	__ASSERT_NO_MSG(err == 0);

	LOG_DBG("Disconnect ACL");

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(err == 0);

	expect_service_disconnected(client);

	LOG_DBG("Remove bond");

	err = bt_unpair(info.id, info.le.dst);
	__ASSERT_NO_MSG(err == 0);

	expect_service_unbound(client);

	PASS("%s\n", __func__);
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "has_client_offline_behavior",
		.test_descr = "Test receiving notifications after reconnection",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client_offline_behavior,
	},
	{
		.test_id = "has_client_connect_err",
		.test_descr = "Test service connection failed to be established",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client_connect_err,
	},
	{
		.test_id = "has_client_bond_deleted_acl",
		.test_descr = "Test bond removal while ACL link is up",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client_bond_deleted_acl_connected,
	},
	{
		.test_id = "has_client_bond_deleted_no_acl",
		.test_descr = "Test bond removal while ACL link is down",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client_bond_deleted_acl_disconnected,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_has_client_install(struct bst_test_list *tests)
{
	if (IS_ENABLED(CONFIG_BT_HAS_CLIENT)) {
		return bst_add_tests(tests, test_has);
	} else {
		return tests;
	}
}
