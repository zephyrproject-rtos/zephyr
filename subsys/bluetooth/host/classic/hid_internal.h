/** @file
 *  @brief Internal APIs for Bluetooth HID Device handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/hid_device.h>

/** @brief HID PSM values for control/interrupt channels. */
#define BT_L2CAP_PSM_HID_CTL 0x0011
#define BT_L2CAP_PSM_HID_INT 0x0013

/** @brief HIDP message types (upper nibble of HID header). */
typedef enum __packed {
	BT_HID_TRANS_HANDSHAKE = 0x00,
	BT_HID_TRANS_CONTROL = 0x01,
	BT_HID_TRANS_GET_REPORT = 0x04,
	BT_HID_TRANS_SET_REPORT = 0x05,
	BT_HID_TRANS_GET_PROTOCOL = 0x06,
	BT_HID_TRANS_SET_PROTOCOL = 0x07,
	BT_HID_TRANS_GET_IDLE = 0x08,
	BT_HID_TRANS_SET_IDLE = 0x09,
	BT_HID_TRANS_DATA = 0x0a,
	BT_HID_TRANS_DATAC = 0x0b,
} bt_hid_msg_type;

/** @brief HID_CONTROL parameters (lower nibble of HID header). */
typedef enum __packed {
	/* NOP (deprecated): no operation */
	BT_HID_PAR_CONTROL_NOP = 0x00,
	/* HARD_RESET (deprecated): device performs POST then initializes */
	BT_HID_PAR_CONTROL_HARD_RESET = 0x01,
	/* SOFT_RESET (deprecated): device initializes internal variables */
	BT_HID_PAR_CONTROL_SOFT_RESET = 0x02,
	/* SUSPEND: enter reduced power mode */
	BT_HID_PAR_CONTROL_SUSPEND = 0x03,
	/* EXIT_SUSPEND: exit reduced power mode */
	BT_HID_PAR_CONTROL_EXIT_SUSPEND = 0x04,
	/* VIRTUAL_CABLE_UNPLUG: virtual cable unplug request */
	BT_HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG = 0x05,
} bt_hid_control_param;

/** @brief Mask for HID protocol parameter in SET/GET_PROTOCOL. */
#define BT_HID_PROTOCOL_MASK 0x01

/** @brief Report type field mask (lower two bits of parameter). */
#define BT_HID_PARAM_REPORT_TYPE_MASK 0x03
/** @brief Report size present flag in parameter field. */
#define BT_HID_PARAM_REPORT_SIZE_MASK 0x04

/** @brief Report type values used in GET/SET/DATA messages. */
typedef enum __packed {
	BT_HID_PAR_REP_TYPE_OTHER = 0x00,
	BT_HID_PAR_REP_TYPE_INPUT = 0x01,
	BT_HID_PAR_REP_TYPE_OUTPUT = 0x02,
	BT_HID_PAR_REP_TYPE_FEATURE = 0x03,
} bt_hid_param_report_type;

/** @brief HID device state machine values. */
enum bt_hid_state {
	BT_HID_STATE_DISCONNECTED = 0x00,
	BT_HID_STATE_CTRL_CONNECTING = 0x01,
	BT_HID_STATE_CTRL_CONNECTED = 0x02,
	BT_HID_STATE_INTR_CONNECTING = 0x03,
	BT_HID_STATE_CONNECTED = 0x04,
	BT_HID_STATE_DISCONNECTING = 0x05,
};

/** @brief HID header encoding (1 byte): upper nibble = message type, lower nibble = parameter. */
#define BT_HID_BUILD_HDR(t, p)       (uint8_t)((((t) & 0x0F) << 4) | ((p) & 0x0F))
#define BT_HID_GET_TRANS_FROM_HDR(x) (((x) >> 4) & 0x0f)
#define BT_HID_GET_PARAM_FROM_HDR(x) ((x) & 0x0f)

/** @brief HID header byte stored in a packed struct for buffer access. */
struct bt_hid_hdr {
	uint8_t header;
} __packed;
