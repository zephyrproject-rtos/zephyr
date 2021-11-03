/* @file
 * @brief Bluetooth ASCS
 */
/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ASCS)
#define LOG_MODULE_NAME bt_ascs
#include "common/log.h"

#include "../hci_core.h"
#include "../conn_internal.h"

#include "endpoint.h"
#include "capabilities.h"
#include "pacs_internal.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)

#define ASE_ID(_ase) ase->ep.status.id
#define ASE_DIR(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_AUDIO_SOURCE : BT_AUDIO_SINK)
#define ASE_UUID(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_UUID_ASCS_ASE_SRC : BT_UUID_ASCS_ASE_SNK)
#define ASE_COUNT (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT)

struct bt_ascs_ase {
	struct bt_ascs *ascs;
	struct bt_audio_ep ep;
	struct k_work work;
};

struct bt_ascs {
	struct bt_conn *conn;
	uint8_t id;
	bt_addr_le_t peer;
	struct bt_ascs_ase ases[ASE_COUNT];
	struct bt_gatt_notify_params params;
	uint16_t handle;
};

static struct bt_ascs sessions[CONFIG_BT_MAX_CONN];

static const struct bt_audio_unicast_server_cb *server_cb;

int bt_audio_unicast_server_register_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		BT_DBG("cb is NULL");
		return -EINVAL;
	}

	if (server_cb != NULL) {
		BT_DBG("callback structure already registered");
		return -EALREADY;
	}

	server_cb = cb;

	return 0;
}

int bt_audio_unicast_server_unregister_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		BT_DBG("cb is NULL");
		return -EINVAL;
	}

	if (server_cb != cb) {
		BT_DBG("callback structure not registered");
		return -EINVAL;
	}

	server_cb = NULL;

	return 0;
}

static void ascs_ase_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, CONFIG_BT_L2CAP_TX_MTU);

static void ascs_cp_rsp_alloc(uint8_t op)
{
	struct bt_ascs_cp_rsp *rsp;

	rsp = net_buf_simple_add(&rsp_buf, sizeof(*rsp));
	rsp->op = op;
	rsp->num_ase = 0;
}

/* Add response to an opcode/ASE ID */
static void ascs_cp_rsp_add(uint8_t id, uint8_t op, uint8_t code,
			    uint8_t reason)
{
	struct bt_ascs_cp_rsp *rsp = (void *)rsp_buf.__buf;
	struct bt_ascs_cp_ase_rsp *ase_rsp;

	BT_DBG("id 0x%02x op %s (0x%02x) code %s (0x%02x) reason %s (0x%02x)",
	       id, bt_ascs_op_str(op), op, bt_ascs_rsp_str(code), code,
	       bt_ascs_reason_str(reason), reason);

	/* Allocate response if buffer is empty */
	if (!rsp_buf.len) {
		ascs_cp_rsp_alloc(op);
	}

	if (rsp->num_ase == 0xff) {
		return;
	}

	switch (code) {
	/* If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be
	 * set to 0xFF.
	 */
	case BT_ASCS_RSP_NOT_SUPPORTED:
	case BT_ASCS_RSP_TRUNCATED:
		rsp->num_ase = 0xff;
		break;
	default:
		rsp->num_ase++;
		break;
	}

	ase_rsp = net_buf_simple_add(&rsp_buf, sizeof(*ase_rsp));
	ase_rsp->id = id;
	ase_rsp->code = code;
	ase_rsp->reason = reason;
}

static void ascs_cp_rsp_add_errno(uint8_t id, uint8_t op, int err,
				  uint8_t reason)
{
	switch (err) {
	case -ENOBUFS:
	case -ENOMEM:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_NO_MEM,
				       BT_ASCS_REASON_NONE);
	case -EINVAL:
		switch (op) {
		case BT_ASCS_CONFIG_OP:
		/* Fallthrough */
		case BT_ASCS_QOS_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_CONF_INVALID,
					       reason);
		case BT_ASCS_ENABLE_OP:
		/* Fallthrough */
		case BT_ASCS_METADATA_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_METADATA_INVALID,
					       reason);
		default:
			return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_UNSPECIFIED,
					       BT_ASCS_REASON_NONE);
		}
	case -ENOTSUP:
		switch (op) {
		case BT_ASCS_CONFIG_OP:
		/* Fallthrough */
		case BT_ASCS_QOS_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_CONF_UNSUPPORTED,
					       reason);
		case BT_ASCS_ENABLE_OP:
		/* Fallthrough */
		case BT_ASCS_METADATA_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_METADATA_UNSUPPORTED,
					       reason);
		default:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_NOT_SUPPORTED,
					       BT_ASCS_REASON_NONE);
		}
	case -EBADMSG:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_INVALID_ASE_STATE,
					       BT_ASCS_REASON_NONE);
	default:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_UNSPECIFIED,
				       BT_ASCS_REASON_NONE);
	}
}

static void ascs_cp_rsp_success(uint8_t id, uint8_t op)
{
	ascs_cp_rsp_add(id, op, BT_ASCS_RSP_SUCCESS, BT_ASCS_REASON_NONE);
}

/* Notify response to control point */
static void ascs_cp_notify(struct bt_ascs *ascs)
{
	struct bt_gatt_attr attr;

	BT_DBG("ascs %p handle 0x%04x len %u", ascs, ascs->handle, rsp_buf.len);

	memset(&attr, 0, sizeof(attr));
	attr.handle = ascs->handle;
	attr.uuid = BT_UUID_ASCS_ASE_CP;

	bt_gatt_notify(ascs->conn, &attr, rsp_buf.data, rsp_buf.len);
}

static void ase_release(struct bt_ascs_ase *ase, bool cache)
{
	int err;

	BT_DBG("ase %p", ase);

	if (server_cb != NULL && server_cb->release != NULL) {
		err = server_cb->release(ase->ep.stream);
	} else {
		err = -EACCES;
	}

	if (err) {
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_RELEASE_OP, err,
				      BT_ASCS_REASON_NONE);
		return;
	}

	if (cache) {
		bt_audio_ep_set_state(&ase->ep,
				      BT_AUDIO_EP_STATE_CODEC_CONFIGURED);
	} else {
		bt_audio_ep_set_state(&ase->ep, BT_AUDIO_EP_STATE_RELEASING);
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_RELEASE_OP);
}

static void ascs_clear(struct bt_ascs *ascs)
{
	int i;

	BT_DBG("ascs %p", ascs);

	bt_addr_le_copy(&ascs->peer, BT_ADDR_LE_ANY);

	for (i = 0; i < ASE_COUNT; i++) {
		struct bt_ascs_ase *ase = &ascs->ases[i];

		if (ase->ep.status.state != BT_AUDIO_EP_STATE_IDLE) {
			ase_release(ase, false);
			bt_audio_ep_set_state(&ase->ep, BT_AUDIO_EP_STATE_IDLE);
		}
	}

	bt_conn_unref(ascs->conn);
	ascs->conn = NULL;
}

static void ase_disable(struct bt_ascs_ase *ase)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p", ase);

	ep = &ase->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_DISABLE_OP,
				      -EBADMSG, BT_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;

	if (server_cb != NULL && server_cb->release != NULL) {
		err = server_cb->disable(stream);
	} else {
		err = -EACCES;
	}

	if (err) {
		BT_ERR("Disable failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_DISABLE_OP,
				      err, BT_ASCS_REASON_NONE);
		return;
	}

	/* The ASE state machine goes into different states from this operation
	 * based on whether it is a source or a sink ASE.
	 */
	if (stream->cap->type == BT_AUDIO_SOURCE) {
		bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_DISABLING);
	} else {
		bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_DISABLE_OP);
}

static void ascs_dettach(struct bt_ascs *ascs)
{
	int i;

	BT_DBG("ascs %p conn %p", ascs, ascs->conn);

	/* Update address in case it has changed */
	ascs->id = ascs->conn->id;
	bt_addr_le_copy(&ascs->peer, &ascs->conn->le.dst);

	/* TODO: Store the ASES in the settings? */

	for (i = 0; i < ASE_COUNT; i++) {
		struct bt_ascs_ase *ase = &ascs->ases[i];

		if (ase->ep.status.state != BT_AUDIO_EP_STATE_IDLE) {
			/* Cache if disconnected with codec configured */
			ase_release(ase, true);
		}
	}

	bt_conn_unref(ascs->conn);
	ascs->conn = NULL;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;

	BT_DBG("");

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (sessions[i].conn != conn) {
			continue;
		}

		/* Release existing ASEs if not bonded */
		if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
			ascs_clear(&sessions[i]);
		} else {
			ascs_dettach(&sessions[i]);
		}
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = disconnected,
};

static uint8_t ascs_attr_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			    void *user_data)
{
	struct bt_ascs *ascs = user_data;

	ascs->handle = handle;

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_ascs *ascs_new(struct bt_conn *conn)
{
	static bool conn_cb_registered;
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!sessions[i].conn &&
		    !bt_addr_le_cmp(&sessions[i].peer, BT_ADDR_LE_ANY)) {
			struct bt_ascs *ascs = &sessions[i];

			memset(ascs->ases, 0, sizeof(ascs->ases));
			ascs->conn = bt_conn_ref(conn);

			/* Register callbacks if not registered */
			if (!conn_cb_registered) {
				bt_conn_cb_register(&conn_cb);
				conn_cb_registered = true;
			}

			if (!ascs->handle) {
				bt_gatt_foreach_attr_type(0x0001, 0xffff,
							  BT_UUID_ASCS_ASE_CP,
							  NULL, 1,
							  ascs_attr_cb, ascs);
			}

			return ascs;
		}
	}

	return NULL;
}

static void ase_stream_add(struct bt_ascs *ascs, struct bt_ascs_ase *ase,
			   struct bt_audio_stream *stream)
{
	BT_DBG("ase %p stream %p", ase, stream);
	ase->ep.stream = stream;
	stream->conn = ascs->conn;
	stream->ep = &ase->ep;
	stream->iso = &ase->ep.iso;
}

static void ascs_attach(struct bt_ascs *ascs, struct bt_conn *conn)
{
	int i;

	BT_DBG("ascs %p conn %p", ascs, conn);

	ascs->conn = bt_conn_ref(conn);

	/* TODO: Load the ASEs from the settings? */

	for (i = 0; i < ASE_COUNT; i++) {
		if (ascs->ases[i].ep.stream) {
			ase_stream_add(ascs, &ascs->ases[i],
				       ascs->ases[i].ep.stream);
		}
	}
}

static struct bt_ascs *ascs_find(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (sessions[i].conn == conn) {
			return &sessions[i];
		}

		/* Check if there is an existing session for the peer */
		if (!sessions[i].conn &&
		    bt_conn_is_peer_addr_le(conn, sessions[i].id,
					    &sessions[i].peer)) {
			ascs_attach(&sessions[i], conn);
			return &sessions[i];
		}
	}

	return NULL;
}

static struct bt_ascs *ascs_get(struct bt_conn *conn)
{
	struct bt_ascs *ascs;

	ascs = ascs_find(conn);
	if (ascs) {
		return ascs;
	}

	return ascs_new(conn);
}

NET_BUF_SIMPLE_DEFINE_STATIC(ase_buf, CONFIG_BT_L2CAP_TX_MTU);

static void ase_process(struct k_work *work)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(work, struct bt_ascs_ase, work);
	struct bt_gatt_attr attr;

	bt_audio_ep_get_status(&ase->ep, &ase_buf);

	memset(&attr, 0, sizeof(attr));
	attr.handle = ase->ep.handle;
	attr.uuid = ASE_UUID(ase->ep.status.id);

	bt_gatt_notify(ase->ascs->conn, &attr, ase_buf.data, ase_buf.len);

	if (ase->ep.status.state == BT_AUDIO_EP_STATE_RELEASING &&
	    ase->ep.stream == NULL) {
		bt_audio_ep_set_state(&ase->ep, BT_AUDIO_EP_STATE_IDLE);
	}

	if (ase->ep.status.state == BT_AUDIO_EP_STATE_IDLE) {
		return;
	}
}

static uint8_t ase_attr_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			   void *user_data)
{
	struct bt_ascs_ase *ase = user_data;

	ase->ep.handle = handle;

	return BT_GATT_ITER_CONTINUE;
}

static void ase_init(struct bt_ascs_ase *ase, uint8_t id)
{
	memset(ase, 0, sizeof(*ase));
	bt_audio_ep_init(&ase->ep, BT_AUDIO_EP_LOCAL, 0x0000, id);
	bt_gatt_foreach_attr_type(0x0001, 0xffff, ASE_UUID(id),
				  UINT_TO_POINTER(id), 1, ase_attr_cb, ase);
	k_work_init(&ase->work, ase_process);
}

static void ase_status_changed(struct bt_audio_ep *ep, uint8_t old_state,
			       uint8_t state)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);

	BT_DBG("ase %p conn %p", ase, ase->ascs->conn);

	if (!ase->ascs->conn || ase->ascs->conn->state != BT_CONN_CONNECTED) {
		return;
	}

	k_work_submit(&ase->work);
}

static struct bt_audio_ep_cb ase_cb = {
	.status_changed = ase_status_changed,
};

static struct bt_ascs_ase *ase_new(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;
	int i;

	if (id) {
		if (id > ASE_COUNT) {
			return NULL;
		}
		i = id;
		ase = &ascs->ases[i - 1];
		goto done;
	}

	for (i = 0; i < ASE_COUNT; i++) {
		ase = &ascs->ases[i];

		if (!ase->ep.status.id) {
			i++;
			goto done;
		}
	}

	return NULL;

done:
	ase_init(ase, i);
	ase->ascs = ascs;

	bt_audio_ep_register_cb(&ase->ep, &ase_cb);

	return ase;
}

static struct bt_ascs_ase *ase_find(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;

	if (!id || id > ASE_COUNT) {
		return NULL;
	}

	ase = &ascs->ases[id - 1];
	if (ase->ep.status.id == id) {
		return ase;
	}

	return NULL;
}

static struct bt_ascs_ase *ase_get(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;

	ase = ase_find(ascs, id);
	if (ase) {
		return ase;
	}

	return ase_new(ascs, id);
}

static ssize_t ascs_ase_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct bt_ascs *ascs;
	struct bt_ascs_ase *ase;

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	ascs = ascs_get(conn);
	if (!ascs) {
		BT_ERR("Unable to get ASCS session");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	ase = ase_get(ascs, POINTER_TO_UINT(attr->user_data));
	if (!ase) {
		BT_ERR("Unable to get ASE");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	bt_audio_ep_get_status(&ase->ep, &ase_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ase_buf.data,
				 ase_buf.len);
}

static void ascs_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static int ase_config(struct bt_ascs *ascs, struct bt_ascs_ase *ase,
		      const struct bt_ascs_config *cfg,
		      struct net_buf_simple *buf)
{
	sys_slist_t *lst;
	struct bt_audio_capability *cap;
	struct bt_audio_stream *stream;
	int err;

	BT_DBG("ase %p latency 0x%02x phy 0x%02x codec 0x%02x "
		"cid 0x%04x vid 0x%04x codec config len 0x%02x", ase,
		cfg->latency, cfg->phy, cfg->codec.id, cfg->codec.cid,
		cfg->codec.vid, cfg->cc_len);

	if (cfg->latency < 0x01 || cfg->latency > 0x03) {
		BT_ERR("Invalid latency: 0x%02x", cfg->latency);
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_CONF_INVALID,
				BT_ASCS_REASON_LATENCY);
		return 0;
	}

	if (cfg->phy < 0x01 || cfg->phy > 0x03) {
		BT_ERR("Invalid PHY: 0x%02x", cfg->phy);
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_CONF_INVALID, BT_ASCS_REASON_PHY);
		return 0;
	}

	switch (ase->ep.status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_AUDIO_EP_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ase->ep.status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_INVALID_ASE_STATE, 0x00);
		return 0;
	}

	/* Check if there are capabilities for the given direction */
	lst = bt_audio_capability_get(ASE_DIR(ase->ep.status.id));
	if (!lst) {
		goto not_found;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_codec codec;

		/* Skip if capabilities don't match */
		if (cfg->codec.id != cap->codec->id) {
			continue;
		}

		/* Store current codec configuration to be able to restore it
		 * in case of error.
		 */
		memcpy(&codec, &ase->ep.codec, sizeof(codec));

		if (bt_audio_ep_set_codec(&ase->ep, cfg->codec.id,
					  sys_le16_to_cpu(cfg->codec.cid),
					  sys_le16_to_cpu(cfg->codec.vid),
					  buf, cfg->cc_len, &ase->ep.codec)) {
			memcpy(&ase->ep.codec, &codec, sizeof(codec));
			ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
					BT_ASCS_RSP_CONF_INVALID,
					BT_ASCS_REASON_CODEC_DATA);
			return 0;
		}

		if (ase->ep.stream != NULL) {
			if (server_cb != NULL && server_cb->config != NULL) {
				err = server_cb->reconfig(ase->ep.stream, cap,
							  &ase->ep.codec);
			} else {
				err = -EACCES;
			}

			if (err) {
				uint8_t reason = BT_ASCS_REASON_CODEC_DATA;

				BT_ERR("Reconfig failed: %d", err);

				memcpy(&ase->ep.codec, &codec, sizeof(codec));
				ascs_cp_rsp_add_errno(ASE_ID(ase),
						      BT_ASCS_CONFIG_OP,
						      err, reason);
				return 0;
			}
			stream = ase->ep.stream;
		} else {
			stream = NULL;
			if (server_cb != NULL && server_cb->config != NULL) {
				err = server_cb->config(ascs->conn, &ase->ep,
							cap, &ase->ep.codec,
							&stream);
			} else {
				err = -EACCES;
			}

			if (err || stream == NULL) {
				BT_ERR("Config failed, err: %d, stream %p",
				       err, stream);

				memcpy(&ase->ep.codec, &codec, sizeof(codec));
				ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
						BT_ASCS_RSP_CONF_REJECTED,
						BT_ASCS_REASON_CODEC_DATA);

				return err;
			}
			ase_stream_add(ascs, ase, stream);
		}

		ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_CONFIG_OP);

		/* TODO: bt_audio_stream_attach duplicates some of the
		 * ase_stream_add. Should be cleaned up.
		 */
		bt_audio_stream_attach(ascs->conn, stream, &ase->ep, cap,
				       &ase->ep.codec);

		bt_audio_ep_set_state(&ase->ep,
				      BT_AUDIO_EP_STATE_CODEC_CONFIGURED);

		return 0;
	}

not_found:
	BT_ERR("Unable to find matching Capability");
	ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
			BT_ASCS_RSP_CAP_UNSUPPORTED, 0x00);
	return 0;
}

static ssize_t ascs_config(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_config_op *req;
	const struct bt_ascs_config *cfg;
	int i;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Malformed ASE Config");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases * sizeof(*cfg)) {
		BT_ERR("Malformed ASE Config: len %u < %zu", buf->len,
		       req->num_ases * sizeof(*cfg));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		int err;

		if (buf->len < sizeof(*cfg)) {
			BT_ERR("Malformed ASE Config: len %u < %zu", buf->len,
			       sizeof(*cfg));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

		if (buf->len < cfg->cc_len) {
			BT_ERR("Malformed ASE Codec Config len %u != %u",
				buf->len, cfg->cc_len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		BT_DBG("ase 0x%02x cc_len %u", cfg->ase, cfg->cc_len);

		if (cfg->ase) {
			ase = ase_get(ascs, cfg->ase);
		} else {
			ase = ase_new(ascs, 0);
		}

		if (!ase) {
			ascs_cp_rsp_add(cfg->ase, BT_ASCS_CONFIG_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		err = ase_config(ascs, ase, cfg, buf);
		if (err != 0) {
			BT_ERR("Malformed ASE Config");
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
	}

	return buf->size;
}

static int ase_stream_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	BT_DBG("stream %p qos %p", stream, qos);

	if (stream == NULL || stream->ep == NULL || qos == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	/* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_ERR("Invalid state: %s",
		bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (!bt_audio_valid_qos(qos)) {
		return -EINVAL;
	}

	if (!bt_audio_valid_stream_qos(stream, qos)) {
		return -EINVAL;
	}

	if (server_cb != NULL && server_cb->config != NULL) {
		int err;

		err = server_cb->qos(stream, qos);
		if (err != 0) {
			return err;
		}
	}

	stream->qos = qos;

	if (stream->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(stream->ep,
				      BT_AUDIO_EP_STATE_QOS_CONFIGURED);
		bt_audio_stream_iso_listen(stream);
	}

	return 0;
}

static void ase_qos(struct bt_ascs_ase *ase, const struct bt_ascs_qos *qos)
{
	struct bt_audio_stream *stream = ase->ep.stream;
	struct bt_codec_qos *cqos = &ase->ep.qos;
	int err;

	cqos->interval = sys_get_le24(qos->interval);
	cqos->framing = qos->framing;
	cqos->phy = qos->phy;
	cqos->sdu = sys_le16_to_cpu(qos->sdu);
	cqos->rtn = qos->rtn;
	cqos->latency = sys_le16_to_cpu(qos->latency);
	cqos->pd = sys_get_le24(qos->pd);

	BT_DBG("ase %p cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
	       "phy 0x%02x sdu %u rtn %u latency %u pd %u", ase, qos->cig,
	       qos->cis, cqos->interval, cqos->framing, cqos->phy, cqos->sdu,
	       cqos->rtn, cqos->latency, cqos->pd);

	err = ase_stream_qos(stream, cqos);
	if (err) {
		uint8_t reason = BT_ASCS_REASON_NONE;

		BT_ERR("QoS failed: err %d", err);

		if (err == -ENOTSUP) {
			if (cqos->interval == 0) {
				reason = BT_ASCS_REASON_INTERVAL;
			} else if (cqos->framing == 0xff) {
				reason = BT_ASCS_REASON_FRAMING;
			} else if (cqos->phy == 0) {
				reason = BT_ASCS_REASON_PHY;
			} else if (cqos->sdu == 0xffff) {
				reason = BT_ASCS_REASON_SDU;
			} else if (cqos->latency == 0) {
				reason = BT_ASCS_REASON_LATENCY;
			} else if (cqos->pd == 0) {
				reason = BT_ASCS_REASON_PD;
			}
		}

		memset(cqos, 0, sizeof(*cqos));

		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_QOS_OP,
				      err, reason);
		return;
	}

	ase->ep.cig_id = qos->cig;
	ase->ep.cis_id = qos->cis;

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_QOS_OP);
}

static ssize_t ascs_qos(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_qos_op *req;
	const struct bt_ascs_qos *qos;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases * sizeof(*qos)) {
		BT_ERR("Malformed ASE QoS: len %u < %zu", buf->len,
		       req->num_ases * sizeof(*qos));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

		BT_DBG("ase 0x%02x", qos->ase);

		ase = ase_find(ascs, qos->ase);
		if (!ase) {
			ascs_cp_rsp_add(qos->ase, BT_ASCS_QOS_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_qos(ase, qos);
	}

	return buf->size;
}

static int ase_metadata(struct bt_ascs_ase *ase, uint8_t op,
			struct bt_ascs_metadata *meta,
			struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	uint8_t state;
	int err;

	BT_DBG("ase %p meta->len %u", ase, meta->len);

	ep = &ase->ep;
	state = ep->status.state;

	switch (state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s", bt_audio_ep_state_str(state));
		err = -EBADMSG;
		ascs_cp_rsp_add_errno(ASE_ID(ase), op, EBADMSG,
				      buf->len ? *buf->data : 0x00);
		return err;
	}

	if (!meta->len) {
		goto done;
	}

	/* TODO: We should ask the upper layer for accept before we store it */
	err = bt_audio_ep_set_metadata(ep, buf, meta->len, &ep->codec);
	if (err) {
		if (err < 0) {
			ascs_cp_rsp_add_errno(ASE_ID(ase), op, err, 0x00);
		} else {
			ascs_cp_rsp_add(ASE_ID(ase), op,
					BT_ASCS_RSP_METADATA_INVALID, err);
		}
		return 0;
	}

	stream = ep->stream;
	if (server_cb != NULL && server_cb->metadata != NULL) {
		err = server_cb->metadata(stream, ep->codec.meta_count,
					  ep->codec.meta);
	} else {
		err = -EACCES;
	}

	if (err) {
		BT_ERR("Metadata failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), op, err,
				      buf->len ? *buf->data : 0x00);
		return -EFAULT;
	}

	/* Set the state to the same state to trigger the notifications */
	bt_audio_ep_set_state(ep, ep->status.state);
done:
	ascs_cp_rsp_success(ASE_ID(ase), op);

	return 0;
}

static int ase_enable(struct bt_ascs_ase *ase, struct bt_ascs_metadata *meta,
		      struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p buf->len %u", ase, buf->len);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		err = -EBADMSG;
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP, err,
				      BT_ASCS_REASON_NONE);
		return err;
	}

	err = bt_audio_ep_set_metadata(ep, buf, meta->len, &ep->codec);
	if (err) {
		if (err < 0) {
			ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP,
					      err, 0x00);
		} else {
			ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_ENABLE_OP,
					BT_ASCS_RSP_METADATA_INVALID, err);
		}
		return 0;
	}

	stream = ep->stream;
	if (server_cb != NULL && server_cb->enable != NULL) {
		err = server_cb->enable(stream, ep->codec.meta_count,
					ep->codec.meta);
	} else {
		err = -EACCES;
	}

	if (err) {
		BT_ERR("Enable rejected: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP, err,
				      BT_ASCS_REASON_NONE);
		return -EFAULT;
	}

	bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_ENABLING);


	if (ASE_DIR(ep->status.id) == BT_AUDIO_SINK) {
		/* SINK ASEs can autonomously go into the streaming state if
		 * the CIS is connected
		 */
		/* TODO: Verify that CIS is connected and set ep state to
		 * BT_AUDIO_EP_STATE_STREAMING
		 */
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_ENABLE_OP);

	return 0;
}

static ssize_t ascs_enable(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_enable_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases * sizeof(*meta)) {
		BT_ERR("Malformed ASE Metadata: len %u < %zu", buf->len,
		       req->num_ases * sizeof(*meta));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));

		BT_DBG("ase 0x%02x meta->len %u", meta->ase, meta->len);

		if (buf->len < meta->len) {
			BT_ERR("Malformed ASE Enable Metadata len %u != %u",
				buf->len, meta->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		ase = ase_find(ascs, meta->ase);
		if (!ase) {
			ascs_cp_rsp_add(meta->ase, BT_ASCS_ENABLE_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_enable(ase, meta, buf);
	}

	return buf->size;
}

static void ase_start(struct bt_ascs_ase *ase)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p", ase);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		err = -EBADMSG;
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_START_OP, err,
				      BT_ASCS_REASON_NONE);
		return;
	}

	/* If the ASE_ID  written by the client represents a Sink ASE, the
	 * server shall not accept the Receiver Start Ready operation for that
	 * ASE. The server shall send a notification of the ASE Control Point
	 * characteristic to the client, and the server shall set the
	 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
	 */
	if (ASE_DIR(ep->status.id) == BT_AUDIO_SINK) {
		BT_ERR("Start failed: invalid operation for Sink");
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_START_OP,
				BT_ASCS_RSP_INVALID_DIR, BT_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;
	if (server_cb != NULL && server_cb->start != NULL) {
		err = server_cb->start(stream);
	} else {
		err = -EACCES;
	}

	if (err) {
		BT_ERR("Start failed: %d", err);
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_START_OP, err,
				BT_ASCS_REASON_NONE);
		return;
	}

	bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_STREAMING);

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_START_OP);
}

static ssize_t ascs_start(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases) {
		BT_ERR("Malformed ASE Start: len %u < %u", buf->len,
		       req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_START_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_start(ase);
	}

	return buf->size;
}

static ssize_t ascs_disable(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_disable_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases) {
		BT_ERR("Malformed ASE Disable: len %u < %u", buf->len,
		       req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_DISABLE_OP,
					BT_ASCS_RSP_INVALID_ASE_STATE, 0);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_disable(ase);
	}

	return buf->size;
}

static void ase_stop(struct bt_ascs_ase *ase)
{
	int err;

	BT_DBG("ase %p", ase);

	/* If the ASE_ID  written by the client represents a Sink ASE, the
	 * server shall not accept the Receiver Start Ready operation for that
	 * ASE. The server shall send a notification of the ASE Control Point
	 * characteristic to the client, and the server shall set the
	 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
	 */
	if (ASE_DIR(ase->ep.status.id) == BT_AUDIO_SINK) {
		BT_ERR("Stop failed: invalid operation for Sink");
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_STOP_OP,
				BT_ASCS_RSP_INVALID_DIR, BT_ASCS_REASON_NONE);
		return;
	}

	err = bt_audio_stream_stop(ase->ep.stream);
	if (err) {
		BT_ERR("Stop failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_STOP_OP, err,
				      BT_ASCS_REASON_NONE);
		return;
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_STOP_OP);
}

static ssize_t ascs_stop(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases) {
		BT_ERR("Malformed ASE Start: len %u < %u", buf->len,
		       req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_STOP_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_stop(ase);
	}

	return buf->size;
}

static ssize_t ascs_metadata(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_metadata_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases * sizeof(*meta)) {
		BT_ERR("Malformed ASE Metadata: len %u < %zu", buf->len,
		       req->num_ases * sizeof(*meta));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));

		if (buf->len < meta->len) {
			BT_ERR("Malformed ASE Metadata: len %u < %u", buf->len,
			       meta->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		BT_DBG("ase 0x%02x meta->len %u", meta->ase, meta->len);

		ase = ase_find(ascs, meta->ase);
		if (!ase) {
			ascs_cp_rsp_add(meta->ase, BT_ASCS_METADATA_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_metadata(ase, BT_ASCS_METADATA_OP, meta, buf);
	}

	return buf->size;
}

static ssize_t ascs_release(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_release_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (buf->len < req->num_ases) {
		BT_ERR("Malformed ASE Release: len %u < %u", buf->len,
		       req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		uint8_t id;
		struct bt_ascs_ase *ase;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_RELEASE_OP,
					BT_ASCS_RSP_INVALID_ASE, 0);
			BT_ERR("Unable to find ASE");
			continue;
		}

		ase_release(ase, false);
	}

	return buf->size;
}

static ssize_t ascs_cp_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_ascs *ascs;
	const struct bt_ascs_ase_cp *req;
	struct net_buf_simple buf;
	ssize_t ret;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *) data, len);

	req = net_buf_simple_pull_mem(&buf, sizeof(*req));

	BT_DBG("conn %p attr %p buf %p len %u offset %u op %s (0x%02x)", conn,
	       attr, data, len, offset, bt_ascs_op_str(req->op), req->op);

	/* Reset/empty response buffer before using it again */
	net_buf_simple_reset(&rsp_buf);

	ascs = ascs_get(conn);
	if (!ascs) {
		BT_ERR("Unable to get ASCS session");
		len = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		ascs_cp_rsp_add(0, req->op, BT_ASCS_RSP_UNSPECIFIED, 0x00);
		goto respond;
	}

	switch (req->op) {
	case BT_ASCS_CONFIG_OP:
		ret = ascs_config(ascs, &buf);
		break;
	case BT_ASCS_QOS_OP:
		ret = ascs_qos(ascs, &buf);
		break;
	case BT_ASCS_ENABLE_OP:
		ret = ascs_enable(ascs, &buf);
		break;
	case BT_ASCS_START_OP:
		ret = ascs_start(ascs, &buf);
		break;
	case BT_ASCS_DISABLE_OP:
		ret = ascs_disable(ascs, &buf);
		break;
	case BT_ASCS_STOP_OP:
		ret = ascs_stop(ascs, &buf);
		break;
	case BT_ASCS_METADATA_OP:
		ret = ascs_metadata(ascs, &buf);
		break;
	case BT_ASCS_RELEASE_OP:
		ret = ascs_release(ascs, &buf);
		break;
	default:
		ascs_cp_rsp_add(0x00, req->op, BT_ASCS_RSP_NOT_SUPPORTED, 0);
		BT_DBG("Unknown opcode");
		len = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	if (ret == BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN)) {
		ascs_cp_rsp_add(0, req->op, BT_ASCS_RSP_TRUNCATED,
				BT_ASCS_REASON_NONE);
	}

respond:
	ascs_cp_notify(ascs);

	return len;
}

BT_GATT_SERVICE_DEFINE(ascs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ASCS),
#if CONFIG_BT_ASCS_ASE_SNK_COUNT > 0
	BT_GATT_CHARACTERISTIC(BT_UUID_ASCS_ASE_SNK,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       ascs_ase_read, NULL, UINT_TO_POINTER(1)),
	BT_GATT_CCC(ascs_ase_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif
#if CONFIG_BT_ASCS_ASE_SNK_COUNT > 1
	BT_GATT_CHARACTERISTIC(BT_UUID_ASCS_ASE_SNK,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       ascs_ase_read, NULL, UINT_TO_POINTER(2)),
	BT_GATT_CCC(ascs_ase_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif
#if CONFIG_BT_ASCS_ASE_SRC_COUNT > 0
	BT_GATT_CHARACTERISTIC(BT_UUID_ASCS_ASE_SRC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       ascs_ase_read, NULL,
			       UINT_TO_POINTER(CONFIG_BT_ASCS_ASE_SNK_COUNT + 1)),
	BT_GATT_CCC(ascs_ase_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif
#if CONFIG_BT_ASCS_ASE_SRC_COUNT > 1
	BT_GATT_CHARACTERISTIC(BT_UUID_ASCS_ASE_SRC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       ascs_ase_read, NULL,
			       UINT_TO_POINTER(CONFIG_BT_ASCS_ASE_SNK_COUNT + 2)),
	BT_GATT_CCC(ascs_ase_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif
	BT_GATT_CHARACTERISTIC(BT_UUID_ASCS_ASE_CP,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, ascs_cp_write, NULL),
	BT_GATT_CCC(ascs_cp_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)
);

#endif /* BT_AUDIO_UNICAST_SERVER */
