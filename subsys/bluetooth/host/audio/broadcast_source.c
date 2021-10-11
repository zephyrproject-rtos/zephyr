/*  Bluetooth Audio Broadcast Source */

/*
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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_BROADCAST_SOURCE)
#define LOG_MODULE_NAME bt_audio_broadcast_source
#include "common/log.h"

#include "chan.h"
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
	struct bt_audio_base_ad_subgroup subgroups[BROADCAST_SUBGROUP_CNT];
} __packed;

static struct bt_audio_broadcast_source broadcast_sources[BROADCAST_SRC_CNT];

static int bt_audio_set_base(const struct bt_audio_broadcast_source *source,
			     struct bt_codec *codec);

static int bt_audio_broadcast_source_setup_chan(uint8_t index,
						struct bt_audio_chan *chan,
						struct bt_codec *codec,
						struct bt_codec_qos *qos)
{
	struct bt_audio_ep *ep;
	int err;

	if (chan->state != BT_AUDIO_CHAN_IDLE) {
		BT_DBG("Channel %p not idle", chan);
		return -EALREADY;
	}

	ep = bt_audio_ep_broadcaster_new(index, BT_AUDIO_SOURCE);
	if (ep == NULL) {
		BT_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	bt_audio_chan_attach(NULL, chan, ep, NULL, codec);
	chan->qos = qos;
	err = bt_audio_chan_codec_qos_to_iso_qos(chan->iso->qos, qos);
	if (err) {
		BT_ERR("Unable to convert codec QoS to ISO QoS");
		return err;
	}

	return 0;
}

static void bt_audio_encode_base(const struct bt_audio_broadcast_source *source,
				 struct bt_codec *codec,
				 struct net_buf_simple *buf)
{
	uint8_t bis_index;
	uint8_t *start;
	uint8_t len;

	__ASSERT(source->subgroup_count == BROADCAST_SUBGROUP_CNT,
		 "Cannot encode BASE with more than a single subgroup");

	net_buf_simple_add_le16(buf, BT_UUID_BASIC_AUDIO_VAL);
	net_buf_simple_add_le24(buf, source->pd);
	net_buf_simple_add_u8(buf, source->subgroup_count);
	/* TODO: The following encoding should be done for each subgroup once
	 * supported
	 */
	net_buf_simple_add_u8(buf, source->chan_count);
	net_buf_simple_add_u8(buf, codec->id);
	net_buf_simple_add_le16(buf, codec->cid);
	net_buf_simple_add_le16(buf, codec->vid);

	/* Insert codec configuration data in LTV format */
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->data_count; i++) {
		const struct bt_data *codec_data = &codec->data[i].data;

		net_buf_simple_add_u8(buf, codec_data->data_len);
		net_buf_simple_add_u8(buf, codec_data->type);
		net_buf_simple_add_mem(buf, codec_data->data,
				       codec_data->data_len -
					sizeof(codec_data->type));

	}
	/* Calcute length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Insert codec metadata in LTV format*/
	start = net_buf_simple_add(buf, sizeof(len));
	for (int i = 0; i < codec->meta_count; i++) {
		const struct bt_data *metadata = &codec->meta[i].data;

		net_buf_simple_add_u8(buf, metadata->data_len);
		net_buf_simple_add_u8(buf, metadata->type);
		net_buf_simple_add_mem(buf, metadata->data,
				       metadata->data_len -
					sizeof(metadata->type));
	}
	/* Calcute length of codec config data */
	len = net_buf_simple_tail(buf) - start - sizeof(len);
	/* Update the length field */
	*start = len;

	/* Create BIS index bitfield */
	bis_index = 0;
	for (int i = 0; i < source->chan_count; i++) {
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

		err = bt_rand(&source->broadcast_id, BT_BROADCAST_ID_SIZE);
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
	for (size_t i = 0; i < source->chan_count; i++) {
		struct bt_audio_chan *chan = &source->chans[i];

		chan->ep->chan = NULL;
		chan->ep = NULL;
		chan->codec = NULL;
		chan->qos = NULL;
		chan->iso = NULL;
		chan->state = BT_AUDIO_CHAN_IDLE;
	}

	(void)memset(source, 0, sizeof(*source));
}

int bt_audio_broadcast_source_create(struct bt_audio_chan *chans,
				     uint8_t num_chan,
				     struct bt_codec *codec,
				     struct bt_codec_qos *qos,
				     struct bt_audio_broadcast_source **out_source)
{
	struct bt_audio_broadcast_source *source;
	struct bt_data ad;
	uint8_t index;
	int err;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_BROADCAST_ID_SIZE);

	/* TODO: Validate codec and qos values */

	/* TODO: The API currently only requires a bt_audio_chan object from
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

	CHECKIF(chans == NULL) {
		BT_DBG("chans is NULL");
		return -EINVAL;
	}

	CHECKIF(codec == NULL) {
		BT_DBG("codec is NULL");
		return -EINVAL;
	}

	CHECKIF(num_chan > BROADCAST_STREAM_CNT) {
		BT_DBG("Too many channels provided: %u/%u",
		       num_chan, BROADCAST_STREAM_CNT);
		return -EINVAL;
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

	source->chans = chans;
	source->chan_count = num_chan;
	for (size_t i = 0; i < num_chan; i++) {
		struct bt_audio_chan *chan = &chans[i];

		err = bt_audio_broadcast_source_setup_chan(index, chan, codec, qos);
		if (err != 0) {
			BT_DBG("Failed to setup chans[%zu]: %d", i, err);
			broadcast_source_cleanup(source);
			return err;
		}

		source->bis[i] = &chan->ep->iso;
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

	source->subgroup_count = BROADCAST_SUBGROUP_CNT;
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

	for (size_t i = 0; i < source->chan_count; i++) {
		struct bt_audio_chan *chan = &chans[i];

		chan->ep->broadcast_source = source;
		bt_audio_chan_set_state(chan, BT_AUDIO_CHAN_CONFIGURED);
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
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	for (size_t i = 0; i < source->chan_count; i++) {
		chan = &source->chans[i];

		bt_audio_chan_attach(NULL, chan, chan->ep, NULL, codec);
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
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
		return -EBADMSG;
	}

	/* Create BIG */
	param.num_bis = source->chan_count;
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

	return 0;
}

int bt_audio_broadcast_source_stop(struct bt_audio_broadcast_source *source)
{
	struct bt_audio_chan *chan;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_STREAMING) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_STREAMING state: %u",
		       chan, chan->state);
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
	struct bt_audio_chan *chan;
	struct bt_le_ext_adv *adv;
	int err;

	CHECKIF(source == NULL) {
		BT_DBG("source is NULL");
		return -EINVAL;
	}

	chan = &source->chans[0];

	if (chan->state != BT_AUDIO_CHAN_CONFIGURED) {
		BT_DBG("Source chan %p is not in the BT_AUDIO_CHAN_CONFIGURED state: %u",
		       chan, chan->state);
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
