/*  Bluetooth Audio Endpoint */

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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_ENDPOINT)
#define LOG_MODULE_NAME bt_audio_ep
#include "common/log.h"

#include "endpoint.h"
#include "chan.h"

#if defined(CONFIG_BT_BAP)
#define CACHE_SIZE CONFIG_BT_BAP_ASE_COUNT
#else
#define CACHE_SIZE 0
#endif

#define EP_ISO(_iso) CONTAINER_OF(_iso, struct bt_audio_ep, iso)

static struct bt_audio_ep cache[CONFIG_BT_MAX_CONN][CACHE_SIZE];

#if defined(CONFIG_BT_BAP)
static struct bt_gatt_subscribe_params cp_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_discover_params cp_disc[CONFIG_BT_MAX_CONN];
#endif

static void ep_iso_recv(struct bt_iso_chan *chan, struct net_buf *buf)
{
	struct bt_audio_ep *ep = EP_ISO(chan);

	BT_DBG("chan %p ep %p len %zu", chan, ep, net_buf_frags_len(buf));

	if (ep->chan->ops->recv) {
		ep->chan->ops->recv(ep->chan, buf);
	}
}

static void ep_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_ep *ep = EP_ISO(chan);

	BT_DBG("chan %p ep %p", chan, ep);

	/* Only auto-start if local endpoint and sink role otherwise wait the
	 * sink to start.
	 */
	if (ep->type == BT_AUDIO_EP_LOCAL &&
	    ep->chan->cap->type == BT_AUDIO_SINK &&
	    ep->status.state == BT_ASCS_ASE_STATE_ENABLING) {
		bt_audio_chan_start(ep->chan);
	}
}

static void ep_iso_disconnected(struct bt_iso_chan *chan)
{
	struct bt_audio_ep *ep = EP_ISO(chan);

	BT_DBG("chan %p ep %p", chan, ep);

	if (ep->type != BT_AUDIO_EP_LOCAL) {
		return;
	}

	switch (ep->status.state) {
	case BT_ASCS_ASE_STATE_QOS:
		bt_audio_chan_bind(ep->chan, ep->chan->qos);
		break;
	case BT_ASCS_ASE_STATE_DISABLING:
		bt_audio_chan_stop(ep->chan);
		break;
	case BT_ASCS_ASE_STATE_ENABLING:
	case BT_ASCS_ASE_STATE_STREAMING:
		bt_audio_ep_set_state(ep, BT_ASCS_ASE_STATE_QOS);
		break;
	}
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= ep_iso_recv,
	.connected	= ep_iso_connected,
	.disconnected	= ep_iso_disconnected,
};

void bt_audio_ep_init(struct bt_audio_ep *ep, uint8_t type, uint16_t handle,
		      uint8_t id)
{
	BT_DBG("ep %p type 0x%02x handle 0x%04x id 0x%02x", ep, type, handle,
	       id);

	memset(ep, 0, sizeof(*ep));
	ep->type = type;
	ep->handle = handle;
	ep->status.id = id;
	ep->iso.ops = &iso_ops;
	ep->iso.qos = &ep->iso_qos;
}

struct bt_audio_ep *bt_audio_ep_find(struct bt_conn *conn, uint16_t handle)
{
	int i, index;

	index = bt_conn_index(conn);

	for (i = 0; i < CACHE_SIZE; i++) {
		struct bt_audio_ep *ep = &cache[index][i];

		if ((handle && ep->handle == handle) ||
		    (!handle && ep->handle)) {
			return ep;
		}
	}

	return NULL;
}

struct bt_audio_ep *bt_audio_ep_find_by_chan(struct bt_audio_chan *chan)
{
	int i, index;

	index = bt_conn_index(chan->conn);

	for (i = 0; i < CACHE_SIZE; i++) {
		struct bt_audio_ep *ep = &cache[index][i];

		if (ep->chan == chan) {
			return ep;
		}
	}

	return NULL;
}

struct bt_audio_ep *bt_audio_ep_new(struct bt_conn *conn, uint16_t handle)
{
	int i, index;

	index = bt_conn_index(conn);

	for (i = 0; i < CACHE_SIZE; i++) {
		struct bt_audio_ep *ep = &cache[index][i];

		if (!ep->handle) {
			bt_audio_ep_init(ep, BT_AUDIO_EP_REMOTE, handle, 0x00);
			return ep;
		}
	}

	return NULL;
}

struct bt_audio_ep *bt_audio_ep_get(struct bt_conn *conn, uint16_t handle)
{
	struct bt_audio_ep *ep;

	ep = bt_audio_ep_find(conn, handle);
	if (ep || !handle) {
		return ep;
	}

	return bt_audio_ep_new(conn, handle);
}

static void ep_idle(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->release) {
		ep->cap->ops->release(ep->chan);
	}

	bt_audio_chan_reset(ep->chan);
}

static void ep_config(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_audio_qos *qos;

	if (buf->len < sizeof(*cfg)) {
		BT_ERR("Config status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

	if (!ep->chan->cap || ep->chan->cap->type != cfg->dir) {
		BT_ERR("Capabilities type mismatched");
		/* TODO: Release the channel? */
		return;
	}

	if (!ep->chan->codec || ep->chan->codec->id != cfg->codec.id) {
		BT_ERR("Codec configuration mismatched");
		/* TODO: Release the channel? */
		return;
	}

	if (buf->len < cfg->cc_len) {
		BT_ERR("Malformed ASE Config status: buf->len %u < %u cc_len",
		       buf->len, cfg->cc_len);
		return;
	}

	qos = &ep->chan->cap->qos;

	/* Convert to interval representation so they can be matched by QoS */
	qos->framing = cfg->framing;
	qos->phy = cfg->phy;
	qos->rtn = cfg->rtn;
	qos->latency = sys_le16_to_cpu(cfg->latency);
	qos->pd_min = sys_get_le24(cfg->pd_min);
	qos->pd_max = sys_get_le24(cfg->pd_max);

	BT_DBG("dir 0x%02x framing 0x%02x phy 0x%02x rtn %u latency %u "
	       "pd_min %u pd_max %u codec 0x%02x ", ep->chan->cap->type,
	       qos->framing, qos->phy, qos->rtn, qos->latency, qos->pd_min,
	       qos->pd_max, ep->chan->codec->id);

	bt_audio_ep_set_codec(ep, cfg->codec.id,
			      sys_le16_to_cpu(cfg->codec.cid),
			      sys_le16_to_cpu(cfg->codec.vid),
			      buf, cfg->cc_len, NULL);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->reconfig) {
		ep->cap->ops->reconfig(ep->chan, ep->cap, ep->chan->codec);
	}
}

static void ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;

	if (buf->len < sizeof(*qos)) {
		BT_ERR("QoS status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

	memcpy(&ep->chan->qos->interval, sys_le24_to_cpu(qos->interval),
	       sizeof(qos->interval));
	ep->chan->qos->framing = qos->framing;
	ep->chan->qos->phy = qos->phy;
	ep->chan->qos->sdu = sys_le16_to_cpu(qos->sdu);
	ep->chan->qos->rtn = qos->rtn;
	ep->chan->qos->latency = sys_le16_to_cpu(qos->latency);
	memcpy(&ep->chan->qos->pd, sys_le24_to_cpu(qos->pd), sizeof(qos->pd));

	BT_DBG("dir 0x%02x codec 0x%02x interval %u framing 0x%02x phy 0x%02x "
	       "rtn %u latency %u pd %u", ep->chan->cap->type,
	       ep->chan->codec->id, ep->chan->qos->interval,
	       ep->chan->qos->framing, ep->chan->qos->phy, ep->chan->qos->rtn,
	       ep->chan->qos->latency, ep->chan->qos->pd);

	/* Disconnect ISO if connected */
	if (ep->chan->iso->state == BT_ISO_CONNECTED) {
		bt_audio_chan_disconnect(ep->chan);
	}

	bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_CONFIGURED);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->qos) {
		ep->cap->ops->qos(ep->chan, ep->chan->qos);
	}
}

static void ep_enabling(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	if (buf->len < sizeof(*enable)) {
		BT_ERR("Enabling status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x", ep->chan->cap->type,
	       ep->cig, ep->cis);

	bt_audio_ep_set_metadata(ep, buf, enable->metadata_len, NULL);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->enable) {
		ep->cap->ops->enable(ep->chan, ep->codec.meta_count,
				     ep->codec.meta);
	}

	bt_audio_chan_start(ep->chan);
}

static void ep_streaming(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	if (buf->len < sizeof(*enable)) {
		BT_ERR("Streaming status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x", ep->chan->cap->type,
	       ep->cig, ep->cis);

	bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_STREAMING);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->start) {
		ep->cap->ops->start(ep->chan);
	}
}

static void ep_disabling(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	if (buf->len < sizeof(*enable)) {
		BT_ERR("Disabling status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x", ep->chan->cap->type,
	       ep->cig, ep->cis);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->disable) {
		ep->cap->ops->disable(ep->chan);
	}

	bt_audio_chan_stop(ep->chan);
}

static void ep_releasing(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	BT_DBG("dir 0x%02x", ep->chan->cap->type);

	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->stop) {
		ep->cap->ops->stop(ep->chan);
	}

	/* The Unicast Client shall terminate any CIS established for that ASE
	 * by following the Connected Isochronous Stream Terminate procedure
	 * defined in Volume 3, Part C, Section 9.3.15 in when the Unicast
	 * Client has determined that the ASE is in the Releasing state.
	 */
	bt_audio_chan_disconnect(ep->chan);
}

void bt_audio_ep_set_status(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status *status;

	if (!ep) {
		return;
	}

	status = net_buf_simple_pull_mem(buf, sizeof(*status));

	BT_DBG("ep %p handle 0x%04x id 0x%02x state %s", ep, ep->handle,
	       status->id, bt_audio_ep_state_str(status->state));

	/* TODO: Warn invalid status transitions? */
	ep->status = *status;

	switch (status->state) {
	case BT_ASCS_ASE_STATE_IDLE:
		ep_idle(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_CONFIG:
		ep_config(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_QOS:
		ep_qos(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_ENABLING:
		ep_enabling(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_STREAMING:
		ep_streaming(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_DISABLING:
		ep_disabling(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_RELEASING:
		ep_releasing(ep, buf);
		break;
	}
}

static void codec_data_add(struct net_buf_simple *buf, const char *prefix,
			   uint8_t num, struct bt_codec_data *data)
{
	struct bt_ascs_codec_config *cc;
	int i;

	for (i = 0; i < num; i++) {
		struct bt_data *d = &data[i].data;

		BT_DBG("#%u: %s type 0x%02x len %u", i, prefix, d->type,
		       d->data_len);
		BT_HEXDUMP_DBG(d->data, d->data_len, prefix);

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);
	}
}

static void ep_get_status_config(struct bt_audio_ep *ep,
				 struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_audio_qos *qos = &ep->chan->cap->qos;

	cfg = net_buf_simple_add(buf, sizeof(*cfg));
	cfg->dir = ep->chan->cap->type;
	cfg->framing = qos->framing;
	cfg->phy = qos->phy;
	cfg->rtn = qos->rtn;
	cfg->latency = sys_cpu_to_le16(qos->latency);
	sys_put_le24(qos->pd_min, cfg->pd_min);
	sys_put_le24(qos->pd_max, cfg->pd_max);
	cfg->codec.id = ep->codec.id;
	cfg->codec.cid = sys_cpu_to_le16(ep->codec.cid);
	cfg->codec.vid = sys_cpu_to_le16(ep->codec.vid);

	BT_DBG("dir 0x%02x framing 0x%02x phy 0x%02x rtn %u latency %u "
	       "pd_min %u pd_max %u codec 0x%02x ", ep->chan->cap->type,
	       qos->framing, qos->phy, qos->rtn, qos->latency, qos->pd_min,
	       qos->pd_max, ep->chan->codec->id);

	cfg->cc_len = buf->len;
	codec_data_add(buf, "data", ep->codec.data_count, ep->codec.data);
	cfg->cc_len = buf->len - cfg->cc_len;
}

static void ep_get_status_qos(struct bt_audio_ep *ep,
			      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;

	qos = net_buf_simple_add(buf, sizeof(*qos));
	qos->cig_id = ep->cig;
	qos->cis_id = ep->cis;
	sys_put_le24(ep->chan->qos->interval, qos->interval);
	qos->framing = ep->chan->qos->framing;
	qos->phy = ep->chan->qos->phy;
	qos->sdu = sys_cpu_to_le16(ep->chan->qos->sdu);
	qos->rtn = ep->chan->qos->rtn;
	qos->latency = sys_cpu_to_le16(ep->chan->qos->latency);
	sys_put_le24(ep->chan->qos->pd, qos->pd);

	BT_DBG("dir 0x%02x codec 0x%02x interval %u framing 0x%02x phy 0x%02x "
	       "rtn %u latency %u pd %u", ep->chan->cap->type,
	       ep->chan->codec->id, ep->chan->qos->interval,
	       ep->chan->qos->framing, ep->chan->qos->phy, ep->chan->qos->rtn,
	       ep->chan->qos->latency, ep->chan->qos->pd);
}

static void ep_get_status_enable(struct bt_audio_ep *ep,
				 struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	enable = net_buf_simple_add(buf, sizeof(*enable));
	enable->cig_id = ep->cig;
	enable->cis_id = ep->cis;

	enable->metadata_len = buf->len;
	codec_data_add(buf, "meta", ep->codec.meta_count, ep->codec.meta);
	enable->metadata_len = buf->len - enable->metadata_len;

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x", ep->chan->cap->type,
	       ep->cig, ep->cis);
}

int bt_audio_ep_get_status(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status *status;

	if (!ep || !buf) {
		return -EINVAL;
	}

	BT_DBG("ep %p id 0x%02x state %s", ep, ep->status.id,
		bt_audio_ep_state_str(ep->status.state));

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	status = net_buf_simple_add_mem(buf, &ep->status,
					sizeof(ep->status));

	switch (ep->status.state) {
	case BT_ASCS_ASE_STATE_IDLE:
	/* Fallthrough */
	case BT_ASCS_ASE_STATE_RELEASING:
		break;
	case BT_ASCS_ASE_STATE_CONFIG:
		ep_get_status_config(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_QOS:
		ep_get_status_qos(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_ENABLING:
	/* Fallthrough */
	case BT_ASCS_ASE_STATE_STREAMING:
	/* Fallthrough */
	case BT_ASCS_ASE_STATE_DISABLING:
		ep_get_status_enable(ep, buf);
		break;
	default:
		BT_ERR("Invalid Endpoint state");
		break;
	}

	return 0;
}

static bool codec_config_store(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *cdata;

	if (codec->data_count >= ARRAY_SIZE(codec->data)) {
		BT_ERR("No slot available for Codec Config");
		return false;
	}

	cdata = &codec->data[codec->data_count];

	if (data->data_len > sizeof(cdata->value)) {
		BT_ERR("Not enough space for Codec Config: %u > %zu",
		       data->data_len, sizeof(cdata->value));
		return false;
	}

	BT_DBG("#%u type 0x%02x len %u", codec->data_count, data->type,
	       data->data_len);

	cdata->data.type = data->type;
	cdata->data.data_len = data->data_len;

	/* Deep copy data contents */
	cdata->data.data = cdata->value;
	memcpy(cdata->value, data->data, data->data_len);

	BT_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec->data_count++;

	return true;
}

int bt_audio_ep_set_codec(struct bt_audio_ep *ep, uint8_t id, uint16_t cid,
			  uint16_t vid, struct net_buf_simple *buf,
			  uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple ad;

	if (!ep && !codec) {
		return -EINVAL;
	}

	BT_DBG("ep %p codec id 0x%02x cid 0x%04x vid 0x%04x len %u", ep, id,
	       cid, vid, len);

	if (!codec) {
		codec = &ep->codec;
	}

	codec->id = id;
	codec->cid = cid;
	codec->vid = vid;
	codec->data_count = 0;

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&ad, net_buf_simple_pull_mem(buf, len),
				      len);

	/* Parse LTV entries */
	bt_data_parse(&ad, codec_config_store, codec);

	/* Check if all entries could be parsed */
	if (ad.len) {
		BT_ERR("Unable to parse Codec Config: len %u", ad.len);
		goto fail;
	}

	return 0;

fail:
	memset(codec, 0, sizeof(*codec));
	return -EINVAL;
}

static bool codec_metadata_store(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *meta;

	if (codec->meta_count >= ARRAY_SIZE(codec->meta)) {
		BT_ERR("No slot available for Codec Config Metadata");
		return false;
	}

	meta = &codec->meta[codec->meta_count];

	if (data->data_len > sizeof(meta->value)) {
		BT_ERR("Not enough space for Codec Config Metadata: %u > %zu",
		       data->data_len, sizeof(meta->value));
		return false;
	}

	BT_DBG("#%u type 0x%02x len %u", codec->meta_count, data->type,
	       data->data_len);

	meta->data.type = data->type;
	meta->data.data_len = data->data_len;

	/* Deep copy data contents */
	meta->data.data = meta->value;
	memcpy(meta->value, data->data, data->data_len);

	BT_HEXDUMP_DBG(meta->value, data->data_len, "data");

	codec->meta_count++;

	return true;
}

int bt_audio_ep_set_metadata(struct bt_audio_ep *ep, struct net_buf_simple *buf,
			     uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple meta;

	if (!ep && !codec) {
		return -EINVAL;
	}

	BT_DBG("ep %p len %u codec %p", ep, len, codec);

	if (!len) {
		return 0;
	}

	if (!codec) {
		codec = &ep->codec;
	}

	/* Reset current metadata */
	codec->meta_count = 0;
	memset(codec->meta, 0, sizeof(codec->meta));

	net_buf_simple_init_with_data(&meta, net_buf_simple_pull_mem(buf, len),
				      len);

	/* Parse LTV entries */
	bt_data_parse(&meta, codec_metadata_store, codec);

	/* Check if all entries could be parsed */
	if (meta.len) {
		BT_ERR("Unable to parse Metadata: len %u",
		       meta.len);
		goto fail;
	}

	return 0;

fail:
	codec->meta_count = 0;
	memset(codec->meta, 0, sizeof(codec->meta));
	return -EINVAL;
}

void bt_audio_ep_register_cb(struct bt_audio_ep *ep, struct bt_audio_ep_cb *cb)
{
	if (!ep) {
		return;
	}

	sys_slist_append(&ep->cbs, &cb->node);
}

void bt_audio_ep_set_state(struct bt_audio_ep *ep, uint8_t state)
{
	struct bt_audio_ep_cb *cb, *tmp;
	uint8_t old_state;

	if (!ep) {
		return;
	}

	BT_DBG("ep %p id 0x%02x %s -> %s", ep, ep->status.id,
	       bt_audio_ep_state_str(ep->status.state),
	       bt_audio_ep_state_str(state));

	old_state = ep->status.state;
	ep->status.state = state;

	/* Notify callbacks */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ep->cbs, cb, tmp, node) {
		cb->status_changed(ep, old_state, state);
	}

	if (!ep->chan || old_state == state) {
		return;
	}

	switch (state) {
	case BT_ASCS_ASE_STATE_IDLE:
		bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_IDLE);
		break;
	case BT_ASCS_ASE_STATE_QOS:
		bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_CONFIGURED);
		break;
	case BT_ASCS_ASE_STATE_STREAMING:
		bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_STREAMING);
		break;
	}
}

#if defined(CONFIG_BT_BAP)
static uint8_t cp_notify(struct bt_conn *conn,
			 struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	struct bt_ascs_cp_rsp *rsp;
	int i;

	struct net_buf_simple buf;

	BT_DBG("conn %p len %u", conn, length);

	if (!data) {
		BT_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		BT_ERR("Control Point Notification too small");
		return BT_GATT_ITER_STOP;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	for (i = 0; i < rsp->num_ase; i++) {
		struct bt_ascs_cp_ase_rsp *ase_rsp;

		if (buf.len < sizeof(*ase_rsp)) {
			BT_ERR("Control Point Notification too small");
			return BT_GATT_ITER_STOP;
		}

		ase_rsp = net_buf_simple_pull_mem(&buf, sizeof(*ase_rsp));

		BT_DBG("op %s (0x%02x) id 0x%02x code %s (0x%02x) "
		       "reason %s (0x%02x)", bt_ascs_op_str(rsp->op), rsp->op,
		       ase_rsp->id, bt_ascs_rsp_str(ase_rsp->code),
		       ase_rsp->code, bt_ascs_reason_str(ase_rsp->reason),
		       ase_rsp->reason);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t ep_notify(struct bt_conn *conn,
			 struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	struct net_buf_simple buf;
	struct bt_audio_ep *ep;

	ep = CONTAINER_OF(params, struct bt_audio_ep, subscribe);

	BT_DBG("conn %p ep %p len %u", conn, ep, length);

	if (!data) {
		BT_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		BT_ERR("Notification too small");
		return BT_GATT_ITER_STOP;
	}

	bt_audio_ep_set_status(ep, &buf);

	return BT_GATT_ITER_CONTINUE;
}

int bt_audio_ep_subscribe(struct bt_conn *conn, struct bt_audio_ep *ep)
{
	BT_DBG("ep %p handle 0x%02x", ep, ep->handle);

	if (ep->subscribe.value_handle) {
		return 0;
	}

	ep->subscribe.value_handle = ep->handle;
	ep->subscribe.ccc_handle = 0x0000;
	ep->subscribe.end_handle = 0xffff;
	ep->subscribe.disc_params = &ep->discover;
	ep->subscribe.notify = ep_notify;
	ep->subscribe.value = BT_GATT_CCC_NOTIFY;
	atomic_set(ep->subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(conn, &ep->subscribe);
}

void bt_audio_ep_set_cp(struct bt_conn *conn, uint16_t handle)
{
	int i, index;

	BT_DBG("conn %p 0x%04x", conn, handle);

	index = bt_conn_index(conn);

	if (!cp_subscribe[index].value_handle) {
		cp_subscribe[index].value_handle = handle;
		cp_subscribe[index].ccc_handle = 0x0000;
		cp_subscribe[index].end_handle = 0xffff;
		cp_subscribe[index].disc_params = &cp_disc[index];
		cp_subscribe[index].notify = cp_notify;
		cp_subscribe[index].value = BT_GATT_CCC_NOTIFY;
		atomic_set(cp_subscribe[index].flags,
			   BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		bt_gatt_subscribe(conn, &cp_subscribe[index]);
	}

	for (i = 0; i < CACHE_SIZE; i++) {
		struct bt_audio_ep *ep = &cache[index][i];

		if (ep->handle) {
			ep->cp_handle = handle;
			bt_audio_ep_subscribe(conn, ep);
		}
	}
}
#endif /* CONFIG_BT_BAP */

void bt_audio_ep_reset(struct bt_conn *conn)
{
	int i, index;

	BT_DBG("conn %p", conn);

	index = bt_conn_index(conn);

	for (i = 0; i < CACHE_SIZE; i++) {
		struct bt_audio_ep *ep = &cache[index][i];

		bt_audio_chan_reset(ep->chan);

		memset(ep, 0, sizeof(*ep));
	}
}

void bt_audio_ep_attach(struct bt_audio_ep *ep,
			      struct bt_audio_chan *chan)
{
	BT_DBG("ep %p chan %p", ep, chan);

	if (!ep || !chan || ep->chan == chan) {
		return;
	}

	if (ep->chan) {
		BT_ERR("ep %p attached to another chan %p", ep, ep->chan);
		return;
	}

	ep->chan = chan;
	chan->ep = ep;

	if (!chan->iso) {
		chan->iso = &ep->iso;
	}
}

void bt_audio_ep_detach(struct bt_audio_ep *ep, struct bt_audio_chan *chan)
{
	BT_DBG("ep %p chan %p", ep, chan);

	if (!ep || !chan || !ep->chan) {
		return;
	}

	if (ep->chan != chan) {
		BT_ERR("ep %p attached to another chan %p", ep, ep->chan);
		return;
	}

	ep->chan = NULL;
	chan->ep = NULL;
}

NET_BUF_SIMPLE_DEFINE_STATIC(ep_buf, CONFIG_BT_L2CAP_TX_MTU);

struct net_buf_simple *bt_audio_ep_create_pdu(uint8_t op)
{
	struct bt_ascs_ase_cp *hdr;

	/* Reset buffer before using */
	net_buf_simple_reset(&ep_buf);

	hdr = net_buf_simple_add(&ep_buf, sizeof(*hdr));
	hdr->op = op;

	return &ep_buf;
}

int bt_audio_ep_config(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		       struct bt_audio_capability *cap, struct bt_codec *codec)
{
	struct bt_ascs_config *req;
	uint8_t cc_len;

	BT_DBG("ep %p buf %p cap %p codec %p", ep, buf, cap, codec);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_ASCS_ASE_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_ASCS_ASE_STATE_CONFIG:
	 /* or 0x02 (QoS Configured) */
	case BT_ASCS_ASE_STATE_QOS:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x dir 0x%02x codec 0x%02x", ep->status.id, cap->type,
	       codec->id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	req->dir = cap->type;
	req->latency = 0x02; /* TODO: Select target latency based on cap? */
	req->phy = 0x02; /* TODO: Select target PHY based on cap? */
	req->codec.id = codec->id;
	req->codec.cid = codec->cid;
	req->codec.vid = codec->vid;

	cc_len = buf->len;
	codec_data_add(buf, "data", codec->data_count, codec->data);
	req->cc_len = buf->len - cc_len;

	return 0;
}

int bt_audio_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		    uint8_t cig, uint8_t cis, struct bt_codec_qos *qos)
{
	struct bt_ascs_qos *req;

	BT_DBG("ep %p buf %p qos %p", ep, buf, qos);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_ASCS_ASE_STATE_CONFIG:
	 /* or 0x02 (QoS Configured) */
	case BT_ASCS_ASE_STATE_QOS:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
	       "phy 0x%02x sdu %u rtn %u latency %u pd %u", ep->status.id,
	       cig, cis, qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	/* TODO: don't hardcode CIG and CIS, they should come from ISO */
	req->cig = cig;
	req->cis = cis;
	sys_put_le24(qos->interval, req->interval);
	req->framing = qos->framing;
	req->phy = qos->phy;
	req->sdu = qos->sdu;
	req->rtn = qos->rtn;
	req->latency = sys_cpu_to_le16(qos->latency);
	sys_put_le24(qos->pd, req->pd);

	return 0;
}

int bt_audio_ep_enable(struct bt_audio_ep *ep, struct net_buf_simple *buf,
		       uint8_t meta_count, struct bt_codec_data *meta)
{
	struct bt_ascs_metadata *req;

	BT_DBG("ep %p buf %p metadata count %u", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_ASCS_ASE_STATE_QOS) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;

	req->len = buf->len;
	codec_data_add(buf, "meta", meta_count, meta);
	req->len = buf->len - req->len;

	return 0;
}

int bt_audio_ep_start(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_ASCS_ASE_STATE_ENABLING &&
	    ep->status.state != BT_ASCS_ASE_STATE_DISABLING) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_audio_ep_disable(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_ASCS_ASE_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_ASCS_ASE_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_audio_ep_stop(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	/* Valid only if ASE_State field value = 0x05 (Disabling). */
	if (ep->status.state != BT_ASCS_ASE_STATE_DISABLING) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_audio_ep_release(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_ASCS_ASE_STATE_CONFIG:
	 /* or 0x02 (QoS Configured) */
	case BT_ASCS_ASE_STATE_QOS:
	 /* or 0x03 (Enabling) */
	case BT_ASCS_ASE_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_ASCS_ASE_STATE_STREAMING:
	 /* or 0x05 (Disabling) */
	case BT_ASCS_ASE_STATE_DISABLING:
		break;
	default:
		if (ep->cap) {
			if (!ep->cap->ops->release(ep->chan)) {
				return 0;
			}
		}

		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_audio_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
		     struct net_buf_simple *buf)
{
	BT_DBG("conn %p ep %p buf %p len %u", conn, ep, buf, buf->len);

	return bt_gatt_write_without_response(conn, ep->cp_handle,
					      buf->data, buf->len, false);
}
