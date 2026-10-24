/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include "test_common.h"

#define LOG_MODULE_NAME test_sdp_discovery_server
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_SDP_DISCOVERY_LOG_LEVEL);

static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static ATOMIC_DEFINE(test_flags, 32);

#define TEST_FLAG_CONN_CONNECTED    0

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && !atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED)) {
		k_sem_give(&br_connect_changed_sem);
		atomic_set_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
	}
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	k_sem_give(&br_connect_changed_sem);
	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

static void test_wait_for_connect(void)
{
	int err;

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED), "Connection failed");
}

static void test_wait_for_disconnect(k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&br_connect_changed_sem, timeout);
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_false(atomic_test_bit(test_flags, TEST_FLAG_CONN_CONNECTED),
		      "Disconnection failed");
}

ZTEST(sdp_server, test_01_no_sdp_records)
{
	test_wait_for_connect();

	test_wait_for_disconnect(K_SECONDS(30));
}

static struct bt_sdp_attribute test_server1_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
			TEST_SERVICE_UUID,
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0xfff0)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(0xfff1)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x1001)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 33),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16(0xffff)
				})
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(0xfff2)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16(0x1002)
				})
			})
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
				},
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					BT_SDP_ARRAY_16(0xfffe)
				})
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(0xfff3)
				})
			})
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 22),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 20),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
				TEST_PROFILE_UUID,
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x1003)
			},
			)
		},
		)
	),
	BT_SDP_SUPPORTED_FEATURES(TEST_SUPPORTED_FEATURE),
	BT_SDP_SERVICE_NAME("test server"),
};

static struct bt_sdp_record test_server1_rec = BT_SDP_RECORD(test_server1_attrs);

ZTEST(sdp_server, test_02_one_sdp_record)
{
	int err;

	err = bt_sdp_register_service(&test_server1_rec);
	zassert_equal(err, 0, "SDP service registration failed (err %d)", err);

	test_wait_for_connect();

	test_wait_for_disconnect(K_SECONDS(30));
}

ZTEST(sdp_server, test_03_one_sdp_record_with_range)
{
	int err;

	err = bt_sdp_register_service(&test_server1_rec);
	zassert_equal(err, 0, "SDP service registration failed (err %d)", err);

	test_wait_for_connect();

	/* Due to the uncertainty of the time, the wait time here is set to `K_FOREVER`.
	 * The timeout is entirely determined by the twister based on the configuration,
	 * or, the disconnection event.
	 */
	test_wait_for_disconnect(K_FOREVER);
}

#define MAX_SDP_RECORD_COUNT CONFIG_TEST_ADDITIONAL_SDP_RECORD_COUNT

#define _SDP_ATTRS_DEFINE(index, sdp_attrs_name) \
static struct bt_sdp_attribute sdp_attrs_name##index[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP + index + 1) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
				BT_SDP_ARRAY_16(0x0102) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("Serial Port"), \
}

#define SDP_ATTRS_DEFINE(index, ...) _SDP_ATTRS_DEFINE(index, ##__VA_ARGS__)

LISTIFY(MAX_SDP_RECORD_COUNT, SDP_ATTRS_DEFINE, (;), spp_attrs);

#define _SDP_REC_DEFINE(index, sdp_attrs_name) BT_SDP_RECORD(sdp_attrs_name##index)
#define SDP_REC_DEFINE(index, ...)             _SDP_REC_DEFINE(index, ##__VA_ARGS__)

static struct bt_sdp_record spp_rec[MAX_SDP_RECORD_COUNT] = {
	LISTIFY(MAX_SDP_RECORD_COUNT, SDP_REC_DEFINE, (,), spp_attrs),
};

static struct bt_sdp_attribute spp_attrs_large[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("==================================================="
			    "==================================================="
			    "large_sdp_record"
			    "==================================================="
			    "==================================================="),
};

static struct bt_sdp_record spp_rec_large = BT_SDP_RECORD(spp_attrs_large);

static struct bt_sdp_attribute spp_attrs_large_valid[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("====================================="
			    "====================================="
			    "large_sdp_record"
			    "====================================="
			    "====================================="),
};

static struct bt_sdp_record spp_rec_large_valid = BT_SDP_RECORD(spp_attrs_large_valid);

ZTEST(sdp_server, test_04_multiple_sdp_records)
{
	int err;

	err = bt_sdp_register_service(&test_server1_rec);
	zassert_equal(err, 0, "SDP service registration failed (err %d)", err);

	ARRAY_FOR_EACH(spp_rec, i) {
		err = bt_sdp_register_service(&spp_rec[i]);
		zassert_equal(err, 0, "[%u] SDP service registration failed (err %d)", i, err);
	}

	err = bt_sdp_register_service(&spp_rec_large);
	zassert_equal(err, 0, "SDP service (long name 1) registration failed (err %d)", err);

	err = bt_sdp_register_service(&spp_rec_large_valid);
	zassert_equal(err, 0, "SDP service (long name 2) registration failed (err %d)", err);

	test_wait_for_connect();

	/* Due to the uncertainty of the time, the wait time here is set to `K_FOREVER`.
	 * The timeout is entirely determined by the twister based on the configuration,
	 * or, the disconnection event.
	 */
	test_wait_for_disconnect(K_FOREVER);
}

ZTEST(sdp_server, test_05_multiple_sdp_records_with_range)
{
	int err;

	err = bt_sdp_register_service(&test_server1_rec);
	zassert_equal(err, 0, "SDP service registration failed (err %d)", err);

	ARRAY_FOR_EACH(spp_rec, i) {
		err = bt_sdp_register_service(&spp_rec[i]);
		zassert_equal(err, 0, "[%u] SDP service registration failed (err %d)", i, err);
	}

	err = bt_sdp_register_service(&spp_rec_large);
	zassert_equal(err, 0, "SDP service (long name 1) registration failed (err %d)", err);

	err = bt_sdp_register_service(&spp_rec_large_valid);
	zassert_equal(err, 0, "SDP service (long name 2) registration failed (err %d)", err);

	test_wait_for_connect();

	/* Due to the uncertainty of the time, the wait time here is set to `K_FOREVER`.
	 * The timeout is entirely determined by the twister based on the configuration,
	 * or, the disconnection event.
	 */
	test_wait_for_disconnect(K_FOREVER);
}

static void *setup(void)
{
	int err;

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = bt_br_set_connectable(true, NULL);
	zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

	err = bt_br_set_discoverable(true, true);
	zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

	return NULL;
}

static void teardown(void *f)
{
	int err;

	LOG_DBG("Disabling Bluetooth");

	/* De-initialize the Bluetooth Subsystem */
	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);

	LOG_DBG("Bluetooth de-initialized");
}

static void before(void *f)
{
	k_sem_reset(&br_connect_changed_sem);

	atomic_clear_bit(test_flags, TEST_FLAG_CONN_CONNECTED);
}

ZTEST_SUITE(sdp_server, NULL, setup, before, NULL, teardown);
