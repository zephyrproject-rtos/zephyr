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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_CHAN)
#define LOG_MODULE_NAME bt_audio_chan
#include "common/log.h"

#if defined(CONFIG_BT_BAP)

struct bt_audio_broadcaster {
	struct bt_iso_chan *bis[BROADCAST_SRC_CNT];
};

static struct bt_audio_chan *enabling[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_audio_broadcaster broadcasters[BROADCAST_CNT];

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

	if (!chan || !chan->ep || !cap || !cap->ops || !codec) {
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

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
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

	BT_DBG("chan %p", chan);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
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

	/* Stop periodic advertising */
	err = bt_le_per_adv_stop(chan->ep->adv);
	if (err != 0) {
		BT_DBG("Failed to stop periodic advertising (err %d)", err);
		return err;
	}

	/* Stop extended advertising */
	err = bt_le_ext_adv_stop(chan->ep->adv);
	if (err != 0) {
		BT_DBG("Failed to stop extended advertising (err %d)", err);
		return err;
	}

	/* Delete extended advertising set */
	err = bt_le_ext_adv_delete(chan->ep->adv);
	if (err != 0) {
		BT_DBG("Failed to delete extended advertising set (err %d)", err);
		return err;
	}

	/* Reset the BIS'es */
	for (int i = 0; i < ARRAY_SIZE(broadcasters); i++) {
		for (int j = 0; j < ARRAY_SIZE(broadcasters[i].bis); j++) {
			if (broadcasters[i].bis[j] == &chan->ep->iso) {
				memset(&broadcasters[i], 0, sizeof(broadcasters[i]));
				break;
			}
		}
	}

	chan->ep->adv = NULL;
	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_IDLE);

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		tmp->ep->adv = NULL;
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

	switch (chan->ep->status.state) {
	 /* or 0x04 (Streaming) */
	case BT_ASCS_ASE_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(chan->ep->status.state));
		return -EBADMSG;
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

int bt_audio_broadcaster_create(struct bt_audio_chan *chan,
				struct bt_codec *codec,
				struct bt_codec_qos *qos)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				BT_UUID_16_ENCODE(BT_UUID_BROADCAST_AUDIO_VAL))
	};
	struct bt_iso_chan **bis_channels = NULL;
	struct bt_audio_chan *tmp;
	uint8_t bis_count = 0;
	uint8_t index;
	int err;

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


	for (index = 0; index < ARRAY_SIZE(broadcasters); index++) {
		if (broadcasters[index].bis[0] == NULL) { /* Find free entry */
			bis_channels = broadcasters[index].bis;
			break;
		}
	}

	if (bis_channels == NULL) {
		BT_DBG("Could not allocate any more broadcasters");
		return -ENOMEM;
	}

	err = bt_audio_broadcaster_setup_chan(index, chan, codec, qos);
	if (err != 0) {
		BT_DBG("Failed to setup chan %p: %d", chan, err);
		return err;
	}

	bis_channels[bis_count++] = &chan->ep->iso;

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		err = bt_audio_broadcaster_setup_chan(index, tmp, codec, qos);
		if (err != 0) {
			BT_DBG("Failed to setup chan %p: %d", chan, err);
			/* TODO: Cleanup already setup channels */
			return err;
		}

		bis_channels[bis_count++] = &tmp->ep->iso;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL,
				   &chan->ep->adv);
	if (err != 0) {
		BT_DBG("Failed to create advertising set (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(chan->ep->adv, BT_LE_PER_ADV_DEFAULT);
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
	err = bt_le_ext_adv_set_data(chan->ep->adv, ad, ARRAY_SIZE(ad), NULL,
				     0);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* TODO: encode BASE and set basic audio announcement in PA data */

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(chan->ep->adv);
	if (err != 0) {
		BT_DBG("Failed to enable periodic advertising (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(chan->ep->adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to start extended advertising (err %d)", err);
		/* TODO: cleanup */
		return err;
	}

	bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_CONFIGURED);

	SYS_SLIST_FOR_EACH_CONTAINER(&chan->links, tmp, node) {
		tmp->ep->adv = chan->ep->adv; /* Sync adv reference */

		bt_audio_chan_set_state(tmp, BT_AUDIO_CHAN_CONFIGURED);
	}

	return 0;
}

#endif /* CONFIG_BT_BAP */
