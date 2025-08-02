/** @file
 *  @brief HID Device Protocol handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HID_DEVICE_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HID_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#define BT_HID_MAX_MTU         64
#define BT_HID_REPORT_DATA_LEN 64

/** @brief HID handshake response code */
enum {
	BT_HID_HANDSHAKE_RSP_SUCCESS = 0,
	BT_HID_HANDSHAKE_RSP_NOT_READY,
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID,
	BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ,
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM,
	BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN = 14,
	BT_HID_HANDSHAKE_RSP_ERR_FATAL,
};

/** @brief HID protocol parameters for set protocal */
enum {
	BT_HID_PROTOCOL_BOOT_MODE = 0,
	BT_HID_PROTOCOL_REPORT_MODE,
};

/** @brief HID report types in get, set, data */
enum {
	BT_HID_REPORT_TYPE_OTHER = 0,
	BT_HID_REPORT_TYPE_INPUT,
	BT_HID_REPORT_TYPE_OUTPUT,
	BT_HID_REPORT_TYPE_FEATURE,
};

/** @brief HID seesion role */
enum bt_hid_session_role {
	BT_HID_SESSION_ROLE_CTRL,
	BT_HID_SESSION_ROLE_INTR
};

/** @brief HID role */
enum bt_hid_role {
	BT_HID_ROLE_ACCEPTOR,
	BT_HID_ROLE_INITIATOR
};

/** @brief HID session structure. */
struct bt_hid_session {
	struct bt_l2cap_br_chan br_chan;
	enum bt_hid_session_role role; /* As ctrl or intr */
};

/** @brief HID Device structure. */
struct bt_hid_device {
	struct bt_conn *conn;
	struct net_buf *buf;

	enum bt_hid_role role; /* As initial or accept */
	struct bt_hid_session ctrl_session;
	struct bt_hid_session intr_session;

	uint8_t state;
	uint8_t plugged;
};

/** @brief HID Device callbacks. */
struct bt_hid_device_cb {
	/**
	 * @brief HID Device Connected Callback
	 *
	 * The callback is called whenever an hid device connected.
	 *
	 *  @param hid hid device connection object.
	 *
	 */
	void (*connected)(struct bt_hid_device *hid);

	/**
	 * @brief HID Device Disconected Callback
	 *
	 * The callback is called whenever an hid device disconnected.
	 *
	 *  @param hid hid device connection object.
	 *
	 */
	void (*disconnected)(struct bt_hid_device *hid);

	/**
	 * @brief HID Device Set Report Callback
	 *
	 *  An hid device set report request from remote.
	 *
	 *  @param hid hid device connection object.
	 *  @param data value.
	 *  @param data len.
	 *
	 */
	void (*set_report)(struct bt_hid_device *hid, uint8_t *data, uint16_t len);

	/**
	 * @brief HID Device Get Report Callback
	 *
	 *  An hid device get report request from remote.
	 *
	 *  @param hid hid device connection object.
	 *  @param data value.
	 *  @param data len.
	 *
	 */
	void (*get_report)(struct bt_hid_device *hid, uint8_t *data, uint16_t len);

	/**
	 * @brief HID Device Set Protocol Callback
	 *
	 *  An hid device set protocol request from remote.
	 *
	 *  @param hid hid device connection object.
	 *  @param protocol protocol.
	 *
	 */
	void (*set_protocol)(struct bt_hid_device *hid, uint8_t protocol);

	/**
	 * @brief HID Device Get Protocol Callback
	 *
	 *  An hid device get protocol request from remote.
	 *
	 *  @param hid hid device connection object.
	 *
	 */
	void (*get_protocol)(struct bt_hid_device *hid);

	/**
	 * @brief HID Device Intr data Callback
	 *
	 *  An hid device intr data request from remote.
	 *
	 *  @param hid hid device connection object.
	 *  @param data value.
	 *  @param data len.
	 *
	 */
	void (*intr_data)(struct bt_hid_device *hid, uint8_t *data, uint16_t len);

	/**
	 * @brief HID Device Unplug Callback
	 *
	 *  An hid device unplug request from remote.
	 *
	 *  @param hid hid device connection object.
	 *
	 */
	void (*vc_unplug)(struct bt_hid_device *hid);
};

/** @brief HID Device Register Callback.
 *
 *  The cb is called when bt_hid_device is called or it is operated by remote device.
 *
 *  @param cb The callback function.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_hid_device_register(struct bt_hid_device_cb *cb);

/** @brief HID Device Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish HID
 *  Device connection between devices.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_hid_device in case of success or NULL in case
 *  of error.
 */
struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn);

/** @brief HID Device Disconnect
 *
 * This function close HID Device connection.
 *
 *  @param hid The bt_hid_device instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_hid_device_disconnect(struct bt_hid_device *hid);

/** @brief HID Device Send Ctrl Data.
 *
 *  Send hid data by ctrl channel.
 *
 *  @param hid The bt_hid_device instance.
 *  @param type HID report type.
 *  @param data send buffer.
 *  @param len buffer size.
 *
 *  @return size in case of success and error code in case of error.
 */
int bt_hid_device_send_ctrl_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len);

/** @brief HID Device Send Intr Data.
 *
 *  Send hid data by Intr channel.
 *
 *  @param hid The bt_hid_device instance.
 *  @param type HID report type.
 *  @param data send buffer.
 *  @param len buffer size.
 *
 *  @return size in case of success and error code in case of error.
 */
int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t type, uint8_t *data,
				 uint16_t len);

/** @brief HID Device Send Error Response.
 *
 *  Send hid error response by ctrl channel.
 *
 *  @param hid The bt_hid_device instance.
 *  @param error error code.
 *
 *  @return size in case of success and error code in case of error.
 */
int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HID_DEVICE_H_ */
