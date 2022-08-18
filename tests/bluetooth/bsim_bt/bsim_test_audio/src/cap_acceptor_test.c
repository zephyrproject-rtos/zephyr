/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_ACCEPTOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_broadcaster_found);
CREATE_FLAG(flag_base_received);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_syncable);
CREATE_FLAG(flag_received);
CREATE_FLAG(flag_pa_sync_lost);

static struct bt_audio_broadcast_sink *g_broadcast_sink;
static struct bt_cap_stream broadcast_sink_streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];

static K_SEM_DEFINE(sem_broadcast_started, 0U, ARRAY_SIZE(broadcast_sink_streams));
static K_SEM_DEFINE(sem_broadcast_stopped, 0U, ARRAY_SIZE(broadcast_sink_streams));

/* Create a mask for the maximum BIS we can sync to using the number of
 * broadcast_sink_streams we have. We add an additional 1 since the bis indexes
 * start from 1 and not 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(broadcast_sink_streams) + 1U);
static uint32_t bis_index_bitfield;

#define UNICAST_CHANNEL_COUNT_1 BIT(0)

static struct bt_codec unicast_lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_16KHZ, BT_CODEC_LC3_DURATION_10,
		     UNICAST_CHANNEL_COUNT_1, 40u, 40u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_audio_stream unicast_streams[CONFIG_BT_ASCS_ASE_SNK_COUNT +
					      CONFIG_BT_ASCS_ASE_SRC_COUNT];

static struct bt_csis *csis;

/* TODO: Expand with CAP service data */
static const struct bt_data cap_acceptor_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_CAS_VAL)),
};

CREATE_FLAG(flag_unicast_connected);
CREATE_FLAG(flag_unicast_stream_configured);

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info,
			 struct net_buf_simple *ad,
			 uint32_t broadcast_id)
{
	SET_FLAG(flag_broadcaster_found);

	return true;
}

static void scan_term_cb(int err)
{
	if (err != 0) {
		FAIL("Scan terminated with error: %d", err);
	}
}

static void pa_synced_cb(struct bt_audio_broadcast_sink *sink,
			 struct bt_le_per_adv_sync *sync,
			 uint32_t broadcast_id)
{
	if (g_broadcast_sink != NULL) {
		FAIL("Unexpected PA sync");
		return;
	}

	printk("PA synced for broadcast sink %p with broadcast ID 0x%06X\n",
	       sink, broadcast_id);

	g_broadcast_sink = sink;

	SET_FLAG(flag_pa_synced);
}

static bool valid_subgroup_metadata(const struct bt_audio_base_subgroup *subgroup)
{
	bool stream_context_found = false;

	for (size_t j = 0U; j < subgroup->codec.meta_count; j++) {
		const struct bt_data *metadata = &subgroup->codec.meta[j].data;

		if (metadata->type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			if (metadata->data_len != 2) { /* Stream context size */
				printk("Subgroup has invalid streaming context length: %u\n",
				       metadata->data_len);
				return false;
			}

			stream_context_found = true;
			break;
		}
	}

	if (!stream_context_found) {
		printk("Subgroup did not have streaming context\n");
		return false;
	}

	return true;
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink,
			 const struct bt_audio_base *base)
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
		const struct bt_audio_base_subgroup *subgroup = &base->subgroups[i];

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

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	printk("Broadcast sink %p syncable with%s encryption\n",
	       sink, encrypted ? "" : "out");
	SET_FLAG(flag_syncable);
}

static void pa_sync_lost_cb(struct bt_audio_broadcast_sink *sink)
{
	if (g_broadcast_sink == NULL) {
		FAIL("Unexpected PA sync lost");
		return;
	}

	printk("Sink %p disconnected\n", sink);

	SET_FLAG(flag_pa_sync_lost);

	g_broadcast_sink = NULL;
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cbs = {
	.scan_recv = scan_recv_cb,
	.scan_term = scan_term_cb,
	.base_recv = base_recv_cb,
	.pa_synced = pa_synced_cb,
	.syncable = syncable_cb,
	.pa_sync_lost = pa_sync_lost_cb
};

static void started_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_broadcast_started);
}

static void stopped_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p stopped\n", stream);
	k_sem_give(&sem_broadcast_stopped);
}

static void recv_cb(struct bt_audio_stream *stream,
		    const struct bt_iso_recv_info *info,
		    struct net_buf *buf)
{
	SET_FLAG(flag_received);
}

static struct bt_audio_stream_ops broadcast_stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.recv = recv_cb
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	SET_FLAG(flag_unicast_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					enum bt_audio_dir dir,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p dir %u, cap %p\n",
	       conn, ep, dir, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		struct bt_audio_stream *stream = &unicast_streams[i];

		if (stream->conn == NULL) {
			printk("ASE Codec Config stream %p\n", stream);
			SET_FLAG(flag_unicast_stream_configured);
			return stream;
		}
	}

	FAIL("No streams available\n");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: stream %p cap %p\n", stream, cap);

	print_codec(codec);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count)
{
	printk("Enable: stream %p meta_count %zu\n", stream, meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

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
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 1 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR: /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO: /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_audio_stream *stream,
			struct bt_codec_data *meta,
			size_t meta_count)
{
	printk("Metadata: stream %p meta_count %zu\n", stream, meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		if (!valid_metadata_type(meta->data.type, meta->data.data_len)) {
			printk("Invalid metadata type %u or length %u\n",
			       meta->data.type, meta->data.data_len);

			return -EINVAL;
		}
	}

	return 0;
}

static int lc3_disable(struct bt_audio_stream *stream)
{
	printk("Disable: stream %p\n", stream);

	return 0;
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	printk("Stop: stream %p\n", stream);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	printk("Release: stream %p\n", stream);

	return 0;
}

static struct bt_audio_capability_ops lc3_ops = {
	.config = lc3_config,
	.reconfig = lc3_reconfig,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.start = lc3_start,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.stop = lc3_stop,
	.release = lc3_release,
};

static void set_location(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_PAC_SNK_LOC)) {
		err = bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
						       BT_AUDIO_LOCATION_FRONT_CENTER);
		if (err != 0) {
			FAIL("Failed to set sink location (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_audio_capability_set_location(BT_AUDIO_DIR_SOURCE,
						       (BT_AUDIO_LOCATION_FRONT_LEFT |
							BT_AUDIO_LOCATION_FRONT_RIGHT));
		if (err != 0) {
			FAIL("Failed to set source location (err %d)\n", err);
			return;
		}
	}

	printk("Location successfully set\n");
}

static void set_available_contexts(void)
{
	int err;

	err = bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SINK,
						BT_AUDIO_CONTEXT_TYPE_MEDIA |
						BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL);
	if (IS_ENABLED(CONFIG_BT_PAC_SNK) && err != 0) {
		FAIL("Failed to set sink available contexts (err %d)\n", err);
		return;
	}

	err = bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SOURCE,
						BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS);
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && err != 0) {
		FAIL("Failed to set source available contexts (err %d)\n", err);
		return;
	}

	printk("Available contexts successfully set\n");
}

static void init(void)
{
	static struct bt_audio_capability caps = {
		.dir = BT_AUDIO_DIR_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000,
				40000, 40000),
		.codec = &unicast_lc3_codec,
		.ops = &lc3_ops,
	};
	const struct bt_csis_register_param csis_param = {
		.set_size = 3,
		.rank = 1,
		.lockable = true,
		/* Using the CSIS test sample SIRK */
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

	err = bt_audio_capability_register(&caps);
	if (err != 0) {
		FAIL("Failed to register capabilities: %d", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		err = bt_cap_acceptor_register(&csis_param, &csis);
		if (err != 0) {
			FAIL("CAP acceptor failed to register (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_UNICAST_SERVER)) {
		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, cap_acceptor_ad,
				ARRAY_SIZE(cap_acceptor_ad), NULL, 0);
		if (err != 0) {
			FAIL("Advertising failed to start (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_SINK)) {
		bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

		UNSET_FLAG(flag_broadcaster_found);
		UNSET_FLAG(flag_base_received);
		UNSET_FLAG(flag_pa_synced);

		for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
			bt_cap_stream_ops_register(&broadcast_sink_streams[i],
						   &broadcast_stream_ops);
		}
	}

	set_available_contexts();
	set_location();
}

static void test_cap_acceptor_unicast(void)
{
	init();

	/* TODO: When babblesim supports ISO, wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_unicast_connected);

	PASS("CAP acceptor unicast passed\n");
}

static void test_cap_acceptor_broadcast(void)
{
	static struct bt_audio_stream *bap_streams[ARRAY_SIZE(broadcast_sink_streams)];
	int err;

	init();

	printk("Scanning for broadcast sources\n");
	err = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
	if (err != 0) {
		FAIL("Unable to start scan for broadcast sources: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_broadcaster_found);
	printk("Broadcast source found, waiting for PA sync\n");
	WAIT_FOR_FLAG(flag_pa_synced);
	printk("Broadcast source PA synced, waiting for BASE\n");
	WAIT_FOR_FLAG(flag_base_received);
	printk("BASE received\n");

	printk("Waiting for BIG syncable\n");
	WAIT_FOR_FLAG(flag_syncable);

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		bap_streams[i] = &broadcast_sink_streams[i].bap_stream;
	}

	printk("Syncing the sink\n");
	err = bt_audio_broadcast_sink_sync(g_broadcast_sink, bis_index_bitfield,
					   bap_streams, NULL);
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
		.test_main_f = test_cap_acceptor_unicast
	},
	{
		.test_id = "cap_acceptor_broadcast",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_acceptor_broadcast
	},
	BSTEST_END_MARKER
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
