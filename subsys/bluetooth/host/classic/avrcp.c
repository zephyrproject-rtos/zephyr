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
	struct bt_avrcp_req ct_req;
	struct bt_avrcp_req tg_req;
	struct k_work_delayable ct_timeout_work;
	struct k_work_delayable tg_timeout_work;

	ATOMIC_DEFINE(flags, BT_AVRCP_NUM_FLAGS);

	uint8_t local_tid;
};

struct avrcp_handler {
	bt_avrcp_opcode_t opcode;
	void (*func)(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr);
};

#define AVRCP_TIMEOUT       K_SECONDS(3) /* Shell be greater than TMTP (1000ms) */
#define AVRCP_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, session)
#define AVRCP_KTGWORK(_work)                                                                    \
	CONTAINER_OF(CONTAINER_OF(_work, struct k_work_delayable, work), struct bt_avrcp,          \
		     tg_timeout_work)
#define AVRCP_KCTWORK(_work)                                                                    \
	CONTAINER_OF(CONTAINER_OF(_work, struct k_work_delayable, work), struct bt_avrcp,          \
		     ct_timeout_work)

static const struct bt_avrcp_cb *avrcp_cb;
static struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_CONN];
static int bt_avrcp_send_unit_info_err_rsp(struct bt_avrcp *avrcp);

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

static void avrcp_ct_timeout(struct k_work *work)
{
	struct bt_avrcp *avrcp = AVRCP_KCTWORK(work);

	LOG_WRN("Controller timeout: tid 0x%X, opc 0x%02X", avrcp->ct_req.tid,
									avrcp->ct_req.opcode);
}

static void avrcp_tg_timeout(struct k_work *work)
{
	struct bt_avrcp *avrcp = AVRCP_KTGWORK(work);

	LOG_WRN("Target timeout: t tid 0x%X, opc 0x%02X", avrcp->tg_req.tid, avrcp->tg_req.opcode);

	if (atomic_test_and_clear_bit(avrcp->flags, BT_AVRCP_TG_REQ)) {
		memset(&avrcp->tg_req, 0, sizeof(struct bt_avrcp_req));
	}
}

/* The AVCTP L2CAP channel established */
static void avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->connected != NULL)) {
		avrcp_cb->connected(avrcp);
	}

	k_work_init_delayable(&avrcp->ct_timeout_work, avrcp_ct_timeout);
	k_work_init_delayable(&avrcp->tg_timeout_work, avrcp_tg_timeout);

}

/* The AVCTP L2CAP channel released */
static void avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->disconnected != NULL)) {
		avrcp_cb->disconnected(avrcp);
	}
}

static void avrcp_ct_vendor_dependent_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static void avrcp_ct_unit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
						bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_unit_info_rsp rsp;

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->unit_info_rsp != NULL)) {
			net_buf_pull(buf, sizeof(*avrcp_hdr));
			if (buf->len != 5) {
				LOG_ERR("Invalid unit info length: %d", buf->len);
				return;
			}
			net_buf_pull_u8(buf); /* Always 0x07 */
			rsp.unit_type = FIELD_GET(GENMASK(7, 3), net_buf_pull_u8(buf));
			rsp.company_id = net_buf_pull_be24(buf);
			avrcp_cb->unit_info_rsp(avrcp, &rsp);
		}
	}
}

static void avrcp_ct_subunit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
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
			avrcp_cb->subunit_info_rsp(avrcp, &rsp);
		}
	}
}

static void avrcp_ct_pass_through_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_passthrough_rsp rsp;
	uint8_t tmp;

	if (cr == BT_AVCTP_CMD) {
		/* ToDo */
	} else { /* BT_AVCTP_RESPONSE */
		if ((avrcp_cb != NULL) && (avrcp_cb->subunit_info_rsp != NULL)) {
			avrcp_hdr = (struct bt_avrcp_header *)buf->data;
			net_buf_pull(buf, sizeof(*avrcp_hdr));
			if (buf->len < 2) {
				LOG_ERR("Invalid passthrough length: %d", buf->len);
				return;
			}

			rsp.response = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
			tmp = net_buf_pull_u8(buf);
			rsp.operation_id = FIELD_GET(GENMASK(6, 0), tmp);
			rsp.state = FIELD_GET(GENMASK(7, 7), tmp);
			rsp.len = net_buf_pull_u8(buf);
			rsp.payload = rsp.len ? buf->data : NULL;

			avrcp_cb->passthrough_rsp(avrcp, &rsp);
		}
	}
}

static const struct avrcp_handler ct_handler[] = {
	{ BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_ct_vendor_dependent_handler },
	{ BT_AVRCP_OPC_UNIT_INFO, avrcp_ct_unit_info_handler },
	{ BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_ct_subunit_info_handler },
	{ BT_AVRCP_OPC_PASS_THROUGH, avrcp_ct_pass_through_handler },
};

static void avrcp_tg_vendor_dependent_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static void avrcp_tg_unit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
						bt_avctp_cr_t cr)
{
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	int err;

	while (1) {
		if (cr == BT_AVCTP_CMD) {
			if ((avrcp_cb != NULL) && (avrcp_cb->unit_info_req != NULL)) {
				if (buf->len < sizeof(*avrcp_hdr)) {
					break;
				}
				avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

				if (buf->len != 5) {
					LOG_ERR("Invalid unit info length");
					break;
				}

				subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);
				if ((subunit_type != BT_AVRCP_SUBUNIT_TYPE_UNIT) ||
					(avrcp_hdr->opcode != BT_AVRCP_OPC_UNIT_INFO)) {
					LOG_ERR("Invalid unit info command");
					break;
				}
				avrcp_cb->unit_info_req(avrcp);
				return;
			}
		} else { /* BT_AVCTP_RESPONSE */
			LOG_ERR("Invalid unit info command");
			return;
		}
	}

	err = bt_avrcp_send_unit_info_err_rsp(avrcp);
	if (err) {
		LOG_ERR("Failed to send unit info error response");
	}
}

static void avrcp_tg_subunit_info_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static void avrcp_tg_pass_through_handler(struct bt_avrcp *avrcp, struct net_buf *buf,
					   bt_avctp_cr_t cr)
{
	/* ToDo */
}

static const struct avrcp_handler tg_handler[] = {
	{ BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_tg_vendor_dependent_handler },
	{ BT_AVRCP_OPC_UNIT_INFO, avrcp_tg_unit_info_handler },
	{ BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_tg_subunit_info_handler },
	{ BT_AVRCP_OPC_PASS_THROUGH, avrcp_tg_pass_through_handler },
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

	if (buf->len < sizeof(struct bt_avctp_header)) {
		LOG_ERR("invalid AVCTP header received");
		return -EINVAL;
	}
	avctp_hdr = (void *)buf->data;
	net_buf_pull(buf, sizeof(*avctp_hdr));
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
	if (cr == BT_AVCTP_RESPONSE) {
		if (avrcp_hdr->opcode == BT_AVRCP_OPC_VENDOR_DEPENDENT &&
		    rsp == BT_AVRCP_RSP_CHANGED) {
			/* Status changed notifiation, do not reset timer */
		} else if (avrcp_hdr->opcode == BT_AVRCP_OPC_PASS_THROUGH) {
			/* No max response time for pass through commands  */
		} else if (tid != avrcp->ct_req.tid || subunit_type != avrcp->ct_req.subunit ||
			   avrcp_hdr->opcode != avrcp->ct_req.opcode) {
			LOG_WRN("unexpected AVRCP response, expected tid:0x%X, subunit:0x%X, "
				"opc:0x%02X",
				avrcp->ct_req.tid, avrcp->ct_req.subunit, avrcp->ct_req.opcode);
		} else {
			k_work_cancel_delayable(&avrcp->ct_timeout_work);
		}
		for (i = 0U; i < ARRAY_SIZE(ct_handler); i++) {
			if (avrcp_hdr->opcode == ct_handler[i].opcode) {
				ct_handler[i].func(avrcp, buf, cr);
				return 0;
			}
		}
	} else if (cr == BT_AVCTP_CMD) {
		if (atomic_test_and_set_bit(avrcp->flags, BT_AVRCP_TG_REQ)) {
			LOG_WRN("An AVRCP request is already ongoing.");
			return -EALREADY;
		}
		if (avrcp_hdr->opcode == BT_AVRCP_OPC_PASS_THROUGH) {
			/* No max response time for pass through commands  */
		} else {
			k_work_reschedule(&avrcp->tg_timeout_work, AVRCP_TIMEOUT);
		}
		avrcp->tg_req.tid = tid;
		avrcp->tg_req.subunit = subunit_type;
		avrcp->tg_req.opcode = avrcp_hdr->opcode;

		for (i = 0U; i < ARRAY_SIZE(tg_handler); i++) {
			if (avrcp_hdr->opcode == tg_handler[i].opcode) {
				tg_handler[i].func(avrcp, buf, cr);
				return 0;
			}
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
	uint8_t tid = (cr == BT_AVCTP_CMD) ? avrcp->local_tid : avrcp->tg_req.tid;

	buf = bt_avctp_create_pdu(&(avrcp->session), cr, BT_AVCTP_PKT_TYPE_SINGLE,
				  BT_AVCTP_IPID_NONE, &tid,
				  sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS));

	return buf;
}

static struct net_buf *avrcp_create_unit_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr,
					uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, ctype_or_rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_UNIT_INFO;

	return buf;
}

static struct net_buf *avrcp_create_subunit_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, cr);
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

static struct net_buf *avrcp_create_passthrough_pdu(struct bt_avrcp *avrcp, bt_avctp_cr_t cr,
						    bt_avrcp_ctype_t ctype)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = avrcp_create_pdu(avrcp, cr);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, ctype);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	cmd->hdr.opcode = BT_AVRCP_OPC_PASS_THROUGH;

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
	bt_avrcp_rsp_t rsp = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	bt_avrcp_subunit_type_t subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);

	LOG_DBG("AVRCP send cr:0x%X, tid:0x%X, ctype: 0x%X, opc:0x%02X\n", cr, tid, ctype,
		avrcp_hdr->opcode);
	if (cr == BT_AVCTP_RESPONSE) {
		if (avrcp_hdr->opcode == BT_AVRCP_OPC_VENDOR_DEPENDENT &&
		    rsp == BT_AVRCP_RSP_CHANGED) {
			/* Status changed notifiation, do not reset timer */
		} else if (avrcp_hdr->opcode == BT_AVRCP_OPC_PASS_THROUGH) {
			/* No max response time for pass through commands  */
		} else if (tid != avrcp->tg_req.tid || subunit_type != avrcp->tg_req.subunit ||
			   avrcp_hdr->opcode != avrcp->tg_req.opcode) {
			LOG_WRN("unexpected AVRCP response, expected tid:0x%X, subunit:0x%X, "
				"opc:0x%02X",
				avrcp->tg_req.tid, avrcp->tg_req.subunit, avrcp->tg_req.opcode);
		} else {
			k_work_cancel_delayable(&avrcp->tg_timeout_work);
		}
		atomic_clear_bit(avrcp->flags, BT_AVRCP_TG_REQ);
	}
	err = bt_avctp_send(&(avrcp->session), buf);
	if (err < 0) {
		net_buf_unref(buf);
		LOG_ERR("AVCTP send fail, err = %d", err);
		return err;
	}

	if (cr == BT_AVCTP_CMD && avrcp_hdr->opcode != BT_AVRCP_OPC_PASS_THROUGH) {
		avrcp->ct_req.tid = tid;
		avrcp->ct_req.subunit = subunit_type;
		avrcp->ct_req.opcode = avrcp_hdr->opcode;

		k_work_reschedule(&avrcp->ct_timeout_work, AVRCP_TIMEOUT);
	}

	return 0;
}

static int bt_avrcp_send_unit_info_err_rsp(struct bt_avrcp *avrcp)
{
	struct net_buf *buf;

	buf = avrcp_create_unit_pdu(avrcp, BT_AVCTP_RESPONSE, BT_AVRCP_RSP_REJECTED);
	if (!buf) {
		LOG_WRN("Insufficient buffer");
		return -ENOMEM;
	}

	return avrcp_send(avrcp, buf);
}
int bt_avrcp_get_unit_info(struct bt_avrcp *avrcp)
{
	struct net_buf *buf;
	uint8_t param[5];

	buf = avrcp_create_unit_pdu(avrcp, BT_AVCTP_CMD, BT_AVRCP_CTYPE_STATUS);
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

int bt_avrcp_passthrough(struct bt_avrcp *avrcp, bt_avrcp_opid_t operation_id,
			 bt_avrcp_button_state_t state, const uint8_t *payload, uint8_t len)
{
	struct net_buf *buf;
	uint8_t param[2];

	buf = avrcp_create_passthrough_pdu(avrcp, BT_AVCTP_CMD, BT_AVRCP_CTYPE_CONTROL);
	if (!buf) {
		return -ENOMEM;
	}

	param[0] = FIELD_PREP(GENMASK(7, 7), state) | FIELD_PREP(GENMASK(6, 0), operation_id);
	param[1] = len;
	net_buf_add_mem(buf, param, sizeof(param));
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

int bt_avrcp_send_unit_info_rsp(struct bt_avrcp *avrcp, struct bt_avrcp_unit_info_rsp *rsp)
{
	struct net_buf *buf;

	buf = avrcp_create_unit_pdu(avrcp, BT_AVCTP_RESPONSE, BT_AVRCP_RSP_STABLE);
	if (!buf) {
		LOG_WRN("Insufficient buffer");
		return -ENOMEM;
	}

	/* The 0x7 is hard-coded in the spec. */
	net_buf_add_u8(buf, 0x07);
	/* Add Unit Type info */
	net_buf_add_u8(buf, FIELD_PREP(GENMASK(7, 3), (rsp->unit_type)));
	/* Company ID */
	net_buf_add_be24(buf, (rsp->company_id));

	return avrcp_send(avrcp, buf);
}
