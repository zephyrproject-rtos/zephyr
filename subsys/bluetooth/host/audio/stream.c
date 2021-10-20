/*  Bluetooth Audio Stream */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
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
#include "capabilities.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM)
#define LOG_MODULE_NAME bt_audio_stream
#include "common/log.h"

#if defined(CONFIG_BT_BAP)
#if defined(CONFIG_BT_AUDIO_UNICAST)
static struct bt_audio_unicast_group unicast_groups[UNICAST_GROUP_CNT];
static struct bt_audio_stream *enabling[CONFIG_BT_ISO_MAX_CHAN];

void bt_audio_stream_attach(struct bt_conn *conn,
			    struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep,
			    struct bt_audio_capability *cap,
			    struct bt_codec *codec)
{
	BT_DBG("conn %p stream %p ep %p cap %p codec %p", conn, stream, ep, cap,
	       codec);

	stream->conn = conn;
	stream->cap = cap;
	stream->codec = codec;

	bt_audio_ep_attach(ep, stream);
}

struct bt_audio_stream *bt_audio_stream_config(struct bt_conn *conn,
					       struct bt_audio_ep *ep,
					       struct bt_audio_capability *cap,
					       struct bt_codec *codec)
{
	struct bt_audio_stream *stream;

	BT_DBG("conn %p ep %p cap %p codec %p codec id 0x%02x codec cid 0x%04x "
	       "codec vid 0x%04x", conn, ep, cap, codec, codec ? codec->id : 0,
	       codec ? codec->cid : 0, codec ? codec->vid : 0);

	if (!conn || !cap || !cap->ops || !codec) {
		return NULL;
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

	stream = cap->ops->config(conn, ep, cap, codec);
	if (stream == NULL) {
		return stream;
	}

	bt_audio_stream_attach(conn, stream, ep, cap, codec);

	if (ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_CODEC_CONFIGURED);
	}

	return stream;
}

int bt_audio_stream_reconfig(struct bt_audio_stream *stream,
			     struct bt_audio_capability *cap,
			     struct bt_codec *codec)
{
	BT_DBG("stream %p cap %p codec %p", stream, cap, codec);

	if (stream == NULL || stream->ep == NULL) {
		BT_DBG("Invalid stream or endpoint");
		return -EINVAL;
	}

	if (codec == NULL) {
		BT_DBG("NULL codec");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast_src(stream->ep)) {
		BT_DBG("Cannot use %s to reconfigure broadcast source streams",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(stream->ep)) {
		BT_DBG("Cannot reconfigure broadcast sink streams");
		return -EINVAL;
	}

	if (stream->cap == NULL || stream->cap->ops == NULL) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_AUDIO_EP_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	/* Check that codec is supported */
	if (cap->codec->id != codec->id) {
		return -ENOTSUP;
	}

	if (cap->ops->reconfig) {
		int err = cap->ops->reconfig(stream, cap, codec);
		if (err) {
			return err;
		}
	}

	bt_audio_stream_attach(stream->conn, stream, stream->ep, cap, codec);

	if (stream->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(stream->ep,
				      BT_AUDIO_EP_STATE_CODEC_CONFIGURED);
	}

	return 0;
}

#define IN_RANGE(_min, _max, _value) \
	(_value >= _min && _value <= _max)

static bool bt_audio_stream_enabling(struct bt_audio_stream *stream)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == stream) {
			return true;
		}
	}

	return false;
}

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

static int bt_audio_cig_create(struct bt_audio_unicast_group *group,
			       struct bt_codec_qos *qos)
{
	struct bt_iso_cig_create_param param;
	struct bt_audio_stream *stream;
	uint8_t cis_count;
	int err;

	BT_DBG("group %p qos %p", group, qos);

	cis_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		group->cis[cis_count++] = stream->iso;
	}

	param.num_cis = cis_count;
	param.cis_channels = group->cis;
	param.framing = qos->framing;
	param.packing = 0; /*  TODO: Add to QoS struct */
	param.interval = qos->interval;
	param.latency = qos->latency;
	param.sca = BT_GAP_SCA_UNKNOWN;

	err = bt_iso_cig_create(&param, &group->cig);
	if (err != 0) {
		BT_ERR("bt_iso_cig_create failed: %d", err);
		return err;
	}

	group->qos = qos;

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

bool bt_audio_valid_stream_qos(const struct bt_audio_stream *stream,
			       const struct bt_codec_qos *qos)
{
	if (stream->cap->pref.latency < qos->latency) {
		BT_DBG("Latency %u higher than preferred max %u",
			qos->latency, stream->cap->pref.latency);
		return false;
	}

	if (!IN_RANGE(stream->cap->pref.pd_min, stream->cap->pref.pd_max, qos->pd)) {
		BT_DBG("Presentation Delay not within range: min %u max %u pd %u",
			stream->cap->pref.pd_min, stream->cap->pref.pd_max,
			qos->pd);
		return false;
	}

	return true;
}

int bt_audio_stream_qos(struct bt_conn *conn,
			struct bt_audio_unicast_group *group,
			struct bt_codec_qos *qos)
{
	struct bt_audio_stream *stream;
	struct net_buf_simple *buf;
	struct bt_ascs_qos_op *op;
	struct bt_audio_ep *ep;
	bool conn_stream_found;
	bool cig_connected;
	int err;

	BT_DBG("conn %p group %p qos %p", conn, group, qos);

	CHECKIF(conn == NULL) {
		BT_DBG("conn is NULL");
		return -EINVAL;
	}

	CHECKIF(group == NULL) {
		BT_DBG("group is NULL");
		return -EINVAL;
	}

	CHECKIF(qos == NULL) {
		BT_DBG("qos is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&group->streams)) {
		BT_DBG("group stream list is empty");
		return -ENOEXEC;
	}

	CHECKIF(!bt_audio_valid_qos(qos)) {
		BT_DBG("Invalid QoS");
		return -EINVAL;
	}

	/* Used to determine if a stream for the supplied connection pointer
	 * was actually found
	 */
	conn_stream_found = false;

	/* User to determine if any stream in the group is in
	 * the connected state
	 */
	cig_connected = false;

	/* Validate streams before starting the QoS execution */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		if (stream->ep == NULL) {
			BT_DBG("stream->ep is NULL");
			return -EINVAL;
		}

		/* Can only be done if all the streams are in the codec
		 * configured state or the QoS configured state
		 */
		switch (stream->ep->status.state) {
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			break;
		default:
			BT_DBG("Invalid state: %s",
			       bt_audio_ep_state_str(stream->ep->status.state));
			return -EINVAL;
		}

		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}
		conn_stream_found = true;

		if (!bt_audio_valid_stream_qos(stream, qos)) {
			return -EINVAL;
		}

		if (stream->iso == NULL) {
			BT_DBG("stream->iso is NULL");
			return -EINVAL;
		}

		err = bt_audio_codec_qos_to_iso_qos(stream->iso->qos, qos);
		if (err) {
			BT_DBG("Unable to convert codec QoS to ISO QoS: %d",
			       err);
			return err;
		}
	}

	if (!conn_stream_found) {
		BT_DBG("No streams in the group %p for conn %p", group, conn);
		return -EINVAL;
	}

	/* Create or recreate the CIG */
	if (group->cig != NULL) {
		/* TODO: Add support to update the CIG:
		 *       For now we need to recreate it
		 */

		if (qos->interval != group->qos->interval ||
		    qos->latency != group->qos->latency ||
		    qos->framing != group->qos->framing) {
			BT_DBG("Cannot override group QoS values");
			return -EINVAL;
		}

		/* TODO: Terminate and recreate CIG */
	}

	err = bt_audio_cig_create(group, qos);
	if (err != 0) {
		BT_DBG("bt_audio_cig_create failed: %d", err);
		return err;
	}

	/* Generate the control point write */
	buf = bt_audio_ep_create_pdu(BT_ASCS_QOS_OP);

	op = net_buf_simple_add(buf, sizeof(*op));

	(void)memset(op, 0, sizeof(*op));
	ep = NULL; /* Needed to find the control point handle */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		op->num_ases++;

		err = bt_audio_ep_qos(stream->ep, buf, qos);
		if (err) {
			return err;
		}

		if (ep == NULL) {
			ep = stream->ep;
		}
	}

	err = bt_audio_ep_send(conn, ep, buf);
	if (err != 0) {
		BT_DBG("Could not send config QoS: %d", err);
		return err;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		stream->qos = qos;

		bt_audio_ep_set_state(stream->ep,
				      BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	}

	return 0;
}

int bt_audio_stream_enable(struct bt_audio_stream *stream,
			   uint8_t meta_count, struct bt_codec_data *meta)
{
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL || stream->cap == NULL ||
	    stream->cap->ops == NULL) {
		return -EINVAL;
	}

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (stream->ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->enable == NULL) {
		goto done;
	}

	err = stream->cap->ops->enable(stream, meta_count, meta);
	if (err) {
		return err;
	}

done:
	if (stream->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	bt_audio_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_ENABLING);

	if (bt_audio_stream_enabling(stream)) {
		return 0;
	}

	if (stream->cap->type == BT_AUDIO_SOURCE) {
		return 0;
	}

	/* After an ASE has been enabled, the Unicast Server acting as an Audio
	 * Sink for that ASE shall autonomously initiate the Handshake
	 * operation to transition the ASE to the Streaming state when the
	 * Unicast Server is ready to consume audio data transmitted by the
	 * Unicast Client.
	 */
	return bt_audio_stream_start(stream);
}

int bt_audio_stream_metadata(struct bt_audio_stream *stream,
			     uint8_t meta_count, struct bt_codec_data *meta)
{
	int err;

	BT_DBG("stream %p metadata count %u", stream, meta_count);

	if (stream == NULL || stream->ep == NULL || stream->cap == NULL ||
	    stream->cap->ops == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->metadata == NULL) {
		goto done;
	}

	err = stream->cap->ops->metadata(stream, meta_count, meta);
	if (err) {
		return err;
	}

done:
	if (stream->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	/* Set the state to the same state to trigger the notifications */
	bt_audio_ep_set_state(stream->ep, stream->ep->status.state);

	return 0;
}

int bt_audio_stream_disable(struct bt_audio_stream *stream)
{
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL || stream->cap == NULL ||
	    stream->cap->ops == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->disable == NULL) {
		err = 0;
		goto done;
	}

	err = stream->cap->ops->disable(stream);
	if (err) {
		return err;
	}

done:
	if (stream->ep->type != BT_AUDIO_EP_LOCAL) {
		return 0;
	}

	bt_audio_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_DISABLING);

	if (stream->cap->type == BT_AUDIO_SOURCE) {
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
	return bt_audio_stream_stop(stream);
}

int bt_audio_stream_start(struct bt_audio_stream *stream)
{
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL) {
		BT_DBG("Invalid stream or endpoint");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast_src(stream->ep)) {
		BT_DBG("Cannot use %s to start broadcast source streams",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(stream->ep)) {
		BT_DBG("Cannot start broadcast sink streams");
		return -EINVAL;
	}

	if (stream->cap == NULL || stream->cap->ops == NULL) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->start == NULL) {
		err = 0;
		goto done;
	}

	err = stream->cap->ops->start(stream);
	if (err) {
		return err;
	}

done:
	if (stream->ep->type == BT_AUDIO_EP_LOCAL) {
		bt_audio_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_STREAMING);
	}

	return err;
}

int bt_audio_stream_stop(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep;
	int err;

	if (stream == NULL || stream->ep == NULL) {
		BT_DBG("Invalid stream or endpoint");
		return -EINVAL;
	}

	ep = stream->ep;

	if (bt_audio_ep_is_broadcast_src(stream->ep)) {
		BT_DBG("Cannot use %s to stop broadcast source streams",
		       __func__);
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(stream->ep)) {
		BT_DBG("Cannot use %s to stop broadcast sink streams",
		       __func__);
		return -EINVAL;
	}

	if (stream->cap == NULL || stream->cap->ops == NULL) {
		BT_DBG("Invalid capabilities or capabilities ops");
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Disabling) */
	case BT_AUDIO_EP_STATE_DISABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->stop == NULL) {
		err = 0;
		goto done;
	}

	err = stream->cap->ops->stop(stream);
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
	if (!bt_audio_stream_disconnect(stream)) {
		return err;
	}

	bt_audio_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	bt_audio_stream_iso_listen(stream);

	return err;
}

int bt_audio_stream_release(struct bt_audio_stream *stream, bool cache)
{
	int err;

	BT_DBG("stream %p cache %s", stream, cache ? "true" : "false");

	CHECKIF(stream == NULL || stream->ep == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	if (bt_audio_ep_is_broadcast_src(stream->ep)) {
		BT_DBG("Cannot release a broadcast source");
		return -EINVAL;
	} else if (bt_audio_ep_is_broadcast_snk(stream->ep)) {
		BT_DBG("Cannot release a broadcast sink");
		return -EINVAL;
	}

	CHECKIF(stream->cap == NULL || stream->cap->ops == NULL) {
		BT_DBG("Capability or capability ops is NULL");
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
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
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	if (stream->cap->ops->release == NULL) {
		err = 0;
		goto done;
	}

	err = stream->cap->ops->release(stream);
	if (err) {
		if (err == -ENOTCONN) {
			return 0;
		}
		return err;
	}

done:
	if (stream->ep->type != BT_AUDIO_EP_LOCAL) {
		return err;
	}

	/* Any previously applied codec configuration may be cached by the
	 * server.
	 */
	if (!cache) {
		bt_audio_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_RELEASING);
	} else {
		bt_audio_ep_set_state(stream->ep,
				      BT_AUDIO_EP_STATE_CODEC_CONFIGURED);
	}

	return err;
}

void bt_audio_stream_detach(struct bt_audio_stream *stream)
{
	const bool is_broadcast = bt_audio_ep_is_broadcast(stream->ep);

	BT_DBG("stream %p", stream);

	bt_audio_ep_detach(stream->ep, stream);

	stream->conn = NULL;
	stream->cap = NULL;
	stream->codec = NULL;

	if (!is_broadcast) {
		bt_audio_stream_disconnect(stream);
	}
}

int bt_audio_cig_terminate(struct bt_audio_stream *stream)
{
	BT_DBG("stream %p", stream);

	if (stream->iso == NULL) {
		BT_DBG("Channel not bound");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(unicast_groups); i++) {
		int err;
		struct bt_iso_cig *cig;

		cig = unicast_groups[i].cig;
		if (cig != NULL &&
		    cig->cis != NULL &&
		    cig->cis[0] == stream->iso) {
			err = bt_iso_cig_terminate(cig);
			if (err == 0) {
				unicast_groups[i].cig = NULL;
			}

			return err;
		}
	}

	BT_DBG("CIG not found for stream %p", stream);
	return 0; /* Return 0 as it would already be terminated */
}

int bt_audio_stream_connect(struct bt_audio_stream *stream)
{
	struct bt_iso_connect_param param;

	BT_DBG("stream %p iso %p", stream, stream ? stream->iso : NULL);

	if (stream == NULL || stream->iso == NULL) {
		return -EINVAL;
	}

	param.acl = stream->conn;
	param.iso_chan = stream->iso;

	switch (stream->iso->state) {
	case BT_ISO_DISCONNECTED:
		return bt_iso_chan_connect(&param, 1);
	case BT_ISO_CONNECT:
		return 0;
	case BT_ISO_CONNECTED:
		return -EALREADY;
	default:
		return bt_iso_chan_connect(&param, 1);
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
		}
	}

	if (stream->iso == NULL || stream->iso->iso == NULL) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(stream->iso);
}

void bt_audio_stream_reset(struct bt_audio_stream *stream)
{
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->conn == NULL) {
		return;
	}

	err = bt_audio_cig_terminate(stream);
	if (err != 0) {
		BT_ERR("Failed to terminate CIG: %d", err);
	}
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

	if (bt_audio_ep_is_broadcast_snk(stream->ep)) {
		BT_DBG("Cannot send on a broadcast sink stream");
		return -EINVAL;
	} else if (!bt_audio_ep_is_broadcast_src(stream->ep)) {
		switch (stream->ep->status.state) {
		/* or 0x04 (Streaming) */
		case BT_AUDIO_EP_STATE_STREAMING:
			break;
		default:
			BT_ERR("Invalid state: %s",
			bt_audio_ep_state_str(stream->ep->status.state));
			return -EBADMSG;
		}
	}

	return bt_iso_chan_send(stream->iso, buf);
}

void bt_audio_stream_cb_register(struct bt_audio_stream *stream,
				 struct bt_audio_stream_ops *ops)
{
	stream->ops = ops;
}
#endif /* CONFIG_BT_AUDIO_UNICAST */

int bt_audio_unicast_group_create(struct bt_audio_stream *streams,
				  uint8_t num_stream,
				  struct bt_audio_unicast_group **out_unicast_group)
{

	struct bt_audio_unicast_group *unicast_group;
	uint8_t index;

	CHECKIF(out_unicast_group == NULL) {
		BT_DBG("out_unicast_group is NULL");
		return -EINVAL;
	}
	/* Set out_unicast_group to NULL until the source has actually been created */
	*out_unicast_group = NULL;

	CHECKIF(streams == NULL) {
		BT_DBG("streams is NULL");
		return -EINVAL;
	}

	CHECKIF(num_stream > UNICAST_GROUP_STREAM_CNT) {
		BT_DBG("Too many streams provided: %u/%u",
		       num_stream, UNICAST_GROUP_STREAM_CNT);
		return -EINVAL;
	}

	unicast_group = NULL;
	for (index = 0; index < ARRAY_SIZE(unicast_groups); index++) {
		/* Find free entry */
		if (sys_slist_is_empty(&unicast_groups[index].streams)) {
			unicast_group = &unicast_groups[index];
			break;
		}
	}

	if (unicast_group == NULL) {
		BT_DBG("Could not allocate any more unicast groups");
		return -ENOMEM;
	}

	for (uint8_t i = 0; i < num_stream; i++) {
		sys_slist_t *group_streams = &unicast_group->streams;
		struct bt_audio_stream *stream;

		stream = &streams[i];

		if (stream->group != NULL) {
			BT_DBG("Channel[%u] (%p) already part of group %p",
			       i, stream, stream->group);

			/* Cleanup */
			for (uint8_t j = 0; j < i; j++) {
				stream = &streams[j];

				(void)sys_slist_find_and_remove(group_streams,
								&stream->node);
				stream->unicast_group = NULL;
			}
			return -EALREADY;
		}

		stream->unicast_group = unicast_group;
		sys_slist_append(group_streams, &stream->node);
	}

	*out_unicast_group = unicast_group;

	return 0;
}

int bt_audio_unicast_group_delete(struct bt_audio_unicast_group *unicast_group)
{
	struct bt_audio_stream *stream;

	CHECKIF(unicast_group == NULL) {
		BT_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, stream, node) {
		if (stream->group == NULL) {
			BT_DBG("stream %p not in a group", stream);
			return -EINVAL;
		}

		stream->unicast_group = NULL;
	}

	/* If all streams are idle, then the CIG has also been terminated */
	__ASSERT(unicast_group->cig == NULL, "CIG shall be NULL");

	(void)memset(unicast_group, 0, sizeof(*unicast_group));

	return 0;
}

#endif /* CONFIG_BT_BAP */
