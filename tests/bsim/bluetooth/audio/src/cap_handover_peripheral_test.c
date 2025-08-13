/*
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"
#include "bap_common.h"

LOG_MODULE_REGISTER(cap_handover_peripheral, LOG_LEVEL_DBG);

#if defined(CONFIG_BT_CAP_ACCEPTOR)
extern enum bst_result_t bst_result;

#define CAP_INITIATOR_DEV_ID 0 /* CAP initiator shall be ID 0 for these tests */

CREATE_FLAG(flag_broadcast_code);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(flag_pa_sync_lost);
CREATE_FLAG(flag_pa_request);
CREATE_FLAG(flag_bis_sync_requested);
CREATE_FLAG(flag_base_metadata_updated);
CREATE_FLAG(flag_stream_configured);
CREATE_FLAG(flag_stream_started);
CREATE_FLAG(flag_stream_stopped);
CREATE_FLAG(flag_broadcast_started);
CREATE_FLAG(flag_broadcast_stopped);

static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_le_per_adv_sync *pa_sync;
static const struct bt_bap_scan_delegator_recv_state *cached_recv_state;
static uint32_t cached_bis_sync_req;
static uint16_t cached_pa_interval;
static struct audio_test_stream
	streams[MIN(CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT,
		    CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT + CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT)];
static uint8_t received_base[UINT8_MAX];
static size_t received_base_size;

static const struct bt_bap_qos_cfg_pref unicast_qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u, 20000u, 40000u);

static bool subgroup_data_func_cb(struct bt_data *data, void *user_data)
{
	bool *stream_context_found = (bool *)user_data;

	LOG_DBG("type %u len %u", data->type, data->data_len);

	if (!valid_metadata_type(data->type, data->data_len)) {
		return false;
	}

	if (data->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
		if (data->data_len != 2) { /* Stream context size */
			return false;
		}

		*stream_context_found = true;
		return false;
	}

	return true;
}

static bool valid_subgroup_metadata_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	static uint8_t metadata[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
	static size_t metadata_size;
	bool stream_context_found = false;
	uint8_t *meta;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &meta);
	if (ret < 0) {
		FAIL("Could not get subgroup meta: %d\n", ret);
		return false;
	}

	if (TEST_FLAG(flag_base_received) && (!util_eq(meta, ret, metadata, metadata_size))) {
		LOG_DBG("Metadata updated");
		SET_FLAG(flag_base_metadata_updated);
	}

	metadata_size = (size_t)ret;

	ret = bt_audio_data_parse(meta, (size_t)ret, subgroup_data_func_cb, &stream_context_found);
	if (ret != 0 && ret != -ECANCELED) {
		return false;
	}

	if (!stream_context_found) {
		LOG_DBG("Subgroup did not have streaming context");
	}

	/* if this is false, the iterator will return early with an error */
	return stream_context_found;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	int ret;

	if (TEST_FLAG(flag_base_received)) {
		/* Don't expect any BASE updates */
		return;
	}

	ret = bt_bap_base_get_subgroup_count(base);
	if (ret < 0) {
		FAIL("Failed to get subgroup count: %d\n", ret);
		return;
	} else if (ret == 0) {
		FAIL("subgroup_count was 0\n");
		return;
	}

	LOG_DBG("Received BASE with %d subgroups from broadcast sink %p", ret, sink);

	ret = bt_bap_base_foreach_subgroup(base, valid_subgroup_metadata_cb, NULL);
	if (ret != 0) {
		FAIL("Failed to parse subgroups: %d\n", ret);
		return;
	}

	(void)memcpy(received_base, base, base_size);
	received_base_size = base_size;

	SET_FLAG(flag_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	LOG_DBG("Broadcast sink %p syncable with%s encryption", sink,
		biginfo->encryption ? "" : "out");
	SET_FLAG(flag_syncable);
}

static void broadcast_sink_started_cb(struct bt_bap_broadcast_sink *sink)
{
	SET_FLAG(flag_broadcast_started);
}

static void broadcast_sink_stopped_cb(struct bt_bap_broadcast_sink *sink, uint8_t reason)
{
	SET_FLAG(flag_broadcast_stopped);
}

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == pa_sync) {
		LOG_DBG("PA sync %p synced for broadcast sink with broadcast ID 0x%06X", sync,
			cached_recv_state->broadcast_id);

		SET_FLAG(flag_pa_synced);
	} else {
		FAIL("Unexpected PA sync: %p\n");
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		LOG_DBG("PA sync %p lost with reason %u", sync, info->reason);
		pa_sync = NULL;

		SET_FLAG(flag_pa_sync_lost);
	}
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	LOG_DBG("Enabled: stream %p ", stream);

	err = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (err != 0) {
		FAIL("Failed to get ep info: %d\n", err);
		return;
	}

	if (ep_info.dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		err = bt_bap_stream_start(stream);

		if (err != 0) {
			FAIL("Failed to start stream: %d\n", err);
			return;
		}
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	memset(&test_stream->last_info, 0, sizeof(test_stream->last_info));
	test_stream->rx_cnt = 0U;
	test_stream->valid_rx_cnt = 0U;
	test_stream->seq_num = 0U;
	test_stream->tx_cnt = 0U;

	LOG_DBG("Started stream %p", stream);

	SET_FLAG(flag_stream_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	SET_FLAG(flag_stream_stopped);
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		/* Already syncing */
		/* TODO: Terminate existing sync and then sync to new?*/
		return -EALREADY;
	}

	LOG_DBG("Sync request");

	cached_pa_interval = pa_interval;
	cached_recv_state = recv_state;

	SET_FLAG(flag_pa_request);

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	if (pa_sync == NULL || recv_state->pa_sync_state == BT_BAP_PA_STATE_NOT_SYNCED) {
		return -EALREADY;
	}

	UNSET_FLAG(flag_pa_request);

	return 0;
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	cached_bis_sync_req = 0U;

	for (uint8_t i = 0U; i < recv_state->num_subgroups; i++) {
		cached_bis_sync_req |= bis_sync_req[i];
	}

	if (cached_bis_sync_req != 0U) {
		SET_FLAG(flag_bis_sync_requested);
	} else {
		UNSET_FLAG(flag_bis_sync_requested);
	}

	LOG_DBG("bis_sync_req 0x%08X", cached_bis_sync_req);

	cached_recv_state = recv_state;

	return 0;
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	LOG_DBG("Broadcast code received for %p", recv_state);

	if (memcmp(broadcast_code, BROADCAST_CODE, sizeof(BROADCAST_CODE)) != 0) {
		FAIL("Failed to receive correct broadcast code\n");
		return;
	}

	SET_FLAG(flag_broadcast_code);
}

static struct bt_bap_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_bap_stream *stream = bap_stream_from_audio_test_stream(&streams[i]);

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int unicast_server_config(struct bt_conn *conn, const struct bt_bap_ep *ep,
				 enum bt_audio_dir dir, const struct bt_audio_codec_cfg *codec_cfg,
				 struct bt_bap_stream **stream,
				 struct bt_bap_qos_cfg_pref *const pref,
				 struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Config: conn %p ep %p dir %u", conn, ep, dir);

	print_codec_cfg(codec_cfg);

	*stream = stream_alloc();
	if (*stream == NULL) {
		LOG_DBG("No streams available");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	LOG_DBG("ASE Codec Config stream %p", *stream);

	SET_FLAG(flag_stream_configured);

	*pref = unicast_qos_pref;

	return 0;
}

static int unicast_server_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
				   const struct bt_audio_codec_cfg *codec_cfg,
				   struct bt_bap_qos_cfg_pref *const pref,
				   struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", stream);

	print_codec_cfg(codec_cfg);

	*pref = unicast_qos_pref;

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int unicast_server_qos(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
			      struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("QoS: stream %p qos %p", stream, qos);

	print_qos(qos);

	return 0;
}

static bool ascs_data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data->type)) {
		LOG_DBG("Invalid metadata type %u or length %u", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);
		return false;
	}

	return true;
}

static int unicast_server_enable(struct bt_bap_stream *stream, const uint8_t meta[],
				 size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Enable: stream %p meta_len %zu", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, ascs_data_func_cb, rsp);
}

static int unicast_server_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Start: stream %p", stream);

	return 0;
}

static int unicast_server_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Metadata: stream %p meta_len %zu", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, ascs_data_func_cb, rsp);
}

static int unicast_server_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Disable: stream %p", stream);

	return 0;
}

static int unicast_server_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Stop: stream %p", stream);

	return 0;
}

static int unicast_server_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Release: stream %p", stream);

	return 0;
}

static void set_location(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_CENTER);
		if (err != 0) {
			FAIL("Failed to set sink location (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
					   BT_AUDIO_LOCATION_FRONT_LEFT |
						   BT_AUDIO_LOCATION_FRONT_RIGHT);
		if (err != 0) {
			FAIL("Failed to set source location (err %d)\n", err);
			return;
		}
	}

	LOG_DBG("Location successfully set");
}

static void set_supported_contexts(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, SINK_CONTEXT);
		if (err != 0) {
			FAIL("Failed to set sink supported contexts (err %d)\n", err);
			return;
		}

		LOG_DBG("Supported sink contexts successfully set to 0x%04X", SINK_CONTEXT);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, SOURCE_CONTEXT);
		if (err != 0) {
			FAIL("Failed to set source supported contexts (err %d)\n", err);
			return;
		}

		LOG_DBG("Supported source contexts successfully set to 0x%04X", SOURCE_CONTEXT);
	}
}

static void set_available_contexts(void)
{
	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		const int err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, SINK_CONTEXT);

		if (err != 0) {
			FAIL("Failed to set sink available contexts (err %d)\n", err);
			return;
		}

		LOG_DBG("Available sink contexts successfully set to 0x%04X", SINK_CONTEXT);
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		const int err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, SOURCE_CONTEXT);

		if (err != 0) {
			FAIL("Failed to set source available contexts (err %d)\n", err);
			return;
		}

		LOG_DBG("Available source contexts successfully set to 0x%04X", SOURCE_CONTEXT);
	}
}

static void test_start_adv(void)
{
	struct bt_le_ext_adv *ext_adv;

	setup_connectable_adv(&ext_adv);
}

static void register_callbacks(void)
{
	static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
		.base_recv = base_recv_cb,
		.syncable = syncable_cb,
		.started = broadcast_sink_started_cb,
		.stopped = broadcast_sink_stopped_cb,
	};
	static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
		.pa_sync_req = pa_sync_req_cb,
		.pa_sync_term_req = pa_sync_term_req_cb,
		.bis_sync_req = bis_sync_req_cb,
		.broadcast_code = broadcast_code_cb,
	};
	static struct bt_bap_unicast_server_cb unicast_server_cbs = {
		.config = unicast_server_config,
		.reconfig = unicast_server_reconfig,
		.qos = unicast_server_qos,
		.enable = unicast_server_enable,
		.start = unicast_server_start,
		.metadata = unicast_server_metadata,
		.disable = unicast_server_disable,
		.stop = unicast_server_stop,
		.release = unicast_server_release,
	};
	static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
		.synced = bap_pa_sync_synced_cb,
		.term = bap_pa_sync_terminated_cb,
	};
	static struct bt_bap_stream_ops stream_ops = {
		.enabled = stream_enabled_cb,
		.started = stream_started_cb,
		.stopped = stream_stopped_cb,
		.recv = bap_stream_rx_recv_cb,
	};
	int err;

	err = bt_bap_unicast_server_register_cb(&unicast_server_cbs);
	if (err != 0) {
		FAIL("Failed to register unicast server callbacks (err %d)\n", err);

		return;
	}

	err = bt_bap_scan_delegator_register(&scan_delegator_cbs);
	if (err != 0) {
		FAIL("Scan deligator register failed (err %d)\n", err);

		return;
	}

	err = bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	if (err != 0) {
		FAIL("Scan deligator register failed (err %d)\n", err);

		return;
	}

	err = bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	if (err != 0) {
		FAIL("Scan deligator register failed (err %d)\n", err);

		return;
	}

	ARRAY_FOR_EACH_PTR(streams, stream) {
		bt_cap_stream_ops_register(cap_stream_from_audio_test_stream(stream), &stream_ops);
	}
}

static void init(void)
{
	static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_ANY,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));
	const struct bt_pacs_register_param pacs_param = {
#if defined(CONFIG_BT_PAC_SNK)
		.snk_pac = true,
#endif /* CONFIG_BT_PAC_SNK */
#if defined(CONFIG_BT_PAC_SNK_LOC)
		.snk_loc = true,
#endif /* CONFIG_BT_PAC_SNK_LOC */
#if defined(CONFIG_BT_PAC_SRC)
		.src_pac = true,
#endif /* CONFIG_BT_PAC_SRC */
#if defined(CONFIG_BT_PAC_SRC_LOC)
		.src_loc = true,
#endif /* CONFIG_BT_PAC_SRC_LOC */
	};
	const struct bt_csip_set_member_register_param csip_set_member_param = {
		.set_size = 3,
		.rank = 1,
		.lockable = true,
		.sirk = TEST_SAMPLE_SIRK,
	};
	static struct bt_bap_unicast_server_register_param param = {
		.snk_cnt = CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
		.src_cnt = CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
	};
	static struct bt_csip_set_member_svc_inst *csip_set_member;
	static struct bt_pacs_cap pacs_cap = {
		.codec_cap = &codec_cap,
	};

	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_pacs_register(&pacs_param);
	if (err) {
		FAIL("Could not register PACS (err %d)\n", err);
		return;
	}

	err = bt_cap_acceptor_register(&csip_set_member_param, &csip_set_member);
	if (err != 0) {
		FAIL("CAP acceptor failed to register (err %d)\n", err);
		return;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &pacs_cap);
	if (err != 0) {
		FAIL("Broadcast capability register failed (err %d)\n", err);

		return;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &pacs_cap);
	if (err != 0) {
		FAIL("Broadcast capability register failed (err %d)\n", err);

		return;
	}

	err = bt_bap_unicast_server_register(&param);
	if (err != 0) {
		FAIL("Failed to register unicast server (err %d)\n", err);

		return;
	}

	set_supported_contexts();
	set_available_contexts();
	set_location();

	register_callbacks();
}

static void pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};
	int err;

	bt_addr_le_copy(&create_params.addr, &cached_recv_state->addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_NONE;
	create_params.sid = cached_recv_state->adv_sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(cached_pa_interval);

	err = bt_le_per_adv_sync_create(&create_params, &pa_sync);
	if (err != 0) {
		FAIL("Could not create Broadcast PA sync: %d\n", err);
		return;
	}

	LOG_DBG("Waiting for PA sync");
	WAIT_FOR_FLAG(flag_pa_synced);
}

static void create_and_sync_sink(void)
{
	struct bt_bap_stream *broadcast_streams[ARRAY_SIZE(streams)];
	int err;

	LOG_DBG("Creating the broadcast sink");
	err = bt_bap_broadcast_sink_create(pa_sync, cached_recv_state->broadcast_id,
					   &broadcast_sink);
	if (err != 0) {
		FAIL("Unable to create the sink: %d\n", err);
		return;
	}

	LOG_DBG("Broadcast source PA synced, waiting for BASE");
	WAIT_FOR_FLAG(flag_base_received);
	LOG_DBG("BASE received");

	LOG_DBG("Waiting for BIG syncable");
	WAIT_FOR_FLAG(flag_syncable);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		broadcast_streams[i] = bap_stream_from_audio_test_stream(&streams[i]);
	}

	if (cached_bis_sync_req == 0U) {
		FAIL("Invalid cached_bis_sync_req: %u", cached_bis_sync_req);
		return;
	}

	LOG_DBG("Syncing the sink to 0x%08x", cached_bis_sync_req);

	err = bt_bap_broadcast_sink_sync(broadcast_sink, cached_bis_sync_req, broadcast_streams,
					 NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	LOG_DBG("Waiting for broadcast streams to be started");
	WAIT_FOR_FLAG(flag_broadcast_started);
}

static void wait_for_data(void)
{
	UNSET_FLAG(flag_audio_received);

	LOG_DBG("Waiting for data");
	WAIT_FOR_FLAG(flag_audio_received);
	LOG_DBG("Data received");
}

static void test_cap_handover_peripheral_unicast_to_broadcast(void)
{
	init();

	test_start_adv();

	WAIT_FOR_FLAG(flag_connected);

	/* Wait until initiator is done starting streams */
	WAIT_FOR_FLAG(flag_stream_started);
	backchannel_sync_wait(CAP_INITIATOR_DEV_ID);

	wait_for_data();

	/* let initiator know we have received what we wanted */
	backchannel_sync_send(CAP_INITIATOR_DEV_ID);

	/* Wait for unicast to be stopped */
	WAIT_FOR_FLAG(flag_stream_stopped);

	/* Wait for a PA sync request to switch from unicast to broadcast */
	LOG_DBG("Waiting for PA sync request");
	WAIT_FOR_FLAG(flag_pa_request);
	pa_sync_create();

	/* Wait for a BIG sync request to sync to broadcast */
	WAIT_FOR_FLAG(flag_bis_sync_requested);
	create_and_sync_sink();

	wait_for_data();

	/* let initiator know we have received what we wanted */
	backchannel_sync_send(CAP_INITIATOR_DEV_ID);

	/* Wait for broadcast to be stopped */
	WAIT_FOR_FLAG(flag_broadcast_stopped);

	PASS("CAP acceptor unicast passed\n");
}

static const struct bst_test_instance test_cap_handover_peripheral[] = {
	{
		.test_id = "cap_handover_peripheral_unicast_to_broadcast",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_handover_peripheral_unicast_to_broadcast,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_handover_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_handover_peripheral);
}

#else /* !(CONFIG_BT_CAP_ACCEPTOR) */

struct bst_test_list *test_cap_handover_peripheral_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_ACCEPTOR */
