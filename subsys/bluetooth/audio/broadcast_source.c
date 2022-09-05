/*  Bluetooth Audio Broadcast Source */

/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_BROADCAST_SOURCE)
#define LOG_MODULE_NAME bt_audio_broadcast_source
#include "common/log.h"

#include "endpoint.h"

struct bt_audio_broadcast_subgroup {
	/* The streams used to create the broadcast source */
	sys_slist_t streams;

	/* The codec of the subgroup */
	struct bt_codec *codec;

	/* List node */
	sys_snode_t _node;
};

static struct bt_audio_iso broadcast_source_iso
	[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT][BROADCAST_STREAM_CNT];
static struct bt_audio_ep broadcast_source_eps
	[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT][BROADCAST_STREAM_CNT];
static struct bt_audio_broadcast_subgroup broadcast_source_subgroups
	[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT][CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT];
static struct bt_audio_broadcast_source broadcast_sources[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT];


static void broadcast_source_set_ep_state(struct bt_audio_ep *ep, uint8_t state)
{
	uint8_t old_state;

	old_state = ep->status.state;

	BT_DBG("ep %p id 0x%02x %s -> %s", ep, ep->status.id,
	       bt_audio_ep_state_str(old_state),
	       bt_audio_ep_state_str(state));


	switch (old_state) {
	case BT_AUDIO_EP_STATE_IDLE:
		if (state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			BT_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		if (state != BT_AUDIO_EP_STATE_IDLE &&
		    state != BT_AUDIO_EP_STATE_ENABLING) {
			BT_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_AUDIO_EP_STATE_ENABLING:
		if (state != BT_AUDIO_EP_STATE_STREAMING) {
			BT_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_AUDIO_EP_STATE_STREAMING:
		if (state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			BT_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	default:
		BT_ERR("Invalid broadcast sync endpoint state: %s",
		       bt_audio_ep_state_str(old_state));
		return;
	}

	ep->status.state = state;

	if (state == BT_AUDIO_EP_STATE_IDLE) {
		struct bt_audio_stream *stream = ep->stream;

		if (stream != NULL) {
			stream->ep = NULL;
			stream->codec = NULL;
			ep->stream = NULL;
		}
	}
}

static void broadcast_source_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *stream = audio_iso->source_stream;
	struct bt_audio_stream_ops *ops = stream->ops;

	BT_DBG("stream %p ep %p", stream, stream->ep);

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(stream);
	}
}

static void broadcast_source_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *stream = audio_iso->source_stream;
	const struct bt_audio_stream_ops *ops;
	struct bt_audio_ep *ep;

	if (stream == NULL) {
		BT_ERR("Could not lookup stream by iso %p", chan);
		return;
	} else if (stream->ep == NULL) {
		BT_ERR("Stream not associated with an ep");
		return;
	}

	ops = stream->ops;
	ep = stream->ep;

	BT_DBG("stream %p ep %p", stream, ep);

	broadcast_source_set_ep_state(ep, BT_AUDIO_EP_STATE_STREAMING);

	if (ops != NULL && ops->started != NULL) {
		ops->started(stream);
	} else {
		BT_WARN("No callback for connected set");
	}
}

static void broadcast_source_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *stream = audio_iso->source_stream;
	const struct bt_audio_stream_ops *ops;

	if (stream == NULL) {
		BT_ERR("Could not lookup stream by iso %p", chan);
		return;
	} else if (stream->ep == NULL) {
		BT_ERR("Stream not associated with an ep");
		return;
	}

	ops = stream->ops;

	BT_DBG("stream %p ep %p reason 0x%02x", stream, stream->ep, reason);

	broadcast_source_set_ep_state(stream->ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream);
	} else {
		BT_WARN("No callback for stopped set");
	}
}

static struct bt_iso_chan_ops broadcast_source_iso_ops = {
	.sent		= broadcast_source_iso_sent,
	.connected	= broadcast_source_iso_connected,
	.disconnected	= broadcast_source_iso_disconnected,
};

bool bt_audio_ep_is_broadcast_src(const struct bt_audio_ep *ep)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_source_eps); i++) {
		if (PART_OF_ARRAY(broadcast_source_eps[i], ep)) {
			return true;
		}
	}

	return false;
}

static void broadcast_source_ep_init(struct bt_audio_ep *ep,
				     struct bt_audio_iso *iso)
{
	struct bt_iso_chan *iso_chan;

	BT_DBG("ep %p", ep);

	(void)memset(ep, 0, sizeof(*ep));
	ep->dir = BT_AUDIO_DIR_SOURCE;
	ep->iso = iso;

	iso_chan = &ep->iso->iso_chan;

	iso_chan->ops = &broadcast_source_iso_ops;
	iso_chan->qos = &iso->iso_qos;
	iso_chan->qos->rx = NULL;
	iso_chan->qos->tx = &iso->source_io_qos;
}

static struct bt_audio_ep *broadcast_source_new_ep(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_source_eps[index]); i++) {
		struct bt_audio_ep *ep = &broadcast_source_eps[index][i];

		/* If ep->stream is NULL the endpoint is unallocated */
		if (ep->stream == NULL) {
			/* Initialize - It is up to the caller to allocate the
			 * stream pointer.
			 */
			broadcast_source_ep_init(ep, &broadcast_source_iso[index][i]);
			return ep;
		}
	}

	return NULL;
}

static struct bt_audio_broadcast_subgroup *broadcast_source_new_subgroup(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_source_subgroups[index]); i++) {
		struct bt_audio_broadcast_subgroup *subgroup =
			&broadcast_source_subgroups[index][i];

		if (sys_slist_is_empty(&subgroup->streams)) {
			return subgroup;
		}
	}

	return NULL;
}

static int bt_audio_broadcast_source_setup_stream(uint8_t index,
						  struct bt_audio_stream *stream,
						  struct bt_codec *codec,
						  struct bt_codec_qos *qos,
						  struct bt_audio_broadcast_source *source)
{
	struct bt_audio_ep *ep;

	ep = broadcast_source_new_ep(index);
	if (ep == NULL) {
		BT_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	bt_audio_stream_attach(NULL, stream, ep, codec);
	ep->iso->source_stream = stream;
	stream->qos = qos;
	stream->iso->qos->tx->path = &ep->iso->source_path;
	stream->iso->qos->tx->path->cc = ep->iso->source_path_cc;
	stream->iso->qos->rx = NULL;
	bt_audio_codec_qos_to_iso_qos(stream->iso->qos->tx, qos);
	bt_audio_codec_to_iso_path(stream->iso->qos->tx->path, codec);
	ep->broadcast_source = source;

	return 0;
}

static bool bt_audio_encode_base_subgroup(struct bt_audio_broadcast_subgroup *subgroup,
					  struct bt_audio_broadcast_stream_data *stream_data,
					  uint8_t *streams_encoded,
					  struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;
	const struct bt_codec *codec;
	uint8_t stream_count;
	uint8_t bis_index;
	uint8_t *start;
	uint8_t len;

	stream_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
		stream_count++;
	}

	codec = subgroup->codec;

	net_buf_simple_add_u8(buf, stream_count);
	net_buf_simple_add_u8(buf, codec->id);
	net_buf_simple_add_le16(buf, codec->cid);
	net_buf_simple_add_le16(buf, codec->vid);


	/* Insert codec configuration data in LTV format */
	start = net_buf_simple_add(buf, sizeof(len));

	for (int i = 0; i < codec->data_count; i++) {
		const struct bt_data *codec_data = &codec->data[i].data;

		if ((buf->size - buf->len) < (sizeof(codec_data->data_len) +
					      sizeof(codec_data->type) +
					      codec_data->data_len)) {
			BT_DBG("No room for codec[%d] with len %u",
			       i, codec_data->data_len);

			return false;
		}

		net_buf_simple_add_u8(buf, codec_data->data_len + sizeof(codec_data->type));
		net_buf_simple_add_u8(buf, codec_data->type);
		net_buf_simple_add_mem(buf, codec_data->data, codec_data->data_len);

	}
	/* Calculate length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	if ((buf->size - buf->len) < sizeof(len)) {
		BT_DBG("No room for metadata length");

		return false;
	}

	/* Insert codec metadata in LTV format*/
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->meta_count; i++) {
		const struct bt_data *metadata = &codec->meta[i].data;

		if ((buf->size - buf->len) < (sizeof(metadata->data_len) +
					      sizeof(metadata->type) +
					      metadata->data_len)) {
			BT_DBG("No room for metadata[%d] with len %u",
			       i, metadata->data_len);

			return false;
		}

		net_buf_simple_add_u8(buf, metadata->data_len + sizeof(metadata->type));
		net_buf_simple_add_u8(buf, metadata->type);
		net_buf_simple_add_mem(buf, metadata->data, metadata->data_len);
	}
	/* Calculate length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Create BIS index bitfield */
	bis_index = 0;
	for (int i = 0; i < stream_count; i++) {
		bis_index++;
		if ((buf->size - buf->len) < (sizeof(bis_index) + sizeof(uint8_t))) {
			BT_DBG("No room for BIS[%d] index", i);

			return false;
		}

		net_buf_simple_add_u8(buf, bis_index);

		if ((buf->size - buf->len) < sizeof(len)) {
			BT_DBG("No room for bis codec config length");

			return false;
		}

		/* Insert codec configuration data in LTV format */
		start = net_buf_simple_add(buf, sizeof(len));

		for (size_t j = 0U; j < stream_data[i].data_count; j++) {
			const struct bt_data *codec_data = &stream_data[i].data[j].data;

			if ((buf->size - buf->len) < (sizeof(codec_data->data_len) +
							sizeof(codec_data->type) +
							codec_data->data_len)) {
				BT_DBG("No room for BIS [%u] codec[%zu] with len %u",
				       bis_index, j, codec_data->data_len);

				return false;
			}

			net_buf_simple_add_u8(buf, codec_data->data_len + sizeof(codec_data->type));
			net_buf_simple_add_u8(buf, codec_data->type);
			net_buf_simple_add_mem(buf, codec_data->data, codec_data->data_len);
		}
		/* Calculate length of codec config data */
		len = net_buf_simple_tail(buf) - start - sizeof(len);
		/* Update the length field */
		*start = len;

		streams_encoded++;
	}

	return true;
}

static bool bt_audio_encode_base(struct bt_audio_broadcast_source *source,
				 struct net_buf_simple *buf)
{
	struct bt_audio_broadcast_subgroup *subgroup;
	uint8_t streams_encoded;
	uint8_t subgroup_count;

	/* 13 is the size of the fixed size values following this check */
	if ((buf->size - buf->len) < 13) {
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
		if (!bt_audio_encode_base_subgroup(subgroup,
						   &source->stream_data[streams_encoded],
						   &streams_encoded,
						   buf)) {
			return false;
		}
	}

	return true;
}

static int generate_broadcast_id(struct bt_audio_broadcast_source *source)
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

static void broadcast_source_cleanup(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_broadcast_subgroup *subgroup, *next_subgroup;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&source->subgroups, subgroup,
					  next_subgroup, _node) {
		struct bt_audio_stream *stream, *next_stream;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&subgroup->streams, stream,
						  next_stream, _node) {
			stream->ep->stream = NULL;
			stream->ep = NULL;
			stream->codec = NULL;
			stream->qos = NULL;
			stream->iso = NULL;
			stream->group = NULL;

			sys_slist_remove(&subgroup->streams, NULL,
					 &stream->_node);
		}
		sys_slist_remove(&source->subgroups, NULL, &subgroup->_node);
	}

	(void)memset(source, 0, sizeof(*source));
}

static bool valid_create_param(const struct bt_audio_broadcast_source_create_param *param)
{
	const struct bt_codec_qos *qos;

	CHECKIF(param == NULL) {
		BT_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->subgroup_count == 0U) {
		BT_DBG("param->count is 0");
		return false;
	}


	qos = param->qos;
	CHECKIF(qos == NULL) {
		BT_DBG("param->qos is NULL");
		return false;
	}

	CHECKIF(!bt_audio_valid_qos(qos)) {
		BT_DBG("param->qos is invalid");
		return false;
	}

	for (size_t i = 0U; i < param->subgroup_count; i++) {
		const struct bt_audio_broadcast_source_subgroup_param *subgroup_param;

		subgroup_param = &param->subgroup_params[i];

		CHECKIF(subgroup_param->stream_count == 0U) {
			BT_DBG("subgroup_params[%zu].count is 0", i);
			return false;
		}

		CHECKIF(subgroup_param->codec == NULL) {
			BT_DBG("subgroup_params[%zu].codec is NULL", i);
			return false;
		}

		for (size_t j = 0U; j < subgroup_param->stream_count; j++) {
			const struct bt_audio_broadcast_source_stream_param *stream_param;

			stream_param = &subgroup_param->stream_params[j];

			CHECKIF(stream_param->stream == NULL) {
				BT_DBG("subgroup_params[%zu].stream_params[%zu]->stream is NULL",
				       i, j);
				return false;
			}

			CHECKIF(stream_param->stream->group != NULL) {
				BT_DBG("subgroup_params[%zu].stream_params[%zu]->stream is "
				       "already part of group %p",
				       i, j, stream_param->stream->group);
				return false;
			}
		}
	}

	return true;
}

static enum bt_audio_state broadcast_source_get_state(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_broadcast_subgroup *subgroup;
	struct bt_audio_stream *stream;
	sys_snode_t *head_node;

	if (source == NULL) {
		BT_DBG("source is NULL");
		return BT_AUDIO_EP_STATE_IDLE;
	}

	if (sys_slist_is_empty(&source->subgroups)) {
		BT_DBG("Source does not have any streams");
		return BT_AUDIO_EP_STATE_IDLE;
	}

	/* Get the first stream */
	head_node = sys_slist_peek_head(&source->subgroups);
	subgroup = CONTAINER_OF(head_node, struct bt_audio_broadcast_subgroup, _node);

	head_node = sys_slist_peek_head(&subgroup->streams);
	stream = CONTAINER_OF(head_node, struct bt_audio_stream, _node);

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (stream->ep == NULL) {
		BT_DBG("stream->ep is NULL");
		return BT_AUDIO_EP_STATE_IDLE;
	}

	return stream->ep->status.state;
}

int bt_audio_broadcast_source_create(struct bt_audio_broadcast_source_create_param *param,
				     struct bt_audio_broadcast_source **out_source)
{
	struct bt_audio_broadcast_subgroup *subgroup;
	struct bt_audio_broadcast_source *source;
	struct bt_codec_qos *qos;
	size_t stream_count;
	uint8_t index;
	int err;

	CHECKIF(out_source == NULL) {
		BT_DBG("out_source is NULL");
		return -EINVAL;
	}
	/* Set out_source to NULL until the source has actually been created */
	*out_source = NULL;

	if (!valid_create_param(param)) {
		BT_DBG("Invalid parameters");
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
		BT_DBG("Could not allocate any more broadcast sources");
		return -ENOMEM;
	}

	stream_count = 0U;

	qos = param->qos;
	/* Go through all subgroups and streams and setup each setup with an
	 * endpoint
	 */
	for (size_t i = 0U; i < param->subgroup_count; i++) {
		const struct bt_audio_broadcast_source_subgroup_param *subgroup_param;


		subgroup_param = &param->subgroup_params[i];

		subgroup = broadcast_source_new_subgroup(index);
		if (subgroup == NULL) {
			BT_DBG("Could not allocate new broadcast subgroup");
			broadcast_source_cleanup(source);
			return -ENOMEM;
		}

		subgroup->codec = subgroup_param->codec;
		sys_slist_append(&source->subgroups, &subgroup->_node);

		for (size_t j = 0U; j < subgroup_param->stream_count; j++) {
			const struct bt_audio_broadcast_source_stream_param *stream_param;
			struct bt_audio_stream *stream;

			stream_param = &subgroup_param->stream_params[j];
			stream = stream_param->stream;

			err = bt_audio_broadcast_source_setup_stream(index,
								     stream,
								     subgroup_param->codec,
								     qos,
								     source);
			if (err != 0) {
				BT_DBG("Failed to setup streams[%zu]: %d", i, err);
				broadcast_source_cleanup(source);
				return err;
			}

			/* Store the BIS specific codec configuration data in
			 * the broadcast source. It is stored in the broadcast
			 * source, instead of the stream object, as this is
			 * only relevant for the broadcast source, and not used
			 * for unicast or broadcast sink.
			 */
			(void)memcpy(source->stream_data[stream_count].data,
				     stream_param->data,
				     stream_param->data_count * sizeof(*stream_param->data));
			source->stream_data[stream_count].data_count = stream_param->data_count;

			sys_slist_append(&subgroup->streams, &stream->_node);
			stream_count++;

			/* Check that we are not above the maximum BIS count */
			if (stream_count > BROADCAST_STREAM_CNT) {
				BT_DBG("Cannot create broadcaster with %zu streams",
				       stream_count);
				broadcast_source_cleanup(source);
				return err;
			}
		}
	}

	err = generate_broadcast_id(source);
	if (err != 0) {
		BT_DBG("Could not generate broadcast id: %d", err);
		return err;
	}

	/* Finalize state changes and store information */
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		struct bt_audio_stream *stream;

		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			broadcast_source_set_ep_state(stream->ep,
						      BT_AUDIO_EP_STATE_QOS_CONFIGURED);
		}
	}
	source->qos = qos;

	BT_DBG("Broadcasting with ID 0x%6X", source->broadcast_id);

	*out_source = source;

	return 0;
}

int bt_audio_broadcast_source_reconfig(struct bt_audio_broadcast_source *source,
				       struct bt_codec *codec,
				       struct bt_codec_qos *qos)
{
	struct bt_audio_broadcast_subgroup *subgroup;
	enum bt_audio_state broadcast_state;
	struct bt_audio_stream *stream;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_source_get_state(source) != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source invalid state: %u",
		       broadcast_state);
		return -EBADMSG;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			bt_audio_stream_attach(NULL, stream, stream->ep, codec);
		}
	}

	source->qos = qos;

	return 0;
}

int bt_audio_broadcast_source_start(struct bt_audio_broadcast_source *source,
				    struct bt_le_ext_adv *adv)
{
	struct bt_iso_chan *bis[BROADCAST_STREAM_CNT];
	struct bt_iso_big_create_param param = { 0 };
	struct bt_audio_broadcast_subgroup *subgroup;
	enum bt_audio_state broadcast_state;
	struct bt_audio_stream *stream;
	size_t bis_count;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_source_get_state(source) != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	bis_count = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			bis[bis_count++] = stream->iso;
		}
	}

	/* Create BIG */
	param.num_bis = bis_count;
	param.bis_channels = bis;
	param.framing = source->qos->framing;
	param.packing = 0; /*  TODO: Add to QoS struct */
	param.interval = source->qos->interval;
	param.latency = source->qos->latency;

	err = bt_iso_big_create(adv, &param, &source->big);
	if (err != 0) {
		BT_DBG("Failed to create BIG: %d", err);
		return err;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&source->subgroups, subgroup, _node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&subgroup->streams, stream, _node) {
			struct bt_audio_ep *ep = stream->ep;

			broadcast_source_set_ep_state(ep, BT_AUDIO_EP_STATE_ENABLING);
		}
	}

	return 0;
}

int bt_audio_broadcast_source_stop(struct bt_audio_broadcast_source *source)
{
	enum bt_audio_state broadcast_state;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state != BT_AUDIO_EP_STATE_STREAMING &&
	    broadcast_state != BT_AUDIO_EP_STATE_ENABLING) {
		BT_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	if (source->big == NULL) {
		BT_DBG("Source is not started");
		return -EALREADY;
	}

	err =  bt_iso_big_terminate(source->big);
	if (err) {
		BT_DBG("Failed to terminate BIG (err %d)", err);
		return err;
	}

	source->big = NULL;

	return 0;
}

int bt_audio_broadcast_source_delete(struct bt_audio_broadcast_source *source)
{
	enum bt_audio_state broadcast_state;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	broadcast_state = broadcast_source_get_state(source);
	if (broadcast_state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source invalid state: %u", broadcast_state);
		return -EBADMSG;
	}

	/* Reset the broadcast source */
	broadcast_source_cleanup(source);

	return 0;
}

int bt_audio_broadcast_source_get_id(const struct bt_audio_broadcast_source *source,
				     uint32_t *const broadcast_id)
{
	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	CHECKIF(broadcast_id == NULL) {
		BT_DBG("broadcast_id is NULL");
		return -EINVAL;
	}

	*broadcast_id = source->broadcast_id;

	return 0;
}

int bt_audio_broadcast_source_get_base(struct bt_audio_broadcast_source *source,
				       struct net_buf_simple *base_buf)
{
	if (!bt_audio_encode_base(source, base_buf)) {
		BT_DBG("base_buf %p with size %u not large enough",
		       base_buf, base_buf->size);

		return -EMSGSIZE;
	}

	return 0;
}
