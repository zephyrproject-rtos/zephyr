/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HOGP Report Host: discovers the HID Device, checks what the discovery found
 * against what the Device registered, receives an Input Report, reads and
 * writes Reports, and checks the errors of the API. It never initiates pairing
 * itself, so the GATT client retries the reads that need encryption once the
 * Security Request of the Device has encrypted the link.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/hogp_host.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include <testlib/att.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#include "common.h"

/* The long Feature Report fills the read buffer exactly */
BUILD_ASSERT(CONFIG_BT_HOGP_HOST_MAX_REPORT_LEN == TEST_LONG_REPORT_LEN);

DEFINE_FLAG_STATIC(flag_discovered);
DEFINE_FLAG_STATIC(flag_input_report);
DEFINE_FLAG_STATIC(flag_report_read);
DEFINE_FLAG_STATIC(flag_report_written);

static struct bt_conn *host_conn;

static int discovered_err;

static uint8_t input_id;
static uint8_t input_data[TEST_INPUT_REPORT_LEN];
static uint16_t input_len;

static enum bt_hids_report_type read_type;
static uint8_t read_id;
static int read_err;
static uint8_t read_data[CONFIG_BT_HOGP_HOST_MAX_REPORT_LEN];
static uint16_t read_len;

static enum bt_hids_report_type written_type;
static uint8_t written_id;
static int written_err;

static void discovered(struct bt_conn *conn, int err)
{
	TEST_ASSERT(conn == host_conn, "Discovered on another connection");

	discovered_err = err;
	SET_FLAG(flag_discovered);
}

static void input_report(struct bt_conn *conn, uint8_t service_index, uint8_t report_id,
			 const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(service_index == 0U, "Input Report of service %u", service_index);
	TEST_ASSERT(len <= sizeof(input_data), "Input Report of %u octets", len);

	input_id = report_id;
	(void)memcpy(input_data, data, len);
	input_len = len;
	SET_FLAG(flag_input_report);
}

static void report_read(struct bt_conn *conn, uint8_t service_index, enum bt_hids_report_type type,
			uint8_t report_id, int err, const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(service_index == 0U, "Report read from service %u", service_index);
	TEST_ASSERT(err != 0 || len <= sizeof(read_data), "Report of %u octets", len);

	read_type = type;
	read_id = report_id;
	read_err = err;
	read_len = len;
	if (len > 0U) {
		(void)memcpy(read_data, data, len);
	}

	SET_FLAG(flag_report_read);
}

static void report_written(struct bt_conn *conn, uint8_t service_index,
			   enum bt_hids_report_type type, uint8_t report_id, int err)
{
	ARG_UNUSED(conn);

	TEST_ASSERT(service_index == 0U, "Report written to service %u", service_index);

	written_type = type;
	written_id = report_id;
	written_err = err;
	SET_FLAG(flag_report_written);
}

static const struct bt_hogp_host_cb hogp_cb = {
	.discovered = discovered,
	.input_report = input_report,
	.report_read = report_read,
	.report_written = report_written,
};

static void connect_device(bool exchange_mtu)
{
	bt_addr_le_t addr;
	int err;

	err = bt_testlib_scan_find_name(&addr, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(err == 0, "Failed to find the HID Device: %d", err);

	err = bt_testlib_connect(&addr, &host_conn);
	TEST_ASSERT(err == 0, "Failed to connect: %d", err);

	if (!exchange_mtu) {
		return;
	}

	/* HOGP: exchange the MTU before the discovery */
	err = bt_testlib_att_exchange_mtu(host_conn);
	TEST_ASSERT(err == 0, "Failed to exchange the MTU: %d", err);
}

static void discover(void)
{
	int err;

	UNSET_FLAG(flag_discovered);
	UNSET_FLAG(flag_input_report);

	err = bt_hogp_host_discover(host_conn);
	TEST_ASSERT(err == 0, "Failed to start the discovery: %d", err);

	err = bt_hogp_host_discover(host_conn);
	TEST_ASSERT(err == -EALREADY, "Started the discovery twice: %d", err);

	WAIT_FOR_FLAG(flag_discovered);
	TEST_ASSERT(discovered_err == 0, "The discovery failed: %d", discovered_err);
	TEST_ASSERT(bt_conn_get_security(host_conn) >= BT_SECURITY_L2,
		    "Discovered without an encrypted link");
}

static void wait_for_input_report(void)
{
	WAIT_FOR_FLAG(flag_input_report);
	UNSET_FLAG(flag_input_report);

	TEST_ASSERT(input_id == TEST_REPORT_ID_INPUT, "Input Report ID %u", input_id);
	TEST_ASSERT(input_len == TEST_INPUT_REPORT_LEN &&
			    memcmp(input_data, test_input_data, input_len) == 0,
		    "Input Report with the wrong payload");
}

static void verify_report_info(const struct bt_hogp_host_report_info *info,
			       enum bt_hids_report_type type, uint8_t id, uint8_t properties)
{
	TEST_ASSERT(info->type == type && info->id == id,
		    "Report of type %u ID %u, expected type %u ID %u", info->type, info->id, type,
		    id);
	TEST_ASSERT((info->properties & properties) == properties,
		    "Report ID %u has properties 0x%02x", id, info->properties);
}

static void verify_discovery(void)
{
	struct bt_hogp_host_report_info reports[TEST_REPORT_COUNT + 1U];
	uint8_t map[sizeof(test_report_map)];
	struct bt_hogp_host_hid_info info;
	uint16_t len;
	uint8_t count;
	int err;

	err = bt_hogp_host_service_count_get(host_conn, &count);
	TEST_ASSERT(err == 0 && count == 1U, "Got %u HID Services: %d", count, err);

	err = bt_hogp_host_hid_info_get(host_conn, 0U, &info);
	TEST_ASSERT(err == 0, "Failed to get the HID Information: %d", err);
	TEST_ASSERT(info.bcd_hid == TEST_BCD_HID && info.country_code == TEST_COUNTRY_CODE &&
			    info.flags == TEST_HID_FLAGS,
		    "Wrong HID Information");

	len = sizeof(map);
	err = bt_hogp_host_report_map_get(host_conn, 0U, map, &len);
	TEST_ASSERT(err == 0, "Failed to get the Report Map: %d", err);
	TEST_ASSERT(len == sizeof(test_report_map) && memcmp(map, test_report_map, len) == 0,
		    "Wrong Report Map");

	/* A buffer that is too small is left untouched */
	(void)memset(map, 0xff, sizeof(map));
	len = sizeof(map) - 1U;
	err = bt_hogp_host_report_map_get(host_conn, 0U, map, &len);
	TEST_ASSERT(err == -ENOMEM, "Got the Report Map into a small buffer: %d", err);
	TEST_ASSERT(len == sizeof(test_report_map), "Needs %u octets for the Report Map", len);
	TEST_ASSERT(map[0] == 0xffU, "Wrote to the small Report Map buffer");

	count = ARRAY_SIZE(reports);
	err = bt_hogp_host_reports_get(host_conn, 0U, reports, &count);
	TEST_ASSERT(err == 0 && count == TEST_REPORT_COUNT, "Got %u Reports: %d", count, err);

	/* The HID Service declares them in this order */
	verify_report_info(&reports[0], BT_HIDS_REPORT_TYPE_INPUT, TEST_REPORT_ID_INPUT,
			   BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY);
	verify_report_info(&reports[1], BT_HIDS_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT,
			   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE);
	verify_report_info(&reports[2], BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE,
			   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE);
	verify_report_info(&reports[3], BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE_LONG,
			   BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE);

	(void)memset(reports, 0xff, sizeof(reports));
	count = TEST_REPORT_COUNT - 1U;
	err = bt_hogp_host_reports_get(host_conn, 0U, reports, &count);
	TEST_ASSERT(err == -ENOMEM, "Got the Reports into a small array: %d", err);
	TEST_ASSERT(count == TEST_REPORT_COUNT, "Needs %u entries for the Reports", count);
	TEST_ASSERT(reports[0].id == 0xffU, "Wrote to the small Report array");
}

static void test_getter_validation(void)
{
	struct bt_hogp_host_report_info reports[TEST_REPORT_COUNT];
	struct bt_hogp_host_hid_info info;
	uint8_t map[sizeof(test_report_map)];
	uint16_t len = sizeof(map);
	uint8_t count = ARRAY_SIZE(reports);
	int err;

	err = bt_hogp_host_service_count_get(NULL, &count);
	TEST_ASSERT(err == -EINVAL, "Got the service count of no connection: %d", err);

	err = bt_hogp_host_service_count_get(host_conn, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the service count into NULL: %d", err);

	err = bt_hogp_host_hid_info_get(NULL, 0U, &info);
	TEST_ASSERT(err == -EINVAL, "Got the HID Information of no connection: %d", err);

	err = bt_hogp_host_hid_info_get(host_conn, 0U, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the HID Information into NULL: %d", err);

	err = bt_hogp_host_hid_info_get(host_conn, 1U, &info);
	TEST_ASSERT(err == -EINVAL, "Got the HID Information of service 1: %d", err);

	err = bt_hogp_host_report_map_get(NULL, 0U, map, &len);
	TEST_ASSERT(err == -EINVAL, "Got the Report Map of no connection: %d", err);

	err = bt_hogp_host_report_map_get(host_conn, 0U, map, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the Report Map without a length: %d", err);

	err = bt_hogp_host_report_map_get(host_conn, 0U, NULL, &len);
	TEST_ASSERT(err == -EINVAL, "Got the Report Map into NULL: %d", err);

	err = bt_hogp_host_report_map_get(host_conn, 1U, map, &len);
	TEST_ASSERT(err == -EINVAL, "Got the Report Map of service 1: %d", err);

	err = bt_hogp_host_reports_get(NULL, 0U, reports, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports of no connection: %d", err);

	err = bt_hogp_host_reports_get(host_conn, 0U, NULL, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports into NULL: %d", err);

	err = bt_hogp_host_reports_get(host_conn, 0U, reports, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the Reports without a count: %d", err);

	err = bt_hogp_host_reports_get(host_conn, 1U, reports, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports of service 1: %d", err);
}

/* Before the discovery, and after the connection is gone */
static void verify_no_state(struct bt_conn *conn)
{
	struct bt_hogp_host_hid_info info;
	uint8_t count;
	int err;

	err = bt_hogp_host_service_count_get(conn, &count);
	TEST_ASSERT(err == -ENOENT, "Got the service count without a discovery: %d", err);

	err = bt_hogp_host_hid_info_get(conn, 0U, &info);
	TEST_ASSERT(err == -ENOENT, "Got the HID Information without a discovery: %d", err);

	err = bt_hogp_host_report_read(conn, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -ENOENT, "Read a Report without a discovery: %d", err);
}

static void read_report(enum bt_hids_report_type type, uint8_t id)
{
	int err;

	UNSET_FLAG(flag_report_read);

	err = bt_hogp_host_report_read(host_conn, 0U, type, id);
	TEST_ASSERT(err == 0, "Failed to start reading Report ID %u: %d", id, err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_type == type && read_id == id, "Read type %u ID %u", read_type, read_id);
	TEST_ASSERT(read_err == 0, "Failed to read Report ID %u: %d", id, read_err);
}

static void write_report(enum bt_hids_report_type type, uint8_t id, const uint8_t *data,
			 uint16_t len)
{
	int err;

	UNSET_FLAG(flag_report_written);

	err = bt_hogp_host_report_write(host_conn, 0U, type, id, data, len);
	TEST_ASSERT(err == 0, "Failed to start writing Report ID %u: %d", id, err);

	WAIT_FOR_FLAG(flag_report_written);
	TEST_ASSERT(written_type == type && written_id == id, "Wrote type %u ID %u", written_type,
		    written_id);
	TEST_ASSERT(written_err == 0, "Failed to write Report ID %u: %d", id, written_err);
}

static void test_reports(void)
{
	uint8_t long_value[TEST_LONG_REPORT_LEN];

	read_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(read_len == sizeof(test_feature_data) &&
			    memcmp(read_data, test_feature_data, read_len) == 0,
		    "Wrong Feature Report");

	/* Longer than ATT_MTU - 1, so read with several ATT requests */
	read_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE_LONG);
	(void)memset(long_value, TEST_LONG_REPORT_FILL, sizeof(long_value));
	TEST_ASSERT(read_len == sizeof(long_value) && memcmp(read_data, long_value, read_len) == 0,
		    "Wrong long Feature Report, %u octets", read_len);

	write_report(BT_HIDS_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT, test_output_data,
		     sizeof(test_output_data));
	write_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE, test_feature_data_new,
		     sizeof(test_feature_data_new));

	/* A read after SET_REPORT returns what was written */
	read_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(read_len == sizeof(test_feature_data_new) &&
			    memcmp(read_data, test_feature_data_new, read_len) == 0,
		    "The Feature Report was not written");
}

static void test_report_validation(void)
{
	/* One octet more than an attribute value holds */
	static const uint8_t too_long[513];
	int err;

	err = bt_hogp_host_report_read(NULL, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EINVAL, "Read a Report of no connection: %d", err);

	err = bt_hogp_host_report_read(host_conn, 1U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EINVAL, "Read a Report of service 1: %d", err);

	/* The Report ID exists, but with another Report Type */
	err = bt_hogp_host_report_read(host_conn, 0U, BT_HIDS_REPORT_TYPE_OUTPUT,
				       TEST_REPORT_ID_INPUT);
	TEST_ASSERT(err == -ENOENT, "Read a Report that does not exist: %d", err);

	err = bt_hogp_host_report_write(host_conn, 0U, BT_HIDS_REPORT_TYPE_OUTPUT,
					TEST_REPORT_ID_OUTPUT, NULL, 1U);
	TEST_ASSERT(err == -EINVAL, "Wrote a Report without a payload: %d", err);

	err = bt_hogp_host_report_write(host_conn, 0U, BT_HIDS_REPORT_TYPE_OUTPUT,
					TEST_REPORT_ID_OUTPUT, test_output_data, 0U);
	TEST_ASSERT(err == -EINVAL, "Wrote an empty Report: %d", err);

	err = bt_hogp_host_report_write(NULL, 0U, BT_HIDS_REPORT_TYPE_OUTPUT, TEST_REPORT_ID_OUTPUT,
					test_output_data, sizeof(test_output_data));
	TEST_ASSERT(err == -EINVAL, "Wrote a Report of no connection: %d", err);

	err = bt_hogp_host_report_write(host_conn, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
					TEST_REPORT_ID_FEATURE_LONG, too_long, sizeof(too_long));
	TEST_ASSERT(err == -EINVAL, "Wrote more than an attribute holds: %d", err);

	/* The Input Report of the HID Service is not writable */
	err = bt_hogp_host_report_write(host_conn, 0U, BT_HIDS_REPORT_TYPE_INPUT,
					TEST_REPORT_ID_INPUT, test_input_data,
					sizeof(test_input_data));
	TEST_ASSERT(err == -ENOTSUP, "Wrote the Input Report: %d", err);

	/* One read or write at a time */
	UNSET_FLAG(flag_report_read);
	err = bt_hogp_host_report_read(host_conn, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == 0, "Failed to start reading the Feature Report: %d", err);

	err = bt_hogp_host_report_read(host_conn, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EBUSY, "Started a second read: %d", err);

	err = bt_hogp_host_report_write(host_conn, 0U, BT_HIDS_REPORT_TYPE_OUTPUT,
					TEST_REPORT_ID_OUTPUT, test_output_data,
					sizeof(test_output_data));
	TEST_ASSERT(err == -EBUSY, "Started a write during a read: %d", err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_err == 0, "Failed to read the Feature Report: %d", read_err);
}

/* The Device makes the long Feature Report one octet longer than
 * CONFIG_BT_HOGP_HOST_MAX_REPORT_LEN, so reading it fails.
 */
static void test_report_too_long(void)
{
	int err;

	UNSET_FLAG(flag_report_read);
	err = bt_hogp_host_report_read(host_conn, 0U, BT_HIDS_REPORT_TYPE_FEATURE,
				       TEST_REPORT_ID_FEATURE_LONG);
	TEST_ASSERT(err == 0, "Failed to start reading the long Feature Report: %d", err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_err == -ENOMEM, "Read a Report longer than the buffer: %d", read_err);
	TEST_ASSERT(read_len == 0U, "Got %u octets of a failed read", read_len);
}

static void test_hogp_host(void)
{
	struct bt_conn *old_conn;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed: %d", err);

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load the settings: %d", err);

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	err = bt_hogp_host_cb_register(NULL);
	TEST_ASSERT(err == -EINVAL, "Registered no callbacks: %d", err);

	err = bt_hogp_host_cb_register(&hogp_cb);
	TEST_ASSERT(err == 0, "Failed to register the callbacks: %d", err);

	err = bt_hogp_host_cb_register(&hogp_cb);
	TEST_ASSERT(err == -EALREADY, "Registered the callbacks twice: %d", err);

	err = bt_hogp_host_discover(NULL);
	TEST_ASSERT(err == -EINVAL, "Discovered no connection: %d", err);

	connect_device(true);
	verify_no_state(host_conn);

	discover();
	verify_discovery();
	test_getter_validation();

	/* The Device notifies the Input Report once we are subscribed */
	bk_sync_send();
	wait_for_input_report();

	test_report_validation();
	test_reports();

	/* The Device has checked both SET_REPORTs and lengthened the long
	 * Feature Report.
	 */
	bk_sync_wait();

	test_report_too_long();

	/* The state of a connection is dropped when it disconnects */
	old_conn = bt_conn_ref(host_conn);
	err = bt_testlib_disconnect(&host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	verify_no_state(old_conn);

	err = bt_hogp_host_discover(old_conn);
	TEST_ASSERT(err == -ENOTCONN, "Discovered a disconnected Device: %d", err);
	bt_conn_unref(old_conn);

	/* The Device is bonded now; the subscription is set up again. Keeping
	 * the default ATT_MTU makes the Report Map take several ATT reads.
	 */
	connect_device(false);
	discover();
	verify_discovery();

	bk_sync_send();
	wait_for_input_report();

	err = bt_testlib_disconnect(&host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);

	TEST_PASS("HOGP Host test passed");
}

static const struct bst_test_instance test_hogp_host_defs[] = {
	{
		.test_id = "hogp_host",
		.test_descr = "HOGP Report Host",
		.test_main_f = test_hogp_host,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_host_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_host_defs);
}
