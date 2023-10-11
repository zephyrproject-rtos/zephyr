/* btp_ccp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/tbs.h>

/* CCP commands */
#define BTP_CCP_READ_SUPPORTED_COMMANDS	0x01
struct btp_ccp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_CCP_DISCOVER_TBS		0x02
struct btp_ccp_discover_tbs_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_CCP_ACCEPT_CALL		0x03
struct btp_ccp_accept_call_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t call_id;
} __packed;

#define BTP_CCP_TERMINATE_CALL		0x04
struct btp_ccp_terminate_call_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t call_id;
} __packed;

#define BTP_CCP_ORIGINATE_CALL		0x05
struct btp_ccp_originate_call_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t uri_len;
	char    uri[0];
} __packed;

#define BTP_CCP_READ_CALL_STATE		0x06
struct btp_ccp_read_call_state_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

/* CCP events */
#define BTP_CCP_EV_DISCOVERED		0x80
struct btp_ccp_discovered_ev {
	int     status;
	uint8_t tbs_count;
	bool	gtbs_found;
} __packed;

#define BTP_CCP_EV_CALL_STATES		0x81
struct btp_ccp_call_states_ev {
	int     status;
	uint8_t inst_index;
	uint8_t call_count;
	struct bt_tbs_client_call_state call_states[0];
} __packed;
