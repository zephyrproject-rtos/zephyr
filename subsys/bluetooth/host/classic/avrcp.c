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
	struct bt_avrcp_req req;
	struct k_work_delayable timeout_work;
	uint8_t local_tid;
};

struct avrcp_handler {
	bt_avrcp_opcode_t opcode;
	void (*func)(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr);
};

#define AVRCP_TIMEOUT       K_SECONDS(3) /* Shell be greater than TMTP (1000ms) */
#define AVRCP_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, session)
#define AVRCP_KWORK(_work)                                                                         \
	CONTAINER_OF(CONTAINER_OF(_work, struct k_work_delayable, work), struct bt_avrcp,          \
		     timeout_work)

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

static void avrcp_timeout(struct k_work *work)
{
	struct bt_avrcp *avrcp = AVRCP_KWORK(work);

	LOG_WRN("Timeout: tid 0x%X, opc 0x%02X", avrcp->req.tid, avrcp->req.opcode);
}

/* The AVCTP L2CAP channel established */
static void avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->connected != NULL)) {
		avrcp_cb->connected(avrcp);
	}

	k_work_init_delayable(&avrcp->timeout_work, avrcp_timeout);
}

/* The AVCTP L2CAP channel released */
static void avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->disconnected != NULL)) {
		avrcp_cb->disconnected(avrcp);
	}
}

static void avrcp_vendor_dependent_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static void avrcp_unit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_unit_info_rsp rsp;

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->unit_info_rsp != NULL)) {
			net_buf_pull(buf, sizeof(*avrcp_hdr));
			if (buf->len != 5) {
				LOG_ERR("Invalid unit info length");
				return;
			}
			net_buf_pull_u8(buf); /* Always 0x07 */
			rsp.unit_type = FIELD_GET(GENMASK(7, 3), net_buf_pull_u8(buf));
			rsp.company_id = net_buf_pull_be24(buf);
			avrcp_cb->unit_info_rsp(avrcp, &rsp);
		}
	}
}

static void avrcp_subunit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_subunit_info_rsp rsp;
	uint8_t tmp;

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->subunit_info_rsp != NULL)) {
			net_buf_pull(buf, sizeof(*avrcp_hdr));
			if (buf->len < 5) {
				LOG_ERR("Invalid subunit info length");
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
			avrcp_cb->subunit_info_rsp(avrcp, &rsp);
		}
	}
}

static void avrcp_pass_through_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static const struct avrcp_handler handler[] = {
	{ BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_vendor_dependent_handler },
	{ BT_AVRCP_OPC_UNIT_INFO, avrcp_unit_info_handler },
	{ BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_subunit_info_handler },
	{ BT_AVRCP_OPC_PASS_THROUGH, avrcp_pass_through_handler },
};

/* An AVRCP message received */
static int avrcp_recv(struct bt_avctp *session, struct net_buf *buf)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);
	struct bt_avctp_header *avctp_hdr;
	struct bt_avrcp_header *avrcp_hdr;
	uint8_t tid, i;
	bt_avctp_cr_t cr;
	bt_avrcp_ctype_t ctype;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_subunit_type_t subunit_type;

	avctp_hdr = (void *)buf->data;
	net_buf_pull(buf, sizeof(*avctp_hdr));
	if (buf->len < sizeof(*avrcp_hdr)) {
		LOG_ERR("invalid AVRCP header received");
		return -EINVAL;
	}

	avrcp_hdr = (void *)buf->data;
	tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(avctp_hdr);
	cr = BT_AVCTP_HDR_GET_CR(avctp_hdr);
	ctype = BT_AVRCP_HDR_GET_CTYPE(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);

	if (avctp_hdr->pid != sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS)) {
		return -EINVAL; /* Ignore other profile */
	}

	LOG_DBG("AVRCP msg received, cr:0x%X, tid:0x%X, ctype: 0x%X, opc:0x%02X,", cr, tid, ctype,
		avrcp_hdr->opcode);
	if (cr == BT_AVCTP_RESPONSE) {
		if (avrcp_hdr->opcode == BT_AVRCP_OPC_VENDOR_DEPENDENT &&
		    ctype == BT_AVRCP_CTYPE_CHANGED) {
			/* Status changed notifiation, do not reset timer */
		} else if (avrcp_hdr->opcode == BT_AVRCP_OPC_PASS_THROUGH) {
			/* No max response time for pass through commands  */
		} else if (tid != avrcp->req.tid || subunit_type != avrcp->req.subunit ||
			   avrcp_hdr->opcode != avrcp->req.opcode) {
			LOG_WRN("unexpected AVRCP response, expected tid:0x%X, subunit:0x%X, "
				"opc:0x%02X",
				avrcp->req.tid, avrcp->req.subunit, avrcp->req.opcode);
		} else {
			k_work_cancel_delayable(&avrcp->timeout_work);
		}
	}

	for (i = 0U; i < ARRAY_SIZE(handler); i++) {
		if (avrcp_hdr->opcode == handler[i].opcode) {
			handler[i].func(avrcp, buf, cr);
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

static struct net_buf *avrcp_create_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr)
{
	struct net_buf *buf;

	buf = bt_avctp_create_pdu(&(avrcp->session), cr, BT_AVCTP_PKT_TYPE_SINGLE,
				  BT_AVCTP_IPID_NONE, &avrcp->local_tid,
				  sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS));

	return buf;
}

static struct net_buf *avrcp_create_unit_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr)
{
	struct net_buf *buf;
	struct bt_avrcp_unit_info_cmd *cmd;

	buf = avrcp_create_pdu(avrcp, cr);
	if (!buf) {
		return buf;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE(&cmd->hdr, cr == BT_AVCTP_CMD ? BT_AVRCP_CTYPE_STATUS
							     : BT_AVRCP_CTYPE_IMPLEMENTED_STABLE);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_UNIT_INFO;

	return buf;
}

static struct net_buf *avrcp_create_subunit_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr)
{
	struct net_buf *buf;
	struct bt_avrcp_unit_info_cmd *cmd;

	buf = avrcp_create_pdu(avrcp, cr);
	if (!buf) {
		return buf;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE(&cmd->hdr, cr == BT_AVCTP_CMD ? BT_AVRCP_CTYPE_STATUS
							     : BT_AVRCP_CTYPE_IMPLEMENTED_STABLE);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_SUBUNIT_INFO;

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
	bt_avrcp_ctype_t ctype = BT_AVRCP_HDR_GET_CTYPE(avrcp_hdr);
	bt_avrcp_subunit_type_t subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);

	LOG_DBG("AVRCP send cr:0x%X, tid:0x%X, ctype: 0x%X, opc:0x%02X\n", cr, tid, ctype,
		avrcp_hdr->opcode);
	err = bt_avctp_send(&(avrcp->session), buf);
	if (err < 0) {
		return err;
	}

	if (cr == BT_AVCTP_CMD && avrcp_hdr->opcode != BT_AVRCP_OPC_PASS_THROUGH) {
		avrcp->req.tid = tid;
		avrcp->req.subunit = subunit_type;
		avrcp->req.opcode = avrcp_hdr->opcode;

		k_work_reschedule(&avrcp->timeout_work, AVRCP_TIMEOUT);
	}

	return 0;
}

int bt_avrcp_get_unit_info(struct bt_avrcp *avrcp)
{
	struct net_buf *buf;
	uint8_t param[5];

	buf = avrcp_create_unit_pdu(avrcp, BT_AVCTP_CMD);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	net_buf_add_mem(buf, param, sizeof(param));

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_get_subunit_info(struct bt_avrcp *avrcp)
{
	struct net_buf *buf;
	uint8_t param[5];

	buf = avrcp_create_subunit_pdu(avrcp, BT_AVCTP_CMD);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	param[0] = FIELD_PREP(GENMASK(6, 4), AVRCP_SUBUNIT_PAGE) |
		   FIELD_PREP(GENMASK(2, 0), AVRCP_SUBUNIT_EXTENSION_COED);
	net_buf_add_mem(buf, param, sizeof(param));

	return avrcp_send(avrcp, buf);
}

int bt_avrcp_register_cb(const struct bt_avrcp_cb *cb)
{
	avrcp_cb = cb;
	return 0;
}
