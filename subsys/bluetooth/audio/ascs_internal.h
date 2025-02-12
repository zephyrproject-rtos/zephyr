/* @file
 * @brief Internal APIs for ASCS handling

 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_ASCS_INTERNAL_H
#define BT_ASCS_INTERNAL_H

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>

#define BT_ASCS_ASE_ID_NONE              0x00

/* The number of ASEs in the notification when the opcode is unsupported or the length of the
 * control point write request is incorrect
 */
#define BT_ASCS_UNSUPP_OR_LENGTH_ERR_NUM_ASE 0xFFU

/* Transport QoS Packing */
#define BT_ASCS_QOS_PACKING_SEQ          0x00
#define BT_ASCS_QOS_PACKING_INT          0x01

/* Transport QoS Framing */
#define BT_ASCS_QOS_FRAMING_UNFRAMED     0x00
#define BT_ASCS_QOS_FRAMING_FRAMED       0x01

/* Format of the ASE characteristic, defined in Table 4.2 */
struct bt_ascs_ase_status {
	uint8_t  id;
	uint8_t  state;
	uint8_t  params[0];
} __packed;

struct bt_ascs_codec_config {
	uint8_t len;
	uint8_t type;
	uint8_t data[0];
} __packed;

struct bt_ascs_codec {
	uint8_t  id;
	uint16_t cid;
	uint16_t vid;
} __packed;

#define BT_ASCS_PD_NO_PREF 0x00000000

/* ASE_State = 0x01 (Codec Configured), defined in Table 4.3. */
struct bt_ascs_ase_status_config {
	uint8_t  framing;
	uint8_t  phy;
	uint8_t  rtn;
	uint16_t latency;
	uint8_t  pd_min[3];
	uint8_t  pd_max[3];
	uint8_t  prefer_pd_min[3];
	uint8_t  prefer_pd_max[3];
	struct bt_ascs_codec codec;
	uint8_t  cc_len;
	/* LTV-formatted Codec-Specific Configuration */
	struct bt_ascs_codec_config cc[0];
} __packed;

/* ASE_State = 0x02 (QoS Configured), defined in Table 4.4. */
struct bt_ascs_ase_status_qos {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  interval[3];
	uint8_t  framing;
	uint8_t  phy;
	uint16_t sdu;
	uint8_t  rtn;
	uint16_t latency;
	uint8_t  pd[3];
} __packed;

/* ASE_Status = 0x03 (Enabling) defined in Table 4.5.
 */
struct bt_ascs_ase_status_enable {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  metadata_len;
	uint8_t  metadata[0];
} __packed;

/* ASE_Status =  0x04 (Streaming) defined in Table 4.5.
 */
struct bt_ascs_ase_status_stream {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  metadata_len;
	uint8_t  metadata[0];
} __packed;

/* ASE_Status = 0x05 (Disabling) as defined in Table 4.5.
 */
struct bt_ascs_ase_status_disable {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  metadata_len;
	uint8_t  metadata[0];
} __packed;

/* ASE Control Point Protocol */
struct bt_ascs_ase_cp {
	/* Request/Notification opcode */
	uint8_t  op;
	uint8_t  pdu[0];
} __packed;

/* Opcodes */
#define BT_ASCS_CONFIG_OP                0x01

#define BT_ASCS_CONFIG_LATENCY_LOW       0x01
#define BT_ASCS_CONFIG_LATENCY_MEDIUM    0x02
#define BT_ASCS_CONFIG_LATENCY_HIGH      0x03

#define BT_ASCS_CONFIG_PHY_LE_1M         0x01
#define BT_ASCS_CONFIG_PHY_LE_2M         0x02
#define BT_ASCS_CONFIG_PHY_LE_CODED      0x03

struct bt_ascs_config {
	/* ASE ID */
	uint8_t  ase;
	/* Target latency */
	uint8_t  latency;
	/* Target PHY */
	uint8_t  phy;
	/* Codec ID */
	struct bt_ascs_codec codec;
	/* Codec Specific Config Length */
	uint8_t  cc_len;
	/* LTV-formatted Codec-Specific Configuration */
	struct bt_ascs_codec_config cc[0];
} __packed;

struct bt_ascs_config_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* Config Parameters */
	struct bt_ascs_config cfg[0];
} __packed;

#define BT_ASCS_QOS_OP                   0x02
struct bt_ascs_qos {
	/* ASE ID */
	uint8_t  ase;
	/* CIG ID*/
	uint8_t  cig;
	/* CIG ID*/
	uint8_t  cis;
	/* Frame interval */
	uint8_t  interval[3];
	/* Frame framing */
	uint8_t  framing;
	/* PHY */
	uint8_t  phy;
	/* Maximum SDU Size */
	uint16_t sdu;
	/* Retransmission Effort */
	uint8_t  rtn;
	/* Transport Latency */
	uint16_t latency;
	/* Presentation Delay */
	uint8_t  pd[3];
} __packed;

struct bt_ascs_qos_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* QoS Parameters */
	struct bt_ascs_qos qos[0];
} __packed;

#define BT_ASCS_ENABLE_OP                0x03
struct bt_ascs_metadata {
	/* ASE ID */
	uint8_t  ase;
	/* Metadata length */
	uint8_t  len;
	/* LTV-formatted Metadata */
	uint8_t  data[0];
} __packed;

struct bt_ascs_enable_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* Metadata */
	struct bt_ascs_metadata metadata[0];
} __packed;

#define BT_ASCS_START_OP                 0x04
struct bt_ascs_start_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* ASE IDs */
	uint8_t  ase[0];
} __packed;

#define BT_ASCS_DISABLE_OP               0x05
struct bt_ascs_disable_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* ASE IDs */
	uint8_t  ase[0];
} __packed;

#define BT_ASCS_STOP_OP                  0x06
struct bt_ascs_stop_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* ASE IDs */
	uint8_t  ase[0];
} __packed;

#define BT_ASCS_METADATA_OP              0x07
struct bt_ascs_metadata_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* Metadata */
	struct bt_ascs_metadata metadata[0];
} __packed;

#define BT_ASCS_RELEASE_OP              0x08
struct bt_ascs_release_op {
	/* Number of ASEs */
	uint8_t  num_ases;
	/* Ase IDs */
	uint8_t  ase[0];
} __packed;

struct bt_ascs_cp_ase_rsp {
	/* ASE ID */
	uint8_t  id;
	/* Response code */
	uint8_t  code;
	/* Response reason */
	uint8_t  reason;
} __packed;

struct bt_ascs_cp_rsp {
	/* Opcode */
	uint8_t  op;
	/* Number of ASEs */
	uint8_t  num_ase;
	/* ASE response */
	struct bt_ascs_cp_ase_rsp ase_rsp[0];
} __packed;

static inline const char *bt_ascs_op_str(uint8_t op)
{
	switch (op) {
	case BT_ASCS_CONFIG_OP:
		return "Config Codec";
	case BT_ASCS_QOS_OP:
		return "Config QoS";
	case BT_ASCS_ENABLE_OP:
		return "Enable";
	case BT_ASCS_START_OP:
		return "Receiver Start Ready";
	case BT_ASCS_DISABLE_OP:
		return "Disable";
	case BT_ASCS_STOP_OP:
		return "Receiver Stop Ready";
	case BT_ASCS_METADATA_OP:
		return "Update Metadata";
	case BT_ASCS_RELEASE_OP:
		return "Release";
	}

	return "Unknown";
}

static inline const char *bt_ascs_rsp_str(uint8_t code)
{
	switch (code) {
	case BT_BAP_ASCS_RSP_CODE_SUCCESS:
		return "Success";
	case BT_BAP_ASCS_RSP_CODE_NOT_SUPPORTED:
		return "Unsupported Opcode";
	case BT_BAP_ASCS_RSP_CODE_INVALID_LENGTH:
		return "Invalid Length";
	case BT_BAP_ASCS_RSP_CODE_INVALID_ASE:
		return "Invalid ASE_ID";
	case BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE:
		return "Invalid ASE State";
	case BT_BAP_ASCS_RSP_CODE_INVALID_DIR:
		return "Invalid ASE Direction";
	case BT_BAP_ASCS_RSP_CODE_CAP_UNSUPPORTED:
		return "Unsupported Capabilities";
	case BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED:
		return "Unsupported Configuration Value";
	case BT_BAP_ASCS_RSP_CODE_CONF_REJECTED:
		return "Rejected Configuration Value";
	case BT_BAP_ASCS_RSP_CODE_CONF_INVALID:
		return "Invalid Configuration Value";
	case BT_BAP_ASCS_RSP_CODE_METADATA_UNSUPPORTED:
		return "Unsupported Metadata";
	case BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED:
		return "Rejected Metadata";
	case BT_BAP_ASCS_RSP_CODE_METADATA_INVALID:
		return "Invalid Metadata";
	case BT_BAP_ASCS_RSP_CODE_NO_MEM:
		return "Insufficient Resources";
	case BT_BAP_ASCS_RSP_CODE_UNSPECIFIED:
		return "Unspecified Error";
	}

	return "Unknown";
}

static inline const char *bt_ascs_reason_str(uint8_t reason)
{
	switch (reason) {
	case BT_BAP_ASCS_REASON_NONE:
		return "None";
	case BT_BAP_ASCS_REASON_CODEC:
		return "Codec ID";
	case BT_BAP_ASCS_REASON_CODEC_DATA:
		return "Codec Specific Configuration";
	case BT_BAP_ASCS_REASON_INTERVAL:
		return "SDU Interval";
	case BT_BAP_ASCS_REASON_FRAMING:
		return "Framing";
	case BT_BAP_ASCS_REASON_PHY:
		return "PHY";
	case BT_BAP_ASCS_REASON_SDU:
		return "Maximum SDU Size";
	case BT_BAP_ASCS_REASON_RTN:
		return "Retransmission Number";
	case BT_BAP_ASCS_REASON_LATENCY:
		return "Maximum Transport Delay";
	case BT_BAP_ASCS_REASON_PD:
		return "Presentation Delay";
	case BT_BAP_ASCS_REASON_CIS:
		return "Invalid ASE CIS Mapping";
	}

	return "Unknown";
}

int bt_ascs_init(const struct bt_bap_unicast_server_cb *cb);
void bt_ascs_cleanup(void);

int ascs_ep_set_state(struct bt_bap_ep *ep, uint8_t state);

int bt_ascs_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
		       struct bt_audio_codec_cfg *codec_cfg,
		       const struct bt_audio_codec_qos_pref *qos_pref);
int bt_ascs_disable_ase(struct bt_bap_ep *ep);
int bt_ascs_release_ase(struct bt_bap_ep *ep);

void bt_ascs_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data);

#endif /* BT_ASCS_INTERNAL_H */
