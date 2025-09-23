/* sdp_internal.h - Service Discovery Protocol handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The PDU identifiers of SDP packets between client and server
 */
#define BT_SDP_ERROR_RSP           0x01
#define BT_SDP_SVC_SEARCH_REQ      0x02
#define BT_SDP_SVC_SEARCH_RSP      0x03
#define BT_SDP_SVC_ATTR_REQ        0x04
#define BT_SDP_SVC_ATTR_RSP        0x05
#define BT_SDP_SVC_SEARCH_ATTR_REQ 0x06
#define BT_SDP_SVC_SEARCH_ATTR_RSP 0x07

/*
 * Some additions to support service registration.
 * These are outside the scope of the Bluetooth specification
 */
#define BT_SDP_SVC_REGISTER_REQ 0x75
#define BT_SDP_SVC_REGISTER_RSP 0x76
#define BT_SDP_SVC_UPDATE_REQ   0x77
#define BT_SDP_SVC_UPDATE_RSP   0x78
#define BT_SDP_SVC_REMOVE_REQ   0x79
#define BT_SDP_SVC_REMOVE_RSP   0x80

/*
 * SDP Error codes
 */
#define BT_SDP_INVALID_VERSION       0x0001
#define BT_SDP_INVALID_RECORD_HANDLE 0x0002
#define BT_SDP_INVALID_SYNTAX        0x0003
#define BT_SDP_INVALID_PDU_SIZE      0x0004
#define BT_SDP_INVALID_CSTATE        0x0005

struct bt_sdp_data_elem_seq {
	uint8_t  type; /* Type: Will be data element sequence */
	uint16_t size; /* We only support 2 byte sizes for now */
} __packed;

struct bt_sdp_hdr {
	uint8_t  op_code;
	uint16_t tid;
	uint16_t param_len;
} __packed;

struct bt_sdp_svc_rsp {
	uint16_t  total_recs;
	uint16_t  current_recs;
} __packed;

struct bt_sdp_att_rsp {
	uint16_t att_list_len;
} __packed;

/* Allowed attributes length in SSA Request PDU to be taken from server */
#define BT_SDP_MAX_ATTR_LEN 0xffff

/* Max allowed length of PDU Continuation State */
#define BT_SDP_MAX_PDU_CSTATE_LEN 16

/* Type mapping SDP PDU Continuation State */
struct bt_sdp_pdu_cstate {
	uint8_t length;
	uint8_t data[BT_SDP_MAX_PDU_CSTATE_LEN];
} __packed;

void bt_sdp_init(void);
