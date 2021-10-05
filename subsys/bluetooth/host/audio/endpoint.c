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
#include "../conn_internal.h"  /* To avoid build errors on use of struct bt_conn" */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_ENDPOINT)
#define LOG_MODULE_NAME bt_audio_ep
#include "common/log.h"

#include "endpoint.h"
#include "chan.h"

#define EP_ISO(_iso) CONTAINER_OF(_iso, struct bt_audio_ep, iso)

static struct bt_audio_ep snks[CONFIG_BT_MAX_CONN][SNK_SIZE];
static struct bt_audio_ep srcs[CONFIG_BT_MAX_CONN][SRC_SIZE];
#if BROADCAST_SRC_CNT > 0
static struct bt_audio_ep broadcast_srcs[BROADCAST_SRC_CNT][BROADCAST_STREAM_CNT];
#endif /* BROADCAST_SRC_CNT > 0 */
#if BROADCAST_SNK_CNT > 0
static struct bt_audio_ep broadcast_snks[BROADCAST_SNK_CNT][BROADCAST_SNK_STREAM_CNT];
#endif /* BROADCAST_SNK_CNT > 0 */

#if defined(CONFIG_BT_BAP)
static struct bt_gatt_subscribe_params cp_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_discover_params cp_disc[CONFIG_BT_MAX_CONN];
#endif

static void ep_iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	struct bt_audio_ep *ep = EP_ISO(chan);
	struct bt_audio_chan_ops *ops = ep->chan->ops;

	BT_DBG("chan %p ep %p len %zu", chan, ep, net_buf_frags_len(buf));

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(ep->chan, buf);
	}
}

static void ep_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_ep *ep = EP_ISO(chan);
	struct bt_audio_chan_ops *ops = ep->chan->ops;
	const bool is_broadcast = bt_audio_ep_is_broadcast(ep);

	BT_DBG("chan %p ep %p", chan, ep);

	if (is_broadcast) {
		bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_STREAMING);
	}

	if (ops != NULL && ops->connected != NULL) {
		ops->connected(ep->chan);
	}

	if (is_broadcast) {
		/* No more actions for broadcast endpoints */
		return;
	}

	if (ep->status.state != BT_ASCS_ASE_STATE_ENABLING) {
		return;
	}

	/* Only auto-start sink role otherwise wait for it to be started. */
	switch (ep->type) {
	case BT_AUDIO_EP_LOCAL:
		/* Server */
		if (ep->chan->cap->type == BT_AUDIO_SINK) {
			bt_audio_chan_start(ep->chan);
		}
		return;
	case BT_AUDIO_EP_REMOTE:
		/* Client */
		if (ep->chan->cap->type == BT_AUDIO_SOURCE) {
			bt_audio_chan_start(ep->chan);
		}
		return;
	}
}

static void ep_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_audio_ep *ep = EP_ISO(chan);
	struct bt_audio_chan_ops *ops = ep->chan->ops;
	const bool is_broadcast = bt_audio_ep_is_broadcast(ep);

	BT_DBG("chan %p ep %p reason 0x%02x", chan, ep, reason);

	if (is_broadcast) {
		if (bt_audio_ep_is_broadcast_src(ep)) {
			bt_audio_chan_set_state(ep->chan,
						BT_AUDIO_CHAN_CONFIGURED);
		} else {
			bt_audio_chan_set_state(ep->chan, BT_AUDIO_CHAN_IDLE);
		}
	}

	if (ops != NULL && ops->disconnected != NULL) {
		ops->disconnected(ep->chan, reason);
	}

	if (is_broadcast) {
		/* No more actions for broadcast endpoints */
		return;
	}

	if (ep->type != BT_AUDIO_EP_LOCAL) {
		return;
	}

	switch (ep->status.state) {
	case BT_ASCS_ASE_STATE_QOS:
		/* noop */
		break;
	case BT_ASCS_ASE_STATE_DISABLING:
		bt_audio_chan_stop(ep->chan);
		break;
	case BT_ASCS_ASE_STATE_ENABLING:
	case BT_ASCS_ASE_STATE_STREAMING:
		bt_audio_chan_disable(ep->chan);
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
	ep->iso.qos->rx = &ep->iso_rx;
	ep->iso.qos->tx = &ep->iso_tx;
}

struct bt_audio_ep *bt_audio_ep_find(struct bt_conn *conn, uint16_t handle)
{
	int i, index;

	index = bt_conn_index(conn);

	for (i = 0; i < SNK_SIZE; i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		if ((handle && ep->handle == handle) ||
		    (!handle && ep->handle)) {
			return ep;
		}
	}

	for (i = 0; i < SRC_SIZE; i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

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

	for (i = 0; i < SNK_SIZE; i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		if (ep->chan == chan) {
			return ep;
		}
	}

	for (i = 0; i < SRC_SIZE; i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		if (ep->chan == chan) {
			return ep;
		}
	}

	return NULL;
}

struct bt_audio_ep *bt_audio_ep_new(struct bt_conn *conn, uint8_t dir,
				    uint16_t handle)
{
	int i, index, size;
	struct bt_audio_ep *cache;

	index = bt_conn_index(conn);

	switch (dir) {
	case BT_AUDIO_SINK:
		cache = snks[index];
		size = SNK_SIZE;
		break;
	case BT_AUDIO_SOURCE:
		cache = srcs[index];
		size = SRC_SIZE;
		break;
	default:
		return NULL;
	}

	for (i = 0; i < size; i++) {
		struct bt_audio_ep *ep = &cache[i];

		if (!ep->handle) {
			bt_audio_ep_init(ep, BT_AUDIO_EP_REMOTE, handle, 0x00);
			return ep;
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_AUDIO_BROADCAST)
struct bt_audio_ep *bt_audio_ep_broadcaster_new(uint8_t index, uint8_t dir)
{
	int i, size;
	struct bt_audio_ep *cache = NULL;

	switch (dir) {
#if BROADCAST_SNK_CNT > 0
	case BT_AUDIO_SINK:
		cache = broadcast_snks[index];
		size = ARRAY_SIZE(broadcast_snks[index]);
		break;
#endif /* BROADCAST_SNK_CNT > 0 */
#if BROADCAST_SRC_CNT > 0
	case BT_AUDIO_SOURCE:
		cache = broadcast_srcs[index];
		size = ARRAY_SIZE(broadcast_srcs[index]);
		break;
#endif /* BROADCAST_SRC_CNT > 0 */
	default:
		return NULL;
	}

	__ASSERT(cache != NULL, "cache is NULL");

	/* Find unallocated endpoint */
	for (i = 0; i < size; i++) {
		struct bt_audio_ep *ep = &cache[i];

		/* If ep->chan is NULL the endpoint is unallocated */
		if (ep->chan == NULL) {
			/* Initialize - It is up to the caller to allocate the
			 * channel pointer.
			 */
			bt_audio_ep_init(ep, BT_AUDIO_EP_LOCAL, 0, 0x00);
			return ep;
		}
	}

	return NULL;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST */

struct bt_audio_ep *bt_audio_ep_get(struct bt_conn *conn, uint8_t dir,
				    uint16_t handle)
{
	struct bt_audio_ep *ep;

	ep = bt_audio_ep_find(conn, handle);
	if (ep || !handle) {
		return ep;
	}

	return bt_audio_ep_new(conn, dir, handle);
}

static void ep_idle(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	/* Notify local capability */
	if (ep->cap && ep->cap->ops && ep->cap->ops->release) {
		ep->cap->ops->release(ep->chan);
	}

	bt_audio_chan_reset(ep->chan);
}

static void ep_qos_reset(struct bt_audio_ep *ep)
{
	ep->iso.qos = memset(&ep->iso_qos, 0, sizeof(ep->iso_qos));
	ep->iso.qos->rx = memset(&ep->iso_rx, 0, sizeof(ep->iso_rx));
	ep->iso.qos->tx = memset(&ep->iso_tx, 0, sizeof(ep->iso_tx));
}

static void ep_config(struct bt_audio_ep *ep, struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_audio_capability_pref *pref;

	if (buf->len < sizeof(*cfg)) {
		BT_ERR("Config status too short");
		return;
	}

	if (!ep->chan) {
		BT_ERR("No channel active for endpoint");
		return;
	}

	cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

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

	/* Reset any exiting QoS configuration */
	ep_qos_reset(ep);

	pref = &ep->chan->cap->pref;

	/* Convert to interval representation so they can be matched by QoS */
	pref->framing = cfg->framing;
	pref->phy = cfg->phy;
	pref->rtn = cfg->rtn;
	pref->latency = sys_le16_to_cpu(cfg->latency);
	pref->pd_min = sys_get_le24(cfg->pd_min);
	pref->pd_max = sys_get_le24(cfg->pd_max);
	pref->pref_pd_min = sys_get_le24(cfg->prefer_pd_min);
	pref->pref_pd_max = sys_get_le24(cfg->prefer_pd_max);

	BT_DBG("dir 0x%02x framing 0x%02x phy 0x%02x rtn %u latency %u "
	       "pd_min %u pd_max %u codec 0x%02x ", ep->chan->cap->type,
	       pref->framing, pref->phy, pref->rtn, pref->latency, pref->pd_min,
	       pref->pd_max, ep->chan->codec->id);

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

	ep->cig_id = qos->cig_id;
	ep->cis_id = qos->cis_id;
	memcpy(&ep->chan->qos->interval, sys_le24_to_cpu(qos->interval),
	       sizeof(qos->interval));
	ep->chan->qos->framing = qos->framing;
	ep->chan->qos->phy = qos->phy;
	ep->chan->qos->sdu = sys_le16_to_cpu(qos->sdu);
	ep->chan->qos->rtn = qos->rtn;
	ep->chan->qos->latency = sys_le16_to_cpu(qos->latency);
	memcpy(&ep->chan->qos->pd, sys_le24_to_cpu(qos->pd), sizeof(qos->pd));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x codec 0x%02x interval %u "
	       "framing 0x%02x phy 0x%02x rtn %u latency %u pd %u",
	       ep->chan->cap->type, ep->cig_id, ep->cis_id, ep->chan->codec->id,
	       ep->chan->qos->interval, ep->chan->qos->framing,
	       ep->chan->qos->phy, ep->chan->qos->rtn, ep->chan->qos->latency,
	       ep->chan->qos->pd);

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
	       ep->cig_id, ep->cis_id);

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
	       ep->cig_id, ep->cis_id);

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
	       ep->cig_id, ep->cis_id);

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
	uint8_t old_state;

	if (!ep) {
		return;
	}

	status = net_buf_simple_pull_mem(buf, sizeof(*status));

	BT_DBG("ep %p handle 0x%04x id 0x%02x state %s", ep, ep->handle,
	       status->id, bt_audio_ep_state_str(status->state));

	old_state = ep->status.state;
	ep->status = *status;

	switch (status->state) {
	case BT_ASCS_ASE_STATE_IDLE:
		ep_idle(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_CONFIG:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x00 (Idle) */
		case BT_ASCS_ASE_STATE_IDLE:
		 /* or 0x01 (Codec Configured) */
		case BT_ASCS_ASE_STATE_CONFIG:
		 /* or 0x02 (QoS Configured) */
		case BT_ASCS_ASE_STATE_QOS:
		 /* or 0x06 (Releasing) */
		case BT_ASCS_ASE_STATE_RELEASING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		ep_config(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_QOS:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x01 (Codec Configured) */
		case BT_ASCS_ASE_STATE_CONFIG:
		 /* or 0x02 (QoS Configured) */
		case BT_ASCS_ASE_STATE_QOS:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		ep_qos(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_ENABLING:
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		if (old_state != BT_ASCS_ASE_STATE_QOS) {
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		ep_enabling(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_STREAMING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		case BT_ASCS_ASE_STATE_QOS:
		 /* or  0x03 (Enabling)*/
		case BT_ASCS_ASE_STATE_ENABLING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		ep_streaming(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_DISABLING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x03 (Enabling) */
		case BT_ASCS_ASE_STATE_ENABLING:
		 /* or 0x04 (Streaming) */
		case BT_ASCS_ASE_STATE_STREAMING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		ep_disabling(ep, buf);
		break;
	case BT_ASCS_ASE_STATE_RELEASING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x01 (Codec Configured) */
		case BT_ASCS_ASE_STATE_CONFIG:
		 /* or 0x02 (QoS Configured) */
		case BT_ASCS_ASE_STATE_QOS:
		 /* or 0x03 (Enabling) */
		case BT_ASCS_ASE_STATE_ENABLING:
		 /* or 0x04 (Streaming) */
		case BT_ASCS_ASE_STATE_STREAMING:
		 /* or 0x04 (Disabling) */
		case BT_ASCS_ASE_STATE_DISABLING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
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
	struct bt_audio_capability_pref *pref = &ep->chan->cap->pref;

	cfg = net_buf_simple_add(buf, sizeof(*cfg));
	cfg->framing = pref->framing;
	cfg->phy = pref->phy;
	cfg->rtn = pref->rtn;
	cfg->latency = sys_cpu_to_le16(pref->latency);
	sys_put_le24(pref->pd_min, cfg->pd_min);
	sys_put_le24(pref->pd_max, cfg->pd_max);
	sys_put_le24(pref->pref_pd_min, cfg->prefer_pd_min);
	sys_put_le24(pref->pref_pd_max, cfg->prefer_pd_max);
	cfg->codec.id = ep->codec.id;
	cfg->codec.cid = sys_cpu_to_le16(ep->codec.cid);
	cfg->codec.vid = sys_cpu_to_le16(ep->codec.vid);

	BT_DBG("dir 0x%02x framing 0x%02x phy 0x%02x rtn %u latency %u "
	       "pd_min %u pd_max %u codec 0x%02x ", ep->chan->cap->type,
	       pref->framing, pref->phy, pref->rtn, pref->latency, pref->pd_min,
	       pref->pd_max, ep->chan->codec->id);

	cfg->cc_len = buf->len;
	codec_data_add(buf, "data", ep->codec.data_count, ep->codec.data);
	cfg->cc_len = buf->len - cfg->cc_len;
}

static void ep_get_status_qos(struct bt_audio_ep *ep,
			      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;

	qos = net_buf_simple_add(buf, sizeof(*qos));
	qos->cig_id = ep->cig_id;
	qos->cis_id = ep->cis_id;
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
	enable->cig_id = ep->cig_id;
	enable->cis_id = ep->cis_id;

	enable->metadata_len = buf->len;
	codec_data_add(buf, "meta", ep->codec.meta_count, ep->codec.meta);
	enable->metadata_len = buf->len - enable->metadata_len;

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x", ep->chan->cap->type,
	       ep->cig_id, ep->cis_id);
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
	int err;

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
		BT_ERR("Unable to parse Metadata: len %u", meta.len);
		err = -EINVAL;

		if (meta.len > 2) {
			/* Value of the Metadata Type field in error */
			err = meta.data[2];
		}

		goto fail;
	}

	return 0;

fail:
	codec->meta_count = 0;
	memset(codec->meta, 0, sizeof(codec->meta));
	return err;
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
	atomic_set_bit(ep->subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

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
		atomic_set_bit(cp_subscribe[index].flags,
			       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		bt_gatt_subscribe(conn, &cp_subscribe[index]);
	}

	for (i = 0; i < SNK_SIZE; i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		if (ep->handle) {
			ep->cp_handle = handle;
		}
	}

	for (i = 0; i < SRC_SIZE; i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		if (ep->handle) {
			ep->cp_handle = handle;
		}
	}
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

int bt_audio_ep_metadata(struct bt_audio_ep *ep, struct net_buf_simple *buf,
			 uint8_t meta_count, struct bt_codec_data *meta)
{
	struct bt_ascs_metadata *req;

	BT_DBG("ep %p buf %p metadata count %u", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
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
#endif /* CONFIG_BT_BAP */

static void ep_reset(struct bt_audio_ep *ep)
{
	BT_DBG("ep %p", ep);

	bt_audio_chan_reset(ep->chan);
	memset(ep, 0, offsetof(struct bt_audio_ep, subscribe));
}

void bt_audio_ep_reset(struct bt_conn *conn)
{
	int i, index;

	BT_DBG("conn %p", conn);

	index = bt_conn_index(conn);

	for (i = 0; i < SNK_SIZE; i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		ep_reset(ep);
	}

	for (i = 0; i < SRC_SIZE; i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		ep_reset(ep);
	}
}

bool bt_audio_ep_is_broadcast_snk(const struct bt_audio_ep *ep)
{
#if BROADCAST_SNK_CNT > 0
	for (int i = 0; i < ARRAY_SIZE(broadcast_snks); i++) {
		if (PART_OF_ARRAY(broadcast_snks[i], ep)) {
			return true;
		}
	}
#endif /* BROADCAST_SNK_CNT > 0 */
	return false;
}

bool bt_audio_ep_is_broadcast_src(const struct bt_audio_ep *ep)
{
#if BROADCAST_SRC_CNT > 0
	for (int i = 0; i < ARRAY_SIZE(broadcast_srcs); i++) {
		if (PART_OF_ARRAY(broadcast_srcs[i], ep)) {
			return true;
		}
	}
#endif /* BROADCAST_SRC_CNT > 0 */
	return false;
}

bool bt_audio_ep_is_broadcast(const struct bt_audio_ep *ep)
{
	return bt_audio_ep_is_broadcast_src(ep) ||
	       bt_audio_ep_is_broadcast_snk(ep);
}

void bt_audio_ep_attach(struct bt_audio_ep *ep, struct bt_audio_chan *chan)
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
