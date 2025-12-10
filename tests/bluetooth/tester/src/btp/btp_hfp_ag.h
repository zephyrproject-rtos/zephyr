/* btp_hfp_ag.h - Bluetooth HFP AG Tester */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* HFP AG Service */
/* Commands */
#define BTP_HFP_AG_READ_SUPPORTED_COMMANDS	0x01
struct btp_hfp_ag_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HFP_AG_CONNECT			0x02
struct btp_hfp_ag_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_HFP_AG_DISCONNECT			0x03
struct btp_hfp_ag_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_REMOTE_INCOMING		0x04
struct btp_hfp_ag_remote_incoming_cmd {
	bt_addr_le_t address;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_AG_OUTGOING			0x05
struct btp_hfp_ag_outgoing_cmd {
	bt_addr_le_t address;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_AG_REMOTE_RINGING		0x06
struct btp_hfp_ag_remote_ringing_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_REMOTE_ACCEPT		0x07
struct btp_hfp_ag_remote_accept_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_REMOTE_REJECT		0x08
struct btp_hfp_ag_remote_reject_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_REMOTE_TERMINATE		0x09
struct btp_hfp_ag_remote_terminate_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_ACCEPT_CALL			0x0a
struct btp_hfp_ag_accept_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_REJECT_CALL			0x0b
struct btp_hfp_ag_reject_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_TERMINATE_CALL		0x0c
struct btp_hfp_ag_terminate_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_HOLD_CALL			0x0d
struct btp_hfp_ag_hold_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_RETRIEVE_CALL		0x0e
struct btp_hfp_ag_retrieve_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_HOLD_INCOMING		0x0f
struct btp_hfp_ag_hold_incoming_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EXPLICIT_CALL_TRANSFER	0x10
struct btp_hfp_ag_explicit_call_transfer_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_AUDIO_CONNECT		0x11
struct btp_hfp_ag_audio_connect_cmd {
	bt_addr_le_t address;
	uint8_t codec_id;
} __packed;

#define BTP_HFP_AG_AUDIO_DISCONNECT		0x12
struct btp_hfp_ag_audio_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_SET_VGM			0x13
struct btp_hfp_ag_set_vgm_cmd {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_AG_SET_VGS			0x14
struct btp_hfp_ag_set_vgs_cmd {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_AG_SET_OPERATOR			0x15
struct btp_hfp_ag_set_operator_cmd {
	bt_addr_le_t address;
	uint8_t mode;
	uint8_t name_len;
	uint8_t name[0];
} __packed;

#define BTP_HFP_AG_SET_INBAND_RINGTONE		0x16
struct btp_hfp_ag_set_inband_ringtone_cmd {
	bt_addr_le_t address;
	uint8_t enable;
} __packed;

#define BTP_HFP_AG_VOICE_RECOGNITION		0x17
struct btp_hfp_ag_voice_recognition_cmd {
	bt_addr_le_t address;
	uint8_t activate;
} __packed;

#define BTP_HFP_AG_VRE_STATE			0x18
struct btp_hfp_ag_vre_state_cmd {
	bt_addr_le_t address;
	uint8_t state;
} __packed;

#define BTP_HFP_AG_VRE_TEXT			0x19
struct btp_hfp_ag_vre_text_cmd {
	bt_addr_le_t address;
	uint8_t state;
	uint16_t text_id;
	uint8_t text_type;
	uint8_t text_operation;
	uint8_t text_len;
	uint8_t text[0];
} __packed;

#define BTP_HFP_AG_SET_SIGNAL_STRENGTH		0x1a
struct btp_hfp_ag_set_signal_strength_cmd {
	bt_addr_le_t address;
	uint8_t strength;
} __packed;

#define BTP_HFP_AG_SET_ROAMING_STATUS		0x1b
struct btp_hfp_ag_set_roaming_status_cmd {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BTP_HFP_AG_SET_BATTERY_LEVEL		0x1c
struct btp_hfp_ag_set_battery_level_cmd {
	bt_addr_le_t address;
	uint8_t level;
} __packed;

#define BTP_HFP_AG_SET_SERVICE_AVAILABILITY	0x1d
struct btp_hfp_ag_set_service_availability_cmd {
	bt_addr_le_t address;
	uint8_t available;
} __packed;

#define BTP_HFP_AG_SET_HF_INDICATOR		0x1e
struct btp_hfp_ag_set_hf_indicator_cmd {
	bt_addr_le_t address;
	uint8_t indicator;
	uint8_t enable;
} __packed;

#define BTP_HFP_AG_SET_ONGOING_CALLS		0x1f
struct btp_hfp_ag_ongoing_call_info {
	uint8_t number_len;
	uint8_t type;
	uint8_t dir;
	uint8_t status;
	uint8_t number[0];
} __packed;
struct btp_hfp_ag_set_ongoing_calls_cmd {
	uint8_t count;
	struct btp_hfp_ag_ongoing_call_info calls[0];
} __packed;

#define BTP_HFP_AG_SET_LAST_NUMBER		0x20
struct btp_hfp_ag_set_last_number_cmd {
	uint8_t type;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_AG_SET_DEFAULT_INDICATOR_VALUE	0x21
struct btp_hfp_ag_set_default_indicator_value_cmd {
	uint8_t service;
	uint8_t signal;
	uint8_t roam;
	uint8_t battery;
} __packed;

#define BTP_HFP_AG_SET_MEMORY_DIAL_MAPPING	0x22
struct btp_hfp_ag_set_memory_dial_mapping_cmd {
	uint8_t location_len;
	uint8_t number_len;
	uint8_t data[0]; /* location string followed by number string */
} __packed;

#define BTP_HFP_AG_SET_SUBSCRIBER_NUMBER	0x23
struct btp_hfp_ag_subscriber_number_info {
	uint8_t type;
	uint8_t service;
	uint8_t number_len;
	uint8_t number[0];
} __packed;
struct btp_hfp_ag_set_subscriber_number_cmd {
	uint8_t count;
	struct btp_hfp_ag_subscriber_number_info numbers[0];
} __packed;

#define BTP_HFP_AG_SET_VOICE_TAG_NUMBER		0x24
struct btp_hfp_ag_set_voice_tag_number_cmd {
	uint8_t number_len;
	uint8_t number[0];
} __packed;

/* Events */
#define BTP_HFP_AG_EV_CONNECTED			0x80
struct btp_hfp_ag_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_DISCONNECTED		0x81
struct btp_hfp_ag_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_SCO_CONNECTED		0x82
struct btp_hfp_ag_sco_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_SCO_DISCONNECTED		0x83
struct btp_hfp_ag_sco_disconnected_ev {
	bt_addr_le_t address;
	uint8_t reason;
} __packed;

#define BTP_HFP_AG_EV_OUTGOING			0x84
struct btp_hfp_ag_outgoing_ev {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_AG_EV_INCOMING			0x85
struct btp_hfp_ag_incoming_ev {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_AG_EV_INCOMING_HELD		0x86
struct btp_hfp_ag_incoming_held_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_RINGING			0x87
struct btp_hfp_ag_ringing_ev {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t in_band;
} __packed;

#define BTP_HFP_AG_EV_CALL_ACCEPTED		0x88
struct btp_hfp_ag_call_accepted_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_CALL_HELD			0x89
struct btp_hfp_ag_call_held_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_CALL_RETRIEVED		0x8a
struct btp_hfp_ag_call_retrieved_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_CALL_REJECTED		0x8b
struct btp_hfp_ag_call_rejected_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_CALL_TERMINATED		0x8c
struct btp_hfp_ag_call_terminated_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_AG_EV_CODEC_IDS			0x8d
struct btp_hfp_ag_codec_ids_ev {
	bt_addr_le_t address;
	uint32_t codec_ids;
} __packed;

#define BTP_HFP_AG_EV_CODEC_NEGOTIATED		0x8e
struct btp_hfp_ag_codec_negotiated_ev {
	bt_addr_le_t address;
	uint8_t codec_id;
	int8_t result;
} __packed;

#define BTP_HFP_AG_EV_AUDIO_CONNECT_REQ		0x8f
struct btp_hfp_ag_audio_connect_req_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_VGM			0x90
struct btp_hfp_ag_vgm_ev {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_AG_EV_VGS			0x91
struct btp_hfp_ag_vgs_ev {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_AG_EV_ECNR_TURN_OFF		0x92
struct btp_hfp_ag_ecnr_turn_off_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_EXPLICIT_CALL_TRANSFER	0x93
struct btp_hfp_ag_explicit_call_transfer_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_VOICE_RECOGNITION		0x94
struct btp_hfp_ag_voice_recognition_ev {
	bt_addr_le_t address;
	uint8_t activate;
} __packed;

#define BTP_HFP_AG_EV_READY_ACCEPT_AUDIO	0x95
struct btp_hfp_ag_ready_accept_audio_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_AG_EV_TRANSMIT_DTMF_CODE	0x96
struct btp_hfp_ag_transmit_dtmf_code_ev {
	bt_addr_le_t address;
	uint8_t code;
} __packed;

#define BTP_HFP_AG_EV_HF_INDICATOR_VALUE	0x97
struct btp_hfp_ag_hf_indicator_value_ev {
	bt_addr_le_t address;
	uint8_t indicator;
	uint32_t value;
} __packed;
