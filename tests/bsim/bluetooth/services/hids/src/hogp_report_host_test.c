/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HOGP Report Host: discovers the HID Device, checks what the discovery hands over against what
 * the HID Device registered, receives an Input Report, reads and writes Reports, and checks the
 * errors of the API. It does not request security: the HID Device sends a Security Request, and
 * the GATT client retries the reads that need encryption once the link is encrypted.
 *
 * It then discovers a second HID Device while connected to the first one, and checks that each
 * HID Service gets its own Input Reports and Report values. Last, it discovers HID Devices whose
 * HID Service is malformed and checks that the discovery fails, also when a failed discovery is
 * run again on the same connection.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/profiles/hogp.h>
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
BUILD_ASSERT(CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORT_LEN == TEST_LONG_REPORT_LEN);

DEFINE_FLAG_STATIC(flag_hid_info);
DEFINE_FLAG_STATIC(flag_discovered);
DEFINE_FLAG_STATIC(flag_input_report);
DEFINE_FLAG_STATIC(flag_report_read);
DEFINE_FLAG_STATIC(flag_report_written);
DEFINE_FLAG_STATIC(flag_discovered_2);
DEFINE_FLAG_STATIC(flag_input_report_2);
DEFINE_FLAG_STATIC(flag_security_2);
DEFINE_FLAG_STATIC(flag_disconnected_2);
DEFINE_FLAG_STATIC(flag_discovered_malformed);
DEFINE_FLAG_STATIC(flag_security_malformed);

static struct bt_conn *host_conn;
static struct bt_hogp_report_host_hids *host_hids;

/* The second HID Device */
static struct bt_conn *conn_2;
static struct bt_hogp_report_host_hids *hids_2;
static int discovered_err_2;
static uint8_t discovered_count_2;
/* Disconnects the second HID Device from the first Report Map fragment, so that the Read Long
 * Characteristic Value procedure fails in the GATT client
 */
static bool disconnect_2_in_discovery;
static uint8_t input_data_2[TEST_INPUT_REPORT_LEN];
static uint16_t input_len_2;

/* The HID Device with a malformed HID Service */
static struct bt_conn *malformed_conn;
static int discovered_err_malformed;

static struct bt_hogp_report_host_hid_info hid_info;

/* The Report Map as the fragments of the discovery deliver it */
static uint8_t report_map[BT_ATT_MAX_ATTRIBUTE_LEN];
static uint16_t report_map_len;
static uint8_t report_map_fragments;

static int discovered_err;

static uint8_t input_id;
static uint8_t input_data[TEST_INPUT_REPORT_LEN];
static uint16_t input_len;

static enum bt_hids_report_type read_type;
static uint8_t read_id;
static int read_err;
static uint8_t read_data[CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORT_LEN];
static uint16_t read_len;

static enum bt_hids_report_type written_type;
static uint8_t written_id;
static int written_err;

/* Calls of the second callback structure */
static uint8_t listener_discovered;
static uint8_t listener_input_reports;

/* Not a HID Service of the profile */
static struct bt_hogp_report_host_hids *const bogus_hids =
	(struct bt_hogp_report_host_hids *)&host_conn;

/* Whether a HID Service is on conn */
static bool hids_on_conn(const struct bt_hogp_report_host_hids *hids, const struct bt_conn *conn)
{
	struct bt_conn *hids_conn;
	bool on_conn;
	int err;

	err = bt_hogp_report_host_conn_get(hids, &hids_conn);
	TEST_ASSERT(err == 0, "Failed to get the connection of a HID Service: %d", err);
	on_conn = (hids_conn == conn);
	bt_conn_unref(hids_conn);

	return on_conn;
}

static void hid_info_cb(struct bt_hogp_report_host_hids *hids,
			const struct bt_hogp_report_host_hid_info *info)
{
	struct bt_hogp_report_host_report_info reports[TEST_REPORT_COUNT];
	uint8_t count = ARRAY_SIZE(reports);
	int err;

	if (hids_on_conn(hids, conn_2)) {
		hids_2 = hids;
		return;
	}

	if (hids_on_conn(hids, malformed_conn)) {
		return;
	}

	TEST_ASSERT(hids_on_conn(hids, host_conn), "HID Information of an unknown connection");

	/* Only the connection can be got before the discovery succeeds */
	err = bt_hogp_report_host_reports_get(hids, reports, &count);
	TEST_ASSERT(err == -EBUSY, "Got the Reports during the discovery: %d", err);

	err = bt_hogp_report_host_report_read(hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EBUSY, "Read a Report during the discovery: %d", err);

	host_hids = hids;
	hid_info = *info;
	SET_FLAG(flag_hid_info);
}

static void report_map_cb(struct bt_hogp_report_host_hids *hids, uint16_t offset,
			  const uint8_t *data, uint16_t len)
{
	int err;

	if (hids == hids_2 && disconnect_2_in_discovery) {
		TEST_ASSERT(offset == 0U && len < sizeof(test_report_map),
			    "The Report Map of the second HID Device comes in one fragment");
		disconnect_2_in_discovery = false;
		err = bt_conn_disconnect(conn_2, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(err == 0, "Failed to disconnect during the discovery: %d", err);
		return;
	}

	if (hids != host_hids) {
		return;
	}

	TEST_ASSERT(offset == report_map_len, "Report Map fragment at %u, expected %u", offset,
		    report_map_len);
	TEST_ASSERT(len <= sizeof(report_map) - offset, "Report Map fragment of %u octets", len);

	(void)memcpy(&report_map[offset], data, len);
	report_map_len += len;
	report_map_fragments++;
}

static void discovered_cb(struct bt_conn *conn, int err,
			  const struct bt_hogp_report_host_services *services)
{
	if (conn == conn_2) {
		TEST_ASSERT(err != 0 || services->hids[0] == hids_2,
			    "Discovered another HID Service on the second HID Device");
		discovered_err_2 = err;
		discovered_count_2++;
		SET_FLAG(flag_discovered_2);
		return;
	}

	if (conn == malformed_conn) {
		TEST_ASSERT((err == 0) == (services != NULL), "HID Services of a failed discovery");
		discovered_err_malformed = err;
		SET_FLAG(flag_discovered_malformed);
		return;
	}

	TEST_ASSERT(conn == host_conn, "Discovered on another connection");

	if (err == 0) {
		TEST_ASSERT(services != NULL && services->count == 1U,
			    "Discovered no HID Service or more than one");
		TEST_ASSERT(services->hids[0] == host_hids,
			    "Discovered another HID Service than the callbacks reported");
	} else {
		TEST_ASSERT(services == NULL, "HID Services of a failed discovery");
	}

	discovered_err = err;
	SET_FLAG(flag_discovered);
}

static void input_report_cb(struct bt_hogp_report_host_hids *hids, uint8_t report_id,
			    const uint8_t *data, uint16_t len)
{
	TEST_ASSERT(len <= sizeof(input_data), "Input Report of %u octets", len);

	if (hids == hids_2) {
		(void)memcpy(input_data_2, data, len);
		input_len_2 = len;
		SET_FLAG(flag_input_report_2);
		return;
	}

	TEST_ASSERT(hids == host_hids, "Input Report of another HID Service");

	input_id = report_id;
	(void)memcpy(input_data, data, len);
	input_len = len;
	SET_FLAG(flag_input_report);
}

static void report_read_cb(struct bt_hogp_report_host_hids *hids, enum bt_hids_report_type type,
			   uint8_t report_id, int err, const uint8_t *data, uint16_t len)
{
	TEST_ASSERT(hids == host_hids || hids == hids_2, "Report read from another HID Service");
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

static void report_written_cb(struct bt_hogp_report_host_hids *hids, enum bt_hids_report_type type,
			      uint8_t report_id, int err)
{
	TEST_ASSERT(hids == host_hids, "Report written to another HID Service");

	written_type = type;
	written_id = report_id;
	written_err = err;
	SET_FLAG(flag_report_written);
}

static struct bt_hogp_report_host_cb hogp_cb = {
	.hid_info = hid_info_cb,
	.report_map = report_map_cb,
	.discovered = discovered_cb,
	.input_report = input_report_cb,
	.report_read = report_read_cb,
	.report_written = report_written_cb,
};

static void listener_discovered_cb(struct bt_conn *conn, int err,
				   const struct bt_hogp_report_host_services *services)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(err);
	ARG_UNUSED(services);

	listener_discovered++;
}

static void listener_input_report_cb(struct bt_hogp_report_host_hids *hids, uint8_t report_id,
				     const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(hids);
	ARG_UNUSED(report_id);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	listener_input_reports++;
}

/* A second callback structure, with only some of the callbacks */
static struct bt_hogp_report_host_cb listener_cb = {
	.discovered = listener_discovered_cb,
	.input_report = listener_input_report_cb,
};

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(level);
	ARG_UNUSED(err);

	if (conn == conn_2) {
		SET_FLAG(flag_security_2);
	}

	if (conn == malformed_conn) {
		SET_FLAG(flag_security_malformed);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (conn == conn_2) {
		SET_FLAG(flag_disconnected_2);
	}
}

static struct bt_conn_cb host_conn_cb = {
	.security_changed = security_changed,
	.disconnected = disconnected,
};

static void connect_name(const char *name, struct bt_conn **conn)
{
	bt_addr_le_t addr;
	int err;

	err = bt_testlib_scan_find_name(&addr, name);
	TEST_ASSERT(err == 0, "Failed to find %s: %d", name, err);

	err = bt_testlib_connect(&addr, conn);
	TEST_ASSERT(err == 0, "Failed to connect to %s: %d", name, err);
}

static void connect_device(bool exchange_mtu)
{
	int err;

	connect_name(CONFIG_BT_DEVICE_NAME, &host_conn);

	if (!exchange_mtu) {
		return;
	}

	/* HOGP: exchange the MTU before the discovery */
	err = bt_testlib_att_exchange_mtu(host_conn);
	TEST_ASSERT(err == 0, "Failed to exchange the MTU: %d", err);
}

/* The Report Map has to come in at least min_fragments fragments */
static void discover(uint8_t min_fragments)
{
	int err;

	UNSET_FLAG(flag_hid_info);
	UNSET_FLAG(flag_discovered);
	UNSET_FLAG(flag_input_report);
	host_hids = NULL;
	report_map_len = 0U;
	report_map_fragments = 0U;

	err = bt_hogp_report_host_discover(host_conn);
	TEST_ASSERT(err == 0, "Failed to start the discovery: %d", err);

	err = bt_hogp_report_host_discover(host_conn);
	TEST_ASSERT(err == -EBUSY, "Started the discovery twice: %d", err);

	WAIT_FOR_FLAG(flag_discovered);
	TEST_ASSERT(discovered_err == 0, "The discovery failed: %d", discovered_err);

	err = bt_hogp_report_host_discover(host_conn);
	TEST_ASSERT(err == -EALREADY, "Started the discovery after it succeeded: %d", err);
	TEST_ASSERT(bt_conn_get_security(host_conn) >= BT_SECURITY_L2,
		    "Discovered without an encrypted link");

	TEST_ASSERT(IS_FLAG_SET(flag_hid_info), "No HID Information before the discovery ended");
	TEST_ASSERT(hid_info.bcd_hid == TEST_BCD_HID &&
			    hid_info.country_code == TEST_COUNTRY_CODE &&
			    hid_info.flags == TEST_HID_FLAGS,
		    "Wrong HID Information");

	TEST_ASSERT(report_map_len == sizeof(test_report_map) &&
			    memcmp(report_map, test_report_map, report_map_len) == 0,
		    "Wrong Report Map, %u octets", report_map_len);
	TEST_ASSERT(report_map_fragments >= min_fragments, "Report Map in %u fragments",
		    report_map_fragments);
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

static void verify_report_info(const struct bt_hogp_report_host_report_info *info,
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
	struct bt_hogp_report_host_report_info reports[TEST_REPORT_COUNT + 1U];
	struct bt_hogp_report_host_services services;
	struct bt_conn *conn;
	uint8_t count;
	int err;

	err = bt_hogp_report_host_services_get(host_conn, &services);
	TEST_ASSERT(err == 0 && services.count == 1U, "Got %u HID Services: %d", services.count,
		    err);
	TEST_ASSERT(services.hids[0] == host_hids, "Got another HID Service");

	err = bt_hogp_report_host_conn_get(host_hids, &conn);
	TEST_ASSERT(err == 0 && conn == host_conn, "Got the connection of the HID Service: %d",
		    err);
	bt_conn_unref(conn);

	count = ARRAY_SIZE(reports);
	err = bt_hogp_report_host_reports_get(host_hids, reports, &count);
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
	err = bt_hogp_report_host_reports_get(host_hids, reports, &count);
	TEST_ASSERT(err == -ENOMEM, "Got the Reports into a small array: %d", err);
	TEST_ASSERT(count == TEST_REPORT_COUNT, "Needs %u entries for the Reports", count);
	TEST_ASSERT(reports[0].id == 0xffU, "Wrote to the small Report array");
}

static void test_getter_validation(void)
{
	struct bt_hogp_report_host_report_info reports[TEST_REPORT_COUNT];
	struct bt_hogp_report_host_services services;
	uint8_t count = ARRAY_SIZE(reports);
	struct bt_conn *conn;
	int err;

	err = bt_hogp_report_host_services_get(NULL, &services);
	TEST_ASSERT(err == -EINVAL, "Got the HID Services of no connection: %d", err);

	err = bt_hogp_report_host_services_get(host_conn, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the HID Services into NULL: %d", err);

	err = bt_hogp_report_host_conn_get(NULL, &conn);
	TEST_ASSERT(err == -EINVAL, "Got the connection of no HID Service: %d", err);

	err = bt_hogp_report_host_conn_get(host_hids, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the connection into NULL: %d", err);

	err = bt_hogp_report_host_conn_get(bogus_hids, &conn);
	TEST_ASSERT(err == -EINVAL, "Got the connection of a bogus HID Service: %d", err);

	err = bt_hogp_report_host_reports_get(NULL, reports, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports of no HID Service: %d", err);

	err = bt_hogp_report_host_reports_get(host_hids, NULL, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports into NULL: %d", err);

	err = bt_hogp_report_host_reports_get(host_hids, reports, NULL);
	TEST_ASSERT(err == -EINVAL, "Got the Reports without a count: %d", err);

	err = bt_hogp_report_host_reports_get(bogus_hids, reports, &count);
	TEST_ASSERT(err == -EINVAL, "Got the Reports of a bogus HID Service: %d", err);
}

/* Before the discovery, and after the connection is gone */
static void verify_no_state(struct bt_conn *conn)
{
	struct bt_hogp_report_host_services services;
	int err;

	err = bt_hogp_report_host_services_get(conn, &services);
	TEST_ASSERT(err == -ENOENT, "Got the HID Services without a discovery: %d", err);
}

/* After the connection is gone, with the HID Service object it had */
static void verify_gone(struct bt_conn *conn, struct bt_hogp_report_host_hids *hids)
{
	struct bt_hogp_report_host_report_info reports[TEST_REPORT_COUNT];
	uint8_t count = ARRAY_SIZE(reports);
	struct bt_conn *hids_conn;
	int err;

	verify_no_state(conn);

	err = bt_hogp_report_host_conn_get(hids, &hids_conn);
	TEST_ASSERT(err == -ENOTCONN, "Got the connection of a disconnected HID Service: %d", err);

	err = bt_hogp_report_host_reports_get(hids, reports, &count);
	TEST_ASSERT(err == -ENOTCONN, "Got the Reports of a disconnected HID Service: %d", err);

	err = bt_hogp_report_host_report_read(hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -ENOTCONN, "Read a Report of a disconnected HID Service: %d", err);
}

static void read_report_of(struct bt_hogp_report_host_hids *hids, enum bt_hids_report_type type,
			   uint8_t id)
{
	int err;

	UNSET_FLAG(flag_report_read);

	err = bt_hogp_report_host_report_read(hids, type, id);
	TEST_ASSERT(err == 0, "Failed to start reading Report ID %u: %d", id, err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_type == type && read_id == id, "Read type %u ID %u", read_type, read_id);
	TEST_ASSERT(read_err == 0, "Failed to read Report ID %u: %d", id, read_err);
}

static void read_report(enum bt_hids_report_type type, uint8_t id)
{
	read_report_of(host_hids, type, id);
}

static void write_report(enum bt_hids_report_type type, uint8_t id, const uint8_t *data,
			 uint16_t len)
{
	int err;

	UNSET_FLAG(flag_report_written);

	err = bt_hogp_report_host_report_write(host_hids, type, id, data, len);
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
	static const uint8_t too_long[BT_ATT_MAX_ATTRIBUTE_LEN + 1U];
	int err;

	err = bt_hogp_report_host_report_read(NULL, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EINVAL, "Read a Report of no HID Service: %d", err);

	err = bt_hogp_report_host_report_read(bogus_hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EINVAL, "Read a Report of a bogus HID Service: %d", err);

	/* The Report ID exists, but with another Report Type */
	err = bt_hogp_report_host_report_read(host_hids, BT_HIDS_REPORT_TYPE_OUTPUT,
					      TEST_REPORT_ID_INPUT);
	TEST_ASSERT(err == -ENOENT, "Read a Report that does not exist: %d", err);

	err = bt_hogp_report_host_report_write(host_hids, BT_HIDS_REPORT_TYPE_OUTPUT,
					       TEST_REPORT_ID_OUTPUT, NULL, 1U);
	TEST_ASSERT(err == -EINVAL, "Wrote a Report without a payload: %d", err);

	err = bt_hogp_report_host_report_write(host_hids, BT_HIDS_REPORT_TYPE_OUTPUT,
					       TEST_REPORT_ID_OUTPUT, test_output_data, 0U);
	TEST_ASSERT(err == -EINVAL, "Wrote an empty Report: %d", err);

	err = bt_hogp_report_host_report_write(NULL, BT_HIDS_REPORT_TYPE_OUTPUT,
					       TEST_REPORT_ID_OUTPUT, test_output_data,
					       sizeof(test_output_data));
	TEST_ASSERT(err == -EINVAL, "Wrote a Report of no HID Service: %d", err);

	err = bt_hogp_report_host_report_write(bogus_hids, BT_HIDS_REPORT_TYPE_OUTPUT,
					       TEST_REPORT_ID_OUTPUT, test_output_data,
					       sizeof(test_output_data));
	TEST_ASSERT(err == -EINVAL, "Wrote a Report of a bogus HID Service: %d", err);

	err = bt_hogp_report_host_report_write(host_hids, BT_HIDS_REPORT_TYPE_FEATURE,
					       TEST_REPORT_ID_FEATURE_LONG, too_long,
					       sizeof(too_long));
	TEST_ASSERT(err == -EINVAL, "Wrote more than an attribute holds: %d", err);

	/* The Input Report of the HID Service is not writable */
	err = bt_hogp_report_host_report_write(host_hids, BT_HIDS_REPORT_TYPE_INPUT,
					       TEST_REPORT_ID_INPUT, test_input_data,
					       sizeof(test_input_data));
	TEST_ASSERT(err == -ENOTSUP, "Wrote the Input Report: %d", err);

	/* One read or write at a time */
	UNSET_FLAG(flag_report_read);
	err = bt_hogp_report_host_report_read(host_hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == 0, "Failed to start reading the Feature Report: %d", err);

	err = bt_hogp_report_host_report_read(host_hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(err == -EBUSY, "Started a second read: %d", err);

	err = bt_hogp_report_host_report_write(host_hids, BT_HIDS_REPORT_TYPE_OUTPUT,
					       TEST_REPORT_ID_OUTPUT, test_output_data,
					       sizeof(test_output_data));
	TEST_ASSERT(err == -EBUSY, "Started a write during a read: %d", err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_err == 0, "Failed to read the Feature Report: %d", read_err);
}

/* The HID Device makes the long Feature Report one octet longer than
 * CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORT_LEN, so reading it fails.
 */
static void test_report_too_long(void)
{
	int err;

	UNSET_FLAG(flag_report_read);
	err = bt_hogp_report_host_report_read(host_hids, BT_HIDS_REPORT_TYPE_FEATURE,
					      TEST_REPORT_ID_FEATURE_LONG);
	TEST_ASSERT(err == 0, "Failed to start reading the long Feature Report: %d", err);

	WAIT_FOR_FLAG(flag_report_read);
	TEST_ASSERT(read_err == -ENOMEM, "Read a Report longer than the buffer: %d", read_err);
	TEST_ASSERT(read_len == 0U, "Got %u octets of a failed read", read_len);
}

/* The connection of the second HID Device fails to be established when its first connection
 * events collide with those of the first one, so this connects again until the link is secured.
 */
static void connect_device_2(void)
{
	for (;;) {
		UNSET_FLAG(flag_security_2);
		UNSET_FLAG(flag_disconnected_2);

		connect_name(TEST_DEVICE_2_NAME, &conn_2);

		while (!IS_FLAG_SET(flag_security_2) && !IS_FLAG_SET(flag_disconnected_2)) {
			k_sleep(K_MSEC(10));
		}

		if (!IS_FLAG_SET(flag_disconnected_2)) {
			return;
		}

		bt_conn_drop(&conn_2);
	}
}

/* Both HID Devices are connected and discovered at the same time, and each HID Service gets its
 * own Input Reports and Report values.
 */
static void test_second_device(void)
{
	struct bt_hogp_report_host_services services;
	struct bt_conn *conn;
	int err;

	/* The connection goes away during the discovery, which reports it once */
	connect_device_2();

	UNSET_FLAG(flag_discovered_2);
	discovered_count_2 = 0U;
	disconnect_2_in_discovery = true;

	err = bt_hogp_report_host_discover(conn_2);
	TEST_ASSERT(err == 0, "Failed to start the discovery of the second HID Device: %d", err);

	WAIT_FOR_FLAG(flag_discovered_2);
	TEST_ASSERT(discovered_err_2 == -ENOTCONN,
		    "The discovery cut by the disconnection ended with %d", discovered_err_2);

	WAIT_FOR_FLAG(flag_disconnected_2);
	/* Lets the profile see the disconnection, which must not report the discovery again */
	k_sleep(K_MSEC(100));
	TEST_ASSERT(discovered_count_2 == 1U, "The discovery ended %u times", discovered_count_2);

	err = bt_hogp_report_host_conn_get(hids_2, &conn);
	TEST_ASSERT(err == -ENOTCONN, "Got the connection of a cut discovery: %d", err);

	bt_conn_drop(&conn_2);
	hids_2 = NULL;

	/* The discovery runs again on the next connection */
	connect_device_2();

	UNSET_FLAG(flag_discovered_2);
	UNSET_FLAG(flag_input_report_2);

	err = bt_hogp_report_host_discover(conn_2);
	TEST_ASSERT(err == 0, "Failed to start the discovery of the second HID Device: %d", err);

	WAIT_FOR_FLAG(flag_discovered_2);
	TEST_ASSERT(discovered_err_2 == 0, "The discovery of the second HID Device failed: %d",
		    discovered_err_2);
	TEST_ASSERT(hids_2 != NULL && hids_2 != host_hids,
		    "The HID Devices share a HID Service object");
	TEST_ASSERT(hids_on_conn(hids_2, conn_2), "Wrong connection of the second HID Service");

	err = bt_hogp_report_host_services_get(host_conn, &services);
	TEST_ASSERT(err == 0 && services.hids[0] == host_hids,
		    "The first HID Device lost its HID Service: %d", err);

	/* The second HID Device notifies once it is subscribed to */
	WAIT_FOR_FLAG(flag_input_report_2);
	TEST_ASSERT(input_len_2 == sizeof(test_input_data_2) &&
			    memcmp(input_data_2, test_input_data_2, input_len_2) == 0,
		    "Input Report of the second HID Device with the wrong payload");
	TEST_ASSERT(!IS_FLAG_SET(flag_input_report),
		    "The Input Report of the second HID Device reached the first");

	read_report_of(hids_2, BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(read_len == sizeof(test_feature_data_2) &&
			    memcmp(read_data, test_feature_data_2, read_len) == 0,
		    "Wrong Feature Report of the second HID Device");

	/* The first HID Device still holds what the Report Host wrote */
	read_report(BT_HIDS_REPORT_TYPE_FEATURE, TEST_REPORT_ID_FEATURE);
	TEST_ASSERT(read_len == sizeof(test_feature_data_new) &&
			    memcmp(read_data, test_feature_data_new, read_len) == 0,
		    "Wrong Feature Report of the first HID Device");

	/* Makes room in the key pool for the malformed HID Device */
	err = bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn_2));
	TEST_ASSERT(err == 0, "Failed to unpair the second HID Device: %d", err);
	bt_conn_drop(&conn_2);
	hids_2 = NULL;
}

static void discover_malformed(int expected_err)
{
	struct bt_hogp_report_host_services services;
	int err;

	UNSET_FLAG(flag_discovered_malformed);

	err = bt_hogp_report_host_discover(malformed_conn);
	TEST_ASSERT(err == 0, "Failed to start the discovery of a malformed HID Service: %d", err);

	WAIT_FOR_FLAG(flag_discovered_malformed);
	TEST_ASSERT(discovered_err_malformed == expected_err,
		    "Discovery of a malformed HID Service ended with %d, expected %d",
		    discovered_err_malformed, expected_err);

	err = bt_hogp_report_host_services_get(malformed_conn, &services);
	TEST_ASSERT(err == (expected_err == 0 ? 0 : -ENOENT),
		    "Got the HID Services of the discovery: %d", err);
}

/* Each discovery fails, without the Report Host pairing with the HID Device. A peer must not be
 * able to make the GATT client assert, or the discovery hang. The last HID Service is valid apart
 * from its security, and is discovered once the Report Host encrypts the link.
 */
static void test_malformed_devices(void)
{
	static const int expected_err[TEST_MALFORMED_COUNT] = {
		[TEST_MALFORMED_EMPTY] = -ENOENT,
		[TEST_MALFORMED_REPORT_LAST] = -ENOENT,
		[TEST_MALFORMED_DUPLICATE] = -EBADMSG,
		[TEST_MALFORMED_DUPLICATE_ID] = -EBADMSG,
		[TEST_MALFORMED_CCC_REFUSED] = BT_ATT_ERR_WRITE_NOT_PERMITTED,
	};
	int err;

	ARRAY_FOR_EACH(expected_err, i) {
		connect_name(TEST_MALFORMED_NAME, &malformed_conn);
		discover_malformed(expected_err[i]);

		if (i == TEST_MALFORMED_CCC_REFUSED) {
			/* The failed discovery runs again on the same connection. The CCC write
			 * succeeds now, but the link is not encrypted.
			 */
			discover_malformed(-EPERM);
			TEST_ASSERT(bt_conn_get_security(malformed_conn) < BT_SECURITY_L2,
				    "The link to the malformed HID Device is encrypted");

			/* That discovery left its subscription, which the next one keeps */
			discover_malformed(-EPERM);

			/* The HID Device puts a Feature Report in front of the Input Report, so the
			 * kept subscription is no longer for the Report at its index
			 */
			discover_malformed(-EBADMSG);

			/* Once the link is encrypted the discovery succeeds. The HID Device
			 * notifies during it, over the subscription that was kept, which the
			 * Report Host must not hand to the application before the discovery
			 * succeeded: input_report_cb() fails the test on any HID Service but
			 * the first.
			 */
			UNSET_FLAG(flag_security_malformed);
			err = bt_conn_set_security(malformed_conn, BT_SECURITY_L2);
			TEST_ASSERT(err == 0, "Failed to encrypt the link: %d", err);
			WAIT_FOR_FLAG(flag_security_malformed);
			discover_malformed(0);
		}

		err = bt_testlib_disconnect(&malformed_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(err == 0, "Failed to disconnect a malformed HID Device: %d", err);
	}
}

static void test_callbacks_register(void)
{
	int err;

	err = bt_hogp_report_host_cb_register(NULL);
	TEST_ASSERT(err == -EINVAL, "Registered no callbacks: %d", err);

	err = bt_hogp_report_host_cb_unregister(NULL);
	TEST_ASSERT(err == -EINVAL, "Unregistered no callbacks: %d", err);

	err = bt_hogp_report_host_cb_unregister(&hogp_cb);
	TEST_ASSERT(err == -EALREADY, "Unregistered callbacks that were never registered: %d", err);

	err = bt_hogp_report_host_cb_register(&hogp_cb);
	TEST_ASSERT(err == 0, "Failed to register the callbacks: %d", err);

	err = bt_hogp_report_host_cb_register(&hogp_cb);
	TEST_ASSERT(err == -EALREADY, "Registered the callbacks twice: %d", err);

	err = bt_hogp_report_host_cb_register(&listener_cb);
	TEST_ASSERT(err == 0, "Failed to register the second callbacks: %d", err);
}

static void test_hogp_report_host(void)
{
	struct bt_hogp_report_host_hids *old_hids;
	struct bt_conn *old_conn;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed: %d", err);

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load the settings: %d", err);

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open the sync channel");

	err = bt_conn_cb_register(&host_conn_cb);
	TEST_ASSERT(err == 0, "Failed to register the connection callbacks: %d", err);

	test_callbacks_register();

	err = bt_hogp_report_host_discover(NULL);
	TEST_ASSERT(err == -EINVAL, "Discovered no connection: %d", err);

	connect_device(true);
	verify_no_state(host_conn);

	discover(1U);
	TEST_ASSERT(listener_discovered == 1U, "The second callbacks saw %u discoveries",
		    listener_discovered);
	verify_discovery();
	test_getter_validation();

	/* The HID Device notifies the Input Report once we are subscribed */
	bk_sync_send();
	wait_for_input_report();
	TEST_ASSERT(listener_input_reports == 1U, "The second callbacks saw %u Input Reports",
		    listener_input_reports);

	test_report_validation();
	test_reports();

	/* The HID Device has checked both SET_REPORTs and lengthened the long Feature Report */
	bk_sync_wait();

	test_report_too_long();

	/* The state of a connection is dropped when it disconnects */
	old_conn = bt_conn_ref(host_conn);
	old_hids = host_hids;
	err = bt_testlib_disconnect(&host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);
	verify_gone(old_conn, old_hids);

	err = bt_hogp_report_host_discover(old_conn);
	TEST_ASSERT(err == -ENOTCONN, "Discovered a disconnected HID Device: %d", err);
	bt_conn_unref(old_conn);

	/* An unregistered callback structure is not called any more */
	err = bt_hogp_report_host_cb_unregister(&listener_cb);
	TEST_ASSERT(err == 0, "Failed to unregister the second callbacks: %d", err);

	err = bt_hogp_report_host_cb_unregister(&listener_cb);
	TEST_ASSERT(err == -EALREADY, "Unregistered the second callbacks twice: %d", err);

	/* The HID Device is bonded now; the subscription is set up again. Keeping the default
	 * ATT_MTU makes the Report Map take several ATT reads.
	 */
	connect_device(false);
	discover(2U);
	verify_discovery();

	bk_sync_send();
	wait_for_input_report();

	TEST_ASSERT(listener_discovered == 1U && listener_input_reports == 1U,
		    "Unregistered callbacks were called");

	test_second_device();

	err = bt_testlib_disconnect(&host_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Failed to disconnect: %d", err);

	/* A new connection can get the HID Service object of the old one */
	host_hids = NULL;

	test_malformed_devices();

	TEST_PASS("HOGP Report Host test passed");
}

static const struct bst_test_instance test_hogp_report_host_defs[] = {
	{
		.test_id = "hogp_report_host",
		.test_descr = "HOGP Report Host",
		.test_main_f = test_hogp_report_host,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_hogp_report_host_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_hogp_report_host_defs);
}
