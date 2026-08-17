/** @file
 *  @brief Internal APIs for Bluetooth HID Device handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/hid_device.h>
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
/** @brief Report size present flag in parameter field. */
#define BT_HID_PARAM_REPORT_SIZE_MASK BIT(2)

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
