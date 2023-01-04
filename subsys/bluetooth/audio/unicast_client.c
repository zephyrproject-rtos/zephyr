/* @file
 * @brief Bluetooth Unicast Client
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/audio/audio.h>

#include "../host/hci_core.h"
#include "../host/conn_internal.h"

#include "audio_iso.h"
#include "audio_internal.h"
#include "endpoint.h"
#include "pacs_internal.h"
#include "unicast_client_internal.h"

#include <zephyr/logging/log.h>

BUILD_ASSERT(CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 ||
	     CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0,
	     "CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT or "
	     "CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT shall be non-zero");

LOG_MODULE_REGISTER(bt_unicast_client, CONFIG_BT_AUDIO_UNICAST_CLIENT_LOG_LEVEL);

#define PAC_DIR_UNUSED(dir) ((dir) != BT_AUDIO_DIR_SINK && (dir) != BT_AUDIO_DIR_SOURCE)

struct bt_unicast_client_ep {
	uint16_t handle;
	uint16_t cp_handle;
	struct bt_gatt_subscribe_params subscribe;
	struct bt_gatt_discover_params discover;
	struct bt_audio_ep ep;
};

static struct unicast_client_pac {
	enum bt_audio_dir dir;
	uint16_t context;
	struct bt_codec codec;
} pac_cache[CONFIG_BT_MAX_CONN][CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];

static const struct bt_uuid *snk_uuid = BT_UUID_PACS_SNK;
static const struct bt_uuid *src_uuid = BT_UUID_PACS_SRC;
static const struct bt_uuid *pacs_context_uuid = BT_UUID_PACS_SUPPORTED_CONTEXT;
static const struct bt_uuid *pacs_snk_loc_uuid = BT_UUID_PACS_SNK_LOC;
static const struct bt_uuid *pacs_src_loc_uuid = BT_UUID_PACS_SRC_LOC;
static const struct bt_uuid *pacs_avail_ctx_uuid = BT_UUID_PACS_AVAILABLE_CONTEXT;
static const struct bt_uuid *ase_snk_uuid = BT_UUID_ASCS_ASE_SNK;
static const struct bt_uuid *ase_src_uuid = BT_UUID_ASCS_ASE_SRC;
static const struct bt_uuid *cp_uuid = BT_UUID_ASCS_ASE_CP;


#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
static struct bt_unicast_client_ep snks[CONFIG_BT_MAX_CONN]
				       [CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
static struct bt_unicast_client_ep srcs[CONFIG_BT_MAX_CONN]
				       [CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

static struct bt_gatt_subscribe_params cp_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_subscribe_params snk_loc_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_subscribe_params src_loc_subscribe[CONFIG_BT_MAX_CONN];
static struct bt_gatt_subscribe_params avail_ctx_subscribe[CONFIG_BT_MAX_CONN];
/* TODO: We should be able to reduce the number of discover params for CCCD
 * discovery, but requires additional work if it is to support an arbitrary
 * number of EATT bearers, as control point and location CCCD discovery
 * needs to be done in serial to avoid using the same discover parameters
 * twice
 */
static struct bt_gatt_discover_params loc_cc_disc[CONFIG_BT_MAX_CONN];
static struct bt_gatt_discover_params avail_ctx_cc_disc[CONFIG_BT_MAX_CONN];

static const struct bt_audio_unicast_client_cb *unicast_client_cbs;

/* TODO: Move the functions to avoid these prototypes */
static int unicast_client_ep_set_metadata(struct bt_audio_ep *ep, void *data,
					  uint8_t len, struct bt_codec *codec);

static int unicast_client_ep_set_codec(struct bt_audio_ep *ep, uint8_t id,
				       uint16_t cid, uint16_t vid,
				       void *data, uint8_t len, struct bt_codec *codec);

static void unicast_client_ep_iso_recv(struct bt_iso_chan *chan,
				       const struct bt_iso_recv_info *info,
				       struct net_buf *buf)
{
	struct bt_audio_iso *iso = CONTAINER_OF(chan, struct bt_audio_iso, chan);
	const struct bt_audio_stream_ops *ops;
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep = iso->rx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		LOG_DBG("ep %p is not in the streaming state: %s",
			ep, bt_audio_ep_state_str(ep->status.state));
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p len %zu", stream, ep, net_buf_frags_len(buf));
	}

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(stream, info, buf);
	} else {
		LOG_WRN("No callback for recv set");
	}
}

static void unicast_client_ep_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *iso = CONTAINER_OF(chan, struct bt_audio_iso, chan);
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep = iso->tx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p", stream, ep);
	}

	if (stream->ops != NULL && stream->ops->sent != NULL) {
		stream->ops->sent(stream);
	}
}

static void unicast_client_ep_iso_connected(struct bt_audio_ep *ep)
{
	struct bt_audio_stream *stream;

	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		LOG_DBG("endpoint not in enabling state: %s",
			bt_audio_ep_state_str(ep->status.state));
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p dir %s",
		stream, ep, bt_audio_dir_str(ep->dir));
}

static void unicast_client_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *iso = CONTAINER_OF(chan, struct bt_audio_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (iso->rx.ep != NULL) {
		unicast_client_ep_iso_connected(iso->rx.ep);
	}

	if (iso->tx.ep != NULL) {
		unicast_client_ep_iso_connected(iso->tx.ep);
	}
}

static void unicast_client_ep_iso_disconnected(struct bt_audio_ep *ep,
					       uint8_t reason)
{
	struct bt_audio_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("Stream not associated with an ep");
		return;
	}

	LOG_DBG("stream %p ep %p reason 0x%02x", stream, ep, reason);

	if (stream->ops != NULL && stream->ops->stopped != NULL) {
		stream->ops->stopped(stream);
	} else {
		LOG_WRN("No callback for stopped set");
	}
}

static void unicast_client_iso_disconnected(struct bt_iso_chan *chan,
					    uint8_t reason)
{
	struct bt_audio_iso *iso = CONTAINER_OF(chan, struct bt_audio_iso, chan);

	if (iso->rx.ep == NULL && iso->tx.ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	if (iso->rx.ep != NULL) {
		unicast_client_ep_iso_disconnected(iso->rx.ep, reason);
	}

	if (iso->tx.ep != NULL) {
		unicast_client_ep_iso_disconnected(iso->tx.ep, reason);
	}
}

static struct bt_iso_chan_ops unicast_client_iso_ops = {
	.recv		= unicast_client_ep_iso_recv,
	.sent		= unicast_client_ep_iso_sent,
	.connected	= unicast_client_iso_connected,
	.disconnected	= unicast_client_iso_disconnected,
};

bool bt_audio_ep_is_unicast_client(const struct bt_audio_ep *ep)
{
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(snks); i++) {
		if (PART_OF_ARRAY(snks[i], ep)) {
			return true;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(srcs); i++) {
		if (PART_OF_ARRAY(srcs[i], ep)) {
			return true;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	return false;
}

static void unicast_client_ep_init(struct bt_audio_ep *ep, uint16_t handle,
				   uint8_t dir)
{
	struct bt_unicast_client_ep *client_ep;

	LOG_DBG("ep %p dir %s handle 0x%04x",
		ep, bt_audio_dir_str(dir), handle);

	client_ep = CONTAINER_OF(ep, struct bt_unicast_client_ep, ep);

	(void)memset(ep, 0, sizeof(*ep));
	client_ep->handle = handle;
	ep->status.id = 0U;
	ep->dir = dir;
}

static struct bt_audio_ep *unicast_client_ep_find(struct bt_conn *conn,
						  uint16_t handle)
{
	uint8_t index;

	index = bt_conn_index(conn);

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_unicast_client_ep *client_ep = &snks[index][i];

		if ((handle && client_ep->handle == handle) ||
		    (!handle && client_ep->handle)) {
			return &client_ep->ep;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_unicast_client_ep *client_ep = &srcs[index][i];

		if ((handle && client_ep->handle == handle) ||
		    (!handle && client_ep->handle)) {
			return &client_ep->ep;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

	return NULL;
}

struct bt_audio_iso *bt_unicast_client_new_audio_iso(void)
{
	struct bt_audio_iso *audio_iso;

	audio_iso = bt_audio_iso_new();
	if (audio_iso == NULL) {
		return NULL;
	}

	bt_audio_iso_init(audio_iso, &unicast_client_iso_ops);

	LOG_DBG("New audio_iso %p", audio_iso);

	return audio_iso;
}

static struct bt_audio_ep *unicast_client_ep_new(struct bt_conn *conn,
						 enum bt_audio_dir dir,
						 uint16_t handle)
{
	size_t i, size;
	uint8_t index;
	struct bt_unicast_client_ep *cache;

	index = bt_conn_index(conn);

	switch (dir) {
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	case BT_AUDIO_DIR_SINK:
		cache = snks[index];
		size = ARRAY_SIZE(snks[index]);
		break;
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */
#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	case BT_AUDIO_DIR_SOURCE:
		cache = srcs[index];
		size = ARRAY_SIZE(srcs[index]);
		break;
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	default:
		return NULL;
	}

	for (i = 0; i < size; i++) {
		struct bt_unicast_client_ep *client_ep = &cache[i];

		if (!client_ep->handle) {
			unicast_client_ep_init(&client_ep->ep, handle, dir);
			return &client_ep->ep;
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
		LOG_WRN("No callback for released set");
	}

	bt_audio_stream_reset(stream);
}

static void unicast_client_ep_qos_update(struct bt_audio_ep *ep,
					 const struct bt_ascs_ase_status_qos *qos)
{
	struct bt_iso_chan_io_qos *iso_io_qos;

	LOG_DBG("ep %p dir %s audio_iso %p",
		ep, bt_audio_dir_str(ep->dir), ep->iso);

	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		/* If the endpoint is a source, then we need to
		 * reset our RX parameters
		 */
		iso_io_qos = &ep->iso->rx.qos;
	} else if (ep->dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * reset our TX parameters
		 */
		iso_io_qos = &ep->iso->tx.qos;
	} else {
		__ASSERT(false, "Invalid ep->dir: %u", ep->dir);
		return;
	}

	iso_io_qos->phy = qos->phy;
	iso_io_qos->sdu = sys_le16_to_cpu(qos->sdu);
	iso_io_qos->rtn = qos->rtn;
}

static void unicast_client_ep_config_state(struct bt_audio_ep *ep,
					   struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_codec_qos_pref *pref;
	struct bt_audio_stream *stream;
	void *cc;

	if (buf->len < sizeof(*cfg)) {
		LOG_ERR("Config status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_WRN("No stream active for endpoint");
		return;
	}

	cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

	if (stream->codec == NULL) {
		LOG_ERR("Stream %p does not have a codec configured", stream);
		return;
	} else if (stream->codec->id != cfg->codec.id) {
		LOG_ERR("Codec configuration mismatched: %u, %u", stream->codec->id, cfg->codec.id);
		/* TODO: Release the stream? */
		return;
	}

	if (buf->len < cfg->cc_len) {
		LOG_ERR("Malformed ASE Config status: buf->len %u < %u cc_len", buf->len,
			cfg->cc_len);
		return;
	}

	cc = net_buf_simple_pull_mem(buf, cfg->cc_len);

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

	LOG_DBG("dir %s unframed_supported 0x%02x phy 0x%02x rtn %u "
	       "latency %u pd_min %u pd_max %u codec 0x%02x ",
	       bt_audio_dir_str(ep->dir), pref->unframed_supported, pref->phy,
	       pref->rtn, pref->latency, pref->pd_min, pref->pd_max,
	       stream->codec->id);

	unicast_client_ep_set_codec(ep, cfg->codec.id,
				    sys_le16_to_cpu(cfg->codec.cid),
				    sys_le16_to_cpu(cfg->codec.vid),
				    cc, cfg->cc_len, NULL);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->configured != NULL) {
		stream->ops->configured(stream, pref);
	} else {
		LOG_WRN("No callback for configured set");
	}
}

static void unicast_client_ep_qos_state(struct bt_audio_ep *ep,
					struct net_buf_simple *buf,
					uint8_t old_state)
{
	struct bt_ascs_ase_status_qos *qos;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*qos)) {
		LOG_ERR("QoS status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	if (ep->dir == BT_AUDIO_DIR_SINK &&
	    stream->ops != NULL &&
	    stream->ops->disabled != NULL) {
		/* If the old state was enabling or streaming, then the sink
		 * ASE has been disabled. Since the sink ASE does not have a
		 * disabling state, we can check if by comparing the old_state
		 */
		const bool disabled = old_state == BT_AUDIO_EP_STATE_ENABLING ||
				      old_state == BT_AUDIO_EP_STATE_STREAMING;

		if (disabled) {
			stream->ops->disabled(stream);
		}
	}

	qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

	/* Update existing QoS configuration */
	unicast_client_ep_qos_update(ep, qos);

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

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x codec 0x%02x interval %u "
	       "framing 0x%02x phy 0x%02x rtn %u latency %u pd %u",
	       bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id,
	       stream->codec->id, stream->qos->interval, stream->qos->framing,
	       stream->qos->phy, stream->qos->rtn, stream->qos->latency,
	       stream->qos->pd);

	/* Disconnect ISO if connected */
	if (ep->iso->chan.state == BT_ISO_STATE_CONNECTED) {
		bt_audio_stream_disconnect(stream);
	} else {
		/* We setup the data path here, as this is the earliest where
		 * we have the ISO <-> EP coupling completed (due to setting
		 * the CIS ID in the QoS procedure).
		 */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			/* If the endpoint is a source, then we need to
			 * configure our RX parameters
			 */
			bt_audio_codec_to_iso_path(&ep->iso->rx.path,
						   stream->codec);
		} else {
			/* If the endpoint is a sink, then we need to
			 * configure our TX parameters
			 */
			bt_audio_codec_to_iso_path(&ep->iso->tx.path,
						   stream->codec);
		}
	}

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->qos_set != NULL) {
		stream->ops->qos_set(stream);
	} else {
		LOG_WRN("No callback for qos_set set");
	}
}

static void unicast_client_ep_enabling_state(struct bt_audio_ep *ep,
					     struct net_buf_simple *buf,
					     bool state_changed)
{
	struct bt_ascs_ase_status_enable *enable;
	struct bt_audio_stream *stream;
	void *metadata;

	if (buf->len < sizeof(*enable)) {
		LOG_ERR("Enabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	enable = net_buf_simple_pull_mem(buf, sizeof(*enable));

	if (buf->len < enable->metadata_len) {
		LOG_ERR("Malformed PDU: remaining len %u expected %u", buf->len,
			enable->metadata_len);
		return;
	}

	metadata = net_buf_simple_pull_mem(buf, enable->metadata_len);

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x",
		bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	unicast_client_ep_set_metadata(ep, metadata, enable->metadata_len, NULL);

	/* Notify upper layer
	 *
	 * If the state did not change then only the metadata was changed
	 */
	if (state_changed) {
		if (stream->ops != NULL && stream->ops->enabled != NULL) {
			stream->ops->enabled(stream);
		} else {
			LOG_WRN("No callback for enabled set");
		}
	} else {
		if (stream->ops != NULL &&
		    stream->ops->metadata_updated != NULL) {
			stream->ops->metadata_updated(stream);
		} else {
			LOG_WRN("No callback for metadata_updated set");
		}
	}
}

static void unicast_client_ep_streaming_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf,
					      bool state_changed)
{
	struct bt_ascs_ase_status_stream *stream_status;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*stream_status)) {
		LOG_ERR("Streaming status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	stream_status = net_buf_simple_pull_mem(buf, sizeof(*stream_status));

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x",
		bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	/* Notify upper layer
	 *
	 * If the state did not change then only the metadata was changed
	 */
	if (state_changed) {
		if (stream->ops != NULL && stream->ops->started != NULL) {
			stream->ops->started(stream);
		} else {
			LOG_WRN("No callback for started set");
		}
	} else {
		if (stream->ops != NULL &&
		    stream->ops->metadata_updated != NULL) {
			stream->ops->metadata_updated(stream);
		} else {
			LOG_WRN("No callback for metadata_updated set");
		}
	}
}

static void unicast_client_ep_disabling_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_disable *disable;
	struct bt_audio_stream *stream;

	if (buf->len < sizeof(*disable)) {
		LOG_ERR("Disabling status too short");
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	disable = net_buf_simple_pull_mem(buf, sizeof(*disable));

	LOG_DBG("dir %s cig 0x%02x cis 0x%02x",
		bt_audio_dir_str(ep->dir), ep->cig_id, ep->cis_id);

	/* Notify upper layer */
	if (stream->ops != NULL && stream->ops->disabled != NULL) {
		stream->ops->disabled(stream);
	} else {
		LOG_WRN("No callback for disabled set");
	}
}

static void unicast_client_ep_releasing_state(struct bt_audio_ep *ep,
					      struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream active for endpoint");
		return;
	}

	LOG_DBG("dir %s", bt_audio_dir_str(ep->dir));

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
	struct bt_unicast_client_ep *client_ep;
	bool state_changed;
	uint8_t old_state;

	if (!ep) {
		return;
	}

	client_ep = CONTAINER_OF(ep, struct bt_unicast_client_ep, ep);

	status = net_buf_simple_pull_mem(buf, sizeof(*status));

	old_state = ep->status.state;
	ep->status = *status;
	state_changed = old_state != ep->status.state;

	LOG_DBG("ep %p handle 0x%04x id 0x%02x dir %s state %s -> %s",
		ep, client_ep->handle, status->id, bt_audio_dir_str(ep->dir),
		bt_audio_ep_state_str(old_state),
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
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_config_state(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		/* QoS configured have different allowed states depending on the endpoint type */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x01 (Codec Configured) */
			case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
			case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			/* or 0x05 (Disabling) */
			case BT_AUDIO_EP_STATE_DISABLING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_audio_ep_state_str(old_state),
					bt_audio_ep_state_str(ep->status.state));
				return;
			}
		} else {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x01 (Codec Configured) */
			case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
			/* or 0x02 (QoS Configured) */
			case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			/* or 0x03 (Enabling) */
			case BT_AUDIO_EP_STATE_ENABLING:
			/* or 0x04 (Streaming)*/
			case BT_AUDIO_EP_STATE_STREAMING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_audio_ep_state_str(old_state),
					bt_audio_ep_state_str(ep->status.state));
				return;
			}
		}

		unicast_client_ep_qos_state(ep, buf, old_state);
		break;
	case BT_AUDIO_EP_STATE_ENABLING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x02 (QoS Configured) */
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		 /* or 0x03 (Enabling) */
		case BT_AUDIO_EP_STATE_ENABLING:
			break;
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_enabling_state(ep, buf, state_changed);
		break;
	case BT_AUDIO_EP_STATE_STREAMING:
		switch (old_state) {
		/* Valid only if ASE_State field = 0x03 (Enabling)*/
		case BT_AUDIO_EP_STATE_ENABLING:
		 /* or 0x04 (Streaming)*/
		case BT_AUDIO_EP_STATE_STREAMING:
			break;
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_streaming_state(ep, buf, state_changed);
		break;
	case BT_AUDIO_EP_STATE_DISABLING:
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			switch (old_state) {
			/* Valid only if ASE_State field = 0x03 (Enabling) */
			case BT_AUDIO_EP_STATE_ENABLING:
			/* or 0x04 (Streaming) */
			case BT_AUDIO_EP_STATE_STREAMING:
				break;
			default:
				LOG_WRN("Invalid state transition: %s -> %s",
					bt_audio_ep_state_str(old_state),
					bt_audio_ep_state_str(ep->status.state));
				return;
			}
		} else {
			/* Sinks cannot go into the disabling state */
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
			return;
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
			break;
		 /* or 0x04 (Disabling) */
		case BT_AUDIO_EP_STATE_DISABLING:
			if (ep->dir == BT_AUDIO_DIR_SOURCE) {
				break;
			} /* else fall through for sink */

			/* fall through */
		default:
			LOG_WRN("Invalid state transition: %s -> %s",
				bt_audio_ep_state_str(old_state),
				bt_audio_ep_state_str(ep->status.state));
			return;
		}

		unicast_client_ep_releasing_state(ep, buf);
		break;
	}
}

static void unicast_client_codec_data_add(struct net_buf_simple *buf,
					  const char *prefix,
					  size_t num,
					  const struct bt_codec_data *data)
{
	for (size_t i = 0; i < num; i++) {
		const struct bt_data *d = &data[i].data;
		struct bt_ascs_codec_config *cc;

		LOG_DBG("#%u: %s type 0x%02x len %u", i, prefix, d->type, d->data_len);
		LOG_HEXDUMP_DBG(d->data, d->data_len, prefix);

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
		LOG_ERR("No slot available for Codec Config");
		return false;
	}

	cdata = &codec->data[codec->data_count];

	if (data->data_len > sizeof(cdata->value)) {
		LOG_ERR("Not enough space for Codec Config: %u > %zu", data->data_len,
			sizeof(cdata->value));
		return false;
	}

	LOG_DBG("#%u type 0x%02x len %u", codec->data_count, data->type, data->data_len);

	cdata->data.type = data->type;
	cdata->data.data_len = data->data_len;

	/* Deep copy data contents */
	cdata->data.data = cdata->value;
	(void)memcpy(cdata->value, data->data, data->data_len);

	LOG_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec->data_count++;

	return true;
}

static int unicast_client_ep_set_codec(struct bt_audio_ep *ep, uint8_t id,
				       uint16_t cid, uint16_t vid,
				       void *data, uint8_t len,
				       struct bt_codec *codec)
{
	struct net_buf_simple ad;

	if (!ep && !codec) {
		return -EINVAL;
	}

	LOG_DBG("ep %p codec id 0x%02x cid 0x%04x vid 0x%04x len %u", ep, id, cid, vid, len);

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

	net_buf_simple_init_with_data(&ad, data, len);

	/* Parse LTV entries */
	bt_data_parse(&ad, unicast_client_codec_config_store, codec);

	/* Check if all entries could be parsed */
	if (ad.len) {
		LOG_ERR("Unable to parse Codec Config: len %u", ad.len);
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
		LOG_ERR("No slot available for Codec Config Metadata");
		return false;
	}

	meta = &codec->meta[codec->meta_count];

	if (data->data_len > sizeof(meta->value)) {
		LOG_ERR("Not enough space for Codec Config Metadata: %u > %zu", data->data_len,
			sizeof(meta->value));
		return false;
	}

	LOG_DBG("#%u type 0x%02x len %u", codec->meta_count, data->type, data->data_len);

	meta->data.type = data->type;
	meta->data.data_len = data->data_len;

	/* Deep copy data contents */
	meta->data.data = meta->value;
	(void)memcpy(meta->value, data->data, data->data_len);

	LOG_HEXDUMP_DBG(meta->value, data->data_len, "data");

	codec->meta_count++;

	return true;
}

static int unicast_client_ep_set_metadata(struct bt_audio_ep *ep, void *data,
					  uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple meta;
	int err;

	if (!ep && !codec) {
		return -EINVAL;
	}

	LOG_DBG("ep %p len %u codec %p", ep, len, codec);

	if (!codec) {
		codec = &ep->codec;
	}

	/* Reset current metadata */
	codec->meta_count = 0;
	(void)memset(codec->meta, 0, sizeof(codec->meta));

	if (!len) {
		return 0;
	}

	net_buf_simple_init_with_data(&meta, data, len);

	/* Parse LTV entries */
	bt_data_parse(&meta, unicast_client_codec_metadata_store, codec);

	/* Check if all entries could be parsed */
	if (meta.len) {
		LOG_ERR("Unable to parse Metadata: len %u", meta.len);
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

static uint8_t unicast_client_cp_notify(struct bt_conn *conn,
					struct bt_gatt_subscribe_params *params,
					const void *data, uint16_t length)
{
	struct bt_ascs_cp_rsp *rsp;
	int i;

	struct net_buf_simple buf;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		LOG_ERR("Control Point Notification too small");
		return BT_GATT_ITER_STOP;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	for (i = 0; i < rsp->num_ase; i++) {
		struct bt_ascs_cp_ase_rsp *ase_rsp;

		if (buf.len < sizeof(*ase_rsp)) {
			LOG_ERR("Control Point Notification too small");
			return BT_GATT_ITER_STOP;
		}

		ase_rsp = net_buf_simple_pull_mem(&buf, sizeof(*ase_rsp));

		LOG_DBG("op %s (0x%02x) id 0x%02x code %s (0x%02x) "
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
	struct bt_unicast_client_ep *client_ep;

	client_ep = CONTAINER_OF(params, struct bt_unicast_client_ep, subscribe);

	LOG_DBG("conn %p ep %p len %u", conn, &client_ep->ep, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		LOG_ERR("Notification too small");
		return BT_GATT_ITER_STOP;
	}

	unicast_client_ep_set_status(&client_ep->ep, &buf);

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_ep_subscribe(struct bt_conn *conn,
				       struct bt_audio_ep *ep)
{
	struct bt_unicast_client_ep *client_ep;

	client_ep = CONTAINER_OF(ep, struct bt_unicast_client_ep, ep);

	LOG_DBG("ep %p handle 0x%02x", ep, client_ep->handle);

	if (client_ep->subscribe.value_handle) {
		return 0;
	}

	client_ep->subscribe.value_handle = client_ep->handle;
	client_ep->subscribe.ccc_handle = 0x0000;
	client_ep->subscribe.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client_ep->subscribe.disc_params = &client_ep->discover;
	client_ep->subscribe.notify = unicast_client_ep_notify;
	client_ep->subscribe.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(client_ep->subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(conn, &client_ep->subscribe);
}

static void unicast_client_cp_sub_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_subscribe_params *sub_params)
{
	struct bt_audio_discover_params *params;

	LOG_DBG("conn %p err %u", conn, err);

	params = CONTAINER_OF(sub_params->disc_params,
			      struct bt_audio_discover_params,
			      discover);

	params->err = err;
	params->func(conn, NULL, NULL, params);
}

static void unicast_client_ep_set_cp(struct bt_conn *conn,
				     struct bt_audio_discover_params *params,
				     uint16_t handle)
{
	uint8_t index;

	LOG_DBG("conn %p 0x%04x", conn, handle);

	index = bt_conn_index(conn);

	if (!cp_subscribe[index].value_handle) {
		int err;

		cp_subscribe[index].value_handle = handle;
		cp_subscribe[index].ccc_handle = 0x0000;
		cp_subscribe[index].end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		cp_subscribe[index].disc_params = &params->discover;
		cp_subscribe[index].notify = unicast_client_cp_notify;
		cp_subscribe[index].value = BT_GATT_CCC_NOTIFY;
		cp_subscribe[index].subscribe = unicast_client_cp_sub_cb;
		atomic_set_bit(cp_subscribe[index].flags,
			       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		err = bt_gatt_subscribe(conn, &cp_subscribe[index]);
		if (err != 0) {
			LOG_DBG("Failed to subscribe: %d", err);

			params->err = BT_ATT_ERR_UNLIKELY;

			params->func(conn, NULL, NULL, params);

			return;
		}
	}

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_unicast_client_ep *client_ep = &snks[index][i];

		if (client_ep->handle) {
			client_ep->cp_handle = handle;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_unicast_client_ep *client_ep = &srcs[index][i];

		if (client_ep->handle) {
			client_ep->cp_handle = handle;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
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
				    const struct bt_codec *codec)
{
	struct bt_ascs_config *req;
	uint8_t cc_len;

	LOG_DBG("ep %p buf %p codec %p", ep, buf, codec);

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
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x dir %s codec 0x%02x",
		ep->status.id, bt_audio_dir_str(ep->dir), codec->id);

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

	LOG_DBG("ep %p buf %p qos %p", ep, buf, qos);

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
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	conn_iso = &ep->iso->chan.iso->iso;

	LOG_DBG("id 0x%02x cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
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

	LOG_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

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

	LOG_DBG("ep %p buf %p metadata count %zu", ep, buf, meta_count);

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
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

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
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING &&
	    ep->status.state != BT_AUDIO_EP_STATE_DISABLING) {
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_disable(struct bt_audio_ep *ep,
				     struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

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
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_stop(struct bt_audio_ep *ep,
				  struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

	if (!ep) {
		return -EINVAL;
	}

	/* Valid only if ASE_State field value = 0x05 (Disabling). */
	if (ep->status.state != BT_AUDIO_EP_STATE_DISABLING) {
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

static int unicast_client_ep_release(struct bt_audio_ep *ep,
				     struct net_buf_simple *buf)
{
	LOG_DBG("ep %p buf %p", ep, buf);

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
		LOG_ERR("Invalid state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EINVAL;
	}

	LOG_DBG("id 0x%02x", ep->status.id);

	net_buf_simple_add_u8(buf, ep->status.id);

	return 0;
}

int bt_unicast_client_ep_send(struct bt_conn *conn, struct bt_audio_ep *ep,
			      struct net_buf_simple *buf)
{
	struct bt_unicast_client_ep *client_ep = CONTAINER_OF(ep, struct bt_unicast_client_ep, ep);

	LOG_DBG("conn %p ep %p buf %p len %u", conn, ep, buf, buf->len);

	return bt_gatt_write_without_response(conn, client_ep->cp_handle,
					      buf->data, buf->len, false);
}

static void unicast_client_reset(struct bt_audio_ep *ep)
{
	struct bt_unicast_client_ep *client_ep = CONTAINER_OF(ep, struct bt_unicast_client_ep, ep);
	struct bt_audio_stream *stream = ep->stream;

	LOG_DBG("ep %p", ep);

	if (stream != NULL && ep->status.state != BT_AUDIO_EP_STATE_IDLE) {
		const struct bt_audio_stream_ops *ops;

		/* Notify upper layer */
		ops = stream->ops;
		if (ops != NULL && ops->released != NULL) {
			ops->released(stream);
		} else {
			LOG_WRN("No callback for released set");
		}
	}

	bt_audio_stream_reset(stream);

	(void)memset(ep, 0, sizeof(*ep));

	client_ep->cp_handle = 0U;
	client_ep->handle = 0U;
	(void)memset(&client_ep->discover, 0, sizeof(client_ep->discover));
	/* Need to keep the subscribe params intact for the callback */
}

static void unicast_client_ep_reset(struct bt_conn *conn)
{
	uint8_t index;

	LOG_DBG("conn %p", conn);

	index = bt_conn_index(conn);

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(snks[index]); i++) {
		struct bt_audio_ep *ep = &snks[index][i].ep;

		unicast_client_reset(ep);
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0
	for (size_t i = 0U; i < ARRAY_SIZE(srcs[index]); i++) {
		struct bt_audio_ep *ep = &srcs[index][i].ep;

		unicast_client_reset(ep);
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
}

int bt_unicast_client_config(struct bt_audio_stream *stream,
			     const struct bt_codec *codec)
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

	LOG_DBG("stream %p", stream);

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

	LOG_DBG("stream %p", stream);

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

	LOG_DBG("stream %p", stream);

	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_START_OP);

	req = net_buf_simple_add(buf, sizeof(*req));
	req->num_ases = 0x00;

	/* If an ASE is in the Enabling state, and if the Unicast Client has
	 * not yet established a CIS for that ASE, the Unicast Client shall
	 * attempt to establish a CIS by using the Connected Isochronous Stream
	 * Central Establishment procedure.
	 */
	err = bt_audio_stream_connect(stream);
	if (err && err != -EALREADY) {
		LOG_DBG("bt_audio_stream_connect failed: %d", err);
		return err;
	} else if (err == -EALREADY) {
		LOG_DBG("ISO %p already connected", bt_audio_stream_iso_chan_get(stream));
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

	LOG_DBG("stream %p", stream);

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

	LOG_DBG("stream %p", stream);

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

	LOG_DBG("stream %p", stream);

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
			LOG_ERR("Unable to find ASE Control Point");
		}
		params->func(conn, NULL, NULL, params);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	unicast_client_ep_set_cp(conn, params, chrc->value_handle);

	return BT_GATT_ITER_STOP;
}

static int unicast_client_ase_cp_discover(struct bt_conn *conn,
					  struct bt_audio_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

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

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err) {
		if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND && params->num_eps) {
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
				LOG_ERR("Unable to discover ASE Control Point");
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
			LOG_ERR("Unable to discover ASE Control Point");
			err = BT_ATT_ERR_UNLIKELY;
			goto fail;
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(struct bt_ascs_ase_status)) {
		LOG_ERR("Read response too small");
		goto fail;
	}

	ep = unicast_client_ep_get(conn, params->dir,
				   read->by_uuid.start_handle);
	if (!ep) {
		LOG_WRN("No space left to parse ASE");
		if (params->num_eps) {
			if (unicast_client_ase_cp_discover(conn, params) < 0) {
				LOG_ERR("Unable to discover ASE Control Point");
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
	LOG_DBG("conn %p params %p", conn, params);

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

static uint8_t unicast_client_pacs_avail_ctx_read_func(struct bt_conn *conn,
						       uint8_t err,
						       struct bt_gatt_read_params *read,
						       const void *data,
						       uint16_t length)
{
	struct bt_audio_discover_params *params;
	struct bt_pacs_context context;
	struct net_buf_simple buf;

	params = CONTAINER_OF(read, struct bt_audio_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || data == NULL || length != sizeof(context)) {
		LOG_DBG("Could not read available context: %d, %p, %u", err, data, length);

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context.snk = net_buf_simple_pull_le16(&buf);
	context.src = net_buf_simple_pull_le16(&buf);

	LOG_DBG("sink context %u, source context %u", context.snk, context.src);

	if (unicast_client_cbs != NULL &&
	    unicast_client_cbs->available_contexts != NULL) {
		unicast_client_cbs->available_contexts(conn, context.snk,
						       context.src);
	}

	/* Read ASE instances */
	if (unicast_client_ase_discover(conn, params) < 0) {
		LOG_ERR("Unable to read ASE");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t unicast_client_pacs_avail_ctx_notify_cb(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data,
						      uint16_t length)
{
	struct bt_pacs_context context;
	struct net_buf_simple buf;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	/* Terminate early if there's no callbacks */
	if (unicast_client_cbs == NULL ||
	    unicast_client_cbs->available_contexts == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len != sizeof(context)) {
		LOG_ERR("Avail_ctx notification incorrect size: %u", length);
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context.snk = net_buf_simple_pull_le16(&buf);
	context.src = net_buf_simple_pull_le16(&buf);

	LOG_DBG("sink context %u, source context %u", context.snk, context.src);

	if (unicast_client_cbs != NULL &&
	    unicast_client_cbs->available_contexts != NULL) {
		unicast_client_cbs->available_contexts(conn, context.snk,
						       context.src);
	}

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_pacs_avail_ctx_read(struct bt_conn *conn,
					      struct bt_audio_discover_params *params,
					      uint16_t handle)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_avail_ctx_read_func;
	params->read.handle_count = 1U;
	params->read.single.handle = handle;
	params->read.single.offset = 0U;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_avail_ctx_discover_cb(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 struct bt_gatt_discover_params *discover)
{
	struct bt_audio_discover_params *params;
	uint8_t index = bt_conn_index(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	params = CONTAINER_OF(discover, struct bt_audio_discover_params,
			      discover);

	if (!attr) {
		/* If available_ctx is not found, we terminate the discovery as
		 * the characteristic is mandatory
		 */

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		struct bt_gatt_subscribe_params *sub_params;

		sub_params = &avail_ctx_subscribe[index];

		if (sub_params->value_handle == 0) {
			LOG_DBG("Subscribing to handle %u", chrc->value_handle);
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = 0x0000; /* auto discover ccc */
			sub_params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
			sub_params->disc_params = &avail_ctx_cc_disc[index];
			sub_params->notify = unicast_client_pacs_avail_ctx_notify_cb;
			sub_params->value = BT_GATT_CCC_NOTIFY;

			err = bt_gatt_subscribe(conn, sub_params);
			if (err != 0 && err != -EALREADY) {
				LOG_ERR("Failed to subscribe to avail_ctx: %d", err);
			}
		} /* else already subscribed */
	} else {
		LOG_DBG("Invalid chrc->properties: %u", chrc->properties);
		/* If the characteristic is not subscribable we terminate the
		 * discovery as BT_GATT_CHRC_NOTIFY is mandatory
		 */
		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	err = unicast_client_pacs_avail_ctx_read(conn, params,
						 chrc->value_handle);
	if (err != 0) {
		LOG_DBG("Failed to read PACS avail_ctx: %d", err);
		params->err = err;

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int unicast_client_pacs_avail_ctx_discover(struct bt_conn *conn,
						  struct bt_audio_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->discover.uuid = pacs_avail_ctx_uuid;
	params->discover.func = unicast_client_pacs_avail_ctx_discover_cb;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
}

static uint8_t unicast_client_pacs_location_read_func(struct bt_conn *conn,
						      uint8_t err,
						      struct bt_gatt_read_params *read,
						      const void *data,
						      uint16_t length)
{
	struct bt_audio_discover_params *params;
	struct net_buf_simple buf;
	uint32_t location;

	params = CONTAINER_OF(read, struct bt_audio_discover_params, read);

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || data == NULL || length != sizeof(location)) {
		LOG_DBG("Unable to read PACS location for dir %s: %u, %p, %u",
			bt_audio_dir_str(params->dir), err, data, length);

		params->func(conn, NULL, NULL, params);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	location = net_buf_simple_pull_le32(&buf);

	LOG_DBG("dir %s loc %X", bt_audio_dir_str(params->dir), location);

	if (unicast_client_cbs != NULL && unicast_client_cbs->location != NULL) {
		unicast_client_cbs->location(conn, params->dir,
					     (enum bt_audio_location)location);
	}

	/* Read available contexts */
	if (unicast_client_pacs_avail_ctx_discover(conn, params) < 0) {
		LOG_ERR("Unable to read available contexts");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t unicast_client_pacs_location_notify_cb(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data,
						      uint16_t length)
{
	struct net_buf_simple buf;
	enum bt_audio_dir dir;
	uint32_t location;

	LOG_DBG("conn %p len %u", conn, length);

	if (!data) {
		LOG_DBG("Unsubscribed");
		params->value_handle = 0x0000;
		return BT_GATT_ITER_STOP;
	}

	/* Terminate early if there's no callbacks */
	if (unicast_client_cbs == NULL || unicast_client_cbs->location == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len != sizeof(location)) {
		LOG_ERR("Location notification incorrect size: %u", length);
		return BT_GATT_ITER_STOP;
	}

	if (PART_OF_ARRAY(snk_loc_subscribe, params)) {
		dir = BT_AUDIO_DIR_SINK;
	} else if (PART_OF_ARRAY(src_loc_subscribe, params)) {
		dir = BT_AUDIO_DIR_SOURCE;
	} else {
		LOG_ERR("Invalid notification");

		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	location = net_buf_simple_pull_le32(&buf);

	LOG_DBG("dir %s loc %X", bt_audio_dir_str(dir), location);

	if (unicast_client_cbs != NULL && unicast_client_cbs->location != NULL) {
		unicast_client_cbs->location(conn, dir,
					     (enum bt_audio_location)location);
	}

	return BT_GATT_ITER_CONTINUE;
}

static int unicast_client_pacs_location_read(struct bt_conn *conn,
					     struct bt_audio_discover_params *params,
					     uint16_t handle)
{
	LOG_DBG("conn %p params %p", conn, params);

	params->read.func = unicast_client_pacs_location_read_func;
	params->read.handle_count = 1U;
	params->read.single.handle = handle;
	params->read.single.offset = 0U;

	return bt_gatt_read(conn, &params->read);
}

static uint8_t unicast_client_pacs_location_discover_cb(struct bt_conn *conn,
							const struct bt_gatt_attr *attr,
							struct bt_gatt_discover_params *discover)
{
	struct bt_audio_discover_params *params;
	uint8_t index = bt_conn_index(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	params = CONTAINER_OF(discover, struct bt_audio_discover_params,
			      discover);

	if (!attr) {
		/* If location is not found, we just continue reading the
		 * available contexts, as location is optional.
		 */
		if (unicast_client_pacs_avail_ctx_discover(conn, params) < 0) {
			LOG_ERR("Unable to read available contexts");

			params->func(conn, NULL, NULL, params);
		}

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	LOG_DBG("conn %p attr %p handle 0x%04x", conn, attr, chrc->value_handle);

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		struct bt_gatt_subscribe_params *sub_params;

		if (params->dir == BT_AUDIO_DIR_SINK) {
			sub_params = &snk_loc_subscribe[index];
		} else {
			sub_params = &src_loc_subscribe[index];
		}

		sub_params->value_handle = chrc->value_handle;
		sub_params->ccc_handle = 0x0000; /* auto discover ccc */
		sub_params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		sub_params->disc_params = &loc_cc_disc[index];
		sub_params->notify = unicast_client_pacs_location_notify_cb;
		sub_params->value = BT_GATT_CCC_NOTIFY;

		err = bt_gatt_subscribe(conn, sub_params);
		if (err != 0 && err != -EALREADY) {
			LOG_ERR("Failed to subscribe to location: %d", err);
		}
	}

	err = unicast_client_pacs_location_read(conn, params, chrc->value_handle);
	if (err != 0) {
		LOG_DBG("Failed to read PACS location: %d", err);
		params->err = err;

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int unicast_client_pacs_location_discover(struct bt_conn *conn,
						 struct bt_audio_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

	if (params->dir == BT_AUDIO_DIR_SINK) {
		params->discover.uuid = pacs_snk_loc_uuid;
	} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
		params->discover.uuid = pacs_src_loc_uuid;
	} else {
		return -EINVAL;
	}

	params->discover.func = unicast_client_pacs_location_discover_cb;
	params->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(conn, &params->discover);
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

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || length < sizeof(uint16_t) * 2) {
		goto discover_loc;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);
	context = net_buf_simple_pull_mem(&buf, sizeof(*context));

	index = bt_conn_index(conn);

	for (i = 0; i < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT; i++) {
		struct unicast_client_pac *pac = &pac_cache[index][i];

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
			LOG_WRN("Cached pac with invalid dir: %u", pac->dir);
		}

		LOG_DBG("pac %p context 0x%04x", pac, pac->context);
	}

discover_loc:
	/* Read ASE instances */
	if (unicast_client_pacs_location_discover(conn, params) < 0) {
		LOG_ERR("Unable to read PACS location");

		params->func(conn, NULL, NULL, params);
	}

	return BT_GATT_ITER_STOP;
}

static int unicast_client_pacs_context_discover(struct bt_conn *conn,
						struct bt_audio_discover_params *params)
{
	LOG_DBG("conn %p params %p", conn, params);

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
		struct unicast_client_pac *pac = &pac_cache[index][i];

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

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || !data) {
		params->err = err;
		goto fail;
	}

	LOG_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	if (buf.len < sizeof(*rsp)) {
		LOG_ERR("Read response too small");
		goto fail;
	}

	rsp = net_buf_simple_pull_mem(&buf, sizeof(*rsp));

	/* If no PAC was found don't bother discovering ASE and ASE CP */
	if (!rsp->num_pac) {
		goto fail;
	}

	while (rsp->num_pac) {
		struct unicast_client_pac *bpac;
		struct bt_pac_codec *pac_codec;
		struct bt_pac_ltv_data *meta, *cc;
		void *cc_ltv, *meta_ltv;

		LOG_DBG("pac #%u", params->num_caps);

		if (buf.len < sizeof(*pac_codec)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*pac_codec));
			break;
		}

		pac_codec = net_buf_simple_pull_mem(&buf, sizeof(*pac_codec));

		if (buf.len < sizeof(*cc)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*cc));
			break;
		}

		cc = net_buf_simple_pull_mem(&buf, sizeof(*cc));
		if (buf.len < cc->len) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len, cc->len);
			break;
		}

		cc_ltv = net_buf_simple_pull_mem(&buf, cc->len);

		if (buf.len < sizeof(*meta)) {
			LOG_ERR("Malformed PAC: remaining len %u expected %zu", buf.len,
				sizeof(*meta));
			break;
		}

		meta = net_buf_simple_pull_mem(&buf, sizeof(*meta));
		if (buf.len < meta->len) {
			LOG_ERR("Malformed PAC: remaining len %u expected %u", buf.len, meta->len);
			break;
		}

		meta_ltv = net_buf_simple_pull_mem(&buf, meta->len);

		bpac = unicast_client_pac_alloc(conn, params->dir);
		if (!bpac) {
			LOG_WRN("No space left to parse PAC");
			break;
		}

		if (unicast_client_ep_set_codec(NULL, pac_codec->id,
						sys_le16_to_cpu(pac_codec->cid),
						sys_le16_to_cpu(pac_codec->vid),
						cc_ltv, cc->len,
						&bpac->codec)) {
			LOG_ERR("Unable to parse Codec");
			break;
		}

		if (unicast_client_ep_set_metadata(NULL, meta_ltv, meta->len, &bpac->codec)) {
			LOG_ERR("Unable to parse Codec Metadata");
			break;
		}

		LOG_DBG("pac %p codec 0x%02x config count %u meta count %u ", bpac, bpac->codec.id,
			bpac->codec.data_count, bpac->codec.meta_count);

		params->func(conn, &bpac->codec, NULL, params);

		rsp->num_pac--;
		params->num_caps++;
	}

	if (!params->num_caps) {
		goto fail;
	}

	/* Read PACS contexts */
	if (unicast_client_pacs_context_discover(conn, params) < 0) {
		LOG_ERR("Unable to read PACS context");
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	params->func(conn, NULL, NULL, params);
	return BT_GATT_ITER_STOP;
}

static void unicast_client_pac_reset(struct bt_conn *conn,
				     enum bt_audio_dir dir)
{
	int index = bt_conn_index(conn);
	int i;

	for (i = 0; i < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT; i++) {
		struct unicast_client_pac *pac = &pac_cache[index][i];

		if (pac->dir == dir) {
			(void)memset(pac, 0, sizeof(*pac));
		}
	}
}

static void unicast_client_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("conn %p reason 0x%02x", conn, reason);

	unicast_client_ep_reset(conn);
	unicast_client_pac_reset(conn, BT_AUDIO_DIR_SINK);
	unicast_client_pac_reset(conn, BT_AUDIO_DIR_SOURCE);
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
		LOG_DBG("Invalid conn role: %u, shall be central", role);
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
	params->read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	if (!conn_cb_registered) {
		bt_conn_cb_register(&conn_cbs);
		conn_cb_registered = true;
	}

	/* Reset existing data for the specified type */
	unicast_client_pac_reset(conn, params->dir);

	params->num_caps = 0u;
	params->num_eps = 0u;

	return bt_gatt_read(conn, &params->read);
}


int bt_audio_unicast_client_register_cb(const struct bt_audio_unicast_client_cb *cbs)
{
	CHECKIF(cbs == NULL) {
		LOG_DBG("cbs is NULL");
		return -EINVAL;
	}

	if (unicast_client_cbs != NULL) {
		LOG_DBG("Callbacks already registered");
		return -EALREADY;
	}

	unicast_client_cbs = cbs;

	return 0;
}
