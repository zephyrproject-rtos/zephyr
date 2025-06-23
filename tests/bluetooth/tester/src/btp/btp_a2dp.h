/* l2cap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 * Copyright (c) 2025 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

#define BTP_A2DP_READ_SUPPORTED_COMMANDS  0x01
struct btp_a2dp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_A2DP_REGISTER_EP 0x02
#define BTP_A2DP_REGISTER_EP_ROLE_SINK    0x01
#define BTP_A2DP_REGISTER_EP_ROLE_SOURCE  0x02
#define BTP_A2DP_REGISTER_EP_CODEC_SBC    0x01
struct btp_a2dp_register_ep_cmd {
        uint8_t role;
        uint8_t codec;
} __packed;

#define BTP_A2DP_CONNECT            0x03
struct btp_a2dp_connect_cmd {
        bt_addr_t address;
} __packed;