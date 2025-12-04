/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest_assert.h>
#include <sys/errno.h>

#include "audio/bap_endpoint.h"
#include "audio/bap_iso.h"

static struct bt_bap_unicast_client_cb *unicast_client_cb;
static struct bt_bap_unicast_group bap_unicast_group;

bool bt_bap_unicast_client_has_ep(const struct bt_bap_ep *ep)
{
	return true;
}

int bt_bap_unicast_client_config(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_cfg *codec_cfg)
{
	if (stream == NULL || stream->ep == NULL || codec_cfg == NULL) {
		return -EINVAL;
	}

	switch (stream->ep->state) {
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

	stream->ep->state = BT_BAP_EP_STATE_CODEC_CONFIGURED;

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
			switch (stream->ep->state) {
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

			stream->ep->state = BT_BAP_EP_STATE_QOS_CONFIGURED;

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

	switch (stream->ep->state) {
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->enable != NULL) {
		unicast_client_cb->enable(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					  BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->state = BT_BAP_EP_STATE_ENABLING;

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

	switch (stream->ep->state) {
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
	struct bt_bap_ep *ep;

	if (stream == NULL) {
		return -EINVAL;
	}

	ep = stream->ep;

	switch (ep->state) {
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
	case BT_BAP_EP_STATE_ENABLING:
		break;
	default:
		return -EINVAL;
	}

	ep->iso->chan.state = BT_ISO_STATE_CONNECTED;
	if (stream->ops != NULL && stream->ops->connected != NULL) {
		stream->ops->connected(stream);
	}

	if (ep->dir == BT_AUDIO_DIR_SINK) {
		/* Mocking that the unicast server automatically starts the stream */
		ep->state = BT_BAP_EP_STATE_STREAMING;

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

	switch (stream->ep->state) {
	case BT_BAP_EP_STATE_ENABLING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->start != NULL) {
		unicast_client_cb->start(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					 BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->state = BT_BAP_EP_STATE_STREAMING;

	if (stream->ops != NULL && stream->ops->started != NULL) {
		stream->ops->started(stream);
	}

	return 0;
}

int bt_bap_unicast_client_disable(struct bt_bap_stream *stream)
{
	if (stream == NULL || stream->ep == NULL) {
		return -EINVAL;
	}

	printk("%s %p %d\n", __func__, stream, stream->ep->dir);

	switch (stream->ep->state) {
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
		stream->ep->state = BT_BAP_EP_STATE_QOS_CONFIGURED;

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
		stream->ep->state = BT_BAP_EP_STATE_DISABLING;

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

	switch (stream->ep->state) {
	case BT_BAP_EP_STATE_DISABLING:
		break;
	default:
		return -EINVAL;
	}

	if (unicast_client_cb != NULL && unicast_client_cb->stop != NULL) {
		unicast_client_cb->stop(stream, BT_BAP_ASCS_RSP_CODE_SUCCESS,
					BT_BAP_ASCS_REASON_NONE);
	}

	stream->ep->state = BT_BAP_EP_STATE_QOS_CONFIGURED;

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

			pair_stream->ep->state = BT_BAP_EP_STATE_QOS_CONFIGURED;

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

	switch (stream->ep->state) {
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

	stream->ep->state = BT_BAP_EP_STATE_IDLE;
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

struct bt_bap_iso *bt_bap_unicast_client_new_audio_iso(void)
{
	static struct bt_iso_chan_ops unicast_client_iso_ops;
	struct bt_bap_iso *bap_iso;

	bap_iso = bt_bap_iso_new();
	if (bap_iso == NULL) {
		return NULL;
	}

	bt_bap_iso_init(bap_iso, &unicast_client_iso_ops);

	return bap_iso;
}

static int unicast_group_add_iso(struct bt_bap_unicast_group *group, struct bt_bap_iso *iso)
{
	struct bt_iso_chan **chan_slot = NULL;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(iso != NULL);

	/* Append iso channel to the group->cis array */
	for (size_t i = 0U; i < ARRAY_SIZE(group->cis); i++) {
		/* Return if already there */
		if (group->cis[i] == &iso->chan) {
			return 0;
		}

		if (chan_slot == NULL && group->cis[i] == NULL) {
			chan_slot = &group->cis[i];
		}
	}

	if (chan_slot == NULL) {
		return -ENOMEM;
	}

	*chan_slot = &iso->chan;

	return 0;
}

static void unicast_client_qos_cfg_to_iso_qos(struct bt_bap_iso *iso,
					      const struct bt_bap_qos_cfg *qos,
					      enum bt_audio_dir dir)
{
	struct bt_iso_chan_io_qos *io_qos;
	struct bt_iso_chan_io_qos *other_io_qos;

	if (dir == BT_AUDIO_DIR_SINK) {
		/* If the endpoint is a sink, then we need to
		 * configure our TX parameters
		 */
		io_qos = iso->chan.qos->tx;
		if (bt_bap_iso_get_ep(true, iso, BT_AUDIO_DIR_SOURCE) == NULL) {
			other_io_qos = iso->chan.qos->rx;
		} else {
			other_io_qos = NULL;
		}
	} else {
		/* If the endpoint is a source, then we need to
		 * configure our RX parameters
		 */
		io_qos = iso->chan.qos->rx;
		if (bt_bap_iso_get_ep(true, iso, BT_AUDIO_DIR_SINK) == NULL) {
			other_io_qos = iso->chan.qos->tx;
		} else {
			other_io_qos = NULL;
		}
	}

	bt_bap_qos_cfg_to_iso_qos(io_qos, qos);
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	iso->chan.qos->num_subevents = qos->num_subevents;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	if (other_io_qos != NULL) {
		/* If the opposing ASE of the CIS is not yet configured, we
		 * still need to set the PHY value when creating the CIG.
		 */
		other_io_qos->phy = io_qos->phy;
	}
}

static void unicast_group_set_iso_stream_param(struct bt_bap_unicast_group *group,
					       struct bt_bap_iso *iso, struct bt_bap_qos_cfg *qos,
					       enum bt_audio_dir dir)
{
	/* Store the stream Codec QoS in the bap_iso */
	unicast_client_qos_cfg_to_iso_qos(iso, qos, dir);

	/* Store the group Codec QoS in the group - This assumes thats the parameters have been
	 * verified first
	 */
	group->cig_param.framing = qos->framing;
	if (dir == BT_AUDIO_DIR_SOURCE) {
		group->cig_param.p_to_c_interval = qos->interval;
		group->cig_param.p_to_c_latency = qos->latency;
	} else {
		group->cig_param.c_to_p_interval = qos->interval;
		group->cig_param.c_to_p_latency = qos->latency;
	}
}

static void unicast_group_add_stream(struct bt_bap_unicast_group *group,
				     struct bt_bap_unicast_group_stream_param *param,
				     struct bt_bap_iso *iso, enum bt_audio_dir dir)
{
	struct bt_bap_stream *stream = param->stream;
	struct bt_bap_qos_cfg *qos = param->qos;

	__ASSERT_NO_MSG(stream->ep == NULL || (stream->ep != NULL && stream->ep->iso == NULL));

	stream->qos = qos;
	stream->group = group;

	/* iso initialized already */
	bt_bap_iso_bind_stream(iso, stream, dir);
	if (stream->ep != NULL) {
		bt_bap_iso_bind_ep(iso, stream->ep);
	}

	unicast_group_set_iso_stream_param(group, iso, qos, dir);

	sys_slist_append(&group->streams, &stream->_node);
}

static int unicast_group_add_stream_pair(struct bt_bap_unicast_group *group,
					 struct bt_bap_unicast_group_stream_pair_param *param)
{
	struct bt_bap_iso *iso;
	int err;

	__ASSERT_NO_MSG(group != NULL);
	__ASSERT_NO_MSG(param != NULL);
	__ASSERT_NO_MSG(param->rx_param != NULL || param->tx_param != NULL);

	iso = bt_bap_unicast_client_new_audio_iso();
	if (iso == NULL) {
		return -ENOMEM;
	}

	err = unicast_group_add_iso(group, iso);
	if (err < 0) {
		bt_bap_iso_unref(iso);
		return err;
	}

	if (param->rx_param != NULL) {
		unicast_group_add_stream(group, param->rx_param, iso, BT_AUDIO_DIR_SOURCE);
	}

	if (param->tx_param != NULL) {
		unicast_group_add_stream(group, param->tx_param, iso, BT_AUDIO_DIR_SINK);
	}

	bt_bap_iso_unref(iso);

	return 0;
}

int bt_bap_unicast_group_create(struct bt_bap_unicast_group_param *param,
				struct bt_bap_unicast_group **unicast_group)
{
	if (bap_unicast_group.allocated) {
		return -ENOMEM;
	}

	bap_unicast_group.allocated = true;
	*unicast_group = &bap_unicast_group;

	sys_slist_init(&bap_unicast_group.streams);
	for (size_t i = 0U; i < param->params_count; i++) {
		struct bt_bap_unicast_group_stream_pair_param *stream_param;
		int err;

		stream_param = &param->params[i];

		err = unicast_group_add_stream_pair(*unicast_group, stream_param);
		__ASSERT(err == 0, "%d", err);
	}

	return 0;
}

int bt_bap_unicast_group_add_streams(struct bt_bap_unicast_group *unicast_group,
				     struct bt_bap_unicast_group_stream_pair_param params[],
				     size_t num_param)
{
	if (unicast_group == NULL || params == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < num_param; i++) {
		if (params[i].rx_param != NULL) {
			sys_slist_append(&unicast_group->streams,
					 &params[i].rx_param->stream->_node);
		}

		if (params[i].tx_param != NULL) {
			sys_slist_append(&unicast_group->streams,
					 &params[i].tx_param->stream->_node);
		}
	}

	return 0;
}

int bt_bap_unicast_group_reconfig(struct bt_bap_unicast_group *unicast_group,
				  const struct bt_bap_unicast_group_param *param)
{
	if (unicast_group == NULL || param == NULL) {
		return -EINVAL;
	}

	return 0;
}

static void unicast_group_free(struct bt_bap_unicast_group *group)
{
	struct bt_bap_stream *stream, *next;

	__ASSERT_NO_MSG(group != NULL);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&group->streams, stream, next, _node) {
		struct bt_bap_iso *bap_iso = CONTAINER_OF(stream->iso, struct bt_bap_iso, chan);
		struct bt_bap_ep *ep = stream->ep;

		stream->group = NULL;
		if (bap_iso != NULL) {
			if (bap_iso->rx.stream == stream) {
				bt_bap_iso_unbind_stream(stream, BT_AUDIO_DIR_SOURCE);
			} else if (bap_iso->tx.stream == stream) {
				bt_bap_iso_unbind_stream(stream, BT_AUDIO_DIR_SINK);
			} else {
				__ASSERT_PRINT("stream %p has invalid bap_iso %p", stream, bap_iso);
			}
		}

		if (ep != NULL && ep->iso != NULL) {
			bt_bap_iso_unbind_ep(ep->iso, ep);
		}

		sys_slist_remove(&group->streams, NULL, &stream->_node);
	}

	group->allocated = false;
}

int bt_bap_unicast_group_delete(struct bt_bap_unicast_group *unicast_group)
{
	if (unicast_group == NULL) {
		return -EINVAL;
	}

	unicast_group_free(unicast_group);

	return 0;
}

int bt_bap_unicast_group_foreach_stream(struct bt_bap_unicast_group *unicast_group,
					bt_bap_unicast_group_foreach_stream_func_t func,
					void *user_data)
{
	struct bt_bap_stream *stream, *next;

	if (unicast_group == NULL) {
		return -EINVAL;
	}

	if (func == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&unicast_group->streams, stream, next, _node) {
		const bool stop = func(stream, user_data);

		if (stop) {
			return -ECANCELED;
		}
	}

	return 0;
}
