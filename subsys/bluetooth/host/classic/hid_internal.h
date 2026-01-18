/** @file
 *  @brief Internal APIs for Bluetooth HID Device handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/hid_device.h>

/* Define the HID PSM */
#define BT_L2CAP_PSM_HID_CTL 0x0011
#define BT_L2CAP_PSM_HID_INT 0x0013

/* Define the HID transaction types */
#define BT_HID_TYPE_HANDSHAKE    0x00
#define BT_HID_TYPE_CONTROL      0x01
#define BT_HID_TYPE_GET_REPORT   0x04
#define BT_HID_TYPE_SET_REPORT   0x05
#define BT_HID_TYPE_GET_PROTOCOL 0x06
#define BT_HID_TYPE_SET_PROTOCOL 0x07
#define BT_HID_TYPE_GET_IDLE     0x08
#define BT_HID_TYPE_SET_IDLE     0x09
#define BT_HID_TYPE_DATA         0x0a
#define BT_HID_TYPE_DATAC        0x0b

/* Parameters for Control */
#define BT_HID_CONTROL_SUSPEND              0x03
#define BT_HID_CONTROL_EXIT_SUSPEND         0x04
#define BT_HID_CONTROL_VIRTUAL_CABLE_UNPLUG 0x05

/* Parameters for Protocol Type */
#define BT_HID_PROTOCOL_MASK   0x01
#define BT_HID_PROTOCOL_REPORT 0x01

/* Parameters for Report Type */
#define HID_PAR_REPORT_TYPE_MASK 0x03
#define HID_PAR_REPORT_SIZE_MASK 0x04

/* HID Device States */
#define BT_HID_STATE_DISCONNECTED    0x00
#define BT_HID_STATE_CTRL_CONNECTING 0x01
#define BT_HID_STATE_CTRL_CONNECTED  0x02
#define BT_HID_STATE_INTR_CONNECTING 0x03
#define BT_HID_STATE_CONNECTED       0x04
#define BT_HID_STATE_DISCONNECTING   0x05

/** @brief HID report types used for Get/Set/Data operations
 *
 * These map to report type fields in HID messages. `INPUT` reports are
 * typically device->host, `OUTPUT` are host->device, and `FEATURE`
 * represents device-specific feature reports.
 */
struct bt_hid_header {
	uint8_t param: 4;
	uint8_t type: 4;
} __packed;

/* Initialize HID service */
int bt_hid_dev_init(void);
