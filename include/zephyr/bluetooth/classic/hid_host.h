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

#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

/** @brief HID Protocol Mode: Boot Mode. */
#define BT_HID_PROTOCOL_BOOT_MODE   0x00
/** @brief HID Protocol Mode: Report Mode. */
#define BT_HID_PROTOCOL_REPORT_MODE 0x01

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HID Host session wrapper for an L2CAP channel.
 *
 * Each HID Host connection maintains two sessions: control and interrupt.
 */
struct bt_hid_host_session {
	struct bt_l2cap_br_chan br_chan;
	uint8_t type;
};

/** @brief HID Host instance.
 *
 * Represents a single HID association with a remote HID Device.
 * Instances are allocated from a fixed pool and must not be freed
 * by the caller.
 */
struct bt_hid_host {
	/** Role: whether we initiated or accepted the connection. */
	uint8_t role;
	/** Control channel session (PSM 0x0011). */
	struct bt_hid_host_session ctrl_session;
	/** Interrupt channel session (PSM 0x0013). */
	struct bt_hid_host_session intr_session;

	/** True when the remote device is in Boot Protocol Mode. */
	bool boot_mode;
	/** True when SUSPEND has been sent. */
	bool suspended;
	/** True when the remote descriptor declares Report IDs. */
	bool has_report_id;
	/** Runtime connection state. */
	uint8_t state;

	/** HID Report Descriptor obtained from SDP. */
	uint8_t descriptor[CONFIG_BT_HID_HOST_DESCRIPTOR_MAX_LEN];
	/** Length of the HID Report Descriptor in bytes (0 if not available). */
	uint16_t descriptor_len;
	/** HIDDeviceSubclass from SDP. */
	uint8_t subclass;
	/** HIDVirtualCable attribute from SDP. */
	bool virtual_cable;
	/** HIDReconnectInitiate attribute from SDP. */
	bool reconnect_initiate;
	/** HIDBootDevice attribute from SDP. */
	bool boot_device;
	/** HIDSupervisionTimeout from SDP (ms). */
	uint16_t supervision_timeout;
	/** HIDSSRHostMaxLatency from SDP. */
	uint16_t ssr_max_latency;
	/** HIDSSRHostMinTimeout from SDP. */
	uint16_t ssr_min_timeout;

	/** @cond INTERNAL_HIDDEN */
	struct k_work_delayable timeout_work;
	uint8_t w4_response;
	/** @endcond */
};

/** @brief Callbacks supplied by the HID Host application.
 *
 * Register via bt_hid_host_register() to receive events from
 * connected HID Devices.
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
	 * @param hid       HID Host instance.
	 * @param report_id Report ID (0 if no Report IDs in descriptor).
	 * @param buf       Report payload (ownership retained by stack).
	 */
	void (*input_report)(struct bt_hid_host *hid, uint8_t report_id,
			     struct net_buf *buf);

	/** @brief GET_REPORT response received.
	 *
	 * DATA message received on the control channel in response to a
	 * previously sent GET_REPORT request.
	 *
	 * @param hid       HID Host instance.
	 * @param type      Report type (INPUT/OUTPUT/FEATURE).
	 * @param report_id Report ID (0 if unused).
	 * @param buf       Report payload (ownership retained by stack).
	 */
	void (*get_report_rsp)(struct bt_hid_host *hid, uint8_t type,
			       uint8_t report_id, struct net_buf *buf);

	/** @brief HANDSHAKE response received.
	 *
	 * Called for SET_REPORT, SET_PROTOCOL responses, or error
	 * responses to GET_REPORT/GET_PROTOCOL.
	 *
	 * @param hid    HID Host instance.
	 * @param result Handshake result code (BT_HID_HANDSHAKE_RSP_*).
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
	 * message. The stack will automatically disconnect and unpair.
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
 * Disconnects all active HID sessions and unregisters L2CAP servers.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_unregister(void);

/** @brief Initiate an HID connection to a remote HID Device.
 *
 * Performs SDP discovery to retrieve the HID descriptor and device
 * attributes, then opens L2CAP control and interrupt channels.
 * For previously known devices (descriptor already cached), SDP is
 * skipped and L2CAP channels are opened directly.
 *
 * @param conn ACL connection to the remote device.
 * @param hid  Pointer to store the HID Host instance on success.
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_connect(struct bt_conn *conn, struct bt_hid_host **hid);

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
 * @param hid         HID Host instance.
 * @param type        Report type (BT_HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE).
 * @param report_id   Report ID (0 if descriptor has no Report IDs).
 * @param buffer_size Maximum response payload size (0 = no limit).
 *
 * @return 0 on success, negative errno on failure.
 */
int bt_hid_host_get_report(struct bt_hid_host *hid, uint8_t type,
			   uint8_t report_id, uint16_t buffer_size);

/** @brief Send a SET_REPORT request to the HID Device.
 *
 * The response arrives via the handshake callback.
 * The buffer must include the Report ID as the first byte if the
 * descriptor declares Report IDs.
 *
 * @param hid  HID Host instance.
 * @param type Report type (BT_HID_REPORT_TYPE_INPUT/OUTPUT/FEATURE).
 * @param buf  Buffer containing report payload (consumed on success).
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
 * This is an asynchronous transfer with no acknowledgement.
 * The buffer must include the Report ID as the first byte if the
 * descriptor declares Report IDs.
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
 * Sends a VCU control message, then destroys bonding information
 * and disconnects per HID spec v1.1.2 Section 3.1.2.2.3.
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
