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

#define BTP_CCP_READ_BEARER_NAME	0x07
struct btp_ccp_read_bearer_name_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_BEARER_UCI		0x08
struct btp_ccp_read_bearer_uci_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_BEARER_TECH	0x09
struct btp_ccp_read_bearer_technology_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_URI_LIST		0x0a
struct btp_ccp_read_uri_list_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_SIGNAL_STRENGTH	0x0b
struct btp_ccp_read_signal_strength_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_SIGNAL_INTERVAL	0x0c
struct btp_ccp_read_signal_interval_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_CURRENT_CALLS	0x0d
struct btp_ccp_read_current_calls_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_CCID		0x0e
struct btp_ccp_read_ccid_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_CALL_URI		0x0f
struct btp_ccp_read_call_uri_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_STATUS_FLAGS	0x10
struct btp_ccp_read_status_flags_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_OPTIONAL_OPCODES	0x11
struct btp_ccp_read_optional_opcodes_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_FRIENDLY_NAME	0x12
struct btp_ccp_read_friendly_name_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_READ_REMOTE_URI		0x13
struct btp_ccp_read_remote_uri_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
} __packed;

#define BTP_CCP_SET_SIGNAL_INTERVAL	0x14
struct btp_ccp_set_signal_interval_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t interval;
} __packed;

#define BTP_CCP_HOLD_CALL		0x15
struct btp_ccp_hold_call_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t call_id;
} __packed;

#define BTP_CCP_RETRIEVE_CALL		0x16
struct btp_ccp_retrieve_call_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t call_id;
} __packed;

#define BTP_CCP_JOIN_CALLS		0x17
struct btp_ccp_join_calls_cmd {
	bt_addr_le_t address;
	uint8_t inst_index;
	uint8_t count;
	uint8_t call_index[];
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

#define BTP_CCP_EV_CHRC_HANDLES		0x82
struct btp_ccp_chrc_handles_ev {
	uint16_t provider_name;
	uint16_t bearer_uci;
	uint16_t bearer_technology;
	uint16_t uri_list;
	uint16_t signal_strength;
	uint16_t signal_interval;
	uint16_t current_calls;
	uint16_t ccid;
	uint16_t status_flags;
	uint16_t bearer_uri;
	uint16_t call_state;
	uint16_t control_point;
	uint16_t optional_opcodes;
	uint16_t termination_reason;
	uint16_t incoming_call;
	uint16_t friendly_name;
};

#define BTP_CCP_EV_CHRC_VAL		0x83
struct btp_ccp_chrc_val_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t inst_index;
	uint8_t value;
};

#define BTP_CCP_EV_CHRC_STR		0x84
struct btp_ccp_chrc_str_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t inst_index;
	uint8_t data_len;
	char data[0];
} __packed;

#define BTP_CCP_EV_CP			0x85
struct btp_ccp_cp_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BTP_CCP_EV_CURRENT_CALLS	0x86
struct btp_ccp_current_calls_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;
