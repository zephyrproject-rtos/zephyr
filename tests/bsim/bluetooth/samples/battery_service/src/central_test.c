/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"
#include <argparse.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_bas, CONFIG_BT_BAS_LOG_LEVEL);

static struct bt_conn *default_conn;
static bt_addr_le_t peer = {};

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;

static struct bt_gatt_subscribe_params battery_level_notify_params;
static struct bt_gatt_subscribe_params battery_level_status_sub_params;
static struct bt_gatt_subscribe_params battery_critical_status_sub_params;

/*
 * Battery Service  test:
 *   We expect to find a connectable peripheral to which we will
 *   connect and discover Battery Service
 *
 *   Test the Read/Notify/Indicate Characteristics of BAS
 */

#define WAIT_TIME                  10 /*seconds*/
#define BAS_BLS_IND_RECEIVED_COUNT 20
#define BAS_BLS_NTF_RECEIVED_COUNT 20

static DEFINE_FLAG(notification_count_reached);
static DEFINE_FLAG(indication_count_reached);
static DEFINE_FLAG(bcs_char_read);

/* Callback for handling Battery Critical Status Read Response */
static uint8_t battery_critical_status_read_cb(struct bt_conn *conn, uint8_t err,
					       struct bt_gatt_read_params *params, const void *data,
					       uint16_t length)
{
	TEST_ASSERT(err == 0, "Failed to read Battery critical status (err %d)", err);
	TEST_ASSERT(length > 0, "No data is sent");

	if (data) {
		uint8_t status_byte = *(uint8_t *)data;

		printk("[READ]  BAS Critical Status:\n");
		printk("Battery state: %s\n",
		       (status_byte & BT_BAS_BCS_BATTERY_CRITICAL_STATE) ? "Critical" : "Normal");
		printk("Immediate service: %s\n",
		       (status_byte & BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED) ? "Required"
									     : "Not Required");
	}
	SET_FLAG(bcs_char_read);
	return BT_GATT_ITER_STOP;
}

static struct bt_gatt_read_params read_bcs_params = {
	.func = battery_critical_status_read_cb,
	.by_uuid.uuid = BT_UUID_BAS_BATTERY_CRIT_STATUS,
	.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
	.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
};

/* Callback for handling Battery Level Read Response */
static uint8_t battery_level_read_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params, const void *data,
				     uint16_t length)
{
	TEST_ASSERT(err == 0, "Failed to read Battery Level (err %d)", err);
	if (data) {
		LOG_DBG("[READ] BAS Battery Level: %d%%\n", *(const uint8_t *)data);
	}

	return BT_GATT_ITER_STOP;
}

static struct bt_gatt_read_params read_blvl_params = {
	.func = battery_level_read_cb,
	.by_uuid.uuid = BT_UUID_BAS_BATTERY_LEVEL_STATUS,
	.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
	.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
};

extern enum bst_result_t bst_result;

static void test_bas_central_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_bas_central_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		TEST_FAIL("test_bas_central failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

/* Callback for handling Battery Level Notifications */
static uint8_t battery_level_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t length)
{
	if (data) {
		LOG_INF("[NOTIFICATION] BAS Battery Level: %d%%", *(const uint8_t *)data);
	} else {
		LOG_INF("Battery Level Notifications disabled");
	}
	return BT_GATT_ITER_CONTINUE;
}

static bool parse_battery_level_status(const uint8_t *data, uint16_t length)
{
	/* Check minimum length for parsing flags and power state */
	if (length < 3) {
		TEST_FAIL("Invalid data length: %d", length);
		return false;
	}

	/* Parse flags (first byte) */
	uint8_t flags = data[0];

	LOG_INF("Parsed Flags: 0x%02x", flags);

	if (flags & BT_BAS_BLS_FLAG_IDENTIFIER_PRESENT) {
		LOG_INF("  Identifier Present");
	} else {
		LOG_INF("  Identifier Not Present");
	}

	if (flags & BT_BAS_BLS_FLAG_BATTERY_LEVEL_PRESENT) {
		LOG_INF("  Battery Level Present");
	} else {
		LOG_INF("  Battery Level Not Present");
	}

	if (flags & BT_BAS_BLS_FLAG_ADDITIONAL_STATUS_PRESENT) {
		LOG_INF("  Additional Status Present");
	} else {
		LOG_INF("  Additional Status Not Present");
	}

	/* Parse power state (next 2 bytes) */
	uint16_t power_state = sys_get_le16(&data[1]);

	LOG_INF("Parsed Power State: 0x%04x", power_state);
	/* Print out each power state value */
	LOG_INF("  Battery Present: %s", (power_state & BIT(0)) ? "Yes" : "No");

	uint8_t wired_power = (power_state >> 1) & 0x03;

	switch (wired_power) {
	case 0:
		LOG_INF("  Wired Power Source: No");
		break;
	case 1:
		LOG_INF("  Wired Power Source: Yes");
		break;
	case 2:
		LOG_INF("  Wired Power Source: Unknown");
		break;
	default:
		LOG_INF("  Wired Power Source: RFU");
		break;
	}

	uint8_t wireless_power = (power_state >> 3) & 0x03;

	switch (wireless_power) {
	case 0:
		LOG_INF("  Wireless Power Source: No");
		break;
	case 1:
		LOG_INF("  Wireless Power Source: Yes");
		break;
	case 2:
		LOG_INF("  Wireless Power Source: Unknown");
		break;
	default:
		LOG_INF("  Wireless Power Source: RFU");
		break;
	}

	uint8_t charge_state = (power_state >> 5) & 0x03;

	switch (charge_state) {
	case 0:
		LOG_INF("  Battery Charge State: Unknown");
		break;
	case 1:
		LOG_INF("  Battery Charge State: Charging");
		break;
	case 2:
		LOG_INF("  Battery Charge State: Discharging (Active)");
		break;
	case 3:
		LOG_INF("  Battery Charge State: Discharging (Inactive)");
		break;
	}

	uint8_t charge_level = (power_state >> 7) & 0x03;

	switch (charge_level) {
	case 0:
		LOG_INF("  Battery Charge Level: Unknown");
		break;
	case 1:
		LOG_INF("  Battery Charge Level: Good");
		break;
	case 2:
		LOG_INF("  Battery Charge Level: Low");
		break;
	case 3:
		LOG_INF("  Battery Charge Level: Critical");
		break;
	}

	uint8_t charging_type = (power_state >> 9) & 0x07;

	switch (charging_type) {
	case 0:
		LOG_INF("  Charging Type: Unknown or Not Charging");
		break;
	case 1:
		LOG_INF("  Charging Type: Constant Current");
		break;
	case 2:
		LOG_INF("  Charging Type: Constant Voltage");
		break;
	case 3:
		LOG_INF("  Charging Type: Trickle");
		break;
	case 4:
		LOG_INF("  Charging Type: Float");
		break;
	default:
		LOG_INF("  Charging Type: RFU");
		break;
	}

	uint8_t charging_fault = (power_state >> 12) & 0x07;

	if (charging_fault) {
		LOG_INF("  Charging Fault Reason: %s%s%s",
			(charging_fault & BIT(0)) ? "Battery " : "",
			(charging_fault & BIT(1)) ? "External Power Source " : "",
			(charging_fault & BIT(2)) ? "Other " : "");
	} else {
		LOG_INF("  Charging Fault Reason: None");
	}

	/* Optional: Check if identifier is present */
	if (IS_ENABLED(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)) {
		/* Check if length is sufficient for identifier */
		if (length < 5) {
			TEST_FAIL("Invalid data length for identifier");
			return false;
		}

		/* Parse identifier (next 2 bytes) */
		uint16_t identifier = sys_get_le16(&data[3]);

		LOG_INF("Parsed Identifier: 0x%04x", identifier);
	}

	/* Optional: Check if battery level is present */
	if (IS_ENABLED(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)) {
		/* Check if length is sufficient for battery level */
		if (length < 6) {
			TEST_FAIL("Invalid data length for battery level");
			return false;
		}

		/* Parse battery level (next byte) */
		uint8_t battery_level = data[5];

		LOG_INF("Parsed Battery Level: %d%%", battery_level);
	}

	/* Optional: Check if additional status is present */
	if (IS_ENABLED(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)) {
		/* Check if length is sufficient for additional status */
		if (length < 7) {
			TEST_FAIL("Invalid data length for additional status");
			return false;
		}

		/* Parse additional status (next byte) */
		uint8_t additional_status = data[6];

		LOG_INF("Parsed Additional Status: 0x%02x", additional_status);

		/* Print out additional status values */
		uint8_t service_required = additional_status & 0x03;

		switch (service_required) {
		case 0:
			LOG_INF("  Service Required: False");
			break;
		case 1:
			LOG_INF("  Service Required: True");
			break;
		case 2:
			LOG_INF("  Service Required: Unknown");
			break;
		default:
			LOG_INF("  Service Required: RFU");
			break;
		}

		bool battery_fault = (additional_status & BIT(2)) ? true : false;

		LOG_INF("  Battery Fault: %s", battery_fault ? "Yes" : "No");
	}

	return true;
}

static unsigned char battery_critical_status_indicate_cb(struct bt_conn *conn,
							 struct bt_gatt_subscribe_params *params,
							 const void *data, uint16_t length)
{
	if (!data) {
		LOG_INF("BAS critical status indication disabled\n");
	} else {
		uint8_t status_byte = ((uint8_t *)data)[0];

		printk("[INDICATION]  BAS Critical Status:\n");
		printk("Battery state: %s\n",
		       (status_byte & BT_BAS_BCS_BATTERY_CRITICAL_STATE) ? "Critical" : "Normal");
		printk("Immediate service: %s\n",
		       (status_byte & BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED) ? "Required"
									     : "Not Required");
	}
	return BT_GATT_ITER_CONTINUE;
}

static unsigned char battery_level_status_indicate_cb(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data, uint16_t length)
{
	if (!data) {
		LOG_INF("bas level status indication disabled\n");
	} else {
		static int ind_received;

		printk("[INDICATION]  BAS Battery Level Status: ");
		for (int i = 0; i < length; i++) {
			printk("%02x ", ((uint8_t *)data)[i]);
		}
		printk("\n");

		if (parse_battery_level_status(data, length)) {
			LOG_INF("Notification parsed successfully");
		} else {
			LOG_ERR("Notification parsing failed");
		}

		if (ind_received++ > BAS_BLS_IND_RECEIVED_COUNT) {
			SET_FLAG(indication_count_reached);
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t battery_level_status_notify_cb(struct bt_conn *conn,
					      struct bt_gatt_subscribe_params *params,
					      const void *data, uint16_t length)
{
	if (!data) {
		LOG_INF("bas level status notification disabled\n");
	} else {
		static int notify_count;

		printk("[NOTIFICATION]  BAS Battery Level Status: ");
		for (int i = 0; i < length; i++) {
			printk("%02x ", ((uint8_t *)data)[i]);
		}
		printk("\n");

		if (parse_battery_level_status(data, length)) {
			LOG_INF("Notification parsed successfully");
		} else {
			LOG_ERR("Notification parsing failed");
		}

		if (notify_count++ > BAS_BLS_NTF_RECEIVED_COUNT) {
			SET_FLAG(notification_count_reached);
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_battery_level(const struct bt_gatt_attr *attr)
{
	int err;
	struct bt_gatt_attr *ccc_attr = bt_gatt_find_by_uuid(attr, 0, BT_UUID_GATT_CCC);

	battery_level_notify_params = (struct bt_gatt_subscribe_params){
		.ccc_handle = bt_gatt_attr_get_handle(ccc_attr),
		.value_handle = bt_gatt_attr_value_handle(attr),
		.value = BT_GATT_CCC_NOTIFY,
		.notify = battery_level_notify_cb,
	};

	err = bt_gatt_subscribe(default_conn, &battery_level_notify_params);
	if (err && err != -EALREADY) {
		TEST_FAIL("Subscribe failed (err %d)\n", err);
	} else {
		LOG_DBG("Battery level [SUBSCRIBED]\n");
	}

	err = bt_gatt_read(default_conn, &read_blvl_params);
	if (err != 0) {
		TEST_FAIL("Battery Level Read failed (err %d)\n", err);
	}
}

static void subscribe_battery_critical_status(const struct bt_gatt_attr *attr)
{
	int err;
	struct bt_gatt_attr *ccc_attr = bt_gatt_find_by_uuid(attr, 0, BT_UUID_GATT_CCC);

	battery_critical_status_sub_params = (struct bt_gatt_subscribe_params){
		.ccc_handle = bt_gatt_attr_get_handle(ccc_attr),
		.value_handle = bt_gatt_attr_value_handle(attr),
		.value = BT_GATT_CCC_INDICATE,
		.notify = battery_critical_status_indicate_cb,
	};

	err = bt_gatt_subscribe(default_conn, &battery_critical_status_sub_params);
	if (err && err != -EALREADY) {
		TEST_FAIL("Subscribe failed (err %d)\n", err);
	} else {
		LOG_DBG("Battery critical status [SUBSCRIBED]\n");
	}

	err = bt_gatt_read(default_conn, &read_bcs_params);
	if (err != 0) {
		TEST_FAIL("Battery Critical Status Read failed (err %d)\n", err);
	}
}

static void subscribe_battery_level_status(const struct bt_gatt_attr *attr)
{
	int err;
	struct bt_gatt_attr *ccc_attr = bt_gatt_find_by_uuid(attr, 0, BT_UUID_GATT_CCC);

	if (get_device_nbr() == 1) { /* One device for Indication */
		battery_level_status_sub_params = (struct bt_gatt_subscribe_params){
			.ccc_handle = bt_gatt_attr_get_handle(ccc_attr),
			.value_handle = bt_gatt_attr_value_handle(attr),
			.value = BT_GATT_CCC_INDICATE,
			.notify = battery_level_status_indicate_cb,
		};
	} else { /* Other device for Notification */
		battery_level_status_sub_params = (struct bt_gatt_subscribe_params){
			.ccc_handle = bt_gatt_attr_get_handle(ccc_attr),
			.value_handle = bt_gatt_attr_value_handle(attr),
			.value = BT_GATT_CCC_NOTIFY,
			.notify = battery_level_status_notify_cb,
		};
	}

	err = bt_gatt_subscribe(default_conn, &battery_level_status_sub_params);
	if (err && err != -EALREADY) {
		TEST_FAIL("Subscribe failed (err %d)\n", err);
	} else {
		LOG_DBG("Battery level status [SUBSCRIBED]\n");
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params, int err)
{
	if (!attr) {
		LOG_DBG("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS)) {
		LOG_DBG("Battery Service\n");
		memcpy(&uuid, BT_UUID_BAS_BATTERY_LEVEL, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TEST_FAIL("Discover failed (err %d)\n", err);
		}

	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS_BATTERY_LEVEL)) {
		LOG_DBG("Subscribe Battery Level Char\n");
		subscribe_battery_level(attr);

		memcpy(&uuid, BT_UUID_BAS_BATTERY_LEVEL_STATUS, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TEST_FAIL("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS_BATTERY_LEVEL_STATUS)) {
		LOG_DBG("Subscribe Batterry Level Status Char\n");
		subscribe_battery_level_status(attr);

		memcpy(&uuid, BT_UUID_BAS_BATTERY_CRIT_STATUS, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TEST_FAIL("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS_BATTERY_CRIT_STATUS)) {
		LOG_DBG("Subscribe Batterry Critical Status Char\n");
		subscribe_battery_critical_status(attr);
	}
	return BT_GATT_ITER_STOP;
}

static void discover_bas_service(struct bt_conn *conn)
{
	int err;

	LOG_DBG("%s\n", __func__);

	memcpy(&uuid, BT_UUID_BAS, sizeof(uuid));
	discover_params.uuid = &uuid.uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		TEST_FAIL("Discover failed(err %d)\n", err);
		return;
	}
}

static void test_bas_central_main(void)
{
	int err;

	/* Mark test as in progress. */
	TEST_START("central");
	/* bk_sync_init only works between two devices in a simulation, with IDs 0 and 1. */
	if (get_device_nbr() == 1) {
		/* Initialize device sync library */
		bk_sync_init();
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized\n");

	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	/* Create a connection using that address */
	err = bt_testlib_connect(&peer, &default_conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	LOG_DBG("Connected");
	discover_bas_service(default_conn);

	if (get_device_nbr() == 1) {
		WAIT_FOR_FLAG(indication_count_reached);
		LOG_INF("Indication Count Reached!");
	} else {
		WAIT_FOR_FLAG(notification_count_reached);
		LOG_INF("Notification Count Reached!");
	}
	/* bk_sync_send only works between two devices in a simulation, with IDs 0 and 1. */
	if (get_device_nbr() == 1) {
		bk_sync_send();
	}

	printk("Read BCS once peripheral sets BLS Addl Status Service Required Flag to false\n");

	UNSET_FLAG(bcs_char_read);

	err = bt_gatt_read(default_conn, &read_bcs_params);
	if (err != 0) {
		TEST_FAIL("Battery Critical Status Read failed (err %d)\n", err);
	}

	WAIT_FOR_FLAG(bcs_char_read);

	if (get_device_nbr() == 1) {
		bk_sync_send();
	}

	bst_result = Passed;
	TEST_PASS("Central Test Passed");
}

static const struct bst_test_instance test_bas_central[] = {
	{
		.test_id = "central",
		.test_descr =
			"Battery Service test. It expects that a peripheral device can be found. "
			"The test will pass if it can receive notifications and indications more "
			"than the threshold set within 15 sec. ",
		.test_pre_init_f = test_bas_central_init,
		.test_tick_f = test_bas_central_tick,
		.test_main_f = test_bas_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bas_central_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_bas_central);
	return tests;
}
