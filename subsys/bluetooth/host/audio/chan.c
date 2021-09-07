/*  Bluetooth Audio Channel */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>

#include "../conn_internal.h"
#include "../iso_internal.h"

#include "endpoint.h"
#include "chan.h"
#include "capabilities.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_CHAN)
#define LOG_MODULE_NAME bt_audio_chan
#include "common/log.h"

#if defined(CONFIG_BT_BAP)

#define PA_SYNC_SKIP              5
#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define BASE_MIN_SIZE             17
#define BASE_BIS_DATA_MIN_SIZE    2 /* index and length */
#define BROADCAST_SYNC_MIN_INDEX  (BIT(1))

/* internal bt_audio_base_ad* structs are primarily designed to help
 * calculate the maximum advertising data length
 */
struct bt_audio_base_ad_bis_specific_data {
	uint8_t index;
	uint8_t codec_config_len; /* currently unused and shall always be 0 */
} __packed;

struct bt_audio_base_ad_codec_data {
	uint8_t type;
	uint8_t data_len;
	uint8_t data[CONFIG_BT_CODEC_MAX_DATA_LEN];
} __packed;

struct bt_audio_base_ad_codec_metadata {
	uint8_t type;
	uint8_t data_len;
	uint8_t data[CONFIG_BT_CODEC_MAX_METADATA_LEN];
} __packed;

struct bt_audio_base_ad_subgroup {
	uint8_t bis_cnt;
	uint8_t codec_id;
	uint16_t company_id;
	uint16_t vendor_id;
	uint8_t codec_config_len;
	struct bt_audio_base_ad_codec_data codec_config[CONFIG_BT_CODEC_MAX_DATA_COUNT];
	uint8_t metadata_len;
	struct bt_audio_base_ad_codec_metadata metadata[CONFIG_BT_CODEC_MAX_METADATA_COUNT];
	struct bt_audio_base_ad_bis_specific_data bis_data[BROADCAST_STREAM_CNT];
} __packed;

struct bt_audio_base_ad {
	uint16_t uuid_val;
	struct bt_audio_base_ad_subgroup subgroups[BROADCAST_SUBGROUP_CNT];
} __packed;

static struct bt_iso_cig *cigs[CONNECTED_AUDIO_GROUP_COUNT];
static struct bt_audio_chan *enabling[CONFIG_BT_ISO_MAX_CHAN];
#if defined(CONFIG_BT_AUDIO_BROADCAST)
static struct bt_audio_broadcast_source broadcast_sources[BROADCAST_SRC_CNT];
static struct bt_audio_broadcast_sink broadcast_sinks[BROADCAST_SNK_CNT];
static struct bt_le_scan_cb broadcast_scan_cb;

static int bt_audio_set_base(const struct bt_audio_broadcast_source *source,
			     struct bt_codec *codec);
#endif /* CONFIG_BT_AUDIO_BROADCAST */

static void chan_attach(struct bt_conn *conn, struct bt_audio_chan *chan,
			struct bt_audio_ep *ep, struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	BT_DBG("conn %p chan %p ep %p cap %p codec %p", conn, chan, ep, cap,
	       codec);

	chan->conn = conn;
	chan->cap = cap;
	chan->codec = codec;

	bt_audio_ep_attach(ep, chan);
}

struct bt_audio_chan *bt_audio_chan_config(struct bt_conn *conn,
					   struct bt_audio_ep *ep,
					   struct bt_audio_capability *cap,
					   struct bt_codec *codec)
{
	struct bt_audio_chan *chan;

	BT_DBG("conn %p ep %p cap %p codec %p codec id 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", conn, ep, cap, codec, codec ? codec->id : 0,
	       codec ? codec->cid : 0, codec ? codec->vid : 0);

	if (!conn || !cap || !cap->ops || !codec) {
		return NULL;
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
		return NULL;
	}

	/* Check that codec and frequency are supported */
	if (cap->codec->id != codec->id) {
		BT_ERR("Invalid codec id");
		return NULL;
	}

	if (!cap->ops->config) {
		return NULL;
	}

	chan = cap->ops->config(conn, ep, cap, codec);
	if (!chan) {
		return chan;
	}

	sys_slist_init(&chan->links);

	chan_attach(conn, chan, ep, cap, codec);

	if (ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(ep, BT_ASCS_ASE_STATE_CONFIG);
	}

	return chan;
}

int bt_audio_chan_reconfig(struct bt_audio_chan *chan,
			   struct bt_audio_capability *cap,
			   struct bt_codec *codec)
{
	BT_DBG("chan %p cap %p codec %p", chan, cap, codec);

	if (!chan || !chan->ep) {
		BT_DBG("Invalid channel or endpoint");
		return -EINVAL;
	}

	if (codec == NULL) {
		BT_DBG("NULL codec");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast_src(chan->ep)) {
		BT_DBG("Cannot use %s to reconfigure broadcast source channels",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(chan->ep)) {
		BT_DBG("Cannot reconfigure broadcast sink channels");
		return -EINVAL;
	}

	if (!chan->cap || !chan->cap->ops) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_ASCS_ASE_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_ASCS_ASE_STATE_CONFIG:
	 /* or 0x02 (QoS Configured) */
	case BT_ASCS_ASE_STATE_QOS:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	/* Check that codec is supported */
	if (cap->codec->id != codec->id) {
		return -ENOTSUP;
	}

	if (cap->ops->reconfig) {
		int err = cap->ops->reconfig(chan, cap, codec);
		if (err) {
			return err;
		}
	}

	chan_attach(chan->conn, chan, chan->ep, cap, codec);

	if (chan->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_CONFIG);
	}

	return 0;
}

#define IN_RANGE(_min, _max, _value) \
	(_value >= _min && _value <= _max)

static bool bt_audio_chan_enabling(struct bt_audio_chan *chan)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == chan) {
			return true;
		}
	}

	return false;
}

static int bt_audio_chan_iso_accept(struct bt_conn *conn,
				    struct bt_iso_chan **chan)
{
	int i;

	BT_DBG("conn %p", conn);

	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		struct bt_audio_chan *c = enabling[i];

		if (c && c->ep->cig == conn->iso.cig_id &&
		    c->ep->cis == conn->iso.cis_id) {
			*chan = enabling[i]->iso;
			enabling[i] = NULL;
			return 0;
		}
	}

	BT_ERR("No channel listening");

	return -EPERM;
}

static struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L2,
	.accept = bt_audio_chan_iso_accept,
};

static bool bt_audio_chan_linked(struct bt_audio_chan *chan1,
				 struct bt_audio_chan *chan2)
{
	struct bt_audio_chan *tmp;

	if (!chan1 || !chan2) {
		return false;
	}

	if (chan1 == chan2) {
		return true;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&chan1->links, tmp, node) {
		if (tmp == chan2) {
			return true;
		}
	}

	return false;
}

static bool bt_audio_chan_iso_linked(struct bt_audio_chan *chan1,
				     struct bt_audio_chan *chan2)
{
	if (!chan1 || !chan2 || chan1->conn != chan2->conn) {
		return false;
	}

	return (chan1->ep->cig == chan2->ep->cig) &&
	       (chan1->ep->cis == chan2->ep->cis);
}

static int bt_audio_chan_iso_listen(struct bt_audio_chan *chan)
{
	static bool server;
	int err, i;
	struct bt_audio_chan **free = NULL;

	BT_DBG("chan %p conn %p", chan, chan->conn);

	if (server) {
		goto done;
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		BT_ERR("bt_iso_server_register: %d", err);
		return err;
	}

	server = true;

done:
	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == chan) {
			return 0;
		}

		if (bt_audio_chan_iso_linked(enabling[i], chan)) {
			bt_audio_chan_link(enabling[i], chan);
			return 0;
		}

		if (!enabling[i] && !free) {
			free = &enabling[i];
		}
	}

	if (free) {
		*free = chan;
		return 0;
	}

	BT_ERR("Unable to listen: no slot left");

	return -ENOSPC;
}

int bt_audio_chan_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	int err;

	BT_DBG("chan %p qos %p", chan, qos);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops || !qos) {
		return -EINVAL;
	}

	CHECKIF(bt_audio_ep_is_broadcast(chan->ep)) {
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_ASCS_ASE_STATE_CONFIG:
	/* or 0x02 (QoS Configured) */
	case BT_ASCS_ASE_STATE_QOS:
		break;
	default:
		BT_ERR("Invalid state: %s",
		bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	/* Allowed Range: 0x0000FF–0xFFFFFF */
	if (qos->interval < 0x0000ff || qos->interval > 0xffffff) {
		BT_ERR("Interval not within allowed range: %u (%u-%u)",
		       qos->interval, 0x0000ff, 0xffffff);
		qos->interval = 0u;
		return -ENOTSUP;
	}

	/* Allowed values: Unframed and Framed */
	if (qos->framing > BT_CODEC_QOS_FRAMED) {
		BT_ERR("Invalid Framing 0x%02x", qos->framing);
		qos->framing = 0xff;
		return -ENOTSUP;
	}

	/* Allowed values: 1M, 2M or Coded */
	if (!qos->phy || qos->phy >
	    (BT_CODEC_QOS_1M | BT_CODEC_QOS_2M | BT_CODEC_QOS_CODED)) {
		BT_ERR("Invalid PHY 0x%02x", qos->phy);
		qos->phy = 0x00;
		return -ENOTSUP;
	}

	/* Allowed Range: 0x00–0x0FFF */
	if (qos->sdu > 0x0fff) {
		BT_ERR("Invalid SDU %u", qos->sdu);
		qos->sdu = 0xffff;
		return -ENOTSUP;
	}

	/* Allowed Range: 0x0005–0x0FA0 */
	if (qos->latency < 0x0005 || qos->latency > 0x0fa0) {
		BT_ERR("Invalid Latency %u", qos->latency);
		qos->latency = 0u;
		return -ENOTSUP;
	}

	if (chan->cap->qos.latency < qos->latency) {
		BT_ERR("Latency not within range: max %u latency %u",
		       chan->cap->qos.latency, qos->latency);
		qos->latency = 0u;
		return -ENOTSUP;
	}

	if (!IN_RANGE(chan->cap->qos.pd_min, chan->cap->qos.pd_max,
		      qos->pd)) {
		BT_ERR("Presentation Delay not within range: min %u max %u "
		       "pd %u", chan->cap->qos.pd_min, chan->cap->qos.pd_max,
		       qos->pd);
		qos->pd = 0u;
		return -ENOTSUP;
	}

	if (!chan->cap->ops->qos) {
		return 0;
	}

	err = chan->cap->ops->qos(chan, qos);
	if (err) {
		return err;
	}

	chan->qos = qos;

	if (chan->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_QOS);
		bt_audio_chan_iso_listen(chan);
	}

	return 0;
}

int bt_audio_chan_enable(struct bt_audio_chan *chan,
			 uint8_t meta_count, struct bt_codec_data *meta)
{
	int err;

	BT_DBG("chan %p", chan);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
		return -EINVAL;
	}

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (chan->ep->status.state != BT_ASCS_ASE_STATE_QOS) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->enable) {
		goto done;
	}

	err = chan->cap->ops->enable(chan, meta_count, meta);
	if (err) {
		return err;
	}

done:
	if (chan->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_ENABLING);

	if (bt_audio_chan_enabling(chan)) {
		return 0;
	}

	if (chan->cap->type == BT_AUDIO_SOURCE) {
		return 0;
	}

	/* After an ASE has been enabled, the Unicast Server acting as an Audio
	 * Sink for that ASE shall autonomously initiate the Handshake
	 * operation to transition the ASE to the Streaming state when the
	 * Unicast Server is ready to consume audio data transmitted by the
	 * Unicast Client.
	 */
	return bt_audio_chan_start(chan);
}

int bt_audio_chan_metadata(struct bt_audio_chan *chan,
			   uint8_t meta_count, struct bt_codec_data *meta)
{
	int err;

	BT_DBG("chan %p metadata count %u", chan, meta_count);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_ASCS_ASE_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_ASCS_ASE_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->metadata) {
		goto done;
	}

	err = chan->cap->ops->metadata(chan, meta_count, meta);
	if (err) {
		return err;
	}

done:
	if (chan->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	/* Set the state to the same state to trigger the notifications */
	bt_audio_ep_set_state(chan->ep, chan->ep->status.state);

	return 0;
}

int bt_audio_chan_disable(struct bt_audio_chan *chan)
{
	int err;

	BT_DBG("chan %p", chan);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_ASCS_ASE_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_ASCS_ASE_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->disable) {
		err = 0;
		goto done;
	}

	err = chan->cap->ops->disable(chan);
	if (err) {
		return err;
	}

done:
	if (chan->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_DISABLING);

	if (chan->cap->type == BT_AUDIO_SOURCE) {
		return 0;
	}

	/* If an ASE is in the Disabling state, and if the Unicast Server is in
	 * the Audio Sink role, the Unicast Server shall autonomously initiate
	 * the Receiver Stop Ready operation when the Unicast Server is ready
	 * to stop consuming audio data transmitted for that ASE by the Unicast
	 * Client. The Unicast Client in the Audio Source role should not stop
	 * transmitting audio data until the Unicast Server transitions the ASE
	 * to the QoS Configured state.
	 */
	return bt_audio_chan_stop(chan);
}

int bt_audio_chan_start(struct bt_audio_chan *chan)
{
	int err;

	BT_DBG("chan %p", chan);

	if (!chan || !chan->ep) {
		BT_DBG("Invalid channel or endpoint");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast_src(chan->ep)) {
		BT_DBG("Cannot use %s to start broadcast source channels",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(chan->ep)) {
		BT_DBG("Cannot start broadcast sink channels");
		return -EINVAL;
	}

	if (!chan->cap || !chan->cap->ops) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_ASCS_ASE_STATE_ENABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->start) {
		err = 0;
		goto done;
	}

	err = chan->cap->ops->start(chan);
	if (err) {
		return err;
	}

done:
	if (chan->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_STREAMING);
	}

	return err;
}

int bt_audio_chan_stop(struct bt_audio_chan *chan)
{
	struct bt_audio_ep *ep;
	int err;

	if (!chan || !chan->ep) {
		BT_DBG("Invalid channel or endpoint");
		return -EINVAL;
	}

	ep = chan->ep;

	if (bt_audio_ep_is_broadcast_src(chan->ep)) {
		BT_DBG("Cannot use %s to stop broadcast source channels",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(chan->ep)) {
		BT_DBG("Cannot use %s to stop broadcast sink channels",
		       __func__);
		return -EINVAL;
	}

	if (!chan->cap || !chan->cap->ops) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Disabling) */
	case BT_ASCS_ASE_STATE_DISABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->stop) {
		err = 0;
		goto done;
	}

	err = chan->cap->ops->stop(chan);
	if (err) {
		return err;
	}

done:
	if (ep->type != BT_AUDIO_EP_LOCAL) {
		return err;
	}

	/* If the Receiver Stop Ready operation has completed successfully the
	 * Unicast Client or the Unicast Server may terminate a CIS established
	 * for that ASE by following the Connected Isochronous Stream Terminate
	 * procedure defined in Volume 3, Part C, Section 9.3.15.
	 */
	if (!bt_audio_chan_disconnect(chan)) {
		return err;
	}

	bt_audio_ep_set_state(ep, BT_ASCS_ASE_STATE_QOS);
	bt_audio_chan_iso_listen(chan);

	return err;
}

int bt_audio_chan_release(struct bt_audio_chan *chan, bool cache)
{
	int err;

	BT_DBG("chan %p cache %s", chan, cache ? "true" : "false");

	CHECKIF(!chan || !chan->ep) {
		BT_DBG("Invalid channel");
		return -EINVAL;
	}

	if (chan->state == BT_AUDIO_CHAN_IDLE) {
		BT_DBG("Audio channel is idle");
		return -EALREADY;
	}

	if (bt_audio_ep_is_broadcast_src(chan->ep)) {
		BT_DBG("Cannot release a broadcast source");
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(chan->ep)) {
		BT_DBG("Cannot release a broadcast sink");
		return -EINVAL;
	}

	CHECKIF(!chan->cap || !chan->cap->ops) {
		BT_DBG("Capability or capability ops is NULL");
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
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
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
	}

	if (!chan->cap->ops->release) {
		err = 0;
		goto done;
	}

	err = chan->cap->ops->release(chan);
	if (err) {
		if (err == -ENOTCONN) {
			bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_IDLE);
			return 0;
		}
		return err;
	}

done:
	if (chan->ep->type != BT_AUDIO_EP_LOCAL) {
		return err;
	}

	/* Any previously applied codec configuration may be cached by the
	 * server.
	 */
	if (!cache) {
		bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_RELEASING);
	} else {
		bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_CONFIG);
	}

	return err;
}

int bt_audio_chan_link(struct bt_audio_chan *chan1, struct bt_audio_chan *chan2)
{
	BT_DBG("chan1 %p chan2 %p", chan1, chan2);

	if (!chan1 || !chan2) {
		return -EINVAL;
	}

	if (chan1->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("chan1 %p is not idle", chan1);
		return -EINVAL;
	}

	if (chan2->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("chan2 %p is not idle", chan2);
		return -EINVAL;
	}

	if (bt_audio_chan_linked(chan1, chan2)) {
		return -EALREADY;
	}

	sys_slist_append(&chan1->links, &chan2->node);
	sys_slist_append(&chan2->links, &chan1->node);

	return 0;
}

int bt_audio_chan_unlink(struct bt_audio_chan *chan1,
			 struct bt_audio_chan *chan2)
{
	BT_DBG("chan1 %p chan2 %p", chan1, chan2);

	if (!chan1) {
		return -EINVAL;
	}

	if (chan1->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("chan1 %p is not idle", chan1);
		return -EINVAL;
	}

	/* Unbind all channels if chan2 is NULL */
	if (!chan2) {
		SYS_SLIST_FOR_EACH_CONTAINER(&chan1->links, chan2, node) {
			int err;

			err = bt_audio_chan_unlink(chan1, chan2);
			if (err) {
				return err;
			}
		}
	} else if (chan2->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("chan2 %p is not idle", chan2);
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&chan1->links, &chan2->node)) {
		return -ENOENT;
	}

	if (!sys_slist_find_and_remove(&chan2->links, &chan1->node)) {
		return -ENOENT;
	}

	return 0;
}

static void chan_detach(struct bt_audio_chan *chan)
{
	const bool is_broadcast = bt_audio_ep_is_broadcast(chan->ep);

	BT_DBG("chan %p", chan);

	bt_audio_ep_detach(chan->ep, chan);

	chan->conn = NULL;
	chan->cap = NULL;
	chan->codec = NULL;

	if (!is_broadcast) {
		bt_audio_chan_disconnect(chan);
	}
}

#if defined(CONFIG_BT_AUDIO_DEBUG_CHAN)
const char *bt_audio_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_AUDIO_CHAN_IDLE:
		return "idle";
	case BT_AUDIO_CHAN_CONFIGURED:
		return "configured";
	case BT_AUDIO_CHAN_STREAMING:
		return "streaming";
	default:
		return "unknown";
	}
}

void bt_audio_chan_set_state_debug(struct bt_audio_chan *chan, uint8_t state,
				   const char *func, int line)
{
	BT_DBG("chan %p %s -> %s", chan,
	       bt_audio_chan_state_str(chan->state),
	       bt_audio_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_AUDIO_CHAN_IDLE:
		/* regardless of old state always allows this state */
		break;
	case BT_AUDIO_CHAN_CONFIGURED:
		/* regardless of old state always allows this state */
		break;
	case BT_AUDIO_CHAN_STREAMING:
		if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		BT_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	if (state == BT_AUDIO_CHAN_IDLE) {
		chan_detach(chan);
	}

	chan->state = state;
}
#else
void bt_audio_chan_set_state(struct bt_audio_chan *chan, uint8_t state)
{
	if (state == BT_AUDIO_CHAN_IDLE) {
		chan_detach(chan);
	}

	chan->state = state;
}
#endif /* CONFIG_BT_AUDIO_DEBUG_CHAN */

static int codec_qos_to_iso_qos(struct bt_iso_chan_qos *qos,
				struct bt_codec_qos *codec)
{
	struct bt_iso_chan_io_qos *io;

	switch (codec->dir) {
	case BT_CODEC_QOS_IN:
		io = qos->rx;
		break;
	case BT_CODEC_QOS_OUT:
		io = qos->tx;
		break;
	case BT_CODEC_QOS_INOUT:
		io = qos->rx = qos->tx;
		break;
	default:
		return -EINVAL;
	}

	io->sdu = codec->sdu;
	io->phy = codec->phy;
	io->rtn = codec->rtn;

	return 0;
}

struct bt_conn_iso *bt_audio_cig_create(struct bt_audio_chan *chan,
					struct bt_codec_qos *qos)
{
	BT_DBG("chan %p iso %p qos %p", chan, chan->iso, qos);

	if (!chan->iso) {
		BT_ERR("Unable to bind: ISO channel not set");
		return NULL;
	}

	if (!qos || !chan->iso->qos) {
		BT_ERR("Unable to bind: QoS not set");
		return NULL;
	}

	/* Fill up ISO QoS settings from Codec QoS*/
	if (chan->qos != qos) {
		int err = codec_qos_to_iso_qos(chan->iso->qos, qos);

		if (err) {
			BT_ERR("Unable to convert codec QoS to ISO QoS");
			return NULL;
		}
	}

	if (!chan->iso->iso) {
		struct bt_iso_cig_create_param param;
		struct bt_iso_cig **free_cig;
		int err;

		free_cig = NULL;
		for (int i = 0; i < ARRAY_SIZE(cigs); i++) {
			if (cigs[i] == NULL) {
				free_cig = &cigs[i];
				break;
			}
		}

		if (free_cig == NULL) {
			return NULL;
		}

		/* TODO: Support more than a single CIS in a CIG */
		param.num_cis = 1;
		param.cis_channels = &chan->iso;
		param.framing = qos->framing;
		param.packing = 0; /*  TODO: Add to QoS struct */
		param.interval = qos->interval;
		param.latency = qos->latency;
		param.sca = BT_GAP_SCA_UNKNOWN;

		err = bt_iso_cig_create(&param, free_cig);
		if (err != 0) {
			BT_ERR("bt_iso_cig_create failed: %d", err);
			return NULL;
		}
	}

	return &chan->iso->iso->iso;
}

int bt_audio_cig_terminate(struct bt_audio_chan *chan)
{
	BT_DBG("chan %p", chan);

	if (chan->iso == NULL) {
		BT_DBG("Channel not bound");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(cigs); i++) {
		struct bt_iso_cig *cig;

		cig = cigs[i];
		if (cig != NULL &&
		    cig->cis != NULL &&
		    cig->cis[0] == chan->iso) {
			return bt_iso_cig_terminate(cig);
		}
	}

	BT_DBG("CIG not found for chan %p", chan);
	return -EINVAL;
}

int bt_audio_chan_connect(struct bt_audio_chan *chan)
{
	struct bt_iso_connect_param param;

	BT_DBG("chan %p iso %p", chan, chan ? chan->iso : NULL);

	if (!chan || !chan->iso) {
		return -EINVAL;
	}

	param.acl = chan->conn;
	param.iso_chan = chan->iso;

	switch (chan->iso->state) {
	case BT_ISO_DISCONNECTED:
		if (!bt_audio_cig_create(chan, chan->qos)) {
			return -ENOTCONN;
		}

		return bt_iso_chan_connect(&param, 1);
	case BT_ISO_CONNECT:
		return 0;
	case BT_ISO_CONNECTED:
		return -EALREADY;
	default:
		return bt_iso_chan_connect(&param, 1);
	}
}

int bt_audio_chan_disconnect(struct bt_audio_chan *chan)
{
	int i;

	BT_DBG("chan %p iso %p", chan, chan->iso);

	if (!chan) {
		return -EINVAL;
	}

	/* Stop listening */
	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == chan) {
			enabling[i] = NULL;
		}
	}

	if (!chan->iso || !chan->iso->iso) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(chan->iso);
}

void bt_audio_chan_reset(struct bt_audio_chan *chan)
{
	int err;

	BT_DBG("chan %p", chan);

	if (!chan || !chan->conn) {
		return;
	}

	err = bt_audio_cig_terminate(chan);
	if (err != 0) {
		BT_ERR("Failed to terminate CIG: %d", err);
	}
	bt_audio_chan_unlink(chan, NULL);
	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_IDLE);
}

int bt_audio_chan_send(struct bt_audio_chan *chan, struct net_buf *buf)
{
	if (!chan || !chan->ep) {
		return -EINVAL;
	}

	if (chan->state != BT_AUDIO_CHAN_STREAMING) {
		BT_DBG("Channel not ready for streaming");
		return -EBADMSG;
	}

	if (bt_audio_ep_is_broadcast_snk(chan->ep)) {
		BT_DBG("Cannot send on a broadcast sink channel");
		return -EINVAL;
	} else if (!bt_audio_ep_is_broadcast_src(chan->ep)) {
		switch (chan->ep->status.state) {
		/* or 0x04 (Streaming) */
		case BT_ASCS_ASE_STATE_STREAMING:
			break;
		default:
			BT_ERR("Invalid state: %s",
			bt_audio_ep_state_str(chan->ep->status.state));
			return -EBADMSG;
		}
	}

	return bt_iso_chan_send(chan->iso, buf);
}

void bt_audio_chan_cb_register(struct bt_audio_chan *chan, struct bt_audio_chan_ops *ops)
{
	chan->ops = ops;
}

#if defined(CONFIG_BT_AUDIO_BROADCAST)
static int bt_audio_broadcast_source_setup_chan(uint8_t index,
						struct bt_audio_chan *chan,
						struct bt_codec *codec,
						struct bt_codec_qos *qos)
{
	struct bt_audio_ep *ep;
	int err;

	if (chan->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("Channel %p not idle", chan);
		return -EALREADY;
	}

	ep = bt_audio_ep_broadcaster_new(index, BT_AUDIO_SOURCE);
	if (ep == NULL) {
		BT_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	chan_attach(NULL, chan, ep, NULL, codec);
	chan->qos = qos;
	err = codec_qos_to_iso_qos(chan->iso->qos, qos);
	if (err) {
		BT_ERR("Unable to convert codec QoS to ISO QoS");
		return err;
	}

	return 0;
}

static void bt_audio_encode_base(const struct bt_audio_broadcast_source *source,
				 struct bt_codec *codec,
				 struct net_buf_simple *buf)
{
	uint8_t bis_index;
	uint8_t *start;
	uint8_t len;

	__ASSERT(source->subgroup_count == BROADCAST_SUBGROUP_CNT,
		 "Cannot encode BASE with more than a single subgroup");

	net_buf_simple_add_le16(buf, BT_UUID_BASIC_AUDIO_VAL);
	net_buf_simple_add_le24(buf, source->pd);
	net_buf_simple_add_u8(buf, source->subgroup_count);
	/* TODO: The following encoding should be done for each subgroup once
	 * supported
	 */
	net_buf_simple_add_u8(buf, source->chan_count);
	net_buf_simple_add_u8(buf, codec->id);
	net_buf_simple_add_le16(buf, codec->cid);
	net_buf_simple_add_le16(buf, codec->vid);

	/* Insert codec configuration data in LTV format */
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->data_count; i++) {
		const struct bt_data *codec_data = &codec->data[i].data;

		net_buf_simple_add_u8(buf, codec_data->data_len);
		net_buf_simple_add_u8(buf, codec_data->type);
		net_buf_simple_add_mem(buf, codec_data->data,
				       codec_data->data_len -
					sizeof(codec_data->type));

	}
	/* Calcute length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Insert codec metadata in LTV format*/
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->meta_count; i++) {
		const struct bt_data *metadata = &codec->meta[i].data;

		net_buf_simple_add_u8(buf, metadata->data_len);
		net_buf_simple_add_u8(buf, metadata->type);
		net_buf_simple_add_mem(buf, metadata->data,
				       metadata->data_len -
					sizeof(metadata->type));
	}
	/* Calcute length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Create BIS index bitfield */
	bis_index = 0;
	for (int i = 0; i < source->chan_count; i++) {
		bis_index++;
		net_buf_simple_add_u8(buf, bis_index);
		net_buf_simple_add_u8(buf, 0); /* unused length field */
	}

	/* NOTE: It is also possible to have the codec configuration data per
	 * BIS index. As our API does not support such specialized BISes we
	 * currently don't do that.
	 */
}


static int generate_broadcast_id(struct bt_audio_broadcast_source *source)
{
	bool unique;

	do {
		int err;

		err = bt_rand(&source->broadcast_id, BT_BROADCAST_ID_SIZE);
		if (err) {
			return err;
		}

		/* Ensure uniqueness */
		unique = true;
		for (int i = 0; i < ARRAY_SIZE(broadcast_sources); i++) {
			if (&broadcast_sources[i] == source) {
				continue;
			}

			if (broadcast_sources[i].broadcast_id == source->broadcast_id) {
				unique = false;
				break;
			}
		}
	} while (!unique);

	return 0;
}

static int bt_audio_set_base(const struct bt_audio_broadcast_source *source,
			     struct bt_codec *codec)
{
	struct bt_data base_ad_data;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(base_buf, sizeof(struct bt_audio_base_ad));

	bt_audio_encode_base(source, codec, &base_buf);

	base_ad_data.type = BT_DATA_SVC_DATA16;
	base_ad_data.data_len = base_buf.len;
	base_ad_data.data = base_buf.data;

	err = bt_le_per_adv_set_data(source->adv, &base_ad_data, 1);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		return err;
	}

	return 0;
}

static void broadcast_source_cleanup(struct bt_audio_broadcast_source *source)
{
	for (size_t i = 0; i < source->chan_count; i++) {
		struct bt_audio_chan *chan = &source->chans[i];

		chan->ep->chan = NULL;
		chan->ep = NULL;
		chan->codec = NULL;
		chan->qos = NULL;
		chan->iso = NULL;
		chan->state = BT_AUDIO_CHAN_IDLE;
	}

	(void)memset(source, 0, sizeof(*source));
}

int bt_audio_broadcast_source_create(struct bt_audio_chan *chans,
				     uint8_t num_chan,
				     struct bt_codec *codec,
				     struct bt_codec_qos *qos,
				     struct bt_audio_broadcast_source **out_source)
{
	struct bt_audio_broadcast_source *source;
	struct bt_data ad;
	uint8_t index;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_BROADCAST_ID_SIZE);

	/* TODO: Validate codec and qos values */

	/* TODO: The API currently only requires a bt_audio_chan object from
	 * the user. We could modify the API such that the extended (and
	 * periodic advertising enabled) advertiser was provided by the user as
	 * well (similar to the ISO API), or even provide the BIG.
	 *
	 * The caveat of that type of API, instead of this, where we, the stack,
	 * control the advertiser, is that the user will be able to change the
	 * advertising data (thus making the broadcast source non-functional in
	 * terms of BAP compliance), or even stop the advertiser without
	 * stopping the BIG (which also goes against the BAP specification).
	 */

	CHECKIF(out_source == NULL) {
		BT_DBG("out_source is NULL");
		return -EINVAL;
	}
	/* Set out_source to NULL until the source has actually been created */
	*out_source = NULL;

	CHECKIF(chans == NULL) {
		BT_DBG("chans is NULL");
		return -EINVAL;
	}

	CHECKIF(codec == NULL) {
		BT_DBG("codec is NULL");
		return -EINVAL;
	}

	CHECKIF(num_chan > BROADCAST_STREAM_CNT) {
		BT_DBG("Too many channels provided: %u/%u",
		       num_chan, BROADCAST_STREAM_CNT);
		return -EINVAL;
	}

	source = NULL;
	for (index = 0; index < ARRAY_SIZE(broadcast_sources); index++) {
		if (broadcast_sources[index].bis[0] == NULL) { /* Find free entry */
			source = &broadcast_sources[index];
			break;
		}
	}

	if (source == NULL) {
		BT_DBG("Could not allocate any more broadcast sources");
		return -ENOMEM;
	}

	source->chans = chans;
	source->chan_count = num_chan;
	for (size_t i = 0; i < num_chan; i++) {
		struct bt_audio_chan *chan = &chans[i];

		err = bt_audio_broadcast_source_setup_chan(index, chan, codec, qos);
		if (err != 0) {
			BT_DBG("Failed to setup chans[%zu]: %d", i, err);
			broadcast_source_cleanup(source);
			return err;
		}

		source->bis[i] = &chan->ep->iso;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL,
				   &source->adv);
	if (err != 0) {
		BT_DBG("Failed to create advertising set (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(source->adv, BT_LE_PER_ADV_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to set periodic advertising parameters (err %d)",
		       err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* TODO: If updating the API to have a user-supplied advertiser, we
	 * should simply add the data here, instead of changing all of it.
	 * Similar, if the application changes the data, we should ensure
	 * that the audio advertising data is still present, similar to how
	 * the GAP device name is added.
	 */
	err = generate_broadcast_id(source);
	if (err != 0) {
		BT_DBG("Could not generate broadcast id: %d", err);
		return err;
	}
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, source->broadcast_id);
	ad.type = BT_DATA_SVC_DATA16;
	ad.data_len = ad_buf.len + sizeof(ad.type);
	ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(source->adv, &ad, 1, NULL, 0);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	source->subgroup_count = BROADCAST_SUBGROUP_CNT;
	source->pd = qos->pd;
	err = bt_audio_set_base(source, codec);
	if (err != 0) {
		BT_DBG("Failed to set base data (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(source->adv);
	if (err != 0) {
		BT_DBG("Failed to enable periodic advertising (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(source->adv,
				  BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to start extended advertising (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	for (size_t i = 0; i < source->chan_count; i++) {
		struct bt_audio_chan *chan = &chans[i];

		chan->ep->broadcast_source = source;
		bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_CONFIGURED);
	}

	source->qos = qos;

	BT_DBG("Broadcasting with ID 0x%6X", source->broadcast_id);

	*out_source = source;

	return 0;
}

int bt_audio_broadcast_source_reconfig(struct bt_audio_broadcast_source *source,
				       struct bt_codec *codec,
				       struct bt_codec_qos *qos)
{
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	for (size_t i = 0; i < source->chan_count; i++) {
		chan = &source->chans[i];

		chan_attach(NULL, chan, chan->ep, NULL, codec);
	}

	err = bt_audio_set_base(source, codec);
	if (err != 0) {
		BT_DBG("Failed to set base data (err %d)", err);
		return err;
	}

	source->qos = qos;

	return 0;
}

int bt_audio_broadcast_source_start(struct bt_audio_broadcast_source *source)
{
	struct bt_iso_big_create_param param = { 0 };
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	/* Create BIG */
	param.num_bis = source->chan_count;
	param.bis_channels = source->bis;
	param.framing = source->qos->framing;
	param.packing = 0; /*  TODO: Add to QoS struct */
	param.interval = source->qos->interval;
	param.latency = source->qos->latency;

	err = bt_iso_big_create(source->adv, &param, &source->big);
	if (err != 0) {
		BT_DBG("Failed to create BIG: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_broadcast_source_stop(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_STREAMING) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_STREAMING state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	if (source->big == NULL) {
		BT_DBG("Source is not started");
		return -EALREADY;
	}

	err =  bt_iso_big_terminate(source->big);
	if (err) {
		BT_DBG("Failed to terminate BIG (err %d)", err);
		return err;
	}

	source->big = NULL;

	return 0;
}

int bt_audio_broadcast_source_delete(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_chan *chan;
	struct bt_le_ext_adv *adv;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	adv = source->adv;

	__ASSERT(adv != NULL, "source %p adv is NULL", source);

	/* Stop periodic advertising */
	err = bt_le_per_adv_stop(adv);
	if (err != 0) {
		BT_DBG("Failed to stop periodic advertising (err %d)", err);
		return err;
	}

	/* Stop extended advertising */
	err = bt_le_ext_adv_stop(adv);
	if (err != 0) {
		BT_DBG("Failed to stop extended advertising (err %d)", err);
		return err;
	}

	/* Delete extended advertising set */
	err = bt_le_ext_adv_delete(adv);
	if (err != 0) {
		BT_DBG("Failed to delete extended advertising set (err %d)", err);
		return err;
	}

	/* Reset the broadcast source */
	broadcast_source_cleanup(source);

	return 0;
}

static struct bt_audio_broadcast_sink *broadcast_sink_syncing_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (broadcast_sinks[i].syncing) {
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static struct bt_audio_broadcast_sink *broadcast_sink_free_get(void)
{
	/* Find free entry */
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (broadcast_sinks[i].pa_sync == NULL) {
			broadcast_sinks[i].index = i;
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static struct bt_audio_broadcast_sink *broadcast_sink_get_by_pa(struct bt_le_per_adv_sync *sync)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (broadcast_sinks[i].pa_sync == sync) {
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static void pa_synced(struct bt_le_per_adv_sync *sync,
		      struct bt_le_per_adv_sync_synced_info *info)
{
	struct bt_audio_broadcast_sink *sink;
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	sink = broadcast_sink_syncing_get();
	if (sink == NULL || sync != sink->pa_sync) {
		/* Not ours */
		return;
	}

	BT_DBG("Synced to broadcast source with ID 0x%06X", sink->broadcast_id);

	sink->syncing = false;

	bt_audio_broadcast_sink_scan_stop();

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		/* Terminate early if we do not have any audio sink
		 * capabilities
		 */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->pa_synced != NULL) {
			ops->pa_synced(sink, sink->pa_sync, sink->broadcast_id);
		}
	}

	/* TODO: Wait for an parse PA data and use the capability ops to
	 * get audio channels from the upper layer
	 * TBD: What if sync to a bad broadcast source that does not send
	 * properly formatted (or any) BASE?
	 */
}

static void pa_term(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	struct bt_audio_broadcast_sink *sink;
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	sink = broadcast_sink_get_by_pa(sync);
	if (sink == NULL) {
		/* Not ours */
		return;
	}

	BT_DBG("PA sync with broadcast source with ID 0x%06X lost",
	       sink->broadcast_id);

	memset(sink, 0, sizeof(*sink));

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		/* Terminate early if we do not have any audio sink
		 * capabilities
		 */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->pa_sync_lost != NULL) {
			ops->pa_sync_lost(sink);
		}
	}
}

static bool net_buf_decode_codec_ltv(struct net_buf_simple *buf,
				     struct bt_codec_data *codec_data)
{
	size_t value_len;
	void *value;

	if (buf->len < sizeof(codec_data->data.data_len)) {
		BT_DBG("Not enough data for LTV length field: %u", buf->len);
		return false;
	}
	codec_data->data.data_len = net_buf_simple_pull_u8(buf);

	if (buf->len < sizeof(codec_data->data.type)) {
		BT_DBG("Not enough data for LTV type field: %u", buf->len);
		return false;
	}
	codec_data->data.type = net_buf_simple_pull_u8(buf);
	codec_data->data.data = codec_data->value;

	value_len = codec_data->data.data_len - sizeof(codec_data->data.type);
	if (buf->len < value_len) {
		BT_DBG("Not enough data for LTV value field: %u/%zu",
		       buf->len, value_len);
		return false;
	}
	value = net_buf_simple_pull_mem(buf, value_len);
	memcpy(codec_data->value, value, value_len);

	return true;
}

static bool net_buf_decode_bis_data(struct net_buf_simple *buf,
				    struct bt_audio_base_bis_data *bis,
				    bool codec_data_already_found)
{
	uint8_t len;

	if (buf->len < BASE_BIS_DATA_MIN_SIZE) {
		BT_DBG("Not enough bytes (%u) to decode BIS data", buf->len);
		return false;
	}

	bis->index = net_buf_simple_pull_u8(buf);
	if (bis->index == 0) {
		BT_DBG("BIS index was 0");
		return false;
	}

	/* codec config data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		BT_DBG("Invalid BIS specific codec config data length: "
		       "%u (buf is %u)", len, buf->len);
		return false;
	}

	if (len > 0) {
		struct net_buf_simple ltv_buf;
		void *ltv_data;

		if (codec_data_already_found) {
			/* Codec config can either be specific to each
			 *  BIS or for all, but not both
			 */
			BT_DBG("BASE contains both codec config data and BIS "
			       "codec config data. Aborting.");
			return false;
		}

		/* TODO: Support codec configuration data per bis */
		BT_WARN("BIS specific codec config data of length %u "
			"was found but is not supported yet", len);

		/* Use an extra net_buf_simple to be able to decode until it
		 * is empty (len = 0)
		 */
		ltv_data = net_buf_simple_pull_mem(buf, len);
		net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

		while (ltv_buf.len != 0) {
			struct bt_codec_data *bis_codec_data;

			bis_codec_data = &bis->data[bis->data_count];

			if (!net_buf_decode_codec_ltv(&ltv_buf,
						      bis_codec_data)) {
				BT_DBG("Failed to decode BIS config data for entry %u",
				       bis->data_count);
				return false;
			}
			bis->data_count++;
		}
	}

	return true;
}

static bool net_buf_decode_subgroup(struct net_buf_simple *buf,
				    struct bt_audio_base_subgroup *subgroup)
{
	struct net_buf_simple ltv_buf;
	struct bt_codec	*codec;
	void *ltv_data;
	uint8_t len;

	codec = &subgroup->codec;

	subgroup->bis_count = net_buf_simple_pull_u8(buf);
	if (subgroup->bis_count > ARRAY_SIZE(subgroup->bis_data)) {
		BT_DBG("BASE has more BIS %u than we support %u",
		       subgroup->bis_count,
		       (uint8_t)ARRAY_SIZE(subgroup->bis_data));
		return false;
	}
	codec->id = net_buf_simple_pull_u8(buf);
	codec->cid = net_buf_simple_pull_le16(buf);
	codec->vid = net_buf_simple_pull_le16(buf);

	/* codec configuration data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		BT_DBG("Invalid codec config data length: %u (buf is %u)",
		len, buf->len);
		return false;
	}

	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *codec_data = &codec->data[codec->data_count++];

		if (!net_buf_decode_codec_ltv(&ltv_buf, codec_data)) {
			BT_DBG("Failed to decode codec config data for entry %u",
			       codec->data_count - 1);
			return false;
		}
	}

	if (sizeof(len) > buf->len) {
		return false;
	}

	/* codec metadata length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		BT_DBG("Invalid codec config data length: %u (buf is %u)",
		len, buf->len);
		return false;
	}


	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *metadata = &codec->meta[codec->meta_count++];

		if (!net_buf_decode_codec_ltv(&ltv_buf, metadata)) {
			BT_DBG("Failed to decode codec metadata for entry %u",
			       codec->meta_count - 1);
			return false;
		}
	}

	for (int i = 0; i < subgroup->bis_count; i++) {
		if (!net_buf_decode_bis_data(buf, &subgroup->bis_data[i],
					     codec->data_count > 0)) {
			BT_DBG("Failed to decode BIS data for bis %d", i);
			return false;
		}
	}

	return true;
}

static bool pa_decode_base(struct bt_data *data, void *user_data)
{
	struct bt_audio_broadcast_sink *sink = (struct bt_audio_broadcast_sink *)user_data;
	struct bt_codec_qos codec_qos = { 0 };
	struct bt_audio_base base = { 0 };
	struct bt_uuid_16 broadcast_uuid;
	struct bt_audio_capability *cap;
	struct net_buf_simple net_buf;
	sys_slist_t *lst;
	void *uuid;

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		/* Terminate early if we do not have any audio sink
		 * capabilities
		 */
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BASE_MIN_SIZE) {
		return true;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)data->data,
				      data->data_len);

	uuid = net_buf_simple_pull_mem(&net_buf, BT_UUID_SIZE_16);

	if (!bt_uuid_create(&broadcast_uuid.uuid, uuid, BT_UUID_SIZE_16)) {
		BT_ERR("bt_uuid_create failed");
		return false;
	}

	if (bt_uuid_cmp(&broadcast_uuid.uuid, BT_UUID_BASIC_AUDIO) != 0) {
		/* Continue parsing */
		return true;
	}

	codec_qos.pd = net_buf_simple_pull_le24(&net_buf);
	sink->subgroup_count = net_buf_simple_pull_u8(&net_buf);
	base.subgroup_count = sink->subgroup_count;
	for (int i = 0; i < base.subgroup_count; i++) {
		if (!net_buf_decode_subgroup(&net_buf, &base.subgroups[i])) {
			BT_DBG("Failed to decode subgroup %d", i);
			return false;
		}
	}

	/* TODO: We can already at this point validate whether the BASE
	 * is useful for us based on our capabilities. E.g. if the codec
	 * or number of BIS doesn't match our capabilities, there's no
	 * reason to continue with the BASE. For now let the upper layer
	 * decide.
	 */

	if (sink->biginfo_received) {
		uint8_t num_bis = 0;

		for (int i = 0; i < base.subgroup_count; i++) {
			num_bis += base.subgroups[i].bis_count;
		}

		if (num_bis > sink->biginfo_num_bis) {
			BT_WARN("BASE contains more BIS than reported by BIGInfo");
			return false;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->base_recv != NULL) {
			ops->base_recv(sink, &base);
		}
	}

	return false;
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	struct bt_audio_broadcast_sink *sink = broadcast_sink_get_by_pa(sync);

	if (sink == NULL) {
		/* Not a PA sync that we control */
		return;
	}

	bt_data_parse(buf, pa_decode_base, (void *)sink);
}

static void biginfo_recv(struct bt_le_per_adv_sync *sync,
			 const struct bt_iso_biginfo *biginfo)
{
	struct bt_audio_broadcast_sink *sink;
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	sink = broadcast_sink_get_by_pa(sync);
	if (sink == NULL) {
		/* Not ours */
		return;
	}

	sink->biginfo_received = true;
	sink->iso_interval = biginfo->iso_interval;
	sink->biginfo_num_bis = biginfo->num_bis;
	sink->big_encrypted = biginfo->encryption;

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		/* Terminate early if we do not have any audio sink
		 * capabilities
		 */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->syncable != NULL) {
			ops->syncable(sink, biginfo->encryption);
		}
	}
}

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint16_t timeout;

	/* Ensure that the following calculation does not overflow silently */
	__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

	/* Add retries and convert to unit in 10's of ms */
	timeout = ((uint32_t)interval * SYNC_RETRY_COUNT) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
			BT_GAP_PER_ADV_MAX_TIMEOUT);

	return timeout;
}

static void sync_broadcast_pa(sys_slist_t *lst,
			      const struct bt_le_scan_recv_info *info,
			      uint32_t broadcast_id,
			      struct bt_audio_capability *cap)
{
	struct bt_le_per_adv_sync_param param;
	struct bt_audio_broadcast_sink *sink;
	static bool pa_cb_registered;
	int err;

	if (!pa_cb_registered) {
		static struct bt_le_per_adv_sync_cb cb = {
			.synced = pa_synced,
			.recv = pa_recv,
			.term = pa_term,
			.biginfo = biginfo_recv
		};

		bt_le_per_adv_sync_cb_register(&cb);
	}

	sink = broadcast_sink_free_get();
	/* Should never happen as we check for free entry before
	 * scanning
	 */
	__ASSERT(sink != NULL, "sink is NULL");

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);
	err = bt_le_per_adv_sync_create(&param, &sink->pa_sync);
	if (err != 0) {
		BT_ERR("Could not sync to PA: %d", err);
		err = bt_le_scan_stop();
		if (err != 0 && err != -EALREADY) {
			BT_ERR("Could not stop scan: %d", err);
		}

		SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
			struct bt_audio_capability_ops *ops;

			ops = cap->ops;

			if (ops != NULL && ops->scan_term != NULL) {
				ops->scan_term(err);
			}
		}
	} else {
		sink->syncing = true;
		sink->pa_interval = info->interval;
		sink->broadcast_id = broadcast_id;
		sink->cap = cap;
	}
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;
	sys_slist_t *lst;
	struct bt_audio_capability *cap;

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		/* Terminate early if we do not have any audio sink
		 * capabilities
		 */
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	if (broadcast_sink_syncing_get() != NULL) {
		/* Already syncing, can maximum sync one */
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	BT_DBG("Found broadcast source with address %s and id 0x%6X",
	       bt_addr_le_str(info->addr), broadcast_id);

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->scan_recv != NULL) {
			bool sync_pa = ops->scan_recv(info, broadcast_id);

			if (sync_pa) {
				sync_broadcast_pa(lst, info, broadcast_id, cap);
				break;
			}
		}
	}

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad)
{
	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) ||
	     info->interval == 0) {
		return;
	}

	bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
}

static void broadcast_scan_timeout(void)
{
	struct bt_audio_capability *cap;
	sys_slist_t *lst;

	bt_le_scan_cb_unregister(&broadcast_scan_cb);

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		BT_WARN("No BT_AUDIO_SINK capabilities");
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->scan_term != NULL) {
			ops->scan_term(-ETIME);
		}
	}
}

int bt_audio_broadcast_sink_scan_start(const struct bt_le_scan_param *param)
{
	sys_slist_t *lst;
	int err;

	CHECKIF(param == NULL) {
		BT_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->timeout != 0) {
		/* This is to avoid having to re-implement the scan timeout
		 * callback as well, and can be modified later if requested
		 */
		BT_DBG("Scan param shall not have a timeout");
		return -EINVAL;
	}

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		BT_WARN("No BT_AUDIO_SINK capabilities");
		return -EINVAL;
	}

	if (broadcast_sink_free_get() == NULL) {
		BT_DBG("No more free broadcast sinks");
		return -ENOMEM;
	}

	/* TODO: check for scan callback */
	err = bt_le_scan_start(param, NULL);
	if (err == 0) {
		broadcast_scan_cb.recv = broadcast_scan_recv;
		broadcast_scan_cb.timeout = broadcast_scan_timeout;
		bt_le_scan_cb_register(&broadcast_scan_cb);
	}

	return err;
}

int bt_audio_broadcast_sink_scan_stop(void)
{
	struct bt_audio_broadcast_sink *sink;
	struct bt_audio_capability *cap;
	sys_slist_t *lst;
	int err;

	sink = broadcast_sink_syncing_get();
	if (sink != NULL) {
		err = bt_le_per_adv_sync_delete(sink->pa_sync);
		if (err != 0) {
			BT_DBG("Could not delete PA sync: %d", err);
			return err;
		}
		sink->pa_sync = NULL;
		sink->syncing = false;
	}

	err = bt_le_scan_stop();
	if (err == 0) {
		bt_le_scan_cb_unregister(&broadcast_scan_cb);
	}

	lst = bt_audio_capability_get(BT_AUDIO_SINK);
	if (lst == NULL) {
		BT_WARN("No BT_AUDIO_SINK capabilities");
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->scan_term != NULL) {
			ops->scan_term(0);
		}
	}

	return err;
}

static int bt_audio_broadcast_sink_setup_chan(uint8_t index,
					      struct bt_audio_chan *chan,
					      struct bt_codec *codec)
{
	static struct bt_iso_chan_io_qos sink_chan_io_qos;
	static struct bt_iso_chan_qos sink_chan_qos;
	static struct bt_codec_qos codec_qos;
	struct bt_audio_ep *ep;
	int err;

	if (chan->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("Channel %p not idle", chan);
		return -EALREADY;
	}

	ep = bt_audio_ep_broadcaster_new(index, BT_AUDIO_SINK);
	if (ep == NULL) {
		BT_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	/* TODO: The values of sink_chan_io_qos and codec_qos are not used,
	 * but the `rx` and `qos` pointers need to be set. This should be fixed.
	 */
	sink_chan_qos.rx = &sink_chan_io_qos;

	chan_attach(NULL, chan, ep, NULL, codec);
	chan->qos = &codec_qos;
	err = codec_qos_to_iso_qos(chan->iso->qos, &codec_qos);
	if (err) {
		BT_ERR("Unable to convert codec QoS to ISO QoS");
		return err;
	}

	return 0;
}

int bt_audio_broadcast_sink_sync(struct bt_audio_broadcast_sink *sink,
				 uint32_t indexes_bitfield,
				 struct bt_audio_chan *chan,
				 const uint8_t broadcast_code[16])
{
	struct bt_iso_big_sync_param param;
	struct bt_audio_chan *tmp;
	uint8_t num_bis;
	int err;

	CHECKIF(sink == NULL) {
		BT_DBG("sink is NULL");
		return -EINVAL;
	}

	CHECKIF(indexes_bitfield == 0) {
		BT_DBG("indexes_bitfield is 0");
		return -EINVAL;
	}

	CHECKIF(indexes_bitfield & BIT(0)) {
		BT_DBG("BIT(0) is not a valid BIS index");
		return -EINVAL;
	}

	CHECKIF(chan == NULL) {
		BT_DBG("chan is NULL");
		return -EINVAL;
	}

	if (sink->pa_sync == NULL) {
		BT_DBG("Sink is not PA synced");
		return -EINVAL;
	}

	if (!sink->biginfo_received) {
		/* TODO: We could store the request to sync and start the sync
		 * once the BIGInfo has been received, and then do the sync
		 * then. This would be similar how LE Create Connection works.
		 */
		BT_DBG("BIGInfo not received, cannot sync yet");
		return -EAGAIN;
	}

	/* Validate that number of bits set is less than number of channels */
	/* 0x1f is maximum number of BIS indexes */
	/* TODO: Replace 0x1F with a macro once merged upstream */
	num_bis = 0;
	tmp = chan;
	for (int i = 1; i < 0x1F; i++) {
		if ((indexes_bitfield & BIT(i)) != 0) {
			if (tmp == NULL) {
				BT_DBG("Mismatch between number of bits and channels");
				return -EINVAL;
			}

			tmp = SYS_SLIST_PEEK_NEXT_CONTAINER(tmp, node);

			num_bis++;
		}
	}


	err = bt_audio_broadcast_sink_setup_chan(sink->index, chan,
						 sink->cap->codec);
	if (err != 0) {
		BT_DBG("Failed to setup chan %p: %d", chan, err);
		return err;
	}

	sink->bis_count = 0;
	sink->bis[sink->bis_count++] = &chan->ep->iso;

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		err = bt_audio_broadcast_sink_setup_chan(sink->index, tmp,
							 sink->cap->codec);
		if (err != 0) {
			BT_DBG("Failed to setup chan %p: %d", tmp, err);
			/* TODO: Cleanup already setup channels */
			return err;
		}

		sink->bis[sink->bis_count++] = &tmp->ep->iso;
	}

	param.bis_channels = sink->bis;
	param.num_bis = num_bis;
	param.bis_bitfield = indexes_bitfield;
	param.mse = 0; /* Let controller decide */
	param.sync_timeout = interval_to_sync_timeout(sink->iso_interval);
	param.encryption = sink->big_encrypted; /* TODO */
	if (param.encryption) {
		CHECKIF(broadcast_code == NULL) {
			BT_DBG("Broadcast code required");
			return -EINVAL;
		}
		memcpy(param.bcode, broadcast_code, sizeof(param.bcode));
	} else {
		memset(param.bcode, 0, sizeof(param.bcode));
	}

	err = bt_iso_big_sync(sink->pa_sync, &param, &sink->big);
	if (err != 0) {
		return err;
	}

	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_CONFIGURED);

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		bt_audio_chan_set_state(tmp, BT_AUDIO_CHAN_CONFIGURED);
	}

	sink->bis_count = num_bis;
	sink->chan = chan;

	return err;
}

int bt_audio_broadcast_sink_stop(struct bt_audio_broadcast_sink *sink)
{
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(sink == NULL) {
		BT_DBG("sink is NULL");
		return -EINVAL;
	}

	chan = sink->chan;

	if (chan->state != BT_AUDIO_CHAN_STREAMING &&
	    chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Channel is not configured or streaming");
		return -EALREADY;
	}

	err = bt_iso_big_terminate(sink->big);
	if (err) {
		BT_DBG("Failed to terminate BIG (err %d)", err);
		return err;
	}

	sink->big = NULL;
	/* Channel states will be updated in the ep_iso_disconnected function */

	return 0;
}

int bt_audio_broadcast_sink_delete(struct bt_audio_broadcast_sink *sink)
{
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(sink == NULL) {
		BT_DBG("sink is NULL");
		return -EINVAL;
	}

	chan = sink->chan;

	if (chan != NULL && chan->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("Sink chan %p is not in the BT_AUDIO_CHAN_IDLE state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	if (sink->pa_sync == NULL) {
		BT_DBG("Broadcast sink is already deleted");
		return -EALREADY;
	}

	err = bt_le_per_adv_sync_delete(sink->pa_sync);
	if (err != 0) {
		BT_DBG("Failed to delete periodic advertising sync (err %d)",
		       err);
		return err;
	}

	/* Reset the broadcast sink */
	(void)memset(sink, 0, sizeof(*sink));

	return 0;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST */
#endif /* CONFIG_BT_BAP */
