/* btp_bap_broadcast.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/bluetooth/audio/audio.h>

#include "bap_endpoint.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap_broadcast
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"
#include "btp_bap_audio_stream.h"
#include "btp_bap_broadcast.h"

static struct btp_bap_broadcast_remote_source remote_broadcast_sources[1];
static struct btp_bap_broadcast_local_source local_source;
/* Only one PA sync supported for now. */
static struct btp_bap_broadcast_remote_source *broadcast_source_to_sync;
/* A mask for the maximum BIS we can sync to. +1 since the BIS indexes start from 1. */
static const uint32_t bis_index_mask = BIT_MASK(CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT + 1);
#define INVALID_BROADCAST_ID      (BT_AUDIO_BROADCAST_ID_MAX + 1)
#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define PA_SYNC_SKIP              5
static struct bt_bap_bass_subgroup
	delegator_subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];

static inline struct btp_bap_broadcast_stream *stream_bap_to_broadcast(struct bt_bap_stream *stream)
{
	return CONTAINER_OF(CONTAINER_OF(CONTAINER_OF(stream, struct bt_cap_stream, bap_stream),
		struct btp_bap_audio_stream, cap_stream), struct btp_bap_broadcast_stream,
		audio_stream);
}

static inline struct bt_bap_stream *stream_broadcast_to_bap(struct btp_bap_broadcast_stream *stream)
{
	return &stream->audio_stream.cap_stream.bap_stream;
}

struct btp_bap_broadcast_local_source *btp_bap_broadcast_local_source_get(uint8_t source_id)
{
	/* Only one local broadcast source supported for now */
	(void) source_id;

	return &local_source;
}

static struct btp_bap_broadcast_remote_source *remote_broadcaster_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(remote_broadcast_sources); i++) {
		struct btp_bap_broadcast_remote_source *broadcaster = &remote_broadcast_sources[i];

		if (broadcaster->broadcast_id == INVALID_BROADCAST_ID) {
			return broadcaster;
		}
	}

	return NULL;
}

static struct btp_bap_broadcast_remote_source *remote_broadcaster_find(const bt_addr_le_t *addr,
							       uint32_t broadcast_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(remote_broadcast_sources); i++) {
		struct btp_bap_broadcast_remote_source *broadcaster = &remote_broadcast_sources[i];

		if (broadcaster->broadcast_id == broadcast_id &&
		    bt_addr_le_cmp(addr, &broadcaster->address) == 0) {
			return broadcaster;
		}
	}

	return NULL;
}

static struct btp_bap_broadcast_remote_source *remote_broadcaster_find_by_sink(
	struct bt_bap_broadcast_sink *sink)
{
	for (size_t i = 0; i < ARRAY_SIZE(remote_broadcast_sources); i++) {
		struct btp_bap_broadcast_remote_source *broadcaster = &remote_broadcast_sources[i];

		if (broadcaster->sink == sink) {
			return broadcaster;
		}
	}

	return NULL;
}

static void btp_send_bis_syced_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
				  uint8_t bis_id)
{
	struct btp_bap_bis_syned_ev ev;

	bt_addr_le_copy(&ev.address, address);
	sys_put_le24(broadcast_id, ev.broadcast_id);
	ev.bis_id = bis_id;

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_SYNCED, &ev, sizeof(ev));
}

static void stream_started(struct bt_bap_stream *stream)
{
	struct btp_bap_broadcast_remote_source *broadcaster;
	struct btp_bap_broadcast_stream *b_stream = stream_bap_to_broadcast(stream);

	/* Callback called on transition to Streaming state */

	LOG_DBG("Started stream %p", stream);

	btp_bap_audio_stream_started(&b_stream->audio_stream);
	b_stream->bis_synced = true;
	broadcaster = &remote_broadcast_sources[b_stream->source_id];

	btp_send_bis_syced_ev(&broadcaster->address, broadcaster->broadcast_id, b_stream->bis_id);
}

static void stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	struct btp_bap_broadcast_stream *b_stream = stream_bap_to_broadcast(stream);

	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	btp_bap_audio_stream_stopped(&b_stream->audio_stream);

	b_stream->bis_synced = false;
}

static void send_bis_stream_received_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
					uint8_t bis_id, uint8_t data_len, uint8_t *data)
{
	struct btp_bap_bis_stream_received_ev *ev;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + data_len, (uint8_t **)&ev);

	LOG_DBG("Stream received, len %d", data_len);

	bt_addr_le_copy(&ev->address, address);
	sys_put_le24(broadcast_id, ev->broadcast_id);
	ev->bis_id = bis_id;
	ev->data_len = data_len;
	memcpy(ev->data, data, data_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_STREAM_RECEIVED, ev,
		     sizeof(*ev) + data_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void stream_recv(struct bt_bap_stream *stream,
			const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	struct btp_bap_broadcast_remote_source *broadcaster;
	struct btp_bap_broadcast_stream *b_stream = stream_bap_to_broadcast(stream);

	if (b_stream->already_sent == false) {
		/* For now, send just a first packet, to limit the number
		 * of logs and not unnecessarily spam through btp.
		 */
		LOG_DBG("Incoming audio on stream %p len %u", stream, buf->len);
		b_stream->already_sent = true;
		broadcaster = &remote_broadcast_sources[b_stream->source_id];
		send_bis_stream_received_ev(&broadcaster->address, broadcaster->broadcast_id,
					    b_stream->bis_id, buf->len, buf->data);
	}
}

static void stream_sent(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream %p sent", stream);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started,
	.stopped = stream_stopped,
	.recv = stream_recv,
	.sent = stream_sent,
};

struct btp_bap_broadcast_stream *btp_bap_broadcast_stream_alloc(
	struct btp_bap_broadcast_local_source *source)
{
	for (size_t i = 0; i < ARRAY_SIZE(source->streams); i++) {
		struct btp_bap_broadcast_stream *stream = &source->streams[i];

		if (stream->in_use == false) {
			bt_bap_stream_cb_register(stream_broadcast_to_bap(stream), &stream_ops);
			stream->in_use = true;

			return stream;
		}
	}

	return NULL;
}

static void remote_broadcaster_free(struct btp_bap_broadcast_remote_source *broadcaster)
{
	(void)memset(broadcaster, 0, sizeof(*broadcaster));

	broadcaster->broadcast_id = INVALID_BROADCAST_ID;

	for (size_t i = 0U; i < ARRAY_SIZE(broadcaster->sink_streams); i++) {
		broadcaster->sink_streams[i] = stream_broadcast_to_bap(&broadcaster->streams[i]);
		broadcaster->sink_streams[i]->ops = &stream_ops;
	}
}

static int setup_broadcast_source(uint8_t streams_per_subgroup,	uint8_t subgroups,
				  struct btp_bap_broadcast_local_source *source,
				  struct bt_audio_codec_cfg *codec_cfg)
{
	int err;
	struct bt_bap_broadcast_source_stream_param
		stream_params[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct bt_bap_broadcast_source_subgroup_param
		subgroup_param[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_bap_broadcast_source_param create_param;

	if (streams_per_subgroup * subgroups > CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT ||
	    subgroups >	CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
		return -EINVAL;
	}

	/* BIS Codec Specific Configuration will be specified on subgroup level,
	 * with a pointer, so let's store the codec_cfg in the first stream instance.
	 */
	memcpy(&source->streams[0].codec_cfg, codec_cfg, sizeof(*codec_cfg));

	for (size_t i = 0U; i < subgroups; i++) {
		subgroup_param[i].params_count = streams_per_subgroup;
		subgroup_param[i].params = stream_params + i * streams_per_subgroup;
		subgroup_param[i].codec_cfg = &source->streams[0].codec_cfg;
	}

	for (size_t j = 0U; j < streams_per_subgroup; j++) {
		struct btp_bap_broadcast_stream *b_stream = &source->streams[j];
		struct bt_bap_stream *stream = stream_broadcast_to_bap(b_stream);

		stream_params[j].stream = stream;
		bt_bap_stream_cb_register(stream, &stream_ops);

		/* BIS Codec Specific Configuration specified on subgroup level */
		stream_params[j].data = NULL;
		stream_params[j].data_len = 0U;
	}

	create_param.params_count = subgroups;
	create_param.params = subgroup_param;
	create_param.qos = &source->qos;
	create_param.encryption = false;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	LOG_DBG("Creating broadcast source with %zu subgroups with %zu streams",
		subgroups, subgroups * streams_per_subgroup);

	if (source->bap_broadcast == NULL) {
		err = bt_bap_broadcast_source_create(&create_param,
						     &source->bap_broadcast);
		if (err != 0) {
			LOG_DBG("Unable to create broadcast source: %d", err);

			return err;
		}
	} else {
		err = bt_bap_broadcast_source_reconfig(source->bap_broadcast,
						       &create_param);
		if (err != 0) {
			LOG_DBG("Unable to reconfig broadcast source: %d", err);

			return err;
		}
	}

	return 0;
}

uint8_t btp_bap_broadcast_source_setup(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_audio_codec_cfg codec_cfg;
	const struct btp_bap_broadcast_source_setup_cmd *cp = cmd;
	struct btp_bap_broadcast_source_setup_rp *rp = rsp;
	struct bt_le_adv_param *param = BT_LE_EXT_ADV_NCONN_NAME;

	/* Only one local source/BIG supported for now */
	struct btp_bap_broadcast_local_source *source = &local_source;

	uint32_t gap_settings = BIT(BTP_GAP_SETTINGS_DISCOVERABLE) |
				BIT(BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);

	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	/* Broadcast Audio Streaming Endpoint advertising data */
	struct bt_data base_ad;
	struct bt_data per_ad;

	LOG_DBG("");

	memset(&codec_cfg, 0, sizeof(codec_cfg));
	codec_cfg.id = cp->coding_format;
	codec_cfg.vid = cp->vid;
	codec_cfg.cid = cp->cid;
	codec_cfg.data_len = cp->cc_ltvs_len;
	memcpy(codec_cfg.data, cp->cc_ltvs, cp->cc_ltvs_len);

	source->qos.phy = BT_AUDIO_CODEC_QOS_2M;
	source->qos.framing = cp->framing;
	source->qos.rtn = cp->retransmission_num;
	source->qos.latency = sys_le16_to_cpu(cp->max_transport_latency);
	source->qos.interval = sys_get_le24(cp->sdu_interval);
	source->qos.pd = sys_get_le24(cp->presentation_delay);
	source->qos.sdu = sys_le16_to_cpu(cp->max_sdu);

	err = setup_broadcast_source(cp->streams_per_subgroup, cp->subgroups, source, &codec_cfg);
	if (err != 0) {
		LOG_DBG("Unable to setup broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_get_id(source->bap_broadcast, &source->broadcast_id);
	if (err != 0) {
		LOG_DBG("Unable to get broadcast ID: %d", err);

		return BTP_STATUS_FAILED;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, source->broadcast_id);
	base_ad.type = BT_DATA_SVC_DATA16;
	base_ad.data_len = ad_buf.len;
	base_ad.data = ad_buf.data;
	err = tester_gap_create_adv_instance(param, BTP_GAP_ADDR_TYPE_IDENTITY, &base_ad, 1, NULL,
					     0, &gap_settings);
	if (err != 0) {
		LOG_DBG("Failed to create extended advertising instance: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = tester_gap_padv_configure(BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_FAST_INT_MIN_2,
							    BT_GAP_PER_ADV_FAST_INT_MAX_2,
							    BT_LE_PER_ADV_OPT_USE_TX_POWER));
	if (err != 0) {
		LOG_DBG("Failed to configure periodic advertising: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_get_base(source->bap_broadcast, &base_buf);
	if (err != 0) {
		LOG_DBG("Failed to get encoded BASE: %d\n", err);

		return BTP_STATUS_FAILED;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = tester_gap_padv_set_data(&per_ad, 1);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	rp->gap_settings = gap_settings;
	sys_put_le24(source->broadcast_id, rp->broadcast_id);
	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_source_release(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_bap_broadcast_local_source *source = &local_source;

	LOG_DBG("");

	err = bt_bap_broadcast_source_delete(source->bap_broadcast);
	if (err != 0) {
		LOG_DBG("Unable to delete broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	memset(source, 0, sizeof(*source));

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_adv_start(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get();

	LOG_DBG("");

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_start_ext_adv();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_padv_start();
	if (err != 0) {
		LOG_DBG("Unable to start periodic advertising: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_adv_stop(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = tester_gap_padv_stop();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = tester_gap_stop_ext_adv();

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_source_start(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_bap_broadcast_local_source *source = &local_source;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get();

	LOG_DBG("");

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_source_start(source->bap_broadcast, ext_adv);
	if (err != 0) {
		LOG_DBG("Unable to start broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_source_stop(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_bap_broadcast_local_source *source = &local_source;

	LOG_DBG("");

	err = bt_bap_broadcast_source_stop(source->bap_broadcast);
	if (err != 0) {
		LOG_DBG("Unable to stop broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static int broadcast_sink_reset(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(remote_broadcast_sources); i++) {
		remote_broadcaster_free(&remote_broadcast_sources[i]);
	}

	return 0;
}

static void btp_send_baa_found_ev(const bt_addr_le_t *address, uint32_t broadcast_id,
				  uint8_t sid, uint16_t interval)
{
	struct btp_bap_baa_found_ev ev;

	bt_addr_le_copy(&ev.address, address);
	sys_put_le24(broadcast_id, ev.broadcast_id);
	ev.advertiser_sid = sid;
	ev.padv_interval = sys_cpu_to_le16(interval);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BAA_FOUND, &ev, sizeof(ev));
}

static bool baa_check(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	/* Parse the scanned Broadcast Audio Announcement */

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

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("Found BAA with ID 0x%06X, addr %s, sid 0x%02X, interval 0x%04X",
		broadcast_id, le_addr, info->sid, info->interval);

	btp_send_baa_found_ev(info->addr, broadcast_id, info->sid, info->interval);

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	/* If 0 there is no periodic advertising. */
	if (info->interval != 0U) {
		bt_data_parse(ad, baa_check, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void btp_send_bis_found_ev(const bt_addr_le_t *address, uint32_t broadcast_id, uint32_t pd,
				  uint8_t subgroup_index, uint8_t bis_index,
				  const struct bt_audio_codec_cfg *codec_cfg)
{
	struct btp_bap_bis_found_ev *ev;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + codec_cfg->data_len, (uint8_t **)&ev);

	bt_addr_le_copy(&ev->address, address);
	sys_put_le24(broadcast_id, ev->broadcast_id);
	sys_put_le24(pd, ev->presentation_delay);
	ev->subgroup_id = subgroup_index;
	ev->bis_id = bis_index;
	ev->coding_format = codec_cfg->id;
	ev->vid = sys_cpu_to_le16(codec_cfg->vid);
	ev->cid = sys_cpu_to_le16(codec_cfg->cid);

	ev->cc_ltvs_len = codec_cfg->data_len;
	memcpy(ev->cc_ltvs, codec_cfg->data, ev->cc_ltvs_len);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_FOUND, ev,
		     sizeof(*ev) + ev->cc_ltvs_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

struct base_parse_data {
	struct btp_bap_broadcast_remote_source *broadcaster;
	uint32_t pd;
	struct bt_audio_codec_cfg codec_cfg;
	uint8_t subgroup_cnt;
	uint32_t bis_bitfield;
	size_t stream_cnt;
};

static bool base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	struct base_parse_data *parse_data = user_data;
	struct bt_audio_codec_cfg *codec_cfg = &parse_data->codec_cfg;
	struct btp_bap_broadcast_remote_source *broadcaster = parse_data->broadcaster;

	parse_data->bis_bitfield |= BIT(bis->index);

	if (parse_data->stream_cnt < ARRAY_SIZE(broadcaster->streams)) {
		struct btp_bap_broadcast_stream *stream =
			&broadcaster->streams[parse_data->stream_cnt++];

		stream->bis_id = bis->index;
		memcpy(&stream->codec_cfg, codec_cfg, sizeof(*codec_cfg));
	}

	btp_send_bis_found_ev(&broadcaster->address, broadcaster->broadcast_id, parse_data->pd,
			      parse_data->subgroup_cnt, bis->index, codec_cfg);

	return true;
}

static bool base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	struct base_parse_data *parse_data = user_data;
	int err;

	err = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &parse_data->codec_cfg);
	if (err != 0) {
		LOG_DBG("Failed to retrieve codec config: %d", err);
		return false;
	}

	err = bt_bap_base_subgroup_foreach_bis(subgroup, base_subgroup_bis_cb, user_data);
	if (err != 0) {
		LOG_DBG("Failed to parse all BIS: %d", err);
		return false;
	}

	return true;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	struct btp_bap_broadcast_remote_source *broadcaster;
	struct base_parse_data parse_data = {0};
	int ret;

	LOG_DBG("");

	broadcaster = remote_broadcaster_find_by_sink(sink);
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return;
	}

	LOG_DBG("Received BASE: broadcast sink %p subgroups %u",
		sink, bt_bap_base_get_subgroup_count(base));

	ret = bt_bap_base_get_pres_delay(base);
	if (ret < 0) {
		LOG_ERR("Failed to get presentation delay: %d", ret);
		return;
	}

	parse_data.broadcaster = broadcaster;
	parse_data.pd = (uint32_t)ret;

	ret = bt_bap_base_foreach_subgroup(base, base_subgroup_cb, &parse_data);
	if (ret != 0) {
		LOG_ERR("Failed to parse subgroups: %d", ret);
		return;
	}

	broadcaster->bis_index_bitfield = parse_data.bis_bitfield & bis_index_mask;
	LOG_DBG("bis_index_bitfield 0x%08x", broadcaster->bis_index_bitfield);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	int err;
	uint32_t index_bitfield;
	struct btp_bap_broadcast_remote_source *broadcaster = remote_broadcaster_find_by_sink(sink);

	if (broadcaster == NULL) {
		LOG_ERR("remote_broadcaster_find_by_sink failed, %p", sink);

		return;
	}

	LOG_DBG("Broadcaster PA found, encrypted %d, requested_bis_sync %d", biginfo->encryption,
		broadcaster->requested_bis_sync);

	if (biginfo->encryption) {
		/* Wait for Set Broadcast Code and start sync at broadcast_code_cb */
		return;
	}

	if (!broadcaster->assistant_request || !broadcaster->requested_bis_sync) {
		/* No sync with any BIS was requested yet */
		return;
	}

	index_bitfield = broadcaster->bis_index_bitfield & broadcaster->requested_bis_sync;
	err = bt_bap_broadcast_sink_sync(broadcaster->sink, index_bitfield,
					 broadcaster->sink_streams,
					 broadcaster->sink_broadcast_code);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);
	}

	broadcaster->assistant_request = false;
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static void pa_timer_handler(struct k_work *work)
{
	if (broadcast_source_to_sync != NULL) {
		enum bt_bap_pa_state pa_state;
		const struct bt_bap_scan_delegator_recv_state *recv_state =
			broadcast_source_to_sync->sink_recv_state;

		if (recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
			pa_state = BT_BAP_PA_STATE_NO_PAST;
		} else {
			pa_state = BT_BAP_PA_STATE_FAILED;
		}

		bt_bap_scan_delegator_set_pa_state(recv_state->src_id, pa_state);
	}

	LOG_DBG("PA timeout");
}

static K_WORK_DELAYABLE_DEFINE(pa_timer, pa_timer_handler);

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	int err;

	LOG_DBG("Sync info: service_data 0x%04X", info->service_data);

	k_work_cancel_delayable(&pa_timer);

	/* We are synced to a PA. We know that this is the Broadcaster PA we wanted
	 * to sync to, because we support only one sync for now.
	 */
	if (broadcast_source_to_sync == NULL) {
		LOG_DBG("Failed to create broadcast sink, NULL ptr");

		return;
	}

	/* In order to parse the BASE and BIG Info from the Broadcast PA, we have to create
	 * a Broadcast Sink instance. From now on callbacks of the broadcast_sink_cbs will be used.
	 */
	err = bt_bap_broadcast_sink_create(sync, broadcast_source_to_sync->broadcast_id,
					   &broadcast_source_to_sync->sink);
	if (err != 0) {
		LOG_DBG("Failed to create broadcast sink: ID 0x%06X, err %d",
			broadcast_source_to_sync->broadcast_id, err);
	}

	broadcast_source_to_sync = NULL;
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
};

static void btp_send_pas_sync_req_ev(struct bt_conn *conn, uint8_t src_id,
				     uint8_t advertiser_sid, uint32_t broadcast_id,
				     bool past_avail, uint16_t pa_interval)
{
	struct btp_bap_pa_sync_req_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.src_id = src_id;
	ev.advertiser_sid = advertiser_sid;
	sys_put_le24(broadcast_id, ev.broadcast_id);
	ev.past_avail = past_avail;
	ev.pa_interval = sys_cpu_to_le16(pa_interval);

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_PA_SYNC_REQ, &ev, sizeof(ev));
}

static void btp_send_scan_delegator_found_ev(struct bt_conn *conn)
{
	struct btp_bap_scan_delegator_found_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_SCAN_DELEGATOR_FOUND, &ev, sizeof(ev));
}

static void btp_send_broadcast_receive_state_ev(struct bt_conn *conn,
	const struct bt_bap_scan_delegator_recv_state *state)
{
	struct btp_bap_broadcast_receive_state_ev *ev;
	size_t len;
	uint8_t *ptr;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + CONFIG_BT_BAP_BASS_MAX_SUBGROUPS *
		sizeof(struct bt_bap_bass_subgroup), (uint8_t **)&ev);

	if (conn) {
		bt_addr_le_copy(&ev->address, bt_conn_get_dst(conn));
	} else {
		(void)memset(&ev->address, 0, sizeof(ev->address));
	}

	ev->src_id = state->src_id;
	bt_addr_le_copy(&ev->broadcaster_address, &state->addr);
	ev->advertiser_sid = state->adv_sid;
	sys_put_le24(state->broadcast_id, ev->broadcast_id);
	ev->pa_sync_state = state->pa_sync_state;
	ev->big_encryption = state->encrypt_state;
	ev->num_subgroups = state->num_subgroups;

	ptr = ev->subgroups;
	for (uint8_t i = 0; i < ev->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		sys_put_le32(subgroup->bis_sync >> 1, ptr);
		ptr += sizeof(subgroup->bis_sync);
		*ptr = subgroup->metadata_len;
		ptr += sizeof(subgroup->metadata_len);
		memcpy(ptr, subgroup->metadata, subgroup->metadata_len);
		ptr += subgroup->metadata_len;
	}

	len = sizeof(*ev) + ptr - ev->subgroups;
	tester_event(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BROADCAST_RECEIVE_STATE, ev, len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static int pa_sync_past(struct bt_conn *conn, uint16_t sync_timeout)
{
	struct bt_le_per_adv_sync_transfer_param param = { 0 };
	int err;

	param.skip = PA_SYNC_SKIP;
	param.timeout = sync_timeout;

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err != 0) {
		LOG_DBG("Could not do PAST subscribe: %d", err);
	} else {
		LOG_DBG("Syncing with PAST: %d", err);
		(void)k_work_reschedule(&pa_timer, K_MSEC(param.timeout * 10));
	}

	return err;
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	struct btp_bap_broadcast_remote_source *broadcaster;

	LOG_DBG("sync state %d ", recv_state->pa_sync_state);

	broadcaster = remote_broadcaster_find(&recv_state->addr, recv_state->broadcast_id);
	if (broadcaster == NULL) {
		/* The Broadcast Assistant gave us the info about the Broadcaster, we have not
		 * scanned this Broadcaster before. The Broadcast Sink does not exist yet.
		 */

		broadcaster = remote_broadcaster_alloc();
		if (broadcaster == NULL) {
			LOG_ERR("Failed to allocate broadcast source");
			return -EINVAL;
		}

		broadcaster->broadcast_id = recv_state->broadcast_id;
		bt_addr_le_copy(&broadcaster->address, &recv_state->addr);
	}

	broadcaster->sink_recv_state = recv_state;

	btp_send_pas_sync_req_ev(conn, recv_state->src_id, recv_state->adv_sid,
				 recv_state->broadcast_id, past_avail, pa_interval);

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct btp_bap_broadcast_remote_source *broadcaster;

	LOG_DBG("");

	broadcaster = remote_broadcaster_find(&recv_state->addr, recv_state->broadcast_id);
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return -EINVAL;
	}

	broadcaster->sink_recv_state = recv_state;

	tester_gap_padv_stop_sync();

	return 0;
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	int err;
	uint32_t index_bitfield;
	struct btp_bap_broadcast_remote_source *broadcaster;

	LOG_DBG("Broadcast code received for %p", recv_state);

	broadcaster = remote_broadcaster_find(&recv_state->addr, recv_state->broadcast_id);
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return;
	}

	broadcaster->sink_recv_state = recv_state;
	(void)memcpy(broadcaster->sink_broadcast_code, broadcast_code,
		     BT_AUDIO_BROADCAST_CODE_SIZE);

	if (!broadcaster->requested_bis_sync) {
		return;
	}

	index_bitfield = broadcaster->bis_index_bitfield & broadcaster->requested_bis_sync;
	err = bt_bap_broadcast_sink_sync(broadcaster->sink, index_bitfield,
					 broadcaster->sink_streams,
					 broadcaster->sink_broadcast_code);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);
	}
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	struct btp_bap_broadcast_remote_source *broadcaster;
	bool bis_synced = false;

	LOG_DBG("BIS sync request received for %p: 0x%08x", recv_state, bis_sync_req[0]);

	broadcaster = remote_broadcaster_find(&recv_state->addr, recv_state->broadcast_id);
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return -EINVAL;
	}

	broadcaster->requested_bis_sync = bis_sync_req[0];
	broadcaster->assistant_request = true;

	for (int i = 0; i < ARRAY_SIZE(broadcaster->streams); i++) {
		if (broadcaster->streams[i].bis_synced) {
			bis_synced = true;
			break;
		}
	}

	/* We only care about a single subgroup in this sample */
	if (bis_synced) {
		/* If the BIS sync request is received while we are already
		 * synced, it means that the requested BIS sync has changed.
		 */
		int err;

		/* The stream stopped callback will be called as part of this,
		 * and we do not need to wait for any events from the
		 * controller. Thus, when this returns, the `bis_synced`
		 * is back to false.
		 */
		err = bt_bap_broadcast_sink_stop(broadcaster->sink);
		if (err != 0) {
			LOG_DBG("Failed to stop Broadcast Sink: %d", err);

			return err;
		}
	}

	return 0;
}

static void recv_state_updated_cb(struct bt_conn *conn,
				  const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	LOG_DBG("Receive state with ID %u updated", recv_state->src_id);

	btp_send_broadcast_receive_state_ev(conn, recv_state);
}

static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
	.recv_state_updated = recv_state_updated_cb,
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.broadcast_code = broadcast_code_cb,
	.bis_sync_req = bis_sync_req_cb,
};

uint8_t btp_bap_broadcast_sink_setup(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = broadcast_sink_reset();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	/* For Scan Delegator role */
	bt_bap_scan_delegator_register_cb(&scan_delegator_cbs);

	/* For Broadcast Sink role */
	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);

	/* For Broadcast Sink or Broadcast Assistant role */
	bt_le_scan_cb_register(&bap_scan_cb);

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_sink_release(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = broadcast_sink_reset();

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_scan_start(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0 && err != -EALREADY) {
		LOG_DBG("Unable to start scan for broadcast sources: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_scan_stop(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_DBG("Failed to stop scan, %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_sink_sync(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	struct btp_bap_broadcast_remote_source *broadcaster;
	const struct btp_bap_broadcast_sink_sync_cmd *cp = cmd;
	struct bt_le_per_adv_sync_param create_params = {0};
	uint32_t broadcast_id = sys_get_le24(cp->broadcast_id);

	LOG_DBG("");

	broadcaster = remote_broadcaster_find(&cp->address, broadcast_id);
	if (broadcaster == NULL) {
		broadcaster = remote_broadcaster_alloc();
		if (broadcaster == NULL) {
			LOG_ERR("Failed to allocate broadcast source");
			return BTP_STATUS_FAILED;
		}

		broadcaster->broadcast_id = broadcast_id;
		bt_addr_le_copy(&broadcaster->address, &cp->address);
	}

	broadcast_source_to_sync = broadcaster;

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) && cp->past_avail) {
		/* The Broadcast Assistant supports PAST transfer, and it has found
		 * a Broadcaster for us. Let's sync to the Broadcaster PA with the PAST.
		 */

		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
		if (!conn) {
			broadcast_source_to_sync = NULL;

			return BTP_STATUS_FAILED;
		}

		err = bt_bap_scan_delegator_set_pa_state(cp->src_id, BT_BAP_PA_STATE_INFO_REQ);
		if (err != 0) {
			LOG_DBG("Failed to set INFO_REQ state: %d", err);
		}

		err = pa_sync_past(conn, cp->sync_timeout);
	} else {
		/* We scanned on our own or the Broadcast Assistant does not support PAST transfer.
		 * Let's sync to the Broadcaster PA without PAST.
		 */
		bt_addr_le_copy(&create_params.addr, &cp->address);
		create_params.options = 0;
		create_params.sid = cp->advertiser_sid;
		create_params.skip = cp->skip;
		create_params.timeout = cp->sync_timeout;
		err = tester_gap_padv_create_sync(&create_params);
	}

	if (err != 0) {
		broadcast_source_to_sync = NULL;

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_sink_stop(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_bap_broadcast_remote_source *broadcaster;
	const struct btp_bap_broadcast_sink_stop_cmd *cp = cmd;
	uint32_t broadcast_id = sys_get_le24(cp->broadcast_id);

	LOG_DBG("");

	broadcaster = remote_broadcaster_find(&cp->address, broadcast_id);
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return BTP_STATUS_FAILED;
	}

	broadcaster->requested_bis_sync = 0;

	err = bt_bap_broadcast_sink_stop(broadcaster->sink);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = tester_gap_padv_stop_sync();
	if (err != 0) {
		LOG_DBG("Failed to stop PA sync, %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_sink_bis_sync(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_bap_broadcast_remote_source *broadcaster;
	const struct btp_bap_broadcast_sink_bis_sync_cmd *cp = cmd;

	LOG_DBG("");

	broadcaster = remote_broadcaster_find(&cp->address, sys_get_le24(cp->broadcast_id));
	if (broadcaster == NULL) {
		LOG_ERR("Failed to find broadcaster");

		return BTP_STATUS_FAILED;
	}

	if (cp->requested_bis_sync == BT_BAP_BIS_SYNC_NO_PREF) {
		broadcaster->requested_bis_sync = sys_le32_to_cpu(cp->requested_bis_sync);
	} else {
		/* For semantic purposes Zephyr API uses BIS Index bitfield
		 * where BIT(1) means BIS Index 1
		 */
		broadcaster->requested_bis_sync = sys_le32_to_cpu(cp->requested_bis_sync) << 1;
	}

	err = bt_bap_broadcast_sink_sync(broadcaster->sink, broadcaster->requested_bis_sync,
					 broadcaster->sink_streams,
					 broadcaster->sink_broadcast_code);
	if (err != 0) {
		LOG_DBG("Unable to sync to BISes, req_bis_sync %d, err %d",
			broadcaster->requested_bis_sync, err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	LOG_DBG("err %d", err);

	if (err != 0) {
		LOG_DBG("BASS discover failed (%d)", err);
	} else {
		LOG_DBG("BASS discover done with %u recv states", recv_state_count);

		btp_send_scan_delegator_found_ev(conn);
	}
}

static void bap_broadcast_assistant_scan_cb(const struct bt_le_scan_recv_info *info,
					    uint32_t broadcast_id)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_DBG("[DEVICE]: %s, broadcast_id 0x%06X, interval (ms) %u), SID 0x%x, RSSI %i", le_addr,
		broadcast_id, BT_GAP_PER_ADV_INTERVAL_TO_MS(info->interval), info->sid, info->rssi);
}

static void bap_broadcast_assistant_recv_state_cb(struct bt_conn *conn, int err,
	const struct bt_bap_scan_delegator_recv_state *state)
{
	LOG_DBG("err: %d", err);

	if (err != 0 || state == NULL) {
		return;
	}

	btp_send_broadcast_receive_state_ev(conn, state);
}

static void bap_broadcast_assistant_recv_state_removed_cb(struct bt_conn *conn, int err,
							  uint8_t src_id)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_scan_start_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_scan_stop_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_add_src_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_mod_src_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_broadcast_code_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static void bap_broadcast_assistant_rem_src_cb(struct bt_conn *conn, int err)
{
	LOG_DBG("err: %d", err);
}

static struct bt_bap_broadcast_assistant_cb broadcast_assistant_cb = {
	.discover = bap_broadcast_assistant_discover_cb,
	.scan = bap_broadcast_assistant_scan_cb,
	.recv_state = bap_broadcast_assistant_recv_state_cb,
	.recv_state_removed = bap_broadcast_assistant_recv_state_removed_cb,
	.scan_start = bap_broadcast_assistant_scan_start_cb,
	.scan_stop = bap_broadcast_assistant_scan_stop_cb,
	.add_src = bap_broadcast_assistant_add_src_cb,
	.mod_src = bap_broadcast_assistant_mod_src_cb,
	.broadcast_code = bap_broadcast_assistant_broadcast_code_cb,
	.rem_src = bap_broadcast_assistant_rem_src_cb,
};

uint8_t btp_bap_broadcast_discover_scan_delegators(const void *cmd, uint16_t cmd_len,
						   void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_bap_discover_scan_delegators_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_assistant_discover(conn);

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_assistant_scan_start(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_bap_broadcast_assistant_scan_start_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_assistant_scan_start(conn, true);

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_assistant_scan_stop(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_bap_broadcast_assistant_scan_stop_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_assistant_scan_stop(conn);

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_assistant_add_src(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len)
{
	int err;
	const uint8_t *ptr;
	struct bt_conn *conn;
	const struct btp_bap_add_broadcast_src_cmd *cp = cmd;
	struct bt_bap_broadcast_assistant_add_src_param param = { 0 };

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	memset(delegator_subgroups, 0, sizeof(delegator_subgroups));
	bt_addr_le_copy(&param.addr, &cp->broadcaster_address);
	param.adv_sid = cp->advertiser_sid;
	param.pa_sync = cp->padv_sync > 0 ? true : false;
	param.broadcast_id = sys_get_le24(cp->broadcast_id);
	param.pa_interval = sys_le16_to_cpu(cp->padv_interval);
	param.num_subgroups = MIN(cp->num_subgroups, CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
	param.subgroups = delegator_subgroups;

	ptr = cp->subgroups;
	for (uint8_t i = 0; i < param.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &delegator_subgroups[i];

		subgroup->bis_sync = sys_get_le32(ptr);
		if (subgroup->bis_sync != BT_BAP_BIS_SYNC_NO_PREF) {
			/* For semantic purposes Zephyr API uses BIS Index bitfield
			 * where BIT(1) means BIS Index 1
			 */
			subgroup->bis_sync <<= 1;
		}

		ptr += sizeof(subgroup->bis_sync);
		subgroup->metadata_len = *ptr;
		ptr += sizeof(subgroup->metadata_len);
		memcpy(subgroup->metadata, ptr, subgroup->metadata_len);
		ptr += subgroup->metadata_len;
	}

	err = bt_bap_broadcast_assistant_add_src(conn, &param);
	if (err != 0) {
		LOG_DBG("err %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_assistant_remove_src(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_bap_remove_broadcast_src_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_assistant_rem_src(conn, cp->src_id);

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_assistant_modify_src(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	int err;
	const uint8_t *ptr;
	struct bt_conn *conn;
	const struct btp_bap_modify_broadcast_src_cmd *cp = cmd;
	struct bt_bap_broadcast_assistant_mod_src_param param = { 0 };

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	memset(delegator_subgroups, 0, sizeof(delegator_subgroups));
	param.src_id = cp->src_id;
	param.pa_sync = cp->padv_sync > 0 ? true : false;
	param.pa_interval = sys_le16_to_cpu(cp->padv_interval);
	param.num_subgroups = MIN(cp->num_subgroups, CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
	param.subgroups = delegator_subgroups;

	ptr = cp->subgroups;
	for (uint8_t i = 0; i < param.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &delegator_subgroups[i];

		subgroup->bis_sync = sys_get_le32(ptr);
		if (subgroup->bis_sync != BT_BAP_BIS_SYNC_NO_PREF) {
			/* For semantic purposes Zephyr API uses BIS Index bitfield
			 * where BIT(1) means BIS Index 1
			 */
			subgroup->bis_sync <<= 1;
		}

		ptr += sizeof(subgroup->bis_sync);
		subgroup->metadata_len = *ptr;
		ptr += sizeof(subgroup->metadata_len);
		memcpy(subgroup->metadata, ptr, subgroup->metadata_len);
		ptr += subgroup->metadata_len;
	}

	err = bt_bap_broadcast_assistant_mod_src(conn, &param);

	return BTP_STATUS_VAL(err);
}

uint8_t btp_bap_broadcast_assistant_set_broadcast_code(const void *cmd, uint16_t cmd_len,
						       void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_bap_set_broadcast_code_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	err = bt_bap_broadcast_assistant_set_broadcast_code(conn, cp->src_id, cp->broadcast_code);
	if (err != 0) {
		LOG_DBG("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t btp_bap_broadcast_assistant_send_past(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	int err;
	uint16_t service_data;
	struct bt_conn *conn;
	struct bt_le_per_adv_sync *pa_sync;
	const struct btp_bap_send_past_cmd *cp = cmd;

	LOG_DBG("");

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	pa_sync = tester_gap_padv_get();
	if (!pa_sync) {
		LOG_DBG("Could not send PAST to Scan Delegator");

		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Sending PAST");

	/* If octet 0 is set to 0, it means AdvA in PAST matches AdvA in ADV_EXT_IND.
	 * Octet 1 shall be set to Source_ID.
	 */
	service_data = cp->src_id << 8;

	err = bt_le_per_adv_sync_transfer(pa_sync, conn, service_data);
	if (err != 0) {
		LOG_DBG("Could not transfer periodic adv sync: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static bool broadcast_inited;

int btp_bap_broadcast_init(void)
{
	if (broadcast_inited) {
		return 0;
	}

	broadcast_sink_reset();

	/* For Broadcast Assistant role */
	bt_bap_broadcast_assistant_register_cb(&broadcast_assistant_cb);

	broadcast_inited = true;

	return 0;
}
