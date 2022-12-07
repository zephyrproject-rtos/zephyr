/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>

#if defined(CONFIG_BT_AUDIO_UNICAST)
static void cap_stream_configured_cb(struct bt_audio_stream *bap_stream,
				     const struct bt_codec_qos_pref *pref)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->configured != NULL) {
		ops->configured(bap_stream, pref);
	}
}

static void cap_stream_qos_set_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->qos_set != NULL) {
		ops->qos_set(bap_stream);
	}
}

static void cap_stream_enabled_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->enabled != NULL) {
		ops->enabled(bap_stream);
	}
}

static void cap_stream_metadata_updated_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->metadata_updated != NULL) {
		ops->metadata_updated(bap_stream);
	}
}

static void cap_stream_disabled_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->disabled != NULL) {
		ops->disabled(bap_stream);
	}
}

static void cap_stream_released_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->released != NULL) {
		ops->released(bap_stream);
	}
}

#endif /* CONFIG_BT_AUDIO_UNICAST */

static void cap_stream_started_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->started != NULL) {
		ops->started(bap_stream);
	}
}

static void cap_stream_stopped_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(bap_stream);
	}
}

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static void cap_stream_recv_cb(struct bt_audio_stream *bap_stream,
			       const struct bt_iso_recv_info *info,
			       struct net_buf *buf)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(bap_stream, info, buf);
	}
}
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static void cap_stream_sent_cb(struct bt_audio_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_audio_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(bap_stream);
	}
}
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SOURCE */

static struct bt_audio_stream_ops bap_stream_ops = {
#if defined(CONFIG_BT_AUDIO_UNICAST)
	.configured = cap_stream_configured_cb,
	.qos_set = cap_stream_qos_set_cb,
	.enabled = cap_stream_enabled_cb,
	.metadata_updated = cap_stream_metadata_updated_cb,
	.disabled = cap_stream_disabled_cb,
	.released = cap_stream_released_cb,
#endif /* CONFIG_BT_AUDIO_UNICAST */
	.started = cap_stream_started_cb,
	.stopped = cap_stream_stopped_cb,
#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	.recv = cap_stream_recv_cb,
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */
#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	.sent = cap_stream_sent_cb,
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SOURCE */
};

void bt_cap_stream_ops_register(struct bt_cap_stream *stream,
				struct bt_audio_stream_ops *ops)
{
	stream->ops = ops;
	bt_audio_stream_cb_register(&stream->bap_stream, &bap_stream_ops);
}
