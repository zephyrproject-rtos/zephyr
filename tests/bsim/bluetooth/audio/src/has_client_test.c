/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/util_macro.h>

#include "../../subsys/bluetooth/audio/has_internal.h"

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(has_client_test, LOG_LEVEL_DBG);

extern enum bst_result_t bst_result;

extern const char *test_preset_name_1;
extern const char *test_preset_name_5;
extern const uint8_t test_preset_index_1;
extern const uint8_t test_preset_index_3;
extern const uint8_t test_preset_index_5;
extern const enum bt_has_properties test_preset_properties;

CREATE_FLAG(g_service_discovered);
CREATE_FLAG(g_preset_switched);
CREATE_FLAG(g_preset_1_found);
CREATE_FLAG(g_preset_3_found);
CREATE_FLAG(g_preset_5_found);

static struct bt_has *g_has;
static uint8_t g_active_index;

static void discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
			enum bt_has_hearing_aid_type type, enum bt_has_capabilities caps)
{
	if (err) {
		FAIL("Failed to discover HAS (err %d)\n", err);
		return;
	}

	LOG_DBG("HAS discovered type %d caps %d", type, caps);

	g_has = has;
	SET_FLAG(g_service_discovered);
}

static void preset_switch_cb(struct bt_has *has, int err, uint8_t index)
{
	if (err != 0) {
		return;
	}

	LOG_DBG("Active preset index %d", index);

	SET_FLAG(g_preset_switched);
	g_active_index = index;
}

static void check_preset_record(const struct bt_has_preset_record *record,
				enum bt_has_properties expected_properties,
				const char *expected_name)
{
	if (record->properties != expected_properties || strcmp(record->name, expected_name)) {
		FAIL("mismatch 0x%02x %s vs 0x%02x %s expected\n",
		     record->properties, record->name, expected_properties, expected_name);
	}
}

static void preset_read_rsp_cb(struct bt_has *has, int err,
			       const struct bt_has_preset_record *record, bool is_last)
{
	if (err) {
		FAIL("%s: err %d\n", __func__, err);
		return;
	}

	if (record->index == test_preset_index_1) {
		SET_FLAG(g_preset_1_found);

		check_preset_record(record, test_preset_properties, test_preset_name_1);
	} else if (record->index == test_preset_index_5) {
		SET_FLAG(g_preset_5_found);

		check_preset_record(record, test_preset_properties, test_preset_name_5);
	} else {
		FAIL("unexpected index 0x%02x", record->index);
	}
}

static void preset_update_cb(struct bt_has *has, uint8_t index_prev,
			     const struct bt_has_preset_record *record, bool is_last)
{
	if (record->index == test_preset_index_1) {
		SET_FLAG(g_preset_1_found);
	} else if (record->index == test_preset_index_3) {
		SET_FLAG(g_preset_3_found);
	} else if (record->index == test_preset_index_5) {
		SET_FLAG(g_preset_5_found);
	}
}

static const struct bt_has_client_cb has_cb = {
	.discover = discover_cb,
	.preset_switch = preset_switch_cb,
	.preset_read_rsp = preset_read_rsp_cb,
	.preset_update = preset_update_cb,
};

static bool test_preset_switch(uint8_t index)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_set(g_has, index, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == index;
}

static bool test_preset_next(uint8_t active_index_expected)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_next(g_has, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == active_index_expected;
}

static bool test_preset_prev(uint8_t active_index_expected)
{
	int err;

	UNSET_FLAG(g_preset_switched);

	err = bt_has_client_preset_prev(g_has, false);
	if (err < 0) {
		LOG_DBG("%s (err %d)", __func__, err);
		return false;
	}

	WAIT_FOR_COND(g_preset_switched);

	return g_active_index == active_index_expected;
}

static void discover_has(void)
{
	int err;

	g_service_discovered = false;

	err = bt_has_client_discover(default_conn);
	if (err < 0) {
		FAIL("Failed to discover HAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_service_discovered);
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_has_client_cb_register(&has_cb);
	if (err < 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
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

	discover_has();
	WAIT_FOR_COND(g_preset_switched);

	err = bt_has_client_presets_read(g_has, BT_HAS_PRESET_INDEX_FIRST, 255);
	if (err < 0) {
		FAIL("Failed to read presets (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(g_preset_1_found);
	WAIT_FOR_COND(g_preset_5_found);

	if (!test_preset_switch(test_preset_index_1)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_switch(test_preset_index_5)) {
		FAIL("Failed to switch preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_next(test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_next(test_preset_index_5)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_next(test_preset_index_1)) {
		FAIL("Failed to set next preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_prev(test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	if (!test_preset_prev(test_preset_index_1)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_1);
		return;
	}

	if (!test_preset_prev(test_preset_index_5)) {
		FAIL("Failed to set previous preset %d\n", test_preset_index_5);
		return;
	}

	PASS("HAS main PASS\n");
}

#define FEATURES_SUB_NTF        BIT(0)
#define ACTIVE_INDEX_SUB_NTF    BIT(1)
#define PRESET_CHANGED_SUB_NTF  BIT(2)
#define SUB_NTF_ALL		(FEATURES_SUB_NTF | ACTIVE_INDEX_SUB_NTF | PRESET_CHANGED_SUB_NTF)

CREATE_FLAG(flag_features_discovered);
CREATE_FLAG(flag_active_preset_index_discovered);
CREATE_FLAG(flag_control_point_discovered);
CREATE_FLAG(flag_all_notifications_received);

enum preset_state {
	STATE_UNKNOWN,
	STATE_AVAILABLE,
	STATE_UNAVAILABLE,
	STATE_DELETED,
};

static enum preset_state preset_state_1;
static enum preset_state preset_state_3;
static enum preset_state preset_state_5;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params features_sub;
static struct bt_gatt_subscribe_params active_preset_index_sub;
static struct bt_gatt_subscribe_params control_point_sub;
static uint8_t notify_received_mask;

static void preset_availability_changed(uint8_t index, bool available)
{
	enum preset_state state = available ? STATE_AVAILABLE : STATE_UNAVAILABLE;

	if (index == test_preset_index_1) {
		preset_state_1 = state;
	} else if (index == test_preset_index_3) {
		preset_state_3 = state;
	} else if (index == test_preset_index_5) {
		preset_state_5 = state;
	} else {
		FAIL("invalid preset index 0x%02x", index);
	}
}

static uint8_t notify_handler(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	LOG_DBG("conn %p params %p data %p length %u", (void *)conn, params, data, length);

	if (params == &features_sub) {
		if (data == NULL) {
			LOG_DBG("features_sub [UNSUBSCRIBED]");
			return BT_GATT_ITER_STOP;
		}

		LOG_DBG("Received features_sub notification");
		notify_received_mask |= FEATURES_SUB_NTF;
	} else if (params == &active_preset_index_sub) {
		if (data == NULL) {
			LOG_DBG("active_preset_index_sub_sub [UNSUBSCRIBED]");
			return BT_GATT_ITER_STOP;
		}

		LOG_DBG("Received active_preset_index_sub_sub notification");
		notify_received_mask |= ACTIVE_INDEX_SUB_NTF;
	} else if (params == &control_point_sub) {
		const struct bt_has_cp_hdr *hdr;

		if (data == NULL) {
			LOG_DBG("control_point_sub [UNSUBSCRIBED]");
			return BT_GATT_ITER_STOP;
		}

		if (length < sizeof(*hdr)) {
			FAIL("malformed bt_has_cp_hdr");
			return BT_GATT_ITER_STOP;
		}

		hdr = data;

		if (hdr->opcode == BT_HAS_OP_PRESET_CHANGED) {
			const struct bt_has_cp_preset_changed *pc;

			if (length < (sizeof(*hdr) + sizeof(*pc))) {
				FAIL("malformed bt_has_cp_preset_changed");
				return BT_GATT_ITER_STOP;
			}

			pc = (const void *)hdr->data;

			switch (pc->change_id) {
			case BT_HAS_CHANGE_ID_GENERIC_UPDATE: {
				const struct bt_has_cp_generic_update *gu;
				bool is_available;

				if (length < (sizeof(*hdr) + sizeof(*pc) + sizeof(*gu))) {
					FAIL("malformed bt_has_cp_generic_update");
					return BT_GATT_ITER_STOP;
				}

				gu = (const void *)pc->additional_params;

				LOG_DBG("Received generic update index 0x%02x props 0x%02x",
					gu->index, gu->properties);

				is_available = (gu->properties & BT_HAS_PROP_AVAILABLE) != 0;

				preset_availability_changed(gu->index, is_available);
				break;
			}
			default:
				LOG_DBG("Unexpected Change ID 0x%02x", pc->change_id);
				return BT_GATT_ITER_STOP;
			}

			if (pc->is_last) {
				notify_received_mask |= PRESET_CHANGED_SUB_NTF;
			}
		} else {
			LOG_DBG("Unexpected opcode 0x%02x", hdr->opcode);
			return BT_GATT_ITER_STOP;
		}
	}

	LOG_DBG("pacs_instance.notify_received_mask is %d", notify_received_mask);

	if (notify_received_mask == SUB_NTF_ALL) {
		SET_FLAG(flag_all_notifications_received);
		notify_received_mask = 0;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		return;
	}

	LOG_DBG("[SUBSCRIBED]");

	if (params == &features_sub) {
		SET_FLAG(flag_features_discovered);
		return;
	}

	if (params == &control_point_sub) {
		SET_FLAG(flag_control_point_discovered);
		return;
	}

	if (params == &active_preset_index_sub) {
		SET_FLAG(flag_active_preset_index_discovered);
		return;
	}
}

static uint8_t discover_features_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		LOG_DBG("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_HAS_HEARING_AID_FEATURES)) {
		LOG_DBG("HAS Hearing Aid Features handle at %d", attr->handle);
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params = &features_sub;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			LOG_DBG("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		LOG_DBG("CCC handle at %d", attr->handle);
		subscribe_params = &features_sub;
		subscribe_params->notify = notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;
		subscribe_params->subscribe = subscribe_cb;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			LOG_DBG("Subscribe failed (err %d)", err);
		}
	} else {
		LOG_DBG("Unknown handle at %d", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_features(void)
{
	int err = 0;

	LOG_DBG("%s", __func__);

	memcpy(&uuid, BT_UUID_HAS_HEARING_AID_FEATURES, sizeof(uuid));
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = discover_features_cb;

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_active_preset_index_cb(struct bt_conn *conn,
					       const struct bt_gatt_attr *attr,
					       struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		LOG_DBG("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX)) {
		LOG_DBG("HAS Hearing Aid Features handle at %d", attr->handle);
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params = &active_preset_index_sub;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			LOG_DBG("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		LOG_DBG("CCC handle at %d", attr->handle);
		subscribe_params = &active_preset_index_sub;
		subscribe_params->notify = notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;
		subscribe_params->subscribe = subscribe_cb;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			LOG_DBG("Subscribe failed (err %d)", err);
		}
	} else {
		LOG_DBG("Unknown handle at %d", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_active_preset_index(void)
{
	int err = 0;

	LOG_DBG("%s", __func__);

	memcpy(&uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX, sizeof(uuid));
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = discover_active_preset_index_cb;

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_control_point_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		LOG_DBG("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_HAS_PRESET_CONTROL_POINT)) {
		LOG_DBG("HAS Control Point handle at %d", attr->handle);
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params = &control_point_sub;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			LOG_DBG("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		LOG_DBG("CCC handle at %d", attr->handle);
		subscribe_params = &control_point_sub;
		subscribe_params->notify = notify_handler;
		subscribe_params->value = BT_GATT_CCC_INDICATE;
		subscribe_params->ccc_handle = attr->handle;
		subscribe_params->subscribe = subscribe_cb;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			LOG_DBG("Subscribe failed (err %d)", err);
		}
	} else {
		LOG_DBG("Unknown handle at %d", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_control_point(void)
{
	int err = 0;

	LOG_DBG("%s", __func__);

	memcpy(&uuid, BT_UUID_HAS_PRESET_CONTROL_POINT, sizeof(uuid));
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = discover_control_point_cb;

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err != 0) {
		FAIL("Control Point failed (err %d)\n", err);
		return;
	}
}

static void test_gatt_client(void)
{
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to set security level %d (err %d)", BT_SECURITY_L2, err);
		return;
	}

	WAIT_FOR_COND(security_level == BT_SECURITY_L2);

	discover_and_subscribe_features();
	WAIT_FOR_FLAG(flag_features_discovered);

	discover_and_subscribe_active_preset_index();
	WAIT_FOR_FLAG(flag_active_preset_index_discovered);

	discover_and_subscribe_control_point();
	WAIT_FOR_FLAG(flag_control_point_discovered);

	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	notify_received_mask = 0;
	UNSET_FLAG(flag_all_notifications_received);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_DBG("Scanning successfully started");

	WAIT_FOR_FLAG(flag_connected);

	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to set security level %d (err %d)\n", BT_SECURITY_L2, err);
		return;
	}

	WAIT_FOR_FLAG(flag_all_notifications_received);

	PASS("HAS main PASS\n");
}

static const struct bst_test_instance test_has[] = {
	{
		.test_id = "has_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "has_client_offline_behavior",
		.test_descr = "Test receiving notifications after reconnection",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gatt_client,
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
