/* btp_hfp_hf.h - Bluetooth HFP HF Tester */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* HFP HF Service */
/* Commands */
#define BTP_HFP_HF_READ_SUPPORTED_COMMANDS	0x01
struct btp_hfp_hf_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HFP_HF_CONNECT			0x02
struct btp_hfp_hf_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_HFP_HF_DISCONNECT			0x03
struct btp_hfp_hf_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_CLI				0x04
struct btp_hfp_hf_cli_cmd {
	bt_addr_le_t address;
	uint8_t enable;
} __packed;

#define BTP_HFP_HF_VGM				0x05
struct btp_hfp_hf_vgm_cmd {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_HF_VGS				0x06
struct btp_hfp_hf_vgs_cmd {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_HF_GET_OPERATOR			0x07
struct btp_hfp_hf_get_operator_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_ACCEPT_CALL			0x08
struct btp_hfp_hf_accept_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_REJECT_CALL			0x09
struct btp_hfp_hf_reject_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_TERMINATE_CALL		0x0a
struct btp_hfp_hf_terminate_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_HOLD_INCOMING		0x0b
struct btp_hfp_hf_hold_incoming_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_QUERY_RESPOND_HOLD_STATUS	0x0c
struct btp_hfp_hf_query_respond_hold_status_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_NUMBER_CALL			0x0d
struct btp_hfp_hf_number_call_cmd {
	bt_addr_le_t address;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_HF_MEMORY_DIAL			0x0e
struct btp_hfp_hf_memory_dial_cmd {
	bt_addr_le_t address;
	uint8_t location_len;
	uint8_t location[0];
} __packed;

#define BTP_HFP_HF_REDIAL			0x0f
struct btp_hfp_hf_redial_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_AUDIO_CONNECT		0x10
struct btp_hfp_hf_audio_connect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_AUDIO_DISCONNECT		0x11
struct btp_hfp_hf_audio_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_SELECT_CODEC			0x12
struct btp_hfp_hf_select_codec_cmd {
	bt_addr_le_t address;
	uint8_t codec_id;
} __packed;

#define BTP_HFP_HF_SET_CODECS			0x13
struct btp_hfp_hf_set_codecs_cmd {
	bt_addr_le_t address;
	uint8_t codec_ids;
} __packed;

#define BTP_HFP_HF_TURN_OFF_ECNR		0x14
struct btp_hfp_hf_turn_off_ecnr_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_CALL_WAITING_NOTIFY		0x15
struct btp_hfp_hf_call_waiting_notify_cmd {
	bt_addr_le_t address;
	uint8_t enable;
} __packed;

#define BTP_HFP_HF_RELEASE_ALL_HELD		0x16
struct btp_hfp_hf_release_all_held_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_SET_UDUB			0x17
struct btp_hfp_hf_set_udub_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_RELEASE_ACTIVE_ACCEPT_OTHER	0x18
struct btp_hfp_hf_release_active_accept_other_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_HOLD_ACTIVE_ACCEPT_OTHER	0x19
struct btp_hfp_hf_hold_active_accept_other_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_JOIN_CONVERSATION		0x1a
struct btp_hfp_hf_join_conversation_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_EXPLICIT_CALL_TRANSFER	0x1b
struct btp_hfp_hf_explicit_call_transfer_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_RELEASE_SPECIFIED_CALL	0x1c
struct btp_hfp_hf_release_specified_call_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_PRIVATE_CONSULTATION_MODE	0x1d
struct btp_hfp_hf_private_consultation_mode_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_VOICE_RECOGNITION		0x1e
struct btp_hfp_hf_voice_recognition_cmd {
	bt_addr_le_t address;
	uint8_t activate;
} __packed;

#define BTP_HFP_HF_READY_TO_ACCEPT_AUDIO	0x1f
struct btp_hfp_hf_ready_to_accept_audio_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_REQUEST_PHONE_NUMBER		0x20
struct btp_hfp_hf_request_phone_number_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_TRANSMIT_DTMF_CODE		0x21
struct btp_hfp_hf_transmit_dtmf_code_cmd {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t code;
} __packed;

#define BTP_HFP_HF_QUERY_SUBSCRIBER		0x22
struct btp_hfp_hf_query_subscriber_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_INDICATOR_STATUS		0x23
struct btp_hfp_hf_indicator_status_cmd {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BTP_HFP_HF_ENHANCED_SAFETY		0x24
struct btp_hfp_hf_enhanced_safety_cmd {
	bt_addr_le_t address;
	uint8_t enable;
} __packed;

#define BTP_HFP_HF_BATTERY			0x25
struct btp_hfp_hf_battery_cmd {
	bt_addr_le_t address;
	uint8_t level;
} __packed;

#define BTP_HFP_HF_QUERY_LIST_CURRENT_CALLS	0x26
struct btp_hfp_hf_query_list_current_calls_cmd {
	bt_addr_le_t address;
} __packed;

/* Events */
#define BTP_HFP_HF_EV_CONNECTED			0x80
struct btp_hfp_hf_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_EV_DISCONNECTED		0x81
struct btp_hfp_hf_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_EV_SCO_CONNECTED		0x82
struct btp_hfp_hf_sco_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_HFP_HF_EV_SCO_DISCONNECTED		0x83
struct btp_hfp_hf_sco_disconnected_ev {
	bt_addr_le_t address;
	uint8_t reason;
} __packed;

#define BTP_HFP_HF_EV_SERVICE			0x84
struct btp_hfp_hf_service_ev {
	bt_addr_le_t address;
	uint32_t value;
} __packed;

#define BTP_HFP_HF_EV_OUTGOING			0x85
struct btp_hfp_hf_outgoing_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_REMOTE_RINGING		0x86
struct btp_hfp_hf_remote_ringing_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_INCOMING			0x87
struct btp_hfp_hf_incoming_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_INCOMING_HELD		0x88
struct btp_hfp_hf_incoming_held_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_CALL_ACCEPTED		0x89
struct btp_hfp_hf_call_accepted_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_CALL_REJECTED		0x8a
struct btp_hfp_hf_call_rejected_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_CALL_TERMINATED		0x8b
struct btp_hfp_hf_call_terminated_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_CALL_HELD			0x8c
struct btp_hfp_hf_call_held_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_CALL_RETRIEVED		0x8d
struct btp_hfp_hf_call_retrieved_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_SIGNAL			0x8e
struct btp_hfp_hf_signal_ev {
	bt_addr_le_t address;
	uint32_t value;
} __packed;

#define BTP_HFP_HF_EV_ROAM			0x8f
struct btp_hfp_hf_roam_ev {
	bt_addr_le_t address;
	uint32_t value;
} __packed;

#define BTP_HFP_HF_EV_BATTERY			0x90
struct btp_hfp_hf_battery_ev {
	bt_addr_le_t address;
	uint32_t value;
} __packed;

#define BTP_HFP_HF_EV_RING_INDICATION		0x91
struct btp_hfp_hf_ring_indication_ev {
	bt_addr_le_t address;
	uint8_t call_index;
} __packed;

#define BTP_HFP_HF_EV_DIALING			0x92
struct btp_hfp_hf_dialing_ev {
	bt_addr_le_t address;
	int8_t result;
} __packed;

#define BTP_HFP_HF_EV_CLIP			0x93
struct btp_hfp_hf_clip_ev {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t type;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_HF_EV_VGM			0x94
struct btp_hfp_hf_vgm_ev {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_HF_EV_VGS			0x95
struct btp_hfp_hf_vgs_ev {
	bt_addr_le_t address;
	uint8_t gain;
} __packed;

#define BTP_HFP_HF_EV_INBAND_RING		0x96
struct btp_hfp_hf_inband_ring_ev {
	bt_addr_le_t address;
	uint8_t inband;
} __packed;

#define BTP_HFP_HF_EV_OPERATOR			0x97
struct btp_hfp_hf_operator_ev {
	bt_addr_le_t address;
	uint8_t mode;
	uint8_t format;
	uint8_t operator_len;
	uint8_t operator_name[0];
} __packed;

#define BTP_HFP_HF_EV_CODEC_NEGOTIATE		0x98
struct btp_hfp_hf_codec_negotiate_ev {
	bt_addr_le_t address;
	uint8_t codec_id;
} __packed;

#define BTP_HFP_HF_EV_ECNR_TURN_OFF		0x99
struct btp_hfp_hf_ecnr_turn_off_ev {
	bt_addr_le_t address;
	int8_t result;
} __packed;

#define BTP_HFP_HF_EV_CALL_WAITING		0x9a
struct btp_hfp_hf_call_waiting_ev {
	bt_addr_le_t address;
	uint8_t call_index;
	uint8_t type;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_HF_EV_VOICE_RECOGNITION		0x9b
struct btp_hfp_hf_voice_recognition_ev {
	bt_addr_le_t address;
	uint8_t activate;
} __packed;

#define BTP_HFP_HF_EV_VRE_STATE			0x9c
struct btp_hfp_hf_vre_state_ev {
	bt_addr_le_t address;
	uint8_t state;
} __packed;

#define BTP_HFP_HF_EV_TEXTUAL_REPRESENTATION	0x9d
struct btp_hfp_hf_textual_representation_ev {
	bt_addr_le_t address;
	uint8_t type;
	uint8_t operation;
	uint8_t id_len;
	uint8_t text_len;
	uint8_t data[0];  /* id_string followed by text_string */
} __packed;

#define BTP_HFP_HF_EV_REQUEST_PHONE_NUMBER	0x9e
struct btp_hfp_hf_request_phone_number_ev {
	bt_addr_le_t address;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_HF_EV_SUBSCRIBER_NUMBER		0x9f
struct btp_hfp_hf_subscriber_number_ev {
	bt_addr_le_t address;
	uint8_t type;
	uint8_t service;
	uint8_t number_len;
	uint8_t number[0];
} __packed;

#define BTP_HFP_HF_EV_QUERY_CALL		0xa0
struct btp_hfp_hf_query_call_ev {
	bt_addr_le_t address;
	uint8_t index;
	uint8_t dir;
	uint8_t status;
	uint8_t mode;
	uint8_t multiparty;
	uint8_t type;
	uint8_t number_len;
	uint8_t number[0];
} __packed;
