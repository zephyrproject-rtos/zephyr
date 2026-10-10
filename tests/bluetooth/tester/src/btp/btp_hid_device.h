/* btp_hid_device.h - Bluetooth HID Device tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BTP_HID_DEVICE_H
#define __BTP_HID_DEVICE_H

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* HID Device Commands */
#define BTP_HID_DEVICE_READ_SUPPORTED_COMMANDS    0x01
struct btp_hid_device_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HID_DEVICE_CMD_CONNECT                0x02
struct btp_hid_device_connect_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_DEVICE_CMD_DISCONNECT             0x03
struct btp_hid_device_disconnect_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_DEVICE_CMD_SEND_REPORT            0x04
struct btp_hid_device_send_report_cmd {
	uint8_t report_type;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_HID_DEVICE_CMD_REGISTER               0x05
struct btp_hid_device_register_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_DEVICE_CMD_UNREGISTER             0x06
struct btp_hid_device_unregister_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_DEVICE_CMD_VIRTUAL_CABLE_UNPLUG   0x07
struct btp_hid_device_virtual_cable_unplug_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_DEVICE_CMD_ENTER_SNIFF_MODE       0x08
struct btp_hid_device_enter_sniff_mode_cmd {
	uint8_t address_type;
	bt_addr_t address;
	uint16_t min_interval;
	uint16_t max_interval;
	uint16_t attempt;
	uint16_t timeout;
} __packed;

/* HID Device Events */
#define BTP_HID_DEVICE_EV_CONNECTED               0x80
struct btp_hid_device_connected_ev {
	bt_addr_t address;
} __packed;

#define BTP_HID_DEVICE_EV_DISCONNECTED            0x81
struct btp_hid_device_disconnected_ev {
	bt_addr_t address;
} __packed;

#endif /* __BTP_HID_DEVICE_H */
