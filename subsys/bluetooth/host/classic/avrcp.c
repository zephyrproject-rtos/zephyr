/** @file
 *  @brief Audio Video Remote Control Profile
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "avctp_internal.h"
#include "avrcp_internal.h"

#define LOG_LEVEL CONFIG_BT_AVRCP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avrcp);

struct bt_avrcp {
	struct bt_avctp session;
};

struct avrcp_handler {
	bt_avrcp_opcode_t opcode;
	void (*func)(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf, bt_avctp_cr_t cr);
};

#define AVRCP_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, session)

static const struct bt_avrcp_cb *avrcp_cb;
static struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_CONN];

#if defined(CONFIG_BT_AVRCP_TARGET)
static struct bt_sdp_attribute avrcp_tg_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_TARGET_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVCTP_VER_1_4)
			},
			)
		},
		)
	),
	/* C1: Browsing not supported */
	/* C2: Cover Art not supported */
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVRCP_VER_1_6)
			},
			)
		},
		)
	),
	BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2),
	/* O: Provider Name not presented */
	BT_SDP_SERVICE_NAME("AVRCP Target"),
};

static struct bt_sdp_record avrcp_tg_rec = BT_SDP_RECORD(avrcp_tg_attrs);
#endif /* CONFIG_BT_AVRCP_TARGET */

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
static struct bt_sdp_attribute avrcp_ct_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVCTP_VER_1_4)
			},
			)
		},
		)
	),
	/* C1: Browsing not supported */
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVRCP_VER_1_6)
			},
			)
		},
		)
	),
	BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2),
	/* O: Provider Name not presented */
	BT_SDP_SERVICE_NAME("AVRCP Controller"),
};

static struct bt_sdp_record avrcp_ct_rec = BT_SDP_RECORD(avrcp_ct_attrs);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

static struct bt_avrcp *get_new_connection(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;

	if (!conn) {
		LOG_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	avrcp = &avrcp_connection[bt_conn_index(conn)];
	memset(avrcp, 0, sizeof(struct bt_avrcp));
	return avrcp;
}

/* The AVCTP L2CAP channel established */
static void avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->connected != NULL)) {
		avrcp_cb->connected(avrcp);
	}
}

/* The AVCTP L2CAP channel released */
static void avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->disconnected != NULL)) {
		avrcp_cb->disconnected(avrcp);
	}
}

static void process_get_cap_rsp(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_avc_pdu *pdu;
	struct bt_avrcp_get_cap_rsp *rsp;
	uint16_t len;
	uint16_t expected_len;

	if (buf->len < sizeof(*pdu)) {
		LOG_ERR("Invalid vendor payload length: %d", buf->len);
		return;
	}

	pdu = net_buf_pull_mem(buf, sizeof(*pdu));

	if (BT_AVRCP_AVC_PDU_GET_PACKET_TYPE(pdu) != BT_AVRVP_PKT_TYPE_SINGLE) {
		LOG_ERR("Invalid packet type");
		return;
	}

	len = sys_be16_to_cpu(pdu->param_len);

	if (len < sizeof(*rsp) || buf->len < len) {
		LOG_ERR("Invalid capability length: %d, buf length = %d", len, buf->len);
		return;
	}

	if ((avrcp_cb == NULL) || (avrcp_cb->get_cap_rsp == NULL)) {
		return;
	}

	rsp = (struct bt_avrcp_get_cap_rsp *)buf->data;

	switch (rsp->cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		expected_len = rsp->cap_cnt * BT_AVRCP_COMPANY_ID_SIZE;
		break;
	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		expected_len = rsp->cap_cnt;
		break;
	default:
		LOG_ERR("Unrecognized capability = 0x%x", rsp->cap_id);
		return;
	}

	if (buf->len < sizeof(*rsp) + expected_len) {
		LOG_ERR("Invalid capability payload length: %d", buf->len);
		return;
	}

	avrcp_cb->get_cap_rsp(avrcp, tid, rsp);
}

static void avrcp_vendor_dependent_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	struct bt_avrcp_avc_pdu *pdu;
	struct bt_avrcp_header *avrcp_hdr;
	uint32_t company_id;
	uint8_t pdu_id;

	if (buf->len < (sizeof(*avrcp_hdr) + BT_AVRCP_COMPANY_ID_SIZE + sizeof(pdu_id))) {
		LOG_ERR("Invalid vendor frame length: %d", buf->len);
		return;
	}

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));
	company_id = net_buf_pull_be24(buf);
	if (company_id != BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG) {
		LOG_ERR("Invalid company id: 0x%06x", company_id);
		return;
	}

	pdu = (struct bt_avrcp_avc_pdu *)buf->data;

	if (cr == BT_AVCTP_CMD) {
		LOG_DBG("Unhandled command: 0x%02x", pdu->pdu_id);
	} else { /* BT_AVCTP_RESPONSE */
		switch (pdu->pdu_id) {
		case BT_AVRCP_PDU_ID_GET_CAPS:
			process_get_cap_rsp(avrcp, tid, buf);
			break;
		default:
			LOG_DBG("Unhandled response: 0x%02x", pdu->pdu_id);
			break;
		}
	}
}

static void avrcp_unit_info_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
				    bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_unit_info_rsp rsp;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->unit_info_rsp != NULL)) {
			if (buf->len != 5) {
				LOG_ERR("Invalid unit info length: %d", buf->len);
				return;
			}
			net_buf_pull_u8(buf); /* Always 0x07 */
			rsp.unit_type = FIELD_GET(GENMASK(7, 3), net_buf_pull_u8(buf));
			rsp.company_id = net_buf_pull_be24(buf);
			avrcp_cb->unit_info_rsp(avrcp, tid, &rsp);
		}
	}
}

static void avrcp_subunit_info_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_subunit_info_rsp rsp;
	uint8_t tmp;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->subunit_info_rsp != NULL)) {
			if (buf->len < 5) {
				LOG_ERR("Invalid subunit info length: %d", buf->len);
				return;
			}
			net_buf_pull_u8(buf); /* Always 0x07 */
			tmp = net_buf_pull_u8(buf);
			rsp.subunit_type = FIELD_GET(GENMASK(7, 3), tmp);
			rsp.max_subunit_id = FIELD_GET(GENMASK(2, 0), tmp);
			if (buf->len < (rsp.max_subunit_id << 1)) {
				LOG_ERR("Invalid subunit info response");
				return;
			}
			rsp.extended_subunit_type = buf->data;
			rsp.extended_subunit_id = rsp.extended_subunit_type + rsp.max_subunit_id;
			avrcp_cb->subunit_info_rsp(avrcp, tid, &rsp);
		}
	}
}

static void avrcp_pass_through_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_passthrough_rsp *rsp;
	bt_avrcp_rsp_t result;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->subunit_info_rsp != NULL)) {
			if (buf->len < sizeof(*rsp)) {
				LOG_ERR("Invalid passthrough length: %d", buf->len);
				return;
			}

			result = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
			rsp = (struct bt_avrcp_passthrough_rsp *)buf->data;

			avrcp_cb->passthrough_rsp(avrcp, tid, result, rsp);
		}
	}
}

static const struct avrcp_handler handler[] = {
	{BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_vendor_dependent_handler},
	{BT_AVRCP_OPC_UNIT_INFO, avrcp_unit_info_handler},
	{BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_subunit_info_handler},
	{BT_AVRCP_OPC_PASS_THROUGH, avrcp_pass_through_handler},
};

/* An AVRCP message received */
static int avrcp_recv(struct bt_avctp *session, struct net_buf *buf)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);
	struct bt_avctp_header *avctp_hdr;
	struct bt_avrcp_header *avrcp_hdr;
	uint8_t tid, i;
	bt_avctp_cr_t cr;
	bt_avrcp_rsp_t rsp;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_subunit_type_t subunit_type;

	avctp_hdr = net_buf_pull_mem(buf, sizeof(*avctp_hdr));
	if (buf->len < sizeof(*avrcp_hdr)) {
		LOG_ERR("invalid AVRCP header received");
		return -EINVAL;
	}

	avrcp_hdr = (void *)buf->data;
	tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(avctp_hdr);
	cr = BT_AVCTP_HDR_GET_CR(avctp_hdr);
	rsp = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);

	if (avctp_hdr->pid != sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS)) {
		return -EINVAL; /* Ignore other profile */
	}

	LOG_DBG("AVRCP msg received, cr:0x%X, tid:0x%X, rsp: 0x%X, opc:0x%02X,", cr, tid, rsp,
		avrcp_hdr->opcode);

	for (i = 0U; i < ARRAY_SIZE(handler); i++) {
		if (avrcp_hdr->opcode == handler[i].opcode) {
			handler[i].func(avrcp, tid, buf, cr);
			return 0;
		}
	}

	return 0;
}

static const struct bt_avctp_ops_cb avctp_ops = {
	.connected = avrcp_connected,
	.disconnected = avrcp_disconnected,
	.recv = avrcp_recv,
};

static int avrcp_accept(struct bt_conn *conn, struct bt_avctp **session)
{
	struct bt_avrcp *avrcp;

	avrcp = get_new_connection(conn);
	if (!avrcp) {
		return -ENOMEM;
	}

	*session = &(avrcp->session);
	avrcp->session.ops = &avctp_ops;

	LOG_DBG("session: %p", &(avrcp->session));

	return 0;
}

static struct bt_avctp_event_cb avctp_cb = {
	.accept = avrcp_accept,
};

int bt_avrcp_init(void)
{
	int err;

	/* Register event handlers with AVCTP */
	err = bt_avctp_register(&avctp_cb);
	if (err < 0) {
		LOG_ERR("AVRCP registration failed");
		return err;
	}

#if defined(CONFIG_BT_AVRCP_TARGET)
	bt_sdp_register_service(&avrcp_tg_rec);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
	bt_sdp_register_service(&avrcp_ct_rec);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

	LOG_DBG("AVRCP Initialized successfully.");
	return 0;
}

struct bt_avrcp *bt_avrcp_connect(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;
	int err;

	avrcp = get_new_connection(conn);
	if (!avrcp) {
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	avrcp->session.ops = &avctp_ops;
	err = bt_avctp_connect(conn, &(avrcp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		memset(avrcp, 0, sizeof(struct bt_avrcp));
		LOG_DBG("AVCTP Connect failed");
		return NULL;
	}

	LOG_DBG("Connection request sent");
	return avrcp;
}

int bt_avrcp_disconnect(struct bt_avrcp *avrcp)
{
	int err;

	err = bt_avctp_disconnect(&(avrcp->session));
	if (err < 0) {
		LOG_DBG("AVCTP Disconnect failed");
		return err;
	}

	return err;
}

static struct net_buf *avrcp_create_pdu(struct bt_avrcp *avrcp, uint8_t tid, bt_avctp_cr_t cr)
{
	struct net_buf *buf;

	buf = bt_avctp_create_pdu(&(avrcp->session), cr, BT_AVCTP_PKT_TYPE_SINGLE,
				  BT_AVCTP_IPID_NONE, tid,
				  sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS));

	return buf;
}

static struct net_buf *avrcp_create_unit_pdu(struct bt_avrcp *avrcp, uint8_t tid, bt_avctp_cr_t cr)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, tid, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, cr == BT_AVCTP_CMD ? BT_AVRCP_CTYPE_STATUS
								    : BT_AVRCP_RSP_STABLE);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_UNIT_INFO;

	return buf;
}

static struct net_buf *avrcp_create_subunit_pdu(struct bt_avrcp *avrcp, uint8_t tid,
						bt_avctp_cr_t cr)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, tid, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, cr == BT_AVCTP_CMD ? BT_AVRCP_CTYPE_STATUS
								    : BT_AVRCP_RSP_STABLE);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_SUBUNIT_INFO;

	return buf;
}

static struct net_buf *avrcp_create_passthrough_pdu(struct bt_avrcp *avrcp, uint8_t tid,
						    bt_avctp_cr_t cr, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, tid, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, ctype_or_rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	cmd->hdr.opcode = BT_AVRCP_OPC_PASS_THROUGH;

	return buf;
}

static struct net_buf *avrcp_create_vendor_pdu(struct bt_avrcp *avrcp, uint8_t tid,
					       bt_avctp_cr_t cr, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, tid, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, ctype_or_rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	cmd->hdr.opcode = BT_AVRCP_OPC_VENDOR_DEPENDENT;

	return buf;
}

static int avrcp_send(struct bt_avrcp *avrcp, struct net_buf *buf)
{
	int err;
	struct bt_avctp_header *avctp_hdr = (struct bt_avctp_header *)(buf->data);
	struct bt_avrcp_header *avrcp_hdr =
		(struct bt_avrcp_header *)(buf->data + sizeof(*avctp_hdr));
	uint8_t tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(avctp_hdr);
	bt_avctp_cr_t cr = BT_AVCTP_HDR_GET_CR(avctp_hdr);
	bt_avrcp_ctype_t ctype = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);

	LOG_DBG("AVRCP send cr:0x%X, tid:0x%X, ctype: 0x%X, opc:0x%02X\n", cr, tid, ctype,
		avrcp_hdr->opcode);
	err = bt_avctp_send(&(avrcp->session), buf);
	if (err < 0) {
		net_buf_unref(buf);
		LOG_ERR("AVCTP send fail, err = %d", err);
		return err;
	}

	return 0;
}

int bt_avrcp_get_cap(struct bt_avrcp *avrcp, uint8_t tid, uint8_t cap_id)
{
	struct net_buf *buf;
	struct bt_avrcp_avc_pdu *pdu;

	buf = avrcp_create_vendor_pdu(avrcp, tid, BT_AVCTP_CMD, BT_AVRCP_CTYPE_STATUS);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_be24(buf, BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG);
	pdu = net_buf_add(buf, sizeof(*pdu));
	pdu->pdu_id = BT_AVRCP_PDU_ID_GET_CAPS;
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, BT_AVRVP_PKT_TYPE_SINGLE);
	pdu->param_len = sys_cpu_to_be16(sizeof(cap_id));
	net_buf_add_u8(buf, cap_id);

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_get_unit_info(struct bt_avrcp *avrcp, uint8_t tid)
{
	struct net_buf *buf;
	uint8_t param[5];

	buf = avrcp_create_unit_pdu(avrcp, tid, BT_AVCTP_CMD);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	net_buf_add_mem(buf, param, sizeof(param));

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_get_subunit_info(struct bt_avrcp *avrcp, uint8_t tid)
{
	struct net_buf *buf;
	uint8_t param[5];

	buf = avrcp_create_subunit_pdu(avrcp, tid, BT_AVCTP_CMD);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	param[0] = FIELD_PREP(GENMASK(6, 4), AVRCP_SUBUNIT_PAGE) |
		   FIELD_PREP(GENMASK(2, 0), AVRCP_SUBUNIT_EXTENSION_CODE);
	net_buf_add_mem(buf, param, sizeof(param));

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_passthrough(struct bt_avrcp *avrcp, uint8_t tid, uint8_t opid, uint8_t state,
			 const uint8_t *payload, uint8_t len)
{
	struct net_buf *buf;

	buf = avrcp_create_passthrough_pdu(avrcp, tid, BT_AVCTP_CMD, BT_AVRCP_CTYPE_CONTROL);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, FIELD_PREP(BIT(7), state) | FIELD_PREP(GENMASK(6, 0), opid));
	net_buf_add_u8(buf, len);
	if (len) {
		net_buf_add_mem(buf, payload, len);
	}

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_register_cb(const struct bt_avrcp_cb *cb)
{
	avrcp_cb = cb;
	return 0;
}
