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

/* internal bt_audio_base_ad* structs are primarily designed to help
 * calculate the maximum advertising data length
 */
struct bt_audio_base_ad_bis_specific_data {
	uint8_t index;
	uint8_t codec_config_len; /* currently unused and shall always be 0 */
} __packed;

struct bt_audio_base_ad_codec_data {
	uint8_t type;
	uint8_t data_len;
	uint8_t data[CONFIG_BT_CODEC_MAX_DATA_LEN];
} __packed;

struct bt_audio_base_ad_codec_metadata {
	uint8_t type;
	uint8_t data_len;
	uint8_t data[CONFIG_BT_CODEC_MAX_METADATA_LEN];
} __packed;

struct bt_audio_base_ad_subgroup {
	uint8_t bis_cnt;
	uint8_t codec_id;
	uint16_t company_id;
	uint16_t vendor_id;
	uint8_t codec_config_len;
	struct bt_audio_base_ad_codec_data codec_config[CONFIG_BT_CODEC_MAX_DATA_COUNT];
	uint8_t metadata_len;
	struct bt_audio_base_ad_codec_metadata metadata[CONFIG_BT_CODEC_MAX_METADATA_COUNT];
	struct bt_audio_base_ad_bis_specific_data bis_data[BROADCAST_STREAM_CNT];
} __packed;

struct bt_audio_base_ad {
	uint16_t uuid_val;
	struct bt_audio_base_ad_subgroup subgroups[CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT];
} __packed;

static struct bt_audio_iso broadcast_source_iso
	[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT][BROADCAST_STREAM_CNT];
static struct bt_audio_ep broadcast_source_eps
	[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT][BROADCAST_STREAM_CNT];
static struct bt_audio_broadcast_source broadcast_sources[CONFIG_BT_AUDIO_BROADCAST_SRC_COUNT];

static int bt_audio_set_base(const struct bt_audio_broadcast_source *source,
			     struct bt_codec *codec);

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

static int bt_audio_broadcast_source_setup_stream(uint8_t index,
						struct bt_audio_stream *stream,
						struct bt_codec *codec,
						struct bt_codec_qos *qos)
{
	struct bt_audio_ep *ep;

	if (stream->group != NULL) {
		BT_DBG("Channel %p already in group %p", stream, stream->group);
		return -EALREADY;
	}

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

	return 0;
}

static void bt_audio_encode_base(const struct bt_audio_broadcast_source *source,
				 struct bt_codec *codec,
				 struct net_buf_simple *buf)
{
	uint8_t bis_index;
	uint8_t *start;
	uint8_t len;

	__ASSERT(source->subgroup_count == CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT,
		 "Cannot encode BASE with more than a %u subgroups",
		 CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT);

	net_buf_simple_add_le16(buf, BT_UUID_BASIC_AUDIO_VAL);
	net_buf_simple_add_le24(buf, source->pd);
	net_buf_simple_add_u8(buf, source->subgroup_count);
	/* TODO: The following encoding should be done for each subgroup once
	 * supported
	 */
	net_buf_simple_add_u8(buf, source->stream_count);
	net_buf_simple_add_u8(buf, codec->id);
	net_buf_simple_add_le16(buf, codec->cid);
	net_buf_simple_add_le16(buf, codec->vid);

	/* Insert codec configuration data in LTV format */
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->data_count; i++) {
		const struct bt_data *codec_data = &codec->data[i].data;

		net_buf_simple_add_u8(buf, codec_data->data_len + sizeof(codec_data->type));
		net_buf_simple_add_u8(buf, codec_data->type);
		net_buf_simple_add_mem(buf, codec_data->data, codec_data->data_len);

	}
	/* Calculate length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Insert codec metadata in LTV format*/
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->meta_count; i++) {
		const struct bt_data *metadata = &codec->meta[i].data;

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
	for (int i = 0; i < source->stream_count; i++) {
		bis_index++;
		net_buf_simple_add_u8(buf, bis_index);
		net_buf_simple_add_u8(buf, 0); /* unused length field */
	}

	/* NOTE: It is also possible to have the codec configuration data per
	 * BIS index. As our API does not support such specialized BISes we
	 * currently don't do that.
	 */
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

static int bt_audio_set_base(const struct bt_audio_broadcast_source *source,
			     struct bt_codec *codec)
{
	struct bt_data base_ad_data;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(base_buf, sizeof(struct bt_audio_base_ad));

	bt_audio_encode_base(source, codec, &base_buf);

	base_ad_data.type = BT_DATA_SVC_DATA16;
	base_ad_data.data_len = base_buf.len;
	base_ad_data.data = base_buf.data;

	err = bt_le_per_adv_set_data(source->adv, &base_ad_data, 1);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		return err;
	}

	return 0;
}

static void broadcast_source_cleanup(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_stream *stream, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&source->streams, stream, next, _node) {
		stream->ep->stream = NULL;
		stream->ep = NULL;
		stream->codec = NULL;
		stream->qos = NULL;
		stream->iso = NULL;
		stream->group = NULL;

		sys_slist_remove(&source->streams, NULL, &stream->_node);
	}

	(void)memset(source, 0, sizeof(*source));
}

int bt_audio_broadcast_source_create(struct bt_audio_stream *streams[],
				     size_t num_stream,
				     struct bt_codec *codec,
				     struct bt_codec_qos *qos,
				     struct bt_audio_broadcast_source **out_source)
{
	struct bt_audio_broadcast_source *source;
	struct bt_data ad;
	uint8_t index;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);

	/* TODO: Validate codec and qos values */

	/* TODO: The API currently only requires a bt_audio_stream object from
	 * the user. We could modify the API such that the extended (and
	 * periodic advertising enabled) advertiser was provided by the user as
	 * well (similar to the ISO API), or even provide the BIG.
	 *
	 * The caveat of that type of API, instead of this, where we, the stack,
	 * control the advertiser, is that the user will be able to change the
	 * advertising data (thus making the broadcast source non-functional in
	 * terms of BAP compliance), or even stop the advertiser without
	 * stopping the BIG (which also goes against the BAP specification).
	 */

	CHECKIF(out_source == NULL) {
		BT_DBG("out_source is NULL");
		return -EINVAL;
	}
	/* Set out_source to NULL until the source has actually been created */
	*out_source = NULL;

	CHECKIF(streams == NULL) {
		BT_DBG("streams is NULL");
		return -EINVAL;
	}

	CHECKIF(codec == NULL) {
		BT_DBG("codec is NULL");
		return -EINVAL;
	}

	CHECKIF(num_stream > BROADCAST_STREAM_CNT) {
		BT_DBG("Too many streams provided: %u/%u",
		       num_stream, BROADCAST_STREAM_CNT);
		return -EINVAL;
	}

	for (size_t i = 0; i < num_stream; i++) {
		CHECKIF(streams[i] == NULL) {
			BT_DBG("streams[%zu] is NULL", i);
			return -EINVAL;
		}
	}

	source = NULL;
	for (index = 0; index < ARRAY_SIZE(broadcast_sources); index++) {
		if (broadcast_sources[index].bis[0] == NULL) { /* Find free entry */
			source = &broadcast_sources[index];
			break;
		}
	}

	if (source == NULL) {
		BT_DBG("Could not allocate any more broadcast sources");
		return -ENOMEM;
	}

	for (size_t i = 0; i < num_stream; i++) {
		struct bt_audio_stream *stream = streams[i];

		err = bt_audio_broadcast_source_setup_stream(index, stream,
							     codec, qos);
		if (err != 0) {
			BT_DBG("Failed to setup streams[%zu]: %d", i, err);
			broadcast_source_cleanup(source);
			return err;
		}

		source->bis[i] = &stream->ep->iso->iso_chan;
		sys_slist_append(&source->streams, &stream->_node);
		source->stream_count++;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL,
				   &source->adv);
	if (err != 0) {
		BT_DBG("Failed to create advertising set (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(source->adv, BT_LE_PER_ADV_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to set periodic advertising parameters (err %d)",
		       err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* TODO: If updating the API to have a user-supplied advertiser, we
	 * should simply add the data here, instead of changing all of it.
	 * Similar, if the application changes the data, we should ensure
	 * that the audio advertising data is still present, similar to how
	 * the GAP device name is added.
	 */
	err = generate_broadcast_id(source);
	if (err != 0) {
		BT_DBG("Could not generate broadcast id: %d", err);
		return err;
	}
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, source->broadcast_id);
	ad.type = BT_DATA_SVC_DATA16;
	ad.data_len = ad_buf.len + sizeof(ad.type);
	ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(source->adv, &ad, 1, NULL, 0);
	if (err != 0) {
		BT_DBG("Failed to set extended advertising data (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	source->subgroup_count = CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT;
	source->pd = qos->pd;
	err = bt_audio_set_base(source, codec);
	if (err != 0) {
		BT_DBG("Failed to set base data (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(source->adv);
	if (err != 0) {
		BT_DBG("Failed to enable periodic advertising (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(source->adv,
				  BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		BT_DBG("Failed to start extended advertising (err %d)", err);
		broadcast_source_cleanup(source);
		return err;
	}

	for (size_t i = 0; i < source->stream_count; i++) {
		struct bt_audio_ep *ep = streams[i]->ep;

		ep->broadcast_source = source;
		broadcast_source_set_ep_state(ep,
					      BT_AUDIO_EP_STATE_QOS_CONFIGURED);
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
	struct bt_audio_stream *stream;
	sys_snode_t *head_node;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&source->streams)) {
		BT_DBG("Source does not have any streams");
		return -EINVAL;
	}

	head_node = sys_slist_peek_head(&source->streams);
	stream = CONTAINER_OF(head_node, struct bt_audio_stream, _node);

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (stream->ep == NULL) {
		BT_DBG("stream->ep is NULL");
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source stream %p invalid state: %u",
		       stream, stream->ep->status.state);
		return -EBADMSG;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&source->streams, stream, _node) {
		bt_audio_stream_attach(NULL, stream, stream->ep, codec);
	}

	err = bt_audio_set_base(source, codec);
	if (err != 0) {
		BT_DBG("Failed to set base data (err %d)", err);
		return err;
	}

	source->qos = qos;

	return 0;
}

int bt_audio_broadcast_source_start(struct bt_audio_broadcast_source *source)
{
	struct bt_iso_big_create_param param = { 0 };
	struct bt_audio_stream *stream;
	sys_snode_t *head_node;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&source->streams)) {
		BT_DBG("Source does not have any streams");
		return -EINVAL;
	}

	head_node = sys_slist_peek_head(&source->streams);
	stream = CONTAINER_OF(head_node, struct bt_audio_stream, _node);

	if (stream->ep == NULL) {
		BT_DBG("stream->ep is NULL");
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source stream %p invalid state: %u",
		       stream, stream->ep->status.state);
		return -EBADMSG;
	}

	/* Create BIG */
	param.num_bis = source->stream_count;
	param.bis_channels = source->bis;
	param.framing = source->qos->framing;
	param.packing = 0; /*  TODO: Add to QoS struct */
	param.interval = source->qos->interval;
	param.latency = source->qos->latency;

	err = bt_iso_big_create(source->adv, &param, &source->big);
	if (err != 0) {
		BT_DBG("Failed to create BIG: %d", err);
		return err;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&source->streams, stream, _node) {
		struct bt_audio_ep *ep = stream->ep;

		broadcast_source_set_ep_state(ep, BT_AUDIO_EP_STATE_ENABLING);
	}

	return 0;
}

int bt_audio_broadcast_source_stop(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_stream *stream;
	sys_snode_t *head_node;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&source->streams)) {
		BT_DBG("Source does not have any streams");
		return -EINVAL;
	}

	head_node = sys_slist_peek_head(&source->streams);
	stream = CONTAINER_OF(head_node, struct bt_audio_stream, _node);

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (stream->ep == NULL) {
		BT_DBG("stream->ep is NULL");
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING &&
	    stream->ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		BT_DBG("Broadcast source stream %p invalid state: %u",
		       stream, stream->ep->status.state);
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
	struct bt_audio_stream *stream;
	struct bt_le_ext_adv *adv;
	sys_snode_t *head_node;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&source->streams)) {
		BT_DBG("Source does not have any streams");
		return -EINVAL;
	}

	head_node = sys_slist_peek_head(&source->streams);
	stream = CONTAINER_OF(head_node, struct bt_audio_stream, _node);

	if (stream->ep == NULL) {
		BT_DBG("stream->ep is NULL");
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		BT_DBG("Broadcast source stream %p invalid state: %u",
		stream, stream->ep->status.state);
		return -EBADMSG;
	}

	adv = source->adv;

	__ASSERT(adv != NULL, "source %p adv is NULL", source);

	/* Stop periodic advertising */
	err = bt_le_per_adv_stop(adv);
	if (err != 0) {
		BT_DBG("Failed to stop periodic advertising (err %d)", err);
		return err;
	}

	/* Stop extended advertising */
	err = bt_le_ext_adv_stop(adv);
	if (err != 0) {
		BT_DBG("Failed to stop extended advertising (err %d)", err);
		return err;
	}

	/* Delete extended advertising set */
	err = bt_le_ext_adv_delete(adv);
	if (err != 0) {
		BT_DBG("Failed to delete extended advertising set (err %d)", err);
		return err;
	}

	/* Reset the broadcast source */
	broadcast_source_cleanup(source);

	return 0;
}
