/** @file
 *  @brief HID Device Protocol handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_DEVICE_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_DEVICE_H_

/**
 * @brief Bluetooth HID Device
 * @defgroup bt_hid_device Bluetooth HID Device
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

/** @name HID protocol mode values
 *
 * These values indicate which HID protocol the host requests the device
 * to use. BOOT_MODE is the legacy boot protocol; REPORT_MODE indicates
 * normal report protocol operation.
 * @{
 */
/** Boot protocol mode (legacy). */
#define BT_HID_PROTOCOL_BOOT_MODE   0x00
/** Report protocol mode (default). */
#define BT_HID_PROTOCOL_REPORT_MODE 0x01
/** @} */

/** @name HID report types used for Get/Set/Data operations
 *
 * These map to report type fields in HID messages. INPUT reports are
 * typically device->host, OUTPUT are host->device, and FEATURE are
 * device-specific feature reports.
 * @{
 */
/** Report type for input reports (1) - device->host. */
#define BT_HID_REPORT_TYPE_INPUT   0x01
/** Report type for output reports (2) - host->device. */
#define BT_HID_REPORT_TYPE_OUTPUT  0x02
/** Report type for feature reports (3) - device-specific. */
#define BT_HID_REPORT_TYPE_FEATURE 0x03
/** @} */

struct bt_hid_device;

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
	 * This callback provides report data sent by the HID host. This
	 * callback is mandatory (SET_REPORT must be supported per HID spec
	 * v1.1.2) and must be provided at registration.
	 *
	 * The buffer contains the raw Report Data Payload as received,
	 * including the Report ID byte if declared in the descriptor.
	 * Return 0 to accept the report. Return a negative errno to trigger
	 * a corresponding handshake error response.
	 *
	 * @param hid HID device object.
	 * @param type Report type (see BT_HID_REPORT_TYPE_*).
	 * @param buf Report payload buffer (ownership retained by stack).
	 *
	 * @return 0 on success or a negative errno on failure.
	 */
	int (*set_report)(struct bt_hid_device *hid, uint8_t type, struct net_buf *buf);

	/** @brief Get_Report request callback
	 *
	 * Called synchronously when the host sends a GET_REPORT request. This
	 * callback is mandatory (GET_REPORT must be supported per HID spec
	 * v1.1.2) and must be provided at registration.
	 *
	 * The application responds from within the callback by appending the
	 * report payload to @p rsp (including the Report ID byte if the report
	 * descriptor declares Report IDs) and returning 0. The stack then
	 * prepends the HIDP DATA header and sends the response.
	 *
	 * If no data is available yet, return -EAGAIN; the stack replies with a
	 * NOT_READY handshake and the host may retry (HID spec v1.1.2 Section
	 * 3.2.1.1). Any other negative errno triggers the matching handshake
	 * error response.
	 *
	 * @p req contains the remaining GET_REPORT parameters after the HIDP
	 * header: an optional Report ID (present per the report descriptor)
	 * followed, when @p size_present is true, by a 2-byte little-endian
	 * BufferSize (the maximum response length the host expects). When
	 * @p size_present is false no BufferSize is present.
	 *
	 * @param hid HID device object.
	 * @param type Report type (see BT_HID_REPORT_TYPE_*).
	 * @param size_present true if the request's Size bit is set, meaning a
	 *                     2-byte BufferSize follows the optional Report ID
	 *                     in @p req.
	 * @param req Remaining request parameters (ownership retained by stack).
	 * @param rsp Response buffer to fill with the report payload (ownership
	 *            retained by stack; sent by the stack on a 0 return).
	 *
	 * @return 0 if @p rsp was filled with the response, -EAGAIN to reply
	 *         with NOT_READY, or another negative errno for a handshake
	 *         error.
	 */
	int (*get_report)(struct bt_hid_device *hid, uint8_t type, bool size_present,
			  struct net_buf *req, struct net_buf *rsp);

	/** @brief Set_Protocol request callback
	 *
	 * Called when the host sends a SET_PROTOCOL request. The application
	 * should return 0 if the protocol mode is supported, or a negative
	 * errno if it is not. The driver updates the internal protocol mode
	 * flag and sends the appropriate HANDSHAKE response based on the
	 * return value.
	 *
	 * @param hid HID device object.
	 * @param protocol Requested protocol mode (BT_HID_PROTOCOL_BOOT_MODE
	 *                 or BT_HID_PROTOCOL_REPORT_MODE).
	 *
	 * @return 0 if the protocol mode is accepted, negative errno otherwise.
	 */
	int (*set_protocol)(struct bt_hid_device *hid, uint8_t protocol);

	/** @brief Output report callback
	 *
	 * This callback provides output report data received on the
	 * interrupt channel from the HID host. The buffer contains
	 * the raw report payload including Report ID if declared in
	 * the descriptor.
	 *
	 * @param hid HID device object.
	 * @param buf Report payload buffer (ownership retained by stack).
	 */
	void (*output_report)(struct bt_hid_device *hid, struct net_buf *buf);

	/** @brief Virtual Cable Unplug notification callback
	 *
	 * Invoked when the HID host sends a VIRTUAL_CABLE_UNPLUG HID_CONTROL
	 * message. Per HID spec v1.1.2 Section 3.1.2.2.3, the recipient shall
	 * destroy all bonding and Virtual Cable information stored for the peer.
	 *
	 * The stack tears down the INTR and CTRL channels on the application's
	 * behalf, but does not clear bonding: doing so requires dropping the
	 * link key (and conditionally the ACL), which may affect other profiles
	 * sharing the same ACL. The application is therefore responsible for
	 * clearing the bonding information when appropriate, for example:
	 *
	 * @code
	 * struct bt_conn *conn = bt_hid_device_get_conn(hid);
	 *
	 * if (conn != NULL) {
	 *         bt_br_unpair(bt_conn_get_dst_br(conn));
	 *         bt_conn_unref(conn);
	 * }
	 * @endcode
	 *
	 * @param hid HID device object.
	 */
	void (*vc_unplug)(struct bt_hid_device *hid);

	/** @brief Suspend/Exit-Suspend notification callback
	 *
	 * SUSPEND and EXIT_SUSPEND are unidirectional HID_CONTROL messages
	 * owned by the HID host. This callback simply relays each such message
	 * to the application as it is received; the device does not maintain
	 * any suspend state machine of its own. Per the HID profile, a
	 * suspended device that detects local user activity wakes the host by
	 * reconnecting, after which the host (optionally) issues EXIT_SUSPEND.
	 * Note EXIT_SUSPEND is not mandatory, so the application must not rely
	 * on it to leave the suspended state.
	 *
	 * Because the messages are relayed verbatim, the callback may fire on
	 * every SUSPEND/EXIT_SUSPEND the host sends, including duplicates. If
	 * the application needs idempotent or per-peer suspend behaviour across
	 * reconnect (e.g. HID DRE/BV-09-C), it tracks that itself, keyed on the
	 * peer identity it obtains from the ACL connection.
	 *
	 * @param hid HID device object.
	 * @param suspended true if entering suspend, false if exiting.
	 */
	void (*suspend)(struct bt_hid_device *hid, bool suspended);
};

/** @brief Register HID device callbacks.
 *
 * The callback pointer must remain valid until bt_hid_device_unregister() is
 * called. A second registration without an intervening unregister is rejected
 * with -EALREADY; the callbacks are not replaced.
 *
 * @param cb  Callbacks to register. Must not be NULL, and the mandatory
 *            @ref bt_hid_device_cb.get_report and
 *            @ref bt_hid_device_cb.set_report callbacks must be provided.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p cb is NULL or a mandatory callback is missing.
 * @retval -EALREADY if a callback set is already registered.
 */
int bt_hid_device_register(const struct bt_hid_device_cb *cb);

/** @brief Unregister HID device callbacks and disable HID services.
 *
 * This clears the callbacks and unregisters the L2CAP servers. The
 * caller must disconnect any active HID session before calling this
 * function; otherwise -EBUSY is returned and no action is taken.
 *
 * @retval 0 on success.
 * @retval -EBUSY if an HID session is still active.
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

/** @brief Allocate/create a PDU buffer suitable for HID over L2CAP.
 *
 * If @p pool is NULL a default pool may be used by the implementation.
 *
 * @note The returned buffer may be fragmented. Use net_buf_frag_add()
 *       to append data.
 *
 * @param pool Buffer pool to allocate from, or NULL to use the
 *             default connection TX pool.
 *
 * @return A new net_buf instance or NULL on failure.
 */
struct net_buf *bt_hid_device_create_pdu(struct net_buf_pool *pool);

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
int bt_hid_device_input_report(struct bt_hid_device *hid, struct net_buf *buf);

/** @brief Trigger a virtual cable unplug procedure for the given HID association.
 *
 * This will request the remote host to treat the device as unplugged.
 *
 * @param hid HID device instance to unplug.
 *
 * @return 0 on success or a negative errno on failure.
 */
int bt_hid_device_virtual_cable_unplug(struct bt_hid_device *hid);

/** @brief Obtain the ACL connection associated with a HID device.
 *
 * Useful from callbacks that only receive the HID device object (for
 * example @ref bt_hid_device_cb.vc_unplug) but need the peer identity, e.g.
 * to clear bonding information.
 *
 * @param hid HID device object.
 *
 * @return Connection object associated with the HID device, or NULL if the
 *         device is not connected. The caller gets a new reference to the
 *         connection object which must be released with bt_conn_unref() once
 *         done using the object.
 */
struct bt_conn *bt_hid_device_get_conn(struct bt_hid_device *hid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_HID_DEVICE_H_ */
