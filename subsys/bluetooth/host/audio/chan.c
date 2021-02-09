/*  Bluetooth Audio Channel */

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

#include "../conn_internal.h"

#include "endpoint.h"
#include "chan.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_CHAN)
#define LOG_MODULE_NAME bt_audio_chan
#include "common/log.h"

static struct bt_audio_chan *chan_listen;

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

	BT_DBG("conn %p ep %p cap %p codec 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", conn, ep, cap, codec->id, codec->cid,
	       codec->vid);

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
	int err;

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
		err = cap->ops->reconfig(chan, cap, codec);
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

static int bt_audio_chan_iso_accept(struct bt_conn *conn,
				    struct bt_iso_chan **chan)
{
	BT_DBG("conn %p", conn);

	if (!chan_listen) {
		BT_ERR("No channel listening\n");
		return -EPERM;
	}

	*chan = chan_listen->iso;
	chan_listen = NULL;

	return 0;
}

static struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L2,
	.accept = bt_audio_chan_iso_accept,
};

static int bt_audio_chan_iso_listen(struct bt_audio_chan *chan)
{
	static bool server;
	int err;

	BT_DBG("chan %p conn %p", chan, chan->conn);

	if (server) {
		chan_listen = chan;
		return 0;
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		BT_ERR("bt_iso_server_register: %d", err);
		return err;
	}

	server = true;
	chan_listen = chan;

	return 0;
}

int bt_audio_chan_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	int err;

	BT_DBG("chan %p qos %p", chan, qos);

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops || !qos) {
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

	if (chan == chan_listen) {
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

int bt_audio_chan_release(struct bt_audio_chan *chan, bool cache)
{
	int err;

	BT_DBG("chan %p cache %s", chan, cache ? "true" : "false");

	if (!chan || !chan->ep || !chan->cap || !chan->cap->ops) {
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

	/* Unbind all channels if chan2 is NULL */
	if (!chan2) {
		SYS_SLIST_FOR_EACH_CONTAINER(&chan1->links, chan2, node) {
			int err;

			err = bt_audio_chan_unlink(chan1, chan2);
			if (err) {
				return err;
			}
		}
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
	BT_DBG("chan %p", chan);

	bt_audio_ep_detach(chan->ep, chan);

	chan->conn = NULL;
	chan->cap = NULL;
	chan->codec = NULL;

	bt_audio_chan_disconnect(chan);
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

	if (state == BT_AUDIO_CHAN_IDLE && chan->conn) {
		chan_detach(chan);
	}

	chan->state = state;
}
#else
void bt_audio_chan_set_state(struct bt_audio_chan *chan, uint8_t state)
{
	if (state == BT_AUDIO_CHAN_IDLE && chan->conn) {
		chan_detach(chan);
	}

	chan->state = state;
}
#endif /* CONFIG_BT_AUDIO_DEBUG_CHAN */

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

	/* Fill up ISO QoS settings from Codec QoS*/
	if (chan->qos != qos) {
		chan->iso->qos->dir = qos->dir;
		chan->iso->qos->interval = qos->interval;
		chan->iso->qos->framing = qos->framing;
		chan->iso->qos->latency = qos->latency;
		chan->iso->qos->sdu = qos->sdu;
		chan->iso->qos->phy = qos->phy;
		chan->iso->qos->rtn = qos->rtn;
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
	BT_DBG("chan %p iso %p", chan, chan->iso);

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
		return -EINPROGRESS;
	case BT_ISO_CONNECTED:
		return 0;
	default:
		return bt_iso_chan_connect(&chan->iso, 1);
	}
}

int bt_audio_chan_disconnect(struct bt_audio_chan *chan)
{
	BT_DBG("chan %p iso %p", chan, chan->iso);

	if (!chan) {
		return -EINVAL;
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
