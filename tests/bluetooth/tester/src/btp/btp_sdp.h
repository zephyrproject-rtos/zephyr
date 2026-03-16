/* btp_sdp.h - Bluetooth tester headers */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* SDP Service */
/* Commands */
#define BTP_SDP_READ_SUPPORTED_COMMANDS		0x01
struct btp_sdp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_SDP_SEARCH_REQ			0x02
struct btp_sdp_search_req_cmd {
	bt_addr_le_t address;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

#define BTP_SDP_ATTR_REQ			0x03
struct btp_sdp_attr_req_cmd {
	bt_addr_le_t address;
	uint32_t service_record_handle;
} __packed;

#define BTP_SDP_SEARCH_ATTR_REQ			0x04
struct btp_sdp_search_attr_req_cmd {
	bt_addr_le_t address;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

/* Events */
#define BTP_SDP_EV_SERVICE_RECORD_HANDLE	0x80
struct btp_sdp_service_record_handle_ev {
	bt_addr_le_t address;
	uint8_t service_record_handle_count;
	uint32_t service_record_handle[];
} __packed;
