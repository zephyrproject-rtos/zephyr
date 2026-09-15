/* btp_spp.h - Bluetooth SPP tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BTP_SPP_H
#define __BTP_SPP_H

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* SPP Commands */
#define BTP_SPP_READ_SUPPORTED_COMMANDS    0x01
struct btp_spp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_SPP_CMD_DISCOVER               0x02
struct btp_spp_discover_cmd {
	uint8_t address_type;
	bt_addr_t address;
} __packed;

#define BTP_SPP_CMD_REGISTER_SERVER         0x03
struct btp_spp_register_server_cmd {
	uint8_t channel;
} __packed;

/* SPP Events */
#define BTP_SPP_EV_DISCOVERED              0x80
struct btp_spp_discover_ev {
	bt_addr_t address;
	uint8_t channel;
} __packed;

#endif /* __BTP_SPP_H */
