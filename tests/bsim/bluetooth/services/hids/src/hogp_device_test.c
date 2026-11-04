/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HID Devices for the HOGP Report Host test:
 * - hogp_device: the HID Service, registered the way a HOGP HID Device application registers it,
 *   with a Security Request on every connection.
 * - hogp_device_2: a second HID Device the Report Host is connected to at the same time, which
 *   notifies its Input Report and holds a Feature Report of its own.
 * - hogp_malformed_device: HID Services a Report Host has to reject, registered one after the
 *   other as plain GATT services, without security requirements.
 *
 * The sync channel only links devices 0 and 1, so the other two are driven through their
 * connections.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_set_report);

static struct bt_conn *device_conn;

/* Whether the HID Device sends a Security Request when the Report Host connects */
static bool request_security;

static enum bt_hids_report_type set_report_type;
static uint8_t set_report_id;
static uint8_t set_report_data[CONFIG_BT_HIDS_MAX_REPORT_LEN];
static uint16_t set_report_len;

static void device_connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0U, "Failed to connect (err 0x%02x)", err);

	device_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);

	if (request_security) {
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		TEST_ASSERT(err == 0, "Failed to request security: %d", err);
	}
}

static void device_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn != device_conn) {
		return;
	}

	bt_conn_drop(&device_conn);
	SET_FLAG(flag_disconnected);
}

static struct bt_conn_cb device_conn_cb = {
	.connected = device_connected,
	.disconnected = device_disconnected,
};

static void set_report(struct bt_conn *conn, enum bt_hids_report_type report_type,
		       uint8_t report_id, const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(len <= sizeof(set_report_data), "SET_REPORT of %u bytes", len);

	set_report_type = report_type;
	set_report_id = report_id;
	(void)memcpy(set_report_data, data, len);
	set_report_len = len;
	SET_FLAG(flag_set_report);
}

static const struct bt_hids_cb hids_cb = {
	.set_report = set_report,
};

static const struct bt_hids_register_param hids_param = {
	.bcd_hid = TEST_BCD_HID,
	.country_code = TEST_COUNTRY_CODE,
	.flags = TEST_HID_FLAGS,
	.report_map = test_report_map,
	.report_map_len = sizeof(test_report_map),
	.input_report_ids = {TEST_REPORT_ID_INPUT},
	.output_report_ids = {TEST_REPORT_ID_OUTPUT},
	.feature_report_ids = {TEST_REPORT_ID_FEATURE, TEST_REPORT_ID_FEATURE_LONG},
	.cb = &hids_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data ad_2[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, TEST_DEVICE_2_NAME, sizeof(TEST_DEVICE_2_NAME) - 1),
};

static const struct bt_data ad_malformed[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, TEST_MALFORMED_NAME, sizeof(TEST_MALFORMED_NAME) - 1),
};

static void connect_host_ad(const struct bt_data *data, size_t len)
{
	int err;

	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_disconnected);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, data, len, NULL, 0);
	TEST_ASSERT(err == 0, "Failed to start advertising: %d", err);

	WAIT_FOR_FLAG(flag_connected);
}

static void connect_host(void)
{
	connect_host_ad(ad, ARRAY_SIZE(ad));
}

static void send_input_report(void)
{
	int err;

	err = bt_hids_send_report(device_conn, TEST_REPORT_ID_INPUT, test_input_data,
				  TEST_INPUT_REPORT_LEN, NULL, NULL);
	TEST_ASSERT(err == 0, "Failed to notify the Input Report: %d", err);
}

static void verify_set_report(enum bt_hids_report_type type, uint8_t id, const uint8_t *data,
			      uint16_t len)
{
	WAIT_FOR_FLAG(flag_set_report);
	UNSET_FLAG(flag_set_report);

	TEST_ASSERT(set_report_type == type && set_report_id == id,
		    "SET_REPORT of type %u ID %u, expected type %u ID %u", set_report_type,
		    set_report_id, type, id);
	TEST_ASSERT(set_report_len == len && memcmp(set_report_data, data, len) == 0,
		    "SET_REPORT with the wrong payload");
}

static void set_long_report(uint16_t len)
{
	uint8_t value[TEST_LONG_REPORT_LEN + 1U];
	int err;

	(void)memset(value, TEST_LONG_REPORT_FILL, len);
	err = bt_hids_report_set(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE_LONG, value,
				 len);
	TEST_ASSERT(err == 0, "Failed to set the long Feature Report: %d", err);
}

static void set_reports(void)
{
	int err;

	err = bt_hids_report_set(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
				 test_feature_data, sizeof(test_feature_data));
	TEST_ASSERT(err == 0, "Failed to set the Feature Report: %d", err);

	set_long_report(TEST_LONG_REPORT_LEN);
}

static void hogp_device_init(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed: %d", err);

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load the settings: %d", err);

	err = bt_conn_cb_register(&device_conn_cb);
	TEST_ASSERT(err == 0, "Failed to register the connection callbacks: %d", err);
}

static void test_hogp_device(void)
{
	int err;

	request_security = true;
	hogp_device_init();

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == 0, "Failed to register the HID Service: %d", err);

	set_reports();
	connect_host();

	/* The Report Host has discovered the service and subscribed */
	bk_sync_wait();
	send_input_report();

	/* SET_REPORT on the Output Report, then on the Feature Report */
	verify_set_report(BT_HIDS_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT, test_output_data,
			  sizeof(test_output_data));
	verify_set_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
			  test_feature_data_new, sizeof(test_feature_data_new));

	/* One octet more than the Report Host reads */
	set_long_report(TEST_LONG_REPORT_LEN + 1U);
	bk_sync_send();

	/* The Report Host reconnects to the bonded HID Device and discovers it again */
	WAIT_FOR_FLAG(flag_disconnected);
	connect_host();

	bk_sync_wait();
	send_input_report();

	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("HOGP Device test passed");
}

/* The connection fails to be established when its first connection events collide with those of
 * the Report Host and the first HID Device, so this advertises again until the Report Host
 * subscribes to the Input Report, which the notification shows.
 */
static void test_hogp_device_2(void)
{
	int err;

	request_security = true;
	hogp_device_init();

	err = bt_hids_register(&hids_param);
	TEST_ASSERT(err == 0, "Failed to register the HID Service: %d", err);

	err = bt_hids_report_set(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
				 test_feature_data_2, sizeof(test_feature_data_2));
	TEST_ASSERT(err == 0, "Failed to set the Feature Report: %d", err);

	connect_host_ad(ad_2, ARRAY_SIZE(ad_2));

	for (;;) {
		struct bt_conn *conn = device_conn;

		if (IS_FLAG_SET(flag_disconnected) || conn == NULL) {
			connect_host_ad(ad_2, ARRAY_SIZE(ad_2));
			continue;
		}

		/* device_disconnected() drops device_conn while this sends */
		conn = bt_conn_ref(conn);

		/* Fails until the Report Host has subscribed over an encrypted link */
		err = bt_hids_send_report(conn, TEST_REPORT_ID_INPUT, test_input_data_2,
					  sizeof(test_input_data_2), NULL, NULL);
		bt_conn_unref(conn);
		if (err == 0) {
			break;
		}

		k_sleep(K_MSEC(10));
	}

	/* The Report Host reads the Feature Report and disconnects */
	WAIT_FOR_FLAG(flag_disconnected);

	TEST_PASS("Second HOGP Device test passed");
}

/* Values of the malformed HID Services */
static const uint8_t malformed_hid_info[] = {0x11, 0x01, 0x00, 0x00};
static const uint8_t malformed_report_ref[] = {TEST_REPORT_ID_INPUT, BT_HIDS_REPORT_TYPE_INPUT};
static const uint8_t malformed_feature_ref[] = {TEST_REPORT_ID_FEATURE,
						BT_HIDS_REPORT_TYPE_FEATURE};

static const struct bt_data malformed_report_map = {
	.data_len = sizeof(test_report_map),
	.data = test_report_map,
};
static const struct bt_data malformed_info = {
	.data_len = sizeof(malformed_hid_info),
	.data = malformed_hid_info,
};
static const struct bt_data malformed_ref = {
	.data_len = sizeof(malformed_report_ref),
	.data = malformed_report_ref,
};
static const struct bt_data malformed_feature = {
	.data_len = sizeof(malformed_feature_ref),
	.data = malformed_feature_ref,
};
static const struct bt_data malformed_input = {
	.data_len = sizeof(test_input_data),
	.data = test_input_data,
};

static ssize_t malformed_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_data *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data, value->data_len);
}

/* The first CCC write is refused, so the first discovery fails at the subscription */
static atomic_t ccc_refused;

static ssize_t malformed_ccc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);

	if (value != 0U && atomic_cas(&ccc_refused, 0, 1)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	return sizeof(value);
}

#define MALFORMED_REPORT_MAP                                                                       \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,      \
			       malformed_read, NULL, (void *)&malformed_report_map)

#define MALFORMED_HID_INFO                                                                         \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,            \
			       malformed_read, NULL, (void *)&malformed_info)

/* A HID Service without characteristics */
static struct bt_gatt_attr empty_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
};

/* A Report characteristic without descriptors, which ends the service */
static struct bt_gatt_attr report_last_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	MALFORMED_REPORT_MAP,
	MALFORMED_HID_INFO,
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       malformed_read, NULL, (void *)&malformed_input),
};

/* A second Report declaration that names the value handle of the first */
#define DUPLICATE_FIRST_VALUE 6U
static struct bt_gatt_chrc duplicate_chrc =
	BT_GATT_CHRC_INIT(BT_UUID_HIDS_REPORT, 0U, BT_GATT_CHRC_READ);
static struct bt_gatt_attr duplicate_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	MALFORMED_REPORT_MAP,
	MALFORMED_HID_INFO,
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       malformed_read, NULL, (void *)&malformed_input),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, malformed_read, NULL,
			   (void *)&malformed_ref),
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL,
			  &duplicate_chrc),
	BT_GATT_ATTRIBUTE(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ, malformed_read, NULL,
			  (void *)&malformed_input),
};

/* Two Feature Reports with the same Report ID */
static struct bt_gatt_attr duplicate_id_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	MALFORMED_REPORT_MAP,
	MALFORMED_HID_INFO,
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       malformed_read, NULL, (void *)&malformed_input),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, malformed_read, NULL,
			   (void *)&malformed_feature),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       malformed_read, NULL, (void *)&malformed_input),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, malformed_read, NULL,
			   (void *)&malformed_feature),
};

/* Value of the Report characteristic in ccc_refused_attrs */
#define CCC_REFUSED_REPORT_VALUE 9U
static struct bt_gatt_attr ccc_refused_attrs[];

/* A characteristic the Report Host ignores, in front of the Input Report. It becomes a Feature
 * Report for one discovery, which moves the Input Report to the next index without changing its
 * handles.
 */
static const struct bt_uuid *const moved_uuid[] = {
	BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,
	BT_UUID_HIDS_REPORT,
};
static struct bt_gatt_chrc moved_chrc =
	BT_GATT_CHRC_INIT(BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT, 0U, BT_GATT_CHRC_READ);
static atomic_t ccc_refused_map_reads;

/* Notifies while the Report Host reads the Report Map, so that the notification reaches it during
 * the discovery, over the subscription a failed discovery left. The GATT client of the Report Host
 * drops it while the link is not encrypted. The fourth discovery finds the Feature Report.
 */
static ssize_t ccc_refused_map_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	if (offset == 0U) {
		(void)bt_gatt_notify(conn, &ccc_refused_attrs[CCC_REFUSED_REPORT_VALUE],
				     test_input_data, sizeof(test_input_data));

		moved_chrc.uuid = moved_uuid[atomic_inc(&ccc_refused_map_reads) == 2 ? 1 : 0];
	}

	return malformed_read(conn, attr, buf, len, offset);
}

/* A valid HID Service, apart from the security it does not require, with a CCC that refuses the
 * first write
 */
static struct bt_gatt_attr ccc_refused_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       ccc_refused_map_read, NULL, (void *)&malformed_report_map),
	MALFORMED_HID_INFO,
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL,
			  &moved_chrc),
	BT_GATT_ATTRIBUTE(BT_UUID_HIDS_REPORT, BT_GATT_PERM_READ, malformed_read, NULL,
			  (void *)&malformed_input),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, malformed_read, NULL,
			   (void *)&malformed_feature),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, malformed_read, NULL, (void *)&malformed_input),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, malformed_read, NULL,
			   (void *)&malformed_ref),
	BT_GATT_CCC_WITH_WRITE_CB(NULL, malformed_ccc_write,
				  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

/* In the order of TEST_MALFORMED_* in common.h */
static struct bt_gatt_service malformed_services[] = {
	BT_GATT_SERVICE(empty_attrs),
	BT_GATT_SERVICE(report_last_attrs),
	BT_GATT_SERVICE(duplicate_attrs),
	BT_GATT_SERVICE(duplicate_id_attrs),
	BT_GATT_SERVICE(ccc_refused_attrs),
};

BUILD_ASSERT(ARRAY_SIZE(malformed_services) == TEST_MALFORMED_COUNT);

/* Never asks for security, so the Report Host never pairs with it */
static void test_hogp_malformed_device(void)
{
	int err;

	request_security = false;
	hogp_device_init();

	ARRAY_FOR_EACH_PTR(malformed_services, svc) {
		err = bt_gatt_service_register(svc);
		TEST_ASSERT(err == 0, "Failed to register a malformed HID Service: %d", err);

		/* The handles are only known once the service is registered */
		if (svc->attrs == duplicate_attrs) {
			duplicate_chrc.value_handle = duplicate_attrs[DUPLICATE_FIRST_VALUE].handle;
		}

		/* The Report Host disconnects once the discovery failed */
		connect_host_ad(ad_malformed, ARRAY_SIZE(ad_malformed));
		WAIT_FOR_FLAG(flag_disconnected);

		err = bt_gatt_service_unregister(svc);
		TEST_ASSERT(err == 0, "Failed to unregister a malformed HID Service: %d", err);
	}

	TEST_PASS("Malformed HOGP Device test passed");
}

static const struct bst_test_instance test_hogp_device_defs[] = {
	{
		.test_id = "hogp_device",
		.test_descr = "HID Device built on the HID Service",
		.test_main_f = test_hogp_device,
	},
	{
		.test_id = "hogp_device_2",
		.test_descr = "Second HID Device of the HOGP Report Host test",
		.test_main_f = test_hogp_device_2,
	},
	{
		.test_id = "hogp_malformed_device",
		.test_descr = "HID Device with malformed HID Services",
		.test_main_f = test_hogp_malformed_device,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_device_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_device_defs);
}
