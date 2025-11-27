/* btp_a2dp.h - Bluetooth A2DP Tester */

/*
 * Copyright 2025 -2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* A2DP Service */
/* Commands */
#define BTP_A2DP_READ_SUPPORTED_COMMANDS 0x01
struct btp_a2dp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_A2DP_CONNECT 0x02
struct btp_a2dp_connect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_A2DP_DISCONNECT 0x03
struct btp_a2dp_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_A2DP_REGISTER_ENDPOINT 0x04
struct btp_a2dp_register_endpoint_cmd {
	uint8_t media_type;
	uint8_t tsep;
	uint8_t codec_type;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_DISCOVER 0x05
struct btp_a2dp_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_A2DP_GET_CAPABILITIES 0x06
struct btp_a2dp_get_capabilities_cmd {
	bt_addr_le_t address;
	uint8_t ep_id;
} __packed;

#define BTP_A2DP_GET_ALL_CAPABILITIES 0x07
struct btp_a2dp_get_all_capabilities_cmd {
	bt_addr_le_t address;
	uint8_t ep_id;
} __packed;

#define BTP_A2DP_CONFIG 0x08
struct btp_a2dp_config_cmd {
	bt_addr_le_t address;
	uint8_t local_ep_id;
	uint8_t remote_ep_id;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_ESTABLISH 0x09
struct btp_a2dp_establish_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_RELEASE 0x0a
struct btp_a2dp_release_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_START 0x0b
struct btp_a2dp_start_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP__SUSPEND 0x0c
struct btp_a2dp_suspend_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_RECONFIG 0x0d
struct btp_a2dp_reconfig_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_ABORT 0x0e
struct btp_a2dp_abort_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_GET_CONFIG 0x0f
struct btp_a2dp_get_config_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_DELAY_REPORT 0x10
struct btp_a2dp_delay_report_cmd {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint16_t delay;
} __packed;

/* Events */
#define BTP_A2DP_EV_CONNECTED 0x80
struct btp_a2dp_connected_ev {
	bt_addr_le_t address;
	int8_t result;
} __packed;

#define BTP_A2DP_EV_DISCONNECTED 0x81
struct btp_a2dp_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_A2DP_EV_DISCOVERED 0x82
struct bt_a2dp_sep_info {
	uint8_t id: 6;
	uint8_t inuse: 1;
	uint8_t rfa0: 1;
	uint8_t media_type: 4;
	uint8_t tsep: 1;
	uint8_t rfa1: 3;
};
struct btp_a2dp_discovered_ev {
	bt_addr_le_t address;
	int8_t result;
	uint8_t len;
	struct bt_a2dp_sep_info sep[0];
} __packed;

#define BTP_A2DP_EV_GET_CAPABILITIES 0x83
struct bt_a2dp_service_category_capabilities {
	uint8_t service_category;
	uint8_t losc;
	uint8_t capability_info[0];
};
struct btp_a2dp_get_capabilities_ev {
	bt_addr_le_t address;
	uint8_t ep_id;
	struct bt_a2dp_service_category_capabilities cap[0];
} __packed;

#define BTP_A2DP_EV_CONFIG_REQ 0x84
struct btp_a2dp_config_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_EV_CONFIG_RSP 0x85
struct btp_a2dp_config_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_RECONFIG_REQ 0x86
struct btp_a2dp_reconfig_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_EV_ESTABLISH_REQ 0x87
struct btp_a2dp_establish_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_ESTABLISH_RSP 0x88
struct btp_a2dp_establish_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_RELEASE_REQ 0x89
struct btp_a2dp_release_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_RELEASE_RSP 0x8a
struct btp_a2dp_release_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_START_REQ 0x8b
struct btp_a2dp_start_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_START_RSP 0x8c
struct btp_a2dp_start_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_SUSPEND_REQ 0x8d
struct btp_a2dp_suspend_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_SUSPEND_RSP 0x8e
struct btp_a2dp_suspend_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_ABORT_REQ 0x8f
struct btp_a2dp_abort_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_ABORT_RSP 0x90
struct btp_a2dp_abort_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;

#define BTP_A2DP_EV_GET_CONFIG_REQ 0x91
struct btp_a2dp_get_config_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_GET_CONFIG_RSP 0x92
struct btp_a2dp_get_config_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
	uint8_t delay_report;
	uint8_t codec_ie_len;
	uint8_t codec_ie[0];
} __packed;

#define BTP_A2DP_EV_STREAM_RECV 0x93
struct btp_a2dp_stream_recv_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint16_t seq_num;
	uint32_t timestamp;
	uint16_t data_len;
	uint8_t data[0];
} __packed;

#define BTP_A2DP_EV_STREAM_SENT 0x94
struct btp_a2dp_stream_sent_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
} __packed;

#define BTP_A2DP_EV_DELAY_REPORT_REQ 0x95
struct btp_a2dp_delay_report_req_ev {
	bt_addr_le_t address;
	uint8_t result;
	uint8_t stream_id;
	uint16_t delay;
} __packed;

#define BTP_A2DP_EV_DELAY_REPORT_RSP 0x96
struct btp_a2dp_delay_report_rsp_ev {
	bt_addr_le_t address;
	uint8_t stream_id;
	uint8_t rsp_err_code;
} __packed;
