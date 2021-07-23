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

static struct bt_audio_chan *enabling[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_audio_broadcaster broadcasters[BROADCAST_SRC_CNT];
static struct bt_audio_broadcast_sink broadcast_sinks[BROADCAST_SNK_CNT];
static struct bt_le_scan_cb broadcast_scan_cb;

static int bt_audio_set_base(const struct bt_audio_broadcaster *broadcaster,
			     struct bt_codec *codec);

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

	if (bt_audio_ep_is_broadcast(chan->ep)) {
		struct bt_audio_broadcaster *broadcaster;
		struct bt_audio_chan *tmp;
		int err;

		broadcaster = chan->ep->broadcaster;

		if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
			BT_DBG("Channel is not in the configured state");
			return -EBADMSG;
		}

		chan_attach(NULL, chan, chan->ep, NULL, codec);

		SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {

			chan_attach(NULL, tmp, tmp->ep, NULL, codec);
		}

		err = bt_audio_set_base(broadcaster, codec);
		if (err != 0) {
			BT_DBG("Failed to set base data (err %d)", err);
			/* TODO: cleanup */
			return err;
		}

		return 0;
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

	if (bt_audio_ep_is_broadcast(chan->ep)) {
		struct bt_iso_big_create_param big_create_param = { 0 };
		struct bt_audio_broadcaster *broadcaster;

		broadcaster = chan->ep->broadcaster;

		/* Create BIG */
		big_create_param.num_bis = broadcaster->bis_count;
		big_create_param.bis_channels = broadcaster->bis;

		err = bt_iso_big_create(broadcaster->adv, &big_create_param,
					&broadcaster->big);
		if (err) {
			BT_DBG("Failed to create BIG (err %d)", err);
			return err;
		}

		return 0;
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
	int err;

	if (!chan || !chan->ep) {
		BT_DBG("Invalid channel or endpoint");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast(chan->ep)) {
		if (chan->state != BT_AUDIO_CHAN_STREAMING) {
			BT_DBG("Channel is not streaming");
			return -EALREADY;
		}

		err = bt_iso_big_terminate(chan->ep->broadcaster->big);
		if (err) {
			BT_DBG("Failed to terminate BIG (err %d)", err);
			return err;
		}

		return 0;
	}

	if (!chan->cap || !chan->cap->ops) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (chan->ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Disabling) */
	case BT_ASCS_ASE_STATE_DISABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
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
	if (chan->ep->type != BT_AUDIO_EP_LOCAL) {
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

	bt_audio_ep_set_state(chan->ep, BT_ASCS_ASE_STATE_QOS);
	bt_audio_chan_iso_listen(chan);

	return err;
}

static int bt_audio_chan_broadcast_release(struct bt_audio_chan *chan)
{
	int err;
	struct bt_audio_chan *tmp;
	struct bt_audio_broadcaster *source;
	struct bt_le_ext_adv *adv;

	source = chan->ep->broadcaster;
	adv = source->adv;

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
	memset(source, 0, sizeof(*source));

	chan->ep->broadcaster = NULL;
	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_IDLE);

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		tmp->ep->broadcaster = NULL;
		bt_audio_chan_set_state(tmp, BT_AUDIO_CHAN_IDLE);
	}

	return 0;
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

	if (bt_audio_ep_is_broadcast(chan->ep)) {
		if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
			BT_DBG("Broadcast must be stopped before release");
			return -EBADMSG;
		}

		return bt_audio_chan_broadcast_release(chan);
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

	qos->framing = codec->framing;
	io->interval = codec->interval;
	io->latency = codec->latency;
	io->sdu = codec->sdu;
	io->phy = codec->phy;
	io->rtn = codec->rtn;

	return 0;
}

struct bt_conn_iso *bt_audio_chan_bind(struct bt_audio_chan *chan,
				       struct bt_codec_qos *qos)
{
	struct bt_conn *conns[1];
	struct bt_iso_chan *chans[1];

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

	conns[0] = chan->conn;
	chans[0] = chan->iso;

	/* Attempt to bind if not bound yet */
	if (!chan->iso->conn) {
		int err;

		err = bt_iso_chan_bind(conns, 1, chans);
		if (err) {
			BT_ERR("bt_iso_chan_bind: %d", err);
			return NULL;
		}
	}

	return &chan->iso->conn->iso;
}

int bt_audio_chan_unbind(struct bt_audio_chan *chan)
{
	BT_DBG("chan %p", chan);

	return bt_iso_chan_unbind(chan->iso);
}

int bt_audio_chan_connect(struct bt_audio_chan *chan)
{
	BT_DBG("chan %p iso %p", chan, chan ? chan->iso : NULL);

	if (!chan || !chan->iso) {
		return -EINVAL;
	}

	switch (chan->iso->state) {
	case BT_ISO_DISCONNECTED:
		if (!bt_audio_chan_bind(chan, chan->qos)) {
			return -ENOTCONN;
		}

		return bt_iso_chan_connect(&chan->iso, 1);
	case BT_ISO_CONNECT:
		return 0;
	case BT_ISO_CONNECTED:
		return -EALREADY;
	default:
		return bt_iso_chan_connect(&chan->iso, 1);
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

	if (!chan->iso || !chan->iso->conn) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(chan->iso);
}

void bt_audio_chan_reset(struct bt_audio_chan *chan)
{
	BT_DBG("chan %p", chan);

	if (!chan || !chan->conn) {
		return;
	}

	bt_audio_chan_unbind(chan);
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

	if (!bt_audio_ep_is_broadcast(chan->ep)) {
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

static int bt_audio_broadcaster_setup_chan(uint8_t index,
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

static void bt_audio_encode_base(const struct bt_audio_broadcaster *broadcaster,
				 struct bt_codec *codec,
				 struct net_buf_simple *buf)
{
	uint8_t bis_index;
	uint8_t *start;
	uint8_t len;

	__ASSERT(broadcaster->subgroup_count == BROADCAST_SUBGROUP_CNT,
		 "Cannot encode BASE with more than a single subgroup");

	net_buf_simple_add_le16(buf, BT_UUID_BASIC_AUDIO_VAL);
	net_buf_simple_add_le24(buf, broadcaster->pd);
	net_buf_simple_add_u8(buf, broadcaster->subgroup_count);
	/* TODO: The following encoding should be done for each subgroup once
	 * supported
	 */
	net_buf_simple_add_u8(buf, broadcaster->bis_count);
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
	for (int i = 0; i < broadcaster->bis_count; i++) {
		bis_index++;
		net_buf_simple_add_u8(buf, bis_index);
		net_buf_simple_add_u8(buf, 0); /* unused length field */
	}

	/* NOTE: It is also possible to have the codec configuration data per
	 * BIS index. As our API does not support such specialized BISes we
	 * currently don't do that.
	 */
}


static int generate_broadcast_id(struct bt_audio_broadcaster *broadcaster)
{
	bool unique;

	do {
		int err;

		err = bt_rand(&broadcaster->broadcast_id, BT_BROADCAST_ID_SIZE);
		if (err) {
			return err;
		}

		/* Ensure uniqueness */
		unique = true;
		for (int i = 0; i < ARRAY_SIZE(broadcasters); i++) {
			if (&broadcasters[i] == broadcasters) {
				continue;
			}

			if (broadcasters[i].broadcast_id == broadcaster->broadcast_id) {
				unique = false;
				break;
			}
		}
	} while (!unique);

	return 0;
}

static int bt_audio_set_base(const struct bt_audio_broadcaster *broadcaster,
			     struct bt_codec *codec)
{
	struct bt_data base_ad_data;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(base_buf, sizeof(struct bt_audio_base_ad));

	bt_audio_encode_base(broadcaster, codec, &base_buf);

	base_ad_data.type = BT_DATA_SVC_DATA16;
	base_ad_data.data_len = base_buf.len;
	base_ad_data.data = base_buf.data;

	err = bt_le_per_adv_set_data(broadcaster->adv, &base_ad_data, 1);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		return err;
	}

	return 0;
}

int bt_audio_broadcaster_create(struct bt_audio_chan *chan,
				struct bt_codec *codec,
				struct bt_codec_qos *qos)
{
	struct bt_audio_broadcaster *broadcaster;
	struct bt_audio_chan *tmp;
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
	 * advertising data (thus making the broadcaster non-functional in
	 * terms of BAP compliance), or even stop the advertiser without
	 * stopping the BIG (which also goes against the BAP specification).
	 */

	CHECKIF(chan == NULL) {
		BT_DBG("chan is NULL");
		return -EINVAL;
	}

	CHECKIF(codec == NULL) {
		BT_DBG("codec is NULL");
		return -EINVAL;
	}

	broadcaster = NULL;
	for (index = 0; index < ARRAY_SIZE(broadcasters); index++) {
		if (broadcasters[index].bis[0] == NULL) { /* Find free entry */
			broadcaster = &broadcasters[index];
			break;
		}
	}

	if (broadcaster == NULL) {
		BT_DBG("Could not allocate any more broadcasters");
		return -ENOMEM;
	}

	err = bt_audio_broadcaster_setup_chan(index, chan, codec, qos);
	if (err != 0) {
		BT_DBG("Failed to setup chan %p: %d", chan, err);
		return err;
	}

	broadcaster->bis_count = 0;
	broadcaster->bis[broadcaster->bis_count++] = &chan->ep->iso;

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		err = bt_audio_broadcaster_setup_chan(index, tmp, codec, qos);
		if (err != 0) {
			BT_DBG("Failed to setup chan %p: %d", chan, err);
			/* TODO: Cleanup already setup channels */
			return err;
		}

		broadcaster->bis[broadcaster->bis_count++] = &tmp->ep->iso;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL,
				   &broadcaster->adv);
	if (err != 0) {
		BT_DBG("Failed to create advertising set (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(broadcaster->adv, BT_LE_PER_ADV_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to set periodic advertising parameters (err %d)",
		       err);
		/* TODO: cleanup */
		return err;
	}

	/* TODO: If updating the API to have a user-supplied advertiser, we
	 * should simply add the data here, instead of changing all of it.
	 * Similar, if the application changes the data, we should ensure
	 * that the audio advertising data is still present, similar to how
	 * the GAP device name is added.
	 */
	err = generate_broadcast_id(broadcaster);
	if (err != 0) {
		BT_DBG("Could not generate broadcast id: %d", err);
		return err;
	}
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcaster->broadcast_id);
	ad.type = BT_DATA_SVC_DATA16;
	ad.data_len = ad_buf.len + sizeof(ad.type);
	ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(broadcaster->adv, &ad, 1, NULL, 0);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	broadcaster->subgroup_count = BROADCAST_SUBGROUP_CNT;
	broadcaster->pd = qos->pd;
	err = bt_audio_set_base(broadcasters, codec);
	if (err != 0) {
		BT_DBG("Failed to set base data (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(broadcaster->adv);
	if (err != 0) {
		BT_DBG("Failed to enable periodic advertising (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(broadcaster->adv,
				  BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to start extended advertising (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	chan->ep->broadcaster = broadcaster;
	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_CONFIGURED);

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		tmp->ep->broadcaster = broadcaster;

		bt_audio_chan_set_state(tmp, BT_AUDIO_CHAN_CONFIGURED);
	}

	BT_DBG("Broadcasting with ID 0x%6X", broadcaster->broadcast_id);

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

	sink = broadcast_sink_syncing_get();
	if (sink == NULL || sync != sink->pa_sync) {
		/* Not ours */
		return;
	}

	BT_DBG("Synced to broadcaster with ID 0x%06X", sink->broadcast_id);

	sink->syncing = false;

	bt_audio_broadcaster_scan_stop();

	/* TODO: Wait for an parse PA data and use the capability ops to
	 * get audio channels from the upper layer
	 * TBD: What if sync to a bad broadcaster that does not send properly
	 * formatted (or any) BASE?
	 */
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

	SYS_SLIST_FOR_EACH_CONTAINER(lst, cap, node) {
		struct bt_audio_capability_ops *ops;

		ops = cap->ops;

		if (ops != NULL && ops->base_recv != NULL) {
			ops->base_recv(&base, sink->broadcast_id);
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

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t sync_timeout;

	/* Ensure that the following calculation does not overflow silently */
	__ASSERT(SYNC_RETRY_COUNT < 10,
			"SYNC_RETRY_COUNT shall be less than 10");

	/* Add retries and convert to unit in 10's of ms */
	sync_timeout = ((uint32_t)pa_interval * SYNC_RETRY_COUNT) / 10;

	/* Enforce restraints */
	sync_timeout = CLAMP(sync_timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
				BT_GAP_PER_ADV_MAX_TIMEOUT);

	return sync_timeout;
}

static void sync_broadcast_pa(sys_slist_t *lst,
			      const struct bt_le_scan_recv_info *info)
{
	struct bt_le_per_adv_sync_param param;
	struct bt_audio_broadcast_sink *sink;
	struct bt_audio_capability *cap;
	static bool pa_cb_registered;
	int err;

	if (!pa_cb_registered) {
		static struct bt_le_per_adv_sync_cb cb = {
			.synced = pa_synced,
			.recv = pa_recv
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
				sync_broadcast_pa(lst, info);
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

int bt_audio_broadcaster_scan_start(const struct bt_le_scan_param *param)
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

int bt_audio_broadcaster_scan_stop(void)
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

#endif /* CONFIG_BT_BAP */
