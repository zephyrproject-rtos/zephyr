/* btp_mcp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/services/ots.h>

/* MCP commands */
#define BTP_MCP_READ_SUPPORTED_COMMANDS		0x01
struct btp_mcp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_MCP_DISCOVER			0x02
struct btp_mcp_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_TRACK_DURATION_READ		0x03
struct btp_mcp_track_duration_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_TRACK_POSITION_READ		0x04
struct btp_mcp_track_position_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_TRACK_POSITION_SET		0x05
struct btp_mcp_track_position_set_cmd {
	bt_addr_le_t address;
	int32_t pos;
} __packed;

#define BTP_MCP_PLAYBACK_SPEED_READ		0x06
struct btp_mcp_playback_speed_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_PLAYBACK_SPEED_SET		0x07
struct btp_mcp_playback_speed_set {
	bt_addr_le_t address;
	int8_t speed;
} __packed;

#define BTP_MCP_SEEKING_SPEED_READ		0x08
struct btp_mcp_seeking_speed_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_ICON_OBJ_ID_READ		0x09
struct btp_mcp_icon_obj_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_NEXT_TRACK_OBJ_ID_READ		0x0a
struct btp_mcp_next_track_obj_id_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_NEXT_TRACK_OBJ_ID_SET		0x0b
struct btp_mcp_set_next_track_obj_id_cmd {
	bt_addr_le_t address;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_PARENT_GROUP_OBJ_ID_READ	0x0c
struct btp_mcp_parent_group_obj_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_CURRENT_GROUP_OBJ_ID_READ	0x0d
struct btp_mcp_current_group_obj_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_CURRENT_GROUP_OBJ_ID_SET	0x0e
struct btp_mcp_current_group_obj_id_set_cmd {
	bt_addr_le_t address;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_PLAYING_ORDER_READ		0x0f
struct btp_mcp_playing_order_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_PLAYING_ORDER_SET		0x10
struct btp_mcp_playing_order_set_cmd {
	bt_addr_le_t address;
	uint8_t order;
} __packed;

#define BTP_MCP_PLAYING_ORDERS_SUPPORTED_READ	0x11
struct btp_mcp_playing_orders_supported_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_MEDIA_STATE_READ		0x12
struct btp_mcp_media_state_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_OPCODES_SUPPORTED_READ		0x13
struct btp_mcp_opcodes_supported_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_CONTENT_CONTROL_ID_READ		0x14
struct btp_mcp_content_control_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_SEGMENTS_OBJ_ID_READ		0x15
struct btp_mcp_segments_obj_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_CURRENT_TRACK_OBJ_ID_READ	0x16
struct btp_mcp_current_track_obj_id_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MCP_CURRENT_TRACK_OBJ_ID_SET	0x17
struct btp_mcp_current_track_obj_id_set_cmd {
	bt_addr_le_t address;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_CMD_SEND			0x18
struct btp_mcp_send_cmd {
	bt_addr_le_t address;
	uint8_t  opcode;
	uint8_t use_param;
	int32_t param;
} __packed;

#define BTP_MCP_CMD_SEARCH			0x19
struct btp_mcp_search_cmd {
	bt_addr_le_t address;
	uint8_t type;
	uint8_t param_len;
	uint8_t param[];
} __packed;

/* MCP events */
#define BTP_MCP_DISCOVERED_EV			0x80
struct btp_mcp_discovered_ev {
	bt_addr_le_t address;
	uint8_t status;
	struct {
		uint16_t player_name;
		uint16_t icon_obj_id;
		uint16_t icon_url;
		uint16_t track_changed;
		uint16_t track_title;
		uint16_t track_duration;
		uint16_t track_position;
		uint16_t playback_speed;
		uint16_t seeking_speed;
		uint16_t segments_obj_id;
		uint16_t current_track_obj_id;
		uint16_t next_track_obj_id;
		uint16_t current_group_obj_id;
		uint16_t parent_group_obj_id;
		uint16_t playing_order;
		uint16_t playing_orders_supported;
		uint16_t media_state;
		uint16_t cp;
		uint16_t opcodes_supported;
		uint16_t search_results_obj_id;
		uint16_t scp;
		uint16_t content_control_id;
	} gmcs_handles;

	struct {
		uint16_t feature;
		uint16_t obj_name;
		uint16_t obj_type;
		uint16_t obj_size;
		uint16_t obj_id;
		uint16_t obj_created;
		uint16_t obj_modified;
		uint16_t obj_properties;
		uint16_t oacp;
		uint16_t olcp;
	} ots_handles;
} __packed;

#define BTP_MCP_TRACK_DURATION_EV		0x81
struct btp_mcp_track_duration_ev {
	bt_addr_le_t address;
	uint8_t status;
	int32_t dur;
} __packed;

#define BTP_MCP_TRACK_POSITION_EV		0x82
struct btp_mcp_track_position_ev {
	bt_addr_le_t address;
	uint8_t status;
	int32_t pos;
} __packed;

#define BTP_MCP_PLAYBACK_SPEED_EV		0x83
struct btp_mcp_playback_speed_ev {
	bt_addr_le_t address;
	uint8_t status;
	int8_t speed;
} __packed;

#define BTP_MCP_SEEKING_SPEED_EV		0x84
struct btp_mcp_seeking_speed_ev {
	bt_addr_le_t address;
	uint8_t status;
	int8_t speed;
} __packed;

#define BTP_MCP_ICON_OBJ_ID_EV			0x85
struct btp_mcp_icon_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_NEXT_TRACK_OBJ_ID_EV		0x86
struct btp_mcp_next_track_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_PARENT_GROUP_OBJ_ID_EV		0x87
struct btp_mcp_parent_group_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_CURRENT_GROUP_OBJ_ID_EV		0x88
struct btp_mcp_current_group_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_PLAYING_ORDER_EV		0x89
struct btp_mcp_playing_order_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t order;
} __packed;

#define BTP_MCP_PLAYING_ORDERS_SUPPORTED_EV	0x8a
struct btp_mcp_playing_orders_supported_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint16_t orders;
} __packed;

#define BTP_MCP_MEDIA_STATE_EV			0x8b
struct btp_mcp_media_state_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t state;
} __packed;

#define BTP_MCP_OPCODES_SUPPORTED_EV		0x8c
struct btp_mcp_opcodes_supported_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint32_t opcodes;
} __packed;

#define BTP_MCP_CONTENT_CONTROL_ID_EV		0x8d
struct btp_mcp_content_control_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t ccid;
} __packed;

#define BTP_MCP_SEGMENTS_OBJ_ID_EV		0x8e
struct btp_mcp_segments_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_CURRENT_TRACK_OBJ_ID_EV		0x8f
struct btp_mcp_current_track_obj_id_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCP_MEDIA_CP_EV			0x90
struct btp_mcp_media_cp_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t opcode;
	bool  use_param;
	int32_t param;
} __packed;

#define BTP_MCP_SEARCH_CP_EV			0x91
struct btp_mcp_search_cp_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t param_len;
	uint8_t search_type;
	uint8_t param[];
} __packed;

#define BTP_MCP_NTF_EV				0x92
struct btp_mcp_cmd_ntf_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t requested_opcode;
	uint8_t result_code;
} __packed;

#define BTP_SCP_NTF_EV				0x93
struct btp_scp_cmd_ntf_ev {
	bt_addr_le_t address;
	uint8_t status;
	uint8_t result_code;
} __packed;
