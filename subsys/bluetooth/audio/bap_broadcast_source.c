/*  Bluetooth Audio Broadcast Source */

/*
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
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bap_broadcast_source, CONFIG_BT_BAP_BROADCAST_SOURCE_LOG_LEVEL);

#include "bap_iso.h"
#include "bap_endpoint.h"

struct bt_bap_broadcast_subgroup {
	/* The streams used to create the broadcast source */
	sys_slist_t streams;

	/* The codec of the subgroup */
	struct bt_audio_codec_cfg *codec_cfg;

	/* List node */
	sys_snode_t _node;
};

static struct bt_bap_ep broadcast_source_eps[CONFIG_BT_BAP_BROADCAST_SRC_COUNT]
					    [BROADCAST_STREAM_CNT];
static struct bt_bap_broadcast_subgroup
	broadcast_source_subgroups[CONFIG_BT_BAP_BROADCAST_SRC_COUNT]
				  [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
static struct bt_bap_broadcast_source broadcast_sources[CONFIG_BT_BAP_BROADCAST_SRC_COUNT];

/**
 * 2 octets UUID
 * 3 octets presentation delay
 * 1 octet number of subgroups
 *
 * Each subgroup then has
 * 1 octet of number of BIS
 * 5 octets of Codec_ID
 * 1 octet codec specific configuration len
 * 0-n octets of codec specific configuration
 * 1 octet metadata len
 * 0-n octets of metadata
 *
 * For each BIS in the subgroup there is
 * 1 octet for the BIS index
 * 1 octet codec specific configuration len
 * 0-n octets of codec specific configuration
 *
 * For a minimal BASE with 1 subgroup and 1 BIS without and other data the
 * total comes to 16
 */
#define MINIMUM_BASE_SIZE 16

static void broadcast_source_set_ep_state(struct bt_bap_ep *ep, uint8_t state)
{
	uint8_t old_state;

	old_state = ep->status.state;

	LOG_DBG("ep %p id 0x%02x %s -> %s", ep, ep->status.id, bt_bap_ep_state_str(old_state),
		bt_bap_ep_state_str(state));

	switch (old_state) {
	case BT_BAP_EP_STATE_IDLE:
		if (state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
			LOG_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_BAP_EP_STATE_QOS_CONFIGURED:
		if (state != BT_BAP_EP_STATE_IDLE && state != BT_BAP_EP_STATE_ENABLING) {
			LOG_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_BAP_EP_STATE_ENABLING:
		if (state != BT_BAP_EP_STATE_STREAMING) {
			LOG_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_BAP_EP_STATE_STREAMING:
		if (state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
			LOG_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	default:
		LOG_ERR("Invalid broadcast sync endpoint state: %s",
			bt_bap_ep_state_str(old_state));
		return;
	}

	ep->status.state = state;
}

static void broadcast_source_set_state(struct bt_bap_broadcast_source *source, uint8_t state)
{
	struct bt_bap_broadcast_subgroup *subgroup;

	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		struct bt_bap_stream *stream;

		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			broadcast_source_set_ep_state(stream->ep, state);
		}
	}
}

static void broadcast_source_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->tx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_BAP_DEBUG_STREAM_DATA)) {
		LOG_DBG("stream %p ep %p", stream, stream->ep);
	}

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(stream);
	}
}

static void broadcast_source_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->tx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p", stream, ep);

	ops = stream->ops;
	if (ops != NULL && ops->connected != NULL) {
		ops->connected(stream);
	}

	broadcast_source_set_ep_state(ep, BT_BAP_EP_STATE_STREAMING);

	if (ops != NULL && ops->started != NULL) {
		ops->started(stream);
	} else {
		LOG_WRN("No callback for started set");
	}
}

static void broadcast_source_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->tx.ep;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p reason 0x%02x", stream, stream->ep, reason);

	ops = stream->ops;
	if (ops != NULL && ops->disconnected != NULL) {
		ops->disconnected(stream, reason);
	}

	broadcast_source_set_ep_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream, reason);
	} else {
		LOG_WRN("No callback for stopped set");
	}
}

static struct bt_iso_chan_ops broadcast_source_iso_ops = {
	.sent		= broadcast_source_iso_sent,
	.connected	= broadcast_source_iso_connected,
	.disconnected	= broadcast_source_iso_disconnected,
};

bool bt_bap_ep_is_broadcast_src(const struct bt_bap_ep *ep)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_source_eps); i++) {
		if (PART_OF_ARRAY(broadcast_source_eps[i], ep)) {
			return true;
		}
	}

	return false;
}

static void broadcast_source_ep_init(struct bt_bap_ep *ep)
{
	LOG_DBG("ep %p", ep);

	(void)memset(ep, 0, sizeof(*ep));
	ep->dir = BT_AUDIO_DIR_SOURCE;
	ep->iso = NULL;
}

static struct bt_bap_ep *broadcast_source_new_ep(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_source_eps[index]); i++) {
		struct bt_bap_ep *ep = &broadcast_source_eps[index][i];

		/* If ep->stream is NULL the endpoint is unallocated */
		if (ep->stream == NULL) {
			broadcast_source_ep_init(ep);
			return ep;
		}
	}

	return NULL;
}

static struct bt_bap_broadcast_subgroup *broadcast_source_new_subgroup(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_source_subgroups[index]); i++) {
		struct bt_bap_broadcast_subgroup *subgroup = &broadcast_source_subgroups[index][i];

		if (sys_slist_is_empty(&subgroup->streams)) {
			return subgroup;
		}
	}

	return NULL;
}

static int broadcast_source_setup_stream(uint8_t index, struct bt_bap_stream *stream,
					 struct bt_audio_codec_cfg *codec_cfg,
					 struct bt_audio_codec_qos *qos,
					 struct bt_bap_broadcast_source *source)
{
	struct bt_bap_iso *iso;
	struct bt_bap_ep *ep;

	ep = broadcast_source_new_ep(index);
	if (ep == NULL) {
		LOG_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	iso = bt_bap_iso_new();
	if (iso == NULL) {
		LOG_DBG("Could not allocate iso");
		return -ENOMEM;
	}

	bt_bap_iso_init(iso, &broadcast_source_iso_ops);
	bt_bap_iso_bind_ep(iso, ep);

	bt_audio_codec_qos_to_iso_qos(iso->chan.qos->tx, qos);
	bt_audio_codec_cfg_to_iso_path(iso->chan.qos->tx->path, codec_cfg);
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	iso->chan.qos->num_subevents = qos->num_subevents;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	bt_bap_iso_unref(iso);

	bt_bap_stream_attach(NULL, stream, ep, codec_cfg);
	stream->qos = qos;
	ep->broadcast_source = source;

	return 0;
}

static bool encode_base_subgroup(struct bt_bap_broadcast_subgroup *subgroup,
				 struct bt_audio_broadcast_stream_data *stream_data,
				 uint8_t *streams_encoded, struct net_buf_simple *buf)
{
	struct bt_bap_stream *stream;
	const struct bt_audio_codec_cfg *codec_cfg;
	uint8_t stream_count;
	uint8_t len;

	stream_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
		stream_count++;
	}

	codec_cfg = subgroup->codec_cfg;

	net_buf_simple_add_u8(buf, stream_count);
	net_buf_simple_add_u8(buf, codec_cfg->id);
	net_buf_simple_add_le16(buf, codec_cfg->cid);
	net_buf_simple_add_le16(buf, codec_cfg->vid);

	net_buf_simple_add_u8(buf, codec_cfg->data_len);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	if ((buf->size - buf->len) < codec_cfg->data_len) {
		LOG_DBG("No room for config data: %zu", codec_cfg->data_len);

		return false;
	}
	net_buf_simple_add_mem(buf, codec_cfg->data, codec_cfg->data_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

	if ((buf->size - buf->len) < sizeof(len)) {
		LOG_DBG("No room for metadata length");

		return false;
	}

	net_buf_simple_add_u8(buf, codec_cfg->meta_len);

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	if ((buf->size - buf->len) < codec_cfg->meta_len) {
		LOG_DBG("No room for metadata data: %zu", codec_cfg->meta_len);

		return false;
	}

	net_buf_simple_add_mem(buf, codec_cfg->meta, codec_cfg->meta_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */

	/* Create BIS index bitfield */
	for (uint8_t i = 0U; i < stream_count; i++) {
		/* Set the bis_index to *streams_encoded plus 1 as the indexes start from 1 */
		const uint8_t bis_index = *streams_encoded + 1;

		if ((buf->size - buf->len) < (sizeof(bis_index) + sizeof(uint8_t))) {
			LOG_DBG("No room for BIS[%d] index", i);

			return false;
		}

		net_buf_simple_add_u8(buf, bis_index);

		if ((buf->size - buf->len) < sizeof(len)) {
			LOG_DBG("No room for bis codec config length");

			return false;
		}

		net_buf_simple_add_u8(buf, stream_data[i].data_len);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
		if ((buf->size - buf->len) < stream_data[i].data_len) {
			LOG_DBG("No room for BIS[%u] data: %zu", i, stream_data[i].data_len);

			return false;
		}

		net_buf_simple_add_mem(buf, stream_data[i].data, stream_data[i].data_len);
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */

		(*streams_encoded)++;
	}

	return true;
}

static bool encode_base(struct bt_bap_broadcast_source *source, struct net_buf_simple *buf)
{
	struct bt_bap_broadcast_subgroup *subgroup;
	uint8_t streams_encoded;
	uint8_t subgroup_count;

	/* 13 is the size of the fixed size values following this check */
	if ((buf->size - buf->len) < MINIMUM_BASE_SIZE) {
		return false;
	}

	subgroup_count = 0U;
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		subgroup_count++;
	}

	net_buf_simple_add_le16(buf, BT_UUID_BASIC_AUDIO_VAL);

	net_buf_simple_add_le24(buf, source->qos->pd);
	net_buf_simple_add_u8(buf, subgroup_count);

	/* Since the `stream_data` is only stored in the broadcast source,
	 * we need to provide that information when encoding each subgroup
	 */
	streams_encoded = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		if (!encode_base_subgroup(subgroup,
					  &source->stream_data[streams_encoded],
					  &streams_encoded, buf)) {
			return false;
		}
	}

	return true;
}

static int generate_broadcast_id(struct bt_bap_broadcast_source *source)
{
	bool unique;

	do {
		int err;

		err = bt_rand(&source->broadcast_id,
			      BT_AUDIO_BROADCAST_ID_SIZE);
		if (err) {
			return err;
		}

		/* Ensure uniqueness */
		unique = true;
		for (int i = 0; i < ARRAY_SIZE(broadcast_sources); i++) {
			if (&broadcast_sources[i] == source) {
				continue;
			}

			if (broadcast_sources[i].broadcast_id == source->broadcast_id) {
				unique = false;
				break;
			}
		}
	} while (!unique);

	return 0;
}

static void broadcast_source_cleanup(struct bt_bap_broadcast_source *source)
{
	struct bt_bap_broadcast_subgroup *subgroup, *next_subgroup;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&source->subgroups, subgroup,
					  next_subgroup, _node) {
		struct bt_bap_stream *stream, *next_stream;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&subgroup->streams, stream,
						  next_stream, _node) {
			bt_bap_iso_unbind_ep(stream->ep->iso, stream->ep);
			stream->ep->stream = NULL;
			stream->ep = NULL;
			stream->codec_cfg = NULL;
			stream->qos = NULL;
			stream->group = NULL;

			sys_slist_remove(&subgroup->streams, NULL,
					 &stream->_node);
		}
		sys_slist_remove(&source->subgroups, NULL, &subgroup->_node);
	}

	(void)memset(source, 0, sizeof(*source));
}

static bool valid_broadcast_source_param(const struct bt_bap_broadcast_source_param *param,
					 const struct bt_bap_broadcast_source *source)
{
	const struct bt_audio_codec_qos *qos;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(!IN_RANGE(param->params_count, 1U, CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT)) {
		LOG_DBG("param->params_count %zu is invalid", param->params_count);
		return false;
	}

	CHECKIF(param->packing != BT_ISO_PACKING_SEQUENTIAL &&
		param->packing != BT_ISO_PACKING_INTERLEAVED) {
		LOG_DBG("param->packing %u is invalid", param->packing);
		return false;
	}

	qos = param->qos;
	CHECKIF(qos == NULL) {
		LOG_DBG("param->qos is NULL");
		return false;
	}

	CHECKIF(bt_audio_verify_qos(qos) != BT_BAP_ASCS_REASON_NONE) {
		LOG_DBG("param->qos is invalid");
		return false;
	}

	CHECKIF(param->qos->rtn > BT_ISO_BROADCAST_RTN_MAX) {
		LOG_DBG("param->qos->rtn %u invalid", param->qos->rtn);
		return false;
	}

	CHECKIF(param->params == NULL) {
		LOG_DBG("param->params is NULL");
		return false;
	}

	CHECKIF(param->params_count == 0) {
		LOG_DBG("param->params_count is 0");
		return false;
	}

	for (size_t i = 0U; i < param->params_count; i++) {
		const struct bt_bap_broadcast_source_subgroup_param *subgroup_param;

		subgroup_param = &param->params[i];

		CHECKIF(subgroup_param->params == NULL) {
			LOG_DBG("subgroup_params[%zu].params is NULL", i);
			return false;
		}

		CHECKIF(!IN_RANGE(subgroup_param->params_count, 1U,
				  CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)) {
			LOG_DBG("subgroup_params[%zu].count (%zu) is invalid", i,
				subgroup_param->params_count);
			return false;
		}

		CHECKIF(!bt_audio_valid_codec_cfg(subgroup_param->codec_cfg)) {
			LOG_DBG("subgroup_params[%zu].codec_cfg  is invalid", i);
			return false;
		}

		for (size_t j = 0U; j < subgroup_param->params_count; j++) {
			const struct bt_bap_broadcast_source_stream_param *stream_param;

			stream_param = &subgroup_param->params[j];

			CHECKIF(stream_param->stream == NULL) {
				LOG_DBG("subgroup_params[%zu].stream_params[%zu]->stream is NULL",
					i, j);
				return false;
			}

			CHECKIF(stream_param->stream->group != NULL &&
				stream_param->stream->group != source) {
				LOG_DBG("subgroup_params[%zu].stream_params[%zu]->stream is "
					"already part of group %p",
					i, j, stream_param->stream->group);
				return false;
			}

			CHECKIF(stream_param->data == NULL && stream_param->data_len != 0) {
				LOG_DBG("subgroup_params[%zu].stream_params[%zu]->data is "
					"NULL with len %zu",
					i, j, stream_param->data_len);
				return false;
			}

#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
			CHECKIF(stream_param->data_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE) {
				LOG_DBG("subgroup_params[%zu].stream_params[%zu]->data_len too "
					"large: %zu > %d",
					i, j, stream_param->data_len,
					CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE);
				return false;
			}
		}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
	}

	return true;
}

static enum bt_bap_ep_state broadcast_source_get_state(struct bt_bap_broadcast_source *source)
{
	struct bt_bap_broadcast_subgroup *subgroup;
	struct bt_bap_stream *stream;
	sys_snode_t *head_node;

	if (source == NULL) {
		LOG_DBG("source is NULL");
		return BT_BAP_EP_STATE_IDLE;
	}

	if (sys_slist_is_empty(&source->subgroups)) {
		LOG_DBG("Source does not have any streams");
		return BT_BAP_EP_STATE_IDLE;
	}

	/* Get the first stream */
	head_node = sys_slist_peek_head(&source->subgroups);
	subgroup = CONTAINER_OF(head_node, struct bt_bap_broadcast_subgroup, _node);

	head_node = sys_slist_peek_head(&subgroup->streams);
	stream = CONTAINER_OF(head_node, struct bt_bap_stream, _node);

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (stream->ep == NULL) {
		LOG_DBG("stream->ep is NULL");
		return BT_BAP_EP_STATE_IDLE;
	}

	return stream->ep->status.state;
}

int bt_bap_broadcast_source_create(struct bt_bap_broadcast_source_param *param,
				   struct bt_bap_broadcast_source **out_source)
{
	struct bt_bap_broadcast_source *source;
	struct bt_audio_codec_qos *qos;
	size_t stream_count;
	uint8_t index;
	int err;

	CHECKIF(out_source == NULL) {
		LOG_DBG("out_source is NULL");
		return -EINVAL;
	}

	/* Set out_source to NULL until the source has actually been created */
	*out_source = NULL;

	if (!valid_broadcast_source_param(param, NULL)) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	source = NULL;
	for (index = 0; index < ARRAY_SIZE(broadcast_sources); index++) {
		if (sys_slist_is_empty(&broadcast_sources[index].subgroups)) { /* Find free entry */
			source = &broadcast_sources[index];
			break;
		}
	}

	if (source == NULL) {
		LOG_DBG("Could not allocate any more broadcast sources");
		return -ENOMEM;
	}

	stream_count = 0U;

	qos = param->qos;
	/* Go through all subgroups and streams and setup each setup with an
	 * endpoint
	 */
	for (size_t i = 0U; i < param->params_count; i++) {
		const struct bt_bap_broadcast_source_subgroup_param *subgroup_param;
		struct bt_bap_broadcast_subgroup *subgroup;

		subgroup_param = &param->params[i];

		subgroup = broadcast_source_new_subgroup(index);
		if (subgroup == NULL) {
			LOG_DBG("Could not allocate new broadcast subgroup");
			broadcast_source_cleanup(source);
			return -ENOMEM;
		}

		subgroup->codec_cfg = subgroup_param->codec_cfg;
		sys_slist_append(&source->subgroups, &subgroup->_node);

		/* Check that we are not above the maximum BIS count */
		if (subgroup_param->params_count + stream_count > BROADCAST_STREAM_CNT) {
			LOG_DBG("Cannot create broadcaster with %zu streams", stream_count);
			broadcast_source_cleanup(source);

			return -ENOMEM;
		}

		for (size_t j = 0U; j < subgroup_param->params_count; j++) {
			const struct bt_bap_broadcast_source_stream_param *stream_param;
			struct bt_bap_stream *stream;

			stream_param = &subgroup_param->params[j];
			stream = stream_param->stream;

			err = broadcast_source_setup_stream(index, stream,
							    subgroup_param->codec_cfg, qos, source);
			if (err != 0) {
				LOG_DBG("Failed to setup streams[%zu]: %d", i, err);
				broadcast_source_cleanup(source);
				return err;
			}

			/* Store the BIS specific codec configuration data in
			 * the broadcast source. It is stored in the broadcast
			 * source, instead of the stream object, as this is
			 * only relevant for the broadcast source, and not used
			 * for unicast or broadcast sink.
			 */
			(void)memcpy(source->stream_data[stream_count].data, stream_param->data,
				     stream_param->data_len * sizeof(*stream_param->data));
			source->stream_data[stream_count].data_len = stream_param->data_len;

			sys_slist_append(&subgroup->streams, &stream->_node);
			stream_count++;
		}
	}

	err = generate_broadcast_id(source);
	if (err != 0) {
		LOG_DBG("Could not generate broadcast id: %d", err);
		return err;
	}

	/* Finalize state changes and store information */
	broadcast_source_set_state(source, BT_BAP_EP_STATE_QOS_CONFIGURED);
	source->qos = qos;
	source->packing = param->packing;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	source->irc = param->irc;
	source->pto = param->pto;
	source->iso_interval = param->iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	source->encryption = param->encryption;
	if (source->encryption) {
		(void)memcpy(source->broadcast_code, param->broadcast_code,
			     sizeof(source->broadcast_code));
	}

	LOG_DBG("Broadcasting with ID 0x%6X", source->broadcast_id);

	*out_source = source;

	return 0;
}

int bt_bap_broadcast_source_reconfig(struct bt_bap_broadcast_source *source,
				     struct bt_bap_broadcast_source_param *param)
{
	struct bt_bap_broadcast_subgroup *subgroup;
	enum bt_bap_ep_state broadcast_state;
	struct bt_audio_codec_qos *qos;
	size_t subgroup_cnt;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	if (!valid_broadcast_source_param(param, source)) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_source_get_state(source) != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	/* Verify that the parameter counts do not exceed existing number of subgroups and streams*/
	subgroup_cnt = 0U;
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		const struct bt_bap_broadcast_source_subgroup_param *subgroup_param =
			&param->params[subgroup_cnt];
		const size_t subgroup_stream_param_cnt = subgroup_param->params_count;
		struct bt_bap_stream *stream;
		size_t subgroup_stream_cnt = 0U;

		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			subgroup_stream_cnt++;
		}

		/* Verify that the param stream is in the subgroup */
		for (size_t i = 0U; i < subgroup_param->params_count; i++) {
			struct bt_bap_stream *subgroup_stream;
			struct bt_bap_stream *param_stream;
			bool stream_in_subgroup = false;

			param_stream = subgroup_param->params[i].stream;

			SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, subgroup_stream, _node) {
				if (subgroup_stream == param_stream) {
					stream_in_subgroup = true;
					break;
				}
			}

			if (!stream_in_subgroup) {
				LOG_DBG("Invalid param->params[%zu]->param[%zu].stream "
					"not in subgroup",
					subgroup_cnt, i);
				return -EINVAL;
			}
		}

		if (subgroup_stream_cnt < subgroup_stream_param_cnt) {
			LOG_DBG("Invalid param->params[%zu]->params_count: %zu "
				"(only %zu streams in subgroup)",
				subgroup_cnt, subgroup_stream_param_cnt, subgroup_stream_cnt);
			return -EINVAL;
		}

		subgroup_cnt++;
	}

	if (subgroup_cnt < param->params_count) {
		LOG_DBG("Invalid param->params_count: %zu (only %zu subgroups in source)",
			param->params_count, subgroup_cnt);
		return -EINVAL;
	}

	qos = param->qos;

	/* We update up to the first param->params_count subgroups */
	for (size_t i = 0U; i < param->params_count; i++) {
		const struct bt_bap_broadcast_source_subgroup_param *subgroup_param;
		struct bt_audio_codec_cfg *codec_cfg;
		struct bt_bap_stream *stream;

		if (i == 0) {
			subgroup =
				SYS_SLIST_PEEK_HEAD_CONTAINER(&source->subgroups, subgroup, _node);
		} else {
			subgroup = SYS_SLIST_PEEK_NEXT_CONTAINER(subgroup, _node);
		}

		subgroup_param = &param->params[i];
		codec_cfg = subgroup_param->codec_cfg;
		subgroup->codec_cfg = codec_cfg;

		for (size_t j = 0U; j < subgroup_param->params_count; j++) {
			const struct bt_bap_broadcast_source_stream_param *stream_param;
			struct bt_audio_broadcast_stream_data *stream_data;
			struct bt_bap_stream *subgroup_stream;
			size_t stream_idx;

			stream_param = &subgroup_param->params[j];
			stream = stream_param->stream;

			stream_idx = 0U;
			SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, subgroup_stream, _node) {
				if (subgroup_stream == stream) {
					break;
				}

				stream_idx++;
			}

			/* Store the BIS specific codec configuration data in the broadcast source.
			 * It is stored in the broadcast* source, instead of the stream object,
			 * as this is only relevant for the broadcast source, and not used
			 * for unicast or broadcast sink.
			 */
			stream_data = &source->stream_data[stream_idx];
			(void)memcpy(stream_data->data, stream_param->data, stream_param->data_len);
			stream_data->data_len = stream_param->data_len;
		}

		/* Apply the codec_cfg to all streams in the subgroup, and not just the ones in the
		 * params
		 */
		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			struct bt_iso_chan_io_qos *iso_qos;

			iso_qos = stream->ep->iso->chan.qos->tx;
			bt_bap_stream_attach(NULL, stream, stream->ep, codec_cfg);
			bt_audio_codec_cfg_to_iso_path(iso_qos->path, codec_cfg);
		}
	}

	/* Finally we apply the new qos and to all streams */
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		struct bt_bap_stream *stream;

		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			struct bt_iso_chan_io_qos *iso_qos;

			iso_qos = stream->ep->iso->chan.qos->tx;
			bt_audio_codec_qos_to_iso_qos(iso_qos, qos);
			stream->qos = qos;
		}
	}

	source->qos = qos;

	return 0;
}

int bt_bap_broadcast_source_update_metadata(struct bt_bap_broadcast_source *source,
					    const uint8_t meta[], size_t meta_len)
{
	struct bt_bap_broadcast_subgroup *subgroup;
	enum bt_bap_ep_state broadcast_state;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");

		return -EINVAL;
	}

	CHECKIF((meta == NULL && meta_len != 0) || (meta != NULL && meta_len == 0)) {
		LOG_DBG("Invalid metadata combination: %p %zu", meta, meta_len);

		return -EINVAL;
	}

	CHECKIF(meta_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
		LOG_DBG("Invalid meta_len: %zu (max %d)", meta_len,
			CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);

		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_source_get_state(source) != BT_BAP_EP_STATE_STREAMING) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);

		return -EBADMSG;
	}

	/* TODO: We should probably find a way to update the metadata
	 * for each subgroup individually
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		memset(subgroup->codec_cfg->meta, 0, sizeof(subgroup->codec_cfg->meta));
		memcpy(subgroup->codec_cfg->meta, meta, meta_len);
		subgroup->codec_cfg->meta_len = meta_len;
	}

	return 0;
}

int bt_bap_broadcast_source_start(struct bt_bap_broadcast_source *source, struct bt_le_ext_adv *adv)
{
	struct bt_iso_chan *bis[BROADCAST_STREAM_CNT];
	struct bt_iso_big_create_param param = { 0 };
	struct bt_bap_broadcast_subgroup *subgroup;
	enum bt_bap_ep_state broadcast_state;
	struct bt_bap_stream *stream;
	size_t bis_count;
	int err;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	CHECKIF(adv == NULL) {
		LOG_DBG("adv is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_source_get_state(source) != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	bis_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			bis[bis_count++] = bt_bap_stream_iso_chan_get(stream);
		}
	}

	/* Create BIG */
	param.num_bis = bis_count;
	param.bis_channels = bis;
	param.framing = source->qos->framing;
	param.packing = source->packing;
	param.interval = source->qos->interval;
	param.latency = source->qos->latency;
	param.encryption = source->encryption;
	if (param.encryption) {
		(void)memcpy(param.bcode, source->broadcast_code,
			     sizeof(param.bcode));
	}
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	param.irc = source->irc;
	param.pto = source->pto;
	param.iso_interval = source->iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	/* Set the enabling state early in case that the BIS is connected before we can manage to
	 * set it afterwards
	 */
	broadcast_source_set_state(source, BT_BAP_EP_STATE_ENABLING);

	err = bt_iso_big_create(adv, &param, &source->big);
	if (err != 0) {
		LOG_DBG("Failed to create BIG: %d", err);
		broadcast_source_set_state(source, BT_BAP_EP_STATE_QOS_CONFIGURED);

		return err;
	}

	return 0;
}

int bt_bap_broadcast_source_stop(struct bt_bap_broadcast_source *source)
{
	enum bt_bap_ep_state broadcast_state;
	int err;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state != BT_BAP_EP_STATE_STREAMING &&
	    broadcast_state != BT_BAP_EP_STATE_ENABLING) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	if (source->big == NULL) {
		LOG_DBG("Source is not started");
		return -EALREADY;
	}

	err =  bt_iso_big_terminate(source->big);
	if (err) {
		LOG_DBG("Failed to terminate BIG (err %d)", err);
		return err;
	}

	source->big = NULL;

	return 0;
}

int bt_bap_broadcast_source_delete(struct bt_bap_broadcast_source *source)
{
	enum bt_bap_ep_state broadcast_state;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	broadcast_source_set_state(source, BT_BAP_EP_STATE_IDLE);

	/* Reset the broadcast source */
	broadcast_source_cleanup(source);

	return 0;
}

int bt_bap_broadcast_source_get_id(struct bt_bap_broadcast_source *source,
				   uint32_t *const broadcast_id)
{
	enum bt_bap_ep_state broadcast_state;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	CHECKIF(broadcast_id == NULL) {
		LOG_DBG("broadcast_id is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state == BT_BAP_EP_STATE_IDLE) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	*broadcast_id = source->broadcast_id;

	return 0;
}

int bt_bap_broadcast_source_get_base(struct bt_bap_broadcast_source *source,
				     struct net_buf_simple *base_buf)
{
	enum bt_bap_ep_state broadcast_state;

	CHECKIF(source == NULL) {
		LOG_DBG("source is NULL");
		return -EINVAL;
	}

	CHECKIF(base_buf == NULL) {
		LOG_DBG("base_buf is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state == BT_BAP_EP_STATE_IDLE) {
		LOG_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	if (!encode_base(source, base_buf)) {
		LOG_DBG("base_buf %p with size %u not large enough", base_buf, base_buf->size);

		return -EMSGSIZE;
	}

	return 0;
}
