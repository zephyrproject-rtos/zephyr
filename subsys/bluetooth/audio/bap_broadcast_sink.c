/*  Bluetooth Audio Broadcast Sink */

/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/bap.h>

#include "../host/conn_internal.h"
#include "../host/iso_internal.h"

#include "bap_iso.h"
#include "bap_endpoint.h"
#include "audio_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_broadcast_sink, CONFIG_BT_BAP_BROADCAST_SINK_LOG_LEVEL);

#include "common/bt_str.h"

#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define BROADCAST_SYNC_MIN_INDEX  (BIT(1))

/* any value above 0xFFFFFF is invalid, so we can just use 0xFFFFFFFF to denote
 * invalid broadcast ID
 */
#define INVALID_BROADCAST_ID 0xFFFFFFFF

static struct bt_bap_ep broadcast_sink_eps[CONFIG_BT_BAP_BROADCAST_SNK_COUNT]
					  [CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct bt_bap_broadcast_sink broadcast_sinks[CONFIG_BT_BAP_BROADCAST_SNK_COUNT];

struct codec_cap_lookup_id_data {
	uint8_t id;
	const struct bt_audio_codec_cap *codec_cap;
};

static sys_slist_t sink_cbs = SYS_SLIST_STATIC_INIT(&sink_cbs);

static void broadcast_sink_cleanup(struct bt_bap_broadcast_sink *sink);

static bool find_recv_state_by_sink_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
				       void *user_data)
{
	const struct bt_bap_broadcast_sink *sink = user_data;

	if (atomic_test_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID) &&
	    sink->bass_src_id == recv_state->src_id) {
		return true;
	}

	return false;
}

static bool find_recv_state_by_pa_sync_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
					  void *user_data)
{
	struct bt_le_per_adv_sync *sync = user_data;
	struct bt_le_per_adv_sync_info sync_info;
	int err;

	err = bt_le_per_adv_sync_get_info(sync, &sync_info);
	if (err != 0) {
		LOG_DBG("Failed to get sync info: %d", err);

		return false;
	}

	if (bt_addr_le_eq(&recv_state->addr, &sync_info.addr) &&
	    recv_state->adv_sid == sync_info.sid) {
		return true;
	}

	return false;
};

static void update_recv_state_big_synced(const struct bt_bap_broadcast_sink *sink)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = {0};
	int err;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	mod_src_param.num_subgroups = sink->subgroup_count;
	for (uint8_t i = 0U; i < sink->subgroup_count; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup_param = &mod_src_param.subgroups[i];
		const struct bt_bap_broadcast_sink_subgroup *sink_subgroup = &sink->subgroups[i];

		/* Set the bis_sync value to the indexes available per subgroup */
		subgroup_param->bis_sync = sink_subgroup->bis_indexes & sink->indexes_bitfield;
	}

	if (recv_state->encrypt_state == BT_BAP_BIG_ENC_STATE_BCODE_REQ) {
		mod_src_param.encrypt_state = BT_BAP_BIG_ENC_STATE_DEC;
	} else {
		mod_src_param.encrypt_state = recv_state->encrypt_state;
	}

	/* Since the mod_src_param struct is 0-initialized the metadata won't
	 * be modified by this
	 */

	/* Copy existing unchanged data */
	mod_src_param.src_id = recv_state->src_id;
	mod_src_param.broadcast_id = recv_state->broadcast_id;

	err = bt_bap_scan_delegator_mod_src(&mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d", sink, err);
	}
}

static void update_recv_state_big_cleared(const struct bt_bap_broadcast_sink *sink,
					  uint8_t reason)
{
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = { 0 };
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	int err;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		/* This is likely due to the receive state being removed while we are BIG synced */
		LOG_DBG("Could not find receive state for sink %p", sink);

		return;
	}

	if ((recv_state->encrypt_state == BT_BAP_BIG_ENC_STATE_BCODE_REQ ||
	     recv_state->encrypt_state == BT_BAP_BIG_ENC_STATE_DEC) &&
	    reason == BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL) {
		/* Sync failed due to bad broadcast code */
		mod_src_param.encrypt_state = BT_BAP_BIG_ENC_STATE_BAD_CODE;
	} else {
		mod_src_param.encrypt_state = recv_state->encrypt_state;
	}

	/* BIS syncs will be automatically cleared since the mod_src_param
	 * struct is 0-initialized
	 *
	 * Since the metadata_len is also 0, then the metadata won't be
	 * modified by the operation either.
	 */

	/* Copy existing unchanged data */
	mod_src_param.num_subgroups = recv_state->num_subgroups;
	mod_src_param.src_id = recv_state->src_id;
	mod_src_param.broadcast_id = recv_state->broadcast_id;

	err = bt_bap_scan_delegator_mod_src(&mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d",
			sink, err);
	}
}

static void broadcast_sink_clear_big(struct bt_bap_broadcast_sink *sink,
				     uint8_t reason)
{
	sink->big = NULL;

	update_recv_state_big_cleared(sink, reason);
}

static struct bt_bap_broadcast_sink *broadcast_sink_lookup_iso_chan(
	const struct bt_iso_chan *chan)
{
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sinks); i++) {
		for (uint8_t j = 0U; j < broadcast_sinks[i].stream_count; j++) {
			if (broadcast_sinks[i].bis[j] == chan) {
				return &broadcast_sinks[i];
			}
		}
	}

	return NULL;
}

static void broadcast_sink_set_ep_state(struct bt_bap_ep *ep, uint8_t state)
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
		if (state != BT_BAP_EP_STATE_IDLE && state != BT_BAP_EP_STATE_STREAMING) {
			LOG_DBG("Invalid broadcast sync endpoint state transition");
			return;
		}
		break;
	case BT_BAP_EP_STATE_STREAMING:
		if (state != BT_BAP_EP_STATE_IDLE) {
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

	if (state == BT_BAP_EP_STATE_IDLE) {
		struct bt_bap_stream *stream = ep->stream;

		if (stream != NULL) {
			bt_bap_iso_unbind_ep(ep->iso, ep);
			stream->ep = NULL;
			stream->codec_cfg = NULL;
			ep->stream = NULL;
		}
	}
}

static void broadcast_sink_iso_recv(struct bt_iso_chan *chan,
				    const struct bt_iso_recv_info *info,
				    struct net_buf *buf)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->rx.ep;

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
		LOG_DBG("stream %p ep %p len %zu", stream, stream->ep, net_buf_frags_len(buf));
	}

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(stream, info, buf);
	} else {
		LOG_WRN("No callback for recv set");
	}
}

static void broadcast_sink_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_broadcast_sink *sink;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->rx.ep;
	bool all_connected;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p", stream);

	ops = stream->ops;
	if (ops != NULL && ops->connected != NULL) {
		ops->connected(stream);
	}

	sink = broadcast_sink_lookup_iso_chan(chan);
	if (sink == NULL) {
		LOG_ERR("Could not lookup sink by iso %p", chan);
		return;
	}

	broadcast_sink_set_ep_state(ep, BT_BAP_EP_STATE_STREAMING);

	if (ops != NULL && ops->started != NULL) {
		ops->started(stream);
	} else {
		LOG_WRN("No callback for started set");
	}

	all_connected = true;
	SYS_SLIST_FOR_EACH_CONTAINER(&sink->streams, stream, _node) {
		__ASSERT(stream->ep, "Endpoint is NULL");

		if (stream->ep->status.state != BT_BAP_EP_STATE_STREAMING) {
			all_connected = false;
			break;
		}
	}

	if (all_connected) {
		update_recv_state_big_synced(sink);
	}
}

static void broadcast_sink_iso_disconnected(struct bt_iso_chan *chan,
					    uint8_t reason)
{
	struct bt_bap_iso *iso = CONTAINER_OF(chan, struct bt_bap_iso, chan);
	const struct bt_bap_stream_ops *ops;
	struct bt_bap_stream *stream;
	struct bt_bap_ep *ep = iso->rx.ep;
	struct bt_bap_broadcast_sink *sink;

	if (ep == NULL) {
		LOG_ERR("iso %p not bound with ep", chan);
		return;
	}

	stream = ep->stream;
	if (stream == NULL) {
		LOG_ERR("No stream for ep %p", ep);
		return;
	}

	LOG_DBG("stream %p ep %p reason 0x%02x", stream, ep, reason);

	ops = stream->ops;
	if (ops != NULL && ops->disconnected != NULL) {
		ops->disconnected(stream, reason);
	}

	broadcast_sink_set_ep_state(ep, BT_BAP_EP_STATE_IDLE);

	sink = broadcast_sink_lookup_iso_chan(chan);
	if (sink == NULL) {
		LOG_ERR("Could not lookup sink by iso %p", chan);
	} else {
		if (!sys_slist_find_and_remove(&sink->streams, &stream->_node)) {
			LOG_DBG("Could not find and remove stream %p from sink %p", stream, sink);
		}

		/* Clear sink->big if not already cleared */
		if (sys_slist_is_empty(&sink->streams) && sink->big) {
			broadcast_sink_clear_big(sink, reason);
		}
	}

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream, reason);
	} else {
		LOG_WRN("No callback for stopped set");
	}
}

static struct bt_iso_chan_ops broadcast_sink_iso_ops = {
	.recv		= broadcast_sink_iso_recv,
	.connected	= broadcast_sink_iso_connected,
	.disconnected	= broadcast_sink_iso_disconnected,
};

static struct bt_bap_broadcast_sink *broadcast_sink_free_get(void)
{
	/* Find free entry */
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (!atomic_test_bit(broadcast_sinks[i].flags,
				     BT_BAP_BROADCAST_SINK_FLAG_INITIALIZED)) {
			broadcast_sinks[i].index = i;
			broadcast_sinks[i].broadcast_id = INVALID_BROADCAST_ID;

			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static struct bt_bap_broadcast_sink *broadcast_sink_get_by_pa(struct bt_le_per_adv_sync *sync)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (broadcast_sinks[i].pa_sync == sync) {
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static void broadcast_sink_add_src(struct bt_bap_broadcast_sink *sink)
{
	struct bt_bap_scan_delegator_add_src_param add_src_param;
	int err;

	add_src_param.pa_sync = sink->pa_sync;
	add_src_param.broadcast_id = sink->broadcast_id;
	/* Will be updated when we receive the BASE */
	add_src_param.encrypt_state = BT_BAP_BIG_ENC_STATE_NO_ENC;
	add_src_param.num_subgroups = 0U;

	err = bt_bap_scan_delegator_add_src(&add_src_param);
	if (err < 0) {
		LOG_WRN("Failed to add sync as Receive State for sink %p: %d",
			sink, err);
	} else {
		sink->bass_src_id = (uint8_t)err;
		atomic_set_bit(sink->flags,
			       BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID);
	}
}

static bool base_subgroup_meta_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct bt_bap_scan_delegator_mod_src_param *mod_src_param = user_data;
	struct bt_bap_scan_delegator_subgroup *subgroup_param;
	uint8_t *meta;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &meta);
	if (ret < 0) {
		return false;
	}

	subgroup_param = &mod_src_param->subgroups[mod_src_param->num_subgroups++];
	subgroup_param->metadata_len = (uint8_t)ret;
	memcpy(subgroup_param->metadata, meta, subgroup_param->metadata_len);

	return true;
}

static int update_recv_state_base_copy_meta(const struct bt_bap_base *base,
					    struct bt_bap_scan_delegator_mod_src_param *param)
{
	int err;

	err = bt_bap_base_foreach_subgroup(base, base_subgroup_meta_cb, param);
	if (err != 0) {
		LOG_DBG("Failed to parse subgroups: %d", err);
		return err;
	}

	return 0;
}

static void update_recv_state_base(const struct bt_bap_broadcast_sink *sink,
				   const struct bt_bap_base *base)
{
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = { 0 };
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	int err;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	err = update_recv_state_base_copy_meta(base, &mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d", sink, err);
		return;
	}

	/* Copy existing unchanged data */
	mod_src_param.src_id = recv_state->src_id;
	mod_src_param.encrypt_state = recv_state->encrypt_state;
	mod_src_param.broadcast_id = recv_state->broadcast_id;

	err = bt_bap_scan_delegator_mod_src(&mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d", sink, err);
	}
}

static bool codec_lookup_id(const struct bt_pacs_cap *cap, void *user_data)
{
	struct codec_cap_lookup_id_data *data = user_data;

	if (cap->codec_cap->id == data->id) {
		data->codec_cap = cap->codec_cap;

		return false;
	}

	return true;
}

static bool base_subgroup_bis_index_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	uint32_t *bis_indexes = user_data;

	*bis_indexes |= BIT(bis->index);

	return true;
}

static bool base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct bt_bap_broadcast_sink *sink = user_data;
	struct bt_bap_broadcast_sink_subgroup *sink_subgroup =
		&sink->subgroups[sink->subgroup_count];
	struct codec_cap_lookup_id_data lookup_data = {0};
	int ret;

	if (sink->subgroup_count == ARRAY_SIZE(sink->subgroups)) {
		/* We've parsed as many subgroups as we support */
		LOG_DBG("Could only store %u subgroups", sink->subgroup_count);
		return false;
	}

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &sink_subgroup->codec_cfg);
	if (ret < 0) {
		LOG_DBG("Could not store codec_cfg: %d", ret);
		return false;
	}

	ret = bt_bap_base_subgroup_foreach_bis(subgroup, base_subgroup_bis_index_cb,
					       &sink_subgroup->bis_indexes);
	if (ret < 0) {
		LOG_DBG("Could not parse BISes: %d", ret);
		return false;
	}

	/* Lookup and assign path_id based on capabilities */
	lookup_data.id = sink_subgroup->codec_cfg.id;

	bt_pacs_cap_foreach(BT_AUDIO_DIR_SINK, codec_lookup_id, &lookup_data);
	if (lookup_data.codec_cap == NULL) {
		LOG_DBG("Codec with id %u is not supported by our capabilities", lookup_data.id);
	} else {
		/* Add BIS to bitfield of valid BIS indexes we support */
		sink->valid_indexes_bitfield |= sink_subgroup->bis_indexes;
	}

	sink->subgroup_count++;

	return true;
}

static int store_base_info(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base)
{
	int ret;

	sink->valid_indexes_bitfield = 0U;
	sink->subgroup_count = 0U;

	ret = bt_bap_base_get_pres_delay(base);
	if (ret < 0) {
		LOG_DBG("Could not get presentation delay: %d", ret);
		return ret;
	}

	sink->codec_qos.pd = (uint32_t)ret;

	ret = bt_bap_base_foreach_subgroup(base, base_subgroup_cb, sink);
	if (ret != 0) {
		LOG_DBG("Failed to parse all subgroups: %d", ret);
		return ret;
	}

	return 0;
}

static bool base_subgroup_bis_count_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	uint8_t *bis_cnt = user_data;
	int ret;

	ret = bt_bap_base_get_subgroup_bis_count(subgroup);
	if (ret < 0) {
		return false;
	}

	*bis_cnt += (uint8_t)ret;

	return true;
}

static int base_get_bis_count(const struct bt_bap_base *base)
{
	uint8_t bis_cnt = 0U;
	int err;

	err = bt_bap_base_foreach_subgroup(base, base_subgroup_bis_count_cb, &bis_cnt);
	if (err != 0) {
		LOG_DBG("Failed to parse subgroups: %d", err);
		return err;
	}

	return bis_cnt;
}

static bool pa_decode_base(struct bt_data *data, void *user_data)
{
	struct bt_bap_broadcast_sink *sink = (struct bt_bap_broadcast_sink *)user_data;
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(data);
	struct bt_bap_broadcast_sink_cb *listener;
	size_t base_size;
	int ret;

	/* Base is NULL if the data does not contain a valid BASE */
	if (base == NULL) {
		return true;
	}

	if (atomic_test_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_BIGINFO_RECEIVED)) {
		ret = base_get_bis_count(base);

		if (ret < 0) {
			LOG_DBG("Invalid BASE: %d", ret);
			return false;
		} else if (ret != sink->biginfo_num_bis) {
			LOG_DBG("BASE contains different amount of BIS (%u) than reported by "
				"BIGInfo (%u)",
				ret, sink->biginfo_num_bis);
			return false;
		}
	}

	/* Store newest BASE info until we are BIG synced */
	if (sink->big == NULL) {
		LOG_DBG("Updating BASE for sink %p with %d subgroups\n", sink,
			bt_bap_base_get_subgroup_count(base));

		ret = store_base_info(sink, base);
		if (ret < 0) {
			LOG_DBG("Could not store BASE information: %d", ret);

			/* If it returns -ECANCELED it means that we stopped parsing ourselves due
			 * to lack of memory. In this case we can still provide the BASE to the
			 * application else abort
			 */
			if (ret != -ECANCELED) {
				return false;
			}
		}
	}

	if (atomic_test_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID)) {
		update_recv_state_base(sink, base);
	}

	/* We provide the BASE without the service data UUID */
	base_size = data->data_len - BT_UUID_SIZE_16;

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->base_recv != NULL) {
			listener->base_recv(sink, base, base_size);
		}
	}

	return false;
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	struct bt_bap_broadcast_sink *sink = broadcast_sink_get_by_pa(sync);

	if (sink == NULL) {
		/* Not a PA sync that we control */
		return;
	}

	if (sys_slist_is_empty(&sink_cbs)) {
		/* Terminate early if we do not have any broadcast sink listeners */
		return;
	}

	bt_data_parse(buf, pa_decode_base, (void *)sink);
}

static void pa_term_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_term_info *info)
{
	struct bt_bap_broadcast_sink *sink = broadcast_sink_get_by_pa(sync);

	if (sink != NULL) {
		sink->pa_sync = NULL;
	}
}

static void update_recv_state_encryption(const struct bt_bap_broadcast_sink *sink)
{
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = { 0 };
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	int err;

	__ASSERT(sink->big == NULL, "Encryption state shall not be updated while synced");

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	/* Only change the encrypt state, and leave the rest as is */
	if (atomic_test_bit(sink->flags,
			    BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED)) {
		mod_src_param.encrypt_state = BT_BAP_BIG_ENC_STATE_BCODE_REQ;
	} else {
		mod_src_param.encrypt_state = BT_BAP_BIG_ENC_STATE_NO_ENC;
	}

	if (mod_src_param.encrypt_state == recv_state->encrypt_state) {
		/* No change, abort*/
		return;
	}

	/* Copy existing data */
	/* TODO: Maybe we need more refined functions to set only specific fields? */
	mod_src_param.src_id = recv_state->src_id;
	mod_src_param.broadcast_id = recv_state->broadcast_id;
	mod_src_param.num_subgroups = recv_state->num_subgroups;
	(void)memcpy(mod_src_param.subgroups,
		     recv_state->subgroups,
		     sizeof(recv_state->num_subgroups));

	err = bt_bap_scan_delegator_mod_src(&mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d", sink, err);
	}
}

static void biginfo_recv(struct bt_le_per_adv_sync *sync,
			 const struct bt_iso_biginfo *biginfo)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;

	sink = broadcast_sink_get_by_pa(sync);
	if (sink == NULL) {
		/* Not ours */
		return;
	}

	if (sink->big != NULL) {
		/* Already synced - ignore */
		return;
	}

	atomic_set_bit(sink->flags,
		       BT_BAP_BROADCAST_SINK_FLAG_BIGINFO_RECEIVED);
	sink->iso_interval = biginfo->iso_interval;
	sink->biginfo_num_bis = biginfo->num_bis;
	if (biginfo->encryption != atomic_test_bit(sink->flags,
						   BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED)) {
		atomic_set_bit_to(sink->flags,
				  BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED,
				  biginfo->encryption);

		if (atomic_test_bit(sink->flags,
				    BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID)) {
			update_recv_state_encryption(sink);
		}
	}

	sink->codec_qos.framing = biginfo->framing;
	sink->codec_qos.phy = biginfo->phy;
	sink->codec_qos.sdu = biginfo->max_sdu;
	sink->codec_qos.interval = biginfo->sdu_interval;

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->syncable != NULL) {
			listener->syncable(sink, biginfo->encryption);
		}
	}
}

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_ms;
	uint16_t timeout;

	/* Ensure that the following calculation does not overflow silently */
	__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

	/* Add retries and convert to unit in 10's of ms */
	interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(interval);
	timeout = (interval_ms * SYNC_RETRY_COUNT) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
			BT_GAP_PER_ADV_MAX_TIMEOUT);

	return timeout;
}

int bt_bap_broadcast_sink_register_cb(struct bt_bap_broadcast_sink_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	sys_slist_append(&sink_cbs, &cb->_node);

	return 0;
}

bool bt_bap_ep_is_broadcast_snk(const struct bt_bap_ep *ep)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sink_eps); i++) {
		if (PART_OF_ARRAY(broadcast_sink_eps[i], ep)) {
			return true;
		}
	}

	return false;
}

static void broadcast_sink_ep_init(struct bt_bap_ep *ep)
{
	LOG_DBG("ep %p", ep);

	(void)memset(ep, 0, sizeof(*ep));
	ep->dir = BT_AUDIO_DIR_SINK;
	ep->iso = NULL;
}

static struct bt_bap_ep *broadcast_sink_new_ep(uint8_t index)
{
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_sink_eps[index]); i++) {
		struct bt_bap_ep *ep = &broadcast_sink_eps[index][i];

		/* If ep->stream is NULL the endpoint is unallocated */
		if (ep->stream == NULL) {
			broadcast_sink_ep_init(ep);
			return ep;
		}
	}

	return NULL;
}

static int bt_bap_broadcast_sink_setup_stream(struct bt_bap_broadcast_sink *sink,
					      struct bt_bap_stream *stream,
					      struct bt_audio_codec_cfg *codec_cfg)
{
	struct bt_bap_iso *iso;
	struct bt_bap_ep *ep;

	if (stream->group != NULL) {
		LOG_DBG("Stream %p already in group %p", stream, stream->group);
		return -EALREADY;
	}

	ep = broadcast_sink_new_ep(sink->index);
	if (ep == NULL) {
		LOG_DBG("Could not allocate new broadcast endpoint");
		return -ENOMEM;
	}

	iso = bt_bap_iso_new();
	if (iso == NULL) {
		LOG_DBG("Could not allocate iso");
		return -ENOMEM;
	}

	bt_bap_iso_init(iso, &broadcast_sink_iso_ops);
	bt_bap_iso_bind_ep(iso, ep);

	bt_audio_codec_qos_to_iso_qos(iso->chan.qos->rx, &sink->codec_qos);
	bt_audio_codec_cfg_to_iso_path(iso->chan.qos->rx->path, codec_cfg);

	bt_bap_iso_unref(iso);

	bt_bap_stream_attach(NULL, stream, ep, codec_cfg);
	stream->qos = &sink->codec_qos;

	return 0;
}

static void broadcast_sink_cleanup_streams(struct bt_bap_broadcast_sink *sink)
{
	struct bt_bap_stream *stream, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sink->streams, stream, next, _node) {
		if (stream->ep != NULL) {
			bt_bap_iso_unbind_ep(stream->ep->iso, stream->ep);
			stream->ep->stream = NULL;
			stream->ep = NULL;
		}

		stream->qos = NULL;
		stream->codec_cfg = NULL;
		stream->group = NULL;

		sys_slist_remove(&sink->streams, NULL, &stream->_node);
	}

	sink->stream_count = 0;
	sink->indexes_bitfield = 0U;
}

static void broadcast_sink_cleanup(struct bt_bap_broadcast_sink *sink)
{
	if (atomic_test_bit(sink->flags,
			    BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID)) {
		int err;

		err = bt_bap_scan_delegator_rem_src(sink->bass_src_id);
		if (err != 0) {
			/* This is likely due to the receive state been removed */
			LOG_DBG("Could not remove Receive State for sink %p: %d", sink, err);
		}
	}

	if (sink->stream_count > 0U) {
		broadcast_sink_cleanup_streams(sink);
	}

	(void)memset(sink, 0, sizeof(*sink)); /* also clears flags */
}
static struct bt_audio_codec_cfg *codec_cfg_from_base_by_index(struct bt_bap_broadcast_sink *sink,
							       uint8_t index)
{
	for (size_t i = 0U; i < sink->subgroup_count; i++) {
		struct bt_bap_broadcast_sink_subgroup *subgroup = &sink->subgroups[i];

		if ((subgroup->bis_indexes & BIT(index)) != 0) {
			return &subgroup->codec_cfg;
		}
	}

	return NULL;
}

int bt_bap_broadcast_sink_create(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id,
				 struct bt_bap_broadcast_sink **out_sink)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_bap_broadcast_sink *sink;

	CHECKIF(pa_sync == NULL) {
		LOG_DBG("pa_sync is NULL");

		return -EINVAL;
	}

	CHECKIF(broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: 0x%X", broadcast_id);

		return -EINVAL;
	}

	CHECKIF(out_sink == NULL) {
		LOG_DBG("sink was NULL");

		return -EINVAL;
	}

	sink = broadcast_sink_free_get();
	if (sink == NULL) {
		LOG_DBG("No more free broadcast sinks");

		return -ENOMEM;
	}

	sink->broadcast_id = broadcast_id;
	sink->pa_sync = pa_sync;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_pa_sync_cb,
						      (void *)pa_sync);
	if (recv_state == NULL) {
		broadcast_sink_add_src(sink);
	} else {
		/* The PA sync is known by the Scan Delegator */
		if (recv_state->broadcast_id != broadcast_id) {
			LOG_DBG("Broadcast ID mismatch: 0x%X != 0x%X",
				recv_state->broadcast_id, broadcast_id);

			broadcast_sink_cleanup(sink);
			return -EINVAL;
		}

		sink->bass_src_id = recv_state->src_id;
		atomic_set_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID);
	}
	atomic_set_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_INITIALIZED);

	*out_sink = sink;
	return 0;
}

int bt_bap_broadcast_sink_sync(struct bt_bap_broadcast_sink *sink, uint32_t indexes_bitfield,
			       struct bt_bap_stream *streams[], const uint8_t broadcast_code[16])
{
	struct bt_iso_big_sync_param param;
	struct bt_audio_codec_cfg *codec_cfgs[CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT] = {NULL};
	uint8_t stream_count;
	int err;

	CHECKIF(sink == NULL) {
		LOG_DBG("sink is NULL");
		return -EINVAL;
	}

	CHECKIF(indexes_bitfield == 0) {
		LOG_DBG("indexes_bitfield is 0");
		return -EINVAL;
	}

	CHECKIF(indexes_bitfield & BIT(0)) {
		LOG_DBG("BIT(0) is not a valid BIS index");
		return -EINVAL;
	}

	CHECKIF(streams == NULL) {
		LOG_DBG("streams is NULL");
		return -EINVAL;
	}

	if (sink->pa_sync == NULL) {
		LOG_DBG("Sink is not PA synced");
		return -EINVAL;
	}

	if (!atomic_test_bit(sink->flags,
			     BT_BAP_BROADCAST_SINK_FLAG_BIGINFO_RECEIVED)) {
		/* TODO: We could store the request to sync and start the sync
		 * once the BIGInfo has been received, and then do the sync
		 * then. This would be similar how LE Create Connection works.
		 */
		LOG_DBG("BIGInfo not received, cannot sync yet");
		return -EAGAIN;
	}

	if (atomic_test_bit(sink->flags,
			    BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED) &&
	    broadcast_code == NULL) {
		LOG_DBG("Broadcast code required");

		return -EINVAL;
	}

	/* Validate that number of bits set is less than number of streams */
	if ((indexes_bitfield & sink->valid_indexes_bitfield) != indexes_bitfield) {
		LOG_DBG("Request BIS indexes 0x%08X contains bits not support by the Broadcast "
			"Sink 0x%08X",
			indexes_bitfield, sink->valid_indexes_bitfield);
		return -EINVAL;
	}

	stream_count = 0;
	for (int i = 1; i < BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
		if ((indexes_bitfield & BIT(i)) != 0) {
			struct bt_audio_codec_cfg *codec_cfg =
				codec_cfg_from_base_by_index(sink, i);

			__ASSERT(codec_cfg != NULL, "Index %d not found in sink", i);

			codec_cfgs[stream_count++] = codec_cfg;

			if (stream_count > CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT) {
				LOG_DBG("Cannot sync to more than %d streams (%u was requested)",
					CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT, stream_count);
				return -EINVAL;
			}
		}
	}

	for (size_t i = 0; i < stream_count; i++) {
		CHECKIF(streams[i] == NULL) {
			LOG_DBG("streams[%zu] is NULL", i);
			return -EINVAL;
		}
	}

	sink->stream_count = 0U;
	for (size_t i = 0; i < stream_count; i++) {
		struct bt_bap_stream *stream;
		struct bt_audio_codec_cfg *codec_cfg;

		stream = streams[i];
		codec_cfg = codec_cfgs[i];

		err = bt_bap_broadcast_sink_setup_stream(sink, stream, codec_cfg);
		if (err != 0) {
			LOG_DBG("Failed to setup streams[%zu]: %d", i, err);
			broadcast_sink_cleanup_streams(sink);
			return err;
		}

		sink->bis[i] = bt_bap_stream_iso_chan_get(stream);
		sys_slist_append(&sink->streams, &stream->_node);
		sink->stream_count++;
	}

	param.bis_channels = sink->bis;
	param.num_bis = sink->stream_count;
	param.bis_bitfield = indexes_bitfield;
	param.mse = 0; /* Let controller decide */
	param.sync_timeout = interval_to_sync_timeout(sink->iso_interval);
	param.encryption = atomic_test_bit(sink->flags,
					   BT_BAP_BROADCAST_SINK_FLAG_BIG_ENCRYPTED);
	if (param.encryption) {
		memcpy(param.bcode, broadcast_code, sizeof(param.bcode));
	} else {
		memset(param.bcode, 0, sizeof(param.bcode));
	}

	err = bt_iso_big_sync(sink->pa_sync, &param, &sink->big);
	if (err != 0) {
		broadcast_sink_cleanup_streams(sink);
		return err;
	}

	sink->indexes_bitfield = indexes_bitfield;
	for (size_t i = 0; i < stream_count; i++) {
		struct bt_bap_ep *ep = streams[i]->ep;

		ep->broadcast_sink = sink;
		broadcast_sink_set_ep_state(ep, BT_BAP_EP_STATE_QOS_CONFIGURED);
	}

	return 0;
}

int bt_bap_broadcast_sink_stop(struct bt_bap_broadcast_sink *sink)
{
	struct bt_bap_stream *stream;
	sys_snode_t *head_node;
	int err;

	CHECKIF(sink == NULL) {
		LOG_DBG("sink is NULL");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&sink->streams)) {
		LOG_DBG("Source does not have any streams (already stopped)");
		return -EALREADY;
	}

	head_node = sys_slist_peek_head(&sink->streams);
	stream = CONTAINER_OF(head_node, struct bt_bap_stream, _node);

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (stream->ep == NULL) {
		LOG_DBG("stream->ep is NULL");
		return -EINVAL;
	}

	if (stream->ep->status.state != BT_BAP_EP_STATE_STREAMING &&
	    stream->ep->status.state != BT_BAP_EP_STATE_QOS_CONFIGURED) {
		LOG_DBG("Broadcast sink stream %p invalid state: %u", stream,
			stream->ep->status.state);
		return -EBADMSG;
	}

	err = bt_iso_big_terminate(sink->big);
	if (err) {
		LOG_DBG("Failed to terminate BIG (err %d)", err);
		return err;
	}

	broadcast_sink_clear_big(sink, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	/* Channel states will be updated in the broadcast_sink_iso_disconnected function */

	return 0;
}

int bt_bap_broadcast_sink_delete(struct bt_bap_broadcast_sink *sink)
{
	CHECKIF(sink == NULL) {
		LOG_DBG("sink is NULL");
		return -EINVAL;
	}

	if (!sys_slist_is_empty(&sink->streams)) {
		struct bt_bap_stream *stream;
		sys_snode_t *head_node;

		head_node = sys_slist_peek_head(&sink->streams);
		stream = CONTAINER_OF(head_node, struct bt_bap_stream, _node);

		/* All streams in a broadcast source is in the same state,
		 * so we can just check the first stream
		 */
		if (stream->ep != NULL) {
			LOG_DBG("Sink is not stopped");
			return -EBADMSG;
		}
	}

	/* Reset the broadcast sink */
	broadcast_sink_cleanup(sink);

	return 0;
}

static int broadcast_sink_init(void)
{
	static struct bt_le_per_adv_sync_cb cb = {
		.recv = pa_recv,
		.biginfo = biginfo_recv,
		.term = pa_term_cb,
	};

	bt_le_per_adv_sync_cb_register(&cb);

	return 0;
}

SYS_INIT(broadcast_sink_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
