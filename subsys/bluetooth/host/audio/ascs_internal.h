/* @file
 * @brief Internal APIs for ASCS handling

 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Response Status Code */
#define BT_ASCS_RSP_SUCCESS		0x00
#define BT_ASCS_RSP_NOT_SUPPORTED	0x01
#define BT_ASCS_RSP_TRUNCATED		0x02
#define BT_ASCS_RSP_INVALID_ASE		0x03
#define BT_ASCS_RSP_INVALID_ASE_STATE	0x04
#define BT_ASCS_RSP_CAP_UNSUPPORTED	0x05
#define BT_ASCS_RSP_CONF_UNSUPPORTED	0x06
#define BT_ASCS_RSP_CONF_REJECTED	0x07
#define BT_ASCS_RSP_CONF_INVALID	0x08
#define BT_ASCS_RSP_METADATA_UNSUPPORTED 0x09
#define BT_ASCS_RSP_METADATA_REJECTED	0x0a
#define BT_ASCS_RSP_METADATA_INVALID	0x0b
#define BT_ASCS_RSP_NO_MEM		0x0c
#define BT_ASCS_RSP_UNSPECIFIED		0x0d

/* Response Reasons */
#define BT_ASCS_REASON_NONE		0x00
#define BT_ASCS_REASON_DIR		0x01
#define BT_ASCS_REASON_CODEC		0x02
#define BT_ASCS_REASON_CODEC_DATA_LEN	0x03
#define BT_ASCS_REASON_CODEC_DATA	0x04
#define BT_ASCS_REASON_INTERVAL		0x05
#define BT_ASCS_REASON_FRAMING		0x06
#define BT_ASCS_REASON_PHY		0x07
#define BT_ASCS_REASON_SDU		0x08
#define BT_ASCS_REASON_RTN		0x09
#define BT_ASCS_REASON_LATENCY		0x0a
#define BT_ASCS_REASON_PD		0x0b
#define BT_ASCS_REASON_METADATA		0x0c

/* Transport QoS Packing */
#define BT_ASCS_QOS_PACKING_SEQ		0x00
#define BT_ASCS_QOS_PACKING_INT		0x01

/* Transport QoS Framing */
#define BT_ASCS_QOS_FRAMING_UNFRAMED	0x00
#define BT_ASCS_QOS_FRAMING_FRAMED	0x01

/* ASE characteristic states */
#define BT_ASCS_ASE_STATE_IDLE		0x00
#define BT_ASCS_ASE_STATE_CONFIG	0x01
#define BT_ASCS_ASE_STATE_QOS		0x02
#define BT_ASCS_ASE_STATE_ENABLING	0x03
#define BT_ASCS_ASE_STATE_STREAMING	0x04
#define BT_ASCS_ASE_STATE_DISABLING	0x05
#define BT_ASCS_ASE_STATE_RELEASING	0x06

/* Format of the ASE characteristic, defined in Table 4.6 */
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

/* ASE_State = 0x01 (Codec Configured), defined in Table 4.7. */
struct bt_ascs_ase_status_config {
	uint8_t  dir;
	uint8_t  framing;
	uint8_t  phy;
	uint8_t  rtn;
	uint16_t latency;
	uint8_t  pd_min[3];
	uint8_t  pd_max[3];
	struct bt_ascs_codec codec;
	uint8_t  cc_len;
	/* LTV-formatted Codec-Specific Configuration */
	struct bt_ascs_codec_config cc[0];
} __packed;

/* ASE_State = 0x02 (QoS Configured), defined in Table 4.8. */
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

/* ASE_Status = 0x03 (Enabling), 0x04 (Streaming), or 0x05 (Disabling) as
 * defined in Table 4.9.
 */
struct bt_ascs_ase_status_enable {
	uint8_t  cig_id;
	uint8_t  cis_id;
	uint8_t  metadata_len;
	uint8_t  metadata[0];
} __packed;

/* ASE Control Point Protocol */
struct bt_ascs_ase_cp {
	uint8_t  op;			/* Request/Notification opcode */
	uint8_t  pdu[0];
} __packed;

/* Opcodes */
#define BT_ASCS_CONFIG_OP		0x01

#define BT_ASCS_CONFIG_LATENCY_LOW	0x01
#define BT_ASCS_CONFIG_LATENCY_MEDIUM	0x02
#define BT_ASCS_CONFIG_LATENCY_HIGH	0x03

#define BT_ASCS_CONFIG_PHY_LE_1M	0x01
#define BT_ASCS_CONFIG_PHY_LE_2M	0x02
#define BT_ASCS_CONFIG_PHY_LE_CODED	0x03

struct bt_ascs_config {
	uint8_t  ase;			/* ASE ID */
	uint8_t  dir;			/* ASE direction */
	uint8_t  latency;		/* Target latency */
	uint8_t  phy;			/* Target PHY */
	struct bt_ascs_codec codec;	/* Codec ID */
	uint8_t  cc_len;		/* Codec Specific Config Length */
	/* LTV-formatted Codec-Specific Configuration */
	struct bt_ascs_codec_config cc[0];
} __packed;

struct bt_ascs_config_op {
	uint8_t  num_ases;			/* Num of ASEs */
	struct bt_ascs_config cfg[0];
} __packed;

#define BT_ASCS_QOS_OP			0x02
struct bt_ascs_qos {
	uint8_t  ase;			/* ASE ID */
	uint8_t  cig;			/* CIG ID*/
	uint8_t  cis;			/* CIG ID*/
	uint8_t  interval[3];		/* Frame interval */
	uint8_t  framing;		/* Frame framing */
	uint8_t  phy;			/* PHY */
	uint16_t sdu;			/* Maximum SDU Size */
	uint8_t  rtn;			/* Retransmission Effort */
	uint16_t latency;		/* Transport Latency */
	uint8_t  pd[3];			/* Presentation Delay */
} __packed;

struct bt_ascs_qos_op {
	uint8_t  num_ases;		/* Num of ASEs */
	struct bt_ascs_qos qos[0];	/* QoS Parameters */
} __packed;

#define BT_ASCS_ENABLE_OP		0x03
struct bt_ascs_metadata {
	uint8_t  ase;			/* ASE ID */
	uint8_t  len;			/* Metadata length */
	uint8_t  data[0];		/* LTV-formatted Metadata */
} __packed;

struct bt_ascs_enable_op {
	uint8_t  num_ases;			/* Num of ASEs */
	struct bt_ascs_metadata metadata[0];	/* Metadata */
} __packed;

#define BT_ASCS_START_OP		0x04
struct bt_ascs_start_op {
	uint8_t  num_ases;		/* Num of ASEs */
	uint8_t  ase[0];		/* Ase IDs */
} __packed;

#define BT_ASCS_DISABLE_OP		0x05
struct bt_ascs_disable_op {
	uint8_t  num_ases;		/* Num of ASEs */
	uint8_t  ase[0];		/* Ase IDs */
} __packed;

#define BT_ASCS_STOP_OP			0x06
struct bt_ascs_stop_op {
	uint8_t  num_ases;		/* Num of ASEs */
	uint8_t  ase[0];		/* Ase IDs */
} __packed;

#define BT_ASCS_METADATA_OP		0x07
struct bt_ascs_metadata_op {
	uint8_t  num_ases;			/* Num of ASEs */
	struct bt_ascs_metadata metadata[0];	/* Metadata */
} __packed;

#define BT_ASCS_RELEASE_OP		0x08
struct bt_ascs_release_op {
	uint8_t  num_ases;		/* Num of ASEs */
	uint8_t  ase[0];		/* Ase IDs */
} __packed;

struct bt_ascs_cp_ase_rsp {
	uint8_t  id;
	uint8_t  code;
	uint8_t  reason;
} __packed;

struct bt_ascs_cp_rsp {
	uint8_t  op;
	uint8_t  num_ase;
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
	case BT_ASCS_RSP_SUCCESS:
		return "Success";
	case BT_ASCS_RSP_NOT_SUPPORTED:
		return "Unsupported Opcode";
	case BT_ASCS_RSP_TRUNCATED:
		return "Truncated Operation";
	case BT_ASCS_RSP_INVALID_ASE:
		return "Invalid ASE_ID";
	case BT_ASCS_RSP_INVALID_ASE_STATE:
		return "Invalid ASE State";
	case BT_ASCS_RSP_CAP_UNSUPPORTED:
		return "Unsupported Capabilities";
	case BT_ASCS_RSP_CONF_UNSUPPORTED:
		return "Unsupported Configuration Value";
	case BT_ASCS_RSP_CONF_REJECTED:
		return "Rejected Configuration Value";
	case BT_ASCS_RSP_CONF_INVALID:
		return "Invalid Configuration Value";
	case BT_ASCS_RSP_METADATA_UNSUPPORTED:
		return "Unsupported Metadata";
	case BT_ASCS_RSP_METADATA_REJECTED:
		return "Rejected Metadata";
	case BT_ASCS_RSP_METADATA_INVALID:
		return "Invalid Metadata";
	case BT_ASCS_RSP_NO_MEM:
		return "Insufficient Resources";
	case BT_ASCS_RSP_UNSPECIFIED:
		return "Unspecified Error";
	}

	return "Unknown";
}

static inline const char *bt_ascs_reason_str(uint8_t reason)
{
	switch (reason) {
	case BT_ASCS_REASON_NONE:
		return "None";
	case BT_ASCS_REASON_DIR:
		return "Direction";
	case BT_ASCS_REASON_CODEC:
		return "Codec ID";
	case BT_ASCS_REASON_CODEC_DATA_LEN:
		return "Codec Configuration Length";
	case BT_ASCS_REASON_CODEC_DATA:
		return "Codec Configuration";
	case BT_ASCS_REASON_INTERVAL:
		return "SDU Interval";
	case BT_ASCS_REASON_FRAMING:
		return "Framing";
	case BT_ASCS_REASON_PHY:
		return "PHY";
	case BT_ASCS_REASON_SDU:
		return "Maximum SDU Size";
	case BT_ASCS_REASON_RTN:
		return "Retransmission Number";
	case BT_ASCS_REASON_LATENCY:
		return "Maximum Transport Delay";
	case BT_ASCS_REASON_PD:
		return "Presentation Delay";
	case BT_ASCS_REASON_METADATA:
		return "Metadata Length";
	}

	return "Unknown";
}
