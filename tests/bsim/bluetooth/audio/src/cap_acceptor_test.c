/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_ACCEPTOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>
#include "common.h"
#include "bap_unicast_common.h"

extern enum bst_result_t bst_result;

#define SINK_CONTEXT                                                                               \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA |                         \
	 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)
#define SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS)

CREATE_FLAG(flag_broadcaster_found);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(flag_received);
CREATE_FLAG(flag_pa_sync_lost);

static struct bt_bap_broadcast_sink *g_broadcast_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct bt_cap_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_LC3_FREQ_ANY, BT_AUDIO_CODEC_LC3_DURATION_ANY,
	BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
	(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static const struct bt_audio_codec_qos_pref unicast_qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u, 20000u, 40000u);

static bool auto_start_sink_streams;

static K_SEM_DEFINE(sem_broadcast_started, 0U, ARRAY_SIZE(broadcast_sink_streams));
static K_SEM_DEFINE(sem_broadcast_stopped, 0U, ARRAY_SIZE(broadcast_sink_streams));

/* Create a mask for the maximum BIS we can sync to using the number of
 * broadcast_sink_streams we have. We add an additional 1 since the bis indexes
 * start from 1 and not 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(broadcast_sink_streams) + 1U);
static uint32_t bis_index_bitfield;

#define UNICAST_CHANNEL_COUNT_1 BIT(0)

static struct bt_cap_stream unicast_streams[CONFIG_BT_ASCS_ASE_SNK_COUNT +
					    CONFIG_BT_ASCS_ASE_SRC_COUNT];

CREATE_FLAG(flag_unicast_stream_configured);

static bool valid_metadata_type(uint8_t type, uint8_t len)
{
	switch (type) {
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT:
		if (len != 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_STREAM_LANG:
		if (len != 3) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (len != 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 2 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR:   /* 2 - 255 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST:
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO:     /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

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

static bool valid_subgroup_metadata(const struct bt_bap_base_subgroup *subgroup)
{
	bool stream_context_found = false;
	int err;

	printk("meta %p len %zu", subgroup->codec_cfg.meta, subgroup->codec_cfg.meta_len);

	err = bt_audio_data_parse(subgroup->codec_cfg.meta, subgroup->codec_cfg.meta_len,
				  subgroup_data_func_cb, &stream_context_found);
	if (err != 0 && err != -ECANCELED) {
		return false;
	}

	if (!stream_context_found) {
		printk("Subgroup did not have streaming context\n");
	}

	return stream_context_found;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base)
{
	uint32_t base_bis_index_bitfield = 0U;

	if (TEST_FLAG(flag_base_received)) {
		return;
	}

	printk("Received BASE with %u subgroups from broadcast sink %p\n",
	       base->subgroup_count, sink);

	if (base->subgroup_count == 0) {
		FAIL("base->subgroup_count was 0");
		return;
	}


	for (size_t i = 0U; i < base->subgroup_count; i++) {
		const struct bt_bap_base_subgroup *subgroup = &base->subgroups[i];

		for (size_t j = 0U; j < subgroup->bis_count; j++) {
			const uint8_t index = subgroup->bis_data[j].index;

			base_bis_index_bitfield |= BIT(index);
		}

		if (!valid_subgroup_metadata(subgroup)) {
			FAIL("Subgroup[%zu] has invalid metadata\n", i);
			return;
		}
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	SET_FLAG(flag_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, bool encrypted)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, encrypted ? "" : "out");
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
	SET_FLAG(flag_received);
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
				 struct bt_audio_codec_qos_pref *const pref,
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
				   struct bt_audio_codec_qos_pref *const pref,
				   struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);

	print_codec_cfg(codec_cfg);

	*pref = unicast_qos_pref;

	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int unicast_server_qos(struct bt_bap_stream *stream, const struct bt_audio_codec_qos *qos,
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
		.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
	};

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

		err = bt_bap_unicast_server_register_cb(&unicast_server_cbs);
		if (err != 0) {
			FAIL("Failed to register unicast server callbacks (err %d)\n",
			     err);

			return;
		}

		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			bt_cap_stream_ops_register(&unicast_streams[i], &unicast_stream_ops);
		}

		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, cap_acceptor_ad,
				      ARRAY_SIZE(cap_acceptor_ad), NULL, 0);
		if (err != 0) {
			FAIL("Advertising failed to start (err %d)\n", err);
			return;
		}
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

		bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
		bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
		bt_le_scan_cb_register(&bap_scan_cb);

		UNSET_FLAG(flag_broadcaster_found);
		UNSET_FLAG(flag_base_received);
		UNSET_FLAG(flag_pa_synced);

		for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
			bt_cap_stream_ops_register(&broadcast_sink_streams[i],
						   &broadcast_stream_ops);
		}
	}

	set_supported_contexts();
	set_available_contexts();
	set_location();
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

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		/* Ensure that the following calculation does not overflow silently */
		__ASSERT(SYNC_RETRY_COUNT < 10, "SYNC_RETRY_COUNT shall be less than 10");

		/* Add retries and convert to unit in 10's of ms */
		pa_timeout = ((uint32_t)pa_interval * SYNC_RETRY_COUNT) / 10;

		/* Enforce restraints */
		pa_timeout =
			CLAMP(pa_timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}

static void test_cap_acceptor_broadcast(void)
{
	static struct bt_bap_stream *bap_streams[ARRAY_SIZE(broadcast_sink_streams)];
	int err;

	init();

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
	err = pa_sync_create();
	if (err != 0) {
		FAIL("Could not create Broadcast PA sync: %d\n", err);
		return;
	}

	printk("Broadcast source found, waiting for PA sync\n");
	WAIT_FOR_FLAG(flag_pa_synced);

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
		bap_streams[i] = &broadcast_sink_streams[i].bap_stream;
	}

	printk("Syncing the sink\n");
	err = bt_bap_broadcast_sink_sync(g_broadcast_sink, bis_index_bitfield, bap_streams, NULL);
	if (err != 0) {
		FAIL("Unable to sync the sink: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for broadcast_sink_streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		k_sem_take(&sem_broadcast_started, K_FOREVER);
	}

	printk("Waiting for data\n");
	WAIT_FOR_FLAG(flag_received);

	/* The order of PA sync lost and BIG Sync lost is irrelevant
	 * and depend on timeout parameters. We just wait for PA first, but
	 * either way will work.
	 */
	printk("Waiting for PA disconnected\n");
	WAIT_FOR_FLAG(flag_pa_sync_lost);

	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		k_sem_take(&sem_broadcast_stopped, K_FOREVER);
	}

	PASS("CAP acceptor broadcast passed\n");
}

static const struct bst_test_instance test_cap_acceptor[] = {
	{
		.test_id = "cap_acceptor_unicast",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_unicast,
	},
	{
		.test_id = "cap_acceptor_unicast_timeout",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_unicast_timeout,
	},
	{
		.test_id = "cap_acceptor_broadcast",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_broadcast,
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
