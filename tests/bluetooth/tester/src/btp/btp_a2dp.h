/* btp_a2dp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 * Copyright (c) 2025 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/l2cap.h>

#define BTP_A2DP_READ_SUPPORTED_COMMANDS   0x01
struct btp_a2dp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_A2DP_REGISTER_EP               0x02
#define BTP_A2DP_REGISTER_EP_CODEC_SBC     0x01
struct btp_a2dp_register_ep_cmd {
	uint8_t sep_type;
	uint8_t media_type;
} __packed;

#define BTP_A2DP_CONNECT                   0x03
struct btp_a2dp_connect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_A2DP_DISCOVER                  0x04

#define BTP_A2DP_CONFIGURE                 0x05

#define BTP_A2DP_ESTABLISH                 0x06

#define BTP_A2DP_START                     0x07

#define BTP_A2DP_SUSPEND                   0x08

#define BTP_A2DP_RELEASE                   0x09

#define BTP_A2DP_ABORT                     0x0A

#define BTP_A2DP_RECONFIGURE               0x0B

#define BTP_A2DP_EV_CONNECTED              0x81
struct btp_a2dp_connected_ev {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_GET_CAPABILITIES_RSP   0x82

#define BTP_A2DP_EV_SET_CONFIGURATION_RSP  0x83
struct btp_a2dp_set_config_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_ESTABLISH_RSP          0x84
struct btp_a2dp_establish_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_RELEASE_RSP            0x85
struct btp_a2dp_release_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_START_RSP              0x86
struct btp_a2dp_start_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_SUSPEND_RSP            0x87
struct btp_a2dp_suspend_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_DISCONNECTED           0x88

#define BTP_A2DP_EV_ABORT_RSP              0x89
struct btp_a2dp_abort_rsp {
	uint8_t errcode;
} __packed;

#define BTP_A2DP_EV_RECV_MEDIA             0x8A
struct btp_a2dp_recv_media {
	uint8_t frames_num;
	uint8_t length;
	uint8_t data[BT_L2CAP_RX_MTU];
} __packed;
