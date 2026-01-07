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

/** @brief Maximum MTU size for HID Device
 *
 * This defines the maximum data payload expected for HID PDUs on both
 * control and interrupt channels. Implementations SHOULD respect this
 * limit when allocating buffers for HID data exchange.
 */
#define BT_HID_MAX_MTU 64

/** @brief HID handshake response codes
 *
 * These values are used in HID handshake messages exchanged over the
 * control channel. They are small integer codes (fit in 4 bits when
 * encoded in HID headers) and follow the Bluetooth HID specification
 * semantics.
 */
enum __packed {
	/**
	 * HID handshake response success (0) - request processed successfully.
	 */
	BT_HID_HANDSHAKE_RSP_SUCCESS = 0,
	/**
	 * HID handshake response indicating the device is not ready / not
	 * initialized to service the request. Developers: use this when the
	 * device's internal state prevents processing.
	 */
	BT_HID_HANDSHAKE_RSP_NOT_READY,
	/** Invalid report ID specified in the request. */
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID,
	/** Request type/operation is unsupported by the device. */
	BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ,
	/** Request contained invalid parameters (length, format etc.). */
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM,
	/**
	 * Unknown error (explicitly assigned value 14). Keep this aligned
	 * with the HID spec if a specific meaning is required later.
	 */
	BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN = 14,
	/** Fatal/unrecoverable error on the device side. */
	BT_HID_HANDSHAKE_RSP_ERR_FATAL,
};

/** @brief HID protocol parameters for `Set Protocol` request
 *
 * These values indicate which HID protocol the host requests the device
 * to use. `BOOT_MODE` is the legacy HID boot protocol; `REPORT_MODE`
 * indicates normal report protocol operation.
 */
enum __packed {
	BT_HID_PROTOCOL_BOOT_MODE = 0,
	BT_HID_PROTOCOL_REPORT_MODE,
};

/** @brief HID report types used for Get/Set/Data operations
 *
 * These map to report type fields in HID messages. `INPUT` reports are
 * typically device->host, `OUTPUT` are host->device, and `FEATURE`
 * represents device-specific feature reports.
 */
enum __packed {
	/** Report type not specified (0) - used for `Set Protocol` request. */
	BT_HID_REPORT_TYPE_OTHER = 0,
	/** Report type for input reports (1) - device->host. */
	BT_HID_REPORT_TYPE_INPUT,
	/** Report type for output reports (2) - host->device. */
	BT_HID_REPORT_TYPE_OUTPUT,
	/** Report type for feature reports (3) - device-specific. */
	BT_HID_REPORT_TYPE_FEATURE,
};

/** @brief Role of the HID session relative to the remote peer
 *
 * `BT_HID_SESSION_ROLE_CTRL` indicates the local device is the control
 * channel. `BT_HID_SESSION_ROLE_INTR` indicates the local device is the
 * interrupt channel.
 */
enum bt_hid_session_role {
	BT_HID_SESSION_ROLE_UNKNOWN,
	BT_HID_SESSION_ROLE_CTRL,
	BT_HID_SESSION_ROLE_INTR,
};

/** @brief Role of the HID device relative to the remote peer
 *
 * `BT_HID_ROLE_ACCEPTOR` indicates the local device accepted an incoming
 * connection (server role). `BT_HID_ROLE_INITIATOR` indicates the local
 * device initiated the connection (client role).
 */
enum bt_hid_role {
	BT_HID_ROLE_ACCEPTOR,
	BT_HID_ROLE_INITIATOR
};

/** @brief HID session wrapper for an L2CAP channel
 *
 * Each HID device maintains two sessions: control and interrupt. The
 * `br_chan` holds the underlying BR/EDR L2CAP channel context and the
 * `role` identifies which session this structure represents.
 */
struct bt_hid_session {
	struct bt_l2cap_br_chan br_chan;
	enum bt_hid_session_role role;
};

/** @brief HID device instance
 *
 * Represents a single HID association with a remote host. Fields:
 * - `conn`: reference to the underlying Bluetooth connection.
 * - `role`: whether this side initiated or accepted the connection.
 * - `ctrl_session` / `intr_session`: control and interrupt L2CAP sessions.
 * - `state`: runtime state (connect/disconnect state machine).
 *
 * Instances are allocated from a fixed array (see implementation) and
 * should be treated as non-moving; callers should not free this
 * structure directly.
 */
struct bt_hid_device {
	struct bt_conn *conn;
	enum bt_hid_role role;
	struct bt_hid_session ctrl_session;
	struct bt_hid_session intr_session;

	uint8_t state;
};

/** @brief Callbacks supplied by the HID application
 *
 * Applications should register one instance of this callback table via
 * `bt_hid_device_register()` to receive events and requests from the
 * HID host. Only the callbacks the application needs must be provided;
 * unimplemented callbacks should be set to NULL.
 */
struct bt_hid_device_cb {
	/** Called when the HID association is fully connected. */
	void (*connected)(struct bt_hid_device *hid);

	/** Called when the HID association is disconnected (clean teardown). */
	void (*disconnected)(struct bt_hid_device *hid);

	/**
	 * Called when the HID host sends a Set_Report request to the device.
	 * - `type` is one of `BT_HID_REPORT_TYPE_*`.
	 * - `buf` contains the report payload; ownership remains with the
	 *   implementation unless documented otherwise.
	 */
	void (*set_report)(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf);

	/**
	 * Called when the HID host sends a Get_Report request to the device.
	 * - `type` is one of `BT_HID_REPORT_TYPE_*`.
	 * - `report_id` is the requested report ID (0 if unused).
	 * - `buffer_size` indicates the maximum buffer the host expects.
	 */
	void (*get_report)(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			   uint16_t buffer_size);

	/**
	 * Set the HID protocol (boot/report).
	 * - `protocol` is one of `BT_HID_PROTOCOL_BOOT_MODE` or `BT_HID_PROTOCOL_REPORT_MODE`.
	 */
	void (*set_protocol)(struct bt_hid_device *hid, uint8_t protocol);

	/** Query the current HID protocol. */
	void (*get_protocol)(struct bt_hid_device *hid);

	/**
	 * Incoming interrupt (input) data received from the HID host.
	 * - `type` is one of `BT_HID_REPORT_TYPE_*`.
	 * - `buf` contains the report payload; ownership remains with the
	 *   implementation unless documented otherwise.
	 * */
	void (*intr_data)(struct bt_hid_device *hid, int8_t type, struct net_buf *buf);

	/** Virtual Cable Unplug notification received from the HID host. */
	void (*vc_unplug)(struct bt_hid_device *hid);
};

/**
 * Allocate/create a PDU buffer suitable for HID over L2CAP. If `pool` is
 * NULL a default pool may be used by the implementation.
 *
 * @note
 * The returned buffer is not guaranteed to be a single fragment; the
 * implementation may split it into multiple fragments. Callers should
 * use `net_buf_frag_add()` to add data to the buffer.
 *
 * @param pool Optional net_buf pool to allocate from.
 *
 * @return A new net_buf instance or NULL on failure.
 */
struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool);

/**
 * Register HID device callbacks. The callback pointer must remain valid
 * for the lifetime of the HID subsystem or until another registration
 * replaces it.
 *
 * @note unregister cloud be done by passing NULL as the callback pointer.
 *
 * @param cb Callbacks to register.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_register(const struct bt_hid_device_cb *cb);

/**
 * Initiate an outgoing HID association on the given Bluetooth
 * connection. Returns a `bt_hid_device` instance on success (caller
 * must not free it). The returned instance represents the session and
 * is used for subsequent operations.
 *
 * @note
 * The connection must be in the connected state and have a valid
 * Bluetooth address.
 *
 * @param conn Bluetooth connection to use for the association.
 *
 * @return A new bt_hid_device instance or NULL on failure.
 */
struct bt_hid_device *bt_hid_device_connect(struct bt_conn *conn);

/**
 * Disconnect a previously connected HID association. Returns 0 on
 * success or a negative errno on failure.
 *
 * @note
 * The connection must be in the connected state and have a valid
 * Bluetooth address.
 *
 * The HID association will be disconnected asynchronously. Callers
 * should not free the `hid` instance until the `disconnected` callback
 * is invoked.
 *
 * @param hid HID device instance to disconnect.
 *
 * @return 0 on success or a negative errno on failure.
 *
 */
int bt_hid_device_disconnect(struct bt_hid_device *hid);

/**
 * Send HID data on the control channel. `param` encodes report type or
 * control parameters per HID spec. `buf` should contain the payload and
 * will be consumed by the send operation on success.
 *
 * @param hid HID device instance to send data on.
 * @param param Report type or control parameter (see HID spec).
 * @param buf Buffer containing the report payload.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_send_ctrl_data(struct bt_hid_device *hid, uint8_t param, struct net_buf *buf);

/**
 * Send HID data on the interrupt channel. Parameters mirror
 * `bt_hid_device_send_ctrl_data` but the data is sent over the
 * interrupt L2CAP channel.
 *
 * @param hid HID device instance to send data on.
 * @param param Report type or control parameter (see HID spec).
 * @param buf Buffer containing the report payload.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_send_intr_data(struct bt_hid_device *hid, uint8_t param, struct net_buf *buf);

/**
 * Report an error back to the host using a HID handshake response code
 * (see `BT_HID_HANDSHAKE_RSP_*`).
 *
 * @param hid HID device instance to send data on.
 * @param error Error code to report (see `BT_HID_HANDSHAKE_RSP_*`).
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error);

/**
 * Trigger a virtual cable unplug procedure for the given HID
 * association. This will request the remote host to treat the device
 * as unplugged.
 *
 * @param hid HID device instance to unplug.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HID_DEVICE_H_ */
