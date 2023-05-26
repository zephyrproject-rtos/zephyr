/* btp_csis.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/csip.h>

/* CSIS commands */
#define BTP_CSIS_READ_SUPPORTED_COMMANDS	0x01
struct btp_csis_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_CSIS_SET_MEMBER_LOCK		0x02
struct btp_csis_set_member_lock_cmd {
	bt_addr_le_t address;
	uint8_t lock;
	uint8_t force;
} __packed;

#define BTP_CSIS_GET_MEMBER_RSI			0x03
struct btp_csis_get_member_rsi_cmd {
	bt_addr_le_t address;
} __packed;

struct btp_csis_get_member_rsi_rp {
	uint8_t rsi[BT_CSIP_RSI_SIZE];
} __packed;
