/* btp_cas.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CAS commands */
#define BTP_CAS_READ_SUPPORTED_COMMANDS	0x01
struct btp_cas_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_CAS_SET_MEMBER_LOCK		0x02
struct btp_cas_set_member_lock_cmd {
	bt_addr_le_t address;
	uint8_t lock;
	uint8_t force;
} __packed;

#define BTP_CAS_GET_MEMBER_RSI		0x03
struct btp_cas_get_member_rsi_cmd {
	bt_addr_le_t address;
} __packed;

struct btp_cas_get_member_rsi_rp {
	uint8_t rsi[BT_CSIP_RSI_SIZE];
} __packed;
