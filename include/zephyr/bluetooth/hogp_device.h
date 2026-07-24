/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth HOGP Device (HID over GATT Profile Device) API.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hids.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_hogp_device HID over GATT Profile Device
 *
 * @since 4.2
 * @version 0.1.0
 *
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
	/** HID flags (BT_HID_INFO_FLAG_*) */
	uint8_t flags;
};

/** HOGP Device callbacks */
struct bt_hogp_device_cb {
	/**
	 * Called on GET_REPORT from host.
	 * Fill buf with current report data and return bytes written,
	 * or negative errno on error (e.g. -ENOENT if report not found).
	 */
	ssize_t (*get_report)(struct bt_conn *conn, uint8_t report_type,
			      uint8_t report_id, uint8_t *buf,
			      uint16_t buf_size);
	/** Called on SET_REPORT from host (Output or Feature report) */
	void (*set_report)(struct bt_conn *conn, uint8_t report_type,
			   uint8_t report_id, const uint8_t *data,
			   uint16_t len);
	/**
	 * Protocol Mode changed notification.
	 * The profile has already updated the internal state. Use
	 * bt_hogp_device_get_protocol_mode() to query current mode.
	 */
	void (*protocol_mode_changed)(struct bt_conn *conn, uint8_t protocol);
	/**
	 * Suspend state changed notification.
	 * The profile has already updated the internal state. Use
	 * bt_hogp_device_is_suspended() to query current state.
	 */
	void (*suspend_changed)(struct bt_conn *conn, bool suspended);
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
 * @param[in] param Initialization parameters.
 *
 * @retval 0 Success.
 * @retval -EALREADY Already registered.
 * @retval -EINVAL Invalid parameters.
 * @retval -ENOMEM Report map or report count exceeds limits.
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
 * @param[in] conn Connection to disconnect.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid connection.
 */
int bt_hogp_device_disconnect(struct bt_conn *conn);

/**
 * @brief Send an Input Report to the connected Host.
 *
 * Uses GATT notification. If conn is NULL, notifies all subscribed connections.
 *
 * @param[in] conn       Target connection, or NULL for all subscribed.
 * @param[in] report_id  HID Report ID.
 * @param[in] data       Report payload (excluding Report ID byte).
 * @param[in] len        Payload length.
 * @param[in] func       Optional completion callback (NULL if not needed).
 * @param[in] user_data  User data passed to completion callback.
 *
 * @retval 0 Success.
 * @retval -ESRCH Device not registered.
 * @retval -ENOENT Report ID not found.
 */
int bt_hogp_device_send_report(struct bt_conn *conn, uint8_t report_id,
			       const uint8_t *data, uint16_t len,
			       bt_gatt_complete_func_t func, void *user_data);


/**
 * @brief Get current Protocol Mode.
 *
 * Returns the protocol mode managed internally by the profile.
 *
 * @return BT_HID_PROTOCOL_BOOT or BT_HID_PROTOCOL_REPORT.
 */
uint8_t bt_hogp_device_get_protocol_mode(void);

/**
 * @brief Check if device is in Suspend state.
 *
 * Returns true if the Host has written Suspend to the HID Control Point.
 *
 * @return true if suspended, false otherwise.
 */
bool bt_hogp_device_is_suspended(void);

/**
 * @brief Initiate Virtual Cable Unplug (disconnect + remove bond).
 *
 * @param[in] conn Connection to unplug.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid connection.
 * @retval -ENOENT Peer address not found.
 */
int bt_hogp_device_virtual_cable_unplug(struct bt_conn *conn);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOGP_DEVICE_H_ */
