/* btp_csip.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/csip.h>

/* CSIP commands */
#define BTP_CSIP_READ_SUPPORTED_COMMANDS	0x01
struct btp_csip_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_CSIP_DISCOVER			0x02
struct btp_csip_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_CSIP_START_ORDERED_ACCESS		0x03
struct btp_csip_start_ordered_access_cmd {
	uint8_t flags;
} __packed;
