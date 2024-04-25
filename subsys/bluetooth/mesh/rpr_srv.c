/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/mesh/rpr_srv.h>
#include <common/bt_str.h>
#include <zephyr/bluetooth/mesh/sar_cfg.h>
#include <zephyr/bluetooth/mesh/keys.h>
#include "access.h"
#include "prov.h"
#include "crypto.h"
#include "rpr.h"
#include "net.h"
#include "mesh.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_rpr_srv);

#define LINK_OPEN_TIMEOUT_DEFAULT 10

#define LINK_CTX(_cli, _send_rel)                                              \
	{                                                                      \
		.net_idx = (_cli)->net_idx, .app_idx = BT_MESH_KEY_DEV_LOCAL,  \
		.addr = (_cli)->addr, .send_ttl = (_cli)->ttl,                 \
		.send_rel = (_send_rel)                                        \
	}

enum {
	SCANNING,
	SCAN_REPORT_PENDING,
	SCAN_EXT_HAS_ADDR,
	NODE_REFRESH,
	URI_MATCHED,
	URI_REQUESTED,

	RPR_SRV_NUM_FLAGS,
};

/** Remote provisioning server instance. */
static struct {
	const struct bt_mesh_model *mod;

	ATOMIC_DEFINE(flags, RPR_SRV_NUM_FLAGS);

	struct {
		struct bt_mesh_rpr_unprov
			devs[CONFIG_BT_MESH_RPR_SRV_SCANNED_ITEMS_MAX];
		uint8_t max_devs;
		enum bt_mesh_rpr_scan state;
		struct k_work_delayable report;
		struct k_work_delayable timeout;
		/* Extended scanning */
		bt_addr_le_t addr;
		uint8_t ad[CONFIG_BT_MESH_RPR_AD_TYPES_MAX];
		uint8_t ad_count;
		/* Time to do regular scanning after extended scanning ends: */
		uint32_t additional_time;
		struct net_buf_simple *adv_data;
		struct bt_mesh_rpr_node cli;
		struct bt_mesh_rpr_unprov *dev;
	} scan;
	struct {
		struct k_work report;
		enum bt_mesh_rpr_link_state state;
		enum bt_mesh_rpr_status status;
		uint8_t close_reason;
		uint8_t tx_pdu;
		uint8_t rx_pdu;
		struct bt_mesh_rpr_node cli;
		struct bt_mesh_rpr_unprov *dev;
	} link;
	struct {
		const struct prov_bearer_cb *cb;
		enum bt_mesh_rpr_node_refresh procedure;
		void *cb_data;
		struct {
			prov_bearer_send_complete_t cb;
			void *cb_data;
		} tx;
	} refresh;
} srv = {
	.scan = {
		.adv_data = NET_BUF_SIMPLE(CONFIG_BT_MESH_RPR_SRV_AD_DATA_MAX)
	}
};

enum bt_mesh_rpr_node_refresh bt_mesh_node_refresh_get(void)
{
	return srv.refresh.procedure;
}

static struct bt_mesh_rpr_unprov *unprov_get(const uint8_t uuid[16])
{
	int i;

	for (i = 0; i < srv.scan.max_devs; ++i) {
		if (uuid) {
			if ((srv.scan.devs[i].flags & BT_MESH_RPR_UNPROV_ACTIVE) &&
			    !memcmp(srv.scan.devs[i].uuid, uuid, 16)) {
				return &srv.scan.devs[i];
			}
		} else if (!(srv.scan.devs[i].flags & BT_MESH_RPR_UNPROV_ACTIVE)) {
			return &srv.scan.devs[i];
		}
	}

	return NULL;
}

static uint8_t *get_ad_type(uint8_t *list, size_t count, uint8_t ad)
{
	int i;

	for (i = 0; i < count; ++i) {
		if (ad == list[i] || (ad == BT_DATA_NAME_SHORTENED &&
				      list[i] == BT_DATA_NAME_COMPLETE)) {
			return &list[i];
		}
	}

	return NULL;
}

static void cli_scan_clear(void)
{
	srv.scan.cli.addr = BT_MESH_ADDR_UNASSIGNED;
	srv.scan.cli.net_idx = BT_MESH_KEY_UNUSED;
}

static void cli_link_clear(void)
{
	srv.link.cli.addr = BT_MESH_ADDR_UNASSIGNED;
	srv.link.cli.net_idx = BT_MESH_KEY_UNUSED;
}

static void scan_status_send(struct bt_mesh_msg_ctx *ctx,
			     enum bt_mesh_rpr_status status)
{
	uint8_t timeout = 0;

	if (atomic_test_bit(srv.flags, SCANNING)) {
		timeout = k_ticks_to_ms_floor32(
			k_work_delayable_remaining_get(&srv.scan.timeout)) /
			MSEC_PER_SEC;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, RPR_OP_SCAN_STATUS, 4);
	bt_mesh_model_msg_init(&rsp, RPR_OP_SCAN_STATUS);
	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_u8(&rsp, srv.scan.state);
	net_buf_simple_add_u8(&rsp, srv.scan.max_devs);
	net_buf_simple_add_u8(&rsp, timeout);

	bt_mesh_model_send(srv.mod, ctx, &rsp, NULL, NULL);
}

static void link_status_send(struct bt_mesh_msg_ctx *ctx,
			     enum bt_mesh_rpr_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, RPR_OP_LINK_STATUS, 2);
	bt_mesh_model_msg_init(&buf, RPR_OP_LINK_STATUS);
	net_buf_simple_add_u8(&buf, status);
	net_buf_simple_add_u8(&buf, srv.link.state);

	bt_mesh_model_send(srv.mod, ctx, &buf, NULL, NULL);
}

static void link_report_send(void)
{
	struct bt_mesh_msg_ctx ctx = LINK_CTX(&srv.link.cli, true);

	BT_MESH_MODEL_BUF_DEFINE(buf, RPR_OP_LINK_REPORT, 3);
	bt_mesh_model_msg_init(&buf, RPR_OP_LINK_REPORT);
	net_buf_simple_add_u8(&buf, srv.link.status);
	net_buf_simple_add_u8(&buf, srv.link.state);
	if (srv.link.status == BT_MESH_RPR_ERR_LINK_CLOSED_BY_SERVER ||
	    srv.link.status == BT_MESH_RPR_ERR_LINK_CLOSED_BY_DEVICE) {
		net_buf_simple_add_u8(&buf, srv.link.close_reason);
	}

	LOG_DBG("%u %u", srv.link.status, srv.link.state);

	bt_mesh_model_send(srv.mod, &ctx, &buf, NULL, NULL);
}

static void scan_report_schedule(void)
{
	uint32_t delay = 0;

	if (k_work_delayable_remaining_get(&srv.scan.report) ||
	    atomic_test_bit(srv.flags, SCAN_REPORT_PENDING)) {
		return;
	}

	(void)bt_rand(&delay, sizeof(uint32_t));
	delay = (delay % 480) + 20;

	k_work_reschedule(&srv.scan.report, K_MSEC(delay));
}

static void scan_report_sent(int err, void *cb_data)
{
	atomic_clear_bit(srv.flags, SCAN_REPORT_PENDING);
	k_work_reschedule(&srv.scan.report, K_NO_WAIT);
}

static const struct bt_mesh_send_cb report_cb = {
	.end = scan_report_sent,
};

static void scan_report_send(void)
{
	struct bt_mesh_msg_ctx ctx = LINK_CTX(&srv.scan.cli, true);
	int i, err;

	if (atomic_test_bit(srv.flags, SCAN_REPORT_PENDING)) {
		return;
	}

	for (i = 0; i < srv.scan.max_devs; ++i) {
		struct bt_mesh_rpr_unprov *dev = &srv.scan.devs[i];

		if (!(dev->flags & BT_MESH_RPR_UNPROV_FOUND) ||
		    (dev->flags & BT_MESH_RPR_UNPROV_REPORTED)) {
			continue;
		}

		BT_MESH_MODEL_BUF_DEFINE(buf, RPR_OP_SCAN_REPORT, 23);
		bt_mesh_model_msg_init(&buf, RPR_OP_SCAN_REPORT);
		net_buf_simple_add_u8(&buf, dev->rssi);
		net_buf_simple_add_mem(&buf, dev->uuid, 16);
		net_buf_simple_add_le16(&buf, dev->oob);
		if (dev->flags & BT_MESH_RPR_UNPROV_HASH) {
			net_buf_simple_add_mem(&buf, &dev->hash, 4);
		}

		atomic_set_bit(srv.flags, SCAN_REPORT_PENDING);

		err = bt_mesh_model_send(srv.mod, &ctx, &buf, &report_cb, NULL);
		if (err) {
			atomic_clear_bit(srv.flags, SCAN_REPORT_PENDING);
			LOG_DBG("tx failed: %d", err);
			break;
		}

		LOG_DBG("Reported unprov #%u", i);
		dev->flags |= BT_MESH_RPR_UNPROV_REPORTED;
		break;
	}
}

static void scan_ext_report_send(void)
{
	struct bt_mesh_msg_ctx ctx = LINK_CTX(&srv.scan.cli, true);
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, RPR_OP_EXTENDED_SCAN_REPORT,
				 19 + CONFIG_BT_MESH_RPR_SRV_AD_DATA_MAX);
	bt_mesh_model_msg_init(&buf, RPR_OP_EXTENDED_SCAN_REPORT);
	net_buf_simple_add_u8(&buf, BT_MESH_RPR_SUCCESS);
	net_buf_simple_add_mem(&buf, srv.scan.dev->uuid, 16);

	if (srv.scan.dev->flags & BT_MESH_RPR_UNPROV_FOUND) {
		net_buf_simple_add_le16(&buf, srv.scan.dev->oob);
	} else {
		LOG_DBG("not found");
		goto send;
	}

	if (srv.scan.dev->flags & BT_MESH_RPR_UNPROV_EXT_ADV_RXD) {
		net_buf_simple_add_mem(&buf, srv.scan.adv_data->data,
				       srv.scan.adv_data->len);
		LOG_DBG("adv data: %s",
			bt_hex(srv.scan.adv_data->data, srv.scan.adv_data->len));
	}

	srv.scan.dev->flags &= ~BT_MESH_RPR_UNPROV_EXT_ADV_RXD;
send:
	err = bt_mesh_model_send(srv.mod, &ctx, &buf, NULL, NULL);
	if (!err) {
		srv.scan.dev->flags |= BT_MESH_RPR_UNPROV_REPORTED;
	}
}

static void scan_stop(void)
{
	LOG_DBG("");

	k_work_cancel_delayable(&srv.scan.report);
	k_work_cancel_delayable(&srv.scan.timeout);
	srv.scan.state = BT_MESH_RPR_SCAN_IDLE;
	cli_scan_clear();
	atomic_clear_bit(srv.flags, SCANNING);
}

static void scan_report_timeout(struct k_work *work)
{
	scan_report_send();
}

static void scan_ext_stop(uint32_t remaining_time)
{
	atomic_clear_bit(srv.flags, URI_MATCHED);
	atomic_clear_bit(srv.flags, URI_REQUESTED);

	if ((remaining_time + srv.scan.additional_time) &&
	    srv.scan.state != BT_MESH_RPR_SCAN_IDLE) {
		k_work_reschedule(
			&srv.scan.timeout,
			K_MSEC(remaining_time + srv.scan.additional_time));
	} else if (srv.scan.state == BT_MESH_RPR_SCAN_MULTI) {
		/* Extended scan might have finished early */
		scan_ext_report_send();
	} else if (srv.scan.state != BT_MESH_RPR_SCAN_IDLE) {
		scan_report_send();
		scan_stop();
	} else {
		atomic_clear_bit(srv.flags, SCANNING);
	}

	if (!(srv.scan.dev->flags & BT_MESH_RPR_UNPROV_REPORTED)) {
		scan_ext_report_send();
	}

	bt_mesh_scan_active_set(false);
	srv.scan.dev = NULL;
}

static void adv_handle_ext_scan(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *buf);

static void scan_timeout(struct k_work *work)
{
	LOG_DBG("%s", (srv.scan.dev ? "Extended scanning" : "Normal scanning"));

	if (srv.scan.dev) {
		scan_ext_stop(0);
	} else {
		scan_report_send();
		scan_stop();
	}
}

static void link_close(enum bt_mesh_rpr_status status,
		       enum prov_bearer_link_status reason)
{
	srv.link.status = status;
	srv.link.close_reason = reason;
	srv.link.state = BT_MESH_RPR_LINK_CLOSING;

	LOG_DBG("status: %u reason: %u", status, reason);

	if (atomic_test_and_clear_bit(srv.flags, NODE_REFRESH)) {
		/* Link closing is an atomic operation: */
		srv.link.state = BT_MESH_RPR_LINK_IDLE;
		link_report_send();
		srv.refresh.cb->link_closed(&pb_remote_srv, srv.refresh.cb_data,
					    srv.link.close_reason);

		cli_link_clear();
	} else {
		bt_mesh_pb_adv.link_close(reason);
	}
}

static void outbound_pdu_report_send(void)
{
	struct bt_mesh_msg_ctx ctx = LINK_CTX(&srv.link.cli, true);

	BT_MESH_MODEL_BUF_DEFINE(buf, RPR_OP_PDU_OUTBOUND_REPORT, 1);
	bt_mesh_model_msg_init(&buf, RPR_OP_PDU_OUTBOUND_REPORT);
	net_buf_simple_add_u8(&buf, srv.link.tx_pdu);

	LOG_DBG("%u", srv.link.tx_pdu);

	bt_mesh_model_send(srv.mod, &ctx, &buf, NULL, NULL);
}

static void pdu_send_complete(int err, void *cb_data)
{
	if (err) {
		link_close(BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_SEND_PDU,
			   PROV_BEARER_LINK_STATUS_FAIL);
	} else if (srv.link.state == BT_MESH_RPR_LINK_SENDING) {
		srv.link.state = BT_MESH_RPR_LINK_ACTIVE;
		srv.link.tx_pdu++;
		outbound_pdu_report_send();
	}
}

static int inbound_pdu_send(struct net_buf_simple *buf,
			    const struct bt_mesh_send_cb *cb)
{
	struct bt_mesh_msg_ctx ctx = LINK_CTX(&srv.link.cli, true);

	BT_MESH_MODEL_BUF_DEFINE(msg, RPR_OP_PDU_REPORT, 66);
	bt_mesh_model_msg_init(&msg, RPR_OP_PDU_REPORT);
	net_buf_simple_add_u8(&msg, srv.link.rx_pdu);
	net_buf_simple_add_mem(&msg, buf->data, buf->len);

	return bt_mesh_model_send(srv.mod, &ctx, &msg, cb, NULL);
}

static void subnet_evt_handler(struct bt_mesh_subnet *subnet,
			       enum bt_mesh_key_evt evt)
{
	if (!srv.mod || evt != BT_MESH_KEY_DELETED) {
		return;
	}

	LOG_DBG("Subnet deleted");

	if (srv.link.state != BT_MESH_RPR_LINK_IDLE &&
	    subnet->net_idx == srv.link.cli.net_idx) {
		link_close(BT_MESH_RPR_ERR_LINK_CLOSED_BY_SERVER,
			   PROV_BEARER_LINK_STATUS_FAIL);
		/* Skip the link closing stage, as specified in the Bluetooth
		 * MshPRTv1.1: 4.4.5.4.
		 */
		srv.link.state = BT_MESH_RPR_LINK_IDLE;
	} else if (atomic_test_bit(srv.flags, SCANNING) &&
		   subnet->net_idx == srv.scan.cli.net_idx) {
		scan_stop();
	}
}

BT_MESH_SUBNET_CB_DEFINE(rpr_srv) = {
	.evt_handler = subnet_evt_handler
};

/*******************************************************************************
 * Prov bearer interface
 ******************************************************************************/
static void pb_link_opened(const struct prov_bearer *bearer, void *cb_data)
{
	LOG_DBG("");

	srv.link.state = BT_MESH_RPR_LINK_ACTIVE;
	srv.link.status = BT_MESH_RPR_SUCCESS;
	link_report_send();
}

static void link_report_send_and_clear(struct k_work *work)
{
	link_report_send();
	cli_link_clear();
}

static void pb_link_closed(const struct prov_bearer *bearer, void *cb_data,
			   enum prov_bearer_link_status reason)
{
	if (srv.link.state == BT_MESH_RPR_LINK_IDLE) {
		return;
	}

	LOG_DBG("%u", reason);

	if (srv.link.state == BT_MESH_RPR_LINK_OPENING) {
		srv.link.status = BT_MESH_RPR_ERR_LINK_OPEN_FAILED;
	} else if (reason == PROV_BEARER_LINK_STATUS_TIMEOUT) {
		if (srv.link.state == BT_MESH_RPR_LINK_SENDING) {
			srv.link.status =
				BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_SEND_PDU;
		} else {
			srv.link.status = BT_MESH_RPR_ERR_LINK_CLOSED_BY_SERVER;
		}
	} else if (reason == PROV_BEARER_LINK_STATUS_FAIL &&
	   srv.link.status != BT_MESH_RPR_ERR_LINK_CLOSED_BY_CLIENT &&
	   srv.link.status != BT_MESH_RPR_ERR_LINK_CLOSED_BY_SERVER) {
		srv.link.status = BT_MESH_RPR_ERR_LINK_CLOSED_BY_DEVICE;
	}

	if (reason == PROV_BEARER_LINK_STATUS_SUCCESS) {
		srv.link.close_reason = PROV_BEARER_LINK_STATUS_SUCCESS;
	} else {
		srv.link.close_reason = PROV_BEARER_LINK_STATUS_FAIL;
	}

	srv.link.state = BT_MESH_RPR_LINK_IDLE;
	k_work_submit(&srv.link.report);
}

static void pb_error(const struct prov_bearer *bearer, void *cb_data,
		     uint8_t err)
{
	if (srv.link.state == BT_MESH_RPR_LINK_IDLE) {
		return;
	}

	LOG_DBG("%d", err);
	srv.link.close_reason = err;
	srv.link.state = BT_MESH_RPR_LINK_IDLE;
	srv.link.status = BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_RECEIVE_PDU;
	link_report_send();
	cli_link_clear();
}

static void pb_rx(const struct prov_bearer *bearer, void *cb_data,
		  struct net_buf_simple *buf)
{
	int err;

	if (srv.link.state != BT_MESH_RPR_LINK_ACTIVE &&
	    srv.link.state != BT_MESH_RPR_LINK_SENDING) {
		return;
	}

	srv.link.rx_pdu++;
	LOG_DBG("");

	err = inbound_pdu_send(buf, NULL);
	if (err) {
		LOG_ERR("PDU send fail: %d", err);
		link_close(BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_SEND_PDU,
			   PROV_BEARER_LINK_STATUS_FAIL);
		bt_mesh_pb_adv.link_close(PROV_ERR_RESOURCES);
	}
}

static const struct prov_bearer_cb prov_bearer_cb = {
	.link_opened = pb_link_opened,
	.link_closed = pb_link_closed,
	.error = pb_error,
	.recv = pb_rx,
};

/*******************************************************************************
 * Message handlers
 ******************************************************************************/

static int handle_scan_caps_get(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, RPR_OP_SCAN_CAPS_STATUS, 2);
	bt_mesh_model_msg_init(&rsp, RPR_OP_SCAN_CAPS_STATUS);
	net_buf_simple_add_u8(&rsp, CONFIG_BT_MESH_RPR_SRV_SCANNED_ITEMS_MAX);
	net_buf_simple_add_u8(&rsp, true);

	bt_mesh_model_send(srv.mod, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_scan_get(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	scan_status_send(ctx, BT_MESH_RPR_SUCCESS);

	return 0;
}

static int handle_scan_start(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_rpr_node cli = RPR_NODE(ctx);
	enum bt_mesh_rpr_status status;
	const uint8_t *uuid = NULL;
	uint8_t max_devs;
	uint8_t timeout;
	int i;

	max_devs = net_buf_simple_pull_u8(buf);
	timeout = net_buf_simple_pull_u8(buf);
	if (!timeout) {
		return -EINVAL;
	}

	if (buf->len == 16) {
		uuid = net_buf_simple_pull_mem(buf, 16);
	} else if (buf->len) {
		return -EINVAL;
	}

	LOG_DBG("max %u devs, %u s %s", max_devs, timeout,
		uuid ? bt_hex(uuid, 16) : "");

	if (max_devs > CONFIG_BT_MESH_RPR_SRV_SCANNED_ITEMS_MAX) {
		status = BT_MESH_RPR_ERR_SCANNING_CANNOT_START;
		goto rsp;
	}

	if (srv.scan.state != BT_MESH_RPR_SCAN_IDLE &&
	    !rpr_node_equal(&cli, &srv.scan.cli)) {
		status = BT_MESH_RPR_ERR_INVALID_STATE;
		goto rsp;
	}

	for (i = 0; i < ARRAY_SIZE(srv.scan.devs); ++i) {
		srv.scan.devs[i].flags = 0;
	}

	if (uuid) {
		srv.scan.state = BT_MESH_RPR_SCAN_SINGLE;

		srv.scan.devs[0].flags = BT_MESH_RPR_UNPROV_ACTIVE;
		memcpy(srv.scan.devs[0].uuid, uuid, 16);
	} else {
		srv.scan.state = BT_MESH_RPR_SCAN_MULTI;
	}

	srv.scan.max_devs =
		(max_devs ? max_devs :
			    CONFIG_BT_MESH_RPR_SRV_SCANNED_ITEMS_MAX);
	srv.scan.cli = cli;
	status = BT_MESH_RPR_SUCCESS;

	atomic_set_bit(srv.flags, SCANNING);
	k_work_reschedule(&srv.scan.timeout, K_SECONDS(timeout));

rsp:
	scan_status_send(ctx, status);

	return 0;
}

static int handle_extended_scan_start(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, RPR_OP_EXTENDED_SCAN_REPORT,
				 19 + CONFIG_BT_MESH_RPR_SRV_AD_DATA_MAX);
	struct bt_mesh_rpr_node cli = RPR_NODE(ctx);
	enum bt_mesh_rpr_status status;
	const uint8_t *uuid;
	uint8_t *ad = NULL;
	uint8_t ad_count;
	uint8_t timeout;
	int i;

	/* According to MshPRTv1.1: 4.4.5.5.1.7, scan reports shall be
	 * sent as segmented messages.
	 */
	ctx->send_rel = true;

	ad_count = net_buf_simple_pull_u8(buf);
	if (buf->len < ad_count || ad_count == 0 || ad_count > 0x10) {
		/* Prohibited */
		return -EINVAL;
	}

	ad = net_buf_simple_pull_mem(buf, ad_count);
	for (i = 0; i < ad_count; ++i) {
		if (ad[i] == BT_DATA_NAME_SHORTENED ||
		    ad[i] == BT_DATA_UUID16_SOME ||
		    ad[i] == BT_DATA_UUID32_SOME ||
		    ad[i] == BT_DATA_UUID128_SOME) {
			return -EINVAL;
		}

		for (int j = 0; j < i; j++) {
			if (ad[i] == ad[j]) {
				/* Duplicate entry */
				return -EINVAL;
			}
		}
	}

	ad_count = MIN(ad_count, CONFIG_BT_MESH_RPR_AD_TYPES_MAX);

	if (!buf->len) {
		const struct bt_mesh_prov *prov = bt_mesh_prov_get();

		LOG_DBG("Self scan");

		/* Want our local info. Could also include additional adv data,
		 * but there's no functionality for this in the mesh stack at
		 * the moment, so we'll only include the URI (if requested)
		 */
		bt_mesh_model_msg_init(&rsp, RPR_OP_EXTENDED_SCAN_REPORT);

		net_buf_simple_add_u8(&rsp, BT_MESH_RPR_SUCCESS);
		net_buf_simple_add_mem(&rsp, prov->uuid, 16);
		net_buf_simple_add_le16(&rsp, prov->oob_info);

		if (prov->uri && get_ad_type(ad, ad_count, BT_DATA_URI)) {
			uint8_t uri_len = strlen(prov->uri);

			if (uri_len < CONFIG_BT_MESH_RPR_SRV_AD_DATA_MAX - 2) {
				net_buf_simple_add_u8(&rsp, uri_len + 1);
				net_buf_simple_add_u8(&rsp, BT_DATA_URI);
				net_buf_simple_add_mem(&rsp, prov->uri,
						       uri_len);
				LOG_DBG("URI added: %s", prov->uri);
			} else {
				LOG_WRN("URI data won't fit in scan report");
			}
		}

		bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
		return 0;
	}

	if (buf->len != 17) {
		return -EINVAL;
	}

	uuid = net_buf_simple_pull_mem(buf, 16);
	timeout = net_buf_simple_pull_u8(buf);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_LOG_LEVEL_DBG)) {
		struct bt_uuid_128 uuid_repr = { .uuid = { BT_UUID_TYPE_128 } };

		memcpy(uuid_repr.val, uuid, 16);
		LOG_DBG("%s AD types: %s", bt_uuid_str(&uuid_repr.uuid),
			bt_hex(ad, ad_count));
	}

	if (timeout < BT_MESH_RPR_EXT_SCAN_TIME_MIN ||
	    timeout > BT_MESH_RPR_EXT_SCAN_TIME_MAX) {
		LOG_ERR("Invalid extended scan timeout %u", timeout);
		return -EINVAL;
	}

	if (srv.link.state != BT_MESH_RPR_LINK_IDLE) {
		status = BT_MESH_RPR_ERR_LIMITED_RESOURCES;
		goto rsp;
	}

	if (srv.scan.dev && (memcmp(srv.scan.dev->uuid, uuid, 16) ||
			!rpr_node_equal(&srv.scan.cli, &cli))) {
		LOG_WRN("Extended scan fail: Busy");
		status = BT_MESH_RPR_ERR_LIMITED_RESOURCES;
		goto rsp;
	}

	if (srv.scan.state == BT_MESH_RPR_SCAN_IDLE) {
		srv.scan.max_devs = 1;
		srv.scan.devs[0].flags = 0;
	}

	srv.scan.dev = unprov_get(uuid);
	if (!srv.scan.dev) {
		srv.scan.dev = unprov_get(NULL);
		if (!srv.scan.dev) {
			LOG_WRN("Extended scan fail: No memory");
			status = BT_MESH_RPR_ERR_LIMITED_RESOURCES;
			goto rsp;
		}

		memcpy(srv.scan.dev->uuid, uuid, 16);
		srv.scan.dev->oob = 0;
		srv.scan.dev->flags = 0;
	}

	memcpy(srv.scan.ad, ad, ad_count);
	srv.scan.ad_count = ad_count;
	net_buf_simple_reset(srv.scan.adv_data);

	atomic_set_bit(srv.flags, SCANNING);
	atomic_clear_bit(srv.flags, SCAN_EXT_HAS_ADDR);
	srv.scan.dev->flags &= ~BT_MESH_RPR_UNPROV_REPORTED;
	srv.scan.dev->flags |= BT_MESH_RPR_UNPROV_ACTIVE | BT_MESH_RPR_UNPROV_EXT;

	if (srv.scan.state == BT_MESH_RPR_SCAN_IDLE) {
		srv.scan.additional_time = 0;
		srv.scan.cli = cli;
	} else if (k_ticks_to_ms_floor32(
			k_work_delayable_remaining_get(&srv.scan.timeout)) <
			(timeout * MSEC_PER_SEC)) {
		srv.scan.additional_time = 0;
	} else {
		srv.scan.additional_time =
			k_ticks_to_ms_floor32(k_work_delayable_remaining_get(&srv.scan.timeout)) -
			(timeout * MSEC_PER_SEC);
	}

	bt_mesh_scan_active_set(true);
	k_work_reschedule(&srv.scan.timeout, K_SECONDS(timeout));
	return 0;

rsp:
	bt_mesh_model_msg_init(&rsp, RPR_OP_EXTENDED_SCAN_REPORT);
	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_mem(&rsp, uuid, 16);
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_scan_stop(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	if (atomic_test_bit(srv.flags, SCANNING)) {
		scan_report_send();
		scan_stop();
	}

	scan_status_send(ctx, BT_MESH_RPR_SUCCESS);

	return 0;
}

static int handle_link_get(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_DBG("");

	link_status_send(ctx, BT_MESH_RPR_SUCCESS);

	return 0;
}

static int handle_link_open(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	bool is_refresh_procedure = (buf->len == 1);
	struct bt_mesh_rpr_node cli = RPR_NODE(ctx);
	int8_t timeout = LINK_OPEN_TIMEOUT_DEFAULT;
	enum bt_mesh_rpr_status status;
	const uint8_t *uuid;
	uint8_t refresh;
	int err;

	if (buf->len != 1 && buf->len != 16 && buf->len != 17) {
		return -EINVAL;
	}

	if (srv.link.state == BT_MESH_RPR_LINK_CLOSING ||
	    srv.link.state == BT_MESH_RPR_LINK_SENDING) {
		status = BT_MESH_RPR_ERR_INVALID_STATE;
		LOG_ERR("Invalid state: %u", srv.link.state);
		goto rsp;
	}

	if (srv.link.state == BT_MESH_RPR_LINK_OPENING ||
	    srv.link.state == BT_MESH_RPR_LINK_ACTIVE) {

		if (!rpr_node_equal(&cli, &srv.link.cli)) {
			status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
			goto rsp;
		}

		if (is_refresh_procedure) {
			refresh = net_buf_simple_pull_u8(buf);
			if (!atomic_test_bit(srv.flags, NODE_REFRESH) ||
			    srv.refresh.procedure != refresh) {
				status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
			} else {
				status = BT_MESH_RPR_SUCCESS;
			}

			goto rsp;
		}

		if (atomic_test_bit(srv.flags, NODE_REFRESH)) {
			status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
			goto rsp;
		}

		uuid = net_buf_simple_pull_mem(buf, 16);

		if (memcmp(uuid, srv.link.dev->uuid, 16)) {
			status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
		} else {
			status = BT_MESH_RPR_SUCCESS;
		}

		goto rsp;
	}

	/* Link state is IDLE */

	if (is_refresh_procedure) {
		refresh = net_buf_simple_pull_u8(buf);
		if (refresh > BT_MESH_RPR_NODE_REFRESH_COMPOSITION) {
			LOG_ERR("Invalid refresh: %u", refresh);
			return -EINVAL;
		}

		if (refresh == BT_MESH_RPR_NODE_REFRESH_COMPOSITION &&
		    !atomic_test_bit(bt_mesh.flags, BT_MESH_COMP_DIRTY)) {
			LOG_WRN("Composition data page 128 is equal to page 0");
			status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
			goto rsp;
		}

		LOG_DBG("Node Refresh: %u", refresh);

		atomic_set_bit(srv.flags, NODE_REFRESH);
		srv.refresh.procedure = refresh;
		srv.link.cli = cli;
		srv.link.rx_pdu = 0;
		srv.link.tx_pdu = 0;
		srv.link.state = BT_MESH_RPR_LINK_ACTIVE;
		srv.link.status = BT_MESH_RPR_SUCCESS;
		srv.refresh.cb->link_opened(&pb_remote_srv, &srv);
		status = BT_MESH_RPR_SUCCESS;
		link_report_send();
		goto rsp;
	}

	uuid = net_buf_simple_pull_mem(buf, 16);
	if (buf->len) {
		timeout = net_buf_simple_pull_u8(buf);
		if (!timeout || timeout > 0x3c) {
			LOG_ERR("Invalid timeout: %u", timeout);
			return -EINVAL;
		}
	}

	LOG_DBG("0x%04x: %s", cli.addr, bt_hex(uuid, 16));

	/* Attempt to reuse the scanned unprovisioned device, to preserve as
	 * much information as possible, but fall back to hijacking the first
	 * slot if none was found.
	 */
	srv.link.dev = unprov_get(uuid);
	if (!srv.link.dev) {
		srv.link.dev = &srv.scan.devs[0];
		memcpy(srv.link.dev->uuid, uuid, 16);
		srv.link.dev->flags = 0;
	}

	err = bt_mesh_pb_adv.link_open(uuid, timeout, &prov_bearer_cb, &srv);
	if (err) {
		status = BT_MESH_RPR_ERR_LINK_CANNOT_OPEN;
		goto rsp;
	}

	srv.link.cli = cli;
	srv.link.rx_pdu = 0;
	srv.link.tx_pdu = 0;
	srv.link.state = BT_MESH_RPR_LINK_OPENING;
	srv.link.status = BT_MESH_RPR_SUCCESS;
	srv.link.dev->flags |= BT_MESH_RPR_UNPROV_HAS_LINK;
	status = BT_MESH_RPR_SUCCESS;

rsp:
	link_status_send(ctx, status);

	return 0;
}

static int handle_link_close(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_rpr_node cli = RPR_NODE(ctx);
	enum prov_bearer_link_status reason;

	reason = net_buf_simple_pull_u8(buf);
	if (reason != PROV_BEARER_LINK_STATUS_SUCCESS &&
	    reason != PROV_BEARER_LINK_STATUS_FAIL) {
		return -EINVAL;
	}

	LOG_DBG("");

	if (srv.link.state == BT_MESH_RPR_LINK_IDLE ||
	    srv.link.state == BT_MESH_RPR_LINK_CLOSING) {
		link_status_send(ctx, BT_MESH_RPR_SUCCESS);
		return 0;
	}

	if (!rpr_node_equal(&cli, &srv.link.cli)) {
		link_status_send(ctx, BT_MESH_RPR_ERR_INVALID_STATE);
		return 0;
	}

	srv.link.state = BT_MESH_RPR_LINK_CLOSING;

	/* Note: The response status isn't the same as the link status state,
	 * which will be used in the link report when the link is fully closed.
	 */

	/* Disable randomization for the Remote Provisioning Link Status message to avoid reordering
	 * of it with the Remote Provisioning Link Report message that shall be sent in a sequence
	 * when closing an active link (see section 4.4.5.5.3.3 of MshPRTv1.1).
	 */
	ctx->rnd_delay = false;

	link_status_send(ctx, BT_MESH_RPR_SUCCESS);
	link_close(BT_MESH_RPR_ERR_LINK_CLOSED_BY_CLIENT, reason);

	return 0;
}

static int handle_pdu_send(const struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_rpr_node cli = RPR_NODE(ctx);
	uint8_t pdu_num;
	int err;

	pdu_num = net_buf_simple_pull_u8(buf);

	if (srv.link.state != BT_MESH_RPR_LINK_ACTIVE) {
		LOG_WRN("Sending PDU while busy (state %u)", srv.link.state);
		return 0;
	}

	if (!rpr_node_equal(&cli, &srv.link.cli)) {
		LOG_WRN("Unknown client 0x%04x", cli.addr);
		return 0;
	}

	if (pdu_num != srv.link.tx_pdu + 1) {
		LOG_WRN("Invalid pdu number: %u, expected %u", pdu_num,
			srv.link.tx_pdu + 1);
		outbound_pdu_report_send();
		return 0;
	}

	LOG_DBG("0x%02x", buf->data[0]);

	if (atomic_test_bit(srv.flags, NODE_REFRESH)) {
		srv.link.tx_pdu++;
		outbound_pdu_report_send();
		srv.refresh.cb->recv(&pb_remote_srv, srv.refresh.cb_data, buf);
	} else {
		srv.link.state = BT_MESH_RPR_LINK_SENDING;
		err = bt_mesh_pb_adv.send(buf, pdu_send_complete, &srv);
		if (err) {
			link_close(
				BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_SEND_PDU,
				PROV_BEARER_LINK_STATUS_FAIL);
		}
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_rpr_srv_op[] = {
	{ RPR_OP_SCAN_CAPS_GET, BT_MESH_LEN_EXACT(0), handle_scan_caps_get },
	{ RPR_OP_SCAN_GET, BT_MESH_LEN_EXACT(0), handle_scan_get },
	{ RPR_OP_SCAN_START, BT_MESH_LEN_MIN(2), handle_scan_start },
	{ RPR_OP_EXTENDED_SCAN_START, BT_MESH_LEN_MIN(1), handle_extended_scan_start },
	{ RPR_OP_SCAN_STOP, BT_MESH_LEN_EXACT(0), handle_scan_stop },
	{ RPR_OP_LINK_GET, BT_MESH_LEN_EXACT(0), handle_link_get },
	{ RPR_OP_LINK_OPEN, BT_MESH_LEN_MIN(1), handle_link_open },
	{ RPR_OP_LINK_CLOSE, BT_MESH_LEN_EXACT(1), handle_link_close },
	{ RPR_OP_PDU_SEND, BT_MESH_LEN_MIN(1), handle_pdu_send },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_rpr_unprov *
adv_handle_beacon(const struct bt_le_scan_recv_info *info,
		  struct bt_data *ad)
{
	struct bt_uuid_128 uuid_repr = { .uuid = { BT_UUID_TYPE_128 } };
	struct bt_mesh_rpr_unprov *dev = NULL;
	const uint8_t *uuid;

	if (ad->data[0] != 0x00 || (ad->data_len != 19 && ad->data_len != 23)) {
		return NULL;
	}

	uuid = &ad->data[1];

	dev = unprov_get(uuid);
	if (!dev) {
		if (srv.scan.state != BT_MESH_RPR_SCAN_MULTI) {
			return NULL;
		}

		dev = unprov_get(NULL);
		if (!dev) {
			return NULL;
		}

		memcpy(dev->uuid, uuid, 16);
		dev->flags = BT_MESH_RPR_UNPROV_ACTIVE;
	} else if (dev->flags & BT_MESH_RPR_UNPROV_FOUND) {
		return dev;
	}

	dev->oob = sys_get_be16(&ad->data[17]);
	dev->rssi = info->rssi;

	if (ad->data_len == 23) {
		memcpy(&dev->hash, &ad->data[19], 4);
		dev->flags |= BT_MESH_RPR_UNPROV_HASH;
	}

	dev->flags |= BT_MESH_RPR_UNPROV_FOUND;
	memcpy(uuid_repr.val, uuid, 16);

	LOG_DBG("Unprov #%u: %s OOB: 0x%04x %s", dev - &srv.scan.devs[0],
		bt_uuid_str(&uuid_repr.uuid), dev->oob,
		(dev->flags & BT_MESH_RPR_UNPROV_HASH) ? bt_hex(&dev->hash, 4) :
							 "(no hash)");

	if (dev != srv.scan.dev && !(dev->flags & BT_MESH_RPR_UNPROV_REPORTED)) {
		scan_report_schedule();
	}

	return dev;
}

static bool pull_ad_data(struct net_buf_simple *buf, struct bt_data *ad)
{
	uint8_t len;

	if (!buf->len) {
		return false;
	}

	len = net_buf_simple_pull_u8(buf);
	if (!len || len > buf->len) {
		return false;
	}

	ad->type = net_buf_simple_pull_u8(buf);
	ad->data_len = len - sizeof(ad->type);
	ad->data = net_buf_simple_pull_mem(buf, ad->data_len);
	return true;
}

static void adv_handle_ext_scan(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *buf)
{
	struct bt_mesh_rpr_unprov *dev = NULL;
	struct net_buf_simple_state initial;
	struct bt_data ad;
	bool uri_match = false;
	bool uri_present = false;
	bool is_beacon = false;

	if (atomic_test_bit(srv.flags, SCAN_EXT_HAS_ADDR) &&
	    !bt_addr_le_cmp(&srv.scan.addr, info->addr)) {
		dev = srv.scan.dev;
	}

	/* Do AD data walk in two rounds: First to figure out which
	 * unprovisioned device this is (if any), and the second to copy out
	 * relevant AD data to the extended scan report.
	 */

	net_buf_simple_save(buf, &initial);
	while (pull_ad_data(buf, &ad)) {
		if (ad.type == BT_DATA_URI) {
			uri_present = true;
		}

		if (ad.type == BT_DATA_MESH_BEACON && !dev) {
			dev = adv_handle_beacon(info, &ad);
			is_beacon = true;
		} else if (ad.type == BT_DATA_URI &&
			   (srv.scan.dev->flags & BT_MESH_RPR_UNPROV_HASH)) {
			uint8_t hash[16];

			if (bt_mesh_s1(ad.data, ad.data_len, hash) ||
			    memcmp(hash, &srv.scan.dev->hash, 4)) {
				continue;
			}

			LOG_DBG("Found matching URI");
			uri_match = true;
			dev = srv.scan.dev;
			srv.scan.dev->flags |= BT_MESH_RPR_UNPROV_EXT_ADV_RXD;
		}
	}

	if (uri_match) {
		atomic_set_bit(srv.flags, URI_MATCHED);
	}

	if (!dev) {
		return;
	}

	/* Do not process advertisement if it was not identified by URI hash from beacon */
	if (!(dev->flags & BT_MESH_RPR_UNPROV_EXT_ADV_RXD)) {
		return;
	}

	srv.scan.addr = *info->addr;
	atomic_set_bit(srv.flags, SCAN_EXT_HAS_ADDR);

	if (IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)) {
		struct bt_uuid_128 uuid_repr = { .uuid = { BT_UUID_TYPE_128 } };

		memcpy(uuid_repr.val, dev->uuid, 16);
		LOG_DBG("Is %s", bt_uuid_str(&uuid_repr.uuid));
	}

	net_buf_simple_restore(buf, &initial);

	/* The ADTypeFilter field of the Remote Provisioning Extended Scan Start message
	 * contains only the URI AD Type, and the URI Hash is not available for the device
	 * with the Device UUID that was requested in the Remote Provisioning Extended Scan
	 * Start message.
	 */
	if (srv.scan.ad_count == 1 &&
	    get_ad_type(srv.scan.ad, 1, BT_DATA_URI) &&
	    !uri_match) {
		goto complete;
	}

	while (srv.scan.ad_count && pull_ad_data(buf, &ad)) {
		uint8_t *ad_entry;

		ad_entry = get_ad_type(srv.scan.ad, srv.scan.ad_count, ad.type);
		if (!ad_entry || (ad.type == BT_DATA_URI && !uri_match)) {
			continue;
		}

		LOG_DBG("AD type 0x%02x", ad.type);

		if (ad.type == BT_DATA_URI) {
			atomic_set_bit(srv.flags, URI_REQUESTED);
		}

		if (ad.data_len + 2 >
		    net_buf_simple_tailroom(srv.scan.adv_data)) {
			LOG_WRN("Can't fit AD 0x%02x in scan report", ad.type);
			continue;
		}

		net_buf_simple_add_u8(srv.scan.adv_data, ad.data_len + 1);
		net_buf_simple_add_u8(srv.scan.adv_data, ad.type);
		net_buf_simple_add_mem(srv.scan.adv_data, ad.data, ad.data_len);

		*ad_entry = srv.scan.ad[--srv.scan.ad_count];
	}

	/* The Remote Provisioning Server collects AD structures corresponding to all
	 * AD Types specified in the ADTypeFilter field of the Remote Provisioning Extended
	 * Scan Start message. The timeout specified in the Timeout field of the Remote
	 * Provisioning Extended Scan Start message expires.
	 * OR
	 * The ADTypeFilter field of the Remote Provisioning Extended Scan Start message
	 * contains only the URI AD Type, and the Remote Provisioning Server has received
	 * an advertising report or scan response with the URI corresponding to the URI Hash
	 * of the device with the Device UUID that was requested in the Remote Provisioning
	 * Extended Scan Start message.
	 */
	if (!srv.scan.ad_count) {
		goto complete;
	}

	/* The ADTypeFilter field of the Remote Provisioning Extended Scan Start message does
	 * not contain the URI AD Type, and the Remote Provisioning Server receives and processes
	 * the scan response data from the device with Device UUID requested in the Remote
	 * Provisioning Extended Scan Start message.
	 */
	if (!is_beacon && !uri_present &&
	    info->adv_type == BT_GAP_ADV_TYPE_SCAN_RSP) {
		goto complete;
	}

	/* The ADTypeFilter field of the Remote Provisioning Extended Scan Start message contains
	 * the URI AD Type and at least one different AD Type in the ADTypeFilter field, and the
	 * Remote Provisioning Server has received an advertising report or scan response with the
	 * URI corresponding to the URI Hash of the device with the Device UUID that was requested
	 * in the Remote Provisioning Extended Scan Start message, and the Remote Provisioning
	 * Server received the scan response from the same device.
	 * OR
	 * The ADTypeFilter field of the Remote Provisioning Extended Scan Start message contains
	 * the URI AD Type and at least one different AD Type in the ADTypeFilter field, and the
	 * URI Hash is not available for the device with the Device UUID that was requested in the
	 * Remote Provisioning Extended Scan Start message, and the Remote Provisioning Server
	 * received the scan response from the same device.
	 */
	if (atomic_get(srv.flags) & URI_REQUESTED &&
	    (atomic_get(srv.flags) & URI_MATCHED ||
	    (dev->flags & ~BT_MESH_RPR_UNPROV_HASH)) &&
	    info->adv_type == BT_GAP_ADV_TYPE_SCAN_RSP) {
		goto complete;
	}

	return;
complete:
	srv.scan.additional_time = 0;
	if (srv.scan.state != BT_MESH_RPR_SCAN_MULTI) {
		k_work_cancel_delayable(&srv.scan.timeout);
	}
	scan_ext_stop(0);
}

static void adv_handle_scan(const struct bt_le_scan_recv_info *info,
			    struct net_buf_simple *buf)
{
	struct bt_data ad;

	if (info->adv_type != BT_HCI_ADV_NONCONN_IND) {
		return;
	}

	while (pull_ad_data(buf, &ad)) {
		if (ad.type == BT_DATA_MESH_BEACON) {
			adv_handle_beacon(info, &ad);
			return;
		}
	}
}

static void scan_packet_recv(const struct bt_le_scan_recv_info *info,
			     struct net_buf_simple *buf)
{
	if (!atomic_test_bit(srv.flags, SCANNING)) {
		return;
	}

	if (srv.scan.dev) {
		adv_handle_ext_scan(info, buf);
	} else {
		adv_handle_scan(info, buf);
	}
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_packet_recv,
};

static int rpr_srv_init(const struct bt_mesh_model *mod)
{
	if (mod->rt->elem_idx || srv.mod) {
		LOG_ERR("Remote provisioning server must be initialized "
			"on first element");
		return -EINVAL;
	}

	srv.mod = mod;

	net_buf_simple_init(srv.scan.adv_data, 0);

	k_work_init_delayable(&srv.scan.timeout, scan_timeout);
	k_work_init_delayable(&srv.scan.report, scan_report_timeout);
	k_work_init(&srv.link.report, link_report_send_and_clear);
	bt_le_scan_cb_register(&scan_cb);
	mod->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	mod->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	return 0;
}

static void rpr_srv_reset(const struct bt_mesh_model *mod)
{
	cli_link_clear();
	cli_scan_clear();
	srv.scan.state = BT_MESH_RPR_SCAN_IDLE;
	srv.link.state = BT_MESH_RPR_LINK_IDLE;
	k_work_cancel_delayable(&srv.scan.timeout);
	k_work_cancel_delayable(&srv.scan.report);
	net_buf_simple_init(srv.scan.adv_data, 0);
	atomic_clear(srv.flags);
	srv.link.dev = NULL;
	srv.scan.dev = NULL;
}

const struct bt_mesh_model_cb _bt_mesh_rpr_srv_cb = {
	.init = rpr_srv_init,
	.reset = rpr_srv_reset,
};

static int node_refresh_link_accept(const struct prov_bearer_cb *cb,
				    void *cb_data)
{
	srv.refresh.cb = cb;
	srv.refresh.cb_data = cb_data;

	return 0;
}

static void node_refresh_tx_complete(int err, void *cb_data)
{
	if (err) {
		link_close(BT_MESH_RPR_ERR_LINK_CLOSED_AS_CANNOT_SEND_PDU,
			   PROV_BEARER_LINK_STATUS_FAIL);
		return;
	}

	if (srv.refresh.tx.cb) {
		srv.refresh.tx.cb(err, srv.refresh.tx.cb_data);
	}
}

static int node_refresh_buf_send(struct net_buf_simple *buf,
				 prov_bearer_send_complete_t cb, void *cb_data)
{
	static const struct bt_mesh_send_cb send_cb = {
		.end = node_refresh_tx_complete,
	};
	int err;

	if (!atomic_test_bit(srv.flags, NODE_REFRESH)) {
		return -EBUSY;
	}

	srv.refresh.tx.cb = cb;
	srv.refresh.tx.cb_data = cb_data;
	srv.link.rx_pdu++;

	LOG_DBG("%u", srv.link.rx_pdu);

	err = inbound_pdu_send(buf, &send_cb);
	if (err) {
		link_close(BT_MESH_RPR_ERR_LINK_CLOSED_BY_SERVER,
			   PROV_BEARER_LINK_STATUS_FAIL);
	}

	return err;
}

static void node_refresh_clear_tx(void)
{
	/* Nothing can be done */
}

const struct prov_bearer pb_remote_srv = {
	.type = BT_MESH_PROV_REMOTE,

	.link_accept = node_refresh_link_accept,
	.send = node_refresh_buf_send,
	.clear_tx = node_refresh_clear_tx,
};
