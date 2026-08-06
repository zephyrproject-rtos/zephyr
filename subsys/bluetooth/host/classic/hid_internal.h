/** @file
 *  @brief Internal APIs shared by the Bluetooth HID Device and Host profiles.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/l2cap.h>

/** @brief HID header size (1 byte). */
#define BT_HID_HDR_SIZE 1U

/** @brief HID PSM values for control/interrupt channels. */
#define BT_L2CAP_PSM_HID_CTRL 0x0011
#define BT_L2CAP_PSM_HID_INTR 0x0013

/** @brief HIDP message types (upper nibble of HID header). */
#define BT_HID_MSG_TYPE_HANDSHAKE    0x00
#define BT_HID_MSG_TYPE_CONTROL      0x01
#define BT_HID_MSG_TYPE_GET_REPORT   0x04
#define BT_HID_MSG_TYPE_SET_REPORT   0x05
#define BT_HID_MSG_TYPE_GET_PROTOCOL 0x06
#define BT_HID_MSG_TYPE_SET_PROTOCOL 0x07
#define BT_HID_MSG_TYPE_GET_IDLE     0x08
#define BT_HID_MSG_TYPE_SET_IDLE     0x09
#define BT_HID_MSG_TYPE_DATA         0x0a
#define BT_HID_MSG_TYPE_DATAC        0x0b

/** @brief HID handshake result codes (4-bit param field). */
#define BT_HID_HS_RSP_SUCCESS               0x00
#define BT_HID_HS_RSP_NOT_READY             0x01
#define BT_HID_HS_RSP_ERR_INVALID_REPORT_ID 0x02
#define BT_HID_HS_RSP_ERR_UNSUPPORTED_REQ   0x03
#define BT_HID_HS_RSP_ERR_INVALID_PARAM     0x04
#define BT_HID_HS_RSP_ERR_UNKNOWN           0x0e
#define BT_HID_HS_RSP_ERR_FATAL             0x0f

/** @brief HID_CONTROL operations (lower nibble of HID header). */
#define BT_HID_CONTROL_NOP                  0x00
#define BT_HID_CONTROL_HARD_RESET           0x01
#define BT_HID_CONTROL_SOFT_RESET           0x02
#define BT_HID_CONTROL_SUSPEND              0x03
#define BT_HID_CONTROL_EXIT_SUSPEND         0x04
#define BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG 0x05

/** @brief Mask for HID protocol parameter in SET/GET_PROTOCOL (bit 0). */
#define BT_HID_PROTOCOL_MASK GENMASK(0, 0)

/** @brief Report type field mask (lower two bits of parameter). */
#define BT_HID_PARAM_REPORT_TYPE_MASK GENMASK(1, 0)
/** @brief Report size present flag in parameter field.
 *
 *  HID spec v1.1.2 Table 3.4: within the GET_REPORT header octet bit 3 is the
 *  Size flag and bit 2 is reserved, so the flag is bit 3 of the parameter.
 */
#define BT_HID_PARAM_REPORT_SIZE_MASK BIT(3)

/** @brief Report type values used in GET/SET/DATA messages. */
#define BT_HID_PAR_REP_TYPE_OTHER   0x00
#define BT_HID_PAR_REP_TYPE_INPUT   0x01
#define BT_HID_PAR_REP_TYPE_OUTPUT  0x02
#define BT_HID_PAR_REP_TYPE_FEATURE 0x03

/** @brief HID header field masks (1 byte): upper nibble = message type,
 *  lower nibble = parameter.
 */
#define BT_HID_HDR_TYPE_MASK  GENMASK(7, 4)
#define BT_HID_HDR_PARAM_MASK GENMASK(3, 0)

/** @brief HID header encoding/decoding helpers. */
#define BT_HID_BUILD_HDR(t, p)                                                                 \
	(uint8_t)(FIELD_PREP(BT_HID_HDR_TYPE_MASK, (t)) | FIELD_PREP(BT_HID_HDR_PARAM_MASK, (p)))
#define BT_HID_HDR_GET_TYPE(x)  FIELD_GET(BT_HID_HDR_TYPE_MASK, (x))
#define BT_HID_HDR_GET_PARAM(x) FIELD_GET(BT_HID_HDR_PARAM_MASK, (x))

/** @brief HID header byte stored in a packed struct for buffer access. */
struct bt_hid_hdr {
	uint8_t header;
} __packed;

/** @brief Type of the HID channel */
enum bt_hid_channel_type {
	/** Channel type unknown or not initialized. */
	BT_HID_CHANNEL_UNKNOWN = 0,
	/** Control channel. */
	BT_HID_CHANNEL_CTRL,
	/** Interrupt channel. */
	BT_HID_CHANNEL_INTR,
};

/** @brief Role of the HID device relative to the remote peer */
enum bt_hid_role {
	/** Local device accepted an incoming connection (server role). */
	BT_HID_ROLE_ACCEPTOR = 0,
	/** Local device initiated the connection (client role). */
	BT_HID_ROLE_INITIATOR
};

/** @brief HID device state machine values. */
enum bt_hid_state {
	BT_HID_STATE_DISCONNECTED = 0x00,
	BT_HID_STATE_CTRL_CONNECTING = 0x01,
	BT_HID_STATE_CTRL_CONNECTED = 0x02,
	BT_HID_STATE_INTR_CONNECTING = 0x03,
	BT_HID_STATE_CONNECTED = 0x04,
	BT_HID_STATE_DISCONNECTING = 0x05,
};

/** @brief HID session wrapper for an L2CAP channel */
struct bt_hid_session {
	/** Underlying BR/EDR L2CAP channel. */
	struct bt_l2cap_br_chan br_chan;
	/** Channel type (control or interrupt). */
	enum bt_hid_channel_type type;
};

/** @brief HID device instance (opaque to applications) */
struct bt_hid_device {
	/** Role of this device (initiator or acceptor). */
	enum bt_hid_role role;
	/** Control channel session. */
	struct bt_hid_session ctrl_session;
	/** Interrupt channel session. */
	struct bt_hid_session intr_session;

	/** True when the HID device is in Boot Protocol Mode. */
	bool boot_mode;
	/** Runtime connection state. */
	uint8_t state;

	/** True while the control channel is connected. */
	bool ctrl_connected;
	/** True while the interrupt channel is connected. */
	bool intr_connected;

	struct k_work_delayable intr_timeout;
	struct k_work vcu_disconnect;
};

/** @brief Control channel transaction the HID Host is waiting a response for.
 *
 * HID spec v1.1.2 Section 3.2.1 allows a single outstanding transaction on the
 * Control channel, so this doubles as the serialization state.
 */
enum bt_hid_w4 {
	/** No transaction outstanding. */
	BT_HID_W4_NONE = 0,
	BT_HID_W4_GET_REPORT,
	BT_HID_W4_SET_REPORT,
	BT_HID_W4_GET_PROTOCOL,
	BT_HID_W4_SET_PROTOCOL,
};

/** @brief Outstanding Control channel transaction of a HID Host connection.
 *
 * Filled in by the requesting thread and consumed either by the RX thread when
 * the reply arrives or by the transaction timeout, see the claim token in
 * @ref bt_hid_host.
 */
struct bt_hid_req {
	/** Transaction type. */
	uint8_t type;
	/** Report type sent with GET_REPORT, echoed by the DATA reply. */
	uint8_t report_type;
	/** BufferSize sent with GET_REPORT, 0 when the field was omitted. */
	uint16_t buffer_size;
	/** Protocol mode sent with SET_PROTOCOL. */
	uint8_t protocol;
};

/** @brief HID Host session wrapper for an L2CAP channel.
 *
 * Each HID Host connection maintains two sessions: control and interrupt.
 */
struct bt_hid_host_session {
	/** Underlying BR/EDR L2CAP channel. */
	struct bt_l2cap_br_chan br_chan;
	/** Channel type: control or interrupt. */
	uint8_t type;
};

/** @brief HID Host instance (opaque to applications) */
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
	/** Runtime connection state. */
	uint8_t state;

	/** True while the control channel is connected. */
	bool ctrl_connected;
	/** True while the interrupt channel is connected. */
	bool intr_connected;
	/** Outstanding Control channel transaction.
	 *
	 * The contents are only valid while the transaction is claimed, see
	 * @ref _req.
	 */
	struct bt_hid_req req;
	/** Claim token of @ref req.
	 *
	 * Holds NULL when no transaction is outstanding, the claiming sentinel
	 * while @ref req is being filled in, and &req once the transaction is
	 * published. Every transition is a single atomic operation because the
	 * requesting thread, the RX thread and the timeout work all race for it.
	 */
	atomic_ptr_t _req;
	/** INTR channel connect timeout. */
	struct k_work_delayable intr_work;
	/** Control channel transaction timeout. */
	struct k_work_delayable trans_work;
	/** Virtual Cable Unplug teardown.
	 *
	 * Runs immediately when the unplug was received, where HID spec v1.1.2
	 * Section 3.1.2.2.3 makes this host the one that disconnects, and after a
	 * delay when the unplug was sent, where it only covers a peer that never
	 * acknowledges.
	 */
	struct k_work_delayable vcu_work;
};
