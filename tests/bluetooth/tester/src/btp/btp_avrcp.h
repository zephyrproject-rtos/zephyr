/* btp_avrcp.h - Bluetooth AVRCP Tester */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

#define BTP_AVRCP_READ_SUPPORTED_COMMANDS 0x01
struct btp_avrcp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_AVRCP_CONTROL_CONNECT 0x02
struct btp_avrcp_control_connect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_CONTROL_DISCONNECT 0x03
struct btp_avrcp_control_disconnect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_BROWSING_CONNECT 0x04
struct btp_avrcp_browsing_connect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_BROWSING_DISCONNECT 0x05
struct btp_avrcp_browsing_disconnect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_UNIT_INFO 0x06
struct btp_avrcp_unit_info_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_SUBUNIT_INFO 0x07
struct btp_avrcp_subunit_info_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_PASS_THROUGH 0x08
struct btp_avrcp_pass_through_cmd {
	bt_addr_t address;
	uint8_t opid;
	uint8_t state;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define BTP_AVRCP_GET_CAPS 0x09
struct btp_avrcp_get_caps_cmd {
	bt_addr_t address;
	uint8_t cap_id;
} __packed;

#define BTP_AVRCP_LIST_PLAYER_APP_SETTING_ATTRS 0x0A
struct btp_avrcp_list_player_app_setting_attrs_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_LIST_PLAYER_APP_SETTING_VALS 0x0B
struct btp_avrcp_list_player_app_setting_vals_cmd {
	bt_addr_t address;
	uint8_t attr_id;
} __packed;

#define BTP_AVRCP_GET_CURR_PLAYER_APP_SETTING_VAL 0x0C
struct btp_avrcp_get_curr_player_app_setting_val_cmd {
	bt_addr_t address;
	uint8_t num_attrs;
	uint8_t attr_ids[0];
} __packed;

#define BTP_AVRCP_SET_PLAYER_APP_SETTING_VAL 0x0D
struct btp_avrcp_set_player_app_setting_val_cmd {
	bt_addr_t address;
	uint8_t num_attrs;
	uint8_t attr_vals[0];
} __packed;

#define BTP_AVRCP_GET_PLAYER_APP_SETTING_ATTR_TEXT 0x0E
struct btp_avrcp_get_player_app_setting_attr_text_cmd {
	bt_addr_t address;
	uint8_t num_attrs;
	uint8_t attr_ids[0];
} __packed;

#define BTP_AVRCP_GET_PLAYER_APP_SETTING_VAL_TEXT 0x0F
struct btp_avrcp_get_player_app_setting_val_text_cmd {
	bt_addr_t address;
	uint8_t attr_id;
	uint8_t num_vals;
	uint8_t val_ids[0];
} __packed;

#define BTP_AVRCP_GET_ELEMENT_ATTRS 0x10
struct btp_avrcp_get_element_attrs_cmd {
	bt_addr_t address;
	uint8_t num_attrs;
	uint32_t attr_ids[0];
} __packed;

#define BTP_AVRCP_GET_PLAY_STATUS 0x11
struct btp_avrcp_get_play_status_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_REGISTER_NOTIFICATION 0x12
struct btp_avrcp_register_notification_cmd {
	bt_addr_t address;
	uint8_t event_id;
	uint32_t interval;
} __packed;

#define BTP_AVRCP_REQ_CONTINUING_RSP 0x13
struct btp_avrcp_req_continuing_rsp_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_ABORT_CONTINUING_RSP 0x14
struct btp_avrcp_abort_continuing_rsp_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_SET_ABSOLUTE_VOLUME 0x15
struct btp_avrcp_set_absolute_volume_cmd {
	bt_addr_t address;
	uint8_t volume;
} __packed;

#define BTP_AVRCP_SET_ADDRESSED_PLAYER 0x16
struct btp_avrcp_set_addressed_player_cmd {
	bt_addr_t address;
	uint16_t player_id;
} __packed;

#define BTP_AVRCP_GET_FOLDER_ITEMS 0x17
struct btp_avrcp_get_folder_items_cmd {
	bt_addr_t address;
	uint8_t scope;
	uint32_t start_item;
	uint32_t end_item;
	uint8_t attr_count;
	uint32_t attr_ids[0];
} __packed;

#define BTP_AVRCP_GET_TOTAL_NUMBER_OF_ITEMS 0x18
struct btp_avrcp_get_total_number_of_items_cmd {
	bt_addr_t address;
	uint8_t scope;
} __packed;

#define BTP_AVRCP_SET_BROWSED_PLAYER 0x19
struct btp_avrcp_set_browsed_player_cmd {
	bt_addr_t address;
	uint16_t player_id;
} __packed;

#define BTP_AVRCP_CHANGE_PATH 0x1A
struct btp_avrcp_change_path_cmd {
	bt_addr_t address;
	uint16_t uid_counter;
	uint8_t direction;
	uint8_t folder_uid[8];
} __packed;

#define BTP_AVRCP_GET_ITEM_ATTRS 0x1B
struct btp_avrcp_get_item_attrs_cmd {
	bt_addr_t address;
	uint8_t scope;
	uint8_t uid[8];
	uint16_t uid_counter;
	uint8_t num_attrs;
	uint32_t attr_ids[0];
} __packed;

#define BTP_AVRCP_PLAY_ITEM 0x1C
struct btp_avrcp_play_item_cmd {
	bt_addr_t address;
	uint8_t scope;
	uint8_t uid[8];
	uint16_t uid_counter;
} __packed;

#define BTP_AVRCP_SEARCH 0x1D
struct btp_avrcp_search_cmd {
	bt_addr_t address;
	uint16_t str_len;
	uint8_t str[0];
} __packed;

#define BTP_AVRCP_ADD_TO_NOW_PLAYING 0x1E
struct btp_avrcp_add_to_now_playing_cmd {
	bt_addr_t address;
	uint8_t scope;
	uint8_t uid[8];
	uint16_t uid_counter;
} __packed;

#define BTP_AVRCP_TG_REGISTER_NOTIFICATION 0x1F
struct btp_avrcp_tg_register_notification_cmd {
	bt_addr_t address;
	uint8_t event_id;
} __packed;

#define BTP_AVRCP_TG_CONTROL_PLAYBACK 0x20
struct btp_avrcp_tg_control_playback_cmd {
	uint8_t action;
	uint8_t long_metadata;
	uint8_t cover_art;
} __packed;

#define BTP_AVRCP_TG_CHANGE_PATH 0x21
struct btp_avrcp_tg_change_path_cmd {
	uint8_t direction;
	uint8_t folder_name_len;
	uint8_t folder_name[0];
} __packed;

#define BTP_AVRCP_CA_CT_CONNECT 0x22
struct btp_avrcp_ca_ct_connect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_CA_CT_DISCONNECT 0x23
struct btp_avrcp_ca_ct_disconnect_cmd {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_CA_CT_GET_IMAGE_PROPS 0x24
struct btp_avrcp_ct_get_image_props_cmd {
	bt_addr_t address;
	uint8_t image_handle[7];
} __packed;

#define BTP_AVRCP_CA_CT_GET_IMAGE 0x25
struct btp_avrcp_ct_get_image_cmd {
	bt_addr_t address;
	uint8_t image_handle[7];
	uint16_t image_desc_len;
	uint8_t image_desc[0];
} __packed;

#define BTP_AVRCP_CA_CT_GET_LINKED_THUMBNAIL 0x26
struct btp_avrcp_ct_get_linked_thumbnail_cmd {
	bt_addr_t address;
	uint8_t image_handle[7];
} __packed;

#define BTP_AVRCP_EV_CONTROL_CONNECTED 0x80
struct btp_avrcp_control_connected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_CONTROL_DISCONNECTED 0x81
struct btp_avrcp_control_disconnected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_BROWSING_CONNECTED 0x82
struct btp_avrcp_browsing_connected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_BROWSING_DISCONNECTED 0x83
struct btp_avrcp_browsing_disconnected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_UNIT_INFO_RSP 0x84
struct btp_avrcp_unit_info_rsp_ev {
	bt_addr_t address;
	uint8_t unit_type;
	uint32_t company_id;
} __packed;

#define BTP_AVRCP_EV_SUBUNIT_INFO_RSP 0x85
struct btp_avrcp_subunit_info_rsp_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_PASS_THROUGH_RSP 0x86
struct btp_avrcp_pass_through_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t opid_state; /**< [7]: state_flag, [6:0]: opid */
	uint8_t len;
	uint8_t data[0];
} __packed;

#define BTP_AVRCP_EV_GET_CAPS_RSP 0x87
struct btp_avrcp_get_caps_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t cap_id;
	uint8_t cap_cnt;
	uint8_t cap[];
} __packed;

#define BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_ATTRS_RSP 0x88
struct btp_avrcp_list_player_app_setting_attrs_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_attrs;
	uint8_t attr_ids[0];
} __packed;

#define BTP_AVRCP_EV_LIST_PLAYER_APP_SETTING_VALS_RSP 0x89
struct btp_avrcp_list_player_app_setting_vals_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_vals;
	uint8_t val_ids[0];
} __packed;

#define BTP_AVRCP_EV_GET_CURR_PLAYER_APP_SETTING_VAL_RSP 0x8A
struct btp_avrcp_get_curr_player_app_setting_val_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_attrs;
	uint8_t attr_vals[0];
} __packed;

#define BTP_AVRCP_EV_SET_PLAYER_APP_SETTING_VAL_RSP 0x8B
struct btp_avrcp_set_player_app_setting_val_rsp_ev {
	bt_addr_t address;
	uint8_t status;
} __packed;

#define BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_ATTR_TEXT_RSP 0x8C
struct btp_avrcp_get_player_app_setting_attr_text_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_attrs;
	uint8_t attrs[0];
} __packed;

#define BTP_AVRCP_EV_GET_PLAYER_APP_SETTING_VAL_TEXT_RSP 0x8D
struct btp_avrcp_get_player_app_setting_val_text_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_vals;
	uint8_t vals[0];
} __packed;

#define BTP_AVRCP_EV_GET_ELEMENT_ATTRS_RSP 0x8E
struct btp_avrcp_get_element_attrs_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_attrs;
	uint8_t attrs[0];
} __packed;

#define BTP_AVRCP_EV_GET_PLAY_STATUS_RSP 0x8F
struct btp_avrcp_get_play_status_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint32_t song_len;
	uint32_t song_pos;
	uint8_t play_status;
} __packed;

#define BTP_AVRCP_EV_REGISTER_NOTIFICATION_RSP 0x90
struct btp_avrcp_register_notification_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t event_id;
	uint8_t data[0];
} __packed;

#define BTP_AVRCP_EV_REQ_CONTINUING_RSP_RSP 0x91
struct btp_avrcp_req_continuing_rsp_rsp_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_ABORT_CONTINUING_RSP_RSP 0x92
struct btp_avrcp_abort_continuing_rsp_rsp_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_SET_ABSOLUTE_VOLUME_RSP 0x93
struct btp_avrcp_set_absolute_volume_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t volume;
} __packed;

#define BTP_AVRCP_EV_SET_ADDRESSED_PLAYER_RSP 0x94
struct btp_avrcp_set_addressed_player_rsp_ev {
	bt_addr_t address;
	uint8_t status;
} __packed;

#define BTP_AVRCP_EV_GET_FOLDER_ITEMS_RSP 0x95
struct btp_avrcp_get_folder_items_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint16_t uid_counter;
	uint16_t num_items;
	uint8_t items[0];
} __packed;

#define BTP_AVRCP_EV_GET_TOTAL_NUMBER_OF_ITEMS_RSP 0x96
struct btp_avrcp_get_total_number_of_items_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint16_t uid_counter;
	uint32_t num_items;
} __packed;

#define BTP_AVRCP_EV_SET_BROWSED_PLAYER_RSP 0x97
struct btp_avrcp_set_browsed_player_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint16_t uid_counter;
	uint32_t num_items;
	uint16_t charset_id;
	uint8_t folder_depth;
	uint8_t folder_names[0];
} __packed;

#define BTP_AVRCP_EV_CHANGE_PATH_RSP 0x98
struct btp_avrcp_change_path_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint32_t num_items;
} __packed;

#define BTP_AVRCP_EV_GET_ITEM_ATTRS_RSP 0x99
struct btp_avrcp_get_item_attrs_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint8_t num_attrs;
	uint8_t attrs[0];
} __packed;

#define BTP_AVRCP_EV_PLAY_ITEM_RSP 0x9A
struct btp_avrcp_play_item_rsp_ev {
	bt_addr_t address;
	uint8_t status;
} __packed;

#define BTP_AVRCP_EV_SEARCH_RSP 0x9B
struct btp_avrcp_search_rsp_ev {
	bt_addr_t address;
	uint8_t status;
	uint16_t uid_counter;
	uint32_t num_items;
} __packed;

#define BTP_AVRCP_EV_ADD_TO_NOW_PLAYING_RSP 0x9C
struct btp_avrcp_add_to_now_playing_rsp_ev {
	bt_addr_t address;
	uint8_t status;
} __packed;

#define BTP_AVRCP_EV_CA_CT_CONNECTED 0x9D
struct btp_avrcp_ca_ct_connected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_CA_CT_DISCONNECTED 0x9E
struct btp_avrcp_ca_ct_disconnected_ev {
	bt_addr_t address;
} __packed;

#define BTP_AVRCP_EV_GET_IMAGE_PROPS_RSP 0x9F
struct btp_avrcp_get_image_props_rsp_ev {
	bt_addr_t address;
	uint8_t rsp_code;
	uint16_t body_len;
	uint8_t body[0];
} __packed;

#define BTP_AVRCP_EV_GET_IMAGE_RSP 0xA0
struct btp_avrcp_get_image_rsp_ev {
	bt_addr_t address;
	uint8_t rsp_code;
	uint16_t body_len;
	uint8_t body[0];
} __packed;

#define BTP_AVRCP_EV_GET_LINKED_THUMBNAIL_RSP 0xA1
struct btp_avrcp_get_linked_thumbnail_rsp_ev {
	bt_addr_t address;
	uint8_t rsp_code;
	uint16_t body_len;
	uint8_t body[0];
} __packed;
