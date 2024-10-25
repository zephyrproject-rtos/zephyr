/* btp_cap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* CAP commands */
#define BTP_CAP_READ_SUPPORTED_COMMANDS		0x01
struct btp_cap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_CAP_DISCOVER			0x02
struct btp_cap_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_CAP_UNICAST_SETUP_ASE		0x03
struct btp_cap_unicast_setup_ase_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t cis_id;
	uint8_t cig_id;
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t sdu_interval[3];
	uint8_t framing;
	uint16_t max_sdu;
	uint8_t retransmission_num;
	uint16_t max_transport_latency;
	uint8_t presentation_delay[3];
	uint8_t cc_ltvs_len;
	uint8_t metadata_ltvs_len;
	uint8_t ltvs[0];
} __packed;

#define BTP_CAP_UNICAST_AUDIO_START		0x04
struct btp_cap_unicast_audio_start_cmd {
	uint8_t cig_id;
	uint8_t set_type;
} __packed;
#define BTP_CAP_UNICAST_AUDIO_START_SET_TYPE_AD_HOC	0x00
#define BTP_CAP_UNICAST_AUDIO_START_SET_TYPE_CSIP	0x01

#define BTP_CAP_UNICAST_AUDIO_UPDATE		0x05
struct btp_cap_unicast_audio_update_cmd {
	uint8_t stream_count;
	uint8_t update_data[0];
} __packed;
struct btp_cap_unicast_audio_update_data {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t metadata_ltvs_len;
	uint8_t metadata_ltvs[0];
} __packed;

#define BTP_CAP_UNICAST_AUDIO_STOP		0x06
struct btp_cap_unicast_audio_stop_cmd {
	uint8_t cig_id;
	uint8_t flags;
} __packed;
#define BTP_CAP_UNICAST_AUDIO_STOP_FLAG_RELEASE BIT(0)

#define BTP_CAP_BROADCAST_SOURCE_SETUP_STREAM	0x07
struct btp_cap_broadcast_source_setup_stream_cmd {
	uint8_t source_id;
	uint8_t subgroup_id;
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t cc_ltvs_len;
	uint8_t metadata_ltvs_len;
	uint8_t ltvs[0];
} __packed;

#define BTP_CAP_BROADCAST_SOURCE_SETUP_SUBGROUP	0x08
struct btp_cap_broadcast_source_setup_subgroup_cmd {
	uint8_t source_id;
	uint8_t subgroup_id;
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t cc_ltvs_len;
	uint8_t metadata_ltvs_len;
	uint8_t ltvs[0];
} __packed;

#define BTP_CAP_BROADCAST_SOURCE_SETUP		0x09
struct btp_cap_broadcast_source_setup_cmd {
	uint8_t source_id;
	uint8_t broadcast_id[3];
	uint8_t sdu_interval[3];
	uint8_t framing;
	uint16_t max_sdu;
	uint8_t retransmission_num;
	uint16_t max_transport_latency;
	uint8_t presentation_delay[3];
	uint8_t flags;
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
} __packed;
struct btp_cap_broadcast_source_setup_rp {
	uint8_t source_id;
	uint32_t gap_settings;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
} __packed;
#define BTP_CAP_BROADCAST_SOURCE_SETUP_FLAG_ENCRYPTION		BIT(0)
#define BTP_CAP_BROADCAST_SOURCE_SETUP_FLAG_SUBGROUP_CODEC	BIT(1)

#define BTP_CAP_BROADCAST_SOURCE_RELEASE	0x0a
struct btp_cap_broadcast_source_release_cmd {
	uint8_t source_id;
} __packed;

#define BTP_CAP_BROADCAST_ADV_START		0x0b
struct btp_cap_broadcast_adv_start_cmd {
	uint8_t source_id;
} __packed;

#define BTP_CAP_BROADCAST_ADV_STOP		0x0c
struct btp_cap_broadcast_adv_stop_cmd {
	uint8_t source_id;
} __packed;

#define BTP_CAP_BROADCAST_SOURCE_START		0x0d
struct btp_cap_broadcast_source_start_cmd {
	uint8_t source_id;
} __packed;

#define BTP_CAP_BROADCAST_SOURCE_STOP		0x0e
struct btp_cap_broadcast_source_stop_cmd {
	uint8_t source_id;
} __packed;

#define BTP_CAP_BROADCAST_SOURCE_UPDATE		0x0f
struct btp_cap_broadcast_source_update_cmd {
	uint8_t source_id;
	uint8_t metadata_ltvs_len;
	uint8_t metadata_ltvs[0];
} __packed;

/* CAP events */
#define BTP_CAP_EV_DISCOVERY_COMPLETED		0x80
struct btp_cap_discovery_completed_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;
#define BTP_CAP_DISCOVERY_STATUS_SUCCESS	0x00
#define BTP_CAP_DISCOVERY_STATUS_FAILED		0x01

#define BTP_CAP_EV_UNICAST_START_COMPLETED	0x81
struct btp_cap_unicast_start_completed_ev {
	uint8_t cig_id;
	uint8_t status;
} __packed;
#define BTP_CAP_UNICAST_START_STATUS_SUCCESS	0x00
#define BTP_CAP_UNICAST_START_STATUS_FAILED	0x01

#define BTP_CAP_EV_UNICAST_STOP_COMPLETED	0x82
struct btp_cap_unicast_stop_completed_ev {
	uint8_t cig_id;
	uint8_t status;
} __packed;
#define BTP_CAP_UNICAST_STOP_STATUS_SUCCESS	0x00
#define BTP_CAP_UNICAST_STOP_STATUS_FAILED	0x01
