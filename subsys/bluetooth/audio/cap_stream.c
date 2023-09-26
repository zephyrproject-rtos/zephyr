/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/sys/check.h>

#include "cap_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_stream, CONFIG_BT_CAP_STREAM_LOG_LEVEL);

#if defined(CONFIG_BT_BAP_UNICAST)
static void cap_stream_configured_cb(struct bt_bap_stream *bap_stream,
				     const struct bt_audio_codec_qos_pref *pref)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		bt_cap_initiator_codec_configured(cap_stream);
	}

	if (ops != NULL && ops->configured != NULL) {
		ops->configured(bap_stream, pref);
	}
}

static void cap_stream_qos_set_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		bt_cap_initiator_qos_configured(cap_stream);
	}

	if (ops != NULL && ops->qos_set != NULL) {
		ops->qos_set(bap_stream);
	}
}

static void cap_stream_enabled_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		bt_cap_initiator_enabled(cap_stream);
	}

	if (ops != NULL && ops->enabled != NULL) {
		ops->enabled(bap_stream);
	}
}

static void cap_stream_metadata_updated_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		bt_cap_initiator_metadata_updated(cap_stream);
	}

	if (ops != NULL && ops->metadata_updated != NULL) {
		ops->metadata_updated(bap_stream);
	}
}

static void cap_stream_disabled_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (ops != NULL && ops->disabled != NULL) {
		ops->disabled(bap_stream);
	}
}

static void cap_stream_released_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR)) {
		bt_cap_initiator_released(cap_stream);
	}

	if (ops != NULL && ops->released != NULL) {
		ops->released(bap_stream);
	}
}

#endif /* CONFIG_BT_BAP_UNICAST */

static void cap_stream_started_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR) && IS_ENABLED(CONFIG_BT_BAP_UNICAST)) {
		bt_cap_initiator_started(cap_stream);
	}

	if (ops != NULL && ops->started != NULL) {
		ops->started(bap_stream);
	}
}

static void cap_stream_stopped_cb(struct bt_bap_stream *bap_stream, uint8_t reason)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	LOG_DBG("%p", cap_stream);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(bap_stream, reason);
	}
}

#if defined(CONFIG_BT_AUDIO_RX)
static void cap_stream_recv_cb(struct bt_bap_stream *bap_stream,
			       const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(bap_stream, info, buf);
	}
}
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
static void cap_stream_sent_cb(struct bt_bap_stream *bap_stream)
{
	struct bt_cap_stream *cap_stream = CONTAINER_OF(bap_stream,
							struct bt_cap_stream,
							bap_stream);
	struct bt_bap_stream_ops *ops = cap_stream->ops;

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(bap_stream);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static struct bt_bap_stream_ops bap_stream_ops = {
#if defined(CONFIG_BT_BAP_UNICAST)
	.configured = cap_stream_configured_cb,
	.qos_set = cap_stream_qos_set_cb,
	.enabled = cap_stream_enabled_cb,
	.metadata_updated = cap_stream_metadata_updated_cb,
	.disabled = cap_stream_disabled_cb,
	.released = cap_stream_released_cb,
#endif /* CONFIG_BT_BAP_UNICAST */
	.started = cap_stream_started_cb,
	.stopped = cap_stream_stopped_cb,
#if defined(CONFIG_BT_AUDIO_RX)
	.recv = cap_stream_recv_cb,
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	.sent = cap_stream_sent_cb,
#endif /* CONFIG_BT_AUDIO_TX */
};

void bt_cap_stream_ops_register_bap(struct bt_cap_stream *cap_stream)
{
	bt_bap_stream_cb_register(&cap_stream->bap_stream, &bap_stream_ops);
}

void bt_cap_stream_ops_register(struct bt_cap_stream *stream,
				struct bt_bap_stream_ops *ops)
{
	stream->ops = ops;

	/* CAP basically just forwards the BAP callbacks after doing what it (CAP) needs to do,
	 * so we can just always register the BAP callbacks here
	 *
	 * It is, however, only the CAP Initiator Unicast that depend on the callbacks being set in
	 * order to work, so for the CAP Initiator Unicast we need an additional register to ensure
	 * correctness.
	 */

	bt_cap_stream_ops_register_bap(stream);
}

#if defined(CONFIG_BT_AUDIO_TX)
int bt_cap_stream_send(struct bt_cap_stream *stream, struct net_buf *buf, uint16_t seq_num,
		       uint32_t ts)
{
	CHECKIF(stream == NULL) {
		LOG_DBG("stream is NULL");

		return -EINVAL;
	}

	return bt_bap_stream_send(&stream->bap_stream, buf, seq_num, ts);
}

int bt_cap_stream_get_tx_sync(struct bt_cap_stream *stream, struct bt_iso_tx_info *info)
{
	CHECKIF(stream == NULL) {
		LOG_DBG("stream is NULL");

		return -EINVAL;
	}

	return bt_bap_stream_get_tx_sync(&stream->bap_stream, info);
}
#endif /* CONFIG_BT_AUDIO_TX */
