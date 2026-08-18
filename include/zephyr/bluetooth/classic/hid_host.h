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
 * Requested with bt_hid_host_set_protocol() and reported by the get_protocol
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

/**
 * @name HID HANDSHAKE result codes
 *
 * Result of a request sent on the control channel, reported by the result
 * callbacks. Defined in HID spec v1.1.2 Section 3.1.2.1.
 *
 * These must stay identical to the copy in hid_internal.h, which the stack
 * uses internally.
 * @{
 */
/** The request completed successfully. */
#define BT_HID_HS_RSP_SUCCESS               0x00
/** The device is too busy to accept the request; the host may retry. */
#define BT_HID_HS_RSP_NOT_READY             0x01
/** The request referenced an unsupported Report ID. */
#define BT_HID_HS_RSP_ERR_INVALID_REPORT_ID 0x02
/** The device does not support the request. */
#define BT_HID_HS_RSP_ERR_UNSUPPORTED_REQ   0x03
/** The request carried an invalid parameter. */
#define BT_HID_HS_RSP_ERR_INVALID_PARAM     0x04
/** The device could not identify the error condition. */
#define BT_HID_HS_RSP_ERR_UNKNOWN           0x0e
/** The device encountered a fatal error and needs to be restarted. */
#define BT_HID_HS_RSP_ERR_FATAL             0x0f
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
 * response arrives. Each request has its own result callback, which reports the
 * HANDSHAKE result code supplied by the device and is therefore only invoked
 * when the device answers. A transaction that is never answered is treated as a
 * lost connection per HID spec v1.1.2 Section 5.2.6: the HID association is
 * torn down and the disconnected callback is invoked instead. Since at most one
 * transaction is outstanding, an application that sees disconnected after
 * issuing a request knows that request was lost.
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

	/** @brief GET_REPORT result.
	 *
	 * Called for every GET_REPORT transaction, whether the device answered
	 * with a DATA message or rejected the request with a HANDSHAKE (HID spec
	 * v1.1.2 Section 3.2.1.1), so the outcome is handled in one place.
	 *
	 * The payload is delivered as received, starting with the Report ID when
	 * one is in use.
	 *
	 * @param hid         HID Host instance.
	 * @param result_code BT_HID_HS_RSP_SUCCESS, or the BT_HID_HS_RSP_ERR_*
	 *                    code the device replied with.
	 * @param type        Report type (INPUT/OUTPUT/FEATURE), undefined when
	 *                    @p result_code is not BT_HID_HS_RSP_SUCCESS.
	 * @param buf         Report payload (ownership retained by stack), NULL
	 *                    when @p result_code is not BT_HID_HS_RSP_SUCCESS.
	 */
	void (*get_report)(struct bt_hid_host *hid, uint8_t result_code, uint8_t type,
			   struct net_buf *buf);

	/** @brief SET_REPORT result.
	 *
	 * Called when the device answers a SET_REPORT request with a HANDSHAKE,
	 * which is the only reply a SET request gets (HID spec v1.1.2
	 * Section 3.2.1.2).
	 *
	 * @param hid         HID Host instance.
	 * @param result_code BT_HID_HS_RSP_SUCCESS, or the BT_HID_HS_RSP_ERR_*
	 *                    code the device replied with.
	 */
	void (*set_report)(struct bt_hid_host *hid, uint8_t result_code);

	/** @brief GET_PROTOCOL result.
	 *
	 * Called for every GET_PROTOCOL transaction, whether the device answered
	 * with a DATA message carrying the protocol mode octet or rejected the
	 * request with a HANDSHAKE (HID spec v1.1.2 Section 3.2.1.5).
	 *
	 * @param hid         HID Host instance.
	 * @param result_code BT_HID_HS_RSP_SUCCESS, or the BT_HID_HS_RSP_ERR_*
	 *                    code the device replied with.
	 * @param protocol    Protocol mode of the device (BT_HID_PROTOCOL_BOOT_MODE
	 *                    or BT_HID_PROTOCOL_REPORT_MODE), undefined when
	 *                    @p result_code is not BT_HID_HS_RSP_SUCCESS.
	 */
	void (*get_protocol)(struct bt_hid_host *hid, uint8_t result_code, uint8_t protocol);

	/** @brief SET_PROTOCOL result.
	 *
	 * Called when the device answers a SET_PROTOCOL request with a HANDSHAKE,
	 * which is the only reply a SET request gets (HID spec v1.1.2
	 * Section 3.2.1.6). The requested mode is in use only when
	 * @p result_code is BT_HID_HS_RSP_SUCCESS.
	 *
	 * @param hid         HID Host instance.
	 * @param result_code BT_HID_HS_RSP_SUCCESS, or the BT_HID_HS_RSP_ERR_*
	 *                    code the device replied with.
	 */
	void (*set_protocol)(struct bt_hid_host *hid, uint8_t result_code);

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
 *         bt_conn_unref(), or NULL if the association is no longer established.
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
 * The response arrives asynchronously via the get_report callback, or the
 * association is torn down if the device never answers.
 *
 * The ReportID field is required as soon as the report descriptor declares
 * Report IDs, and in Boot Protocol Mode it is always required (HID spec v1.1.2
 * Section 3.1.2.3 and Section 3.3.1). Only the application knows the
 * descriptor, so it decides whether the field is sent.
 *
 * @param hid         HID Host instance.
 * @param type        Report type (BT_HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE).
 * @param report_id   Report ID to request, or 0 to omit the field. Report ID 0
 *                    is reserved by the USB HID specification, so it cannot
 *                    identify a report.
 * @param buffer_size Maximum response payload size (0 = no limit). Must not
 *                    exceed what the control channel can receive.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_get_report(struct bt_hid_host *hid, uint8_t type, uint8_t report_id,
			   uint16_t buffer_size);

/** @brief Send a SET_REPORT request to the HID Device.
 *
 * The response arrives via the set_report callback.
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
 * The response arrives via the get_protocol callback.
 *
 * @param hid HID Host instance.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_get_protocol(struct bt_hid_host *hid);

/** @brief Send a SET_PROTOCOL request to the HID Device.
 *
 * The response arrives via the set_protocol callback.
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
 * This is an asynchronous transfer with no acknowledgment: HID spec v1.1.2
 * Section 3.1.2.9 leaves interrupt channel transfers unanswered.
 * The buffer must start with the Report ID when the report descriptor declares
 * Report IDs or the device is in Boot Protocol Mode.
 *
 * @param hid HID Host instance.
 * @param buf Buffer containing output report, allocated with
 *            bt_hid_host_create_pdu() so the HIDP header fits (consumed on
 *            success).
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_output_report(struct bt_hid_host *hid, struct net_buf *buf);

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
