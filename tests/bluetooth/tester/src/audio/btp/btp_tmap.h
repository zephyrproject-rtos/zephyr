/* btp_tmap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2024 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>

/* TMAP commands */
#define BTP_TMAP_READ_SUPPORTED_COMMANDS	0x01
struct btp_tmap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_TMAP_DISCOVER			0x02
struct btp_tmap_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BT_TMAP_EV_DISCOVERY_COMPLETE		0x80
struct btp_tmap_discovery_complete_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint16_t role;
} __packed;
