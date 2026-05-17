/** @file
 *  @brief Internal APIs for Bluetooth HID handling.
 */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_HID_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_HID_INTERNAL_H_

#include <stdint.h>

/** @brief HID header size in bytes. */
#define BT_HID_HDR_SIZE 1U

/** @brief HID PSM values for control/interrupt channels. */
#define BT_L2CAP_PSM_HID_CTL 0x0011
#define BT_L2CAP_PSM_HID_INT 0x0013

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

/** @brief HID_CONTROL parameters (lower nibble of HID header). */
#define BT_HID_PAR_CONTROL_NOP                  0x00
#define BT_HID_PAR_CONTROL_HARD_RESET           0x01
#define BT_HID_PAR_CONTROL_SOFT_RESET           0x02
#define BT_HID_PAR_CONTROL_SUSPEND              0x03
#define BT_HID_PAR_CONTROL_EXIT_SUSPEND         0x04
#define BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG 0x05

/** @brief Mask for HID protocol parameter in SET/GET_PROTOCOL. */
#define BT_HID_PROTOCOL_MASK 0x01

/** @brief HID Protocol Mode values. */
#define BT_HID_PROTOCOL_BOOT_MODE   0x00
#define BT_HID_PROTOCOL_REPORT_MODE 0x01

/** @brief Report type field mask (lower two bits of parameter). */
#define BT_HID_PARAM_REPORT_TYPE_MASK 0x03
/** @brief Report size present flag in parameter field. */
#define BT_HID_PARAM_REPORT_SIZE_MASK 0x04

/** @brief Report type values used in GET/SET/DATA messages. */
#define BT_HID_PAR_REP_TYPE_OTHER   0x00
#define BT_HID_PAR_REP_TYPE_INPUT   0x01
#define BT_HID_PAR_REP_TYPE_OUTPUT  0x02
#define BT_HID_PAR_REP_TYPE_FEATURE 0x03

/** @brief Handshake result codes. */
#define BT_HID_HANDSHAKE_RSP_SUCCESS             0x00
#define BT_HID_HANDSHAKE_RSP_NOT_READY           0x01
#define BT_HID_HANDSHAKE_RSP_ERR_INVALID_REP_ID  0x02
#define BT_HID_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ 0x03
#define BT_HID_HANDSHAKE_RSP_ERR_INVALID_PARAM   0x04
#define BT_HID_HANDSHAKE_RSP_ERR_UNKNOWN         0x0e
#define BT_HID_HANDSHAKE_RSP_ERR_FATAL           0x0f

/** @brief HID device state machine values. */
enum bt_hid_state {
	BT_HID_STATE_DISCONNECTED = 0x00,
	BT_HID_STATE_CTRL_CONNECTING = 0x01,
	BT_HID_STATE_CTRL_CONNECTED = 0x02,
	BT_HID_STATE_INTR_CONNECTING = 0x03,
	BT_HID_STATE_CONNECTED = 0x04,
	BT_HID_STATE_DISCONNECTING = 0x05,
};

/** @brief HID channel type. */
enum bt_hid_channel {
	BT_HID_CHANNEL_UNKNOWN = 0,
	BT_HID_CHANNEL_CTRL,
	BT_HID_CHANNEL_INTR,
};

/** @brief HID connection role. */
enum bt_hid_role {
	BT_HID_ROLE_ACCEPTOR = 0,
	BT_HID_ROLE_INITIATOR,
};

/** @brief HID header encoding (1 byte): upper nibble = message type, lower nibble = parameter. */
#define BT_HID_BUILD_HDR(t, p)       (uint8_t)((((t) & 0x0F) << 4) | ((p) & 0x0F))
#define BT_HID_GET_TRANS_FROM_HDR(x) (((x) >> 4) & 0x0f)
#define BT_HID_GET_PARAM_FROM_HDR(x) ((x) & 0x0f)

/** @brief HID header byte stored in a packed struct for buffer access. */
struct bt_hid_hdr {
	uint8_t header;
} __packed;

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_HID_INTERNAL_H_ */
