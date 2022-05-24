/*  Bluetooth Audio Stream */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>

#include "../host/conn_internal.h"
#include "../host/iso_internal.h"

#include "endpoint.h"
#include "unicast_client_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM)
#define LOG_MODULE_NAME bt_audio_stream
#include "common/log.h"

void bt_audio_codec_qos_to_iso_qos(struct bt_iso_chan_io_qos *io,
				  const struct bt_codec_qos *codec)
{
	io->sdu = codec->sdu;
	io->phy = codec->phy;
	io->rtn = codec->rtn;
}

void bt_audio_stream_attach(struct bt_conn *conn,
			    struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep,
			    struct bt_codec *codec)
{
	BT_DBG("conn %p stream %p ep %p codec %p", conn, stream, ep, codec);

	if (conn != NULL) {
		__ASSERT(stream->conn == NULL || stream->conn == conn,
			 "stream->conn already attached");
		stream->conn = bt_conn_ref(conn);
	}
	stream->codec = codec;
	stream->ep = ep;
	ep->stream = stream;

	if (stream->iso == NULL) {
		stream->iso = &ep->iso->iso_chan;
	}
}

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
int bt_audio_stream_send(struct bt_audio_stream *stream, struct net_buf *buf,
			 uint32_t seq_num, uint32_t ts)
{
	struct bt_audio_ep *ep;

	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	ep = stream->ep;

	if (ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		BT_DBG("Channel %p not ready for streaming (state: %s)",
		       stream, bt_audio_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	/* TODO: Add checks for broadcast sink */

	return bt_iso_chan_send(stream->iso, buf, seq_num, ts);
}
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_UNICAST)
#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static struct bt_audio_unicast_group unicast_groups[UNICAST_GROUP_CNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
static struct bt_audio_stream *enabling[CONFIG_BT_ISO_MAX_CHAN];

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

			BT_DBG("iso_chan %p", *iso_chan);

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
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */

bool bt_audio_valid_qos(const struct bt_codec_qos *qos)
{
	if (qos->interval < BT_ISO_SDU_INTERVAL_MIN ||
	    qos->interval > BT_ISO_SDU_INTERVAL_MAX) {
		BT_DBG("Interval not within allowed range: %u (%u-%u)",
		       qos->interval, BT_ISO_SDU_INTERVAL_MIN, BT_ISO_SDU_INTERVAL_MAX);
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
	return (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_SOURCE) &&
		bt_audio_ep_is_broadcast_src(stream->ep)) ||
	       (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_SINK) &&
		bt_audio_ep_is_broadcast_snk(stream->ep));
}

bool bt_audio_valid_stream_qos(const struct bt_audio_stream *stream,
			       const struct bt_codec_qos *qos)
{
	const struct bt_codec_qos_pref *qos_pref = &stream->ep->qos_pref;

	if (qos_pref->latency < qos->latency) {
		/* Latency is a preferred value. Print debug info but do not fail. */
		BT_DBG("Latency %u higher than preferred max %u",
			qos->latency, qos_pref->latency);
	}

	if (!IN_RANGE(qos->pd, qos_pref->pd_min, qos_pref->pd_max)) {
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

	if (stream->conn != NULL) {
		bt_conn_unref(stream->conn);
		stream->conn = NULL;
	}
	stream->codec = NULL;
	stream->ep->stream = NULL;
	stream->ep = NULL;

	if (!is_broadcast) {
		bt_audio_stream_disconnect(stream);
	}
}

int bt_audio_stream_disconnect(struct bt_audio_stream *stream)
{
	BT_DBG("stream %p iso %p", stream, stream->iso);

	if (stream == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
	/* Stop listening */
	for (size_t i = 0; i < ARRAY_SIZE(enabling); i++) {
		if (enabling[i] == stream) {
			enabling[i] = NULL;
			break;
		}
	}
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */

	if (stream->iso == NULL || stream->iso->iso == NULL) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(stream->iso);
}

void bt_audio_stream_reset(struct bt_audio_stream *stream)
{
	BT_DBG("stream %p", stream);

	if (stream == NULL) {
		return;
	}

	bt_audio_stream_detach(stream);
}

void bt_audio_stream_cb_register(struct bt_audio_stream *stream,
				 struct bt_audio_stream_ops *ops)
{
	stream->ops = ops;
}

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

int bt_audio_stream_config(struct bt_conn *conn,
			   struct bt_audio_stream *stream,
			   struct bt_audio_ep *ep,
			   struct bt_codec *codec)
{
	uint8_t role;
	int err;

	BT_DBG("conn %p stream %p, ep %p codec %p codec id 0x%02x "
	       "codec cid 0x%04x codec vid 0x%04x", conn, stream, ep,
	       codec, codec ? codec->id : 0, codec ? codec->cid : 0,
	       codec ? codec->vid : 0);

	CHECKIF(conn == NULL || stream == NULL || codec == NULL) {
		BT_DBG("NULL value(s) supplied)");
		return -EINVAL;
	}

	role = conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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
		return -EBADMSG;
	}

	bt_audio_stream_attach(conn, stream, ep, codec);

	err = bt_unicast_client_config(stream, codec);
	if (err != 0) {
		BT_DBG("Failed to configure stream: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_reconfig(struct bt_audio_stream *stream,
			     struct bt_codec *codec)
{
	struct bt_audio_ep *ep;
	uint8_t role;
	int err;

	BT_DBG("stream %p codec %p", stream, codec);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	ep = stream->ep;

	if (codec == NULL) {
		BT_DBG("NULL codec");
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
		return -EBADMSG;
	}

	bt_audio_stream_attach(stream->conn, stream, ep, codec);

	err = bt_unicast_client_config(stream, codec);
	if (err) {
		return err;
	}

	stream->codec = codec;

	return 0;
}

static void bt_audio_codec_qos_to_cig_param(struct bt_iso_cig_param *cig_param,
					    const struct bt_codec_qos *qos)
{
	cig_param->framing = qos->framing;
	cig_param->packing = BT_ISO_PACKING_SEQUENTIAL; /*  TODO: Add to QoS struct */
	cig_param->interval = qos->interval;
	cig_param->latency = qos->latency;
	cig_param->sca = BT_GAP_SCA_UNKNOWN;
}

static int bt_audio_cig_create(struct bt_audio_unicast_group *group,
			       const struct bt_codec_qos *qos)
{
	struct bt_iso_cig_param param;
	struct bt_audio_stream *stream;
	uint8_t cis_count;
	int err;

	BT_DBG("group %p qos %p", group, qos);

	cis_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		if (stream->iso != NULL) {
			bool already_added = false;

			for (size_t i = 0U; i < cis_count; i++) {
				if (group->cis[i] == stream->iso) {
					already_added = true;
					break;
				}
			}

			if (!already_added) {
				group->cis[cis_count++] = stream->iso;
			}
		}
	}

	param.num_cis = cis_count;
	param.cis_channels = group->cis;
	bt_audio_codec_qos_to_cig_param(&param, qos);

	err = bt_iso_cig_create(&param, &group->cig);
	if (err != 0) {
		BT_ERR("bt_iso_cig_create failed: %d", err);
		return err;
	}

	group->qos = qos;

	return 0;
}

static int bt_audio_cig_reconfigure(struct bt_audio_unicast_group *group,
				    const struct bt_codec_qos *qos)
{
	struct bt_iso_cig_param param;
	struct bt_audio_stream *stream;
	uint8_t cis_count;
	int err;

	BT_DBG("group %p qos %p", group, qos);

	cis_count = 0U;
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		group->cis[cis_count++] = stream->iso;
	}

	param.num_cis = cis_count;
	param.cis_channels = group->cis;
	bt_audio_codec_qos_to_cig_param(&param, qos);

	err = bt_iso_cig_reconfigure(group->cig, &param);
	if (err != 0) {
		BT_ERR("bt_iso_cig_create failed: %d", err);
		return err;
	}

	group->qos = qos;

	return 0;
}

static void audio_stream_qos_cleanup(const struct bt_conn *conn,
				     struct bt_audio_unicast_group *group)
{
	struct bt_audio_stream *stream;

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		if (stream->conn != conn && stream->ep != NULL) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		bt_unicast_client_ep_unbind_audio_iso(stream->ep);
	}
}

int bt_audio_stream_qos(struct bt_conn *conn,
			struct bt_audio_unicast_group *group)
{
	struct bt_audio_stream *stream;
	struct net_buf_simple *buf;
	struct bt_ascs_qos_op *op;
	struct bt_audio_ep *ep;
	bool conn_stream_found;
	bool cig_connected;
	uint8_t role;
	int err;

	BT_DBG("conn %p group %p", conn, group);

	CHECKIF(conn == NULL) {
		BT_DBG("conn is NULL");
		return -EINVAL;
	}

	CHECKIF(group == NULL) {
		BT_DBG("group is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&group->streams)) {
		BT_DBG("group stream list is empty");
		return -ENOEXEC;
	}

	role = conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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
		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}
		conn_stream_found = true;

		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}
		conn_stream_found = true;

		ep = stream->ep;
		if (ep == NULL) {
			BT_DBG("stream->ep is NULL");
			return -EINVAL;
		}

		/* Can only be done if all the streams are in the codec
		 * configured state or the QoS configured state
		 */
		switch (ep->status.state) {
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			break;
		default:
			BT_DBG("Invalid state: %s",
			       bt_audio_ep_state_str(stream->ep->status.state));
			return -EINVAL;
		}

		if (!bt_audio_valid_stream_qos(stream, stream->qos)) {
			return -EINVAL;
		}

		/* Verify ep->dir */
		switch (ep->dir) {
		case BT_AUDIO_DIR_SINK:
		case BT_AUDIO_DIR_SOURCE:
			break;
		default:
			__ASSERT(false, "invalid endpoint dir: %u", ep->dir);
			return -EINVAL;
		}

		if (bt_unicast_client_audio_iso_by_stream(stream) == NULL) {
			/* This can only happen if the stream was somehow added
			 * to a group without the audio_iso being bound to it
			 */
			BT_ERR("Could not find audio_iso for stream %p", stream);
			return -EINVAL;
		}
	}

	if (!conn_stream_found) {
		BT_DBG("No streams in the group %p for conn %p", group, conn);
		return -EINVAL;
	}

	/* Setup endpoints before starting the QoS execution */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		struct bt_audio_iso *audio_iso;

		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		audio_iso = bt_unicast_client_audio_iso_by_stream(stream);
		/* Don't need to check for audio_iso validity as we have already verified that */

		bt_unicast_client_ep_bind_audio_iso(stream->ep, audio_iso);
	}

	/* Generate the control point write */
	buf = bt_unicast_client_ep_create_pdu(BT_ASCS_QOS_OP);

	op = net_buf_simple_add(buf, sizeof(*op));

	(void)memset(op, 0, sizeof(*op));
	ep = NULL; /* Needed to find the control point handle */
	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, node) {
		if (stream->conn != conn) {
			/* Channel not part of this ACL, skip */
			continue;
		}

		op->num_ases++;

		err = bt_unicast_client_ep_qos(stream->ep, buf, stream->qos);
		if (err) {
			audio_stream_qos_cleanup(conn, group);
			return err;
		}

		if (ep == NULL) {
			ep = stream->ep;
		}
	}

	err = bt_unicast_client_ep_send(conn, ep, buf);
	if (err != 0) {
		BT_DBG("Could not send config QoS: %d", err);
		audio_stream_qos_cleanup(conn, group);
		return err;
	}

	return 0;
}

int bt_audio_stream_enable(struct bt_audio_stream *stream,
			   struct bt_codec_data *meta,
			   size_t meta_count)
{
	uint8_t role;
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (stream->ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	err = bt_unicast_client_enable(stream, meta, meta_count);
	if (err != 0) {
		BT_DBG("Failed to enable stream: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_metadata(struct bt_audio_stream *stream,
			     struct bt_codec_data *meta,
			     size_t meta_count)
{
	uint8_t role;
	int err;

	BT_DBG("stream %p metadata count %u", stream, meta_count);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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

	err = bt_unicast_client_metadata(stream, meta, meta_count);
	if (err != 0) {
		BT_DBG("Updating metadata failed: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_disable(struct bt_audio_stream *stream)
{
	uint8_t role;
	int err;

	BT_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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

	err = bt_unicast_client_disable(stream);
	if (err != 0) {
		BT_DBG("Disabling stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_start(struct bt_audio_stream *stream)
{
	uint8_t role;
	int err;

	BT_DBG("stream %p ep %p", stream, stream == NULL ? NULL : stream->ep);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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

	err = bt_unicast_client_start(stream);
	if (err != 0) {
		BT_DBG("Starting stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_stop(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep;
	uint8_t role;
	int err;

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	ep = stream->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Disabling) */
	case BT_AUDIO_EP_STATE_DISABLING:
		break;
	default:
		BT_ERR("Invalid state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	err = bt_unicast_client_stop(stream);
	if (err != 0) {
		BT_DBG("Stopping stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_stream_release(struct bt_audio_stream *stream, bool cache)
{
	uint8_t role;
	int err;

	BT_DBG("stream %p cache %s", stream, cache ? "true" : "false");

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		BT_DBG("Invalid stream");
		return -EINVAL;
	}

	role = stream->conn->role;
	if (role != BT_HCI_ROLE_CENTRAL) {
		BT_DBG("Invalid conn role: %u, shall be central", role);
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

	err = bt_unicast_client_release(stream);
	if (err != 0) {
		BT_DBG("Stopping stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_audio_cig_terminate(struct bt_audio_unicast_group *group)
{
	BT_DBG("group %p", group);

	return bt_iso_cig_terminate(group->cig);
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
	case BT_ISO_STATE_DISCONNECTED:
		return bt_iso_chan_connect(&param, 1);
	case BT_ISO_STATE_CONNECTING:
		return 0;
	case BT_ISO_STATE_CONNECTED:
		return -EALREADY;
	default:
		return bt_iso_chan_connect(&param, 1);
	}
}

static bool unicast_group_valid_qos(const struct bt_codec_qos *group_qos,
				    const struct bt_codec_qos *stream_qos)
{
	if (group_qos->framing != stream_qos->framing ||
	    group_qos->interval != stream_qos->interval ||
	    group_qos->latency != stream_qos->latency) {
		return false;
	}

	return true;
}

static void unicast_group_param_cleanup(struct bt_audio_unicast_group *group,
					const struct bt_audio_unicast_group_param params[],
					size_t num_param)
{
	for (size_t i = 0U; i < num_param; i++) {
		if (!sys_slist_find_and_remove(&group->streams, &params[i].stream->node)) {
			/* We can stop once `sys_slist_find_and_remove` fails as
			 * that means that the this and the following streams
			 * were never added to the group
			 */
			return;
		}
	}
}

int bt_audio_unicast_group_create(struct bt_audio_unicast_group_param params[],
				  size_t num_param,
				  struct bt_audio_unicast_group **out_unicast_group)
{
	struct bt_audio_unicast_group *unicast_group;
	const struct bt_codec_qos *group_qos = NULL;
	int err;

	CHECKIF(out_unicast_group == NULL) {
		BT_DBG("out_unicast_group is NULL");
		return -EINVAL;
	}
	/* Set out_unicast_group to NULL until the source has actually been created */
	*out_unicast_group = NULL;

	CHECKIF(params == NULL) {
		BT_DBG("streams is NULL");
		return -EINVAL;
	}

	CHECKIF(num_param > UNICAST_GROUP_STREAM_CNT) {
		BT_DBG("Too many streams provided: %u/%u",
		       num_param, UNICAST_GROUP_STREAM_CNT);
		return -EINVAL;
	}

	for (size_t i = 0U; i < num_param; i++) {
		CHECKIF(params[i].stream == NULL ||
			params[i].qos == NULL ||
			(params[i].dir != BT_AUDIO_DIR_SINK &&
			 params[i].dir != BT_AUDIO_DIR_SOURCE)) {
			BT_DBG("Invalid params[%zu] values", i);
			return -EINVAL;
		}

		if (params[i].stream->group != NULL) {
			BT_DBG("params[%zu] stream (%p) already part of group %p",
			       i, params[i].stream, params[i].stream->group);
			return -EALREADY;
		}

		if (group_qos == NULL) {
			group_qos = params[i].qos;
		} else if (!unicast_group_valid_qos(group_qos, params[i].qos)) {
			BT_DBG("Stream[%zu] QoS incompatible with group QoS", i);
			return -EINVAL;
		}

		CHECKIF(!bt_audio_valid_qos(params[i].qos)) {
			BT_DBG("Invalid QoS");
			return -EINVAL;
		}
	}

	unicast_group = NULL;
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_groups); i++) {
		/* Find free entry */
		if (!unicast_groups[i].allocated) {
			unicast_group = &unicast_groups[i];
			unicast_group->index = i;
			break;
		}
	}

	if (unicast_group == NULL) {
		BT_DBG("Could not allocate any more unicast groups");
		return -ENOMEM;
	}

	unicast_group->allocated = true;
	for (size_t i = 0U; i < num_param; i++) {
		sys_slist_t *group_streams = &unicast_group->streams;
		const enum bt_audio_dir dir = params[i].dir;
		struct bt_iso_chan_qos *iso_qos;
		struct bt_audio_stream *stream = params[i].stream;
		struct bt_audio_iso *audio_iso;
		struct bt_iso_chan_io_qos *io;

		audio_iso = bt_unicast_client_new_audio_iso(unicast_group, stream, dir);
		if (audio_iso == NULL) {
			unicast_group_param_cleanup(unicast_group, params, num_param);

			return -ENOMEM;
		}

		bt_unicast_client_stream_bind_audio_iso(stream, audio_iso, dir);

		iso_qos = &audio_iso->iso_qos;

		if (dir == BT_AUDIO_DIR_SINK) {
			/* If the endpoint is a sink, then we need to
			 * configure our TX parameters
			 */
			io = iso_qos->tx;
		} else {
			/* If the endpoint is a source, then we need to
			 * configure our RX parameters
			 */
			io = iso_qos->rx;
		}

		bt_audio_codec_qos_to_iso_qos(io, params[i].qos);
		stream->unicast_group = unicast_group;
		stream->qos = params[i].qos;
		sys_slist_append(group_streams, &stream->node);

		BT_DBG("Added stream %p to group %p", stream, unicast_group);
	}

	err = bt_audio_cig_create(unicast_group, group_qos);
	if (err != 0) {
		BT_DBG("bt_audio_cig_create failed: %d", err);
		unicast_group_param_cleanup(unicast_group, params, num_param);

		return err;
	}

	*out_unicast_group = unicast_group;

	return 0;
}

int bt_audio_unicast_group_add_streams(struct bt_audio_unicast_group *unicast_group,
				       struct bt_audio_unicast_group_param params[],
				       size_t num_param)
{
	const struct bt_codec_qos *group_qos = unicast_group->qos;
	struct bt_audio_stream *tmp_stream;
	size_t total_stream_cnt;
	struct bt_iso_cig *cig;
	int err;

	CHECKIF(unicast_group == NULL) {
		BT_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	CHECKIF(params == NULL) {
		BT_DBG("params is NULL");
		return -EINVAL;
	}

	CHECKIF(num_param == 0) {
		BT_DBG("num_param is 0");
		return -EINVAL;
	}

	for (size_t i = 0U; i < num_param; i++) {
		CHECKIF(params[i].stream == NULL ||
			params[i].qos == NULL ||
			(params[i].dir != BT_AUDIO_DIR_SINK &&
			 params[i].dir != BT_AUDIO_DIR_SOURCE)) {
			BT_DBG("Invalid params[%zu] values", i);
			return -EINVAL;
		}

		if (params[i].stream->group != NULL) {
			BT_DBG("params[%zu] stream (%p) already part of group %p",
			       i, params[i].stream, params[i].stream->group);
			return -EALREADY;
		}

		if (group_qos == NULL) {
			group_qos = params[i].qos;
		} else if (!unicast_group_valid_qos(group_qos, params[i].qos)) {
			BT_DBG("Stream[%zu] QoS incompatible with group QoS", i);
			return -EINVAL;
		}
	}

	total_stream_cnt = num_param;
	SYS_SLIST_FOR_EACH_CONTAINER(&unicast_group->streams, tmp_stream, node) {
		total_stream_cnt++;
	}

	if (total_stream_cnt > UNICAST_GROUP_STREAM_CNT) {
		BT_DBG("Too many streams provided: %u/%u",
		       total_stream_cnt, UNICAST_GROUP_STREAM_CNT);
		return -EINVAL;

	}

	/* We can just check the CIG state to see if any streams have started as
	 * that would start the ISO connection procedure
	 */
	cig = unicast_group->cig;
	if (cig != NULL && cig->state != BT_ISO_CIG_STATE_CONFIGURED) {
		BT_DBG("At least one unicast group stream is started");
		return -EBADMSG;
	}

	for (size_t i = 0U; i < num_param; i++) {
		sys_slist_t *group_streams = &unicast_group->streams;
		struct bt_audio_stream *stream = params[i].stream;

		stream->unicast_group = unicast_group;
		sys_slist_append(group_streams, &stream->node);

		BT_DBG("Added stream %p to group %p", stream, unicast_group);
	}

	err = bt_audio_cig_reconfigure(unicast_group, group_qos);
	if (err != 0) {
		BT_DBG("bt_audio_cig_reconfigure failed: %d", err);
		unicast_group_param_cleanup(unicast_group, params, num_param);

		return err;
	}

	return 0;
}

int bt_audio_unicast_group_delete(struct bt_audio_unicast_group *unicast_group)
{
	struct bt_audio_stream *stream, *tmp;

	CHECKIF(unicast_group == NULL) {
		BT_DBG("unicast_group is NULL");
		return -EINVAL;
	}

	if (unicast_group->cig != NULL) {
		const int err = bt_audio_cig_terminate(unicast_group);

		if (err != 0) {
			BT_DBG("bt_audio_cig_terminate failed with err %d",
			       err);

			return err;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&unicast_group->streams, stream, tmp, node) {
		stream->unicast_group = NULL;
		sys_slist_remove(&unicast_group->streams, NULL, &stream->node);
		bt_unicast_client_stream_unbind_audio_iso(stream);
	}

	(void)memset(unicast_group, 0, sizeof(*unicast_group));

	return 0;
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
#endif /* CONFIG_BT_AUDIO_UNICAST */
