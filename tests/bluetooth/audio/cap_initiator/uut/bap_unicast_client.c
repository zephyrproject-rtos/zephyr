/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/slist.h>
#include <zephyr/ztest_assert.h>
#include <sys/errno.h>

#include "bap_endpoint.h"
#include "bap_iso.h"

static struct bt_bap_unicast_client_cb *unicast_client_cb;

bool bt_bap_ep_is_unicast_client(const struct bt_bap_ep *ep)
{
	return false;
}

int bt_bap_unicast_client_config(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_cfg *codec_cfg)
{
	if (stream == NULL || stream->ep == NULL || codec_cfg == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_IDLE:
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->config != NULL) {
		unicast_client_cb->config(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					  BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->status.state = BT_BAP_EP_STATE_CODEC_CONFIGURED;

	if (stream->ops != NULL && stream->ops->configured != NULL) {
		const struct bt_bap_qos_cfg_pref pref = {0};

		stream->ops->configured(stream, &pref);
	}

	return 0;
}

int bt_bap_unicast_client_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group)
{
	struct bt_bap_stream *stream;

	if (conn == NULL || group == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->conn == conn) {
			switch (stream->ep->status.state) {
			case BT_BAP_EP_STATE_CODEC_CONFIGURED:
			case BT_BAP_EP_STATE_QOS_CONFIGURED:
				break;
			default:
				return -EINVAL;
			}
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&group->streams, stream, _node) {
		if (stream->conn == conn) {
			if (unicast_client_cb != NULL && unicast_client_cb->qos != NULL) {
				unicast_client_cb->qos(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
						       BT_BAP_ASCS_REASON_NONE);
			}

			stream->ep->status.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

			if (stream->ops != NULL && stream->ops->qos_set != NULL) {
				stream->ops->qos_set(stream);
			}
		}
	}

	return 0;
}

int bt_bap_unicast_client_enable(struct bt_bap_stream *stream, const uint8_t meta[],
				 size_t meta_len)
{
	if (stream == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->enable != NULL) {
		unicast_client_cb->enable(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					  BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->status.state = BT_BAP_EP_STATE_ENABLING;

	if (stream->ops != NULL && stream->ops->enabled != NULL) {
		stream->ops->enabled(stream);
	}

	return 0;
}

int bt_bap_unicast_client_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len)
{
	if (stream == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_ENABLING:
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->metadata != NULL) {
		unicast_client_cb->metadata(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					    BT_BAP_ASCS_REASON_NONE);
	}

	if (stream->ops != NULL && stream->ops->metadata_updated != NULL) {
		stream->ops->metadata_updated(stream);
	}

	return 0;
}

int bt_bap_unicast_client_connect(struct bt_bap_stream *stream)
{
	if (stream == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
	case BT_BAP_EP_STATE_ENABLING:
		break;
	default:
		return -EINVAL;
	}

	if (stream->ops != NULL && stream->ops->connected != NULL) {
		stream->ops->connected(stream);
	}

	if (stream->ep != NULL && stream->ep->dir == BT_AUDIO_DIR_SINK) {
		/* Mocking that the unicast server automatically starts the stream */
		stream->ep->status.state = BT_BAP_EP_STATE_STREAMING;

		if (stream->ops != NULL && stream->ops->started != NULL) {
			stream->ops->started(stream);
		}
	}

	return 0;
}

int bt_bap_unicast_client_start(struct bt_bap_stream *stream)
{
	/* As per the ASCS spec, only source streams can be started by the client */
	if (stream == NULL || stream->ep == NULL || stream->ep->dir == BT_AUDIO_DIR_SINK) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_ENABLING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->start != NULL) {
		unicast_client_cb->start(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					 BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->status.state = BT_BAP_EP_STATE_STREAMING;

	if (stream->ops != NULL && stream->ops->started != NULL) {
		stream->ops->started(stream);
	}

	return 0;
}

int bt_bap_unicast_client_disable(struct bt_bap_stream *stream)
{
	printk("%s %p %d\n", __func__, stream, stream->ep->dir);

	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_ENABLING:
	case BT_BAP_EP_STATE_STREAMING:
		break;
	default:
		return -EINVAL;
	}

	/* Even though the ASCS spec does not have the disabling state for sink ASEs, the unicast
	 * client implementation fakes the behavior of it and always calls the disabled callback
	 * when leaving the streaming state in a non-release manner
	 */

	if (unicast_client_cb != NULL && unicast_client_cb->disable != NULL) {
		unicast_client_cb->disable(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					   BT_BAP_ASCS_REASON_NONE);
	}

	/* Disabled sink ASEs go directly to the QoS configured state */
	if (stream->ep->dir == BT_AUDIO_DIR_SINK) {
		stream->ep->status.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

		if (stream->ops != NULL && stream->ops->disabled != NULL) {
			stream->ops->disabled(stream);
		}

		if (stream->ops != NULL && stream->ops->stopped != NULL) {
			stream->ops->stopped(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
		}

		if (stream->ops != NULL && stream->ops->qos_set != NULL) {
			stream->ops->qos_set(stream);
		}
	} else if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
		stream->ep->status.state = BT_BAP_EP_STATE_DISABLING;

		if (stream->ops != NULL && stream->ops->disabled != NULL) {
			stream->ops->disabled(stream);
		}
	}

	return 0;
}

int bt_bap_unicast_client_stop(struct bt_bap_stream *stream)
{
	printk("%s %p\n", __func__, stream);

	/* As per the ASCS spec, only source streams can be stopped by the client */
	if (stream == NULL || stream->ep == NULL || stream->ep->dir == BT_AUDIO_DIR_SINK) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->stop != NULL) {
		unicast_client_cb->stop(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->status.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

	if (stream->ops != NULL && stream->ops->stopped != NULL) {
		stream->ops->stopped(stream, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	}

	if (stream->ops != NULL && stream->ops->qos_set != NULL) {
		stream->ops->qos_set(stream);
	}

	/* If the stream can be disconnected, BAP will disconnect the stream once it reaches the
	 * QoS Configured state. We simulator that behavior here, and if the stream is disconnected,
	 * then the Unicast Server will set any paired stream to the QoS Configured state
	 * autonomously as well.
	 */
	if (bt_bap_stream_can_disconnect(stream)) {
		struct bt_bap_ep *pair_ep = bt_bap_iso_get_paired_ep(stream->ep);

		if (pair_ep != NULL && pair_ep->stream != NULL) {
			struct bt_bap_stream *pair_stream = pair_ep->stream;

			pair_stream->ep->status.state = BT_BAP_EP_STATE_QOS_CONFIGURED;

			if (pair_stream->ops != NULL && pair_stream->ops->stopped != NULL) {
				pair_stream->ops->stopped(pair_stream,
							  BT_HCI_ERR_LOCALHOST_TERM_CONN);
			}

			if (pair_stream->ops != NULL && pair_stream->ops->qos_set != NULL) {
				pair_stream->ops->qos_set(pair_stream);
			}
		}
	}

	return 0;
}

int bt_bap_unicast_client_release(struct bt_bap_stream *stream)
{
	printk("%s %p\n", __func__, stream);

	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->status.state) {
	case BT_BAP_EP_STATE_CODEC_CONFIGURED:
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
	case BT_BAP_EP_STATE_ENABLING:
	case BT_BAP_EP_STATE_STREAMING:
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->release != NULL) {
		unicast_client_cb->release(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					   BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->status.state = BT_BAP_EP_STATE_IDLE;
	bt_bap_stream_reset(stream);

	if (stream->ops != NULL && stream->ops->released != NULL) {
		stream->ops->released(stream);
	}

	return 0;
}

int bt_bap_unicast_client_register_cb(struct bt_bap_unicast_client_cb *cb)
{
	unicast_client_cb = cb;

	return 0;
}
