/* btp_ascs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* ASCS commands */
#define BTP_ASCS_READ_SUPPORTED_COMMANDS	0x01
struct btp_ascs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_ASCS_CONFIGURE_CODEC	0x02
struct btp_ascs_configure_codec_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t coding_format;
	uint16_t vid;
	uint16_t cid;
	uint8_t cc_ltvs_len;
	uint8_t cc_ltvs[0];
} __packed;

#define BTP_ASCS_CONFIGURE_QOS	0x03
struct btp_ascs_configure_qos_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t cig_id;
	uint8_t cis_id;
	uint8_t sdu_interval[3];
	uint8_t framing;
	uint16_t max_sdu;
	uint8_t retransmission_num;
	uint16_t max_transport_latency;
	uint8_t presentation_delay[3];
} __packed;

#define BTP_ASCS_ENABLE	0x04
struct btp_ascs_enable_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_RECEIVER_START_READY	0x05
struct btp_ascs_receiver_start_ready_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_RECEIVER_STOP_READY	0x06
struct btp_ascs_receiver_stop_ready_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_DISABLE	0x07
struct btp_ascs_disable_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_RELEASE	0x08
struct btp_ascs_release_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_UPDATE_METADATA	0x09
struct btp_ascs_update_metadata_cmd {
	bt_addr_le_t address;
	uint8_t ase_id;
} __packed;

#define BTP_ASCS_ADD_ASE_TO_CIS		0x0a
struct btp_ascs_add_ase_to_cis {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t cig_id;
	uint8_t cis_id;
} __packed;

#define BTP_ASCS_PRECONFIGURE_QOS	0x0b
struct btp_ascs_preconfigure_qos_cmd {
	uint8_t cig_id;
	uint8_t cis_id;
	uint8_t sdu_interval[3];
	uint8_t framing;
	uint16_t max_sdu;
	uint8_t retransmission_num;
	uint16_t max_transport_latency;
	uint8_t presentation_delay[3];
} __packed;

/* ASCS events */
#define BTP_ASCS_EV_OPERATION_COMPLETED	0x80
struct btp_ascs_operation_completed_ev {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t opcode;
	uint8_t status;

	/* RFU */
	uint8_t flags;
} __packed;

#define BTP_ASCS_EV_CHARACTERISTIC_SUBSCRIBED 0x81

#define BTP_ASCS_EV_ASE_STATE_CHANGED	0x82
struct btp_ascs_ase_state_changed_ev {
	bt_addr_le_t address;
	uint8_t ase_id;
	uint8_t state;
} __packed;

#define BTP_ASCS_STATUS_SUCCESS	0x00
#define BTP_ASCS_STATUS_FAILED	0x01
