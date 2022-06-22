/* @file
 * @brief Bluetooth Unicast Client
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>

#include "../host/hci_core.h"
#include "../host/conn_internal.h"

#include "endpoint.h"
#include "pacs_internal.h"
#include "unicast_client_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_UNICAST_CLIENT)
#define LOG_MODULE_NAME bt_unicast_client
#include "common/log.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

#define PAC_DIR_UNUSED(dir) ((dir) != BT_AUDIO_DIR_SINK && (dir) != BT_AUDIO_DIR_SOURCE)

static struct unicast_client_pac {
	enum bt_audio_dir dir;
	uint16_t context;
	struct bt_codec codec;
} cache[CONFIG_BT_MAX_CONN][CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];

static const struct bt_uuid *snk_uuid = BT_UUID_PACS_SNK;
static const struct bt_uuid *src_uuid = BT_UUID_PACS_SRC;
static const struct bt_uuid *pacs_context_uuid = BT_UUID_PACS_SUPPORTED_CONTEXT;
static const struct bt_uuid *ase_snk_uuid = BT_UUID_ASCS_ASE_SNK;
static const struct bt_uuid *ase_src_uuid = BT_UUID_ASCS_ASE_SRC;
static const struct bt_uuid *cp_uuid = BT_UUID_ASCS_ASE_CP;

static struct bt_audio_ep snks[CONFIG_BT_MAX_CONN][CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_audio_ep srcs[CONFIG_BT_MAX_CONN][CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_audio_iso isos[CONFIG_BT_MAX_CONN][CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT +
						    CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];

static struct bt_gatt_subscribe_params cp_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_discover_params cp_disc[CONFIG_BT_MAX_CONN];


/* TODO: Move the functions to avoid these prototypes */
static int unicast_client_ep_set_metadata(struct bt_audio_ep *ep,
					  struct net_buf_simple *buf,
					  uint8_t len, struct bt_codec *codec);

static int unicast_client_ep_set_codec(struct bt_audio_ep *ep, uint8_t id,
				       uint16_t cid, uint16_t vid,
				       struct net_buf_simple *buf,
				       uint8_t len, struct bt_codec *codec);

static void unicast_client_ep_iso_recv(struct bt_iso_chan *chan,
				       const struct bt_iso_recv_info *info,
				       struct net_buf *buf)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_ep *ep = audio_iso->sink_ep;
	const struct bt_audio_stream_ops *ops;

	if (ep == NULL) {
		BT_ERR("Could not lookup ep by iso %p", chan);
		return;
	}

	ops = ep->stream->ops;

	BT_DBG("stream %p ep %p len %zu", chan, ep, net_buf_frags_len(buf));

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(ep->stream, info, buf);
	} else {
		BT_WARN("No callback for recv set");
	}
}

static void unicast_client_ep_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_ep *ep = audio_iso->source_ep;
	struct bt_audio_stream_ops *ops = ep->stream->ops;

	BT_DBG("stream %p ep %p", chan, ep);

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(ep->stream);
	}
}

static void unicast_client_ep_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_ep *source_ep = audio_iso->source_ep;
	struct bt_audio_ep *sink_ep = audio_iso->sink_ep;
	struct bt_audio_ep *ep;

	if (&sink_ep->iso->iso_chan == chan) {
		ep = sink_ep;
	} else {
		ep = source_ep;
	}

	if (ep == NULL) {
		BT_ERR("Could not lookup ep by iso %p", chan);
		return;
	}

	BT_DBG("stream %p ep %p dir %u", chan, ep, ep != NULL ? ep->dir : 0);

	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		BT_DBG("endpoint not in enabling state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return;
	}
}

static void unicast_client_ep_iso_disconnected(struct bt_iso_chan *chan,
					       uint8_t reason)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_ep *source_ep = audio_iso->source_ep;
	struct bt_audio_ep *sink_ep = audio_iso->sink_ep;
	const struct bt_audio_stream_ops *ops;
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;

	if (&sink_ep->iso->iso_chan == chan) {
		ep = sink_ep;
	} else {
		ep = source_ep;
	}

	if (ep == NULL) {
		BT_ERR("Could not lookup ep by iso %p", chan);
		return;
	}

	ops = ep->stream->ops;
	stream = ep->stream;

	BT_DBG("stream %p ep %p reason 0x%02x", chan, ep, reason);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream);
	} else {
		BT_WARN("No callback for stopped set");
	}

	bt_unicast_client_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
}

static struct bt_iso_chan_ops unicast_client_iso_ops = {
	.recv		= unicast_client_ep_iso_recv,
	.sent		= unicast_client_ep_iso_sent,
	.connected	= unicast_client_ep_iso_connected,
	.disconnected	= unicast_client_ep_iso_disconnected,
};

void bt_unicast_client_ep_unbind_audio_iso(struct bt_audio_ep *ep)
{
	struct bt_audio_iso *audio_iso = ep->iso;
	struct bt_iso_chan_qos *qos;
	const uint8_t dir = ep->dir;

	BT_DBG("ep %p dir %u audio_iso %p", ep, ep->dir, audio_iso);

	if (audio_iso == NULL) {
		return;
	}

	ep->iso = NULL;
	qos = audio_iso->iso_chan.qos;

	if (dir == BT_AUDIO_DIR_SOURCE) {
		/* If the endpoint is a source, then we need to
		 * reset our RX parameters
		 */
		audio_iso->sink_ep = NULL;
		qos->rx = NULL;
	} else if (dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * reset our TX parameters
		 */
		audio_iso->source_ep = NULL;
		qos->tx = NULL;
	} else {
		__ASSERT(false, "Invalid dir: %u", dir);
	}
}

void bt_unicast_client_ep_bind_audio_iso(struct bt_audio_ep *ep,
					 struct bt_audio_iso *audio_iso)
{
	struct bt_iso_chan *iso_chan;
	struct bt_iso_chan_qos *qos;
	const uint8_t dir = ep->dir;

	ep->iso = audio_iso;

	iso_chan = &ep->iso->iso_chan;

	iso_chan->ops = &unicast_client_iso_ops;
	qos = iso_chan->qos = &ep->iso->iso_qos;

	BT_DBG("ep %p, audio_iso %p, iso_chan %p, dir %u",
	       ep, audio_iso, iso_chan, ep->dir);

	if (dir == BT_AUDIO_DIR_SOURCE) {
		/* If the endpoint is a source, then we need to
		 * configure our RX parameters
		 */
		audio_iso->sink_ep = ep;
		qos->rx = &ep->iso_io_qos;
	} else if (dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * configure our TX parameters
		 */
		audio_iso->source_ep = ep;
		qos->tx = &ep->iso_io_qos;
	} else {
		__ASSERT(false, "Invalid dir: %u", dir);
	}

	ep->stream->iso = iso_chan;
}

static void unicast_client_ep_init(struct bt_audio_ep *ep, uint16_t handle,
				   uint8_t dir)
{

	BT_DBG("ep %p dir 0x%02x handle 0x%04x", ep, dir, handle);

	(void)memset(ep, 0, sizeof(*ep));
	ep->handle = handle;
	ep->status.id = 0U;
	ep->dir = dir;
}

static struct bt_audio_ep *unicast_client_ep_find(struct bt_conn *conn,
						  uint16_t handle)
{
	size_t i;
	uint8_t index;

	index = bt_conn_index(conn);

	for (i = 0; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		if ((handle && ep->handle == handle) ||
		    (!handle && ep->handle)) {
			return ep;
		}
	}

	for (i = 0; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		if ((handle && ep->handle == handle) ||
		    (!handle && ep->handle)) {
			return ep;
		}
	}

	return NULL;
}

struct bt_audio_iso *bt_unicast_client_new_audio_iso(const struct bt_audio_stream *stream)
{
	const struct bt_audio_unicast_group *unicast_group = stream->unicast_group;
	struct bt_conn *acl_conn = stream->conn;
	const uint8_t index = bt_conn_index(acl_conn);
	struct bt_audio_iso *cache = isos[index];
	struct bt_audio_iso *free_audio_iso;

	free_audio_iso = NULL;
	for (size_t i = 0; i < ARRAY_SIZE(isos[index]); i++) {
		struct bt_audio_iso *audio_iso = &cache[i];
		struct bt_audio_ep *ep;

		switch (stream->ep->dir) {
		case BT_AUDIO_DIR_SINK:
			ep = audio_iso->source_ep;

			if (ep == NULL) {
				if (free_audio_iso == NULL) {
					free_audio_iso = audio_iso;
				}
			} else if (ep->unicast_group == unicast_group &&
				   ep->stream->conn == acl_conn) {
				return audio_iso;
			}
			break;
		case BT_AUDIO_DIR_SOURCE:
			ep = audio_iso->sink_ep;

			if (ep == NULL) {
				if (free_audio_iso == NULL) {
					free_audio_iso = audio_iso;
				}
			} else if (ep->unicast_group == unicast_group &&
				   ep->stream->conn == acl_conn) {
				return audio_iso;
			}
			break;
		default:
			return NULL;
		}
	}

	return free_audio_iso;
}

static struct bt_audio_ep *unicast_client_ep_new(struct bt_conn *conn,
						 enum bt_audio_dir dir,
						 uint16_t handle)
{
	size_t i, size;
	uint8_t index;
	struct bt_audio_ep *cache;

	index = bt_conn_index(conn);

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		cache = snks[index];
		size = ARRAY_SIZE(snks[index]);
		break;
	case BT_AUDIO_DIR_SOURCE:
		cache = srcs[index];
		size = ARRAY_SIZE(srcs[index]);
		break;
	default:
		return NULL;
	}

	for (i = 0; i < size; i++) {
		struct bt_audio_ep *ep = &cache[i];

		if (!ep->handle) {
			unicast_client_ep_init(ep, handle, dir);
			return ep;
		}
	}

	return NULL;
}

static struct bt_audio_ep *unicast_client_ep_get(struct bt_conn *conn,
						 enum bt_audio_dir dir,
						 uint16_t handle)
{
	struct bt_audio_ep *ep;

	ep = unicast_client_ep_find(conn, handle);
	if (ep || !handle) {
		return ep;
	}

	return unicast_client_ep_new(conn, dir, handle);
}

static void unicast_client_ep_idle_state(struct bt_audio_ep *ep,
					 struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream = ep->stream;
	const struct bt_audio_stream_ops *ops;

	if (stream == NULL) {
		return;
	}

	/* Notify upper layer */
	ops = stream->ops;
	if (ops != NULL && ops->released != NULL) {
		ops->released(stream);
	} else {
		BT_WARN("No callback for released set");
	}

	bt_audio_stream_reset(stream);
}

static void unicast_client_ep_qos_reset(struct bt_audio_ep *ep)
{
	BT_DBG("ep %p dir %u", ep, ep->dir);

	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		/* If the endpoint is a source, then we need to
		 * configure our RX parameters
		 */
		ep->iso->iso_qos.rx = memset(&ep->iso_io_qos, 0,
					     sizeof(ep->iso_io_qos));
	} else if (ep->dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * configure our TX parameters
		 */
		ep->iso->iso_qos.tx = memset(&ep->iso_io_qos, 0,
					     sizeof(ep->iso_io_qos));
	} else {
		__ASSERT(false, "Invalid ep->dir: %u", ep->dir);
	}
}

static void unicast_client_ep_config_state(struct bt_audio_ep *ep,
					   struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_codec_qos_pref *pref;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*cfg)) {
		BT_ERR("Config status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

	if (stream->codec == NULL || stream->codec->id != cfg->codec.id) {
		BT_ERR("Codec configuration mismatched");
		/* TODO: Release the stream? */
		return;
	}

	if (buf->len < cfg->cc_len) {
		BT_ERR("Malformed ASE Config status: buf->len %u < %u cc_len",
		       buf->len, cfg->cc_len);
		return;
	}

	pref = &stream->ep->qos_pref;

	/* Convert to interval representation so they can be matched by QoS */
	pref->unframed_supported = cfg->framing == BT_ASCS_QOS_FRAMING_UNFRAMED;
	pref->phy = cfg->phy;
	pref->rtn = cfg->rtn;
	pref->latency = sys_le16_to_cpu(cfg->latency);
	pref->pd_min = sys_get_le24(cfg->pd_min);
	pref->pd_max = sys_get_le24(cfg->pd_max);
	pref->pref_pd_min = sys_get_le24(cfg->prefer_pd_min);
	pref->pref_pd_max = sys_get_le24(cfg->prefer_pd_max);

	BT_DBG("dir 0x%02x unframed_supported 0x%02x phy 0x%02x rtn %u "
	       "latency %u pd_min %u pd_max %u codec 0x%02x ",
	       ep->dir, pref->unframed_supported, pref->phy, pref->rtn,
	       pref->latency, pref->pd_min, pref->pd_max, stream->codec->id);

	unicast_client_ep_set_codec(ep, cfg->codec.id,
				    sys_le16_to_cpu(cfg->codec.cid),
				    sys_le16_to_cpu(cfg->codec.vid),
				    buf, cfg->cc_len, NULL);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->configured != NULL) {
		stream->ops->configured(stream, pref);
	} else {
		BT_WARN("No callback for configured set");
	}
}

static void unicast_client_ep_qos_state(struct bt_audio_ep *ep,
					struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*qos)) {
		BT_ERR("QoS status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

	/* Reset any existing QoS configuration */
	unicast_client_ep_qos_reset(ep);

	ep->cig_id = qos->cig_id;
	ep->cis_id = qos->cis_id;
	(void)memcpy(&stream->qos->interval, sys_le24_to_cpu(qos->interval),
		     sizeof(qos->interval));
	stream->qos->framing = qos->framing;
	stream->qos->phy = qos->phy;
	stream->qos->sdu = sys_le16_to_cpu(qos->sdu);
	stream->qos->rtn = qos->rtn;
	stream->qos->latency = sys_le16_to_cpu(qos->latency);
	(void)memcpy(&stream->qos->pd, sys_le24_to_cpu(qos->pd),
		     sizeof(qos->pd));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x codec 0x%02x interval %u "
	       "framing 0x%02x phy 0x%02x rtn %u latency %u pd %u",
	       ep->dir, ep->cig_id, ep->cis_id, stream->codec->id,
	       stream->qos->interval, stream->qos->framing,
	       stream->qos->phy, stream->qos->rtn, stream->qos->latency,
	       stream->qos->pd);

	/* Disconnect ISO if connected */
	if (stream->iso->state == BT_ISO_STATE_CONNECTED) {
		bt_audio_stream_disconnect(stream);
	}

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->qos_set != NULL) {
		stream->ops->qos_set(stream);
	} else {
		BT_WARN("No callback for qos_set set");
	}
}

static void unicast_client_ep_enabling_state(struct bt_audio_ep *ep,
					     struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*enable)) {
		BT_ERR("Enabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x",
	       ep->dir, ep->cig_id, ep->cis_id);

	unicast_client_ep_set_metadata(ep, buf, enable->metadata_len, NULL);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->enabled != NULL) {
		stream->ops->enabled(stream);
	} else {
		BT_WARN("No callback for enabled set");
	}
}

static void unicast_client_ep_streaming_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_stream *stream_status;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*stream_status)) {
		BT_ERR("Streaming status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	stream_status = net_buf_simple_pull_mem(buf, sizeof(*stream_status));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x",
	       ep->dir, ep->cig_id, ep->cis_id);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->started != NULL) {
		stream->ops->started(stream);
	} else {
		BT_WARN("No callback for started set");
	}
}

static void unicast_client_ep_disabling_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_disable *disable;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*disable)) {
		BT_ERR("Disabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	disable = net_buf_simple_pull_mem(buf, sizeof(*disable));

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x",
	       ep->dir, ep->cig_id, ep->cis_id);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->disabled != NULL) {
		stream->ops->disabled(stream);
	} else {
		BT_WARN("No callback for disabled set");
	}
}

static void unicast_client_ep_releasing_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		BT_ERR("No stream active for endpoint");
		return;
	}

	BT_DBG("dir 0x%02x", ep->dir);

	/* The Unicast Client shall terminate any CIS established for that ASE
	 * by following the Connected Isochronous Stream Terminate procedure
	 * defined in Volume 3, Part C, Section 9.3.15 in when the Unicast
	 * Client has determined that the ASE is in the Releasing state.
	 */
	bt_audio_stream_disconnect(stream);
}

static void unicast_client_ep_set_status(struct bt_audio_ep *ep,
					 struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status *status;
	uint8_t old_state;

	if (!ep) {
		return;
	}

	status = net_buf_simple_pull_mem(buf, sizeof(*status));


	old_state = ep->status.state;
	ep->status = *status;

	BT_DBG("ep %p handle 0x%04x id 0x%02x state %s -> %s", ep, ep->handle,
	       status->id, bt_audio_ep_state_str(old_state),
	       bt_audio_ep_state_str(status->state));

	switch (status->state) {
	case BT_AUDIO_EP_STATE_IDLE:
		unicast_client_ep_idle_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x00 (Idle) */
		case BT_AUDIO_EP_STATE_IDLE:
		 /* or 0x01 (Codec Configured) */
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		 /* or 0x02 (QoS Configured) */
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		 /* or 0x06 (Releasing) */
		case BT_AUDIO_EP_STATE_RELEASING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_config_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x01 (Codec Configured) */
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		 /* or 0x02 (QoS Configured) */
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_qos_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_ENABLING:
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		if (old_state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_enabling_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_STREAMING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		 /* or  0x03 (Enabling)*/
		case BT_AUDIO_EP_STATE_ENABLING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_streaming_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_DISABLING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x03 (Enabling) */
		case BT_AUDIO_EP_STATE_ENABLING:
		 /* or 0x04 (Streaming) */
		case BT_AUDIO_EP_STATE_STREAMING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_disabling_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_RELEASING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x01 (Codec Configured) */
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		 /* or 0x02 (QoS Configured) */
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		 /* or 0x03 (Enabling) */
		case BT_AUDIO_EP_STATE_ENABLING:
		 /* or 0x04 (Streaming) */
		case BT_AUDIO_EP_STATE_STREAMING:
		 /* or 0x04 (Disabling) */
		case BT_AUDIO_EP_STATE_DISABLING:
			break;
		default:
			BT_WARN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
		}
		unicast_client_ep_releasing_state(ep, buf);
		break;
	}
}

static void unicast_client_codec_data_add(struct net_buf_simple *buf,
					  const char *prefix,
					  size_t num,
					  struct bt_codec_data *data)
{
	for (size_t i = 0; i < num; i++) {
		struct bt_data *d = &data[i].data;
		struct bt_ascs_codec_config *cc;

		BT_DBG("#%u: %s type 0x%02x len %u", i, prefix, d->type,
		       d->data_len);
		BT_HEXDUMP_DBG(d->data, d->data_len, prefix);

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);
	}
}

static bool unicast_client_codec_config_store(struct bt_data *data,
					      void *user_data)
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
	(void)memcpy(cdata->value, data->data, data->data_len);

	BT_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec->data_count++;

	return true;
}

static int unicast_client_ep_set_codec(struct bt_audio_ep *ep, uint8_t id,
				       uint16_t cid, uint16_t vid,
				       struct net_buf_simple *buf, uint8_t len,
				       struct bt_codec *codec)
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

	/* Reset current metadata */
	codec->data_count = 0;
	(void)memset(codec->data, 0, sizeof(codec->data));

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&ad, net_buf_simple_pull_mem(buf, len),
				      len);

	/* Parse LTV entries */
	bt_data_parse(&ad, unicast_client_codec_config_store, codec);

	/* Check if all entries could be parsed */
	if (ad.len) {
		BT_ERR("Unable to parse Codec Config: len %u", ad.len);
		goto fail;
	}

	return 0;

fail:
	(void)memset(codec, 0, sizeof(*codec));
	return -EINVAL;
}

static bool unicast_client_codec_metadata_store(struct bt_data *data,
						void *user_data)
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
	(void)memcpy(meta->value, data->data, data->data_len);

	BT_HEXDUMP_DBG(meta->value, data->data_len, "data");

	codec->meta_count++;

	return true;
}

static int unicast_client_ep_set_metadata(struct bt_audio_ep *ep,
					  struct net_buf_simple *buf,
					  uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple meta;
	int err;

	if (!ep && !codec) {
		return -EINVAL;
	}

	BT_DBG("ep %p len %u codec %p", ep, len, codec);

	if (!codec) {
		codec = &ep->codec;
	}

	/* Reset current metadata */
	codec->meta_count = 0;
	(void)memset(codec->meta, 0, sizeof(codec->meta));

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&meta, net_buf_simple_pull_mem(buf, len),
				      len);

	/* Parse LTV entries */
	bt_data_parse(&meta, unicast_client_codec_metadata_store, codec);

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
	(void)memset(codec->meta, 0, sizeof(codec->meta));
	return err;
}

void bt_unicast_client_ep_set_state(struct bt_audio_ep *ep, uint8_t state)
{
	uint8_t old_state;

	if (!ep) {
		return;
	}

	BT_DBG("ep %p id 0x%02x %s -> %s", ep, ep->status.id,
	       bt_audio_ep_state_str(ep->status.state),
	       bt_audio_ep_state_str(state));

	old_state = ep->status.state;
	ep->status.state = state;

	if (!ep->stream || old_state == state) {
		return;
	}

	if (state == BT_AUDIO_EP_STATE_CODEC_CONFIGURED) {
		bt_unicast_client_ep_unbind_audio_iso(ep);
	} else if (state == BT_AUDIO_EP_STATE_IDLE) {
		bt_unicast_client_ep_unbind_audio_iso(ep);
		bt_audio_stream_detach(ep->stream);
	}
}

static uint8_t unicast_client_cp_notify(struct bt_conn *conn,
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

static uint8_t unicast_client_ep_notify(struct bt_conn *conn,
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

	unicast_client_ep_set_status(ep, &buf);

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_ep_subscribe(struct bt_conn *conn,
				       struct bt_audio_ep *ep)
{
	BT_DBG("ep %p handle 0x%02x", ep, ep->handle);

	if (ep->subscribe.value_handle) {
		return 0;
	}

	ep->subscribe.value_handle = ep->handle;
	ep->subscribe.ccc_handle = 0x0000;
	ep->subscribe.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	ep->subscribe.disc_params = &ep->discover;
	ep->subscribe.notify = unicast_client_ep_notify;
	ep->subscribe.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(ep->subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(conn, &ep->subscribe);
}

static void unicast_client_ep_set_cp(struct bt_conn *conn, uint16_t handle)
{
	size_t i;
	uint8_t index;

	BT_DBG("conn %p 0x%04x", conn, handle);

	index = bt_conn_index(conn);

	if (!cp_subscribe[index].value_handle) {
		cp_subscribe[index].value_handle = handle;
		cp_subscribe[index].ccc_handle = 0x0000;
		cp_subscribe[index].end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		cp_subscribe[index].disc_params = &cp_disc[index];
		cp_subscribe[index].notify = unicast_client_cp_notify;
		cp_subscribe[index].value = BT_GATT_CCC_NOTIFY;
		atomic_set_bit(cp_subscribe[index].flags,
			       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		bt_gatt_subscribe(conn, &cp_subscribe[index]);
	}

	for (i = 0; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		if (ep->handle) {
			ep->cp_handle = handle;
		}
	}

	for (i = 0; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		if (ep->handle) {
			ep->cp_handle = handle;
		}
	}
}

NET_BUF_SIMPLE_DEFINE_STATIC(ep_buf, CONFIG_BT_L2CAP_TX_MTU);

struct net_buf_simple *bt_unicast_client_ep_create_pdu(uint8_t op)
{
	struct bt_ascs_ase_cp *hdr;

	/* Reset buffer before using */
	net_buf_simple_reset(&ep_buf);

	hdr = net_buf_simple_add(&ep_buf, sizeof(*hdr));
	hdr->op = op;

	return &ep_buf;
}

static int unicast_client_ep_config(struct bt_audio_ep *ep,
				    struct net_buf_simple *buf,
				    struct bt_codec *codec)
{
	struct bt_ascs_config *req;
	uint8_t cc_len;

	BT_DBG("ep %p buf %p codec %p", ep, buf, codec);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_AUDIO_EP_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x dir 0x%02x codec 0x%02x", ep->status.id,
	       ep->dir, codec->id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	req->latency = 0x02; /* TODO: Select target latency based on additional input? */
	req->phy = 0x02; /* TODO: Select target PHY based on additional input? */
	req->codec.id = codec->id;
	req->codec.cid = codec->cid;
	req->codec.vid = codec->vid;

	cc_len = buf->len;
	unicast_client_codec_data_add(buf, "data", codec->data_count,
				      codec->data);
	req->cc_len = buf->len - cc_len;

	return 0;
}

int bt_unicast_client_ep_qos(struct bt_audio_ep *ep, struct net_buf_simple *buf,
			     struct bt_codec_qos *qos)
{
	struct bt_ascs_qos *req;
	struct bt_conn_iso *conn_iso;

	BT_DBG("ep %p buf %p qos %p", ep, buf, qos);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	conn_iso = &ep->iso->iso_chan.iso->iso;

	BT_DBG("id 0x%02x cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
	       "phy 0x%02x sdu %u rtn %u latency %u pd %u", ep->status.id,
	       conn_iso->cig_id, conn_iso->cis_id,
	       qos->interval, qos->framing, qos->phy, qos->sdu,
	       qos->rtn, qos->latency, qos->pd);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;
	/* TODO: don't hardcode CIG and CIS, they should come from ISO */
	req->cig = conn_iso->cig_id;
	req->cis = conn_iso->cis_id;
	sys_put_le24(qos->interval, req->interval);
	req->framing = qos->framing;
	req->phy = qos->phy;
	req->sdu = qos->sdu;
	req->rtn = qos->rtn;
	req->latency = sys_cpu_to_le16(qos->latency);
	sys_put_le24(qos->pd, req->pd);

	return 0;
}

static int unicast_client_ep_enable(struct bt_audio_ep *ep,
				    struct net_buf_simple *buf,
				    struct bt_codec_data *meta,
				    size_t meta_count)
{
	struct bt_ascs_metadata *req;

	BT_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->ase = ep->status.id;

	req->len = buf->len;
	unicast_client_codec_data_add(buf, "meta", meta_count, meta);
	req->len = buf->len - req->len;

	return 0;
}

static int unicast_client_ep_metadata(struct bt_audio_ep *ep,
				      struct net_buf_simple *buf,
				      struct bt_codec_data *meta,
				      size_t meta_count)
{
	struct bt_ascs_metadata *req;

	BT_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
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
	unicast_client_codec_data_add(buf, "meta", meta_count, meta);
	req->len = buf->len - req->len;

	return 0;
}

static int unicast_client_ep_start(struct bt_audio_ep *ep,
				   struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING &&
	    ep->status.state != BT_AUDIO_EP_STATE_DISABLING) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_disable(struct bt_audio_ep *ep,
				     struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
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

static int unicast_client_ep_stop(struct bt_audio_ep *ep,
				  struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	/* Valid only if ASE_State field value = 0x05 (Disabling). */
	if (ep->status.state != BT_AUDIO_EP_STATE_DISABLING) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	BT_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_release(struct bt_audio_ep *ep,
				     struct net_buf_simple *buf)
{
	BT_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
	 /* or 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
	 /* or 0x05 (Disabling) */
	case BT_AUDIO_EP_STATE_DISABLING:
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

int bt_unicast_client_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
			      struct net_buf_simple *buf)
{
	BT_DBG("conn %p ep %p buf %p len %u", conn, ep, buf, buf->len);

	return bt_gatt_write_without_response(conn, ep->cp_handle,
					      buf->data, buf->len, false);
}

static void unicast_client_reset(struct bt_audio_ep *ep)
{
	BT_DBG("ep %p", ep);

	bt_audio_stream_reset(ep->stream);

	switch (ep->dir) {
	case BT_AUDIO_DIR_SINK:
		ep->iso->source_ep = NULL;
		break;
	case BT_AUDIO_DIR_SOURCE:
		ep->iso->sink_ep = NULL;
		break;
	default:
		BT_DBG("Invalid ep->dir %u", ep->dir);
		break;
	}

	(void)memset(ep, 0, offsetof(struct bt_audio_ep, subscribe));
}

static void unicast_client_ep_reset(struct bt_conn *conn)
{
	size_t i;
	uint8_t index;

	BT_DBG("conn %p", conn);

	index = bt_conn_index(conn);

	for (i = 0; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_audio_ep *ep = &snks[index][i];

		unicast_client_reset(ep);
	}

	for (i = 0; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_audio_ep *ep = &srcs[index][i];

		unicast_client_reset(ep);
	}
}

void bt_unicast_client_ep_attach(struct bt_audio_ep *ep,
				 struct bt_audio_stream *stream)
{
	BT_DBG("ep %p stream %p", ep, stream);

	if (ep == NULL || stream == NULL || ep->stream == stream) {
		return;
	}

	if (ep->stream) {
		BT_ERR("ep %p attached to another stream %p", ep, ep->stream);
		return;
	}

	ep->stream = stream;
	stream->ep = ep;

	if (stream->iso == NULL) {
		stream->iso = &ep->iso->iso_chan;
	}
}

void bt_unicast_client_ep_detach(struct bt_audio_ep *ep,
				 struct bt_audio_stream *stream)
{
	BT_DBG("ep %p stream %p", ep, stream);

	if (ep == NULL || stream == NULL || !ep->stream) {
		return;
	}

	if (ep->stream != stream) {
		BT_ERR("ep %p attached to another stream %p", ep, ep->stream);
		return;
	}

	ep->stream = NULL;
	stream->ep = NULL;
}

int bt_unicast_client_config(struct bt_audio_stream *stream,
			     struct bt_codec *codec)
{
	struct bt_audio_ep *ep = stream->ep;
	struct bt_ascs_config_op *op;
	struct net_buf_simple *buf;
	int err;

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_CONFIG_OP);

	op = net_buf_simple_add(buf, sizeof(*op));
	op->num_ases = 0x01;

	err = unicast_client_ep_config(ep, buf, codec);
	if (err) {
		return err;
	}

	err = bt_unicast_client_ep_send(stream->conn, ep, buf);
	if (err) {
		return err;
	}

	return 0;
}

int bt_unicast_client_enable(struct bt_audio_stream *stream,
			     struct bt_codec_data *meta,
			     size_t meta_count)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	BT_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_ENABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_enable(ep, buf, meta, meta_count);
	if (err) {
		return err;
	}

	return bt_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_unicast_client_metadata(struct bt_audio_stream *stream,
			       struct bt_codec_data *meta,
			       size_t meta_count)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_enable_op *req;
	int err;

	BT_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_METADATA_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_metadata(ep, buf, meta, meta_count);
	if (err) {
		return err;
	}

	return bt_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_unicast_client_start(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	BT_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_START_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* If an ASE is in the Enabling state, and if the Unicast Client has
	 * not yet established a CIS for that ASE, the Unicast Client shall
	 * attempt to establish a CIS by using the Connected Isochronous Stream
	 * Central Establishment procedure.
	 */
	err = bt_audio_stream_connect(stream);
	if (!err) {
		return 0;
	}

	if (err && err != -EALREADY) {
		BT_DBG("bt_audio_stream_connect failed: %d", err);
		return err;
	} else if (err == -EALREADY) {
		BT_DBG("ISO %p already connected", stream->iso);
	}

	/* When initiated by the client, valid only if Direction field
	 * parameter value = 0x02 (Server is Audio Source)
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		err = unicast_client_ep_start(ep, buf);
		if (err) {
			return err;
		}
		req->num_ases++;

		return bt_unicast_client_ep_send(stream->conn, ep, buf);
	}

	return 0;
}

int bt_unicast_client_disable(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err;

	BT_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_DISABLE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;

	err = unicast_client_ep_disable(ep, buf);
	if (err) {
		return err;
	}

	return bt_unicast_client_ep_send(stream->conn, ep, buf);
}

int bt_unicast_client_stop(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_start_op *req;
	int err;

	BT_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_STOP_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* When initiated by the client, valid only if Direction field
	 * parameter value = 0x02 (Server is Audio Source)
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		err = unicast_client_ep_stop(ep, buf);
		if (err) {
			return err;
		}
		req->num_ases++;

		return bt_unicast_client_ep_send(stream->conn, ep, buf);
	}

	return 0;
}

int bt_unicast_client_release(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep = stream->ep;
	struct net_buf_simple *buf;
	struct bt_ascs_disable_op *req;
	int err, len;

	BT_DBG("stream %p", stream);

	if (stream->conn == NULL || stream->conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_RELEASE_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x01;
	len = buf->len;

	/* Only attempt to release if not IDLE already */
	if (stream->ep->status.state == BT_AUDIO_EP_STATE_IDLE) {
		bt_audio_stream_reset(stream);
	} else {
		err = unicast_client_ep_release(ep, buf);
		if (err) {
			return err;
		}
	}

	/* Check if anything needs to be send */
	if (len == buf->len) {
		return 0;
	}

	return bt_unicast_client_ep_send(stream->conn, ep, buf);
}

static uint8_t unicast_client_cp_discover_func(struct bt_conn *conn,
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
	unicast_client_ep_set_cp(conn, chrc->value_handle);

	params->func(conn, NULL, NULL, params);

	return BT_GATT_ITER_STOP;
}

static int unicast_client_ase_cp_discover(struct bt_conn *conn,
					  struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;
	params->discover.uuid = cp_uuid;
	params->discover.func = unicast_client_cp_discover_func;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t unicast_client_ase_read_func(struct bt_conn *conn, uint8_t err,
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
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
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
		if (params->num_eps &&
		    unicast_client_ase_cp_discover(conn, params) < 0) {
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

	ep = unicast_client_ep_get(conn, params->dir,
				   read->by_uuid.start_handle);
	if (!ep) {
		BT_WARN("No space left to parse ASE");
		if (params->num_eps) {
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
				BT_ERR("Unable to discover ASE Control Point");
				err = BT_ATT_ERR_UNLIKELY;
				goto fail;
			}
			return BT_GATT_ITER_STOP;
		}
		goto fail;
	}

	unicast_client_ep_set_status(ep, &buf);
	unicast_client_ep_subscribe(conn, ep);

	params->func(conn, NULL, ep, params);

	params->num_eps++;

	return BT_GATT_ITER_CONTINUE;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static int unicast_client_ase_discover(struct bt_conn *conn,
				       struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_ase_read_func;
	params->read.handle_count = 0u;

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->read.by_uuid.uuid = ase_snk_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->read.by_uuid.uuid = ase_src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_context_read_func(struct bt_conn *conn,
						     uint8_t err,
						     struct bt_gatt_read_params *read,
						     const void *data,
						     uint16_t length)
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

	for (i = 0; i < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT; i++) {
		struct unicast_client_pac *pac = &cache[index][i];

		if (PAC_DIR_UNUSED(pac->dir)) {
			continue;
		}

		switch (pac->dir) {
		case BT_AUDIO_DIR_SINK:
			pac->context = sys_le16_to_cpu(context->snk);
			break;
		case BT_AUDIO_DIR_SOURCE:
			pac->context = sys_le16_to_cpu(context->src);
			break;
		default:
			BT_WARN("Cached pac with invalid dir: %u",
				pac->dir);
		}

		BT_DBG("pac %p context 0x%04x", pac, pac->context);
	}

discover_ase:
	/* Read ASE instances */
	if (unicast_client_ase_discover(conn, params) < 0) {
		BT_ERR("Unable to read ASE");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int unicast_client_pacs_context_discover(struct bt_conn *conn,
						struct bt_audio_discover_params *params)
{
	BT_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_context_read_func;
	params->read.handle_count = 0u;
	params->read.by_uuid.uuid = pacs_context_uuid;
	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(conn, &params->read);
}

static struct unicast_client_pac *unicast_client_pac_alloc(struct bt_conn *conn,
							   enum bt_audio_dir dir)
{
	int i, index;

	index = bt_conn_index(conn);

	for (i = 0; i < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT; i++) {
		struct unicast_client_pac *pac = &cache[index][i];

		if (PAC_DIR_UNUSED(pac->dir)) {
			pac->dir = dir;
			return pac;
		}
	}

	return NULL;
}

static uint8_t unicast_client_read_func(struct bt_conn *conn, uint8_t err,
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
		struct unicast_client_pac *bpac;
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

		bpac = unicast_client_pac_alloc(conn, params->dir);
		if (!bpac) {
			BT_WARN("No space left to parse PAC");
			break;
		}

		bpac->codec.id = pac->codec.id;
		bpac->codec.cid = sys_le16_to_cpu(pac->codec.cid);
		bpac->codec.vid = sys_le16_to_cpu(pac->codec.vid);

		if (unicast_client_ep_set_codec(NULL, pac->codec.id,
						sys_le16_to_cpu(pac->codec.cid),
						sys_le16_to_cpu(pac->codec.vid),
						&buf, pac->cc_len,
						&bpac->codec)) {
			BT_ERR("Unable to parse Codec");
			break;
		}

		meta = net_buf_simple_pull_mem(&buf, sizeof(*meta));

		if (buf.len < meta->len) {
			BT_ERR("Malformed PAC: remaining len %u expected %u",
			       buf.len, meta->len);
			break;
		}

		if (unicast_client_ep_set_metadata(NULL, &buf, meta->len,
						   &bpac->codec)) {
			BT_ERR("Unable to parse Codec Metadata");
			break;
		}

		BT_DBG("pac %p codec 0x%02x config count %u meta count %u ",
		       pac, bpac->codec.id, bpac->codec.data_count,
		       bpac->codec.meta_count);

		params->func(conn, &bpac->codec, NULL, params);

		rsp->num_pac--;
		params->num_caps++;
	}

	if (!params->num_caps) {
		goto fail;
	}

	/* Read PACS contexts */
	if (unicast_client_pacs_context_discover(conn, params) < 0) {
		BT_ERR("Unable to read PACS context");
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static void unicast_client_pac_reset(struct bt_conn *conn)
{
	int index = bt_conn_index(conn);
	int i;

	for (i = 0; i < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT; i++) {
		struct unicast_client_pac *pac = &cache[index][i];

		if (!PAC_DIR_UNUSED(pac->dir)) {
			(void)memset(pac, 0, sizeof(*pac));
		}
	}
}

static void unicast_client_disconnected(struct bt_conn *conn, uint8_t reason)
{
	BT_DBG("conn %p reason 0x%02x", conn, reason);

	unicast_client_ep_reset(conn);
	unicast_client_pac_reset(conn);
}

static struct bt_conn_cb conn_cbs = {
	.disconnected = unicast_client_disconnected,
};

int bt_audio_discover(struct bt_conn *conn,
		      struct bt_audio_discover_params *params)
{
	static bool conn_cb_registered;
	uint8_t role;

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	role = conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->read.by_uuid.uuid = snk_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->read.by_uuid.uuid = src_uuid;
	} else {
		return -EINVAL;
	}

	params->read.func = unicast_client_read_func;
	params->read.handle_count = 0u;

	if (!params->read.by_uuid.start_handle) {
		params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	}

	if (!params->read.by_uuid.end_handle) {
		params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
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
