/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* HID Device role of the HID over GATT Profile (HOGP).
 *
 * The role is a composition of the services the profile makes mandatory, which
 * the application puts together:
 *
 * - the HID Service (CONFIG_BT_HIDS), registered below,
 * - the Battery Service (CONFIG_BT_BAS),
 * - the Device Information Service (CONFIG_BT_DIS) including the PnP ID
 *   characteristic the profile requires (CONFIG_BT_DIS_PNP),
 *
 * on a bondable (CONFIG_BT_BONDABLE) and encrypted link, as the profile
 * requires. The profile also recommends the Peripheral Security Request
 * procedure to inform the Host of the security requirements of the Device,
 * which this sample does from its connected callback.
 *
 * The advertising data and the NormallyConnectable behavior belong to the
 * application as well, as it owns the advertising state.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/class/hid.h>

LOG_MODULE_REGISTER(peripheral_hogp, LOG_LEVEL_INF);

/* The Report Map below has no Report ID item, so the Report is the only one of
 * its type and its Report Reference descriptor carries a Report ID of 0.
 */
#define REPORT_ID_MOUSE 0x00U

#define MOUSE_BUTTON_COUNT 2

/* Byte indices and length of the mouse Input Report. The layout matches
 * HID_MOUSE_REPORT_DESC() below: buttons + X + Y + wheel, no Report ID.
 */
enum mouse_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_LEN = 4,
};

#define MOUSE_BTN_LEFT  0
#define MOUSE_BTN_RIGHT 1

/* Movement one key press reports */
#define MOUSE_STEP 10

#define BATTERY_LEVEL 100U

/* Standard mouse Report Map from the HID class helpers, the same one the
 * USB HID mouse sample uses.
 */
static const uint8_t report_map[] = HID_MOUSE_REPORT_DESC(MOUSE_BUTTON_COUNT);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL), BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (CONFIG_BT_DEVICE_APPEARANCE & 0xFF),
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* All HID Service characteristics require an encrypted link, so the Device asks
 * the Host for security instead of waiting for a failing read.
 */
static void request_security(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get connection info (err %d)", err);
		return;
	}

	/* The HID Device is a GAP Peripheral */
	if (info.type != BT_CONN_TYPE_LE || info.role != BT_CONN_ROLE_PERIPHERAL) {
		return;
	}

	if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
		return;
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err != 0) {
		LOG_WRN("Failed to request security (err %d)", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		LOG_ERR("Failed to connect to %s (err 0x%02x %s)", bt_conn_dst_str(conn), err,
			bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("Connected %s", bt_conn_dst_str(conn));

	request_security(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected from %s (reason 0x%02x %s)", bt_conn_dst_str(conn), reason,
		bt_hci_err_to_str(reason));
}

/* The HID Information declares the Device Normally Connectable and HOGP
 * requires a Device that is not connected to a Host to be in Undirected
 * Connectable Mode, so the advertiser is started again after every
 * disconnection. BT_LE_ADV_OPT_CONN stops the advertiser when the connection is
 * created, and the connection object is only free to be reused once the
 * recycled callback has run.
 */
static void start_advertising(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		LOG_ERR("Failed to start advertising (err %d)", err);
		return;
	}

	LOG_INF("Advertising as %s", CONFIG_BT_DEVICE_NAME);
}

static void recycled(void)
{
	start_advertising();
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err == BT_SECURITY_ERR_SUCCESS) {
		LOG_INF("Security level of %s is %u", bt_conn_dst_str(conn), level);
	} else {
		LOG_ERR("Security failed on %s, level %u, err %s (%d)", bt_conn_dst_str(conn),
			level, bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
	.security_changed = security_changed,
};

static void ctrl_point(struct bt_conn *conn, enum bt_hids_ctrl_point cmd)
{
	/* No default case, so that a new HID Control Point command does not
	 * silently get the message of an existing one.
	 */
	switch (cmd) {
	case BT_HIDS_CTRL_SUSPEND:
		LOG_INF("%s suspended the Device", bt_conn_dst_str(conn));
		break;
	case BT_HIDS_CTRL_EXIT_SUSPEND:
		LOG_INF("%s resumed the Device", bt_conn_dst_str(conn));
		break;
	}
}

static void ccc_changed(struct bt_conn *conn, uint8_t report_id, uint8_t report_type, bool enabled)
{
	ARG_UNUSED(report_type);

	LOG_INF("%s %s notifications of Report ID %u", bt_conn_dst_str(conn),
		enabled ? "enabled" : "disabled", report_id);
}

static const struct bt_hids_cb hids_cb = {
	.ctrl_point = ctrl_point,
	.ccc_changed = ccc_changed,
};

static const struct bt_hids_register_param hids_param = {
	/* clang-format off */
	.info = {
		.bcd_hid = 0x0111,
		.b_country_code = 0x00,
		/* The Device reconnects on its own, so it is normally
		 * connectable.
		 */
		.flags = BT_HID_INFO_FLAG_REMOTE_WAKE |
			 BT_HID_INFO_FLAG_NORMALLY_CONNECTABLE,
	},
	/* clang-format on */
	.report_map = report_map,
	.report_map_len = sizeof(report_map),
	.input_report_ids = {REPORT_ID_MOUSE},
	.cb = &hids_cb,
};

static void send_mouse_report(const uint8_t report[MOUSE_REPORT_LEN])
{
	int err;

	/* Keep what a Host reads with GET_REPORT in step with what is notified */
	(void)bt_hids_report_set(BT_HID_REPORT_TYPE_INPUT, REPORT_ID_MOUSE, report,
				 MOUSE_REPORT_LEN);

	err = bt_hids_send_report(NULL, REPORT_ID_MOUSE, report, MOUSE_REPORT_LEN, NULL, NULL);
	/* -ENOTCONN means that no Host is subscribed to the Input Report, which
	 * is the normal state while no Host is connected. Subscriptions of a
	 * bonded Host are restored by GATT without going through the
	 * ccc_changed callback, so the callback cannot be used to tell.
	 */
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Failed to notify the Input Report (err %d)", err);
	}

	/* The Boot Mouse Input Report is the first three octets of the layout
	 * above, buttons, X and Y, which is what the Boot Protocol Mode of a
	 * mouse defines. A Boot Host subscribes to it instead of the Report.
	 * Which of the two a Host uses is the Host's decision, so both are
	 * notified and the CCC of each decides whether anything is sent.
	 */
	(void)bt_hids_boot_report_set(BT_HIDS_BOOT_REPORT_MOUSE_INPUT, report,
				      BT_HIDS_BOOT_MOUSE_IN_LEN);

	err = bt_hids_boot_report_send(NULL, BT_HIDS_BOOT_REPORT_MOUSE_INPUT, report,
				       BT_HIDS_BOOT_MOUSE_IN_LEN, NULL, NULL);
	if (err != 0 && err != -ENOTCONN) {
		LOG_WRN("Failed to notify the Boot Mouse Input Report (err %d)", err);
	}
}

/* Turn board button events into mouse Input Reports, the same way the USB HID
 * mouse sample does: two buttons plus relative motion driven by four keys. The
 * board's devicetree maps its buttons to INPUT_KEY_0..3, see the bundled
 * native_sim overlay. gpio-keys delivers events from a workqueue, so notifying
 * from here is safe.
 */
static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t report[MOUSE_REPORT_LEN];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_KEY_0:
		WRITE_BIT(report[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, evt->value);
		break;
	case INPUT_KEY_1:
		WRITE_BIT(report[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_RIGHT, evt->value);
		break;
	case INPUT_KEY_2:
		if (evt->value != 0) {
			report[MOUSE_X_REPORT_IDX] = MOUSE_STEP;
		}

		break;
	case INPUT_KEY_3:
		if (evt->value != 0) {
			report[MOUSE_Y_REPORT_IDX] = MOUSE_STEP;
		}

		break;
	default:
		LOG_WRN("Unrecognized input code %u value %d", evt->code, evt->value);
		return;
	}

	send_mouse_report(report);

	/* Relative axes are one-shot, button bits persist until released */
	report[MOUSE_X_REPORT_IDX] = 0U;
	report[MOUSE_Y_REPORT_IDX] = 0U;
	report[MOUSE_WHEEL_REPORT_IDX] = 0U;
}
INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Failed to enable Bluetooth (err %d)", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_hids_register(&hids_param);
	if (err != 0) {
		LOG_ERR("Failed to register the HID Service (err %d)", err);
		return 0;
	}

	err = bt_bas_set_battery_level(BATTERY_LEVEL);
	if (err != 0) {
		LOG_ERR("Failed to set the battery level (err %d)", err);
		return 0;
	}

	start_advertising();

	LOG_INF("Press the board buttons to move the pointer");

	return 0;
}
