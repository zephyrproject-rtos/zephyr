/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bstests.h"
#include "common.h"
#include "bap_common.h"

#if defined(CONFIG_BT_CAP_ACCEPTOR)
extern enum bst_result_t bst_result;

#define SINK_CONTEXT                                                                               \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA |                         \
	 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)
#define SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS)

CREATE_FLAG(flag_broadcaster_found);
CREATE_FLAG(flag_broadcast_code);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(flag_received);
CREATE_FLAG(flag_pa_sync_lost);
CREATE_FLAG(flag_pa_request);
CREATE_FLAG(flag_bis_sync_requested);
CREATE_FLAG(flag_base_metadata_updated);
CREATE_FLAG(flag_unicast_stream_configured);

static struct bt_bap_broadcast_sink *g_broadcast_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct audio_test_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static const struct bt_bap_qos_cfg_pref unicast_qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u, 20000u, 40000u);

static bool auto_start_sink_streams;

static K_SEM_DEFINE(sem_broadcast_started, 0U, ARRAY_SIZE(broadcast_sink_streams));
static K_SEM_DEFINE(sem_broadcast_stopped, 0U, ARRAY_SIZE(broadcast_sink_streams));

static uint32_t bis_index_bitfield;

#define UNICAST_CHANNEL_COUNT_1 BIT(0)

static struct bt_cap_stream unicast_streams[CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT +
					    CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT];

static bool subgroup_data_func_cb(struct bt_data *data, void *user_data)
{
	bool *stream_context_found = (bool *)user_data;

	printk("type %u len %u\n", data->type, data->data_len);

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

	if (TEST_FLAG(flag_base_received) &&
	    ((size_t)ret != metadata_size || memcmp(meta, metadata, metadata_size) != 0)) {
		printk("Metadata updated\n");
		SET_FLAG(flag_base_metadata_updated);
	}

	metadata_size = (size_t)ret;

	ret = bt_audio_data_parse(meta, (size_t)ret, subgroup_data_func_cb, &stream_context_found);
	if (ret != 0 && ret != -ECANCELED) {
		return false;
	}

	if (!stream_context_found) {
		printk("Subgroup did not have streaming context\n");
	}

	/* if this is false, the iterator will return early with an error */
	return stream_context_found;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	/* Create a mask for the maximum BIS we can sync to using the number of
	 * broadcast_sink_streams we have. We add an additional 1 since the bis indexes
	 * start from 1 and not 0.
	 */
	const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(broadcast_sink_streams) + 1U);
	uint32_t base_bis_index_bitfield = 0U;
	int ret;

	ret = bt_bap_base_get_subgroup_count(base);
	if (ret < 0) {
		FAIL("Failed to get subgroup count: %d\n", ret);
		return;
	}

	printk("Received BASE with %d subgroups from broadcast sink %p\n", ret, sink);

	if (ret == 0) {
		FAIL("subgroup_count was 0");
		return;
	}

	ret = bt_bap_base_foreach_subgroup(base, valid_subgroup_metadata_cb, NULL);
	if (ret != 0) {
		FAIL("Failed to parse subgroups: %d\n", ret);
		return;
	}

	ret = bt_bap_base_get_bis_indexes(base, &base_bis_index_bitfield);
	if (ret != 0) {
		FAIL("Failed to BIS indexes: %d\n", ret);
		return;
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	SET_FLAG(flag_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, biginfo->encryption ? "" : "out");
	SET_FLAG(flag_syncable);
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (TEST_FLAG(flag_broadcaster_found)) {
		/* no-op*/
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

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Found broadcaster with ID 0x%06X and addr %s and sid 0x%02X\n", broadcast_id,
	       le_addr, info->sid);

	SET_FLAG(flag_broadcaster_found);

	/* Store info for PA sync parameters */
	memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
	bt_addr_le_copy(&broadcaster_addr, info->addr);
	broadcaster_broadcast_id = broadcast_id;

	/* Stop parsing */
	return false;
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (info->interval != 0U) {
		bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
	}
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       broadcaster_broadcast_id);

		SET_FLAG(flag_pa_synced);
	}
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p lost with reason %u\n", sync, info->reason);
		pa_sync = NULL;

		SET_FLAG(flag_pa_sync_lost);
	}
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static void started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_broadcast_started);
}

static void stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_broadcast_stopped);
}

static void recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	if ((test_stream->rx_cnt % 50U) == 0U) {
		printk("[%zu]: Incoming audio on stream %p len %u and ts %u\n", test_stream->rx_cnt,
		       stream, buf->len, info->ts);
	}

	if (test_stream->rx_cnt > 0U && info->ts == test_stream->last_info.ts) {
		FAIL("Duplicated timestamp received: %u\n", test_stream->last_info.ts);
		return;
	}

	if (test_stream->rx_cnt > 0U && info->seq_num == test_stream->last_info.seq_num) {
		FAIL("Duplicated PSN received: %u\n", test_stream->last_info.seq_num);
		return;
	}

	if (info->flags & BT_ISO_FLAGS_ERROR) {
		/* Fail the test if we have not received what we expected */
		if (!TEST_FLAG(flag_received)) {
			FAIL("ISO receive error\n");
		}

		return;
	}

	if (info->flags & BT_ISO_FLAGS_LOST) {
		FAIL("ISO receive lost\n");
		return;
	}

	if (memcmp(buf->data, mock_iso_data, buf->len) == 0) {
		test_stream->rx_cnt++;

		if (test_stream->rx_cnt >= MIN_SEND_COUNT) {
			/* We set the flag is just one stream has received the expected */
			SET_FLAG(flag_received);
		}
	} else {
		FAIL("Unexpected data received\n");
	}
}

static struct bt_bap_stream_ops broadcast_stream_ops = {
	.started = started_cb, .stopped = stopped_cb, .recv = recv_cb};

static void unicast_stream_enabled_cb(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	printk("Enabled: stream %p (auto_start_sink_streams %d)\n", stream,
	       auto_start_sink_streams);

	err = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (err != 0) {
		FAIL("Failed to get ep info: %d\n", err);
		return;
	}

	if (auto_start_sink_streams && ep_info.dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		err = bt_bap_stream_start(stream);

		if (err != 0) {
			FAIL("Failed to start stream: %d\n", err);
			return;
		}
	}
}

static struct bt_bap_stream_ops unicast_stream_ops = {
	.enabled = unicast_stream_enabled_cb,
};

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

	printk("Sync request\n");

	bt_addr_le_copy(&broadcaster_addr, &recv_state->addr);
	broadcaster_info.sid = recv_state->adv_sid;
	broadcaster_info.interval = pa_interval;

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
	/* We only care about a single subgroup in this test */
	broadcaster_broadcast_id = recv_state->broadcast_id;
	if (bis_sync_req[0] != 0) {
		SET_FLAG(flag_bis_sync_requested);
	} else {
		UNSET_FLAG(flag_bis_sync_requested);
	}

	return 0;
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	printk("Broadcast code received for %p\n", recv_state);

	SET_FLAG(flag_broadcast_code);
}

static struct bt_bap_scan_delegator_cb scan_delegator_cbs = {
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.bis_sync_req = bis_sync_req_cb,
	.broadcast_code = broadcast_code_cb,
};

/* TODO: Expand with CAP service data */
static const struct bt_data cap_acceptor_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_CAS_VAL)),
};

static struct bt_csip_set_member_svc_inst *csip_set_member;

static struct bt_bap_stream *unicast_stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_bap_stream *stream = &unicast_streams[i].bap_stream;

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
	printk("ASE Codec Config: conn %p ep %p dir %u\n", conn, ep, dir);

	print_codec_cfg(codec_cfg);

	*stream = unicast_stream_alloc();
	if (*stream == NULL) {
		printk("No streams available\n");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);

		return -ENOMEM;
	}

	printk("ASE Codec Config stream %p\n", *stream);

	SET_FLAG(flag_unicast_stream_configured);

	*pref = unicast_qos_pref;

	return 0;
}

static int unicast_server_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
				   const struct bt_audio_codec_cfg *codec_cfg,
				   struct bt_bap_qos_cfg_pref *const pref,
				   struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);

	print_codec_cfg(codec_cfg);

	*pref = unicast_qos_pref;

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int unicast_server_qos(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
			      struct bt_bap_ascs_rsp *rsp)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int unicast_server_enable(struct bt_bap_stream *stream, const uint8_t meta[],
				 size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_len %zu\n", stream, meta_len);

	return 0;
}

static int unicast_server_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static bool ascs_data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data->type)) {
		printk("Invalid metadata type %u or length %u\n", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);
		return false;
	}

	return true;
}

static int unicast_server_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_len %zu\n", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, ascs_data_func_cb, rsp);
}

static int unicast_server_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Disable: stream %p\n", stream);

	return 0;
}

static int unicast_server_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Stop: stream %p\n", stream);

	return 0;
}

static int unicast_server_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Release: stream %p\n", stream);

	return 0;
}

static struct bt_bap_unicast_server_register_param param = {
	CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
	CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
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

static void set_location(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					   BT_AUDIO_LOCATION_FRONT_CENTER);
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

	printk("Location successfully set\n");
}

static int set_supported_contexts(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, SINK_CONTEXT);
		if (err != 0) {
			printk("Failed to set sink supported contexts (err %d)\n",
			       err);

			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, SOURCE_CONTEXT);
		if (err != 0) {
			printk("Failed to set source supported contexts (err %d)\n",
			       err);

			return err;
		}
	}

	printk("Supported contexts successfully set\n");

	return 0;
}

void test_start_adv(void)
{
	int err;
	struct bt_le_ext_adv *ext_adv;

	/* Create a connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_FAST_1, NULL, &ext_adv);
	if (err != 0) {
		FAIL("Failed to create advertising set (err %d)\n", err);

		return;
	}

	/* Add cap acceptor advertising data */
	err = bt_le_ext_adv_set_data(ext_adv, cap_acceptor_ad, ARRAY_SIZE(cap_acceptor_ad), NULL,
				     0);
	if (err != 0) {
		FAIL("Failed to set advertising data (err %d)\n", err);

		return;
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		return;
	}
}

static void set_available_contexts(void)
{
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, SINK_CONTEXT);
	if (IS_ENABLED(CONFIG_BT_PAC_SNK) && err != 0) {
		FAIL("Failed to set sink available contexts (err %d)\n", err);
		return;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, SOURCE_CONTEXT);
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && err != 0) {
		FAIL("Failed to set source available contexts (err %d)\n", err);
		return;
	}

	printk("Available contexts successfully set\n");
}

static void init(void)
{
	const struct bt_csip_set_member_register_param csip_set_member_param = {
		.set_size = 3,
		.rank = 1,
		.lockable = true,
		/* Using the CSIP_SET_MEMBER test sample SIRK */
		.sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
	};
	static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_ANY,
		BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
		(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		err = bt_cap_acceptor_register(&csip_set_member_param, &csip_set_member);
		if (err != 0) {
			FAIL("CAP acceptor failed to register (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER)) {
		static struct bt_pacs_cap unicast_cap = {
			.codec_cap = &codec_cap,
		};

		err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &unicast_cap);
		if (err != 0) {
			FAIL("Broadcast capability register failed (err %d)\n",
			     err);

			return;
		}

		err = bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &unicast_cap);
		if (err != 0) {
			FAIL("Broadcast capability register failed (err %d)\n", err);

			return;
		}

		err = bt_bap_unicast_server_register(&param);
		if (err != 0) {
			FAIL("Failed to register unicast server (err %d)\n", err);

			return;
		}

		err = bt_bap_unicast_server_register_cb(&unicast_server_cbs);
		if (err != 0) {
			FAIL("Failed to register unicast server callbacks (err %d)\n",
			     err);

			return;
		}

		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			bt_cap_stream_ops_register(&unicast_streams[i], &unicast_stream_ops);
		}

		err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, cap_acceptor_ad,
				      ARRAY_SIZE(cap_acceptor_ad), NULL, 0);
		if (err != 0) {
			FAIL("Advertising failed to start (err %d)\n", err);
			return;
		}
		test_start_adv();
	}

	if (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK)) {
		static struct bt_pacs_cap broadcast_cap = {
			.codec_cap = &codec_cap,
		};

		err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &broadcast_cap);
		if (err != 0) {
			FAIL("Broadcast capability register failed (err %d)\n",
			     err);

			return;
		}

		err = bt_bap_scan_delegator_register(&scan_delegator_cbs);
		if (err != 0) {
			FAIL("Scan deligator register failed (err %d)\n", err);

			return;
		}

		bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
		bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
		bt_le_scan_cb_register(&bap_scan_cb);

		UNSET_FLAG(flag_broadcaster_found);
		UNSET_FLAG(flag_broadcast_code);
		UNSET_FLAG(flag_base_received);
		UNSET_FLAG(flag_pa_synced);
		UNSET_FLAG(flag_pa_request);
		UNSET_FLAG(flag_received);
		UNSET_FLAG(flag_base_metadata_updated);

		for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
			bt_cap_stream_ops_register(
				cap_stream_from_audio_test_stream(&broadcast_sink_streams[i]),
				&broadcast_stream_ops);
		}
	}

	if (IS_ENABLED(CONFIG_BT_PACS)) {
		set_supported_contexts();
		set_available_contexts();
		set_location();
	}

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND)) {
		char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT][16];
		char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT][16];
		struct bt_vcp_vol_rend_register_param vcp_param = {0};

		for (size_t i = 0U; i < ARRAY_SIZE(vcp_param.vocs_param); i++) {
			vcp_param.vocs_param[i].location_writable = true;
			vcp_param.vocs_param[i].desc_writable = true;
			snprintf(output_desc[i], sizeof(output_desc[i]), "Output %d", i + 1);
			vcp_param.vocs_param[i].output_desc = output_desc[i];
			vcp_param.vocs_param[i].cb = NULL;
		}

		for (size_t i = 0U; i < ARRAY_SIZE(vcp_param.aics_param); i++) {
			vcp_param.aics_param[i].desc_writable = true;
			snprintf(input_desc[i], sizeof(input_desc[i]), "VCP Input %d", i + 1);
			vcp_param.aics_param[i].description = input_desc[i];
			vcp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
			vcp_param.aics_param[i].status = true;
			vcp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
			vcp_param.aics_param[i].units = 1;
			vcp_param.aics_param[i].min_gain = 0;
			vcp_param.aics_param[i].max_gain = 100;
			vcp_param.aics_param[i].cb = NULL;
		}

		vcp_param.step = 1;
		vcp_param.mute = BT_VCP_STATE_UNMUTED;
		vcp_param.volume = 100;
		vcp_param.cb = NULL;
		err = bt_vcp_vol_rend_register(&vcp_param);
		if (err != 0) {
			FAIL("Failed to register VCS (err %d)\n", err);

			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MICP_MIC_DEV)) {
		struct bt_micp_mic_dev_register_param micp_param = {0};

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
		char input_desc[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT][16];

		for (int i = 0; i < ARRAY_SIZE(micp_param.aics_param); i++) {
			micp_param.aics_param[i].desc_writable = true;
			snprintf(input_desc[i], sizeof(input_desc[i]), "MICP Input %d", i + 1);
			micp_param.aics_param[i].description = input_desc[i];
			micp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
			micp_param.aics_param[i].status = true;
			micp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
			micp_param.aics_param[i].units = 1;
			micp_param.aics_param[i].min_gain = 0;
			micp_param.aics_param[i].max_gain = 100;
			micp_param.aics_param[i].cb = NULL;
		}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

		err = bt_micp_mic_dev_register(&micp_param);
		if (err != 0) {
			FAIL("Failed to register MICS (err %d)\n", err);
			return;
		}
	}
}

static void test_cap_acceptor_unicast(void)
{
	init();

	auto_start_sink_streams = true;

	/* TODO: wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);

	PASS("CAP acceptor unicast passed\n");
}

static void test_cap_acceptor_unicast_timeout(void)
{
	init();

	auto_start_sink_streams = false; /* Cause unicast_audio_start timeout */

	/* TODO: wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);

	PASS("CAP acceptor unicast passed\n");
}

static void pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};
	int err;

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	err = bt_le_per_adv_sync_create(&create_params, &pa_sync);
	if (err != 0) {
		FAIL("Could not create Broadcast PA sync: %d\n", err);
		return;
	}

	printk("Broadcast source found, waiting for PA sync\n");
	WAIT_FOR_FLAG(flag_pa_synced);
}

static void pa_sync_to_broadcaster(void)
{
	int err;

	printk("Scanning for broadcast sources\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_broadcaster_found);

	printk("Broadcast source found, stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("bt_le_scan_stop failed with %d\n", err);
		return;
	}

	printk("Scan stopped, attempting to PA sync to the broadcaster with id 0x%06X\n",
	       broadcaster_broadcast_id);

	pa_sync_create();
}

static void create_and_sync_sink(struct bt_bap_stream *bap_streams[], size_t *stream_count)
{
	int err;

	printk("Creating the broadcast sink\n");
	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, &g_broadcast_sink);
	if (err != 0) {
		FAIL("Unable to create the sink: %d\n", err);
		return;
	}

	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(flag_base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		bap_streams[i] = bap_stream_from_audio_test_stream(&broadcast_sink_streams[i]);
	}

	printk("Syncing the sink\n");
	*stream_count = 0;
	for (int i = 1; i < BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
		if ((bis_index_bitfield & BIT(i)) != 0) {
			*stream_count += 1;
		}
	}

	err = bt_bap_broadcast_sink_sync(g_broadcast_sink, bis_index_bitfield, bap_streams, NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for %zu streams to be started\n", *stream_count);
	for (size_t i = 0U; i < *stream_count; i++) {
		k_sem_take(&sem_broadcast_started, K_FOREVER);
	}
}

static void sink_wait_for_data(void)
{
	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);
	backchannel_sync_send_all(); /* let other devices know we have received what we wanted */
}

static void base_wait_for_metadata_update(void)
{
	printk("Waiting for meta update\n");
	WAIT_FOR_FLAG(flag_base_metadata_updated);
	backchannel_sync_send_all(); /* let others know we have received a metadata update */
}

static void wait_for_streams_stop(int stream_count)
{
	/* The order of PA sync lost and BIG Sync lost is irrelevant
	 * and depend on timeout parameters. We just wait for PA first, but
	 * either way will work.
	 */
	printk("Waiting for PA disconnected\n");
	WAIT_FOR_FLAG(flag_pa_sync_lost);

	printk("Waiting for %zu streams to be stopped\n", stream_count);
	for (size_t i = 0U; i < stream_count; i++) {
		k_sem_take(&sem_broadcast_stopped, K_FOREVER);
	}
}

static void test_cap_acceptor_broadcast(void)
{
	static struct bt_bap_stream *bap_streams[ARRAY_SIZE(broadcast_sink_streams)];
	size_t stream_count;

	init();

	pa_sync_to_broadcaster();

	create_and_sync_sink(bap_streams, &stream_count);

	sink_wait_for_data();

	wait_for_streams_stop(stream_count);

	PASS("CAP acceptor broadcast passed\n");
}

static void test_cap_acceptor_broadcast_reception(void)
{
	static struct bt_bap_stream *bap_streams[ARRAY_SIZE(broadcast_sink_streams)];
	size_t stream_count;

	init();

	WAIT_FOR_FLAG(flag_pa_request);

	pa_sync_create();

	create_and_sync_sink(bap_streams, &stream_count);

	sink_wait_for_data();

	/* Since we are re-using the BAP broadcast source test
	 * we get a metadata update, and we need to send an extra
	 * backchannel sync
	 */
	base_wait_for_metadata_update();

	backchannel_sync_send_all(); /* let broadcaster know we can stop the source */

	wait_for_streams_stop(stream_count);

	PASS("CAP acceptor broadcast reception passed\n");
}

static void test_cap_acceptor_capture_and_render(void)
{
	init();

	WAIT_FOR_FLAG(flag_connected);

	PASS("CAP acceptor unicast passed\n");
}

static const struct bst_test_instance test_cap_acceptor[] = {
	{
		.test_id = "cap_acceptor_unicast",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_unicast,
	},
	{
		.test_id = "cap_acceptor_unicast_timeout",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_unicast_timeout,
	},
	{
		.test_id = "cap_acceptor_broadcast",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_broadcast,
	},
	{
		.test_id = "cap_acceptor_broadcast_reception",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_broadcast_reception,
	},
	{
		.test_id = "cap_acceptor_capture_and_render",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_capture_and_render,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_acceptor_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_acceptor);
}

#else /* !(CONFIG_BT_CAP_ACCEPTOR) */

struct bst_test_list *test_cap_acceptor_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_ACCEPTOR */
