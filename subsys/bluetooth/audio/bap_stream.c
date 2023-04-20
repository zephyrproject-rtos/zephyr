/*  Bluetooth Audio Stream */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#include "../host/iso_internal.h"

#include "bap_iso.h"
#include "audio_internal.h"
#include "bap_endpoint.h"
#include "bap_unicast_client_internal.h"
#include "bap_unicast_server.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_stream, CONFIG_BT_BAP_STREAM_LOG_LEVEL);

static uint8_t pack_bt_codec_cc(const struct bt_codec *codec, uint8_t cc[])
{
	uint8_t len;

	len = 0U;
	for (size_t i = 0U; i < codec->data_count; i++) {
		const struct bt_data *data = &codec->data[i].data;

		/* We assume that data_len and data has previously been verified
		 * and that based on the Kconfigs we can assume that the length
		 * will always fit in `cc`
		 */
		cc[len++] = data->data_len + 1;
		cc[len++] = data->type;
		(void)memcpy(cc + len, data->data, data->data_len);
		len += data->data_len;
	}

	return len;
}

void bt_audio_codec_to_iso_path(struct bt_iso_chan_path *path,
				const struct bt_codec *codec)
{
	path->pid = codec->path_id;
	path->format = codec->id;
	path->cid = codec->cid;
	path->vid = codec->vid;
	path->delay = 0; /* TODO: Add to bt_codec? Use presentation delay? */
	path->cc_len = pack_bt_codec_cc(codec, path->cc);
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT) || defined(CONFIG_BT_BAP_BROADCAST_SOURCE) ||            \
	defined(CONFIG_BT_BAP_BROADCAST_SINK)
void bt_audio_codec_qos_to_iso_qos(struct bt_iso_chan_io_qos *io,
				   const struct bt_codec_qos *codec_qos)
{
	io->sdu = codec_qos->sdu;
	io->phy = codec_qos->phy;
	io->rtn = codec_qos->rtn;
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT ||                                                          \
	* CONFIG_BT_BAP_BROADCAST_SOURCE ||                                                        \
	* CONFIG_BT_BAP_BROADCAST_SINK                                                             \
	*/

void bt_bap_stream_init(struct bt_bap_stream *stream)
{
	struct bt_bap_stream_ops *ops;
	void *user_data;

	/* Save the stream->ops and stream->user_data owned by API user */
	ops = stream->ops;
	user_data = stream->user_data;

	memset(stream, 0, sizeof(*stream));

	/* Restore */
	stream->ops = ops;
	stream->user_data = user_data;
}

void bt_bap_stream_attach(struct bt_conn *conn, struct bt_bap_stream *stream, struct bt_bap_ep *ep,
			  struct bt_codec *codec)
{
	LOG_DBG("conn %p stream %p ep %p codec %p", (void *)conn, stream, ep, codec);

	if (conn != NULL) {
		__ASSERT(stream->conn == NULL || stream->conn == conn,
			 "stream->conn %p already attached", (void *)stream->conn);
		if (stream->conn == NULL) {
			stream->conn = bt_conn_ref(conn);
		}
	}
	stream->codec = codec;
	stream->ep = ep;
	ep->stream = stream;
}

struct bt_iso_chan *bt_bap_stream_iso_chan_get(struct bt_bap_stream *stream)
{
	if (stream != NULL && stream->ep != NULL && stream->ep->iso != NULL) {
		return &stream->ep->iso->chan;
	}

	return NULL;
}

void bt_bap_stream_cb_register(struct bt_bap_stream *stream,
				 struct bt_bap_stream_ops *ops)
{
	stream->ops = ops;
}

int bt_bap_ep_get_info(const struct bt_bap_ep *ep, struct bt_bap_ep_info *info)
{
	info->id = ep->status.id;
	info->state = ep->status.state;
	info->dir = ep->dir;

	return 0;
}

#if defined(CONFIG_BT_AUDIO_TX)
enum bt_bap_ascs_reason bt_audio_verify_qos(const struct bt_codec_qos *qos)
{
	if (qos->interval < BT_ISO_SDU_INTERVAL_MIN ||
	    qos->interval > BT_ISO_SDU_INTERVAL_MAX) {
		LOG_DBG("Interval not within allowed range: %u (%u-%u)", qos->interval,
			BT_ISO_SDU_INTERVAL_MIN, BT_ISO_SDU_INTERVAL_MAX);
		return BT_BAP_ASCS_REASON_INTERVAL;
	}

	if (qos->framing > BT_CODEC_QOS_FRAMED) {
		LOG_DBG("Invalid Framing 0x%02x", qos->framing);
		return BT_BAP_ASCS_REASON_FRAMING;
	}

	if (qos->phy != BT_CODEC_QOS_1M &&
	    qos->phy != BT_CODEC_QOS_2M &&
	    qos->phy != BT_CODEC_QOS_CODED) {
		LOG_DBG("Invalid PHY 0x%02x", qos->phy);
		return BT_BAP_ASCS_REASON_PHY;
	}

	if (qos->sdu > BT_ISO_MAX_SDU) {
		LOG_DBG("Invalid SDU %u", qos->sdu);
		return BT_BAP_ASCS_REASON_SDU;
	}

	if (qos->latency < BT_ISO_LATENCY_MIN ||
	    qos->latency > BT_ISO_LATENCY_MAX) {
		LOG_DBG("Invalid Latency %u", qos->latency);
		return BT_BAP_ASCS_REASON_LATENCY;
	}

	if (qos->pd > BT_AUDIO_PD_MAX) {
		LOG_DBG("Invalid presentation delay %u", qos->pd);
		return BT_BAP_ASCS_REASON_PD;
	}

	return BT_BAP_ASCS_REASON_NONE;
}

#if CONFIG_BT_CODEC_MAX_DATA_LEN > 0
bool bt_audio_valid_codec_data(const struct bt_codec_data *data)
{
	if (data->data.data_len > ARRAY_SIZE(data->value)) {
		LOG_DBG("data invalid length: %zu/%zu", data->data.data_len,
			ARRAY_SIZE(data->value));
		return false;
	}

	return true;
}
#endif /* CONFIG_BT_CODEC_MAX_DATA_LEN > 0 */

bool bt_audio_valid_codec(const struct bt_codec *codec)
{
	if (codec == NULL) {
		LOG_DBG("codec is NULL");
		return false;
	}

#if CONFIG_BT_CODEC_MAX_DATA_COUNT > 0
	if (codec->data_count > CONFIG_BT_CODEC_MAX_DATA_COUNT) {
		LOG_DBG("codec->data_count (%zu) is invalid", codec->data_count);
		return false;
	}

	for (size_t i = 0U; i < codec->data_count; i++) {
		if (!bt_audio_valid_codec_data(&codec->data[i])) {
			LOG_DBG("codec->data[%zu] invalid", i);
			return false;
		}
	}
#endif /* CONFIG_BT_CODEC_MAX_DATA_COUNT > 0 */

#if CONFIG_BT_CODEC_MAX_METADATA_COUNT > 0
	if (codec->meta_count > CONFIG_BT_CODEC_MAX_METADATA_COUNT) {
		LOG_DBG("codec->meta_count (%zu) is invalid", codec->meta_count);
		return false;
	}

	for (size_t i = 0U; i < codec->meta_count; i++) {
		if (!bt_audio_valid_codec_data(&codec->meta[i])) {
			LOG_DBG("codec->meta[%zu] invalid", i);
			return false;
		}
	}
#endif /* CONFIG_BT_CODEC_MAX_METADATA_COUNT > 0 */

	return true;
}

int bt_bap_stream_send(struct bt_bap_stream *stream, struct net_buf *buf,
			 uint16_t seq_num, uint32_t ts)
{
	struct bt_bap_ep *ep;

	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	ep = stream->ep;

	if (ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		LOG_DBG("Channel %p not ready for streaming (state: %s)", stream,
			bt_bap_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	/* TODO: Add checks for broadcast sink */

	return bt_iso_chan_send(bt_bap_stream_iso_chan_get(stream),
				buf, seq_num, ts);
}
#endif /* CONFIG_BT_AUDIO_TX */

#if defined(CONFIG_BT_BAP_UNICAST)
static bool bt_bap_stream_is_broadcast(const struct bt_bap_stream *stream)
{
	return (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SOURCE) &&
		bt_bap_ep_is_broadcast_src(stream->ep)) ||
	       (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK) && bt_bap_ep_is_broadcast_snk(stream->ep));
}

enum bt_bap_ascs_reason bt_bap_stream_verify_qos(const struct bt_bap_stream *stream,
						 const struct bt_codec_qos *qos)
{
	const struct bt_codec_qos_pref *qos_pref = &stream->ep->qos_pref;

	if (qos_pref->latency < qos->latency) {
		/* Latency is a preferred value. Print debug info but do not fail. */
		LOG_DBG("Latency %u higher than preferred max %u", qos->latency, qos_pref->latency);
	}

	if (!IN_RANGE(qos->pd, qos_pref->pd_min, qos_pref->pd_max)) {
		LOG_DBG("Presentation Delay not within range: min %u max %u pd %u",
			qos_pref->pd_min, qos_pref->pd_max, qos->pd);
		return BT_BAP_ASCS_REASON_PD;
	}

	return BT_BAP_ASCS_REASON_NONE;
}

void bt_bap_stream_detach(struct bt_bap_stream *stream)
{
	const bool is_broadcast = bt_bap_stream_is_broadcast(stream);

	LOG_DBG("stream %p", stream);

	if (stream->conn != NULL) {
		bt_conn_unref(stream->conn);
		stream->conn = NULL;
	}
	stream->codec = NULL;
	stream->ep->stream = NULL;
	stream->ep = NULL;

	if (!is_broadcast) {
		bt_bap_stream_disconnect(stream);
	}
}

int bt_bap_stream_disconnect(struct bt_bap_stream *stream)
{
	struct bt_iso_chan *iso_chan = bt_bap_stream_iso_chan_get(stream);

	LOG_DBG("stream %p iso %p", stream, iso_chan);

	if (stream == NULL) {
		return -EINVAL;
	}

	if (iso_chan == NULL || iso_chan->iso == NULL) {
		return -ENOTCONN;
	}

	return bt_iso_chan_disconnect(iso_chan);
}

void bt_bap_stream_reset(struct bt_bap_stream *stream)
{
	LOG_DBG("stream %p", stream);

	if (stream == NULL) {
		return;
	}

	if (stream->ep != NULL && stream->ep->iso != NULL) {
		bt_bap_iso_unbind_ep(stream->ep->iso, stream->ep);
	}

	bt_bap_stream_detach(stream);
}

static uint8_t conn_get_role(const struct bt_conn *conn)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	__ASSERT(err == 0, "Failed to get conn info");

	return info.role;
}

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)

int bt_bap_stream_config(struct bt_conn *conn, struct bt_bap_stream *stream, struct bt_bap_ep *ep,
			 struct bt_codec *codec)
{
	uint8_t role;
	int err;

	LOG_DBG("conn %p stream %p, ep %p codec %p codec id 0x%02x "
	       "codec cid 0x%04x codec vid 0x%04x", (void *)conn, stream, ep,
	       codec, codec ? codec->id : 0, codec ? codec->cid : 0,
	       codec ? codec->vid : 0);

	CHECKIF(conn == NULL || stream == NULL || codec == NULL) {
		LOG_DBG("NULL value(s) supplied)");
		return -EINVAL;
	}

	if (stream->conn != NULL) {
		LOG_DBG("Stream already configured for conn %p", (void *)stream->conn);
		return -EALREADY;
	}

	role = conn_get_role(conn);
	if (role != BT_HCI_ROLE_CENTRAL) {
		LOG_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_BAP_EP_STATE_IDLE:
		/* or 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	bt_bap_stream_attach(conn, stream, ep, codec);

	err = bt_bap_unicast_client_config(stream, codec);
	if (err != 0) {
		LOG_DBG("Failed to configure stream: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group)
{
	uint8_t role;
	int err;

	LOG_DBG("conn %p group %p", (void *)conn, group);

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	CHECKIF(group == NULL) {
		LOG_DBG("group is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&group->streams)) {
		LOG_DBG("group stream list is empty");
		return -ENOEXEC;
	}

	role = conn_get_role(conn);
	if (role != BT_HCI_ROLE_CENTRAL) {
		LOG_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	err = bt_bap_unicast_client_qos(conn, group);
	if (err != 0) {
		LOG_DBG("Failed to configure stream: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_enable(struct bt_bap_stream *stream,
			   struct bt_codec_data *meta,
			   size_t meta_count)
{
	uint8_t role;
	int err;

	LOG_DBG("stream %p", stream);

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	role = conn_get_role(stream->conn);
	if (role != BT_HCI_ROLE_CENTRAL) {
		LOG_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (stream->ep->status.state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(stream->ep->status.state));
		return -EBADMSG;
	}

	err = bt_bap_unicast_client_enable(stream, meta, meta_count);
	if (err != 0) {
		LOG_DBG("Failed to enable stream: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_stop(struct bt_bap_stream *stream)
{
	struct bt_bap_ep *ep;
	uint8_t role;
	int err;

	if (stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	role = conn_get_role(stream->conn);
	if (role != BT_HCI_ROLE_CENTRAL) {
		LOG_DBG("Invalid conn role: %u, shall be central", role);
		return -EINVAL;
	}

	ep = stream->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Disabling) */
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	err = bt_bap_unicast_client_stop(stream);
	if (err != 0) {
		LOG_DBG("Stopping stream failed: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

int bt_bap_stream_reconfig(struct bt_bap_stream *stream,
			     struct bt_codec *codec)
{
	uint8_t state;
	uint8_t role;
	int err;

	LOG_DBG("stream %p codec %p", stream, codec);

	CHECKIF(stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	CHECKIF(codec == NULL) {
		LOG_DBG("codec is NULL");
		return -EINVAL;
	}

	state = stream->ep->status.state;
	switch (state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_BAP_EP_STATE_IDLE:
		/* or 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(state));
		return -EBADMSG;
	}

	role = conn_get_role(stream->conn);
	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) && role == BT_HCI_ROLE_CENTRAL) {
		err = bt_bap_unicast_client_config(stream, codec);
	} else if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && role == BT_HCI_ROLE_PERIPHERAL) {
		err = bt_bap_unicast_server_reconfig(stream, codec);
	} else {
		err = -EOPNOTSUPP;
	}

	if (err != 0) {
		LOG_DBG("reconfiguring stream failed: %d", err);
	} else {
		stream->codec = codec;
	}

	return 0;
}

int bt_bap_stream_start(struct bt_bap_stream *stream)
{
	uint8_t state;
	uint8_t role;
	int err;

	LOG_DBG("stream %p ep %p", stream, stream == NULL ? NULL : stream->ep);

	CHECKIF(stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	state = stream->ep->status.state;
	switch (state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(state));
		return -EBADMSG;
	}

	role = conn_get_role(stream->conn);
	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) && role == BT_HCI_ROLE_CENTRAL) {
		err = bt_bap_unicast_client_start(stream);
	} else if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && role == BT_HCI_ROLE_PERIPHERAL) {
		err = bt_bap_unicast_server_start(stream);
	} else {
		err = -EOPNOTSUPP;
	}

	if (err != 0) {
		LOG_DBG("Starting stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_metadata(struct bt_bap_stream *stream,
			     struct bt_codec_data *meta,
			     size_t meta_count)
{
	uint8_t state;
	uint8_t role;
	int err;

	LOG_DBG("stream %p metadata count %u", stream, meta_count);

	CHECKIF(stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	CHECKIF((meta == NULL && meta_count != 0U) ||
		(meta != NULL && meta_count == 0U)) {
		LOG_DBG("Invalid meta (%p) or count (%zu)", meta, meta_count);
		return -EINVAL;
	}

	state = stream->ep->status.state;
	switch (state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(state));
		return -EBADMSG;
	}

	role = conn_get_role(stream->conn);
	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) && role == BT_HCI_ROLE_CENTRAL) {
		err = bt_bap_unicast_client_metadata(stream, meta, meta_count);
	} else if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && role == BT_HCI_ROLE_PERIPHERAL) {
		err = bt_bap_unicast_server_metadata(stream, meta, meta_count);
	} else {
		err = -EOPNOTSUPP;
	}

	if (err != 0) {
		LOG_DBG("Updating metadata failed: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_disable(struct bt_bap_stream *stream)
{
	uint8_t state;
	uint8_t role;
	int err;

	LOG_DBG("stream %p", stream);

	CHECKIF(stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	state = stream->ep->status.state;
	switch (state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(state));
		return -EBADMSG;
	}

	role = conn_get_role(stream->conn);
	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) && role == BT_HCI_ROLE_CENTRAL) {
		err = bt_bap_unicast_client_disable(stream);
	} else if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && role == BT_HCI_ROLE_PERIPHERAL) {
		err = bt_bap_unicast_server_disable(stream);
	} else {
		err = -EOPNOTSUPP;
	}

	if (err != 0) {
		LOG_DBG("Disabling stream failed: %d", err);
		return err;
	}

	return 0;
}

int bt_bap_stream_release(struct bt_bap_stream *stream)
{
	uint8_t state;
	uint8_t role;
	int err;

	LOG_DBG("stream %p", stream);

	CHECKIF(stream == NULL || stream->ep == NULL || stream->conn == NULL) {
		LOG_DBG("Invalid stream");
		return -EINVAL;
	}

	state = stream->ep->status.state;
	switch (state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		/* or 0x02 (QoS Configured) */
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		/* or 0x03 (Enabling) */
	case BT_BAP_EP_STATE_ENABLING:
		/* or 0x04 (Streaming) */
	case BT_BAP_EP_STATE_STREAMING:
		/* or 0x04 (Disabling) */
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		LOG_ERR("Invalid state: %s", bt_bap_ep_state_str(state));
		return -EBADMSG;
	}

	role = conn_get_role(stream->conn);
	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT) && role == BT_HCI_ROLE_CENTRAL) {
		err = bt_bap_unicast_client_release(stream);
	} else if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER) && role == BT_HCI_ROLE_PERIPHERAL) {
		err = bt_bap_unicast_server_release(stream);
	} else {
		err = -EOPNOTSUPP;
	}

	if (err != 0) {
		LOG_DBG("Releasing stream failed: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_BAP_UNICAST */
