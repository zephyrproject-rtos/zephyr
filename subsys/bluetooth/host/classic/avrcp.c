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
};

struct bt_avrcp_tg {
	struct bt_avrcp *avrcp;

	/* AVRCP TG TX pending */
	sys_slist_t tx_pending;

	/* Critical locker */
	struct k_sem lock;

	/* TX work */
	struct k_work_delayable tx_work;
};

struct avrcp_handler {
	bt_avrcp_opcode_t opcode;
	void (*func)(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf);
};

struct avrcp_pdu_vendor_handler  {
	bt_avrcp_pdu_id_t pdu_id;
	uint8_t min_len;
	void (*func)(struct bt_avrcp *avrcp, uint8_t tid, uint8_t result, struct net_buf *buf);
};

typedef struct {
	uint8_t pdu_id;
	bt_avrcp_ctype_t cmd_type;
} bt_avrcp_pdu_cmd_map_t;

static const bt_avrcp_pdu_cmd_map_t avrcp_pdu_cmd_table[] = {
	{  BT_AVRCP_PDU_ID_GET_CAPS                          , BT_AVRCP_CTYPE_STATUS },
	{  BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS     , BT_AVRCP_CTYPE_STATUS },
	{  BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS      , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL   , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL        , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT  , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT   , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET	     , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT	     , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS                 , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_GET_PLAY_STATUS                   , BT_AVRCP_CTYPE_STATUS},
	{  BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION	     , BT_AVRCP_CTYPE_NOTIFY},
	{  BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME               , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER              , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_PLAY_ITEM                         , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING                , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP                , BT_AVRCP_CTYPE_CONTROL},
	{  BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP              , BT_AVRCP_CTYPE_CONTROL},
};

NET_BUF_POOL_FIXED_DEFINE(avrcp_pool, CONFIG_BT_MAX_CONN*3,
			  CONFIG_BT_AVRCP_DATA_BUF_SIZE,
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

/*
 * This macros returns true if the CT/TG has been initialized, which
 * typically happens after the avrcp callack have been registered.
 * Use these macros to determine whether the CT/TG role is supported.
 */
#define IS_CT_ROLE_SUPPORTED() (avrcp_ct_cb != NULL)
#define IS_TG_ROLE_SUPPORTED() (avrcp_tg_cb != NULL)

static const struct bt_avrcp_ct_cb *avrcp_ct_cb;
static const struct bt_avrcp_tg_cb *avrcp_tg_cb;
static struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_CONN];
static struct bt_avrcp_ct bt_avrcp_ct_pool[CONFIG_BT_MAX_CONN];
static struct bt_avrcp_tg bt_avrcp_tg_pool[CONFIG_BT_MAX_CONN];

static struct bt_avctp_server avctp_server;
#if defined(CONFIG_BT_AVRCP_BROWSING)
static struct bt_avctp_server avctp_browsing_server;
#endif /* CONFIG_BT_AVRCP_BROWSING */
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
	BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2 | AVRCP_BROWSING_ENABLE),
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
	BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2 | AVRCP_BROWSING_ENABLE),
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

static struct net_buf *avrcp_create_subunit_pdu(struct bt_avrcp *avrcp,  uint8_t ctype_or_rsp)
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

	buf = bt_avctp_create_pdu(&avrcp_pool);
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
	return bt_avctp_create_pdu(NULL);
}

static int avrcp_browsing_send(struct bt_avrcp *avrcp, struct net_buf *buf, bt_avctp_cr_t cr,
			       uint8_t tid)
{
	int err;
	struct bt_avrcp_avc_brow_pdu *hdr =
		(struct bt_avrcp_avc_brow_pdu *)(buf->data);

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

static int init_fragmentation_context(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t rsp,
				      uint16_t total_len)
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
		return -ENOMEM;
	}

	/* Allocate reassembly buffer */
	ct->reassembly_buf = net_buf_alloc(&avrcp_pool, K_NO_WAIT);
	if (ct->reassembly_buf == NULL) {
		LOG_ERR("Failed to allocate reassembly buffer");
		return -ENOMEM;
	}

	__ASSERT_NO_MSG(ct->reassembly_buf->user_data_size >= sizeof(*ctx));
	ctx = net_buf_user_data(ct->reassembly_buf);

	ctx->tid = tid;
	ctx->rsp = rsp;
	ctx->total_len = total_len;
	ctx->received_len = 0U;
	return 0;
}

static struct bt_avrcp_ct_frag_reassembly_ctx * get_fragmentation_context(struct bt_avrcp_ct *ct)
{
	struct bt_avrcp_ct_frag_reassembly_ctx *ctx;

	if (ct == NULL) {
		return NULL;
	}

	__ASSERT_NO_MSG(ct->reassembly_buf->user_data_size >= sizeof(*ctx));
	ctx = net_buf_user_data(ct->reassembly_buf);
	return ctx;
}

static int add_fragment_data(struct bt_avrcp_ct *ct, const uint8_t *data, uint16_t data_len)
{
	struct bt_avrcp_ct_frag_reassembly_ctx *ctx;

	if ((ct == NULL) || (data == NULL) || (ct->reassembly_buf == NULL)) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(ct->reassembly_buf->user_data_size >= sizeof(*ctx));
	ctx = net_buf_user_data(ct->reassembly_buf);

	/* Check if adding this fragment would exceed total expected length */
	if ((ctx->received_len + data_len) > ctx->total_len) {
		LOG_ERR("Fragment data exceeds expected total length");
		return -EINVAL;
	}

	if (net_buf_tailroom(ct->reassembly_buf) < data_len) {
		LOG_ERR("Insufficient space in reassembly buffer");
		return -ENOMEM;
	}

	/* Add fragment data to reassembly buffer */
	net_buf_add_mem(ct->reassembly_buf, data, data_len);
	ctx->received_len += data_len;

	if (ctx->received_len >= ctx->total_len) {
		LOG_ERR("Fragment data exceeds expected total length");
		return -ENOMEM;
	}
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

inline static bt_avrcp_ctype_t get_cmd_type_by_pdu(uint8_t pdu_id) {

	ARRAY_FOR_EACH(avrcp_pdu_cmd_table, i) {
		if (avrcp_pdu_cmd_table[i].pdu_id == pdu_id) {
		return avrcp_pdu_cmd_table[i].cmd_type;
		}
	}
	LOG_WRN("Unknown PDU ID: 0x%02X", pdu_id);
	return (bt_avrcp_ctype_t)-1;
}

static struct net_buf *avrcp_prepare_vendor_pdu(void *avrcp, bt_avrcp_pkt_type_t pkt_type,
						uint8_t avrcp_type, uint8_t pdu_id, uint16_t param_len)
{
	struct net_buf *buf;
	struct bt_avrcp_avc_pdu *pdu;
	size_t required_size = sizeof(*pdu);

	if (avrcp == NULL) {
		return NULL;
	}

	buf = avrcp_create_vendor_pdu(avrcp, avrcp_type);
	if (buf == NULL) {
		LOG_WRN("Insufficient buffer");
		return NULL;
	}

	if (net_buf_tailroom(buf) < required_size) {
		LOG_WRN("Not enough tailroom: required");
		net_buf_unref(buf);
		return NULL;
	}

	pdu = net_buf_add(buf, sizeof(*pdu));
	sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, pdu->company_id);
	pdu->pdu_id = pdu_id;
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, pkt_type);
	pdu->param_len = sys_cpu_to_be16(param_len);

	return buf;
}

static int send_single_vendor_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t rsp, uint8_t pdu_id,
				  struct net_buf *buf)
{
	struct bt_avrcp_header *hdr;
	struct bt_avrcp_avc_pdu *pdu;
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
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, BT_AVRVP_PKT_TYPE_SINGLE);
	pdu->param_len = sys_cpu_to_be16(param_len);

	hdr = net_buf_push(buf, sizeof(struct bt_avrcp_header));
	memset(hdr, 0, sizeof(struct bt_avrcp_header));
	BT_AVRCP_HDR_SET_CTYPE_OR_RSP(hdr, get_cmd_type_by_pdu(pdu_id));
	BT_AVRCP_HDR_SET_SUBUNIT_ID(hdr, BT_AVRCP_SUBUNIT_ID_ZERO);
	BT_AVRCP_HDR_SET_SUBUNIT_TYPE(hdr, BT_AVRCP_SUBUNIT_TYPE_PANEL);
	hdr->opcode = BT_AVRCP_OPC_VENDOR_DEPENDENT;

	return avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
}

static int send_fragmented_vendor_rsp(struct bt_avrcp_tg_tx *tx, bt_avrcp_pkt_type_t pkt_type,
				      uint8_t *data, uint16_t data_len)
{
	struct bt_avrcp_tg *tg = tx->tg;
	struct net_buf *buf;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	buf = avrcp_prepare_vendor_pdu(tg->avrcp, pkt_type, tx->rsp, tx->pdu_id, data_len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (data_len > 0U) {
		if (net_buf_tailroom(buf) < data_len) {
			LOG_WRN("Not enough tailroom: required");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_mem(buf, data, data_len);
	}
	return avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tx->tid);
}

static int bt_avrcp_ct_send_req_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t rsp,
				    uint8_t pdu_id)
{
	struct net_buf *buf;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	buf = avrcp_prepare_vendor_pdu(ct->avrcp, BT_AVRVP_PKT_TYPE_SINGLE, BT_AVRCP_CTYPE_CONTROL,
				       rsp, sizeof(pdu_id));
	if (buf == NULL) {
		return -ENOMEM;
	}

	/* Add pdu_id status */
	if (net_buf_tailroom(buf) < sizeof(pdu_id)) {
		LOG_WRN("Not enough tailroom for pdu_id");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, pdu_id);

	return avrcp_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
}

static int bt_avrcp_tg_send_vendor_err_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
					   uint8_t pdu_id, uint8_t err_code)
{
	struct net_buf *buf;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	buf = avrcp_prepare_vendor_pdu(tg->avrcp, BT_AVRVP_PKT_TYPE_SINGLE, BT_AVRCP_RSP_REJECTED,
				       pdu_id, 1U);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(err_code)) {
		LOG_WRN("Not enough tailroom for err_code");
		net_buf_unref(buf);
		return -ENOMEM;
	}
	net_buf_add_u8(buf, err_code);

	err = avrcp_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		net_buf_unref(buf);
		if (bt_avrcp_disconnect(tg->avrcp->acl_conn)) {
			LOG_ERR("Failed to disconnect AVRCP connection");
		}
	}
	return err;
}

static int bt_avrcp_ct_send_req_continuing_rsp(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t pdu_id)
{
	return bt_avrcp_ct_send_req_rsp(ct, tid, BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP, pdu_id);
}

static int bt_avrcp_ct_send_abort_continuing_rsp(struct bt_avrcp_ct *ct, uint8_t tid,
						 uint8_t pdu_id)
{
	tid++;
	tid &= 0x0F;
	return bt_avrcp_ct_send_req_rsp(ct, tid, BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP, pdu_id);
}

static void bt_avrcp_tg_set_tx_state(struct bt_avrcp_tg *tg, avrcp_tg_rsp_state_t state, uint8_t tid,
				     struct net_buf *buf)
{
	sys_snode_t *node;
	struct bt_avrcp_tg_tx *tx;
	struct net_buf *tx_buf;
	uint8_t pdu_id;

	avrcp_tg_lock(tg);

	node = sys_slist_peek_head(&tg->tx_pending);
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

	tx->state = state;
	tx->tid = tid;

	switch (state) {
	case AVRCP_STATE_ABORT_CONTINUING:
		tx->rsp = (pdu_id == tx->pdu_id) ? BT_AVRCP_RSP_ACCEPTED : BT_AVRCP_RSP_REJECTED;
		tx->pdu_id = BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP;
		break;

	case AVRCP_STATE_SENDING_CONTINUING:
		if (pdu_id != tx->pdu_id) {
			tx->rsp = BT_AVRCP_RSP_REJECTED;
			tx->pdu_id = BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP;
		}
		break;

	default:
		break;
	}

	avrcp_tg_unlock(tg);
}

static void avrcp_tg_tx_remove(struct bt_avrcp_tg *tg, struct net_buf *buf)
{
	avrcp_tg_lock(tg);
	sys_slist_find_and_remove(&tg->tx_pending, &buf->node);
	avrcp_tg_unlock(tg);
}

static void bt_avrcp_tg_vendor_tx_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_avrcp_tg *tg = CONTAINER_OF(dwork, struct bt_avrcp_tg, tx_work);
	sys_snode_t *node;
	struct net_buf *buf;
	struct bt_avrcp_tg_tx *tx;
	uint16_t max_payload_size;
	uint16_t sent_len = 0U;
	int err;

	avrcp_tg_lock(tg);

	node = sys_slist_peek_head(&tg->tx_pending);
	if (node == NULL) {
		LOG_WRN("No pending tx");
		avrcp_tg_unlock(tg);
		return;
	}
	avrcp_tg_unlock(tg);

	buf = CONTAINER_OF(node, struct net_buf, node);

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*tx));
	tx = net_buf_user_data(buf);

	/* Calculate maximum payload size per fragment */
	max_payload_size = BT_AVRCP_FRAGMENT_SIZE - sizeof(struct bt_avrcp_header)
			   - sizeof(struct bt_avrcp_avc_pdu);

	/* Check if fragmentation is needed */
	if ((tx->sent_len == 0) && (buf->len <= max_payload_size)) {
		/* Single packet response */
		LOG_WRN("headroom :%d", net_buf_headroom(buf));
		err = send_single_vendor_rsp(tg, tx->tid, tx->rsp, tx->pdu_id, buf);
		if (err < 0) {
			LOG_ERR("Failed to send fragment at offset %u", sent_len);
			goto done;
		}
		avrcp_tg_tx_remove(tg, buf);
		return;
	}
	/* Multi-packet fragmented response */
	bt_avrcp_pkt_type_t pkt_type = BT_AVRVP_PKT_TYPE_SINGLE;
	uint16_t chunk_size = MIN(max_payload_size, buf->len);

	if ((tx->state == AVRCP_STATE_ABORT_CONTINUING) ||
	    (tx->rsp == BT_AVRCP_RSP_REJECTED)) {
		LOG_ERR("Abort to continuting send OR REJECT");
		struct net_buf *rsp_buf;
		uint8_t error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;

		rsp_buf = bt_avrcp_create_pdu(NULL);
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
		pkt_type = BT_AVRVP_PKT_TYPE_START;
	} else if (tx->state == AVRCP_STATE_SENDING_CONTINUING) {
		/* Last fragment */
		if (chunk_size >= buf->len) {
			pkt_type = BT_AVRVP_PKT_TYPE_END;
		} else {
			pkt_type = BT_AVRVP_PKT_TYPE_CONTINUE;
		}
	}

	/* Send fragment */
	err = send_fragmented_vendor_rsp(tx, pkt_type, buf->data, chunk_size);
	if (err < 0) {
		LOG_ERR("Failed to send fragment at offset %u", tx->sent_len);
		goto done;
	}

	if (buf->len < chunk_size) {
		LOG_WRN("Not enough buf len (%d < %d)", buf->len, chunk_size);
		goto done;
	}
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
	k_work_reschedule(&tg->tx_work, K_NO_WAIT);
}

static int bt_avrcp_tg_send_vendor_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t pdu_id,
				       uint8_t result, struct net_buf *buf)
{
	struct bt_avrcp_tg_tx *tx;

	if ((tg == NULL) || (tg->avrcp == NULL) || (buf == NULL)) {
		net_buf_unref(buf);
		return -EINVAL;
	}

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*tx));
	tx = net_buf_user_data(buf);

	tx->tg = tg;
	tx->tid = tid;
	tx->rsp = result;
	tx->pdu_id = pdu_id;
	tx->sent_len = 0U;
	tx->state = AVRCP_STATE_IDLE;

	LOG_WRN("Sending vendor dependent response: tid=%u, total_len=%u", tid, buf->len);
	avrcp_tg_lock(tg);
	sys_slist_append(&tg->tx_pending, &buf->node);
	avrcp_tg_unlock(tg);

	k_work_reschedule(&tg->tx_work, K_NO_WAIT);

	return 0;
}

static void process_get_caps_rsp(struct bt_avrcp *avrcp, uint8_t tid, bt_avrcp_rsp_t result,
				 struct net_buf *buf)
{
	struct bt_avrcp_get_caps_rsp *rsp;
	uint16_t expected_len;

	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->get_caps_rsp == NULL)) {
		return;
	}

	rsp = (struct bt_avrcp_get_caps_rsp *)buf->data;

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

	avrcp_ct_cb->get_caps_rsp(get_avrcp_ct(avrcp), tid, rsp);
}

static const struct avrcp_pdu_vendor_handler  rsp_vendor_handlers[] = {
	{BT_AVRCP_PDU_ID_GET_CAPS, sizeof(struct bt_avrcp_get_caps_rsp), process_get_caps_rsp},
};

static int handle_vendor_pdu(struct bt_avrcp *avrcp, uint8_t tid, struct net_buf *buf, uint8_t ctype,
			     uint8_t pdu_id, const struct avrcp_pdu_vendor_handler *handlers,
			     size_t num_handlers)
{
	for (size_t i = 0; i < num_handlers; i++) {
		const struct avrcp_pdu_vendor_handler *handler = &handlers[i];

		if (handler->pdu_id != pdu_id) {
			continue;
		}

		if (buf->len < handler->min_len) {
			LOG_ERR("Too small (%u bytes) pdu_id 0x%02x", buf->len, pdu_id);
			return -EINVAL;
		}

		handler->func(avrcp, tid, ctype, buf);
		return 0;
	}

	return -EOPNOTSUPP;
}

static void process_common_vendor_rsp(struct bt_avrcp *avrcp, struct bt_avrcp_avc_pdu *pdu,
				      uint8_t tid, bt_avrcp_rsp_t result, struct net_buf *buf)
{
	uint16_t param_len;

	if (pdu == NULL || buf == NULL) {
		LOG_ERR("Invalid parameters");
		return;
	}

	param_len = sys_be16_to_cpu(pdu->param_len);
	if (buf->len < param_len) {
		LOG_ERR("Invalid element attributes length: %u, buf len %u", param_len, buf->len);
		return;
	}

 	handle_vendor_pdu(avrcp, tid, buf, result, pdu->pdu_id, rsp_vendor_handlers,
			  ARRAY_SIZE(rsp_vendor_handlers));
}

static void avrcp_vendor_dependent_rsp_handler(struct bt_avrcp *avrcp, uint8_t tid,
					       struct net_buf *buf)
{
	struct bt_avrcp_avc_pdu *pdu;
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	bt_avrcp_rsp_t rsp;
	uint8_t pdu_id;
	int err;

	if (buf->len < (sizeof(*avrcp_hdr) + BT_AVRCP_COMPANY_ID_SIZE + sizeof(pdu_id))) {
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

	if (buf->len < sizeof(*pdu)) {
		LOG_ERR("Invalid vendor payload length: %u", buf->len);
		return;
	}
	pdu = net_buf_pull_mem(buf, sizeof(*pdu));
	if (sys_get_be24(pdu->company_id) != BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG) {
		LOG_ERR("Invalid company id: 0x%06x", sys_get_be24(pdu->company_id));
		return;
	}
	switch (pdu->pkt_type) {
	case BT_AVRVP_PKT_TYPE_SINGLE:
		/* Single packet should not have incomplete fragment  */
		if (NULL != get_avrcp_ct(avrcp)->reassembly_buf) {
			LOG_ERR("Single packet should not have incomplete fragment");
			goto failure;
		}
		process_common_vendor_rsp(avrcp, pdu, tid, rsp, buf);
		break;

	case BT_AVRVP_PKT_TYPE_START:
		err = init_fragmentation_context(get_avrcp_ct(avrcp), tid, rsp,
						 CONFIG_BT_AVRCP_DATA_BUF_SIZE);
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

	case BT_AVRVP_PKT_TYPE_CONTINUE:
		/* Continuation fragment */
		if ((NULL == get_avrcp_ct(avrcp)->reassembly_buf) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->tid != tid) ||
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

	case BT_AVRVP_PKT_TYPE_END:
		/* Final fragment */
		if ((NULL == get_avrcp_ct(avrcp)->reassembly_buf) ||
		    (get_fragmentation_context(get_avrcp_ct(avrcp))->tid != tid) ||
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
		process_common_vendor_rsp(avrcp, pdu, tid, rsp, get_avrcp_ct(avrcp)->reassembly_buf);

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

static void process_get_caps_cmd(struct bt_avrcp *avrcp, uint8_t tid, uint8_t ctype_or_rsp,
				struct net_buf *buf)
{
	uint8_t cap_id;
	uint8_t error_code;
	int err;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->get_cap_req == NULL)) {
		error_code = BT_AVRCP_STATUS_INTERNAL_ERROR;
		goto err_rsp;
	}

	cap_id = net_buf_pull_u8(buf);

	/* Validate capability ID */
	if ((cap_id != BT_AVRCP_CAP_COMPANY_ID) &&
	    (cap_id != BT_AVRCP_CAP_EVENTS_SUPPORTED)) {
		LOG_ERR("Invalid capability ID: 0x%02x", cap_id);
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	return avrcp_tg_cb->get_cap_req(get_avrcp_tg(avrcp), tid, cap_id);

err_rsp:
	err = bt_avrcp_tg_send_vendor_err_rsp(get_avrcp_tg(avrcp), tid,
					      BT_AVRCP_PDU_ID_GET_CAPS,
					      error_code);
	if (err < 0) {
		LOG_ERR("Failed to send GetElementAttributes error response");
	}
}

static void handle_avrcp_continuing_rsp(struct bt_avrcp *avrcp, uint8_t tid, uint8_t ctype_or_rsp,
					struct net_buf *buf)
{
	LOG_DBG("Received Continuing Response");
	bt_avrcp_tg_set_tx_state(get_avrcp_tg(avrcp), AVRCP_STATE_SENDING_CONTINUING, tid, buf);
	k_work_reschedule(&get_avrcp_tg(avrcp)->tx_work, K_NO_WAIT);
}

static void handle_avrcp_abort_continuing_rsp(struct bt_avrcp *avrcp, uint8_t tid,
					      uint8_t ctype_or_rsp, struct net_buf *buf)
{
	LOG_DBG("Received Abort Continuing Response");
	bt_avrcp_tg_set_tx_state(get_avrcp_tg(avrcp), AVRCP_STATE_ABORT_CONTINUING, tid, buf);
	k_work_reschedule(&get_avrcp_tg(avrcp)->tx_work, K_NO_WAIT);
}

static const struct avrcp_pdu_vendor_handler  cmd_vendor_handlers[] = {
	{BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP, sizeof(uint8_t), handle_avrcp_continuing_rsp},
	{BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP, sizeof(uint8_t), handle_avrcp_abort_continuing_rsp},
	{BT_AVRCP_PDU_ID_GET_CAPS, sizeof(uint8_t), process_get_caps_cmd},
};

static void avrcp_vendor_dependent_cmd_handler(struct bt_avrcp *avrcp, uint8_t tid,
					       struct net_buf *buf)
{
	struct bt_avrcp_header *avrcp_hdr;
	bt_avrcp_subunit_type_t subunit_type;
	bt_avrcp_subunit_id_t subunit_id;
	struct bt_avrcp_avc_pdu *pdu;
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
		LOG_ERR("Invalid company id: 0x%06x", sys_get_be24(pdu->company_id) );
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	if (BT_AVRCP_AVC_PDU_GET_PACKET_TYPE(pdu) != BT_AVRVP_PKT_TYPE_SINGLE) {
		LOG_ERR("Invalid packet type");
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	len = sys_be16_to_cpu(pdu->param_len);

	if ( buf->len != len) {
		LOG_ERR("Invalid length: %d, buf length = %d", len, buf->len);
		error_code = BT_AVRCP_STATUS_INVALID_PARAMETER;
		goto err_rsp;
	}

	if (ctype_or_rsp !=  get_cmd_type_by_pdu(pdu->pdu_id)) {
		LOG_ERR("Invalid ctype 0x%02x for pdu_id 0x%02x", ctype_or_rsp, pdu->pdu_id);
		error_code = BT_AVRCP_STATUS_INVALID_COMMAND;
		goto err_rsp;
	}

 	handle_vendor_pdu(avrcp, tid, buf, ctype_or_rsp, pdu->pdu_id, cmd_vendor_handlers,
			  ARRAY_SIZE(cmd_vendor_handlers));

err_rsp:
	err = bt_avrcp_tg_send_vendor_err_rsp(get_avrcp_tg(avrcp), tid, pdu_id, error_code);
	if (err < 0) {
		LOG_ERR("Failed to send ADD_TO_NOW_PLAYING error response");
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

	session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
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
	if ((avrcp_ct_cb == NULL) || (avrcp_ct_cb->browsed_player_rsp == NULL)) {
		return -EINVAL;
	}

	avrcp_ct_cb->browsed_player_rsp(get_avrcp_ct(avrcp), tid, buf);

	return 0;
}

static const struct avrcp_pdu_handler rsp_brow_handlers[] = {
	{BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER, sizeof(struct bt_avrcp_set_browsed_player_rsp),
	 avrcp_ct_handle_set_browsed_player},
};

static int avrcp_tg_handle_set_browsed_player_req(struct bt_avrcp *avrcp,
						  uint8_t tid, struct net_buf *buf)
{
	uint16_t player_id;
	struct net_buf *rsp_buf;
	int err;

	if ((avrcp_tg_cb == NULL) || (avrcp_tg_cb->set_browsed_player_req == NULL)) {
		goto error_rsp;
	}

	player_id = net_buf_pull_be16(buf);

	LOG_DBG("Set browsed player request: player_id=0x%04x", player_id);

	avrcp_tg_cb->set_browsed_player_req(get_avrcp_tg(avrcp), tid, player_id);
	return 0;

error_rsp:
	rsp_buf = bt_avrcp_create_pdu(NULL);
	__ASSERT(rsp_buf != NULL, "Failed to allocate response buffer");

	if (net_buf_tailroom(rsp_buf) < sizeof(uint8_t)) {
		LOG_ERR("Insufficient space in response buffer");
		net_buf_unref(rsp_buf);
		return -ENOMEM;
	}
	net_buf_add_u8(rsp_buf, BT_AVRCP_STATUS_INTERNAL_ERROR);

	err = bt_avrcp_tg_send_set_browsed_player_rsp(get_avrcp_tg(avrcp), tid, rsp_buf);
	if (err < 0) {
		LOG_ERR("Failed to send browsed player error response (err: %d)", err);
		net_buf_unref(rsp_buf);
	}
	return err;
}

static const struct avrcp_pdu_handler cmd_brow_handlers[] = {
	{BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER, sizeof(uint16_t),
	 avrcp_tg_handle_set_browsed_player_req},
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
			return -EINVAL;
		}

		return handler->func(avrcp, tid, buf);
	}

	return -EOPNOTSUPP;
}

static int browsing_avrcp_recv(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			       uint8_t tid)
{
	struct bt_avrcp *avrcp = AVRCP_BROW_AVCTP(session);
	struct bt_avrcp_avc_brow_pdu *brow;

	if (buf->len < sizeof(struct bt_avrcp_avc_brow_pdu)) {
		LOG_ERR("Invalid AVRCP browsing header received: buffer too short (%u)", buf->len);
		return -EMSGSIZE;
	}

	brow = net_buf_pull_mem(buf, sizeof(struct bt_avrcp_avc_brow_pdu));

	if (buf->len != sys_be16_to_cpu(brow->param_len)) {
		LOG_ERR("Invalid AVRCP browsing PDU length: expected %u, got %u",
			sys_be16_to_cpu(brow->param_len), buf->len);
		return -EMSGSIZE;
	}

	LOG_DBG("AVRCP browsing msg received, cr:0x%X, tid:0x%X, pdu_id:0x%02X", cr,
		tid, brow->pdu_id);

	if (cr == BT_AVCTP_RESPONSE) {
		return handle_pdu(avrcp, tid, buf, brow->pdu_id, rsp_brow_handlers,
				  ARRAY_SIZE(rsp_brow_handlers));
	}

	return handle_pdu(avrcp, tid, buf, brow->pdu_id, cmd_brow_handlers,
			  ARRAY_SIZE(cmd_brow_handlers));
}

static const struct bt_avctp_ops_cb browsing_avctp_ops = {
	.connected = browsing_avrcp_connected,
	.disconnected = browsing_avrcp_disconnected,
	.recv = browsing_avrcp_recv,
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
		k_work_init_delayable(&bt_avrcp_tg_pool[i].tx_work, bt_avrcp_tg_vendor_tx_work);
		sys_slist_init(&bt_avrcp_tg_pool[i].tx_pending);


		k_sem_init(&bt_avrcp_tg_pool[i].lock, 1, 1);

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

	if (avrcp->browsing_session.br_chan.chan.conn != NULL) {
		/* If browsing session is still active, disconnect it first */
		err = bt_avrcp_browsing_disconnect(conn);
		if (err < 0) {
			LOG_ERR("Browsing session disconnect failed: %d", err);
			return err;
		}
	}

	err = bt_avctp_disconnect(&(avrcp->session));
	if (err < 0) {
		LOG_DBG("AVCTP Disconnect failed");
		return err;
	}

	return err;
}

struct net_buf *bt_avrcp_create_pdu(struct net_buf_pool *pool)
{
	return bt_conn_create_pdu(pool,
				  sizeof(struct bt_l2cap_hdr) +
				  sizeof(struct bt_avctp_header_start) +
				  sizeof(struct bt_avrcp_header) +
				  sizeof(struct bt_avrcp_avc_pdu));
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

#if defined(CONFIG_BT_AVRCP_BROWSING)
int bt_avrcp_ct_set_browsed_player(struct bt_avrcp_ct *ct, uint8_t tid, uint16_t player_id)
{
	struct net_buf *buf;
	struct bt_avrcp_avc_brow_pdu *pdu;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	if (ct->avrcp->browsing_session.br_chan.chan.conn == NULL) {
		LOG_ERR("Browsing session not connected");
		return -ENOTCONN;
	}

	buf = avrcp_create_browsing_pdu(ct->avrcp);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(*pdu) + sizeof(player_id)) {
		LOG_ERR("Not enough tailroom in buffer for browsing PDU");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	pdu = net_buf_add(buf, sizeof(*pdu));
	pdu->pdu_id = BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER;
	pdu->param_len = sys_cpu_to_be16(sizeof(player_id));
	net_buf_add_be16(buf, player_id);

	err = avrcp_browsing_send(ct->avrcp, buf, BT_AVCTP_CMD, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP browsing PDU (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

static int bt_avrcp_ct_vendor_dependent(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t pdu_id,
					struct net_buf *buf)
{
	struct bt_avrcp_header *hdr;
	struct bt_avrcp_avc_pdu *pdu;
	uint16_t param_len;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
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
	BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, BT_AVRVP_PKT_TYPE_SINGLE);
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

int bt_avrcp_ct_get_cap(struct bt_avrcp_ct *ct, uint8_t tid, uint8_t cap_id)
{
	struct net_buf *buf;
	int err;

	if ((ct == NULL) || (ct->avrcp == NULL)) {
		return -EINVAL;
	}
	if (!IS_CT_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	buf = bt_avrcp_create_pdu(&avrcp_pool);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (net_buf_tailroom(buf) < sizeof(cap_id)) {
		LOG_WRN("Not enough headroom: for vcap_id");
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

static int build_get_caps_rsp_data(const struct bt_avrcp_get_caps_rsp *rsp,
				  struct net_buf *buf)
{
	uint16_t param_len;
	uint8_t cap_item_size;

	/* Validate capability ID and calculate parameter length */
	switch (rsp->cap_id) {
	case BT_AVRCP_CAP_COMPANY_ID:
		cap_item_size = BT_AVRCP_COMPANY_ID_SIZE;
		break;
	case BT_AVRCP_CAP_EVENTS_SUPPORTED:
		cap_item_size = 1U;
		break;
	default:
		LOG_ERR("Invalid capability ID: 0x%02x", rsp->cap_id);
		net_buf_unref(buf);
		return -EINVAL;
	}

	param_len = sizeof(rsp->cap_id) + sizeof(rsp->cap_cnt) +
		    (rsp->cap_cnt * cap_item_size);

	if (net_buf_tailroom(buf) < param_len) {
		LOG_ERR("Not enough space in net_buf");
		return -ENOMEM;
	}

	/* Add capability ID */
	net_buf_add_u8(buf, rsp->cap_id);

	/* Add capability count */
	net_buf_add_u8(buf, rsp->cap_cnt);

	/* Add capability data */
	if (rsp->cap_cnt > 0U) {
		net_buf_add_mem(buf, rsp->cap, rsp->cap_cnt * cap_item_size);
	}
	return 0;
}

#if defined(CONFIG_BT_AVRCP_BROWSING)
int bt_avrcp_tg_send_set_browsed_player_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
					    struct net_buf *buf)
{
	struct bt_avrcp_avc_brow_pdu *hdr;
	uint16_t param_len;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL) || (buf == NULL)) {
		LOG_ERR("Invalid AVRCP target");
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		LOG_ERR("Target role not supported");
		return -ENOTSUP;
	}

	if (tg->avrcp->browsing_session.br_chan.chan.conn == NULL) {
		LOG_ERR("Browsing session not connected");
		return -ENOTCONN;
	}

	param_len = buf->len;

	if (net_buf_headroom(buf) < sizeof(struct bt_avrcp_avc_brow_pdu)) {
		LOG_ERR("Not enough headroom in buffer for bt_avrcp_avc_brow_pdu");
		return -ENOMEM;
	}
	hdr = net_buf_push(buf, sizeof(struct bt_avrcp_avc_brow_pdu));
	memset(hdr, 0, sizeof(struct bt_avrcp_avc_brow_pdu));
	hdr->pdu_id = BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER;
	hdr->param_len = sys_cpu_to_be16(param_len);

	err = avrcp_browsing_send(tg->avrcp, buf, BT_AVCTP_RESPONSE, tid);
	if (err < 0) {
		LOG_ERR("Failed to send AVRCP browsing PDU (err: %d)", err);
	}
	return err;
}
#endif /* CONFIG_BT_AVRCP_BROWSING */

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

static int bt_avrcp_tg_send_vendor_dependent_rsp(struct bt_avrcp_tg *tg, uint8_t tid, uint8_t pdu_id,
					  bt_avrcp_rsp_t result, struct net_buf *buf)
{
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL)) {
		return -EINVAL;
	}

	if (!IS_TG_ROLE_SUPPORTED()) {
		return -ENOTSUP;
	}

	err = bt_avrcp_tg_send_vendor_rsp(tg, tid, pdu_id, result, buf);
	if (err < 0) {
		LOG_ERR("Failed to send vendor PDU (err: %d)", err);
	}
	return err;
}

int bt_avrcp_tg_send_get_caps_rsp(struct bt_avrcp_tg *tg, uint8_t tid,
				  const struct bt_avrcp_get_caps_rsp *rsp)
{
	struct net_buf *buf;
	int err;

	if ((tg == NULL) || (tg->avrcp == NULL) || (rsp == NULL)) {
		return -EINVAL;
	}

	buf = bt_avrcp_create_pdu(&avrcp_pool);
	if (NULL == buf) {
		LOG_ERR("Failed to allocate temporary buffer");
		return -ENOMEM;
	}

	err = build_get_caps_rsp_data(rsp, buf);
	if (err < 0) {
		net_buf_unref(buf);
		return err;
	}

	err = bt_avrcp_tg_send_vendor_dependent_rsp(tg, tid, BT_AVRCP_PDU_ID_GET_CAPS,
						    BT_AVRCP_RSP_STABLE, buf);
	if (err < 0) {
		LOG_ERR("Failed to send Get Capabilities response (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}
