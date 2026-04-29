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

/**
 * @brief Bluetooth HID Device
 * @defgroup bt_hid_device Bluetooth HID Device
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HID header size (1 byte). */
#define BT_HID_HDR_SIZE 1U

/** @brief Maximum TX payload for HID PDUs (excludes HID header). */
#define BT_HID_TX_MTU (BT_L2CAP_TX_MTU - BT_HID_HDR_SIZE)

/** @brief Maximum RX payload for HID PDUs (excludes HID header). */
#define BT_HID_RX_MTU (BT_L2CAP_RX_MTU - BT_HID_HDR_SIZE)

/** @brief HID handshake response codes
 *
 * These values are used in HID handshake messages exchanged over the
 * control channel. They fit in 4 bits and follow the Bluetooth HID
 * specification semantics.
 */
enum __packed bt_hid_handshake_rsp {
	/**
	 * HID handshake response success (0) - request processed successfully.
	 */
	BT_HID_HANDSHAKE_RSP_SUCCESS = 0x00,
	/**
	 * HID handshake response indicating the device is not ready / not
	 * initialized to service the request. Developers: use this when the
	 * device's internal state prevents processing.
	 */
	BT_HID_HANDSHAKE_RSP_NOT_READY = 0x01,
	/** Invalid report ID specified in the request. */
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID = 0x02,
	/** Request type/operation is unsupported by the device. */
	BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ = 0x03,
	/** Request contained invalid parameters (length, format etc.). */
	BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM = 0x04,
	/**
	 * Unknown error (explicitly assigned value 14). Keep this aligned
	 * with the HID spec if a specific meaning is required later.
	 */
	BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN = 0x0e,
	/** Fatal/unrecoverable error on the device side. */
	BT_HID_HANDSHAKE_RSP_ERR_FATAL = 0x0f,
};

/** @brief HID protocol parameters for Set_Protocol
 *
 * These values indicate which HID protocol the host requests the device
 * to use. BOOT_MODE is the legacy boot protocol; REPORT_MODE indicates
 * normal report protocol operation.
 */
enum __packed bt_hid_protocol {
	/** Boot protocol mode (legacy). */
	BT_HID_PROTOCOL_BOOT_MODE = 0x00,
	/** Report protocol mode (default). */
	BT_HID_PROTOCOL_REPORT_MODE,
};

/** @brief HID report types used for Get/Set/Data operations
 *
 * These map to report type fields in HID messages. INPUT reports are
 * typically device->host, OUTPUT are host->device, and FEATURE are
 * device-specific feature reports.
 */
enum __packed bt_hid_report_type {
	/** Report type not specified (0) - used for `Set Protocol` request. */
	BT_HID_REPORT_TYPE_OTHER = 0x00,
	/** Report type for input reports (1) - device->host. */
	BT_HID_REPORT_TYPE_INPUT = 0x01,
	/** Report type for output reports (2) - host->device. */
	BT_HID_REPORT_TYPE_OUTPUT = 0x02,
	/** Report type for feature reports (3) - device-specific. */
	BT_HID_REPORT_TYPE_FEATURE = 0x03,
};

/** @brief Type of the HID channel
 *
 * Enumeration defining which HID channel the session represents.
 */
enum bt_hid_channel_type {
	/** Channel type unknown or not initialized. */
	BT_HID_CHANNEL_UNKNOWN = 0,
	/** Control channel. */
	BT_HID_CHANNEL_CTRL,
	/** Interrupt channel. */
	BT_HID_CHANNEL_INTR,
};

/** @brief Role of the HID device relative to the remote peer
 *
 * Enumeration defining whether the local device accepted or initiated
 * the HID association.
 */
enum bt_hid_role {
	/** Local device accepted an incoming connection (server role). */
	BT_HID_ROLE_ACCEPTOR = 0,
	/** Local device initiated the connection (client role). */
	BT_HID_ROLE_INITIATOR
};

/** @brief HID session wrapper for an L2CAP channel
 *
 * Each HID device maintains two sessions: control and interrupt. The
 * `br_chan` holds the underlying BR/EDR L2CAP channel context and the
 * `role` identifies which session this structure represents.
 */
struct bt_hid_session {
	/** Underlying BR/EDR L2CAP channel. */
	struct bt_l2cap_br_chan br_chan;
	/** Channel type (control or interrupt). */
	enum bt_hid_channel_type type;
};

/** @brief HID device instance
 *
 * Represents a single HID association with a remote host.
 *
 * - conn: underlying Bluetooth connection.
 * - role: whether this side initiated or accepted the connection.
 * - ctrl_session / intr_session: control and interrupt L2CAP sessions.
 * - boot_mode: true when in Boot Protocol Mode.
 * - state: runtime state (connect/disconnect state machine).
 *
 * Instances are allocated from a fixed array (see implementation) and
 * should be treated as non-moving; callers must not free this object.
 */
struct bt_hid_device {
	/** Underlying Bluetooth connection. */
	struct bt_conn *conn;
	/** Role of this device (initiator or acceptor). */
	enum bt_hid_role role;
	/** Control channel session. */
	struct bt_hid_session ctrl_session;
	/** Interrupt channel session. */
	struct bt_hid_session intr_session;

	/** True when the HID device is in Boot Protocol Mode. */
	bool boot_mode;
	/** True when the HID host has sent SUSPEND. */
	bool suspended;
	/** True when report descriptor declares Report IDs. */
	bool has_report_id;
	/** True when the device supports Boot Protocol (HIDBootDevice). */
	bool boot_device;
	/** Runtime connection state. */
	uint8_t state;

	/** @cond INTERNAL_HIDDEN */
	struct k_work_delayable intr_timeout;
	/** @endcond */
};

/** @brief Callbacks supplied by the HID application
 *
 * Applications should register one instance of this callback table via
 * `bt_hid_device_register()` to receive events and requests from the
 * HID host. Only the callbacks the application needs must be provided;
 * unimplemented callbacks should be set to NULL.
 */
struct bt_hid_device_cb {
	/** @brief connected callback to application
	 *
	 * If this callback is provided it will be called whenever the HID
	 * association completes. Both control and interrupt channels are
	 * established and the device is ready for HID traffic.
	 *
	 * @param hid HID device object.
	 */
	void (*connected)(struct bt_hid_device *hid);

	/** @brief disconnected callback to application
	 *
	 * If this callback is provided it will be called whenever the HID
	 * association gets disconnected, including rejected or cancelled
	 * connections and errors during setup. The HID device object remains
	 * valid until this callback returns.
	 *
	 * @param hid HID device object.
	 */
	void (*disconnected)(struct bt_hid_device *hid);

	/** @brief Set_Report request callback
	 *
	 * This callback provides report data sent by the HID host.
	 * Return 0 to accept the report. Return a negative errno to trigger
	 * a corresponding handshake error response.
	 *
	 * @param hid HID device object.
	 * @param type Report type (see BT_HID_REPORT_TYPE_*).
	 * @param report_id Report ID (0 if descriptor has no Report IDs).
	 * @param buf Report payload buffer (ownership retained by stack).
	 *
	 * @return 0 on success or a negative errno on failure.
	 */
	int (*set_report)(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			  struct net_buf *buf);

	/** @brief Get_Report request callback
	 *
	 * The application should respond by sending a DATA message with a
	 * report payload that does not exceed `buffer_size`.
	 * Return 0 if the response will be sent. Return a negative errno to
	 * trigger a handshake error response.
	 *
	 * @param hid HID device object.
	 * @param type Report type (see BT_HID_REPORT_TYPE_*).
	 * @param report_id Requested report ID (0 if unused).
	 * @param buffer_size Maximum report payload size expected by host.
	 *
	 * @return 0 on success or a negative errno on failure.
	 */
	int (*get_report)(struct bt_hid_device *hid, uint8_t type, uint8_t report_id,
			  uint16_t buffer_size);

	/** @brief Set_Protocol request callback
	 *
	 * Return 0 to accept the requested protocol. Return a negative errno
	 * to trigger a corresponding handshake error response.
	 *
	 * @param hid HID device object.
	 * @param protocol Requested protocol (boot/report).
	 *
	 * @return 0 on success or a negative errno on failure.
	 */
	int (*set_protocol)(struct bt_hid_device *hid, uint8_t protocol);

	/** @brief Get_Protocol request callback
	 *
	 * The application should respond with the current protocol using a
	 * DATA message.
	 * Return 0 if the response will be sent. Return a negative errno to
	 * trigger a handshake error response.
	 *
	 * @param hid HID device object.
	 *
	 * @return 0 on success or a negative errno on failure.
	 */
	int (*get_protocol)(struct bt_hid_device *hid);

	/** @brief Interrupt channel data callback
	 *
	 * This callback provides report data received on the interrupt channel.
	 *
	 * @param hid HID device object.
	 * @param report_id Report ID.
	 * @param buf Report payload buffer (ownership retained by stack).
	 */
	void (*intr_data)(struct bt_hid_device *hid, uint8_t report_id, struct net_buf *buf);

	/** @brief Virtual Cable Unplug notification callback
	 *
	 * @param hid HID device object.
	 */
	void (*vc_unplug)(struct bt_hid_device *hid);

	/** @brief Suspend/Exit-Suspend notification callback
	 *
	 * Called when the HID host sends SUSPEND or EXIT_SUSPEND control.
	 *
	 * @param hid HID device object.
	 * @param suspended true if entering suspend, false if exiting.
	 */
	void (*suspend)(struct bt_hid_device *hid, bool suspended);
};

/** @brief Allocate/create a PDU buffer suitable for HID over L2CAP.
 *
 * If @p pool is NULL a default pool may be used by the implementation.
 *
 * @note The returned buffer may be fragmented. Use net_buf_frag_add()
 *       to append data.
 *
 * @param pool Optional net_buf pool to allocate from.
 *
 * @return A new net_buf instance or NULL on failure.
 */
struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool);

/** @brief Register HID device callbacks.
 *
 * The callback pointer must remain valid for the lifetime of the HID
 * subsystem or until another registration replaces it.
 *
 * @param cb Callbacks to register.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_register(const struct bt_hid_device_cb *cb);

/** @brief Set whether the report descriptor uses Report IDs.
 *
 * When true, GET_REPORT, SET_REPORT and DATA messages include a
 * Report ID byte per HID spec v1.1.2 Sections 3.1.2.4 and 3.1.2.8.
 * Defaults to false. Should be called after connection is established.
 *
 * @param hid    HID device instance.
 * @param has_id true if the report descriptor declares Report IDs.
 */
void bt_hid_device_set_report_id(struct bt_hid_device *hid, bool has_id);

/** @brief Set whether the device supports Boot Protocol.
 *
 * When false, GET_PROTOCOL and SET_PROTOCOL requests are rejected
 * with ERR_UNSUPPORTED_REQ per HID spec v1.1.2 Section 3.1.2.6.
 * Defaults to false.
 *
 * @param hid         HID device instance.
 * @param boot_device true if the device supports Boot Protocol.
 */
void bt_hid_device_set_boot_device(struct bt_hid_device *hid, bool boot_device);

/** @brief Unregister HID device callbacks and disable HID services.
 *
 * This clears the callbacks, disconnects any active HID session, and
 * unregisters the L2CAP servers.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_unregister(void);

/** @brief Initiate an outgoing HID association on the given connection.
 *
 * On success the bt_hid_device instance is stored in @p hid. The caller
 * must not free this object.
 *
 * @param conn Bluetooth connection to use for the association.
 * @param hid  Pointer to store the HID device instance on success.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_connect(struct bt_conn *conn, struct bt_hid_device **hid);

/** @brief Disconnect a previously connected HID association.
 *
 * The disconnect is asynchronous. Callers should not free the @p hid
 * instance until the disconnected callback is invoked.
 *
 * @param hid HID device instance to disconnect.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_disconnect(struct bt_hid_device *hid);

/** @brief Send a Get_Report response on the control channel.
 *
 * The buffer should contain the full report payload including the
 * Report ID byte if the descriptor declares Report IDs
 * (HID spec v1.1.2 Section 3.1.2.4).
 *
 * @note The buffer must be allocated with bt_hid_device_create_pdu()
 * to ensure sufficient headroom for the HID and L2CAP headers.
 *
 * @param hid HID device instance to send data on.
 * @param type Report type used in the DATA header param field
 *             (BT_HID_REPORT_TYPE_INPUT, _OUTPUT, or _FEATURE).
 * @param buf Buffer containing the report payload (consumed on success).
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_get_report_response(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf);

/** @brief Send a Get_Protocol response on the control channel.
 *
 * This sends a DATA message with the protocol byte as payload.
 *
 * @param hid HID device instance to send data on.
 * @param protocol Protocol value to report (boot/report).
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_get_protocol_response(struct bt_hid_device *hid, uint8_t protocol);

/** @brief Send a HID interrupt input report to the HID host.
 *
 * The buffer should contain the full report payload including the
 * Report ID byte if the descriptor declares Report IDs
 * (HID spec v1.1.2 Section 3.1.2.8).
 *
 * The HID device can transmit interrupt channel messages to the host
 * at any time.
 *
 * @note The buffer must be allocated with bt_hid_device_create_pdu()
 * to ensure sufficient headroom for the HID and L2CAP headers.
 *
 * @param hid HID device instance to send data on.
 * @param buf Buffer containing the report payload (consumed on success).
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_send(struct bt_hid_device *hid, struct net_buf *buf);

/** @brief Report an error back to the host using a HID handshake response code.
 *
 * @param hid HID device instance to send data on.
 * @param error Error code to report (see BT_HID_HANDSHAKE_RSP_*).
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_report_error(struct bt_hid_device *hid, uint8_t error);

/** @brief Trigger a virtual cable unplug procedure for the given HID association.
 *
 * This will request the remote host to treat the device as unplugged.
 *
 * @param hid HID device instance to unplug.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid);

/** @brief Initiate a reconnection to a previously bonded HID host.
 *
 * The HID device initiates L2CAP connections for the control and
 * interrupt channels on an existing ACL connection.
 *
 * @note This currently re-establishes L2CAP channels only. Full
 * HIDReconnectInitiate per HID spec v1.1.2 Appendix A.2 (SDP
 * attribute validation, role switch, reconnect timeout) is not
 * yet implemented.
 *
 * @param conn Bluetooth connection to the host.
 * @param hid  Pointer to store the HID device instance on success.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_reconnect(struct bt_conn *conn, struct bt_hid_device **hid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HID_DEVICE_H_ */
