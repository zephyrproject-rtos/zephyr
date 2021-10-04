/* @file
 * @brief Bluetooth BAP
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>

#include "../hci_core.h"
#include "../conn_internal.h"

#include "endpoint.h"
#include "capabilities.h"
#include "pacs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_BAP)
#define LOG_MODULE_NAME bt_bap
#include "common/log.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

static struct bap_pac {
	struct bt_audio_capability cap;
	struct bt_codec            codec;
} cache[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_PAC_COUNT];

static const struct bt_uuid *snk_uuid = BT_UUID_PACS_SNK;
static const struct bt_uuid *src_uuid = BT_UUID_PACS_SRC;
static const struct bt_uuid *pacs_context_uuid = BT_UUID_PACS_SUPPORTED_CONTEXT;
static const struct bt_uuid *ase_snk_uuid = BT_UUID_ASCS_ASE_SNK;
static const struct bt_uuid *ase_src_uuid = BT_UUID_ASCS_ASE_SRC;
static const struct bt_uuid *cp_uuid = BT_UUID_ASCS_ASE_CP;

static struct bt_audio_chan *bap_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	sys_slist_t *lst;
	struct bt_audio_capability *lcap;
	struct bt_audio_chan *chan;
	uint8_t type = 0x00;

	BT_DBG("conn %p cap %p codec %p", conn, cap, codec);

	if (cap->type == BT_AUDIO_SINK) {
		type = BT_AUDIO_SOURCE;
	} else if (cap->type == BT_AUDIO_SOURCE) {
		type = BT_AUDIO_SINK;
	} else {
		BT_ERR("Invalid capability type: 0x%02x", type);
		return NULL;
	}

	/* Check if there are capabilities for the given direction */
	lst = bt_audio_capability_get(type);
	if (!lst) {
		BT_ERR("Unable to find matching capability type: 0x%02x", type);
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, lcap, node) {
		if (lcap->codec->id != cap->codec->id) {
			continue;
		}

		/* Configure channel using local capabilities */
		chan = bt_audio_chan_config(conn, ep, lcap, codec);
		if (chan) {
			if (!bt_audio_chan_reconfig(chan, cap, codec)) {
				ep->cap = lcap;
				return chan;
			}
			bt_audio_chan_release(chan, false);
		}
	}

	return NULL;
}

static int bap_reconfig(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap, struct bt_codec *codec)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_config_op *op;
	int err;

	BT_DBG("chan %p cap %p codec %p", chan, cap, codec);

	buf = bt_audio_ep_create_pdu(BT_ASCS_CONFIG_OP);

	op = net_buf_simple_add(buf, sizeof(*op));
	op->num_ases = 0x01;

	err = bt_audio_ep_config(ep, buf, cap, codec);
	if (err) {
		return err;
	}

	err = bt_audio_ep_send(chan->conn, ep, buf);
	if (err) {
		return err;
	}

	chan->cap = cap;
	chan->codec = codec;

	/* Terminate CIG if there is an existing QoS,
	 * so that we can create a new one
	 */
	if (chan->iso != NULL) {
		err = bt_audio_cig_terminate(chan);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int bap_enable(struct bt_audio_chan *chan,
		      uint8_t meta_count, struct bt_codec_data *meta)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	BT_DBG("chan %p", chan);

	buf = bt_audio_ep_create_pdu(BT_ASCS_ENABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = bt_audio_ep_enable(ep, buf, meta_count, meta);
	if (err) {
		return err;
	}

	return bt_audio_ep_send(chan->conn, ep, buf);
}

static int bap_metadata(struct bt_audio_chan *chan,
			uint8_t meta_count, struct bt_codec_data *meta)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	BT_DBG("chan %p", chan);

	buf = bt_audio_ep_create_pdu(BT_ASCS_METADATA_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = bt_audio_ep_metadata(ep, buf, meta_count, meta);
	if (err) {
		return err;
	}

	return bt_audio_ep_send(chan->conn, ep, buf);
}

static int bap_start(struct bt_audio_chan *chan)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	BT_DBG("chan %p", chan);

	buf = bt_audio_ep_create_pdu(BT_ASCS_START_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* If an ASE is in the Enabling state, and if the Unicast Client has
	 * not yet established a CIS for that ASE, the Unicast Client shall
	 * attempt to establish a CIS by using the Connected Isochronous Stream
	 * Central Establishment procedure.
	 */
	err = bt_audio_chan_connect(chan);
	if (!err) {
		return 0;
	}

	if (err && err != -EALREADY) {
		BT_ERR("bt_audio_chan_connect: %d", err);
		return err;
	}

	/* When initiated by the client, valid only if Direction field
	 * parameter value = 0x02 (Server is Audio Source)
	 */
	if (chan->cap->type == BT_AUDIO_SOURCE) {
		err = bt_audio_ep_start(ep, buf);
		if (err) {
			return err;
		}
		req->num_ases++;

		return bt_audio_ep_send(chan->conn, ep, buf);
	}

	return 0;
}

static int bap_disable(struct bt_audio_chan *chan)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err;

	BT_DBG("chan %p", chan);

	buf = bt_audio_ep_create_pdu(BT_ASCS_DISABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = bt_audio_ep_disable(ep, buf);
	if (err) {
		return err;
	}

	return bt_audio_ep_send(chan->conn, ep, buf);
}

static int bap_stop(struct bt_audio_chan *chan)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	BT_DBG("chan %p", chan);

	buf = bt_audio_ep_create_pdu(BT_ASCS_STOP_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* When initiated by the client, valid only if Direction field
	 * parameter value = 0x02 (Server is Audio Source)
	 */
	if (chan->cap->type == BT_AUDIO_SOURCE) {
		err = bt_audio_ep_stop(ep, buf);
		if (err) {
			return err;
		}
		req->num_ases++;

		return bt_audio_ep_send(chan->conn, ep, buf);
	}

	return 0;
}

static int bap_release(struct bt_audio_chan *chan)
{
	struct bt_audio_ep *ep = chan->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err, len;

	BT_DBG("chan %p", chan);

	if (!chan->conn || chan->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_audio_ep_create_pdu(BT_ASCS_RELEASE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;
	len = buf->len;

	/* Only attempt to release if not IDLE already */
	if (chan->ep->status.state == BT_AUDIO_EP_STATE_IDLE) {
		bt_audio_chan_reset(chan);
	} else {
		err = bt_audio_ep_release(ep, buf);
		if (err) {
			return err;
		}
	}

	/* Check if anything needs to be send */
	if (len == buf->len) {
		return 0;
	}

	return bt_audio_ep_send(chan->conn, ep, buf);
}

static struct bt_audio_capability_ops cap_ops = {
	.config		= bap_config,
	.reconfig	= bap_reconfig,
	.qos		= NULL,
	.enable		= bap_enable,
	.metadata	= bap_metadata,
	.start		= bap_start,
	.disable	= bap_disable,
	.stop		= bap_stop,
	.release	= bap_release,
};

static uint8_t cp_discover_func(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *discover)
{
	struct bt_audio_discover_params *params;
	struct bt_gatt_chrc *chrc;

	params = CONTAINER_OF(discover, struct bt_audio_discover_params,
			      discover);

	if (!attr) {
		if (params->err) {
			BT_ERR("Unable to find ASE Control Point");
		}
		params->func(conn, NULL, NULL, params);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	BT_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	params->err = 0;
	bt_audio_ep_set_cp(conn, chrc->value_handle);

	params->func(conn, NULL, NULL, params);

	return BT_GATT_ITER_STOP;
}

static int ase_cp_discover(struct bt_conn *conn,
			   struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;
	params->discover.uuid = cp_uuid;
	params->discover.func = cp_discover_func;
	params->discover.start_handle = 0x0001;
	params->discover.end_handle = 0xffff;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t ase_read_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_read_params *read,
			  const void *data, uint16_t length)
{
	struct bt_audio_discover_params *params;
	struct net_buf_simple buf;
	struct bt_audio_ep *ep;

	params = CONTAINER_OF(read, struct bt_audio_discover_params, read);

	BT_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err) {
		if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND && params->num_eps) {
			if (ase_cp_discover(conn, params) < 0) {
				BT_ERR("Unable to discover ASE Control Point");
				err = BT_ATT_ERR_UNLIKELY;
				goto fail;
			}
			return BT_GATT_ITER_STOP;
		}
		params->err = err;
		params->func(conn, NULL, NULL, params);
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		if (params->num_eps && ase_cp_discover(conn, params) < 0) {
			BT_ERR("Unable to discover ASE Control Point");
			err = BT_ATT_ERR_UNLIKELY;
			goto fail;
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		BT_ERR("Read response too small");
		goto fail;
	}

	ep = bt_audio_ep_get(conn, params->type, read->by_uuid.start_handle);
	if (!ep) {
		BT_WARN("No space left to parse ASE");
		if (params->num_eps) {
			if (ase_cp_discover(conn, params) < 0) {
				BT_ERR("Unable to discover ASE Control Point");
				err = BT_ATT_ERR_UNLIKELY;
				goto fail;
			}
			return BT_GATT_ITER_STOP;
		}
		goto fail;
	}

	bt_audio_ep_set_status(ep, &buf);
	bt_audio_ep_subscribe(conn, ep);

	params->func(conn, NULL, ep, params);

	params->num_eps++;

	return BT_GATT_ITER_CONTINUE;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static int ase_discover(struct bt_conn *conn,
			struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->read.func = ase_read_func;
	params->read.handle_count = 0u;

	if (params->type == BT_AUDIO_SINK) {
		params->read.by_uuid.uuid = ase_snk_uuid;
	} else if (params->type == BT_AUDIO_SOURCE) {
		params->read.by_uuid.uuid = ase_src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.by_uuid.start_handle = 0x0001;
	params->read.by_uuid.end_handle = 0xffff;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t pacs_context_read_func(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *read,
				      const void *data, uint16_t length)
{
	struct bt_audio_discover_params *params;
	struct net_buf_simple buf;
	struct bt_pacs_context *context;
	int i, index;

	params = CONTAINER_OF(read, struct bt_audio_discover_params, read);

	BT_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || length < sizeof(uint16_t) * 2) {
		goto discover_ase;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context = net_buf_simple_pull_mem(&buf, sizeof(*context));

	index = bt_conn_index(conn);

	for (i = 0; i < CONFIG_BT_BAP_PAC_COUNT; i++) {
		struct bap_pac *pac = &cache[index][i];

		if (!pac->cap.ops) {
			continue;
		}

		switch (pac->cap.type) {
		case BT_AUDIO_SINK:
			pac->cap.context = sys_le16_to_cpu(context->snk);
			break;
		case BT_AUDIO_SOURCE:
			pac->cap.context = sys_le16_to_cpu(context->src);
			break;
		default:
			BT_WARN("Cached pac with invalid type: %u",
				pac->cap.type);
		}

		BT_DBG("pac %p context 0x%04x", pac, pac->cap.context);
	}

discover_ase:
	/* Read ASE instances */
	if (ase_discover(conn, params) < 0) {
		BT_ERR("Unable to read ASE");
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static int pacs_context_discover(struct bt_conn *conn,
				 struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->read.func = pacs_context_read_func;
	params->read.handle_count = 0u;
	params->read.by_uuid.uuid = pacs_context_uuid;
	params->read.by_uuid.start_handle = 0x0001;
	params->read.by_uuid.end_handle = 0xffff;

	return bt_gatt_read(conn, &params->read);
}

static struct bap_pac *pac_alloc(struct bt_conn *conn, uint8_t type)
{
	int i, index;

	index = bt_conn_index(conn);

	for (i = 0; i < CONFIG_BT_BAP_PAC_COUNT; i++) {
		struct bap_pac *pac = &cache[index][i];

		if (!pac->cap.ops) {
			pac->cap.type = type;
			pac->cap.ops = &cap_ops;
			pac->cap.codec = &pac->codec;
			return pac;
		}
	}

	return NULL;
}

static uint8_t read_func(struct bt_conn *conn, uint8_t err,
			 struct bt_gatt_read_params *read,
			 const void *data, uint16_t length)
{
	struct bt_audio_discover_params *params;
	struct net_buf_simple buf;
	struct bt_pacs_read_rsp *rsp;

	params = CONTAINER_OF(read, struct bt_audio_discover_params, read);

	BT_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || !data) {
		params->err = err;
		goto fail;
	}

	BT_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		BT_ERR("Read response too small");
		goto fail;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	/* If no PAC was found don't bother discovering ASE and ASE CP */
	if (!rsp->num_pac) {
		goto fail;
	}

	while (rsp->num_pac) {
		struct bap_pac *bpac;
		struct bt_pac *pac;
		struct bt_pac_meta *meta;

		if (buf.len < sizeof(*pac)) {
			BT_ERR("Malformed PAC: remaining len %u expected %zu",
			       buf.len, sizeof(*pac));
			break;
		}

		pac = net_buf_simple_pull_mem(&buf, sizeof(*pac));

		BT_DBG("pac #%u: cc len %u", params->num_caps, pac->cc_len);

		if (buf.len < pac->cc_len) {
			BT_ERR("Malformed PAC: buf->len %u < %u pac->cc_len",
			       buf.len, pac->cc_len);
			break;
		}

		bpac = pac_alloc(conn, params->type);
		if (!bpac) {
			BT_WARN("No space left to parse PAC");
			break;
		}

		bpac->codec.id = pac->codec.id;
		bpac->codec.cid = sys_le16_to_cpu(pac->codec.cid);
		bpac->codec.vid = sys_le16_to_cpu(pac->codec.vid);

		if (bt_audio_ep_set_codec(NULL, pac->codec.id,
					  sys_le16_to_cpu(pac->codec.cid),
					  sys_le16_to_cpu(pac->codec.vid),
					  &buf, pac->cc_len, &bpac->codec)) {
			BT_ERR("Unable to parse Codec Capabilities");
			break;
		}

		meta = net_buf_simple_pull_mem(&buf, sizeof(*meta));

		if (buf.len < meta->len) {
			BT_ERR("Malformed PAC: remaining len %u expected %u",
			       buf.len, meta->len);
			break;
		}

		if (bt_audio_ep_set_metadata(NULL, &buf, meta->len,
					     &bpac->codec)) {
			BT_ERR("Unable to parse Codec Metadata");
			break;
		}

		BT_DBG("pac %p codec 0x%02x config count %u meta count %u ",
		       pac, bpac->codec.id, bpac->codec.data_count,
		       bpac->codec.meta_count);

		params->func(conn, &bpac->cap, NULL, params);

		rsp->num_pac--;
		params->num_caps++;
	}

	if (!params->num_caps) {
		goto fail;
	}

	/* Read PACS contexts */
	if (pacs_context_discover(conn, params) < 0) {
		BT_ERR("Unable to read PACS context");
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static void pac_reset(struct bt_conn *conn)
{
	int index = bt_conn_index(conn);
	int i;

	for (i = 0; i < CONFIG_BT_BAP_PAC_COUNT; i++) {
		struct bap_pac *pac = &cache[index][i];

		if (pac->cap.ops) {
			memset(pac, 0, sizeof(*pac));
		}
	}
}

static void bap_disconnected(struct bt_conn *conn, uint8_t reason)
{
	BT_DBG("conn %p reason 0x%02x", conn, reason);

	bt_audio_ep_reset(conn);
	pac_reset(conn);
}

static struct bt_conn_cb conn_cbs = {
	.disconnected = bap_disconnected,
};

int bt_audio_discover(struct bt_conn *conn,
		      struct bt_audio_discover_params *params)
{
	static bool conn_cb_registered;

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (params->type == BT_AUDIO_SINK) {
		params->read.by_uuid.uuid = snk_uuid;
	} else if (params->type == BT_AUDIO_SOURCE) {
		params->read.by_uuid.uuid = src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.func = read_func;
	params->read.handle_count = 0u;

	if (!params->read.by_uuid.start_handle) {
		params->read.by_uuid.start_handle = 0x0001;
	}

	if (!params->read.by_uuid.end_handle) {
		params->read.by_uuid.end_handle = 0xffff;
	}

	if (!conn_cb_registered) {
		bt_conn_cb_register(&conn_cbs);
		conn_cb_registered = true;
	}

	params->num_caps = 0u;
	params->num_eps = 0u;

	return bt_gatt_read(conn, &params->read);
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
