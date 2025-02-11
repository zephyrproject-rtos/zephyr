/* btp_bap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/audio.h>

/* BAP commands */
#define BTP_BAP_READ_SUPPORTED_COMMANDS		0x01
struct btp_bap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_BAP_DISCOVER			0x02
struct btp_bap_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_DISCOVERY_STATUS_SUCCESS	0x00
#define BTP_BAP_DISCOVERY_STATUS_FAILED		0x01

#define BTP_BAP_SEND				0x03
struct btp_bap_send_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t data_len;
	uint8_t data[0];
} __packed;

struct btp_bap_send_rp {
	uint8_t data_len;
} __packed;

#define BTP_BAP_BROADCAST_SOURCE_SETUP		0x04
struct btp_bap_broadcast_source_setup_cmd {
	uint8_t streams_per_subgroup;
	uint8_t subgroups;
	uint8_t sdu_interval[3];
	uint8_t framing;
	uint16_t max_sdu;
	uint8_t retransmission_num;
	uint16_t max_transport_latency;
	uint8_t presentation_delay[3];
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t cc_ltvs_len;
	uint8_t cc_ltvs[];
} __packed;
struct btp_bap_broadcast_source_setup_rp {
	uint32_t gap_settings;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_SOURCE_RELEASE	0x05
struct btp_bap_broadcast_source_release_cmd {
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_ADV_START		0x06
struct btp_bap_broadcast_adv_start_cmd {
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_ADV_STOP		0x07
struct btp_bap_broadcast_adv_stop_cmd {
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_SOURCE_START		0x08
struct btp_bap_broadcast_source_start_cmd {
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_SOURCE_STOP		0x09
struct btp_bap_broadcast_source_stop_cmd {
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_SINK_SETUP		0x0a
struct btp_bap_broadcast_sink_setup_cmd {
} __packed;

#define BTP_BAP_BROADCAST_SINK_RELEASE		0x0b
struct btp_bap_broadcast_sink_release_cmd {
} __packed;

#define BTP_BAP_BROADCAST_SCAN_START		0x0c
struct btp_bap_broadcast_scan_start_cmd {
} __packed;

#define BTP_BAP_BROADCAST_SCAN_STOP		0x0d
struct btp_bap_broadcast_scan_stop_cmd {
} __packed;

#define BTP_BAP_BROADCAST_SINK_SYNC		0x0e
struct btp_bap_broadcast_sink_sync_cmd {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t advertiser_sid;
	uint16_t skip;
	uint16_t sync_timeout;
	uint8_t past_avail;
	uint8_t src_id;
} __packed;

#define BTP_BAP_BROADCAST_SINK_STOP		0x0f
struct btp_bap_broadcast_sink_stop_cmd {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;

#define BTP_BAP_BROADCAST_SINK_BIS_SYNC		0x10
struct btp_bap_broadcast_sink_bis_sync_cmd {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint32_t requested_bis_sync;
} __packed;

#define BTP_BAP_DISCOVER_SCAN_DELEGATORS	0x11
struct btp_bap_discover_scan_delegators_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_BROADCAST_ASSISTANT_SCAN_START	0x12
struct btp_bap_broadcast_assistant_scan_start_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_BROADCAST_ASSISTANT_SCAN_STOP	0x13
struct btp_bap_broadcast_assistant_scan_stop_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_ADD_BROADCAST_SRC		0x14
struct btp_bap_add_broadcast_src_cmd {
	bt_addr_le_t address;
	bt_addr_le_t broadcaster_address;
	uint8_t advertiser_sid;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t padv_sync;
	uint16_t padv_interval;
	uint8_t num_subgroups;
	uint8_t subgroups[0];
} __packed;

#define BTP_BAP_REMOVE_BROADCAST_SRC		0x15
struct btp_bap_remove_broadcast_src_cmd {
	bt_addr_le_t address;
	uint8_t src_id;
} __packed;

#define BTP_BAP_MODIFY_BROADCAST_SRC		0x16
struct btp_bap_modify_broadcast_src_cmd {
	bt_addr_le_t address;
	uint8_t src_id;
	uint8_t padv_sync;
	uint16_t padv_interval;
	uint8_t num_subgroups;
	uint8_t subgroups[0];
} __packed;

#define BTP_BAP_SET_BROADCAST_CODE		0x17
struct btp_bap_set_broadcast_code_cmd {
	bt_addr_le_t address;
	uint8_t src_id;
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
} __packed;

#define BTP_BAP_SEND_PAST			0x18
struct btp_bap_send_past_cmd {
	bt_addr_le_t address;
	uint8_t src_id;
} __packed;

/* BAP events */
#define BTP_BAP_EV_DISCOVERY_COMPLETED		0x80
struct btp_bap_discovery_completed_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BTP_BAP_EV_CODEC_CAP_FOUND		0x81
struct btp_bap_codec_cap_found_ev {
	bt_addr_le_t address;
	uint8_t dir;
	uint8_t coding_format;
	uint16_t frequencies;
	uint8_t frame_durations;
	uint32_t octets_per_frame;
	uint8_t channel_counts;
} __packed;

#define BTP_BAP_EV_ASE_FOUND			0x82
struct btp_ascs_ase_found_ev {
	bt_addr_le_t address;
	uint8_t dir;
	uint8_t ase_id;
} __packed;

#define BTP_BAP_EV_STREAM_RECEIVED		0x83
struct btp_bap_stream_received_ev {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BAP_EV_BAA_FOUND			0x84
struct btp_bap_baa_found_ev {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t advertiser_sid;
	uint16_t padv_interval;
} __packed;

#define BTP_BAP_EV_BIS_FOUND			0x85
struct btp_bap_bis_found_ev {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t presentation_delay[3];
	uint8_t subgroup_id;
	uint8_t bis_id;
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t cc_ltvs_len;
	uint8_t cc_ltvs[];
} __packed;

#define BTP_BAP_EV_BIS_SYNCED			0x86
struct btp_bap_bis_syned_ev {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t bis_id;
} __packed;

#define BTP_BAP_EV_BIS_STREAM_RECEIVED		0x87
struct btp_bap_bis_stream_received_ev {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t bis_id;
	uint8_t data_len;
	uint8_t data[];
} __packed;

#define BTP_BAP_EV_SCAN_DELEGATOR_FOUND		0x88
struct btp_bap_scan_delegator_found_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_EV_BROADCAST_RECEIVE_STATE	0x89
struct btp_bap_broadcast_receive_state_ev {
	bt_addr_le_t address;
	uint8_t src_id;
	bt_addr_le_t broadcaster_address;
	uint8_t advertiser_sid;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t pa_sync_state;
	uint8_t big_encryption;
	uint8_t num_subgroups;
	uint8_t subgroups[0];
} __packed;

#define BTP_BAP_EV_PA_SYNC_REQ			0x8a
struct btp_bap_pa_sync_req_ev {
	bt_addr_le_t address;
	uint8_t src_id;
	uint8_t advertiser_sid;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t past_avail;
	uint16_t pa_interval;
} __packed;
