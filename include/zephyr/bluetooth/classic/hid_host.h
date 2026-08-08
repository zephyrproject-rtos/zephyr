/** @file
 *  @brief HID Host Protocol handling.
 */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HID_HOST_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HID_HOST_H_

/**
 * @brief Bluetooth HID Host
 * @defgroup bt_hid_host Bluetooth HID Host
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name HID protocol mode values
 *
 * Requested with bt_hid_host_set_protocol() and reported by the protocol_mode
 * callback. Defined in HID spec v1.1.2 Section 2.1.2, which makes Report
 * Protocol Mode the default.
 * @{
 */
/** Boot Protocol Mode (legacy). */
#define BT_HID_PROTOCOL_BOOT_MODE   0x00U
/** Report Protocol Mode (default). */
#define BT_HID_PROTOCOL_REPORT_MODE 0x01U
/** @} */

/**
 * @name HID report type values
 *
 * The report type carried by GET_REPORT, SET_REPORT and DATA transfers.
 * Defined in HID spec v1.1.2 Table 3.4; report type 0 is reserved.
 * @{
 */
/** Input report (device to host). */
#define BT_HID_REPORT_TYPE_INPUT   0x01U
/** Output report (host to device). */
#define BT_HID_REPORT_TYPE_OUTPUT  0x02U
/** Feature report. */
#define BT_HID_REPORT_TYPE_FEATURE 0x03U
/** @} */

/** @brief HID Host instance.
 *
 * Represents a single HID association with a remote HID Device. Instances are
 * allocated from a fixed pool and owned by the stack; applications only hold
 * the pointer handed out by bt_hid_host_connect() or the callbacks and must
 * not allocate or free one.
 */
struct bt_hid_host;

/** @brief Callbacks supplied by the HID Host application.
 *
 * Register via bt_hid_host_register() to receive events from
 * connected HID Devices.
 *
 * Only one control channel transaction may be outstanding at a time
 * (HID spec v1.1.2 Section 3.2.1); further requests return -EBUSY until the
 * response arrives. A transaction that is never answered by the device is
 * treated as a lost connection per HID spec v1.1.2 Section 5.2.6: the HID
 * association is torn down and the disconnected callback is invoked.
 */
struct bt_hid_host_cb {
	/** @brief HID connection established.
	 *
	 * Called when both control and interrupt L2CAP channels are open
	 * and the HID association is ready for traffic. This is called
	 * for both outgoing (Host-initiated) and incoming (Device-initiated
	 * reconnection) connections.
	 *
	 * @param hid HID Host instance.
	 */
	void (*connected)(struct bt_hid_host *hid);

	/** @brief HID connection terminated.
	 *
	 * Called when the HID association is fully torn down.
	 *
	 * @param hid HID Host instance.
	 */
	void (*disconnected)(struct bt_hid_host *hid);

	/** @brief Input report received on the interrupt channel.
	 *
	 * Asynchronous input report data sent by the HID Device.
	 *
	 * The payload is delivered as received. It starts with the Report ID when
	 * the report descriptor declares Report IDs or the device is in Boot
	 * Protocol Mode, which only the application can tell.
	 *
	 * @param hid HID Host instance.
	 * @param buf Report payload (ownership retained by stack).
	 */
	void (*input_report)(struct bt_hid_host *hid, struct net_buf *buf);

	/** @brief GET_REPORT response received.
	 *
	 * DATA message received on the control channel in response to a
	 * previously sent GET_REPORT request.
	 *
	 * The payload is delivered as received, starting with the Report ID when
	 * one is in use.
	 *
	 * @param hid  HID Host instance.
	 * @param type Report type (INPUT/OUTPUT/FEATURE).
	 * @param buf  Report payload (ownership retained by stack).
	 */
	void (*get_report_rsp)(struct bt_hid_host *hid, uint8_t type, struct net_buf *buf);

	/** @brief HANDSHAKE response received.
	 *
	 * Called for SET_REPORT, SET_PROTOCOL responses, or error
	 * responses to GET_REPORT/GET_PROTOCOL.
	 *
	 * @param hid    HID Host instance.
	 * @param result Handshake result code as defined in HID spec v1.1.2
	 *               Section 3.1.2.1 (0x00 is SUCCESSFUL).
	 */
	void (*handshake)(struct bt_hid_host *hid, uint8_t result);

	/** @brief GET_PROTOCOL response received.
	 *
	 * DATA message with protocol mode byte received on the control
	 * channel in response to a GET_PROTOCOL request.
	 *
	 * @param hid  HID Host instance.
	 * @param mode Protocol mode (BT_HID_PROTOCOL_BOOT_MODE or
	 *             BT_HID_PROTOCOL_REPORT_MODE).
	 */
	void (*protocol_mode)(struct bt_hid_host *hid, uint8_t mode);

	/** @brief Virtual Cable Unplug received from Device.
	 *
	 * The remote HID Device has sent a VIRTUAL_CABLE_UNPLUG control
	 * message. The stack disconnects the HID association. HID spec v1.1.2
	 * Section 3.1.2.2.3 also
	 * requires the bonding to be destroyed; this is left to the
	 * application because bt_br_unpair() would drop the whole ACL and
	 * with it any co-located profile.
	 *
	 * @param hid HID Host instance.
	 */
	void (*vc_unplug)(struct bt_hid_host *hid);
};

/** @brief Register HID Host callbacks.
 *
 * Registers the application callbacks and sets up L2CAP servers to
 * accept incoming Device-initiated reconnections on PSM 0x0011/0x0013.
 *
 * @param cb Callback structure. Must remain valid for the lifetime
 *           of the HID Host subsystem.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_register(const struct bt_hid_host_cb *cb);

/** @brief Unregister HID Host callbacks.
 *
 * Unregisters the L2CAP servers so incoming Device-initiated reconnections
 * are no longer accepted. All HID associations must be disconnected first.
 *
 * @return 0 on success, -EBUSY if an HID association is still active,
 *         negative errno on other failures.
 */
int bt_hid_host_unregister(void);

/** @brief Initiate an HID connection to a remote HID Device.
 *
 * Opens the L2CAP control channel and, once it is up, the interrupt channel.
 *
 * The HID service record is not read here. Applications that need the report
 * descriptor or the device attributes discover them over SDP themselves, which
 * also leaves them free to decide whether to do so on every connection.
 *
 * @param conn ACL connection to the remote device.
 * @param hid  Pointer to store the HID Host instance on success.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_connect(struct bt_conn *conn, struct bt_hid_host **hid);

/** @brief Get the ACL connection of an HID association.
 *
 * Needed by applications that have to act on the peer identity, for example to
 * destroy the bonding after a Virtual Cable Unplug (HID spec v1.1.2
 * Section 3.1.2.2.3), which the stack leaves to the application.
 *
 * @param hid HID Host instance.
 *
 * @return New reference to the ACL connection which must be released with
 *         bt_conn_unref(), or NULL if the association has no connection.
 */
struct bt_conn *bt_hid_host_get_conn(struct bt_hid_host *hid);

/** @brief Disconnect an HID association.
 *
 * Tears down the interrupt channel first, then the control channel
 * per HID spec v1.1.2 Section 5.2.2.
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_disconnect(struct bt_hid_host *hid);

/** @brief Send a GET_REPORT request to the HID Device.
 *
 * The response arrives asynchronously via the get_report_rsp callback
 * (on success) or the handshake callback (on error).
 *
 * The ReportID field is required as soon as the report descriptor declares
 * Report IDs, and in Boot Protocol Mode it is always required (HID spec v1.1.2
 * Section 3.1.2.3 and Section 3.3.1). Only the application knows the
 * descriptor, so it decides whether the field is sent.
 *
 * @param hid         HID Host instance.
 * @param type        Report type (BT_HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE).
 * @param report_id   Report ID to request, or NULL to omit the field.
 * @param buffer_size Maximum response payload size (0 = no limit).
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_get_report(struct bt_hid_host *hid, uint8_t type,
			   const uint8_t *report_id, uint16_t buffer_size);

/** @brief Send a SET_REPORT request to the HID Device.
 *
 * The response arrives via the handshake callback.
 * The buffer must start with the Report ID when the report descriptor declares
 * Report IDs or the device is in Boot Protocol Mode.
 *
 * @param hid  HID Host instance.
 * @param type Report type (BT_HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE).
 * @param buf  Buffer containing report payload, allocated with
 *             bt_hid_host_create_pdu(). Retained by the caller on failure.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_set_report(struct bt_hid_host *hid, uint8_t type,
			   struct net_buf *buf);

/** @brief Send a GET_PROTOCOL request to the HID Device.
 *
 * The response arrives via the protocol_mode callback (success) or
 * handshake callback (error).
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_get_protocol(struct bt_hid_host *hid);

/** @brief Send a SET_PROTOCOL request to the HID Device.
 *
 * The response arrives via the handshake callback.
 *
 * @param hid      HID Host instance.
 * @param protocol Target protocol (BT_HID_PROTOCOL_BOOT_MODE or
 *                 BT_HID_PROTOCOL_REPORT_MODE).
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_set_protocol(struct bt_hid_host *hid, uint8_t protocol);

/** @brief Send an output report on the interrupt channel.
 *
 * This is an asynchronous transfer with no acknowledgment.
 * The buffer must start with the Report ID when the report descriptor declares
 * Report IDs or the device is in Boot Protocol Mode.
 *
 * @param hid HID Host instance.
 * @param buf Buffer containing output report (consumed on success).
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_send_output_report(struct bt_hid_host *hid,
				   struct net_buf *buf);

/** @brief Send SUSPEND control to the HID Device.
 *
 * Informs the device that the host is entering a low-power state.
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_suspend(struct bt_hid_host *hid);

/** @brief Send EXIT_SUSPEND control to the HID Device.
 *
 * Informs the device that the host is exiting low-power state.
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_exit_suspend(struct bt_hid_host *hid);

/** @brief Send Virtual Cable Unplug to the HID Device.
 *
 * Sends a VCU control message and disconnects the HID association once the
 * message has been transmitted, per HID spec v1.1.2 Section 3.1.2.2.3.
 * Destroying the bonding is left to the application, see the vc_unplug
 * callback.
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_virtual_cable_unplug(struct bt_hid_host *hid);

/** @brief Allocate a PDU buffer for HID Host transmissions.
 *
 * @param pool Buffer pool to allocate from, or NULL for default.
 *
 * @return net_buf on success, NULL on failure.
 */
struct net_buf *bt_hid_host_create_pdu(struct net_buf_pool *pool);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HID_HOST_H_ */
