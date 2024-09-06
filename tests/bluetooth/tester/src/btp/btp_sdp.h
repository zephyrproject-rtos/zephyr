/* btp_sdp.h - Bluetooth tester headers */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* SDP Service */
/* commands */
#define BTP_SDP_SEARCH_REQ		0x01
struct btp_sdp_search_req {
	bt_addr_t address;
	uint16_t uuid;
} __packed;

#define BTP_SDP_ATTR_REQ		0x02
struct btp_sdp_attr_req {
	bt_addr_t address;
	uint32_t service_record_handle;
} __packed;

#define BTP_SDP_SEARCH_ATTR_REQ		0x03
struct btp_sdp_search_attr_req {
	bt_addr_t address;
	uint16_t uuid;
} __packed;

#define BTP_SDP_EV_SERVICE_RECORD_HANDLE	0x81
struct btp_sdp_service_record_handle_ev {
	bt_addr_t address;
	uint32_t service_record_handle;
} __packed;
