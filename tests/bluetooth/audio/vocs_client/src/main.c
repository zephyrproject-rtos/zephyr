/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "conn.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, bt_gatt_discover, struct bt_conn *, struct bt_gatt_discover_params *);
FAKE_VALUE_FUNC(int, bt_gatt_subscribe, struct bt_conn *, struct bt_gatt_subscribe_params *);
FAKE_VALUE_FUNC(int, bt_gatt_read, struct bt_conn *, struct bt_gatt_read_params *);
FAKE_VALUE_FUNC(int, bt_gatt_write, struct bt_conn *, struct bt_gatt_write_params *);
FAKE_VALUE_FUNC(int, bt_gatt_write_without_response_cb, struct bt_conn *, uint16_t, const void *,
		uint16_t, bool, bt_gatt_complete_func_t, void *);

FAKE_VOID_FUNC(discover_cb, struct bt_vocs *, int);
FAKE_VOID_FUNC(set_offset_cb, struct bt_vocs *, int);

#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(bt_gatt_discover)                                                                     \
	FAKE(bt_gatt_subscribe)                                                                    \
	FAKE(bt_gatt_read)                                                                         \
	FAKE(bt_gatt_write)                                                                        \
	FAKE(bt_gatt_write_without_response_cb)                                                    \
	FAKE(discover_cb)                                                                          \
	FAKE(set_offset_cb)

struct vocs_client_test_suite_fixture {
	struct bt_conn conn;
	struct bt_vocs *inst;
};

static void vocs_client_discover(struct vocs_client_test_suite_fixture *fixture)
{
	const struct bt_gatt_chrc chrcs[] = {
		BT_GATT_CHRC_INIT(BT_UUID_VOCS_STATE, 0x0003U,
				  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
		BT_GATT_CHRC_INIT(BT_UUID_VOCS_LOCATION, 0x0006U, BT_GATT_CHRC_READ),
		BT_GATT_CHRC_INIT(BT_UUID_VOCS_CONTROL, 0x0008U, BT_GATT_CHRC_WRITE),
		BT_GATT_CHRC_INIT(BT_UUID_VOCS_DESCRIPTION, 0x000AU, BT_GATT_CHRC_READ),
	};
	const struct bt_vocs_discover_param param = {
		.start_handle = 0x0001U,
		.end_handle = 0x000AU,
	};
	struct bt_gatt_discover_params *params;
	int err;

	err = bt_vocs_discover(&fixture->conn, fixture->inst, &param);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(bt_gatt_discover_fake.call_count, 1U);

	params = bt_gatt_discover_fake.arg1_val;

	ARRAY_FOR_EACH_PTR(chrcs, chrc) {
		struct bt_gatt_chrc value = *chrc;
		const struct bt_gatt_attr attr = {
			.uuid = BT_UUID_GATT_CHRC,
			.handle = chrc->value_handle - 1U,
			.user_data = &value,
		};

		zassert_equal(params->func(&fixture->conn, &attr, params), BT_GATT_ITER_CONTINUE);
	}

	zassert_equal(params->func(&fixture->conn, NULL, params), BT_GATT_ITER_STOP);
	zassert_equal(discover_cb_fake.call_count, 1U);
	zassert_equal(discover_cb_fake.arg1_val, 0);
}

static void *vocs_client_test_suite_setup(void)
{
	static struct bt_vocs_cb vocs_cb = {
		.discover = discover_cb,
		.set_offset = set_offset_cb,
	};
	static struct vocs_client_test_suite_fixture fixture;

	fixture.inst = bt_vocs_client_free_instance_get();
	zassert_not_null(fixture.inst);

	bt_vocs_client_cb_register(fixture.inst, &vocs_cb);

	return &fixture;
}

static void vocs_client_test_suite_before(void *f)
{
	struct vocs_client_test_suite_fixture *fixture = f;

	FFF_FAKES_LIST(RESET_FAKE);
	FFF_RESET_HISTORY();

	memset(&fixture->conn, 0, sizeof(fixture->conn));
	vocs_client_discover(fixture);
}

static void vocs_client_test_suite_after(void *f)
{
	struct vocs_client_test_suite_fixture *fixture = f;

	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

ZTEST_SUITE(vocs_client_test_suite, NULL, vocs_client_test_suite_setup,
	    vocs_client_test_suite_before, vocs_client_test_suite_after, NULL);

static void read_rsp(struct vocs_client_test_suite_fixture *fixture, const void *data,
		     uint16_t length)
{
	struct bt_gatt_read_params *params = bt_gatt_read_fake.arg1_val;

	zassert_not_null(params);
	zassert_equal(params->func(&fixture->conn, 0U, params, data, length), BT_GATT_ITER_STOP);
}

static void write_rsp(struct vocs_client_test_suite_fixture *fixture, uint8_t err)
{
	struct bt_gatt_write_params *params = bt_gatt_write_fake.arg1_val;

	zassert_not_null(params);
	params->func(&fixture->conn, err, params);
}

static ZTEST_F(vocs_client_test_suite, test_set_offset_counter_retry_state_empty)
{
	/* Offset 10, change counter 4 */
	const uint8_t state[] = {0x0AU, 0x00U, 0x04U};
	int err;

	err = bt_vocs_state_set(fixture->inst, 10);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(bt_gatt_write_fake.call_count, 1U);

	/* A rejected change counter makes the client read the state before writing again */
	write_rsp(fixture, BT_VOCS_ERR_INVALID_COUNTER);
	zassert_equal(bt_gatt_read_fake.call_count, 1U);

	read_rsp(fixture, NULL, 0U);

	zassert_equal(bt_gatt_write_fake.call_count, 1U);
	zassert_equal(set_offset_cb_fake.call_count, 1U);
	zassert_equal(set_offset_cb_fake.arg1_val, BT_ATT_ERR_UNLIKELY, "Unexpected err %d",
		      set_offset_cb_fake.arg1_val);

	err = bt_vocs_state_set(fixture->inst, 10);
	zassert_equal(err, 0, "Unexpected return value %d", err);
	zassert_equal(bt_gatt_write_fake.call_count, 2U);

	/* The failed retry does not keep the next operation from retrying */
	write_rsp(fixture, BT_VOCS_ERR_INVALID_COUNTER);
	zassert_equal(bt_gatt_read_fake.call_count, 2U);

	read_rsp(fixture, state, sizeof(state));
	zassert_equal(bt_gatt_write_fake.call_count, 3U);

	write_rsp(fixture, 0U);
	zassert_equal(set_offset_cb_fake.call_count, 2U);
	zassert_equal(set_offset_cb_fake.arg1_val, 0, "Unexpected err %d",
		      set_offset_cb_fake.arg1_val);
}

static ZTEST_F(vocs_client_test_suite, test_set_offset_counter_retry_state_short)
{
	/* Only the first octet is received */
	const uint8_t state[] = {0x0AU, 0x00U, 0x04U};
	int err;

	err = bt_vocs_state_set(fixture->inst, 10);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	write_rsp(fixture, BT_VOCS_ERR_INVALID_COUNTER);
	zassert_equal(bt_gatt_read_fake.call_count, 1U);

	read_rsp(fixture, state, 1U);

	zassert_equal(bt_gatt_write_fake.call_count, 1U);
	zassert_equal(set_offset_cb_fake.call_count, 1U);
	zassert_equal(set_offset_cb_fake.arg1_val, BT_ATT_ERR_UNLIKELY, "Unexpected err %d",
		      set_offset_cb_fake.arg1_val);
}
