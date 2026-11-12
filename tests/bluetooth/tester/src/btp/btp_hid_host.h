/* btp_hid_host.h - Bluetooth HID Host tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BTP_HID_HOST_H
#define __BTP_HID_HOST_H

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* HID Host Commands */
#define BTP_HID_HOST_READ_SUPPORTED_COMMANDS    0x01
struct btp_hid_host_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HID_HOST_CMD_REGISTER               0x02
struct btp_hid_host_register_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_HOST_CMD_UNREGISTER             0x03
struct btp_hid_host_unregister_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_HOST_CMD_CONNECT                0x04
struct btp_hid_host_connect_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_HOST_CMD_DISCONNECT             0x05
struct btp_hid_host_disconnect_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_HOST_CMD_GET_REPORT             0x06
struct btp_hid_host_get_report_cmd {
	uint8_t report_type;
	uint8_t report_id;
	uint16_t buffer_size;
} __packed;

#define BTP_HID_HOST_CMD_SET_REPORT             0x07
struct btp_hid_host_set_report_cmd {
	uint8_t report_type;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_HID_HOST_CMD_GET_PROTOCOL           0x08
struct btp_hid_host_get_protocol_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_HOST_CMD_SET_PROTOCOL           0x09
struct btp_hid_host_set_protocol_cmd {
	uint8_t protocol;
} __packed;

#define BTP_HID_HOST_CMD_SEND_OUTPUT_REPORT     0x0a
struct btp_hid_host_send_output_report_cmd {
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_HID_HOST_CMD_SUSPEND                0x0b
struct btp_hid_host_suspend_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_HOST_CMD_EXIT_SUSPEND           0x0c
struct btp_hid_host_exit_suspend_cmd {
	uint8_t unused;
} __packed;

#define BTP_HID_HOST_CMD_VIRTUAL_CABLE_UNPLUG   0x0d
struct btp_hid_host_virtual_cable_unplug_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_HID_HOST_CMD_SNIFF_SUBRATING        0x0e
struct btp_hid_host_sniff_subrating_cmd {
	uint8_t address_type;
	bt_addr_t address;
	uint16_t max_latency;
	uint16_t min_remote_timeout;
	uint16_t min_local_timeout;
} __packed;

/* HID Host Events */
#define BTP_HID_HOST_EV_CONNECTED               0x80
struct btp_hid_host_connected_ev {
	bt_addr_t address;
} __packed;

#define BTP_HID_HOST_EV_DISCONNECTED            0x81
struct btp_hid_host_disconnected_ev {
	bt_addr_t address;
} __packed;

#define BTP_HID_HOST_EV_INPUT_REPORT            0x82
struct btp_hid_host_input_report_ev {
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_HID_HOST_EV_GET_REPORT              0x83
struct btp_hid_host_get_report_ev {
	uint8_t result_code;
	uint8_t report_type;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_HID_HOST_EV_SET_REPORT              0x84
struct btp_hid_host_set_report_ev {
	uint8_t result_code;
} __packed;

#define BTP_HID_HOST_EV_GET_PROTOCOL            0x85
struct btp_hid_host_get_protocol_ev {
	uint8_t result_code;
	uint8_t protocol;
} __packed;

#define BTP_HID_HOST_EV_SET_PROTOCOL            0x86
struct btp_hid_host_set_protocol_ev {
	uint8_t result_code;
} __packed;

#endif /* __BTP_HID_HOST_H */
