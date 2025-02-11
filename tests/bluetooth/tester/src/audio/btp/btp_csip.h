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

#define BTP_CSIP_SET_COORDINATOR_LOCK		0x04
struct btp_csip_set_coordinator_lock_cmd {
	uint8_t addr_cnt;
	bt_addr_le_t addr[];
} __packed;

#define BTP_CSIP_SET_COORDINATOR_RELEASE	0x05
struct btp_csip_set_coordinator_release_cmd {
	uint8_t addr_cnt;
	bt_addr_le_t addr[];
} __packed;

/* CSIP Events */
#define BTP_CSIP_DISCOVERED_EV			0x80
struct btp_csip_discovered_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint16_t sirk_handle;
	uint16_t size_handle;
	uint16_t lock_handle;
	uint16_t rank_handle;
} __packed;

#define BTP_CSIP_SIRK_EV			0x81
struct btp_csip_sirk_ev {
	bt_addr_le_t address;
	uint8_t sirk[BT_CSIP_SIRK_SIZE];
} __packed;

#define BTP_CSIP_LOCK_EV			0x82
struct btp_csip_lock_ev {
	uint8_t status;
} __packed;
