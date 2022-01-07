/*  Bluetooth Audio Stream */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/audio.h>

#include "../host/conn_internal.h"
#include "../host/iso_internal.h"

#include "endpoint.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM)
#define LOG_MODULE_NAME bt_audio_stream
#include "common/log.h"

#if defined(CONFIG_BT_AUDIO_UNICAST)
static struct bt_audio_stream *enabling[CONFIG_BT_ISO_MAX_CHAN];

void bt_audio_stream_attach(struct bt_conn *conn,
			    struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep,
			    struct bt_codec *codec)
{
	BT_DBG("conn %p stream %p ep %p codec %p", conn, stream, ep, codec);

	stream->conn = conn;
	stream->codec = codec;
	stream->ep = ep;
	ep->stream = stream;

	if (stream->iso == NULL) {
		stream->iso = &ep->iso;
	}
}

#define IN_RANGE(_min, _max, _value) \
	(_value >= _min && _value <= _max)

static int bt_audio_stream_iso_accept(const struct bt_iso_accept_info *info,
				      struct bt_iso_chan **iso_chan)
{
	int i;

	BT_DBG("acl %p", info->acl);

	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		struct bt_audio_stream *c = enabling[i];

		if (c && c->ep->cig_id == info->cig_id &&
		    c->ep->cis_id == info->cis_id) {
			*iso_chan = enabling[i]->iso;
			enabling[i] = NULL;
			return 0;
		}
	}

	BT_ERR("No channel listening");

	return -EPERM;
}

static struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L2,
	.accept = bt_audio_stream_iso_accept,
};

int bt_audio_stream_iso_listen(struct bt_audio_stream *stream)
{
	static bool server;
	int err, i;
	struct bt_audio_stream **free_stream = NULL;

	BT_DBG("stream %p conn %p", stream, stream->conn);

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
		if (enabling[i] == stream) {
			return 0;
		}

		if (enabling[i] == NULL && free_stream == NULL) {
			free_stream = &enabling[i];
		}
	}

	if (free_stream != NULL) {
		*free_stream = stream;
		return 0;
	}

	BT_ERR("Unable to listen: no slot left");

	return -ENOSPC;
}

int bt_audio_codec_qos_to_iso_qos(struct bt_iso_chan_qos *qos,
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

bool bt_audio_valid_qos(const struct bt_codec_qos *qos)
{
	if (qos->interval < BT_ISO_INTERVAL_MIN ||
	    qos->interval > BT_ISO_INTERVAL_MAX) {
		BT_DBG("Interval not within allowed range: %u (%u-%u)",
		       qos->interval, BT_ISO_INTERVAL_MIN, BT_ISO_INTERVAL_MAX);
		return false;
	}

	if (qos->framing > BT_CODEC_QOS_FRAMED) {
		BT_DBG("Invalid Framing 0x%02x", qos->framing);
		return false;
	}

	if (qos->phy != BT_CODEC_QOS_1M &&
	    qos->phy != BT_CODEC_QOS_2M &&
	    qos->phy != BT_CODEC_QOS_CODED) {
		BT_DBG("Invalid PHY 0x%02x", qos->phy);
		return false;
	}

	if (qos->sdu > BT_ISO_MAX_SDU) {
		BT_DBG("Invalid SDU %u", qos->sdu);
		return false;
	}

	if (qos->latency < BT_ISO_LATENCY_MIN ||
	    qos->latency > BT_ISO_LATENCY_MAX) {
		BT_DBG("Invalid Latency %u", qos->latency);
		return false;
	}

	return true;
}

static bool bt_audio_stream_is_broadcast(const struct bt_audio_stream *stream)
{
	/* TODO: Implement */
	return false;
}

bool bt_audio_valid_stream_qos(const struct bt_audio_stream *stream,
			       const struct bt_codec_qos *qos)
{
	const struct bt_codec_qos_pref *qos_pref = &stream->ep->qos_pref;

	if (qos_pref->latency < qos->latency) {
		BT_DBG("Latency %u higher than preferred max %u",
			qos->latency, qos_pref->latency);
		return false;
	}

	if (!IN_RANGE(qos_pref->pd_min, qos_pref->pd_max, qos->pd)) {
		BT_DBG("Presentation Delay not within range: min %u max %u pd %u",
			qos_pref->pd_min, qos_pref->pd_max, qos->pd);
		return false;
	}

	return true;
}

void bt_audio_stream_detach(struct bt_audio_stream *stream)
{
	const bool is_broadcast = bt_audio_stream_is_broadcast(stream);

	BT_DBG("stream %p", stream);

	stream->conn = NULL;
	stream->codec = NULL;
	stream->ep->stream = NULL;
	stream->ep = NULL;

	if (!is_broadcast) {
		bt_audio_stream_disconnect(stream);
	}
}

int bt_audio_stream_disconnect(struct bt_audio_stream *stream)
{
	int i;

	BT_DBG("stream %p iso %p", stream, stream->iso);

	if (stream == NULL) {
		return -EINVAL;
	}

	/* Stop listening */
	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == stream) {
			enabling[i] = NULL;
			break;
		}
	}

	if (stream->iso == NULL || stream->iso->iso == NULL) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(stream->iso);
}

int bt_audio_stream_send(struct bt_audio_stream *stream, struct net_buf *buf)
{
	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		BT_DBG("Channel not ready for streaming");
		return -EBADMSG;
	}

	/* TODO: Add checks for broadcast sink */

	return bt_iso_chan_send(stream->iso, buf);
}

void bt_audio_stream_cb_register(struct bt_audio_stream *stream,
				 struct bt_audio_stream_ops *ops)
{
	stream->ops = ops;
}
#endif /* CONFIG_BT_AUDIO_UNICAST */
