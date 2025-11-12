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
	/* ACL connection handle */
	struct bt_conn *acl_conn;

	struct bt_avctp browsing_session;
};

struct bt_avrcp_ct {
	struct bt_avrcp *avrcp;

	struct net_buf *reassembly_buf;	/**< Buffer for reassembling fragments */

	struct bt_avrcp_ct_notify_registration ct_notify[BT_AVRCP_EVT_VOLUME_CHANGED + 1];
};

struct bt_avrcp_tg {
	struct bt_avrcp *avrcp;

	/* AVRCP vendor dependent response TX pending */
	sys_slist_t vd_rsp_tx_pending;

	/* Critical locker */
	struct k_sem lock;

	/* AVRCP vendor dependent response TX work */
	struct k_work_delayable vd_rsp_tx_work;

	struct bt_avrcp_tg_notify_state tg_notify[BT_AVRCP_EVT_VOLUME_CHANGED + 1];
};

struct avrcp_handler {
	bt_avrcp_opcode_t opcode;
	void (*func)(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf);
};

struct avrcp_pdu_vendor_handler  {
	bt_avrcp_pdu_id_t pdu_id;
	uint8_t min_len;
	bt_avrcp_ctype_t cmd_type;
	int (*func)(struct bt_avrcp *avrcp, uint8_t tid, uint8_t result, struct net_buf *buf);
};

NET_BUF_POOL_FIXED_DEFINE(avrcp_vd_rx_pool, CONFIG_BT_MAX_CONN,
			  CONFIG_BT_AVRCP_VD_RX_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct avrcp_pdu_handler {
	bt_avrcp_pdu_id_t pdu_id;
	uint8_t min_len;
	int (*func)(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf);
};

#define AVRCP_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, session)
#define AVRCP_BROW_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, browsing_session)

NET_BUF_POOL_FIXED_DEFINE(avctp_ctrl_rx_pool, CONFIG_BT_MAX_CONN, BT_AVRCP_FRAGMENT_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_FIXED_DEFINE(avctp_ctrl_tx_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_DEFINE(avctp_browsing_rx_pool, BT_BUF_ACL_RX_COUNT,
		    CONFIG_BT_AVRCP_BROWSING_L2CAP_MTU,
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/*
 * This macros returns true if the CT/TG has been initialized, which
 * typically happens after the avrcp callack have been registered.
 * Use these macros to determine whether the CT/TG role is supported.
 */
#define IS_CT_ROLE_SUPPORTED() (avrcp_ct_cb != NULL)
#define IS_TG_ROLE_SUPPORTED() (avrcp_tg_cb != NULL)

#define AVRCP_TX_RETRY_DELAY_MS (100)

#define AVRCP_STATUS_IS_REJECTED(status) (\
	((uint8_t)(status) <= BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED) &&   \
	((uint8_t)(status) != BT_AVRCP_STATUS_OPERATION_COMPLETED)           \
)

static const struct bt_avrcp_ct_cb *avrcp_ct_cb;
static const struct bt_avrcp_tg_cb *avrcp_tg_cb;
static struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_CONN];
static struct bt_avrcp_ct bt_avrcp_ct_pool[CONFIG_BT_MAX_CONN];
static struct bt_avrcp_tg bt_avrcp_tg_pool[CONFIG_BT_MAX_CONN];

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
static uint16_t bt_avrcp_tg_cover_art_psm;
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

static struct bt_avctp_server avctp_server;
#if defined(CONFIG_BT_AVRCP_BROWSING)
static struct bt_avctp_server avctp_browsing_server;
#endif /* CONFIG_BT_AVRCP_BROWSING */

static void avrcp_tx_buf_destroy(struct net_buf *buf)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_avrcp_tg *tg = &bt_avrcp_tg_pool[i];

		if ((tg->avrcp != NULL) && (tg->avrcp->acl_conn != NULL)) {
			(void)k_work_reschedule(&tg->vd_rsp_tx_work, K_NO_WAIT);
		}
	}

	net_buf_destroy(buf);
}

NET_BUF_POOL_FIXED_DEFINE(avrcp_vd_tx_pool, CONFIG_BT_MAX_CONN,
			  BT_L2CAP_BUF_SIZE(BT_AVRCP_FRAGMENT_SIZE) +
			  sizeof(struct bt_avctp_header_start),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, avrcp_tx_buf_destroy);

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
	/* Browsing channel */
#if defined(CONFIG_BT_AVRCP_BROWSING) || defined(CONFIG_BT_AVRCP_TG_COVER_ART)
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
#if defined(CONFIG_BT_AVRCP_BROWSING) && defined(CONFIG_BT_AVRCP_TG_COVER_ART)
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 33),
#elif defined(CONFIG_BT_AVRCP_BROWSING)
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 18),
#elif defined(CONFIG_BT_AVRCP_TG_COVER_ART)
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
#else
#error Invalid configuration
#endif /* CONFIG_BT_AVRCP_BROWSING && CONFIG_BT_AVRCP_TG_COVER_ART */
		BT_SDP_DATA_ELEM_LIST(
#if defined(CONFIG_BT_AVRCP_BROWSING)
		{
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
					BT_SDP_ARRAY_16(BT_L2CAP_PSM_AVRCP_BROWSING)
				})
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
				})
			})
		}
#endif /* CONFIG_BT_AVRCP_BROWSING */
#if defined(CONFIG_BT_AVRCP_BROWSING) && defined(CONFIG_BT_AVRCP_TG_COVER_ART)
		,
#endif /* CONFIG_BT_AVRCP_BROWSING && CONFIG_BT_AVRCP_TG_COVER_ART */
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
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
					&bt_avrcp_tg_cover_art_psm
				})
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST(
				{
					BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					BT_SDP_ARRAY_16(BT_UUID_OBEX_VAL)
				})
			})
		}
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
		)
	),
#endif /* CONFIG_BT_AVRCP_BROWSING || CONFIG_BT_AVRCP_TG_COVER_ART*/
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
	BT_SDP_SUPPORTED_FEATURES(AVRCP_TG_CAT_1 | AVRCP_TG_CAT_2 | AVRCP_TG_BROWSING_ENABLE |
				  AVRCP_TG_COVER_ART_ENABLE),
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
	/* Browsing channel */
#if defined(CONFIG_BT_AVRCP_BROWSING)
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 18),
		BT_SDP_DATA_ELEM_LIST(
		{
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
					BT_SDP_ARRAY_16(BT_L2CAP_PSM_AVRCP_BROWSING)
				})
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
				})
			})
		})
	),
#endif /* CONFIG_BT_AVRCP_BROWSING */
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
	BT_SDP_SUPPORTED_FEATURES(AVRCP_CT_CAT_1 | AVRCP_CT_CAT_2 | AVRCP_CT_BROWSING_ENABLE |
				  AVRCP_CT_COVER_ART_FEAT_ENABLE),
	BT_SDP_SERVICE_NAME("AVRCP Controller"),
};

static struct bt_sdp_record avrcp_ct_rec = BT_SDP_RECORD(avrcp_ct_attrs);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

static void avrcp_tg_lock(struct bt_avrcp_tg *tg)
{
	k_sem_take(&tg->lock, K_FOREVER);
}

static void avrcp_tg_unlock(struct bt_avrcp_tg *tg)
{
	k_sem_give(&tg->lock);
}

static struct bt_avrcp *avrcp_get_connection(struct bt_conn *conn)
{
	size_t index;

	if (!conn) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(avrcp_connection), "Conn index is out of bounds");

	return &avrcp_connection[index];
}

static inline struct bt_avrcp_ct *get_avrcp_ct(struct bt_avrcp *avrcp)
{
	size_t index;

	if (avrcp == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	index = (size_t)bt_conn_index(avrcp->acl_conn);
	__ASSERT(index < ARRAY_SIZE(avrcp_connection), "Conn index is out of bounds");

	return &bt_avrcp_ct_pool[index];
}

static inline struct bt_avrcp_tg *get_avrcp_tg(struct bt_avrcp *avrcp)
{
	size_t index;

	if (avrcp == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	index = (size_t)bt_conn_index(avrcp->acl_conn);
	__ASSERT(index < ARRAY_SIZE(avrcp_connection), "Conn index is out of bounds");

	return &bt_avrcp_tg_pool[index];
}

static int bt_avrcp_rsp_to_status(uint8_t rsp, struct net_buf *buf, uint8_t *status)
{
	if ((status == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	switch (rsp) {
	case BT_AVRCP_RSP_STABLE:
		*status = BT_AVRCP_STATUS_SUCCESS;
		break;
	case BT_AVRCP_RSP_IN_TRANSITION:
		*status = BT_AVRCP_STATUS_IN_TRANSITION;
		break;
	case BT_AVRCP_RSP_ACCEPTED:
		*status = BT_AVRCP_STATUS_SUCCESS;
		break;
	case BT_AVRCP_RSP_NOT_IMPLEMENTED:
		*status = BT_AVRCP_STATUS_NOT_IMPLEMENTED;
		break;
	case BT_AVRCP_RSP_REJECTED:
		if (buf->len < sizeof(*status)) {
			LOG_ERR("AVRCP rejected response missing error code");
			*status = BT_AVRCP_STATUS_INTERNAL_ERROR;
			return -EINVAL;
		}
		*status = net_buf_pull_u8(buf);
		break;
	default:
		*status = BT_AVRCP_STATUS_INTERNAL_ERROR;
		break;
	}

	return 0;
}

/* The AVCTP L2CAP channel established */
static void avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->connected != NULL)) {
		avrcp_ct_cb->connected(session->br_chan.chan.conn, get_avrcp_ct(avrcp));
	}

	if ((avrcp_tg_cb != NULL) && (avrcp_tg_cb->connected != NULL)) {
		avrcp_tg_cb->connected(session->br_chan.chan.conn, get_avrcp_tg(avrcp));
	}
}

/* The AVCTP L2CAP channel released */
static void avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->disconnected != NULL)) {
		avrcp_ct_cb->disconnected(get_avrcp_ct(avrcp));
	}

	if ((avrcp_tg_cb != NULL) && (avrcp_tg_cb->disconnected != NULL)) {
		avrcp_tg_cb->disconnected(get_avrcp_tg(avrcp));
	}

	if (avrcp->acl_conn != NULL) {
		bt_conn_unref(avrcp->acl_conn);
		avrcp->acl_conn = NULL;
	}
}

static struct net_buf *avrcp_create_unit_pdu(struct bt_avrcp *avrcp, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = bt_avctp_create_pdu(NULL);
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

static struct net_buf *avrcp_create_subunit_pdu(struct bt_avrcp *avrcp, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = bt_avctp_create_pdu(NULL);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(&cmd->hdr, ctype_or_rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(&cmd->hdr, BT_AVRCP_SUBUNIT_ID_IGNORE);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(&cmd->hdr, BT_AVRCP_SUBUNIT_TYPE_UNIT);
	cmd->hdr.opcode = BT_AVRCP_OPC_SUBUNIT_INFO;

	return buf;
}

static void avrcp_set_passthrough_header(struct bt_avrcp_header *hdr, uint8_t ctype_or_rsp)
{
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(hdr, ctype_or_rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	hdr->opcode = BT_AVRCP_OPC_PASS_THROUGH;
}

static struct net_buf *avrcp_create_passthrough_pdu(struct bt_avrcp *avrcp, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = bt_avctp_create_pdu(NULL);
	if (!buf) {
		return NULL;
	}

	cmd = net_buf_add(buf, sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	avrcp_set_passthrough_header(&cmd->hdr, ctype_or_rsp);

	return buf;
}

static struct net_buf *avrcp_create_vendor_pdu(struct bt_avrcp *avrcp, uint8_t ctype_or_rsp)
{
	struct net_buf *buf;
	struct bt_avrcp_frame *cmd;

	buf = bt_avctp_create_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
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

static int avrcp_send(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid)
{
	int err;
	struct bt_avrcp_header *avrcp_hdr =
		(struct bt_avrcp_header *)(buf->data);
	bt_avrcp_ctype_t ctype = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);

	LOG_DBG("AVRCP send cr:0x%X, tid:0x%X, ctype: 0x%X, opc:0x%02X\n", cr, tid, ctype,
		avrcp_hdr->opcode);
	err = bt_avctp_send(&(avrcp->session), buf, cr, tid);
	if (err < 0) {
		LOG_ERR("AVCTP send fail, err = %d", err);
		return err;
	}

	return 0;
}

static struct net_buf *avrcp_create_browsing_pdu(struct bt_avrcp *avrcp)
{
	return bt_avrcp_create_pdu(NULL);
}

static int avrcp_browsing_send(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr,
			       uint8_t tid)
{
	int err;
	struct bt_avrcp_brow_pdu *hdr =
		(struct bt_avrcp_brow_pdu *)(buf->data);

	LOG_DBG("AVRCP browsing send cr:0x%X, tid:0x%X, pdu_id:0x%02X\n", cr, tid,
		hdr->pdu_id);
	err = bt_avctp_send(&(avrcp->browsing_session), buf, cr, tid);
	if (err < 0) {
		LOG_ERR("AVCTP browsing send fail, err = %d", err);
		return err;
	}

	return 0;
}

static int bt_avrcp_send_unit_info_err_rsp(struct bt_avrcp *avrcp, uint8_t tid)
{
	struct net_buf *buf;
	int err;

	buf = avrcp_create_unit_pdu(avrcp, BT_AVRCP_RSP_REJECTED);
	if (!buf) {
		LOG_WRN("Insufficient buffer");
		return -ENOMEM;
	}

	err = avrcp_send(avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

static void avrcp_fill_subunit_info_param(uint8_t param[BT_AVRCP_SUBUNIT_INFO_RSP_SIZE],
					  uint8_t subunit_type, uint8_t max_subunit_id)
{
	memset(param, 0xFF, BT_AVRCP_SUBUNIT_INFO_RSP_SIZE);

	param[0] = FIELD_PREP(GENMASK(6, 4), AVRCP_SUBUNIT_PAGE) |
		   FIELD_PREP(GENMASK(2, 0), AVRCP_SUBUNIT_EXTENSION_CODE);

	param[1] = FIELD_PREP(GENMASK(7, 3), subunit_type) |
		   FIELD_PREP(GENMASK(2, 0), max_subunit_id);
}

static int bt_avrcp_send_subunit_info(struct bt_avrcp *avrcp, uint8_t tid, uint8_t rsp_type)
{
	struct net_buf *buf;
	uint8_t param[BT_AVRCP_SUBUNIT_INFO_RSP_SIZE] = {0};
	int err;

	buf = avrcp_create_subunit_pdu(avrcp, rsp_type);
	if (buf == NULL) {
		LOG_WRN("Insufficient buffer");
		return -ENOMEM;
	}

	/* If the unit implements this profile, it shall return PANEL subunit in the subunit_type
	 * field, and value 0 in the max_subunit_ID field in the response frame.
	 */
	avrcp_fill_subunit_info_param(param, BT_AVRCP_SUBUNIT_TYPE_PANEL, 0);

	if (net_buf_tailroom(buf) < sizeof(param)) {
		LOG_WRN("Not enough tailroom in buffer");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_mem(buf, param, sizeof(param));

	err = avrcp_send(avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

static int init_fragmentation_context(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t pdu_id,
				      uint8_t rsp)
{
	struct bt_avrcp_ct_frag_reassembly_ctx *ctx;

	if (ct == NULL) {
		return -EINVAL;
	}

	/* Clean up any existing reassembly buffer */
	if (ct->reassembly_buf != NULL) {
		LOG_WRN("Interleaving fragments not allowed (tid=%u, rsp=%u)", tid, rsp);
		net_buf_unref(ct->reassembly_buf);
		ct->reassembly_buf = NULL;
	}

	/* Allocate reassembly buffer */
	ct->reassembly_buf = net_buf_alloc(&avrcp_vd_rx_pool, K_NO_WAIT);
	if (ct->reassembly_buf == NULL) {
		LOG_ERR("Failed to allocate reassembly buffer");
		return -ENOBUFS;
	}

	__ASSERT_NO_MSG(ct->reassembly_buf->user_data_size >= sizeof(*ctx));
	ctx = net_buf_user_data(ct->reassembly_buf);

	ctx->tid = tid;
	ctx->pdu_id = pdu_id;
	ctx->rsp = rsp;
	return 0;
}

static struct bt_avrcp_ct_frag_reassembly_ctx *get_fragmentation_context(struct bt_avrcp_ct *ct)
{
	struct bt_avrcp_ct_frag_reassembly_ctx *ctx;

	if (ct == NULL) {
		return NULL;
	}

	ctx = net_buf_user_data(ct->reassembly_buf);

	return ctx;
}

static int add_fragment_data(struct bt_avrcp_ct *ct, const uint8_t *data, uint16_t data_len)
{
	if ((ct == NULL) || (data == NULL) || (ct->reassembly_buf == NULL)) {
		return -EINVAL;
	}

	if (net_buf_tailroom(ct->reassembly_buf) < data_len) {
		LOG_ERR("Insufficient space in reassembly buffer");
		return -ENOMEM;
	}

	/* Add fragment data to reassembly buffer */
	net_buf_add_mem(ct->reassembly_buf, data, data_len);
	return 0;
}

static void cleanup_fragmentation_context(struct bt_avrcp_ct *ct)
{
	if (ct == NULL) {
		return;
	}

	if (ct->reassembly_buf != NULL) {
		net_buf_unref(ct->reassembly_buf);
		ct->reassembly_buf = NULL;
	}
}

static struct net_buf *avrcp_prepare_vendor_pdu(struct bt_avrcp *avrcp,
						bt_avrcp_pkt_type_t pkt_type, uint8_t avrcp_type,
						uint8_t pdu_id, uint16_t param_len)
{
	struct net_buf *buf;
	struct bt_avrcp_avc_vendor_pdu *pdu;

	if (avrcp == NULL) {
		return NULL;
	}

	buf = avrcp_create_vendor_pdu(avrcp, avrcp_type);
	if (buf == NULL) {
		LOG_WRN("Insufficient buffer");
		return NULL;
	}

	if (net_buf_tailroom(buf) < sizeof(*pdu)) {
		LOG_WRN("Not enough tailroom: required");
		net_buf_unref(buf);
		return NULL;
	}

	pdu = net_buf_add(buf, sizeof(*pdu));
	sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, pdu->company_id);
	pdu->pdu_id = pdu_id;
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, pkt_type);
	pdu->param_len = sys_cpu_to_be16(param_len);
	if (net_buf_tailroom(buf) < param_len) {
		LOG_WRN("Not enough tailroom: required for param");
		net_buf_unref(buf);
		return NULL;
	}
	return buf;
}

static int send_single_vendor_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t rsp, uint8_t pdu_id,
				  struct net_buf *buf)
{
	struct bt_avrcp_header *hdr;
	struct bt_avrcp_avc_vendor_pdu *pdu;
	uint16_t param_len;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	param_len = buf->len;

	if (net_buf_headroom(buf) < (sizeof(*pdu) + sizeof(*hdr))) {
		LOG_WRN("Not enough headroom: for vendor dependent PDU");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	pdu = net_buf_push(buf, sizeof(*pdu));
	sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, pdu->company_id);
	pdu->pdu_id = pdu_id;
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, BT_AVRCP_PKT_TYPE_SINGLE);
	pdu->param_len = sys_cpu_to_be16(param_len);

	hdr = net_buf_push(buf, sizeof(struct bt_avrcp_header));
	memset(hdr, 0, sizeof(struct bt_avrcp_header));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(hdr, rsp);
	BT_AVRCP_HDR_SET_SUBUNIT_ID(hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	hdr->opcode = BT_AVRCP_OPC_VENDOR_DEPENDENT;

	return avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
}

static int send_fragmented_vendor_rsp(struct bt_avrcp_tg_vd_rsp_tx *tx,
				      bt_avrcp_pkt_type_t pkt_type,
				      uint8_t *data, uint16_t data_len)
{
	struct bt_avrcp_tg *tg = tx->tg;
	struct net_buf *buf;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	buf = avrcp_prepare_vendor_pdu(tg->avrcp, pkt_type, tx->rsp, tx->pdu_id, data_len);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (data_len > 0U) {
		net_buf_add_mem(buf, data, data_len);
	}
	return avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tx->tid);
}

static int bt_avrcp_ct_continuing_rsp_cmd_send(struct bt_avrcp_ct *ct, uint8_t tid,
					       uint8_t op_pdu_id, uint8_t target_pdu_id)
{
	struct net_buf *buf;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	buf = avrcp_prepare_vendor_pdu(ct->avrcp, BT_AVRCP_PKT_TYPE_SINGLE,
				       BT_AVRCP_CTYPE_CONTROL, op_pdu_id,
				       sizeof(target_pdu_id));
	if (buf == NULL) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, target_pdu_id);

	return avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
}

static int bt_avrcp_ct_send_req_continuing_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t pdu_id)
{
	return bt_avrcp_ct_continuing_rsp_cmd_send(ct, tid, BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP,
						   pdu_id);
}

static int bt_avrcp_ct_send_abort_continuing_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
						 uint8_t pdu_id)
{
	return bt_avrcp_ct_continuing_rsp_cmd_send(ct, tid, BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP,
						   pdu_id);
}

static void bt_avrcp_tg_process_continuing_cmd(struct bt_avrcp_tg *tg, bool abort_continuing,
					       uint8_t tid, struct net_buf *buf)
{
	sys_snode_t *node;
	struct bt_avrcp_tg_vd_rsp_tx *tx;
	struct net_buf *tx_buf;
	uint8_t pdu_id;

	avrcp_tg_lock(tg);

	node = sys_slist_peek_head(&tg->vd_rsp_tx_pending);
	if (!node) {
		LOG_WRN("No pending tx");
		avrcp_tg_unlock(tg);
		return;
	}

	tx_buf = CONTAINER_OF(node, struct net_buf, node);

	__ASSERT_NO_MSG(tx_buf->user_data_size >= sizeof(*tx));
	tx = net_buf_user_data(tx_buf);

	if (buf->len < sizeof(pdu_id)) {
		LOG_WRN("Invalid AVRCP buffer: no PDU id");
		avrcp_tg_unlock(tg);
		return;
	}
	pdu_id = net_buf_pull_u8(buf);

	/* Clear TX_ONGOING and update TID */
	atomic_clear_bit(tx->flags, BT_AVRCP_TG_FLAG_TX_ONGOING);
	tx->tid = tid;

	if (abort_continuing) {
		/* ABORT_CONTINUING: set abort flag, clear sending flag */
		atomic_set_bit(tx->flags, BT_AVRCP_TG_FLAG_ABORT_CONTINUING);

		/* Same PDU is ACCEPTED; otherwise REJECTED */
		tx->rsp = (pdu_id == tx->pdu_id) ? BT_AVRCP_RSP_ACCEPTED : BT_AVRCP_RSP_REJECTED;
		tx->pdu_id = BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP;
	} else {
		/* Different PDU REJECTED with CONTINUING_RSP PDU ID */
		if (pdu_id != tx->pdu_id) {
			tx->rsp = BT_AVRCP_RSP_REJECTED;
			tx->pdu_id = BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP;
		}
	}

	LOG_DBG("Continuing cmd processed: abort=%d, pdu_id=0x%02x", abort_continuing, pdu_id);

	avrcp_tg_unlock(tg);
}

static void avrcp_tg_tx_remove(struct bt_avrcp_tg *tg, struct net_buf *buf)
{
	avrcp_tg_lock(tg);
	sys_slist_find_and_remove(&tg->vd_rsp_tx_pending, &buf->node);
	avrcp_tg_unlock(tg);
}

static void bt_avrcp_tg_vendor_tx_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_avrcp_tg *tg = CONTAINER_OF(dwork, struct bt_avrcp_tg, vd_rsp_tx_work);
	sys_snode_t *node;
	struct net_buf *buf;
	struct bt_avrcp_tg_vd_rsp_tx *tx;
	uint16_t max_payload_size;
	int err;

	avrcp_tg_lock(tg);

	node = sys_slist_peek_head(&tg->vd_rsp_tx_pending);
	if (node == NULL) {
		LOG_DBG("No pending tx");
		avrcp_tg_unlock(tg);
		return;
	}
	avrcp_tg_unlock(tg);

	buf = CONTAINER_OF(node, struct net_buf, node);

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*tx));
	tx = net_buf_user_data(buf);

	if (atomic_test_bit(tx->flags, BT_AVRCP_TG_FLAG_TX_ONGOING)) {
		return;
	}
	/* Calculate maximum payload size per fragment */
	max_payload_size = BT_AVRCP_FRAGMENT_SIZE - sizeof(struct bt_avrcp_header)
			   - sizeof(struct bt_avrcp_avc_vendor_pdu);

	/* Check if fragmentation is needed */
	if ((tx->sent_len == 0) && (buf->len <= max_payload_size)) {
		/* Single packet response */
		err = send_single_vendor_rsp(tg, tx->tid, tx->rsp, tx->pdu_id, buf);
		if (err < 0) {
			LOG_ERR("Failed to send single len: %u", buf->len);
			goto done;
		}
		avrcp_tg_tx_remove(tg, buf);
		k_work_reschedule(&tg->vd_rsp_tx_work, K_NO_WAIT);
		return;
	}
	/* Multi-packet fragmented response */
	bt_avrcp_pkt_type_t pkt_type = BT_AVRCP_PKT_TYPE_SINGLE;
	uint16_t chunk_size = MIN(max_payload_size, buf->len);

	if (atomic_test_and_clear_bit(tx->flags, BT_AVRCP_TG_FLAG_ABORT_CONTINUING) ||
	    (tx->rsp == BT_AVRCP_RSP_REJECTED)) {
		LOG_WRN("Abort to continuing send OR REJECT");
		struct net_buf *rsp_buf;
		uint8_t error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;

		rsp_buf = bt_avrcp_create_vendor_pdu(NULL);
		if (rsp_buf == NULL) {
			LOG_ERR("Failed to allocate response buffer");
			goto done;
		}
		if (tx->rsp == BT_AVRCP_RSP_REJECTED) {
			if (net_buf_tailroom(rsp_buf) < sizeof(uint8_t)) {
				LOG_ERR("Insufficient space in response buffer");
				goto done;
			}
			net_buf_add_u8(rsp_buf, error_code);
		}

		err = send_single_vendor_rsp(tg, tx->tid, tx->rsp, tx->pdu_id, rsp_buf);
		if (err < 0) {
			LOG_ERR("Failed to send rsp_buf");
			net_buf_unref(rsp_buf);
		}
		goto done;
	}

	/* Determine packet type */
	if (tx->sent_len == 0U) {
		pkt_type = BT_AVRCP_PKT_TYPE_START;
	} else {
		/* Last fragment */
		if (chunk_size >= buf->len) {
			pkt_type = BT_AVRCP_PKT_TYPE_END;
		} else {
			pkt_type = BT_AVRCP_PKT_TYPE_CONTINUE;
		}
	}

	/* Send fragment */
	err = send_fragmented_vendor_rsp(tx, pkt_type, buf->data, chunk_size);
	if (err < 0) {
		if (err == -ENOBUFS) {
			LOG_WRN("Retry send fragment %p", buf);
			k_work_reschedule(&tg->vd_rsp_tx_work, K_MSEC(AVRCP_TX_RETRY_DELAY_MS));
			return;
		}
		LOG_ERR("Failed to send fragment offset %u len: %u", tx->sent_len, chunk_size);
		goto done;
	}

	atomic_set_bit(tx->flags, BT_AVRCP_TG_FLAG_TX_ONGOING);
	net_buf_pull_mem(buf, chunk_size);

	tx->sent_len += chunk_size;
	if (buf->len == 0) {
		avrcp_tg_tx_remove(tg, buf);
		net_buf_unref(buf);
	}
	return;

done:
	avrcp_tg_tx_remove(tg, buf);
	if (buf != NULL) {
		net_buf_unref(buf);
	}
	/* restart the tx work */
	k_work_reschedule(&tg->vd_rsp_tx_work, K_NO_WAIT);
}

static int bt_avrcp_tg_send_vendor_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t pdu_id,
				       uint8_t result, struct net_buf *buf)
{
	struct bt_avrcp_tg_vd_rsp_tx *tx;

	if ((tg == NULL) || (tg->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*tx));
	tx = net_buf_user_data(buf);

	tx->tg = tg;
	tx->tid = tid;
	tx->rsp = result;
	tx->pdu_id = pdu_id;
	tx->sent_len = 0U;
	atomic_clear_bit(tx->flags, BT_AVRCP_TG_FLAG_TX_ONGOING);
	atomic_clear_bit(tx->flags, BT_AVRCP_TG_FLAG_ABORT_CONTINUING);

	LOG_DBG("Sending vendor dependent response: tid=%u, total_len=%u", tid, buf->len);
	avrcp_tg_lock(tg);
	sys_slist_append(&tg->vd_rsp_tx_pending, &buf->node);
	avrcp_tg_unlock(tg);

	k_work_reschedule(&tg->vd_rsp_tx_work, K_NO_WAIT);

	return 0;
}

static int process_get_caps_rsp(struct bt_avrcp *avrcp, uint8_t tid, uint8_t rsp_code,
				 struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_caps == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_caps(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_register_notification_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					     uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_avrcp_event_data *event_data;
	uint8_t event_id;
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	uint16_t expected_len;
	struct bt_avrcp_ct *ct = get_avrcp_ct(avrcp);
	uint8_t failed_evt = 0;
	bool found = false;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->notification == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	if (rsp_code == BT_AVRCP_RSP_REJECTED) {
		if (buf->len < sizeof(status)) {
			LOG_ERR("Invalid notification status response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}

		status = net_buf_pull_u8(buf);
		goto notify_callback;
	}

	if (rsp_code == BT_AVRCP_RSP_NOT_IMPLEMENTED) {
		status = BT_AVRCP_STATUS_NOT_IMPLEMENTED;
		goto notify_callback;
	}

	/* The first byte is the event_id */
	if (buf->len < sizeof(event_id)) {
		LOG_ERR("Invalid notification response length");
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	event_id = net_buf_pull_u8(buf);
	if (event_id >= ARRAY_SIZE(ct->ct_notify)) {
		LOG_ERR("Invalid event_id");
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	event_data = (struct bt_avrcp_event_data *)buf->data;

	/* Parse event-specific data */
	switch (event_id) {
	case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
		if (buf->len < sizeof(event_data->play_status)) {
			LOG_ERR("Invalid PLAYBACK_STATUS_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}

		if (event_data->play_status > BT_AVRCP_PLAYBACK_STATUS_REV_SEEK &&
			event_data->play_status != BT_AVRCP_PLAYBACK_STATUS_ERROR) {
			LOG_ERR("Invalid playback status: %d", event_data->play_status);
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		break;
	case BT_AVRCP_EVT_TRACK_CHANGED:
		if (buf->len < sizeof(event_data->identifier)) {
			LOG_ERR("Invalid TRACK_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		uint64_t identifier = sys_get_be64((const uint8_t *)event_data->identifier);

		memcpy(event_data->identifier, &identifier, sizeof(uint64_t));
		break;
	case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
		if (buf->len < sizeof(event_data->playback_pos)) {
			LOG_ERR("Invalid PLAYBACK_POS_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		event_data->playback_pos = sys_be32_to_cpu(event_data->playback_pos);
		break;
	case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
		if (buf->len < sizeof(event_data->battery_status)) {
			LOG_ERR("Invalid BATT_STATUS_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		break;
	case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
		if (buf->len < sizeof(event_data->system_status)) {
			LOG_ERR("Invalid SYSTEM_STATUS_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		if (event_data->system_status > BT_AVRCP_SYSTEM_STATUS_UNPLUGGED) {
			LOG_ERR("Invalid system status: %d", event_data->system_status);
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		break;
	case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
		expected_len = sizeof(event_data->setting_changed.num_of_attr) +
			       event_data->setting_changed.num_of_attr *
			       sizeof(struct bt_avrcp_app_setting_attr_val);
		if (buf->len < expected_len) {
			LOG_ERR("Invalid PLAYER_APP_SETTING_CHANGED attribute length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		break;
	case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
		if (buf->len < sizeof(event_data->addressed_player_changed)) {
			LOG_ERR("Invalid ADDRESSED_PLAYER_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		event_data->addressed_player_changed.player_id =
		sys_be16_to_cpu(event_data->addressed_player_changed.player_id);
		event_data->addressed_player_changed.uid_counter =
		sys_be16_to_cpu(event_data->addressed_player_changed.uid_counter);
		break;
	case BT_AVRCP_EVT_UIDS_CHANGED:
		if (buf->len < sizeof(event_data->uid_counter)) {
			LOG_ERR("Invalid UIDS_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		event_data->uid_counter = sys_be16_to_cpu(event_data->uid_counter);
		break;
	case BT_AVRCP_EVT_VOLUME_CHANGED:
		if (buf->len < sizeof(event_data->absolute_volume)) {
			LOG_ERR("Invalid VOLUME_CHANGED response length");
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}

		if (event_data->absolute_volume > BT_AVRCP_MAX_ABSOLUTE_VOLUME) {
			LOG_ERR("Invalid absolute volume: %d", event_data->absolute_volume);
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}
		break;
	case BT_AVRCP_EVT_TRACK_REACHED_END:
	case BT_AVRCP_EVT_TRACK_REACHED_START:
	case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
	case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
		if (buf->len != 0) {
			LOG_WRN("Zero-parameter event (0x%02X) with %u trailing bytes",
				event_id, buf->len);
		}
		event_data = NULL;
		break;
	default:
		LOG_WRN("Unknown notification event_id: 0x%02X", event_id);
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	if (tid != ct->ct_notify[event_id].tid) {
		LOG_WRN("Mismatched transaction ID: received %u, expected %u", tid,
			ct->ct_notify[event_id].tid);
	}

	if (rsp_code != BT_AVRCP_RSP_INTERIM && rsp_code != BT_AVRCP_RSP_CHANGED) {
		LOG_WRN("Unexpected rsp_code: 0x%02X", rsp_code);
	}

	if ((rsp_code == BT_AVRCP_RSP_INTERIM) && (ct->ct_notify[event_id].interim_received == 0)) {
		/* Mark as interim_received flag on interim response */
		ct->ct_notify[event_id].interim_received = 1;
		avrcp_ct_cb->notification(get_avrcp_ct(avrcp), tid, BT_AVRCP_STATUS_SUCCESS,
					  event_id, (struct bt_avrcp_event_data *)event_data);
		return BT_AVRCP_STATUS_OPERATION_COMPLETED;

	}

	if ((ct->ct_notify[event_id].interim_received == 1) && (rsp_code == BT_AVRCP_RSP_CHANGED)) {
		ct->ct_notify[event_id].interim_received = 0;
		if (ct->ct_notify[event_id].cb != NULL) {
			bt_avrcp_notify_changed_cb_t cb;

			cb = ct->ct_notify[event_id].cb;
			ct->ct_notify[event_id].cb = NULL;
			cb(ct, event_id, (struct bt_avrcp_event_data *)event_data);
		}
	}

	return BT_AVRCP_STATUS_OPERATION_COMPLETED;

notify_callback:
	/* Find the event registered with this TID and clear ONLY that one */
	ARRAY_FOR_EACH(ct->ct_notify, i) {
		if (ct->ct_notify[i].tid == tid && ct->ct_notify[i].cb != NULL) {
			failed_evt = i;
			ct->ct_notify[i].cb = NULL;
			ct->ct_notify[i].interim_received = 0;
			found = true;
			break;
		}
	}

	avrcp_ct_cb->notification(get_avrcp_ct(avrcp), tid, status, found ? failed_evt : 0, NULL);

	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_set_absolute_volume_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					   uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	uint8_t absolute_volume;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->set_absolute_volume == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		avrcp_ct_cb->set_absolute_volume(get_avrcp_ct(avrcp), tid, status, 0);
		return BT_AVRCP_STATUS_OPERATION_COMPLETED;
	}

	if (buf->len < sizeof(absolute_volume)) {
		LOG_ERR("Invalid absolute volume response length");
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}
	absolute_volume = net_buf_pull_u8(buf);
	/* PTS may respond with bit 7 set in the absolute volume parameter invalid behavior */
	absolute_volume &= BT_AVRCP_MAX_ABSOLUTE_VOLUME;

	avrcp_ct_cb->set_absolute_volume(get_avrcp_ct(avrcp), tid, status, absolute_volume);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_list_player_app_setting_attrs_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						     uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->list_player_app_setting_attrs == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->list_player_app_setting_attrs(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_list_player_app_setting_vals_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						    uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->list_player_app_setting_vals == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->list_player_app_setting_vals(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_curr_player_app_setting_val_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						       uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_curr_player_app_setting_val == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_curr_player_app_setting_val(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_set_player_app_setting_val_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						  uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->set_player_app_setting_val == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);

	avrcp_ct_cb->set_player_app_setting_val(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_player_app_setting_attr_text_rsp(struct bt_avrcp *avrcp, uint8_t tid,
							uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_player_app_setting_attr_text == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_player_app_setting_attr_text(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_player_app_setting_val_text_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						       uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_player_app_setting_val_text == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_player_app_setting_val_text(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_inform_displayable_char_set_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						   uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->inform_displayable_char_set == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);

	avrcp_ct_cb->inform_displayable_char_set(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_inform_batt_status_of_ct_rsp(struct bt_avrcp *avrcp, uint8_t tid,
						uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->inform_batt_status_of_ct == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);

	avrcp_ct_cb->inform_batt_status_of_ct(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_element_attrs_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					 uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_element_attrs == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_element_attrs(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_play_status_rsp(struct bt_avrcp *avrcp, uint8_t tid,
				       uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;
	int err;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_play_status == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	err = bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	if ((err < 0) || (status != BT_AVRCP_STATUS_SUCCESS)) {
		buf = NULL;
	}

	avrcp_ct_cb->get_play_status(get_avrcp_ct(avrcp), tid, status, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_set_addressed_player_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					    uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->set_addressed_player == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);

	avrcp_ct_cb->set_addressed_player(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_play_item_rsp(struct bt_avrcp *avrcp, uint8_t tid,
				 uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->play_item == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	avrcp_ct_cb->play_item(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_add_to_now_playing_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					  uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t status = BT_AVRCP_STATUS_INTERNAL_ERROR;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->add_to_now_playing == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	bt_avrcp_rsp_to_status(rsp_code, buf, &status);
	avrcp_ct_cb->add_to_now_playing(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

/** Response vendor handlers table.
 * Note: The min_len field specifies the minimum payload/parameter length,
 * not including status/error codes. For REJECTED responses, only 1 byte
 * for the error code is required regardless of the min_len specified here.
 */
static const struct avrcp_pdu_vendor_handler rsp_vendor_handlers[] = {
	{ BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  NULL },
	{ BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  NULL },
	{ BT_AVRCP_PDU_ID_GET_CAPS, sizeof(struct bt_avrcp_get_caps_rsp), BT_AVRCP_CTYPE_STATUS,
	  process_get_caps_rsp },
	{ BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION, sizeof(uint8_t), BT_AVRCP_CTYPE_NOTIFY,
	  process_register_notification_rsp },
	{ BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS,
	  sizeof(struct bt_avrcp_list_player_app_setting_attrs_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_list_player_app_setting_attrs_rsp },
	{ BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS,
	  sizeof(struct bt_avrcp_list_player_app_setting_vals_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_list_player_app_setting_vals_rsp },
	{ BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL,
	  sizeof(struct bt_avrcp_get_curr_player_app_setting_val_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_get_curr_player_app_setting_val_rsp },
	{ BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL, 0, BT_AVRCP_CTYPE_CONTROL,
	  process_set_player_app_setting_val_rsp },
	{ BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT,
	  sizeof(struct bt_avrcp_get_player_app_setting_attr_text_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_get_player_app_setting_attr_text_rsp },
	{ BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT,
	  sizeof(struct bt_avrcp_get_player_app_setting_val_text_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_get_player_app_setting_val_text_rsp },
	{ BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET, 0, BT_AVRCP_CTYPE_CONTROL,
	  process_inform_displayable_char_set_rsp },
	{ BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT, 0, BT_AVRCP_CTYPE_CONTROL,
	  process_inform_batt_status_of_ct_rsp },
	{ BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  process_set_absolute_volume_rsp },
	{ BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS, sizeof(struct bt_avrcp_get_element_attrs_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_get_element_attrs_rsp },
	{ BT_AVRCP_PDU_ID_GET_PLAY_STATUS, sizeof(struct bt_avrcp_get_play_status_rsp),
	  BT_AVRCP_CTYPE_STATUS, process_get_play_status_rsp },
	{ BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  process_set_addressed_player_rsp },
	{ BT_AVRCP_PDU_ID_PLAY_ITEM, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  process_play_item_rsp },
	{ BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  process_add_to_now_playing_rsp },
};

static inline uint8_t get_cmd_type_by_pdu(uint8_t pdu_id)
{
	ARRAY_FOR_EACH(rsp_vendor_handlers, i) {
		if (rsp_vendor_handlers[i].pdu_id == pdu_id) {
			return rsp_vendor_handlers[i].cmd_type;
		}
	}
	__ASSERT(false, "Unknown PDU ID: 0x%02X", pdu_id);
	return BT_AVRCP_RSP_NOT_IMPLEMENTED;
}

static inline uint8_t get_rsp_min_len_by_pdu(uint8_t pdu_id)
{
	ARRAY_FOR_EACH(rsp_vendor_handlers, i) {
		if (rsp_vendor_handlers[i].pdu_id == pdu_id) {
			return rsp_vendor_handlers[i].min_len;
		}
	}
	__ASSERT(false, "Unknown PDU ID: 0x%02X", pdu_id);
	return 0;
}

static inline int bt_avrcp_status_to_rsp(uint8_t pdu_id, uint8_t status, uint8_t *rsp_code)
{
	if (rsp_code == NULL) {
		return -EINVAL;
	}

	switch (status) {
	case BT_AVRCP_STATUS_SUCCESS:
		if (get_cmd_type_by_pdu(pdu_id) == BT_AVRCP_CTYPE_CONTROL) {
			*rsp_code = BT_AVRCP_RSP_ACCEPTED;
		} else if (get_cmd_type_by_pdu(pdu_id) == BT_AVRCP_CTYPE_STATUS) {
			*rsp_code = BT_AVRCP_RSP_STABLE;
		} else {
			__ASSERT(false, "Unknown PDU ID: 0x%02X", pdu_id);
			*rsp_code = BT_AVRCP_RSP_REJECTED;
			return -EINVAL;
		}
		break;
	case BT_AVRCP_STATUS_IN_TRANSITION:
		*rsp_code = BT_AVRCP_RSP_IN_TRANSITION;
		break;
	case BT_AVRCP_STATUS_NOT_IMPLEMENTED:
		*rsp_code = BT_AVRCP_RSP_NOT_IMPLEMENTED;
		break;
	default:
		if (AVRCP_STATUS_IS_REJECTED(status) == false) {
			return -EINVAL;
		}
		*rsp_code = BT_AVRCP_RSP_REJECTED;
		break;
	}
	return 0;
}

static int bt_avrcp_tg_send_vendor_err_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
					   uint8_t pdu_id, uint8_t status)
{
	struct net_buf *buf;
	uint8_t rsp_code;
	int err;
	size_t data_len;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	err = bt_avrcp_status_to_rsp(pdu_id, status, &rsp_code);
	if (err < 0) {
		return err;
	}

	/* Only REJECTED response needs the status byte */
	data_len = (rsp_code == BT_AVRCP_RSP_REJECTED) ? sizeof(status) : 0;

	buf = avrcp_prepare_vendor_pdu(tg->avrcp, BT_AVRCP_PKT_TYPE_SINGLE, rsp_code,
				       pdu_id, data_len);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (rsp_code == BT_AVRCP_RSP_REJECTED) {
		if (net_buf_tailroom(buf) < sizeof(status)) {
			LOG_WRN("Not enough tailroom for err_code");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_u8(buf, status);
	}

	err = avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		net_buf_unref(buf);
		if (bt_avrcp_disconnect(tg->avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
	return err;
}

static int handle_vendor_pdu(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
			     uint8_t ctype, uint8_t pdu_id,
			     const struct avrcp_pdu_vendor_handler *handlers, size_t num_handlers)
{
	size_t min_len;

	for (size_t i = 0; i < num_handlers; i++) {
		const struct avrcp_pdu_vendor_handler *handler = &handlers[i];

		if (handler->pdu_id != pdu_id) {
			continue;
		}

		if (handlers != rsp_vendor_handlers && ctype != handler->cmd_type) {
			LOG_ERR("Invalid ctype 0x%02x for pdu_id 0x%02x", ctype, pdu_id);
			return BT_AVRCP_STATUS_INVALID_COMMAND;
		}

		/** For REJECTED responses, only need 1 byte for error code.
		 *  For NOT_IMPLEMENTED and IN_TRANSITION, no additional data is needed.
		 *  For other responses, use the handler's minimum length requirement.
		 */
		if (ctype == BT_AVRCP_RSP_REJECTED) {
			min_len = sizeof(uint8_t);
		} else if (ctype == BT_AVRCP_RSP_NOT_IMPLEMENTED ||
			   ctype == BT_AVRCP_RSP_IN_TRANSITION) {
			min_len = 0;
		} else {
			min_len = handler->min_len;
		}

		if (buf->len < min_len) {
			LOG_ERR("Too small (%u < %zu) for pdu_id 0x%02x (rsp=0x%02x)",
				buf->len, min_len, pdu_id, ctype);
			return BT_AVRCP_STATUS_PARAMETER_CONTENT_ERROR;
		}

		return handler->func(avrcp, tid, ctype, buf);
	}

	return BT_AVRCP_STATUS_INVALID_COMMAND;
}

static void process_common_vendor_rsp(struct bt_avrcp *avrcp, uint8_t pdu_id, uint8_t tid,
				      uint8_t rsp_code, struct net_buf *buf)
{
	if (buf == NULL) {
		LOG_ERR("Invalid parameters");
		return;
	}

	handle_vendor_pdu(avrcp, tid, buf, rsp_code, pdu_id, rsp_vendor_handlers,
			  ARRAY_SIZE(rsp_vendor_handlers));
}

static void avrcp_vendor_dependent_rsp_handler(struct bt_avrcp *avrcp, uint8_t tid,
					       struct net_buf *buf)
{
	struct bt_avrcp_avc_vendor_pdu *pdu;
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_rsp_t rsp;
	uint16_t len;
	int err;

	if (buf->len < (sizeof(*avrcp_hdr) + sizeof(*pdu))) {
		LOG_ERR("Invalid vendor frame length: %d", buf->len);
		return;
	}

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));
	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	rsp = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	if ((subunit_type != BT_AVRCP_SUBUNIT_TYPE_PANEL) ||
	    (subunit_id != BT_AVRCP_SUBUNIT_ID_ZERO)) {
		LOG_ERR("Invalid vendor dependent command");
		return;
	}

	pdu = net_buf_pull_mem(buf, sizeof(*pdu));
	if (sys_get_be24(pdu->company_id) != BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG) {
		LOG_ERR("Invalid company id: 0x%06x", sys_get_be24(pdu->company_id));
		return;
	}

	len = sys_be16_to_cpu(pdu->param_len);
	if (buf->len != len) {
		LOG_ERR("Invalid length: %d, buf length = %d", len, buf->len);
		return;
	}

	switch (pdu->pkt_type) {
	case BT_AVRCP_PKT_TYPE_SINGLE:
		/* Single packet should not have incomplete fragment  */
		if (NULL != get_avrcp_ct(avrcp)->reassembly_buf) {
			LOG_ERR("Single packet should not have incomplete fragment");
			goto failure;
		}
		process_common_vendor_rsp(avrcp, pdu->pdu_id, tid, rsp, buf);
		break;

	case BT_AVRCP_PKT_TYPE_START:
		err = init_fragmentation_context(get_avrcp_ct(avrcp), tid, pdu->pdu_id, rsp);
		if (err < 0) {
			LOG_ERR("init fragmentation context: %d", err);
			goto failure;
		}
		/* Add first fragment data */
		err = add_fragment_data(get_avrcp_ct(avrcp), buf->data, buf->len);
		if (err < 0) {
			LOG_ERR("Failed to add first fragment: %d", err);
			goto failure;
		}

		LOG_DBG("First fragment added: %u", buf->len);
		bt_avrcp_ct_send_req_continuing_rsp(get_avrcp_ct(avrcp), tid, pdu->pdu_id);
		break;

	case BT_AVRCP_PKT_TYPE_CONTINUE:
		/* Continuation fragment */
		if ((get_avrcp_ct(avrcp)->reassembly_buf == NULL) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->tid != tid) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->pdu_id != pdu->pdu_id) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->rsp != rsp)) {
			LOG_ERR("Unexpected continue packet");
			goto failure;
		}

		/* Add fragment data */
		err = add_fragment_data(get_avrcp_ct(avrcp), buf->data, buf->len);
		if (err < 0) {
			LOG_ERR("Failed to add continue fragment: %d", err);
			goto failure;
		}

		LOG_DBG("Continue frag added: %u ", buf->len);
		bt_avrcp_ct_send_req_continuing_rsp(get_avrcp_ct(avrcp), tid, pdu->pdu_id);
		break;

	case BT_AVRCP_PKT_TYPE_END:
		/* Final fragment */
		if ((get_avrcp_ct(avrcp)->reassembly_buf == NULL) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->tid != tid) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->pdu_id != pdu->pdu_id) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->rsp != rsp)) {
			LOG_ERR("Unexpected END packet");
			goto failure;
		}

		err = add_fragment_data(get_avrcp_ct(avrcp), buf->data, buf->len);
		if (err < 0) {
			LOG_ERR("Failed to add end fragment: %d", err);
			goto failure;
		}

		LOG_DBG("Continue frag added: %u ", buf->len);

		/* Parse complete reassembled response */
		process_common_vendor_rsp(avrcp, pdu->pdu_id, tid, rsp,
					  get_avrcp_ct(avrcp)->reassembly_buf);

		/* Clean up fragmentation context */
		cleanup_fragmentation_context(get_avrcp_ct(avrcp));
		break;

	default:
		LOG_DBG("Unhandled response: 0x%02x", pdu->pdu_id);
		break;
	}
	return;

failure:
	LOG_ERR("Failed to handle vendor dependent response");
	cleanup_fragmentation_context(get_avrcp_ct(avrcp));
	if (bt_avrcp_ct_send_abort_continuing_rsp(get_avrcp_ct(avrcp), tid, pdu->pdu_id) < 0) {
		LOG_ERR("Failed to send abort continuing response");
		if (bt_avrcp_disconnect(avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
}

static void avrcp_unit_info_rsp_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_unit_info_rsp rsp;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->unit_info_rsp != NULL)) {
		if (buf->len != BT_AVRCP_UNIT_INFO_RSP_SIZE) {
			LOG_ERR("Invalid unit info length: %d", buf->len);
			return;
		}
		net_buf_pull_u8(buf); /* Always 0x07 */
		rsp.unit_type = FIELD_GET(GENMASK(7, 3), net_buf_pull_u8(buf));
		rsp.company_id = net_buf_pull_be24(buf);
		avrcp_ct_cb->unit_info_rsp(get_avrcp_ct(avrcp), tid, &rsp);
	}
}

static void avrcp_subunit_info_rsp_handler(struct bt_avrcp *avrcp, uint8_t tid,
					   struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_subunit_info_rsp rsp;
	uint8_t tmp;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->subunit_info_rsp != NULL)) {
		if (buf->len < BT_AVRCP_SUBUNIT_INFO_RSP_SIZE) {
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
		avrcp_ct_cb->subunit_info_rsp(get_avrcp_ct(avrcp), tid, &rsp);
	}
}

static void avrcp_pass_through_rsp_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct bt_avrcp_passthrough_rsp *rsp;
	bt_avrcp_rsp_t result;

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->subunit_info_rsp != NULL)) {
		if (buf->len < sizeof(*rsp)) {
			LOG_ERR("Invalid passthrough length: %d", buf->len);
			return;
		}

		result = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
		rsp = (struct bt_avrcp_passthrough_rsp *)buf->data;

		avrcp_ct_cb->passthrough_rsp(get_avrcp_ct(avrcp), tid, result, rsp);
	}
}

static const struct avrcp_handler rsp_handlers[] = {
	{BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_vendor_dependent_rsp_handler},
	{BT_AVRCP_OPC_UNIT_INFO, avrcp_unit_info_rsp_handler},
	{BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_subunit_info_rsp_handler},
	{BT_AVRCP_OPC_PASS_THROUGH, avrcp_pass_through_rsp_handler},
};

static void avrcp_unit_info_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_ctype_t ctype;
	int err;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->unit_info_req == NULL)) {
		goto err_rsp;
	}

	if (buf->len < sizeof(*avrcp_hdr)) {
		goto err_rsp;
	}

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));
	if (buf->len != BT_AVRCP_UNIT_INFO_CMD_SIZE) {
		LOG_ERR("Invalid unit info length");
		goto err_rsp;
	}

	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	ctype = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	if ((subunit_type != BT_AVRCP_SUBUNIT_TYPE_UNIT) || (ctype != BT_AVRCP_CTYPE_STATUS) ||
	    (subunit_id != BT_AVRCP_SUBUNIT_ID_IGNORE) ||
	    (avrcp_hdr->opcode != BT_AVRCP_OPC_UNIT_INFO)) {
		LOG_ERR("Invalid unit info command");
		goto err_rsp;
	}

	return avrcp_tg_cb->unit_info_req(get_avrcp_tg(avrcp), tid);

err_rsp:
	err = bt_avrcp_send_unit_info_err_rsp(avrcp, tid);
	if (err) {
		LOG_ERR("Failed to send unit info error response");
		if (bt_avrcp_disconnect(avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
}

static int process_get_caps_cmd(struct bt_avrcp *avrcp, uint8_t tid, uint8_t ctype_or_rsp,
				struct net_buf *buf)
{
	uint8_t cap_id;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_caps == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	cap_id = net_buf_pull_u8(buf);

	/* Validate capability ID */
	if ((cap_id != BT_AVRCP_CAP_COMPANY_ID) &&
	    (cap_id != BT_AVRCP_CAP_EVENTS_SUPPORTED)) {
		LOG_ERR("Invalid capability ID: 0x%02x", cap_id);
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	avrcp_tg_cb->get_caps(get_avrcp_tg(avrcp), tid, cap_id);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_register_notification_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid,
						   uint8_t ctype_or_rsp, struct net_buf *buf)
{
	struct bt_avrcp_register_notification_cmd *cmd;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->register_notification == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	if (cmd->event_id >= ARRAY_SIZE(get_avrcp_tg(avrcp)->tg_notify)) {
		LOG_ERR("Invalid event_id");
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	get_avrcp_tg(avrcp)->tg_notify[cmd->event_id].registered = true;
	get_avrcp_tg(avrcp)->tg_notify[cmd->event_id].interim_sent = false;
	avrcp_tg_cb->register_notification(get_avrcp_tg(avrcp), tid, cmd->event_id, cmd->interval);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int handle_avrcp_continuing_rsp(struct bt_avrcp *avrcp, uint8_t tid, uint8_t ctype_or_rsp,
				       struct net_buf *buf)
{
	LOG_DBG("Received Continuing Response");
	bt_avrcp_tg_process_continuing_cmd(get_avrcp_tg(avrcp), false, tid, buf);
	k_work_reschedule(&get_avrcp_tg(avrcp)->vd_rsp_tx_work, K_NO_WAIT);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int handle_avrcp_abort_continuing_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					      uint8_t ctype_or_rsp, struct net_buf *buf)
{
	LOG_DBG("Received Abort Continuing Response");
	bt_avrcp_tg_process_continuing_cmd(get_avrcp_tg(avrcp), true, tid, buf);
	k_work_reschedule(&get_avrcp_tg(avrcp)->vd_rsp_tx_work, K_NO_WAIT);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_list_player_app_setting_attrs_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						     uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->list_player_app_setting_attrs == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->list_player_app_setting_attrs(get_avrcp_tg(avrcp), tid);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_list_player_app_setting_vals_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						    uint8_t ctype_or_rsp, struct net_buf *buf)
{
	uint8_t attr_id;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->list_player_app_setting_vals == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	attr_id = net_buf_pull_u8(buf);
	if (attr_id > BT_AVRCP_PLAYER_ATTR_SCAN) {
		LOG_ERR("Invalid attr_id: %d", attr_id);
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	avrcp_tg_cb->list_player_app_setting_vals(get_avrcp_tg(avrcp), tid, attr_id);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_curr_player_app_setting_val_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						       uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_curr_player_app_setting_val == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->get_curr_player_app_setting_val(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_set_player_app_setting_val_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						  uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->set_player_app_setting_val == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->set_player_app_setting_val(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_player_app_setting_attr_text_cmd(struct bt_avrcp *avrcp, uint8_t tid,
							uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_player_app_setting_attr_text == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->get_player_app_setting_attr_text(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_player_app_setting_val_text_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						       uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_player_app_setting_val_text == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->get_player_app_setting_val_text(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_inform_displayable_char_set_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						   uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->inform_displayable_char_set == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->inform_displayable_char_set(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_inform_batt_status_of_ct_cmd(struct bt_avrcp *avrcp, uint8_t tid,
						uint8_t ctype_or_rsp, struct net_buf *buf)
{
	uint8_t battery_status;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->inform_batt_status_of_ct == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	battery_status = net_buf_pull_u8(buf);
	if (battery_status > BT_AVRCP_BATTERY_STATUS_FULL) {
		LOG_ERR("Invalid battery_status: %d", battery_status);
		return BT_AVRCP_STATUS_INVALID_PARAMETER;
	}

	avrcp_tg_cb->inform_batt_status_of_ct(get_avrcp_tg(avrcp), tid, battery_status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}


static int process_set_absolute_volume_cmd(struct bt_avrcp *avrcp, uint8_t tid,
					   uint8_t ctype_or_rsp, struct net_buf *buf)
{
	uint8_t absolute_volume;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->set_absolute_volume == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	absolute_volume = net_buf_pull_u8(buf);
	/* PTS may Set command with bit 7 set in the absolute volume parameter invalid behavior */
	absolute_volume &= BT_AVRCP_MAX_ABSOLUTE_VOLUME;

	avrcp_tg_cb->set_absolute_volume(get_avrcp_tg(avrcp), tid, absolute_volume);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_element_attrs_cmd(struct bt_avrcp *avrcp, uint8_t tid,
					 uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_element_attrs == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->get_element_attrs(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_get_play_status_cmd(struct bt_avrcp *avrcp, uint8_t tid,
				       uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_play_status == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->get_play_status(get_avrcp_tg(avrcp), tid);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_set_addressed_player_cmd(struct bt_avrcp *avrcp, uint8_t tid,
					    uint8_t ctype_or_rsp, struct net_buf *buf)
{
	uint16_t player_id;

	player_id = net_buf_pull_be16(buf);

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->set_addressed_player == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->set_addressed_player(get_avrcp_tg(avrcp), tid, player_id);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_play_item_cmd(struct bt_avrcp *avrcp, uint8_t tid,
				 uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->play_item == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->play_item(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int process_add_to_now_playing_cmd(struct bt_avrcp *avrcp, uint8_t tid,
					  uint8_t ctype_or_rsp, struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->add_to_now_playing == NULL)) {
		return BT_AVRCP_STATUS_NOT_IMPLEMENTED;
	}

	avrcp_tg_cb->add_to_now_playing(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static const struct avrcp_pdu_vendor_handler cmd_vendor_handlers[] = {
	{ BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  handle_avrcp_continuing_rsp },
	{ BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  handle_avrcp_abort_continuing_rsp },
	{ BT_AVRCP_PDU_ID_GET_CAPS, sizeof(uint8_t), BT_AVRCP_CTYPE_STATUS, process_get_caps_cmd },
	{ BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION, sizeof(struct bt_avrcp_register_notification_cmd),
	  BT_AVRCP_CTYPE_NOTIFY, avrcp_register_notification_cmd_handler },
	{ BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS, 0, BT_AVRCP_CTYPE_STATUS,
	  process_list_player_app_setting_attrs_cmd },
	{ BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS,
	  sizeof(struct bt_avrcp_list_player_app_setting_vals_cmd), BT_AVRCP_CTYPE_STATUS,
	  process_list_player_app_setting_vals_cmd },
	{ BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL,
	  sizeof(struct bt_avrcp_get_curr_player_app_setting_val_cmd), BT_AVRCP_CTYPE_STATUS,
	  process_get_curr_player_app_setting_val_cmd },
	{ BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL,
	  sizeof(struct bt_avrcp_set_player_app_setting_val_cmd), BT_AVRCP_CTYPE_CONTROL,
	  process_set_player_app_setting_val_cmd },
	{ BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT,
	  sizeof(struct bt_avrcp_get_player_app_setting_attr_text_cmd), BT_AVRCP_CTYPE_STATUS,
	  process_get_player_app_setting_attr_text_cmd },
	{ BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT,
	  sizeof(struct bt_avrcp_get_player_app_setting_val_text_cmd), BT_AVRCP_CTYPE_STATUS,
	  process_get_player_app_setting_val_text_cmd },
	{ BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET,
	  sizeof(struct bt_avrcp_inform_displayable_char_set_cmd), BT_AVRCP_CTYPE_CONTROL,
	  process_inform_displayable_char_set_cmd },
	{ BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT,
	  sizeof(struct bt_avrcp_inform_batt_status_of_ct_cmd), BT_AVRCP_CTYPE_CONTROL,
	  process_inform_batt_status_of_ct_cmd },
	{ BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME, sizeof(uint8_t), BT_AVRCP_CTYPE_CONTROL,
	  process_set_absolute_volume_cmd },
	{ BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS, sizeof(struct bt_avrcp_get_element_attrs_cmd),
	  BT_AVRCP_CTYPE_STATUS, process_get_element_attrs_cmd },
	{ BT_AVRCP_PDU_ID_GET_PLAY_STATUS, 0, BT_AVRCP_CTYPE_STATUS, process_get_play_status_cmd },
	{ BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER, sizeof(struct bt_avrcp_set_addressed_player_cmd),
	  BT_AVRCP_CTYPE_CONTROL, process_set_addressed_player_cmd },
	{ BT_AVRCP_PDU_ID_PLAY_ITEM, sizeof(struct bt_avrcp_play_item_cmd), BT_AVRCP_CTYPE_CONTROL,
	  process_play_item_cmd },
	{ BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING, sizeof(struct bt_avrcp_add_to_now_playing_cmd),
	  BT_AVRCP_CTYPE_CONTROL, process_add_to_now_playing_cmd },
};

static inline uint8_t get_cmd_min_len_by_pdu(uint8_t pdu_id)
{
	ARRAY_FOR_EACH(cmd_vendor_handlers, i) {
		if (cmd_vendor_handlers[i].pdu_id == pdu_id) {
			return cmd_vendor_handlers[i].min_len;
		}
	}
	__ASSERT(false, "Unknown PDU ID: 0x%02X", pdu_id);
	return 0;
}

static void avrcp_vendor_dependent_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid,
					       struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	struct bt_avrcp_avc_vendor_pdu *pdu;
	uint8_t pdu_id = 0;
	int error_code = BT_AVRCP_STATUS_OPERATION_COMPLETED;
	uint8_t ctype_or_rsp;
	uint16_t len;
	int err;

	if (buf->len < (sizeof(*avrcp_hdr) + sizeof(*pdu))) {
		LOG_ERR("Invalid vendor frame length: %d", buf->len);
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));
	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	ctype_or_rsp = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	if ((subunit_type != BT_AVRCP_SUBUNIT_TYPE_PANEL) ||
	    (subunit_id != BT_AVRCP_SUBUNIT_ID_ZERO)) {
		LOG_ERR("Invalid vendor dependent command");
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	pdu = net_buf_pull_mem(buf, sizeof(*pdu));
	pdu_id = pdu->pdu_id;
	if (sys_get_be24(pdu->company_id) != BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG) {
		LOG_ERR("Invalid company id: 0x%06x", sys_get_be24(pdu->company_id));
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	if (BT_AVRCP_AVC_PDU_GET_PACKET_TYPE(pdu) != BT_AVRCP_PKT_TYPE_SINGLE) {
		LOG_ERR("Invalid packet type");
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	len = sys_be16_to_cpu(pdu->param_len);

	if (buf->len != len) {
		LOG_ERR("Invalid length: %d, buf length = %d", len, buf->len);
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}


	error_code = handle_vendor_pdu(avrcp, tid, buf, ctype_or_rsp, pdu->pdu_id,
				       cmd_vendor_handlers, ARRAY_SIZE(cmd_vendor_handlers));
	if (error_code != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		goto err_rsp;
	}
	return;

err_rsp:
	err = bt_avrcp_tg_send_vendor_err_rsp(get_avrcp_tg(avrcp), tid, pdu_id, error_code);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent error response %d", err);
	}
}

static void avrcp_subunit_info_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid,
					   struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_ctype_t ctype;
	uint8_t page;
	uint8_t extension_code;
	int err;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->subunit_info_req == NULL)) {
		goto err_rsp;
	}

	if (buf->len < sizeof(*avrcp_hdr)) {
		goto err_rsp;
	}

	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));
	if (buf->len != BT_AVRCP_SUBUNIT_INFO_CMD_SIZE) {
		LOG_ERR("Invalid subunit info length");
		goto err_rsp;
	}

	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	ctype = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);

	/* First byte contains page and extension code */
	page = FIELD_GET(GENMASK(6, 4), buf->data[0]);
	extension_code = FIELD_GET(GENMASK(2, 0), buf->data[0]);

	if ((subunit_type != BT_AVRCP_SUBUNIT_TYPE_UNIT) || (ctype != BT_AVRCP_CTYPE_STATUS) ||
	    (subunit_id != BT_AVRCP_SUBUNIT_ID_IGNORE) || (page != AVRCP_SUBUNIT_PAGE) ||
	    (avrcp_hdr->opcode != BT_AVRCP_OPC_SUBUNIT_INFO) ||
	    (extension_code != AVRCP_SUBUNIT_EXTENSION_CODE)) {
		LOG_ERR("Invalid subunit info command");
		goto err_rsp;
	}

	return avrcp_tg_cb->subunit_info_req(get_avrcp_tg(avrcp), tid);

err_rsp:
	err = bt_avrcp_send_subunit_info(avrcp, tid, BT_AVRCP_RSP_REJECTED);
	if (err < 0) {
		LOG_ERR("Failed to send subunit info error response");
		if (bt_avrcp_disconnect(avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
}

static void avrcp_pass_through_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid,
					   struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	struct net_buf *rsp_buf;
	int err;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->passthrough_req == NULL)) {
		goto err_rsp;
	}

	if (buf->len < (sizeof(*avrcp_hdr) + sizeof(struct bt_avrcp_passthrough_cmd))) {
		LOG_ERR("Invalid passthrough command length: %d", buf->len);
		goto err_rsp;
	}
	avrcp_hdr = net_buf_pull_mem(buf, sizeof(*avrcp_hdr));

	if (BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr) != BT_AVRCP_SUBUNIT_TYPE_PANEL ||
	    BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr) != BT_AVRCP_SUBUNIT_ID_ZERO ||
	    BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr) != BT_AVRCP_CTYPE_CONTROL) {
		LOG_ERR("Invalid passthrough command ");
		goto err_rsp;
	}

	return avrcp_tg_cb->passthrough_req(get_avrcp_tg(avrcp), tid, buf);

err_rsp:
	rsp_buf = bt_avrcp_create_pdu(NULL);
	if (rsp_buf == NULL) {
		LOG_ERR("Failed to allocate response buffer");
		return;
	}

	err = bt_avrcp_tg_send_passthrough_rsp(get_avrcp_tg(avrcp), tid, BT_AVRCP_RSP_REJECTED,
					       rsp_buf);
	if (err < 0) {
		LOG_ERR("Failed to send passthrough error response");
		net_buf_unref(rsp_buf);
		if (bt_avrcp_disconnect(avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
}

static const struct avrcp_handler cmd_handlers[] = {
	{ BT_AVRCP_OPC_VENDOR_DEPENDENT, avrcp_vendor_dependent_cmd_handler},
	{ BT_AVRCP_OPC_UNIT_INFO, avrcp_unit_info_cmd_handler},
	{ BT_AVRCP_OPC_SUBUNIT_INFO, avrcp_subunit_info_cmd_handler},
	{ BT_AVRCP_OPC_PASS_THROUGH, avrcp_pass_through_cmd_handler},
};

/* An AVRCP message received */
static int avrcp_recv(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_rsp_t rsp;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_subunit_type_t subunit_type;

	if (buf->len < sizeof(*avrcp_hdr)) {
		LOG_ERR("invalid AVRCP header received");
		return -EINVAL;
	}

	avrcp_hdr = (void *)buf->data;
	rsp = BT_AVRCP_HDR_GET_CTYPE_OR_RSP(avrcp_hdr);
	subunit_id = BT_AVRCP_HDR_GET_SUBUNIT_ID(avrcp_hdr);
	subunit_type = BT_AVRCP_HDR_GET_SUBUNIT_TYPE(avrcp_hdr);

	LOG_DBG("AVRCP msg received, cr:0x%X, tid:0x%X, rsp: 0x%X, opc:0x%02X,", cr, tid, rsp,
		avrcp_hdr->opcode);
	if (cr == BT_AVCTP_RESPONSE) {
		ARRAY_FOR_EACH(rsp_handlers, i) {
			if (avrcp_hdr->opcode == rsp_handlers[i].opcode) {
				rsp_handlers[i].func(avrcp, tid, buf);
				return 0;
			}
		}
	} else {
		ARRAY_FOR_EACH(cmd_handlers, i) {
			if (avrcp_hdr->opcode == cmd_handlers[i].opcode) {
				cmd_handlers[i].func(avrcp, tid, buf);
				return 0;
			}
		}
	}

	LOG_WRN("received unknown opcode : 0x%02X", avrcp_hdr->opcode);
	return 0;
}

static void init_avctp_control_channel(struct bt_avctp *session)
{
	LOG_DBG("session %p", session);

	session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	session->br_chan.required_sec_level = BT_SECURITY_L2;
	session->pid = BT_SDP_AV_REMOTE_SVCLASS;
	session->tx_pool = &avctp_ctrl_tx_pool;
	session->max_tx_payload_size = CONFIG_BT_L2CAP_TX_MTU;
	session->rx_pool = &avctp_ctrl_rx_pool;
}

static const struct bt_avctp_ops_cb avctp_ops = {
	.connected = avrcp_connected,
	.disconnected = avrcp_disconnected,
	.recv = avrcp_recv,
};

static int avrcp_accept(struct bt_conn *conn, struct bt_avctp **session)
{
	struct bt_avrcp *avrcp;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		return -ENOMEM;
	}

	if (avrcp->acl_conn != NULL) {
		return -EALREADY;
	}

	init_avctp_control_channel(&(avrcp->session));
	*session = &(avrcp->session);
	avrcp->session.ops = &avctp_ops;
	avrcp->acl_conn = bt_conn_ref(conn);

	LOG_DBG("session: %p", &(avrcp->session));

	return 0;
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
static void init_avctp_browsing_channel(struct bt_avctp *session)
{
	LOG_DBG("session %p", session);

	session->br_chan.rx.mtu = CONFIG_BT_AVRCP_BROWSING_L2CAP_MTU;
	session->br_chan.required_sec_level = BT_SECURITY_L2;
	session->br_chan.rx.optional = false;
	session->br_chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	session->br_chan.rx.max_transmit = 3;
	session->br_chan.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
	session->br_chan.tx.monitor_timeout = CONFIG_BT_L2CAP_BR_MONITOR_TIMEOUT;
	session->pid = BT_SDP_AV_REMOTE_SVCLASS;
	session->tx_pool = NULL;
	session->max_tx_payload_size = 0;
	session->rx_pool = NULL;
}

/* The AVCTP L2CAP channel established */
static void browsing_avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_BROW_AVCTP(session);

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->browsing_connected != NULL)) {
		avrcp_ct_cb->browsing_connected(session->br_chan.chan.conn, get_avrcp_ct(avrcp));
	}

	if ((avrcp_tg_cb != NULL) && (avrcp_tg_cb->browsing_connected != NULL)) {
		avrcp_tg_cb->browsing_connected(session->br_chan.chan.conn, get_avrcp_tg(avrcp));
	}
}

/* The AVCTP L2CAP channel released */
static void browsing_avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_BROW_AVCTP(session);

	if ((avrcp_ct_cb != NULL) && (avrcp_ct_cb->disconnected != NULL)) {
		avrcp_ct_cb->browsing_disconnected(get_avrcp_ct(avrcp));
	}
	if ((avrcp_tg_cb != NULL) && (avrcp_tg_cb->disconnected != NULL)) {
		avrcp_tg_cb->browsing_disconnected(get_avrcp_tg(avrcp));
	}
}

static int avrcp_ct_handle_set_browsed_player(struct bt_avrcp *avrcp,
					      uint8_t tid, struct net_buf *buf)
{
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->set_browsed_player == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_ct_cb->set_browsed_player(get_avrcp_ct(avrcp), tid, buf);

	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_get_folder_items(struct bt_avrcp *avrcp,
					    uint8_t tid, struct net_buf *buf)
{
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_folder_items == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_ct_cb->get_folder_items(get_avrcp_ct(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_change_path(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	uint8_t status;
	uint32_t num_items = 0;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->change_path == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	status = net_buf_pull_u8(buf);
	if (status == BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		num_items = net_buf_pull_be32(buf);
	}

	avrcp_ct_cb->change_path(get_avrcp_ct(avrcp), tid, status, num_items);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_get_item_attrs(struct bt_avrcp *avrcp,
					  uint8_t tid, struct net_buf *buf)
{
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_item_attrs == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_ct_cb->get_item_attrs(get_avrcp_ct(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_get_total_number_of_items(struct bt_avrcp *avrcp,
						     uint8_t tid, struct net_buf *buf)
{
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_total_number_of_items == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_ct_cb->get_total_number_of_items(get_avrcp_ct(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_search(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf)
{
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->search == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_ct_cb->search(get_avrcp_ct(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_ct_handle_browsing_general_reject(struct bt_avrcp *avrcp,
						   uint8_t tid, struct net_buf *buf)
{
	uint8_t status;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->browsing_general_reject == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	status = net_buf_pull_u8(buf);
	avrcp_ct_cb->browsing_general_reject(get_avrcp_ct(avrcp), tid, status);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static const struct avrcp_pdu_handler rsp_brow_handlers[] = {
	{BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER, sizeof(uint8_t), avrcp_ct_handle_set_browsed_player},
	{ BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS, sizeof(uint8_t), avrcp_ct_handle_get_folder_items},
	{ BT_AVRCP_PDU_ID_CHANGE_PATH, sizeof(uint8_t), avrcp_ct_handle_change_path},
	{ BT_AVRCP_PDU_ID_GET_ITEM_ATTRS, sizeof(uint8_t), avrcp_ct_handle_get_item_attrs},
	{ BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS, sizeof(uint8_t),
	  avrcp_ct_handle_get_total_number_of_items},
	{ BT_AVRCP_PDU_ID_SEARCH, sizeof(uint8_t), avrcp_ct_handle_search},
	{ BT_AVRCP_PDU_ID_GENERAL_REJECT, sizeof(uint8_t), avrcp_ct_handle_browsing_general_reject},
};

static int avrcp_tg_handle_set_browsed_player_req(struct bt_avrcp *avrcp,
						  uint8_t tid, struct net_buf *buf)
{
	uint16_t player_id;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->set_browsed_player == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	player_id = net_buf_pull_be16(buf);

	LOG_DBG("Set browsed player request: player_id=0x%04x", player_id);

	avrcp_tg_cb->set_browsed_player(get_avrcp_tg(avrcp), tid, player_id);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_tg_handle_get_folder_items_req(struct bt_avrcp *avrcp, uint8_t tid,
						struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_folder_items == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_tg_cb->get_folder_items(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_tg_handle_change_path_req(struct bt_avrcp *avrcp, uint8_t tid,
					   struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->change_path == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_tg_cb->change_path(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_tg_handle_get_item_attrs_req(struct bt_avrcp *avrcp, uint8_t tid,
					      struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_item_attrs == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_tg_cb->get_item_attrs(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_tg_handle_get_total_number_of_items_req(struct bt_avrcp *avrcp,
							 uint8_t tid, struct net_buf *buf)
{
	uint8_t scope;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_total_number_of_items == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	scope = net_buf_pull_u8(buf);

	avrcp_tg_cb->get_total_number_of_items(get_avrcp_tg(avrcp), tid, scope);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static int avrcp_tg_handle_search_req(struct bt_avrcp *avrcp, uint8_t tid,
				      struct net_buf *buf)
{
	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->search == NULL)) {
		return BT_AVRCP_STATUS_INTERNAL_ERROR;
	}

	avrcp_tg_cb->search(get_avrcp_tg(avrcp), tid, buf);
	return BT_AVRCP_STATUS_OPERATION_COMPLETED;
}

static const struct avrcp_pdu_handler cmd_brow_handlers[] = {
	{BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER, sizeof(uint16_t),
	 avrcp_tg_handle_set_browsed_player_req},
	{BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS, sizeof(struct bt_avrcp_get_folder_items_cmd),
	 avrcp_tg_handle_get_folder_items_req},
	{BT_AVRCP_PDU_ID_CHANGE_PATH, sizeof(struct bt_avrcp_change_path_cmd),
	 avrcp_tg_handle_change_path_req},
	{BT_AVRCP_PDU_ID_GET_ITEM_ATTRS, sizeof(struct bt_avrcp_get_item_attrs_cmd),
	 avrcp_tg_handle_get_item_attrs_req},
	{BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS,
	 sizeof(struct bt_avrcp_get_total_number_of_items_cmd),
	 avrcp_tg_handle_get_total_number_of_items_req},
	{BT_AVRCP_PDU_ID_SEARCH, sizeof(struct bt_avrcp_search_cmd),
	 avrcp_tg_handle_search_req},
};

static int handle_pdu(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf,
		      uint8_t pdu_id, const struct avrcp_pdu_handler *handlers, size_t num_handlers)
{
	for (size_t i = 0; i < num_handlers; i++) {
		const struct avrcp_pdu_handler *handler = &handlers[i];

		if (handler->pdu_id != pdu_id) {
			continue;
		}

		if (buf->len < handler->min_len) {
			LOG_ERR("Too small (%u bytes) pdu_id 0x%02x", buf->len, pdu_id);
			return BT_AVRCP_STATUS_INVALID_PARAMETER;
		}

		return handler->func(avrcp, tid, buf);
	}

	return BT_AVRCP_STATUS_INVALID_COMMAND;
}

static int browsing_avrcp_recv(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			       uint8_t tid)
{
	struct bt_avrcp *avrcp = AVRCP_BROW_AVCTP(session);
	struct bt_avrcp_brow_pdu *brow;
	int status;
	int err;

	if (buf->len < sizeof(struct bt_avrcp_brow_pdu)) {
		LOG_ERR("Invalid AVRCP browsing header received: buffer too short (%u)", buf->len);
		return -EMSGSIZE;
	}

	brow = net_buf_pull_mem(buf, sizeof(struct bt_avrcp_brow_pdu));

	if (buf->len != sys_be16_to_cpu(brow->param_len)) {
		LOG_ERR("Invalid AVRCP browsing PDU length: expected %u, got %u",
			sys_be16_to_cpu(brow->param_len), buf->len);
		return -EMSGSIZE;
	}

	LOG_DBG("AVRCP browsing msg received, cr:0x%X, tid:0x%X, pdu_id:0x%02X", cr,
		tid, brow->pdu_id);

	if (cr == BT_AVCTP_RESPONSE) {
		handle_pdu(avrcp, tid, buf, brow->pdu_id, rsp_brow_handlers,
			   ARRAY_SIZE(rsp_brow_handlers));
		return 0;
	}

	status = handle_pdu(avrcp, tid, buf, brow->pdu_id, cmd_brow_handlers,
			 ARRAY_SIZE(cmd_brow_handlers));
	if (status != BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		err = bt_avrcp_tg_browsing_general_reject(get_avrcp_tg(avrcp), tid,
							  status);
		if (err < 0) {
			LOG_ERR("Failed to send browsing general reject: %d", err);
		}
		return err;
	}

	return 0;
}

static struct net_buf *browsing_avrcp_l2cap_alloc_buf(struct bt_avctp *session)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&avctp_browsing_rx_pool, K_FOREVER);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
	}

	return buf;
}

static const struct bt_avctp_ops_cb browsing_avctp_ops = {
	.connected = browsing_avrcp_connected,
	.disconnected = browsing_avrcp_disconnected,
	.recv = browsing_avrcp_recv,
	.alloc_buf = browsing_avrcp_l2cap_alloc_buf,
};

static int avrcp_browsing_accept(struct bt_conn *conn, struct bt_avctp **session)
{
	struct bt_avrcp *avrcp;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		LOG_ERR("Cannot allocate memory");
		return -ENOTCONN;
	}

	if (avrcp->acl_conn == NULL) {
		LOG_ERR("The control channel not established");
		return -ENOTCONN;
	}

	if (avrcp->browsing_session.br_chan.chan.conn != NULL) {
		LOG_ERR("Browsing session already connected");
		return -EALREADY;
	}

	init_avctp_browsing_channel(&(avrcp->browsing_session));
	avrcp->browsing_session.ops = &browsing_avctp_ops;
	*session = &(avrcp->browsing_session);

	LOG_DBG("browsing_session: %p", &(avrcp->browsing_session));

	return 0;
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

int bt_avrcp_init(void)
{
	int err;

	/* Register event handlers with AVCTP */
	avctp_server.l2cap.psm = BT_L2CAP_PSM_AVRCP;
	avctp_server.accept = avrcp_accept;
	err = bt_avctp_server_register(&avctp_server);
	if (err < 0) {
		LOG_ERR("AVRCP registration failed");
		return err;
	}

#if defined(CONFIG_BT_AVRCP_BROWSING)
	avctp_browsing_server.l2cap.psm = BT_L2CAP_PSM_AVRCP_BROWSING;
	avctp_browsing_server.accept = avrcp_browsing_accept;
	err = bt_avctp_server_register(&avctp_browsing_server);
	if (err < 0) {
		LOG_ERR("AVRCP browsing registration failed");
		return err;
	}
#endif /* CONFIG_BT_AVRCP_BROWSING */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
	err = bt_avrcp_tg_cover_art_init(&bt_avrcp_tg_cover_art_psm);
	if (err < 0) {
		LOG_ERR("AVRCP Cover Art initialization failed");
		return err;
	}
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_TARGET)
	bt_sdp_register_service(&avrcp_tg_rec);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
	bt_sdp_register_service(&avrcp_ct_rec);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

	/* Init CT and TG connection pool*/
	__ASSERT(ARRAY_SIZE(bt_avrcp_ct_pool) == ARRAY_SIZE(avrcp_connection), "CT size mismatch");
	__ASSERT(ARRAY_SIZE(bt_avrcp_tg_pool) == ARRAY_SIZE(avrcp_connection), "TG size mismatch");

	ARRAY_FOR_EACH(avrcp_connection, i) {
		bt_avrcp_ct_pool[i].avrcp = &avrcp_connection[i];
		bt_avrcp_tg_pool[i].avrcp = &avrcp_connection[i];
		/* Init delay work */
		k_work_init_delayable(&bt_avrcp_tg_pool[i].vd_rsp_tx_work,
				      bt_avrcp_tg_vendor_tx_work);
		sys_slist_init(&bt_avrcp_tg_pool[i].vd_rsp_tx_pending);

		k_sem_init(&bt_avrcp_tg_pool[i].lock, 1, 1);

		memset(bt_avrcp_ct_pool[i].ct_notify, 0, sizeof(bt_avrcp_ct_pool[i].ct_notify));

		memset(bt_avrcp_tg_pool[i].tg_notify, 0, sizeof(bt_avrcp_tg_pool[i].tg_notify));
	}
	LOG_DBG("AVRCP Initialized successfully.");
	return 0;
}

int bt_avrcp_connect(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;
	int err;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		LOG_ERR("Cannot allocate memory");
		return -ENOTCONN;
	}

	if (avrcp->acl_conn != NULL) {
		return -EALREADY;
	}

	avrcp->session.ops = &avctp_ops;
	init_avctp_control_channel(&(avrcp->session));
	err = bt_avctp_connect(conn, BT_L2CAP_PSM_AVRCP, &(avrcp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		memset(avrcp, 0, sizeof(struct bt_avrcp));
		LOG_DBG("AVCTP Connect failed");
		return err;
	}
	avrcp->acl_conn = bt_conn_ref(conn);

	LOG_DBG("Connection request sent");
	return err;
}

int bt_avrcp_disconnect(struct bt_conn *conn)
{
	int err;
	struct bt_avrcp *avrcp;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		LOG_ERR("Get avrcp connection failure");
		return -ENOTCONN;
	}

#if defined(CONFIG_BT_AVRCP_BROWSING)
	if (avrcp->browsing_session.br_chan.chan.conn != NULL) {
		/* If browsing session is still active, disconnect it first */
		err = bt_avrcp_browsing_disconnect(conn);
		if (err < 0) {
			LOG_ERR("Browsing session disconnect failed: %d", err);
			return err;
		}
	}
#endif /* CONFIG_BT_AVRCP_BROWSING */

	err = bt_avctp_disconnect(&(avrcp->session));
	if (err < 0) {
		LOG_DBG("AVCTP Disconnect failed");
		return err;
	}

	return err;
}

struct net_buf *bt_avrcp_create_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool, BT_AVRCP_HEADROOM);
}

struct net_buf *bt_avrcp_create_vendor_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool, BT_AVRCP_HEADROOM +
				  sizeof(struct bt_avrcp_avc_vendor_pdu));
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
int bt_avrcp_browsing_connect(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;
	int err;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		LOG_ERR("Cannot allocate memory");
		return -ENOTCONN;
	}

	if (avrcp->acl_conn == NULL) {
		LOG_ERR("The control channel not established");
		return -ENOTCONN;
	}

	if (avrcp->browsing_session.br_chan.chan.conn != NULL) {
		return -EALREADY;
	}

	avrcp->browsing_session.ops = &browsing_avctp_ops;
	init_avctp_browsing_channel(&(avrcp->browsing_session));
	err = bt_avctp_connect(conn, BT_L2CAP_PSM_AVRCP_BROWSING, &(avrcp->browsing_session));
	if (err < 0) {
		LOG_ERR("AVCTP browsing connect failed");
		return err;
	}

	LOG_DBG("Browsing connection request sent");

	return 0;
}

int bt_avrcp_browsing_disconnect(struct bt_conn *conn)
{
	int err;
	struct bt_avrcp *avrcp;

	avrcp = avrcp_get_connection(conn);
	if (avrcp == NULL) {
		LOG_ERR("Get avrcp connection failure");
		return -ENOTCONN;
	}

	err = bt_avctp_disconnect(&(avrcp->browsing_session));
	if (err < 0) {
		LOG_ERR("AVCTP browsing disconnect failed");
		return err;
	}

	return err;
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

int bt_avrcp_ct_get_unit_info(struct bt_avrcp_ct *ct, uint8_t tid)
{
	struct net_buf *buf;
	int err;
	uint8_t param[5];

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	buf = avrcp_create_unit_pdu(ct->avrcp, BT_AVRCP_CTYPE_STATUS);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	net_buf_add_mem(buf, param, sizeof(param));

	err = avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_get_subunit_info(struct bt_avrcp_ct *ct, uint8_t tid)
{
	struct net_buf *buf;
	uint8_t param[5];
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	buf = avrcp_create_subunit_pdu(ct->avrcp, BT_AVRCP_CTYPE_STATUS);
	if (!buf) {
		return -ENOMEM;
	}

	memset(param, 0xFF, ARRAY_SIZE(param));
	param[0] = FIELD_PREP(GENMASK(6, 4), AVRCP_SUBUNIT_PAGE) |
		   FIELD_PREP(GENMASK(2, 0), AVRCP_SUBUNIT_EXTENSION_CODE);
	net_buf_add_mem(buf, param, sizeof(param));

	err = avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_passthrough(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t opid, uint8_t state,
			    const uint8_t *payload, uint8_t len)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	buf = avrcp_create_passthrough_pdu(ct->avrcp, BT_AVRCP_CTYPE_CONTROL);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, FIELD_PREP(BIT(7), state) | FIELD_PREP(GENMASK(6, 0), opid));
	net_buf_add_u8(buf, len);
	if (len) {
		net_buf_add_mem(buf, payload, len);
	}

	err = avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_register_notification(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t event_id,
				      uint32_t interval, bt_avrcp_notify_changed_cb_t cb)
{
	struct net_buf *buf;
	uint16_t param_len = sizeof(event_id) + sizeof(interval);
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->notification == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	if (event_id >= ARRAY_SIZE(ct->ct_notify)) {
		return -EINVAL;
	}

	if (ct->ct_notify[event_id].cb != NULL) {
		return -EBUSY;
	}
	ct->ct_notify[event_id].cb = cb;
	ct->ct_notify[event_id].interim_received = 0;
	ct->ct_notify[event_id].tid = tid;

	buf = avrcp_prepare_vendor_pdu(ct->avrcp, BT_AVRCP_PKT_TYPE_SINGLE, BT_AVRCP_CTYPE_NOTIFY,
				       BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION, param_len);
	if (buf == NULL) {
		return -ENOBUFS;
	}
	/* Add event ID */
	net_buf_add_u8(buf, event_id);

	/* Add playback interval */
	net_buf_add_be32(buf, interval);

	err = avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, ct->ct_notify[event_id].tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
		/* Roll back state so the app can retry */
		ct->ct_notify[event_id].cb = NULL;
		ct->ct_notify[event_id].interim_received = 0;
	}
	return err;
}

static int bt_avrcp_ct_vendor_dependent(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t pdu_id,
					struct net_buf *buf)
{
	struct bt_avrcp_header *hdr;
	struct bt_avrcp_avc_vendor_pdu *pdu;
	uint16_t param_len;
	uint8_t min_len;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	min_len = get_cmd_min_len_by_pdu(pdu_id);
	if (buf->len < min_len) {
		LOG_ERR("CMD Buffer too small for PDU 0x%02X (len=%u, min=%u)",
			pdu_id, buf->len, min_len);
		return -EINVAL;
	}

	param_len = buf->len;

	if (net_buf_headroom(buf) < (sizeof(*pdu) + sizeof(*hdr))) {
		LOG_WRN("Not enough headroom: for vendor dependent PDU");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	pdu = net_buf_push(buf, sizeof(*pdu));
	sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, pdu->company_id);
	pdu->pdu_id = pdu_id;
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, BT_AVRCP_PKT_TYPE_SINGLE);
	pdu->param_len = sys_cpu_to_be16(param_len);

	hdr = net_buf_push(buf, sizeof(struct bt_avrcp_header));
	memset(hdr, 0, sizeof(struct bt_avrcp_header));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(hdr, get_cmd_type_by_pdu(pdu_id));
	BT_AVRCP_HDR_SET_SUBUNIT_ID(hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	hdr->opcode = BT_AVRCP_OPC_VENDOR_DEPENDENT;

	err = avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send vendor PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_caps(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t cap_id)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_caps == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(cap_id)) {
		LOG_WRN("Not enough tailroom: for cap_id");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, cap_id);

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_GET_CAPS, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_list_player_app_setting_attrs(struct bt_avrcp_ct *ct, uint8_t tid)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->list_player_app_setting_attrs == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_list_player_app_setting_vals(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t attr_id)
{
	int err;
	struct net_buf *buf;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->list_player_app_setting_vals == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(attr_id)) {
		LOG_WRN("Not enough tailroom: for attr_id");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	net_buf_add_u8(buf, attr_id);

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_get_curr_player_app_setting_val(struct bt_avrcp_ct *ct, uint8_t tid,
						struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_curr_player_app_setting_val == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_set_player_app_setting_val(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->set_player_app_setting_val == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_player_app_setting_attr_text(struct bt_avrcp_ct *ct, uint8_t tid,
						 struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_player_app_setting_attr_text == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid,
					   BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_player_app_setting_val_text(struct bt_avrcp_ct *ct, uint8_t tid,
						struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_player_app_setting_val_text == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_inform_displayable_char_set(struct bt_avrcp_ct *ct, uint8_t tid,
					    struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->inform_displayable_char_set == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_inform_batt_status_of_ct(struct bt_avrcp_ct *ct, uint8_t tid,
					 uint8_t battery_status)
{
	int err;
	struct net_buf *buf;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->inform_batt_status_of_ct == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	if (battery_status > BT_AVRCP_BATTERY_STATUS_FULL) {
		LOG_ERR("Invalid battery status: %d", battery_status);
		return -EINVAL;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(battery_status)) {
		LOG_WRN("Not enough tailroom: for battery_status");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	net_buf_add_u8(buf, battery_status);

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT,
					   buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_set_absolute_volume(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t absolute_volume)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->set_absolute_volume == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	if (absolute_volume > BT_AVRCP_MAX_ABSOLUTE_VOLUME) {
		LOG_ERR("Invalid absolute volume: %d", absolute_volume);
		return -EINVAL;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(absolute_volume)) {
		LOG_WRN("Not enough tailroom: for absolute_volume");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, absolute_volume);

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_get_element_attrs(struct bt_avrcp_ct *ct, uint8_t tid,
				  struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_element_attrs == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_play_status(struct bt_avrcp_ct *ct, uint8_t tid)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->get_play_status == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_GET_PLAY_STATUS, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_set_addressed_player(struct bt_avrcp_ct *ct, uint8_t tid, uint16_t player_id)
{
	int err;
	struct net_buf *buf;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->set_addressed_player == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(player_id)) {
		LOG_WRN("Not enough tailroom: for player_id");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	net_buf_add_be16(buf, player_id);

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_play_item(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->play_item == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_PLAY_ITEM, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_add_to_now_playing(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (avrcp_ct_cb->add_to_now_playing == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_ct_vendor_dependent(ct, tid, BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING, buf);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_register_cb(const struct bt_avrcp_ct_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (avrcp_ct_cb) {
		return -EALREADY;
	}

	avrcp_ct_cb = cb;

	return 0;
}

int bt_avrcp_tg_register_cb(const struct bt_avrcp_tg_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (avrcp_tg_cb) {
		return -EALREADY;
	}

	avrcp_tg_cb = cb;

	return 0;
}

int bt_avrcp_tg_send_unit_info_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
				   struct bt_avrcp_unit_info_rsp *rsp)
{
	struct net_buf *buf;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL) || (rsp == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	buf = avrcp_create_unit_pdu(tg->avrcp, BT_AVRCP_RSP_STABLE);
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

	err = avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_send_subunit_info_rsp(struct bt_avrcp_tg *tg, uint8_t tid)
{
	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	return bt_avrcp_send_subunit_info(tg->avrcp, tid, BT_AVRCP_RSP_STABLE);
}

static int bt_avrcp_browsing_send(struct bt_avrcp *avrcp, struct net_buf *buf, uint8_t pdu_id,
				  uint8_t avctp_type, uint8_t tid)
{
	struct bt_avrcp_brow_pdu *hdr;
	uint16_t param_len;

	if ((avrcp == NULL) || (buf == NULL)) {
		LOG_ERR("Invalid AVRCP context or buffer");
		return -EINVAL;
	}

	if (avrcp->browsing_session.br_chan.chan.conn == NULL) {
		LOG_ERR("Browsing session not connected");
		return -ENOTCONN;
	}

	param_len = buf->len;

	/* Add minimum size check based on avctp_type */
	if (avctp_type == BT_AVCTP_CMD) {
		for (size_t i = 0; i < ARRAY_SIZE(cmd_brow_handlers); i++) {
			if (cmd_brow_handlers[i].pdu_id == pdu_id) {
				if (param_len < cmd_brow_handlers[i].min_len) {
					LOG_ERR("Too small (%u < %u) for cmd pdu_id 0x%02x",
						param_len, cmd_brow_handlers[i].min_len, pdu_id);
					return -EINVAL;
				}
				break;
			}
		}
	} else {
		for (size_t i = 0; i < ARRAY_SIZE(rsp_brow_handlers); i++) {
			if (rsp_brow_handlers[i].pdu_id == pdu_id) {
				if (param_len < rsp_brow_handlers[i].min_len) {
					LOG_ERR("Too small (%u < %u) for rsp pdu_id 0x%02x",
						param_len, rsp_brow_handlers[i].min_len, pdu_id);
					return -EINVAL;
				}
				break;
			}
		}
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_avrcp_brow_pdu)) {
		LOG_ERR("Not enough headroom for browsing PDU header");
		return -ENOMEM;
	}

	hdr = net_buf_push(buf, sizeof(struct bt_avrcp_brow_pdu));
	memset(hdr, 0, sizeof(*hdr));
	hdr->pdu_id    = pdu_id;
	hdr->param_len = sys_cpu_to_be16(param_len);

	return avrcp_browsing_send(avrcp, buf, avctp_type, tid);
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
static inline int avrcp_ct_precheck(struct bt_avrcp_ct *ct, struct net_buf *buf)
{
	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	return 0;
}

int bt_avrcp_ct_set_browsed_player(struct bt_avrcp_ct *ct, uint8_t tid, uint16_t player_id)
{
	struct net_buf *buf;
	int err;

	if (avrcp_ct_cb->set_browsed_player == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = avrcp_create_browsing_pdu(ct->avrcp);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (net_buf_tailroom(buf) < sizeof(player_id)) {
		LOG_ERR("Not enough tailroom for SET_BROWSED_PLAYER cmd");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_be16(buf, player_id);

	err =  bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER,
				      BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP browsing PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_get_folder_items(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		return err;
	}

	if (avrcp_ct_cb->get_folder_items == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS,
				     BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_FOLDER_ITEMS cmd (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_change_path(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		return err;
	}

	if (avrcp_ct_cb->change_path == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_CHANGE_PATH,
				     BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send CHANGE_PATH cmd (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_item_attrs(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		return err;
	}

	if (avrcp_ct_cb->get_item_attrs == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_GET_ITEM_ATTRS,
				     BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_ITEM_ATTRIBUTES cmd (err: %d)", err);
	}
	return err;
}

int bt_avrcp_ct_get_total_number_of_items(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t scope)
{
	struct net_buf *buf;
	int err;

	if (scope > BT_AVRCP_SCOPE_NOW_PLAYING) {
		return -EINVAL;
	}

	if (avrcp_ct_cb->get_total_number_of_items == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	buf = avrcp_create_browsing_pdu(ct->avrcp);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (net_buf_tailroom(buf) < sizeof(scope)) {
		LOG_ERR("Not enough tailroom for scope");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, scope);

	err = bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS,
				     BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_TOTAL_NUMBER_OF_ITEMS cmd (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_ct_search(struct bt_avrcp_ct *ct, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_ct_precheck(ct, buf);
	if (err < 0) {
		LOG_ERR("AVRCP CT precheck failed: %d", err);
		return err;
	}

	if (avrcp_ct_cb->search == NULL) {
		LOG_WRN("Rsp callback not registered");
		return -EOPNOTSUPP;
	}

	err = bt_avrcp_browsing_send(ct->avrcp, buf, BT_AVRCP_PDU_ID_SEARCH,
				     BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send SEARCH cmd (err: %d)", err);
	}
	return err;
}

static inline int avrcp_tg_precheck(struct bt_avrcp_tg *tg, struct net_buf *buf)
{
	if ((tg == NULL) || (tg->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	return 0;
}

int bt_avrcp_tg_set_browsed_player(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		return err;
	}

	err =  bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER,
				      BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP browsing PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_get_folder_items(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		return err;
	}

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_FOLDER_ITEMS rsp (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_change_path(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status, uint32_t num_items)
{
	struct net_buf *buf;
	int err;

	buf = avrcp_create_browsing_pdu(tg->avrcp);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (net_buf_tailroom(buf) < sizeof(status) +
	   ((status == BT_AVRCP_STATUS_OPERATION_COMPLETED) ? sizeof(num_items) : 0U)) {
		LOG_ERR("Not enough tailroom for CHANGE_PATH rsp");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	net_buf_add_u8(buf, status);
	if (status == BT_AVRCP_STATUS_OPERATION_COMPLETED) {
		net_buf_add_be32(buf, num_items);
	}

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_CHANGE_PATH,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send CHANGE_PATH rsp (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_get_item_attrs(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		return err;
	}

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_GET_ITEM_ATTRS,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_ITEM_ATTRIBUTES rsp (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_get_total_number_of_items(struct bt_avrcp_tg *tg, uint8_t tid,
						   struct net_buf *buf)
{
	int err;

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		return err;
	}

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GET_TOTAL_NUMBER_OF_ITEMS rsp (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_search(struct bt_avrcp_tg *tg, uint8_t tid, struct net_buf *buf)
{
	int err;

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		return err;
	}

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_SEARCH,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send SEARCH rsp (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_browsing_general_reject(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	if (AVRCP_STATUS_IS_REJECTED(status) != true) {
		LOG_ERR("Invalid status");
		return -EINVAL;
	}

	buf = avrcp_create_browsing_pdu(tg->avrcp);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	err = avrcp_tg_precheck(tg, buf);
	if (err < 0) {
		LOG_ERR("AVRCP TG precheck failed: %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (net_buf_tailroom(buf) < sizeof(status)) {
		LOG_ERR("Not enough tailroom for GENERAL_REJECT rsp");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, status);

	err = bt_avrcp_browsing_send(tg->avrcp, buf, BT_AVRCP_PDU_ID_GENERAL_REJECT,
				     BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send GENERAL_REJECT rsp (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;

}
#endif /* CONFIG_BT_AVRCP_BROWSING */

static int build_notification_rsp_data(uint8_t event_id, struct bt_avrcp_event_data *data,
				       struct net_buf *buf)
{
	uint16_t param_len = sizeof(event_id);

	/* Calculate parameter length based on event type */
	switch (event_id) {
	case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
		param_len += sizeof(data->play_status);
		break;
	case BT_AVRCP_EVT_TRACK_CHANGED:
		param_len += sizeof(data->identifier);
		break;
	case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
		param_len += sizeof(data->playback_pos);
		break;
	case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
		param_len += sizeof(data->battery_status);
		break;
	case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
		param_len += sizeof(data->system_status);
		break;
	case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
		param_len += sizeof(data->setting_changed.num_of_attr) +
		data->setting_changed.num_of_attr * sizeof(struct bt_avrcp_app_setting_attr_val);
		break;
	case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
		param_len += sizeof(data->addressed_player_changed);
		break;
	case BT_AVRCP_EVT_UIDS_CHANGED:
		param_len += sizeof(data->uid_counter);
		break;
	case BT_AVRCP_EVT_VOLUME_CHANGED:
		param_len += sizeof(data->absolute_volume);
		break;
	case BT_AVRCP_EVT_TRACK_REACHED_END:
	case BT_AVRCP_EVT_TRACK_REACHED_START:
	case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
	case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
		break;
	default:
		return -EINVAL;
	}

	if (net_buf_tailroom(buf) < param_len) {
		LOG_ERR("Not enough space in net_buf");
		return -ENOMEM;
	}

	net_buf_add_u8(buf, event_id);
	switch (event_id) {
	case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
		if (data->play_status > BT_AVRCP_PLAYBACK_STATUS_REV_SEEK &&
		    data->play_status != BT_AVRCP_PLAYBACK_STATUS_ERROR) {
			LOG_ERR("Invalid playback status: %d", data->play_status);
			return -EINVAL;
		}
		net_buf_add_u8(buf, data->play_status);
		break;
	case BT_AVRCP_EVT_TRACK_CHANGED: {
		uint64_t identifier = sys_get_be64(data->identifier);

		net_buf_add_be64(buf, identifier);
		break;
	}
	case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
		net_buf_add_be32(buf, data->playback_pos);
		break;
	case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
		if (data->battery_status > BT_AVRCP_BATTERY_STATUS_FULL) {
			LOG_ERR("Invalid battery status: %d", data->battery_status);
			return -EINVAL;
		}
		net_buf_add_u8(buf, data->battery_status);
		break;
	case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
		if (data->system_status > BT_AVRCP_SYSTEM_STATUS_UNPLUGGED) {
			LOG_ERR("Invalid system status: %d", data->system_status);
			return -EINVAL;
		}
		net_buf_add_u8(buf, data->system_status);
		break;
	case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
		net_buf_add_u8(buf, data->setting_changed.num_of_attr);
		net_buf_add_mem(buf, data->setting_changed.attr_vals,
		data->setting_changed.num_of_attr * sizeof(struct bt_avrcp_app_setting_attr_val));
		break;
	case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
		net_buf_add_be16(buf, data->addressed_player_changed.player_id);
		net_buf_add_be16(buf, data->addressed_player_changed.uid_counter);
		break;
	case BT_AVRCP_EVT_UIDS_CHANGED:
		net_buf_add_be16(buf, data->uid_counter);
		break;
	case BT_AVRCP_EVT_VOLUME_CHANGED:
		if (data->absolute_volume > BT_AVRCP_MAX_ABSOLUTE_VOLUME) {
			LOG_ERR("Invalid absolute volume: %d", data->absolute_volume);
			return -EINVAL;
		}
		net_buf_add_u8(buf, data->absolute_volume);
		break;
	case BT_AVRCP_EVT_TRACK_REACHED_END:
	case BT_AVRCP_EVT_TRACK_REACHED_START:
	case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
	case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int bt_avrcp_tg_notification(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status, uint8_t event_id,
			     struct bt_avrcp_event_data *data)
{
	uint8_t rsp_code = BT_AVRCP_RSP_INTERIM;
	struct net_buf *buf;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (event_id >= ARRAY_SIZE(tg->tg_notify)) {
		LOG_ERR("Invalid event_id");
		return -EINVAL;
	}

	if (tg->tg_notify[event_id].registered == false) {
		LOG_ERR("Notification response failed: event not registered");
		return -ENOENT;
	}

	switch (status) {
	case BT_AVRCP_STATUS_SUCCESS:
		if (tg->tg_notify[event_id].interim_sent == false) {
			rsp_code = BT_AVRCP_RSP_INTERIM;
		} else {
			rsp_code = BT_AVRCP_RSP_CHANGED;
		}
		break;
	case BT_AVRCP_STATUS_NOT_IMPLEMENTED:
		if (tg->tg_notify[event_id].interim_sent == true) {
			LOG_ERR("Not support INTERIM has been responded");
			return -EINVAL;
		}
		rsp_code = BT_AVRCP_RSP_NOT_IMPLEMENTED;
		break;
	case BT_AVRCP_STATUS_IN_TRANSITION:
		LOG_ERR("Not support IN_TRANSITION");
		return -EINVAL;

	default:
		rsp_code = BT_AVRCP_RSP_REJECTED;
		break;
	}

	buf = bt_avrcp_create_vendor_pdu(NULL);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	if (rsp_code == BT_AVRCP_RSP_REJECTED) {
		if (net_buf_tailroom(buf) < sizeof(status)) {
			LOG_ERR("Not enough space in net_buf");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_u8(buf, status);
	} else {
		err = build_notification_rsp_data(event_id, data, buf);
		if (err < 0) {
			net_buf_unref(buf);
			return err;
		}
	}

	err =  bt_avrcp_tg_send_vendor_rsp(tg, tid, BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION,
					   rsp_code, buf);
	if (err < 0) {
		LOG_ERR("Failed to send notification response (err: %d)", err);
		net_buf_unref(buf);
		return err;
	}

	if (rsp_code == BT_AVRCP_RSP_INTERIM) {
		tg->tg_notify[event_id].interim_sent = true;
	} else {
		tg->tg_notify[event_id].registered = false;
		tg->tg_notify[event_id].interim_sent = false;
	}

	return err;
}

int bt_avrcp_tg_send_passthrough_rsp(struct bt_avrcp_tg *tg, uint8_t tid, bt_avrcp_rsp_t result,
				     struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (net_buf_headroom(buf) < sizeof(struct bt_avrcp_header)) {
		LOG_ERR("Not enough headroom in buffer for bt_avrcp_header");
		return -ENOMEM;
	}
	avrcp_hdr = net_buf_push(buf, sizeof(struct bt_avrcp_header));
	memset(avrcp_hdr, 0, sizeof(struct bt_avrcp_header));

	avrcp_set_passthrough_header(avrcp_hdr, result);

	err = avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP PDU (err: %d)", err);
	}
	return err;
}

static int bt_avrcp_tg_send_vendor_dependent_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t pdu_id
						 , uint8_t status, struct net_buf *buf)
{
	uint8_t rsp_code;
	struct net_buf *status_buf = NULL;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	err = bt_avrcp_status_to_rsp(pdu_id, status, &rsp_code);
	if (err < 0) {
		return err;
	}

	if (status != BT_AVRCP_STATUS_SUCCESS) {
		status_buf = bt_avrcp_create_vendor_pdu(NULL);
		if (status_buf == NULL) {
			LOG_ERR("Failed to allocate status buffer");
			return -ENOBUFS;
		}
	} else {
		__ASSERT(buf != NULL, "Buffer is NULL for PDU ID: 0x%02X", pdu_id);
	}

	/* Set the REJECTED response with the status payload */
	if (rsp_code == BT_AVRCP_RSP_REJECTED) {
		if (net_buf_tailroom(status_buf) < sizeof(status)) {
			LOG_ERR("Not enough space in status net_buf");
			net_buf_unref(status_buf);
			return -ENOMEM;
		}
		net_buf_add_u8(status_buf, status);
	}

	if (status != BT_AVRCP_STATUS_SUCCESS) {
		err = bt_avrcp_tg_send_vendor_rsp(tg, tid, pdu_id, rsp_code, status_buf);
		if (err < 0) {
			LOG_ERR("Failed to send vendor status PDU (err: %d)", err);
			net_buf_unref(status_buf);
			return err;
		}
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		return 0;
	}

	/* SUCCESS response send caller-provided buffer */
	uint8_t min_len = get_rsp_min_len_by_pdu(pdu_id);

	if (buf->len < min_len) {
		LOG_ERR("Buffer too small for PDU 0x%02X (len=%u, min=%u)",
			pdu_id, buf->len, min_len);
		return -EINVAL;
	}

	err = bt_avrcp_tg_send_vendor_rsp(tg, tid, pdu_id, rsp_code, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_get_caps(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status, struct net_buf *buf)
{
	int err;

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_GET_CAPS, status,
						    buf);
	if (err < 0) {
		LOG_ERR("Failed to send Get Capabilities response (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_list_player_app_setting_attrs(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
					      struct net_buf *buf)
{

	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						     BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS,
						     status, buf);
}

int bt_avrcp_tg_list_player_app_setting_vals(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
					     struct net_buf *buf)
{
	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						     BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS,
						     status, buf);
}

int bt_avrcp_tg_get_curr_player_app_setting_val(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
						struct net_buf *buf)
{
	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						     BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL
						     , status, buf);
}

int bt_avrcp_tg_set_player_app_setting_val(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						    BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_get_player_app_setting_attr_text(struct bt_avrcp_tg *tg, uint8_t tid,
						 uint8_t status, struct net_buf *buf)
{
	uint8_t pdu_id = BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT;

	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, pdu_id, status, buf);
}

int bt_avrcp_tg_get_player_app_setting_val_text(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
						struct net_buf *buf)
{
	uint8_t pdu_id = BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT;

	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, pdu_id, status, buf);
}

int bt_avrcp_tg_inform_displayable_char_set(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						    BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_inform_batt_status_of_ct(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						    BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_absolute_volume(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				uint8_t absolute_volume)
{
	struct net_buf *buf;
	int err;

	if (absolute_volume > BT_AVRCP_MAX_ABSOLUTE_VOLUME) {
		return -EINVAL;
	}

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	if (net_buf_tailroom(buf) < sizeof(absolute_volume)) {
		LOG_ERR("Not enough space in net_buf");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, absolute_volume);

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to absolute volume (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_get_element_attrs(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				  struct net_buf *buf)
{
	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						     BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS,
						     status, buf);
}

int bt_avrcp_tg_get_play_status(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status,
				struct net_buf *buf)
{
	return bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid,
						     BT_AVRCP_PDU_ID_GET_PLAY_STATUS,
						     status, buf);
}

int bt_avrcp_tg_set_addressed_player(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	if (status <= BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED) {
		if (net_buf_tailroom(buf) < sizeof(status)) {
			LOG_ERR("Not enough space in net_buf");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_u8(buf, status);
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_play_item(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	if (status <= BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED) {
		if (net_buf_tailroom(buf) < sizeof(status)) {
			LOG_ERR("Not enough space in net_buf");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_u8(buf, status);
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_PLAY_ITEM,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

int bt_avrcp_tg_add_to_now_playing(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t status)
{
	struct net_buf *buf;
	int err;

	buf = bt_avrcp_create_vendor_pdu(&avrcp_vd_tx_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOBUFS;
	}

	if (status <= BT_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED) {
		if (net_buf_tailroom(buf) < sizeof(status)) {
			LOG_ERR("Not enough space in net_buf");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_u8(buf, status);
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING,
						    status, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor dependent (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

struct bt_avrcp_tg *bt_avrcp_get_tg(struct bt_conn *conn, uint16_t psm)
{
	size_t index;

	ARG_UNUSED(psm);

	if (conn == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(bt_avrcp_tg_pool), "Conn index is out of bounds");

	return &bt_avrcp_tg_pool[index];
}

struct bt_conn *bt_avrcp_ct_get_acl_conn(struct bt_avrcp_ct *ct)
{
	if (ct == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	if (ct->avrcp->acl_conn == NULL) {
		LOG_ERR("No connection");
		return NULL;
	}

	return bt_conn_ref(ct->avrcp->acl_conn);
}
