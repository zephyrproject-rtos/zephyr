/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HOGP Device (HID over GATT Profile Device) API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_DEVICE_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_DEVICE_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_hogp_device HID over GATT Profile Device
 * @ingroup bluetooth
 * @{
 */

/** HID Report descriptor */
struct bt_hogp_device_report {
	/** Report ID */
	uint8_t id;
	/** Report type (BT_HID_REPORT_TYPE_xxx) */
	uint8_t type;
};

/** HID Information characteristic value */
struct bt_hogp_device_info {
	/** HID specification version (BCD, e.g. 0x0111 for 1.1.1) */
	uint16_t bcd_hid;
	/** HID country code */
	uint8_t b_country_code;
	/** HID flags (bit0: RemoteWake, bit1: NormallyConnectable) */
	uint8_t flags;
};

/** HOGP Device callbacks */
struct bt_hogp_device_cb {
	/** Called when a BLE host connects */
	void (*connected)(struct bt_conn *conn);
	/** Called when a BLE host disconnects */
	void (*disconnected)(struct bt_conn *conn, uint8_t reason);
	/** Called on GET_REPORT from host */
	void (*get_report)(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, uint16_t buf_size);
	/** Called on SET_REPORT from host */
	void (*set_report)(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, const uint8_t *data,
			   uint16_t len);
	/** Called on Protocol Mode write from host */
	void (*set_protocol)(struct bt_conn *conn, uint8_t protocol);
	/** Called on HID Control Point write from host */
	void (*ctrl_point)(struct bt_conn *conn, uint8_t value);
	/** Called when CCC state changes for a report */
	void (*ccc_changed)(struct bt_conn *conn, uint8_t report_id,
			    uint8_t report_type, bool enabled);
};

/** HOGP Device initialization parameters */
struct bt_hogp_device_init_param {
	/** HID Information values */
	struct bt_hogp_device_info info;
	/** HID Report Map descriptor data */
	const uint8_t *report_map;
	/** Length of report_map in bytes */
	uint16_t report_map_len;
	/** Array of report definitions */
	const struct bt_hogp_device_report *reports;
	/** Number of reports in the array */
	uint8_t report_count;
	/** Application callbacks */
	const struct bt_hogp_device_cb *cb;
};

/**
 * @brief Register HOGP Device (HID Service).
 *
 * Builds and registers GATT HID Service attributes dynamically.
 * Only one device instance is supported (singleton).
 *
 * @param param Initialization parameters.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_device_register(const struct bt_hogp_device_init_param *param);

/**
 * @brief Unregister HOGP Device.
 *
 * Disconnects all connections and unregisters GATT service.
 */
void bt_hogp_device_unregister(void);

/**
 * @brief Disconnect a specific Host connection.
 *
 * @param conn Connection to disconnect.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_device_disconnect(struct bt_conn *conn);

/**
 * @brief Send an Input Report to the connected Host.
 *
 * Uses GATT notification. If conn is NULL, notifies all subscribed connections.
 *
 * @param conn       Target connection, or NULL for all subscribed.
 * @param report_id  HID Report ID.
 * @param data       Report payload (excluding Report ID byte).
 * @param len        Payload length.
 * @param func       Optional completion callback (NULL if not needed).
 * @param user_data  User data passed to completion callback.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_device_send_report(struct bt_conn *conn, uint8_t report_id,
			       const uint8_t *data, uint16_t len,
			       bt_gatt_complete_func_t func, void *user_data);

/**
 * @brief Respond to a GET_REPORT request.
 *
 * @note Phase 1: Returns -ENOTSUP. Reserved for future async GATT read.
 */
int bt_hogp_device_get_report_response(struct bt_conn *conn,
				       uint8_t report_id,
				       uint8_t report_type,
				       const uint8_t *data, uint16_t len);

/**
 * @brief Report error to Host in response to GET_REPORT.
 *
 * @note Phase 1: Returns -ENOTSUP. Reserved for future async GATT read.
 */
int bt_hogp_device_report_error(struct bt_conn *conn, uint8_t error);

/**
 * @brief Initiate Virtual Cable Unplug (disconnect + remove bond).
 *
 * @param conn Connection to unplug.
 * @return 0 on success, negative errno on failure.
 */
int bt_hogp_device_virtual_cable_unplug(struct bt_conn *conn);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HOGP_DEVICE_H_ */
