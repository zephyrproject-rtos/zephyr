/*  Bluetooth Audio Broadcast Sink */

/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
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

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_broadcast_sink, CONFIG_BT_BAP_BROADCAST_SINK_LOG_LEVEL);

#include "common/bt_str.h"

#define PA_SYNC_SKIP              5
#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define BASE_MIN_SIZE             17
#define BASE_BIS_DATA_MIN_SIZE    2 /* index and length */
#define BROADCAST_SYNC_MIN_INDEX  (BIT(1))

/* any value above 0xFFFFFF is invalid, so we can just use 0xFFFFFFFF to denote
 * invalid broadcast ID
 */
#define INVALID_BROADCAST_ID 0xFFFFFFFF

static struct bt_bap_ep broadcast_sink_eps[CONFIG_BT_BAP_BROADCAST_SNK_COUNT]
					    [BROADCAST_SNK_STREAM_CNT];
static struct bt_bap_broadcast_sink broadcast_sinks[CONFIG_BT_BAP_BROADCAST_SNK_COUNT];
static struct bt_le_scan_cb broadcast_scan_cb;

struct codec_lookup_id_data {
	uint8_t id;
	struct bt_codec *codec;
};

static sys_slist_t sink_cbs = SYS_SLIST_STATIC_INIT(&sink_cbs);

static void broadcast_sink_cleanup(struct bt_bap_broadcast_sink *sink);

static enum bt_bap_scan_delegator_iter
find_recv_state_by_sink_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
			   void *user_data)
{
	const struct bt_bap_broadcast_sink *sink = user_data;

	if (atomic_test_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID) &&
	    sink->bass_src_id == recv_state->src_id) {
		return BT_BAP_SCAN_DELEGATOR_ITER_STOP;
	}

	return BT_BAP_SCAN_DELEGATOR_ITER_CONTINUE;
}

static enum bt_bap_scan_delegator_iter
find_recv_state_by_pa_sync_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
			      void *user_data)
{
	struct bt_le_per_adv_sync *sync = user_data;
	struct bt_le_per_adv_sync_info sync_info;
	int err;

	err = bt_le_per_adv_sync_get_info(sync, &sync_info);
	if (err != 0) {
		LOG_DBG("Failed to get sync info: %d", err);

		return BT_BAP_SCAN_DELEGATOR_ITER_CONTINUE;
	}

	if (bt_addr_le_eq(&recv_state->addr, &sync_info.addr) &&
	    recv_state->adv_sid == sync_info.sid) {
		return BT_BAP_SCAN_DELEGATOR_ITER_STOP;
	}

	return BT_BAP_SCAN_DELEGATOR_ITER_CONTINUE;
};

static void update_recv_state_big_synced(const struct bt_bap_broadcast_sink *sink)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = { 0 };
	const struct bt_bap_base *base;
	int err;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	base = &sink->base;

	mod_src_param.num_subgroups = base->subgroup_count;
	for (uint8_t i = 0U; i < base->subgroup_count; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup_param = &mod_src_param.subgroups[i];
		const struct bt_bap_base_subgroup *subgroup = &base->subgroups[i];

		/* Update the BIS sync indexes for the subgroup */
		for (size_t j = 0U; j < subgroup->bis_count; j++) {
			const struct bt_bap_base_bis_data *bis_data = &subgroup->bis_data[j];

			subgroup_param->bis_sync |= BIT(bis_data->index);
		}
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
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	if (recv_state->encrypt_state == BT_BAP_BIG_ENC_STATE_BCODE_REQ &&
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
			stream->codec = NULL;
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

	ops = stream->ops;

	LOG_DBG("stream %p", stream);

	sink = broadcast_sink_lookup_iso_chan(chan);
	if (sink == NULL) {
		LOG_ERR("Could not lookup sink by iso %p", chan);
		return;
	}

	broadcast_sink_set_ep_state(ep, BT_BAP_EP_STATE_STREAMING);

	if (ops != NULL && ops->started != NULL) {
		ops->started(stream);
	} else {
		LOG_WRN("No callback for connected set");
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
	bool all_disconnected;

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

	LOG_DBG("stream %p ep %p reason 0x%02x", stream, ep, reason);

	broadcast_sink_set_ep_state(ep, BT_BAP_EP_STATE_IDLE);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream, reason);
	} else {
		LOG_WRN("No callback for stopped set");
	}

	sink = broadcast_sink_lookup_iso_chan(chan);
	if (sink == NULL) {
		LOG_ERR("Could not lookup sink by iso %p", chan);
		return;
	}

	all_disconnected = true;
	SYS_SLIST_FOR_EACH_CONTAINER(&sink->streams, stream, _node) {
		/* stream->ep is cleared when the steam disconnects */
		if (stream->ep != NULL) {
			all_disconnected = false;
			break;
		}
	}

	/* Clear sink->big if not already cleared */
	if (all_disconnected && sink->big) {
		broadcast_sink_clear_big(sink, reason);
	}
}

static struct bt_iso_chan_ops broadcast_sink_iso_ops = {
	.recv		= broadcast_sink_iso_recv,
	.connected	= broadcast_sink_iso_connected,
	.disconnected	= broadcast_sink_iso_disconnected,
};

static struct bt_bap_broadcast_sink *broadcast_sink_syncing_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (atomic_test_bit(broadcast_sinks[i].flags,
				    BT_BAP_BROADCAST_SINK_FLAG_SYNCING)) {
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

static struct bt_bap_broadcast_sink *broadcast_sink_scanning_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (atomic_test_bit(broadcast_sinks[i].flags,
				    BT_BAP_BROADCAST_SINK_FLAG_SCANNING)) {
			return &broadcast_sinks[i];
		}
	}

	return NULL;
}

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

static struct bt_bap_broadcast_sink *broadcast_sink_get_by_broadcast_id(uint32_t broadcast_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sinks); i++) {
		if (broadcast_sinks[i].broadcast_id == broadcast_id) {
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

static void handle_past_sync(struct bt_le_per_adv_sync *sync)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_pa_sync_cb, (void *)sync);
	if (recv_state != NULL) {
		/* If we receive a PAST transfer that fits a
		 * known BASS Receive State, then we create it
		 * as a Broadcast Sink
		 *
		 * The PA state in the Receive State will be
		 * updated by the Scan Delegator
		 */
		int err;

		err = bt_bap_broadcast_sink_create(sync, recv_state->broadcast_id);
		if (err != 0) {
			LOG_WRN("Failed to create Broadcast Sink: %d", err);
		}
	}
}

static void pa_synced(struct bt_le_per_adv_sync *sync,
		      struct bt_le_per_adv_sync_synced_info *info)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;
	int err;

	sink = broadcast_sink_syncing_get();
	if (sink == NULL || sync != sink->pa_sync) {
		if (info->conn) { /* PAST */
			handle_past_sync(sync);
		} else {
			/* Not ours */
		}

		return;
	}

	LOG_DBG("Synced to broadcast source with ID 0x%06X", sink->broadcast_id);

	atomic_clear_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SYNCING);

	err = bt_bap_broadcast_sink_scan_stop();
	if (err != 0 && err != -EALREADY) {
		LOG_WRN("Failed to stop sink scan: %d", err);
		/* Even if we cannot stop scanning here, we can still move on */
	}

	/* Add the PA sync to the scan delegator or modify if it already exists */
	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_pa_sync_cb, (void *)sync);
	if (recv_state == NULL) {
		broadcast_sink_add_src(sink);
	} else {
		/* Set PA sync state */
		err = bt_bap_scan_delegator_set_pa_state(recv_state->src_id,
							 BT_BAP_PA_STATE_SYNCED);
		if (err != 0) {
			LOG_WRN("Failed to set PA state: %d", err);
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->pa_synced != NULL) {
			listener->pa_synced(sink, sink->pa_sync, sink->broadcast_id);
		}
	}

	/* TBD: What if sync to a bad broadcast source that does not send
	 * properly formatted (or any) BASE?
	 */
}

static void pa_term(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;

	sink = broadcast_sink_get_by_pa(sync);
	if (sink == NULL) {
		/* Not ours */
		return;
	}

	LOG_DBG("PA sync with broadcast source with ID 0x%06X lost", sink->broadcast_id);

	if (sink->big != NULL) {
		const int err = bt_iso_big_terminate(sink->big);

		if (err != 0) {
			LOG_ERR("Failed to disconnect BIG sync: %d", err);
		}
	}

	broadcast_sink_cleanup(sink);
	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->pa_sync_lost != NULL) {
			listener->pa_sync_lost(sink);
		}
	}
}

static void update_recv_state_base_copy_meta(const struct bt_bap_base *base,
					     struct bt_bap_scan_delegator_mod_src_param *param)
{
	for (uint8_t i = 0U; i < base->subgroup_count; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup_param = &param->subgroups[i];
		const struct bt_bap_base_subgroup *subgroup = &base->subgroups[i];
		uint8_t *metadata_param = subgroup_param->metadata;
		size_t total_len;

		/* Copy metadata into subgroup_param, changing it from an array
		 * of bt_codec_data to a uint8_t buffer
		 */
		total_len = 0U;
		for (size_t j = 0; j < subgroup->codec.meta_count; j++) {
			const struct bt_codec_data *meta = &subgroup->codec.meta[j];
			const struct bt_data *data = &meta->data;
			const uint8_t len = data->data_len;
			const uint8_t type = data->type;
			const size_t ltv_len = sizeof(len) + sizeof(type) + len;

			if (total_len + ltv_len > sizeof(subgroup_param->metadata)) {
				LOG_WRN("Could not fit entire metadata for subgroup[%u]", i);

				return;
			}

			metadata_param[total_len++] = len + 1;
			metadata_param[total_len++] = type;
			(void)memcpy(&metadata_param[total_len], data->data,
				     len);
			total_len += len;
		}

		subgroup_param->metadata_len = total_len;
	}
}

static void update_recv_state_base(const struct bt_bap_broadcast_sink *sink)
{
	struct bt_bap_scan_delegator_mod_src_param mod_src_param = { 0 };
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	const struct bt_bap_base *base;
	int err;

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_sink_cb, (void *)sink);
	if (recv_state == NULL) {
		LOG_WRN("Failed to find receive state for sink %p", sink);

		return;
	}

	base = &sink->base;

	mod_src_param.num_subgroups = base->subgroup_count;
	update_recv_state_base_copy_meta(base, &mod_src_param);

	/* Copy existing unchanged data */
	mod_src_param.src_id = recv_state->src_id;
	mod_src_param.encrypt_state = recv_state->encrypt_state;
	mod_src_param.broadcast_id = recv_state->broadcast_id;

	err = bt_bap_scan_delegator_mod_src(&mod_src_param);
	if (err != 0) {
		LOG_WRN("Failed to modify Receive State for sink %p: %d", sink, err);
	}
}

static bool net_buf_decode_codec_ltv(struct net_buf_simple *buf,
				     struct bt_codec_data *codec_data)
{
	void *value;

	if (buf->len < sizeof(codec_data->data.data_len)) {
		LOG_DBG("Not enough data for LTV length field: %u", buf->len);
		return false;
	}
	codec_data->data.data_len = net_buf_simple_pull_u8(buf);

	if (buf->len < sizeof(codec_data->data.type)) {
		LOG_DBG("Not enough data for LTV type field: %u", buf->len);
		return false;
	}

	/* LTV structures include the data.type in the length field,
	 * but we do not do that for the bt_data struct in Zephyr
	 */
	codec_data->data.data_len -= sizeof(codec_data->data.type);

	codec_data->data.type = net_buf_simple_pull_u8(buf);
	codec_data->data.data = codec_data->value;

	if (buf->len < codec_data->data.data_len) {
		LOG_DBG("Not enough data for LTV value field: %u/%zu", buf->len,
			codec_data->data.data_len);
		return false;
	}
	value = net_buf_simple_pull_mem(buf, codec_data->data.data_len);
	(void)memcpy(codec_data->value, value, codec_data->data.data_len);

	return true;
}

static bool net_buf_decode_bis_data(struct net_buf_simple *buf, struct bt_bap_base_bis_data *bis)
{
	uint8_t len;

	if (buf->len < BASE_BIS_DATA_MIN_SIZE) {
		LOG_DBG("Not enough bytes (%u) to decode BIS data", buf->len);
		return false;
	}

	bis->index = net_buf_simple_pull_u8(buf);
	if (!IN_RANGE(bis->index, BT_ISO_BIS_INDEX_MIN, BT_ISO_BIS_INDEX_MAX)) {
		LOG_DBG("Invalid BIS index %u", bis->index);
		return false;
	}

	/* codec config data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid BIS specific codec config data length: "
		       "%u (buf is %u)", len, buf->len);
		return false;
	}

	if (len > 0) {
		struct net_buf_simple ltv_buf;
		void *ltv_data;

		/* Use an extra net_buf_simple to be able to decode until it
		 * is empty (len = 0)
		 */
		ltv_data = net_buf_simple_pull_mem(buf, len);
		net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

		while (ltv_buf.len != 0) {
			struct bt_codec_data *bis_codec_data;

			if (bis->data_count >= ARRAY_SIZE(bis->data)) {
				LOG_WRN("BIS data overflow; discarding");
				break;
			}

			bis_codec_data = &bis->data[bis->data_count];

			if (!net_buf_decode_codec_ltv(&ltv_buf,
						      bis_codec_data)) {
				LOG_DBG("Failed to decode BIS config data for entry %u",
					bis->data_count);
				return false;
			}

			bis->data_count++;
		}
	}

	return true;
}

static bool net_buf_decode_subgroup(struct net_buf_simple *buf,
				    struct bt_bap_base_subgroup *subgroup)
{
	struct net_buf_simple ltv_buf;
	struct bt_codec	*codec;
	void *ltv_data;
	uint8_t len;

	codec = &subgroup->codec;

	subgroup->bis_count = net_buf_simple_pull_u8(buf);
	if (subgroup->bis_count > ARRAY_SIZE(subgroup->bis_data)) {
		LOG_DBG("BASE has more BIS %u than we support %u", subgroup->bis_count,
			(uint8_t)ARRAY_SIZE(subgroup->bis_data));
		return false;
	}
	codec->id = net_buf_simple_pull_u8(buf);
	codec->cid = net_buf_simple_pull_le16(buf);
	codec->vid = net_buf_simple_pull_le16(buf);

	/* codec configuration data length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);
		return false;
	}

	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *codec_data;

		if (codec->data_count >= ARRAY_SIZE(codec->data)) {
			LOG_WRN("BIS codec data overflow; discarding");
			break;
		}

		codec_data = &codec->data[codec->data_count];

		if (!net_buf_decode_codec_ltv(&ltv_buf, codec_data)) {
			LOG_DBG("Failed to decode codec config data for entry %u",
				codec->data_count);
			return false;
		}

		codec->data_count++;
	}

	if (buf->len < sizeof(len)) {
		return false;
	}

	/* codec metadata length */
	len = net_buf_simple_pull_u8(buf);
	if (len > buf->len) {
		LOG_DBG("Invalid codec config data length: %u (buf is %u)", len, buf->len);
		return false;
	}


	/* Use an extra net_buf_simple to be able to decode until it
	 * is empty (len = 0)
	 */
	ltv_data = net_buf_simple_pull_mem(buf, len);
	net_buf_simple_init_with_data(&ltv_buf, ltv_data, len);

	/* The loop below is very similar to codec_config_store with notable
	 * exceptions that it can do early termination, and also does not log
	 * every LTV entry, which would simply be too much for handling
	 * broadcasted BASEs
	 */
	while (ltv_buf.len != 0) {
		struct bt_codec_data *metadata;

		if (codec->meta_count >= ARRAY_SIZE(codec->meta)) {
			LOG_WRN("BIS codec metadata overflow; discarding");
			break;
		}

		metadata = &codec->meta[codec->meta_count];

		if (!net_buf_decode_codec_ltv(&ltv_buf, metadata)) {
			LOG_DBG("Failed to decode codec metadata for entry %u",
				codec->meta_count);
			return false;
		}

		codec->meta_count++;
	}

	for (int i = 0; i < subgroup->bis_count; i++) {
		if (!net_buf_decode_bis_data(buf, &subgroup->bis_data[i])) {
			LOG_DBG("Failed to decode BIS data for bis %d", i);
			return false;
		}
	}

	return true;
}

static bool pa_decode_base(struct bt_data *data, void *user_data)
{
	struct bt_bap_broadcast_sink *sink = (struct bt_bap_broadcast_sink *)user_data;
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_base base = {0};
	struct bt_uuid_16 broadcast_uuid;
	struct net_buf_simple net_buf;
	void *uuid;

	if (sys_slist_is_empty(&sink_cbs)) {
		/* Terminate early if we do not have any broadcast sink listeners */
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BASE_MIN_SIZE) {
		return true;
	}

	net_buf_simple_init_with_data(&net_buf, (void *)data->data,
				      data->data_len);

	uuid = net_buf_simple_pull_mem(&net_buf, BT_UUID_SIZE_16);

	if (!bt_uuid_create(&broadcast_uuid.uuid, uuid, BT_UUID_SIZE_16)) {
		LOG_ERR("bt_uuid_create failed");
		return false;
	}

	if (bt_uuid_cmp(&broadcast_uuid.uuid, BT_UUID_BASIC_AUDIO) != 0) {
		/* Continue parsing */
		return true;
	}

	base.pd = net_buf_simple_pull_le24(&net_buf);
	base.subgroup_count = net_buf_simple_pull_u8(&net_buf);

	if (base.subgroup_count > ARRAY_SIZE(base.subgroups)) {
		LOG_DBG("Cannot decode BASE with %u subgroups (max supported is %zu)",
			base.subgroup_count, ARRAY_SIZE(base.subgroups));

		return false;
	}

	for (int i = 0; i < base.subgroup_count; i++) {
		if (!net_buf_decode_subgroup(&net_buf, &base.subgroups[i])) {
			LOG_DBG("Failed to decode subgroup %d", i);
			return false;
		}
	}

	if (atomic_test_bit(sink->flags,
			    BT_BAP_BROADCAST_SINK_FLAG_BIGINFO_RECEIVED)) {
		uint8_t num_bis = 0;

		for (int i = 0; i < base.subgroup_count; i++) {
			num_bis += base.subgroups[i].bis_count;
		}

		if (num_bis > sink->biginfo_num_bis) {
			LOG_WRN("BASE contains more BIS than reported by BIGInfo");
			return false;
		}
	}

	sink->codec_qos.pd = base.pd;
	if (memcmp(&sink->base, &base, sizeof(base)) != 0) {
		/* We only overwrite the sink->base data once the base has
		 * successfully been decoded to avoid overwriting it with
		 * invalid data
		 */
		(void)memcpy(&sink->base, &base, sizeof(base));

		if (atomic_test_bit(sink->flags,
				    BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID)) {
			update_recv_state_base(sink);
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->base_recv != NULL) {
			listener->base_recv(sink, &base);
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

	bt_data_parse(buf, pa_decode_base, (void *)sink);
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
	uint16_t timeout;

	/* Ensure that the following calculation does not overflow silently */
	__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

	/* Add retries and convert to unit in 10's of ms */
	timeout = ((uint32_t)interval * SYNC_RETRY_COUNT) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
			BT_GAP_PER_ADV_MAX_TIMEOUT);

	return timeout;
}

static void sync_broadcast_pa(const struct bt_le_scan_recv_info *info,
			      uint32_t broadcast_id)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_le_per_adv_sync_param param;
	struct bt_bap_broadcast_sink *sink;
	int err;

	sink = broadcast_sink_scanning_get();
	/* Should never happen as we set the scanning flag before registering
	 * the scanning callbacks
	 */
	__ASSERT(sink != NULL, "sink is NULL");

	/* Unregister the callbacks to prevent broadcast_scan_recv to be called again */
	bt_le_scan_cb_unregister(&broadcast_scan_cb);
	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Could not stop scan: %d", err);
	} else {
		atomic_clear_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_SCANNING);
	}

	bt_addr_le_copy(&param.addr, info->addr);
	param.options = 0;
	param.sid = info->sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(info->interval);
	err = bt_le_per_adv_sync_create(&param, &sink->pa_sync);
	if (err != 0) {
		LOG_ERR("Could not sync to PA: %d", err);
		broadcast_sink_cleanup(sink);

		SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
			if (listener->scan_term != NULL) {
				listener->scan_term(err);
			}
		}
	} else {
		atomic_set_bit(sink->flags,
			       BT_BAP_BROADCAST_SINK_FLAG_SYNCING);
		sink->broadcast_id = broadcast_id;
	}
}

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	uint32_t *broadcast_id = user_data;
	struct bt_uuid_16 adv_uuid;

	if (sys_slist_is_empty(&sink_cbs)) {
		/* Terminate early if we do not have any broadcast sink listeners */
		return false;
	}

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	if (broadcast_sink_syncing_get() != NULL) {
		/* Already syncing, can maximum sync one */
		return true;
	}

	*broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *ad)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct net_buf_simple_state state;
	uint32_t broadcast_id;

	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) ||
	     info->interval == 0) {
		return;
	}

	/* As scan_check_and_sync_broadcast modifies the AD data,
	 * we store the state before parsing it
	 */
	net_buf_simple_save(ad, &state);
	broadcast_id = INVALID_BROADCAST_ID;
	bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)&broadcast_id);
	net_buf_simple_restore(ad, &state);

	/* We check if `broadcast_id` was modified by `scan_check_and_sync_broadcast`.
	 * If it was then that means that we found a broadcast source
	 */
	if (broadcast_id != INVALID_BROADCAST_ID) {
		LOG_DBG("Found broadcast source with address %s and id 0x%6X",
			bt_addr_le_str(info->addr), broadcast_id);

		if (broadcast_sink_get_by_broadcast_id(broadcast_id) != NULL) {
			LOG_DBG("Broadcast sink with broadcast_id 0x%X already exists",
				broadcast_id);

			return;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
			if (listener->scan_recv != NULL) {
				bool sync_pa;


				/* As the callback receiver may modify the AD
				 * data, we store the state so that we can
				 * restore it for each callback
				 */
				net_buf_simple_save(ad, &state);

				sync_pa = listener->scan_recv(info, ad, broadcast_id);

				if (sync_pa) {
					sync_broadcast_pa(info, broadcast_id);
					break;
				}

				net_buf_simple_restore(ad, &state);
			}
		}
	}
}

static void broadcast_scan_timeout(void)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;

	bt_le_scan_cb_unregister(&broadcast_scan_cb);

	sink = broadcast_sink_scanning_get();
	/* Should never happen as we set the scanning flag before registering
	 * the scanning callbacks
	 */
	__ASSERT(sink != NULL, "sink is NULL");

	broadcast_sink_cleanup(sink);

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->scan_term != NULL) {
			listener->scan_term(-ETIME);
		}
	}
}

void bt_bap_broadcast_sink_register_cb(struct bt_bap_broadcast_sink_cb *cb)
{
	sys_slist_append(&sink_cbs, &cb->_node);
}

int bt_bap_broadcast_sink_scan_start(const struct bt_le_scan_param *param)
{
	struct bt_bap_broadcast_sink *sink;
	int err;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->timeout != 0) {
		/* This is to avoid having to re-implement the scan timeout
		 * callback as well, and can be modified later if requested
		 */
		LOG_DBG("Scan param shall not have a timeout");
		return -EINVAL;
	}

	if (sys_slist_is_empty(&sink_cbs)) {
		LOG_WRN("No broadcast sink callbacks registered");
		return -EINVAL;
	}

	if (broadcast_sink_scanning_get() != NULL) {
		LOG_DBG("Already scanning");

		return -EALREADY;
	}

	sink = broadcast_sink_free_get();
	if (sink == NULL) {
		LOG_DBG("No more free broadcast sinks");
		return -ENOMEM;
	}

	/* TODO: check for scan callback */
	err = bt_le_scan_start(param, NULL);
	if (err == 0) {
		atomic_set_bit(sink->flags,
			       BT_BAP_BROADCAST_SINK_FLAG_INITIALIZED);
		atomic_set_bit(sink->flags,
			       BT_BAP_BROADCAST_SINK_FLAG_SCANNING);

		broadcast_scan_cb.recv = broadcast_scan_recv;
		broadcast_scan_cb.timeout = broadcast_scan_timeout;
		bt_le_scan_cb_register(&broadcast_scan_cb);
	}

	return err;
}

int bt_bap_broadcast_sink_scan_stop(void)
{
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;
	int err;

	sink = broadcast_sink_scanning_get();
	if (sink == NULL) {
		LOG_DBG("Not scanning");

		return -EALREADY;
	}

	if (sink->pa_sync != NULL) {
		err = bt_le_per_adv_sync_delete(sink->pa_sync);
		if (err != 0) {
			LOG_DBG("Could not delete PA sync: %d", err);
			return err;
		}
	}

	broadcast_sink_cleanup(sink);

	err = bt_le_scan_stop();
	if (err == 0) {
		bt_le_scan_cb_unregister(&broadcast_scan_cb);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->scan_term != NULL) {
			listener->scan_term(err);
		}
	}

	return err;
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
					      struct bt_bap_stream *stream, struct bt_codec *codec)
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
	bt_audio_codec_to_iso_path(iso->chan.qos->rx->path, codec);

	bt_bap_iso_unref(iso);

	bt_bap_stream_attach(NULL, stream, ep, codec);
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
		stream->codec = NULL;
		stream->group = NULL;

		sys_slist_remove(&sink->streams, NULL, &stream->_node);
	}

	sink->stream_count = 0;
}

static void broadcast_sink_cleanup(struct bt_bap_broadcast_sink *sink)
{
	if (atomic_test_bit(sink->flags,
			    BT_BAP_BROADCAST_SINK_FLAG_SRC_ID_VALID)) {
		int err;

		err = bt_bap_scan_delegator_rem_src(sink->bass_src_id);
		if (err != 0) {
			LOG_WRN("Failed to remove Receive State for sink %p: %d",
				sink, err);
		}
	}

	if (sink->stream_count > 0U) {
		broadcast_sink_cleanup_streams(sink);
	}

	(void)memset(sink, 0, sizeof(*sink)); /* also clears flags */
}

static struct bt_codec *codec_from_base_by_index(struct bt_bap_base *base, uint8_t index)
{
	for (size_t i = 0U; i < base->subgroup_count; i++) {
		struct bt_bap_base_subgroup *subgroup = &base->subgroups[i];

		for (size_t j = 0U; j < subgroup->bis_count; j++) {
			if (subgroup->bis_data[j].index == index) {
				return &subgroup->codec;
			}
		}
	}

	return NULL;
}

static bool codec_lookup_id(const struct bt_pacs_cap *cap, void *user_data)
{
	struct codec_lookup_id_data *data = user_data;

	if (cap->codec->id == data->id) {
		data->codec = cap->codec;

		return false;
	}

	return true;
}

int bt_bap_broadcast_sink_create(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_bap_broadcast_sink_cb *listener;
	struct bt_bap_broadcast_sink *sink;

	CHECKIF(pa_sync == NULL) {
		LOG_DBG("pa_sync is NULL");

		return -EINVAL;
	}

	CHECKIF(broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: 0x%X", broadcast_id);

		return -EINVAL;
	}

	if (broadcast_sink_get_by_broadcast_id(broadcast_id) != NULL) {
		LOG_DBG("Broadcast sink with broadcast_id 0x%X already exists",
			broadcast_id);

		return -EALREADY;
	}

	sink = broadcast_sink_free_get();
	if (sink == NULL) {
		LOG_DBG("No more free broadcast sinks");

		return -ENOMEM;
	}

	recv_state = bt_bap_scan_delegator_find_state(find_recv_state_by_pa_sync_cb,
						      (void *)pa_sync);
	if (recv_state == NULL) {
		broadcast_sink_add_src(sink);
	} else {
		/* The PA sync is known by the Scan Delegator */
		if (recv_state->broadcast_id != broadcast_id) {
			LOG_DBG("Broadcast ID mismatch: 0x%X != 0x%X",
				recv_state->broadcast_id, broadcast_id);

			return -EINVAL;
		}

		sink->bass_src_id = recv_state->src_id;
	}

	sink->broadcast_id = broadcast_id;
	sink->pa_sync = pa_sync;
	atomic_set_bit(sink->flags, BT_BAP_BROADCAST_SINK_FLAG_INITIALIZED);

	SYS_SLIST_FOR_EACH_CONTAINER(&sink_cbs, listener, _node) {
		if (listener->pa_synced != NULL) {
			listener->pa_synced(sink, sink->pa_sync,
					    sink->broadcast_id);
		}
	}

	return 0;
}

int bt_bap_broadcast_sink_sync(struct bt_bap_broadcast_sink *sink, uint32_t indexes_bitfield,
			       struct bt_bap_stream *streams[], const uint8_t broadcast_code[16])
{
	struct bt_iso_big_sync_param param;
	struct bt_codec *codecs[BROADCAST_SNK_STREAM_CNT] = { NULL };
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
	stream_count = 0;
	for (int i = 1; i < BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
		if ((indexes_bitfield & BIT(i)) != 0) {
			struct bt_codec *codec = codec_from_base_by_index(&sink->base, i);
			struct codec_lookup_id_data lookup_data = { };

			if (codec == NULL) {
				LOG_DBG("Index %d not found in BASE", i);
				return -EINVAL;
			}

			/* Lookup and assign path_id based on capabilities */
			lookup_data.id = codec->id;

			bt_pacs_cap_foreach(BT_AUDIO_DIR_SINK, codec_lookup_id,
					    &lookup_data);
			if (lookup_data.codec == NULL) {
				LOG_DBG("Codec with id %u is not supported by our capabilities",
				       codec->id);

				return -ENOENT;
			}

			codec->path_id = lookup_data.codec->path_id;

			codecs[stream_count++] = codec;

			if (stream_count > BROADCAST_SNK_STREAM_CNT) {
				LOG_DBG("Cannot sync to more than %d streams",
					BROADCAST_SNK_STREAM_CNT);
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
		struct bt_codec *codec;

		stream = streams[i];
		codec = codecs[i];

		err = bt_bap_broadcast_sink_setup_stream(sink, stream, codec);
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
		LOG_DBG("Source does not have any streams");
		return -EINVAL;
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
	int err;

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

	if (sink->pa_sync == NULL) {
		LOG_DBG("Broadcast sink is already deleted");
		return -EALREADY;
	}

	err = bt_le_per_adv_sync_delete(sink->pa_sync);
	if (err != 0) {
		LOG_DBG("Failed to delete periodic advertising sync (err %d)", err);
		return err;
	}

	/* Reset the broadcast sink */
	broadcast_sink_cleanup(sink);

	return 0;
}

static int broadcast_sink_init(const struct device *dev)
{
	static struct bt_le_per_adv_sync_cb cb = {
		.synced = pa_synced,
		.recv = pa_recv,
		.term = pa_term,
		.biginfo = biginfo_recv
	};

	bt_le_per_adv_sync_cb_register(&cb);

	return 0;
}

SYS_INIT(broadcast_sink_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
