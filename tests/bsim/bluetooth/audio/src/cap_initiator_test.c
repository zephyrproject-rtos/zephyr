/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_INITIATOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include "common.h"
#include "bap_unicast_common.h"


#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
#define BROADCAST_STREMT_CNT CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT
#else
#define BROADCAST_STREMT_CNT 0
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED (BROADCAST_ENQUEUE_COUNT * BROADCAST_STREMT_CNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * BROADCAST_STREMT_CNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

extern enum bst_result_t bst_result;
static struct bt_cap_stream broadcast_source_streams[BROADCAST_STREMT_CNT];
static struct bt_cap_stream *broadcast_streams[ARRAY_SIZE(broadcast_source_streams)];
static struct bt_bap_lc3_preset broadcast_preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset unicast_preset_16_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct bt_cap_stream unicast_client_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *unicast_sink_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];

static K_SEM_DEFINE(sem_broadcast_started, 0U, ARRAY_SIZE(broadcast_streams));
static K_SEM_DEFINE(sem_broadcast_stopped, 0U, ARRAY_SIZE(broadcast_streams));

CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_updated);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_broadcast_stopping);

static void broadcast_started_cb(struct bt_bap_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_broadcast_started);
}

static void broadcast_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_broadcast_stopped);
}

static void broadcast_sent_cb(struct bt_bap_stream *stream)
{
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
	static bool mock_data_initialized;
	static uint32_t seq_num;
	struct net_buf *buf;
	int ret;

	if (broadcast_preset_16_2_1.qos.sdu > CONFIG_BT_ISO_TX_MTU) {
		FAIL("Invalid SDU %u for the MTU: %d",
		     broadcast_preset_16_2_1.qos.sdu, CONFIG_BT_ISO_TX_MTU);
		return;
	}

	if (TEST_FLAG(flag_broadcast_stopping)) {
		return;
	}

	if (!mock_data_initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
			/* Initialize mock data */
			mock_data[i] = (uint8_t)i;
		}
		mock_data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, mock_data, broadcast_preset_16_2_1.qos.sdu);
	ret = bt_bap_stream_send(stream, buf, seq_num++, BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}
}

static struct bt_bap_stream_ops broadcast_stream_ops = {.started = broadcast_started_cb,
							.stopped = broadcast_stopped_cb,
							.sent = broadcast_sent_cb};


static void unicast_stream_configured(struct bt_bap_stream *stream,
			      const struct bt_codec_qos_pref *pref)
{
	printk("Configured stream %p\n", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */
}

static void unicast_stream_qos_set(struct bt_bap_stream *stream)
{
	printk("QoS set stream %p\n", stream);
}

static void unicast_stream_enabled(struct bt_bap_stream *stream)
{
	printk("Enabled stream %p\n", stream);
}

static void unicast_stream_started(struct bt_bap_stream *stream)
{
	printk("Started stream %p\n", stream);
}

static void unicast_stream_metadata_updated(struct bt_bap_stream *stream)
{
	printk("Metadata updated stream %p\n", stream);
}

static void unicast_stream_disabled(struct bt_bap_stream *stream)
{
	printk("Disabled stream %p\n", stream);
}

static void unicast_stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stopped stream with reason 0x%02X%p\n", stream, reason);
}

static void unicast_stream_released(struct bt_bap_stream *stream)
{
	printk("Released stream %p\n", stream);
}

static struct bt_bap_stream_ops unicast_stream_ops = {
	.configured = unicast_stream_configured,
	.qos_set = unicast_stream_qos_set,
	.enabled = unicast_stream_enabled,
	.started = unicast_stream_started,
	.metadata_updated = unicast_stream_metadata_updated,
	.disabled = unicast_stream_disabled,
	.stopped = unicast_stream_stopped,
	.released = unicast_stream_released,
};

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Failed to discover CAS: %d", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL)  {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_discovered);
}

static void unicast_start_complete_cb(struct bt_bap_unicast_group *unicast_group,
				      int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to start (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_started);
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to update (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_updated);
}

static void unicast_stop_complete_cb(struct bt_bap_unicast_group *unicast_group, int err,
				     struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to stop (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_stopped);
}

static struct bt_cap_initiator_cb cap_cb = {
	.unicast_discovery_complete = cap_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

static void add_remote_sink(struct bt_bap_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	unicast_sink_eps[index] = ep;
}

static void print_remote_codec(struct bt_codec *codec, int index, enum bt_audio_dir dir)
{
	printk("#%u: codec %p dir 0x%02x\n", index, codec, dir);

	print_codec(codec);
}

static void discover_sink_cb(struct bt_conn *conn, struct bt_codec *codec, struct bt_bap_ep *ep,
			     struct bt_bap_unicast_client_discover_params *params)
{
	static bool codec_found;
	static bool endpoint_found;

	if (params->err != 0) {
		FAIL("Discovery failed: %d\n", params->err);
		return;
	}

	if (codec != NULL) {
		print_remote_codec(codec, params->num_caps, params->dir);
		codec_found = true;

		return;
	}

	if (ep != NULL) {
		if (params->dir == BT_AUDIO_DIR_SINK) {
			add_remote_sink(ep, params->num_eps);
			endpoint_found = true;
		} else {
			FAIL("Invalid param dir: %u\n", params->dir);
		}

		return;
	}

	printk("Sink discover complete\n");

	(void)memset(params, 0, sizeof(*params));

	if (endpoint_found && codec_found) {
		SET_FLAG(flag_sink_discovered);
	} else {
		FAIL("Did not discover endpoint and codec\n");
	}
}

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT)) {
		bt_gatt_cb_register(&gatt_callbacks);

		err = bt_cap_initiator_register_cb(&cap_cb);
		if (err != 0) {
			FAIL("Failed to register CAP callbacks (err %d)\n", err);
			return;
		}

		for (size_t i = 0; i < ARRAY_SIZE(broadcast_streams); i++) {
			bt_cap_stream_ops_register(&unicast_client_streams[i],
						   &unicast_stream_ops);
		}
	}

	if (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SOURCE)) {
		(void)memset(broadcast_source_streams, 0,
			     sizeof(broadcast_source_streams));

		for (size_t i = 0; i < ARRAY_SIZE(broadcast_streams); i++) {
			broadcast_streams[i] = &broadcast_source_streams[i];
			bt_cap_stream_ops_register(broadcast_streams[i],
						   &broadcast_stream_ops);
		}
	}
}

static void scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
}

static void discover_sink(void)
{
	static struct bt_bap_unicast_client_discover_params params;
	int err;

	params.func = discover_sink_cb;
	params.dir = BT_AUDIO_DIR_SINK;

	err = bt_bap_unicast_client_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_sink_discovered);
}

static void discover_cas(void)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void unicast_group_create(struct bt_bap_unicast_group **out_unicast_group)
{
	struct bt_bap_unicast_group_stream_param group_stream_params;
	struct bt_bap_unicast_group_stream_pair_param pair_params;
	struct bt_bap_unicast_group_param group_param;
	int err;

	group_stream_params.qos = &unicast_preset_16_2_1.qos;
	group_stream_params.stream = &unicast_client_streams[0].bap_stream;
	pair_params.tx_param = &group_stream_params;
	pair_params.rx_param = NULL;

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = 1;
	group_param.params = &pair_params;

	err = bt_bap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void unicast_audio_start(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param[1];
	struct bt_cap_unicast_audio_start_param param;
	int err;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = 1u;
	param.stream_params = stream_param;
	stream_param[0].member.member = default_conn;
	stream_param[0].stream = &unicast_client_streams[0];
	stream_param[0].ep = unicast_sink_eps[0];
	stream_param[0].codec = &unicast_preset_16_2_1.codec;
	stream_param[0].qos = &unicast_preset_16_2_1.qos;

	UNSET_FLAG(flag_started);

	err = bt_cap_initiator_unicast_audio_start(&param, unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_started);
}

static void unicast_audio_update(void)
{
	struct bt_cap_unicast_audio_update_param param;
	int err;

	param.stream = &unicast_client_streams[0];
	param.meta = unicast_preset_16_2_1.codec.meta;
	param.meta_count = unicast_preset_16_2_1.codec.meta_count;

	UNSET_FLAG(flag_updated);

	err = bt_cap_initiator_unicast_audio_update(&param, 1);
	if (err != 0) {
		FAIL("Failed to update unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_updated);
}

static void unicast_audio_stop(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	UNSET_FLAG(flag_stopped);

	err = bt_cap_initiator_unicast_audio_stop(unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_stopped);
}

static void unicast_group_delete(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void test_cap_initiator_unicast(void)
{
	struct bt_bap_unicast_group *unicast_group;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas();

	discover_sink();

	unicast_group_create(&unicast_group);

	unicast_audio_start(unicast_group);

	unicast_audio_update();

	unicast_audio_stop(unicast_group);

	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	PASS("CAP initiator unicast passed\n");
}

static int setup_extended_adv(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, adv);
	if (err != 0) {
		printk("Unable to create extended advertising set: %d\n", err);
		return err;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters: %d\n",
		       err);
		return err;
	}

	return 0;
}

static int setup_extended_adv_data(struct bt_cap_broadcast_source *source,
				   struct bt_le_ext_adv *adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf,
			      BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	err = bt_cap_initiator_broadcast_get_id(source, &broadcast_id);
	if (err != 0) {
		printk("Unable to get broadcast ID: %d\n", err);
		return err;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len + sizeof(ext_ad.type);
	ext_ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		printk("Failed to set extended advertising data: %d\n", err);
		return err;
	}

	/* Setup periodic advertising data */
	err = bt_cap_initiator_broadcast_get_base(source, &base_buf);
	if (err != 0) {
		printk("Failed to get encoded BASE: %d\n", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		printk("Failed to set periodic advertising data: %d\n", err);
		return err;
	}

	return 0;
}

static int start_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising: %d\n", err);
		return err;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising: %d\n", err);
		return err;
	}

	return 0;
}

static int stop_and_delete_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Stop extended advertising */
	err = bt_le_per_adv_stop(adv);
	if (err) {
		printk("Failed to stop periodic advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising: %d\n", err);
		return err;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		printk("Failed to delete extended advertising: %d\n", err);
		return err;
	}

	return 0;
}

static void test_cap_initiator_broadcast(void)
{
	struct bt_codec_data bis_codec_data = BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FREQ,
							    BT_CODEC_CONFIG_LC3_FREQ_16KHZ);
	const uint16_t mock_ccid = 0x1234;
	const struct bt_codec_data new_metadata[] = {
		BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
			      BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_MEDIA)),
		BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_CCID_LIST,
			      BT_BYTES_LIST_LE16(mock_ccid)),
	};
	struct bt_cap_initiator_broadcast_stream_param
		stream_params[ARRAY_SIZE(broadcast_source_streams)];
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_param;
	struct bt_cap_initiator_broadcast_create_param create_param;
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_le_ext_adv *adv;
	int err;

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(broadcast_streams); i++) {
		stream_params[i].stream = &broadcast_source_streams[i];
		bt_cap_stream_ops_register(stream_params[i].stream,
					   &broadcast_stream_ops);
		stream_params[i].data_count = 1U;
		stream_params[i].data = &bis_codec_data;
	}

	subgroup_param.stream_count = ARRAY_SIZE(broadcast_streams);
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec = &broadcast_preset_16_2_1.codec;

	create_param.subgroup_count = 1U;
	create_param.subgroup_params = &subgroup_param;
	create_param.qos = &broadcast_preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	init();

	printk("Creating broadcast source with %zu broadcast_streams\n",
	       ARRAY_SIZE(broadcast_streams));

	err = setup_extended_adv(&adv);
	if (err != 0) {
		FAIL("Unable to setup extended advertiser: %d\n", err);
		return;
	}

	err = bt_cap_initiator_broadcast_audio_start(&create_param, adv,
						     &broadcast_source);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	err = setup_extended_adv_data(broadcast_source, adv);
	if (err != 0) {
		FAIL("Unable to setup extended advertising data: %d\n", err);
		return;
	}

	err = start_extended_adv(adv);
	if (err != 0) {
		FAIL("Unable to start extended advertiser: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for broadcast_streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_streams); i++) {
		k_sem_take(&sem_broadcast_started, K_FOREVER);
	}

	/* Initialize sending */
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			broadcast_sent_cb(&broadcast_streams[i]->bap_stream);
		}
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(5));

	err = bt_cap_initiator_broadcast_audio_update(broadcast_source,
						      new_metadata,
						      ARRAY_SIZE(new_metadata));
	if (err != 0) {
		FAIL("Failed to update broadcast source metadata: %d\n", err);
		return;
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(5));

	err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for broadcast_streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_streams); i++) {
		k_sem_take(&sem_broadcast_stopped, K_FOREVER);
	}

	err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}
	broadcast_source = NULL;

	err = stop_and_delete_extended_adv(adv);
	if (err != 0) {
		FAIL("Failed to stop and delete extended advertising: %d\n", err);
		return;
	}
	adv = NULL;

	PASS("CAP initiator broadcast passed\n");
}

static const struct bst_test_instance test_cap_initiator[] = {
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	{.test_id = "cap_initiator_unicast",
	 .test_post_init_f = test_init,
	 .test_tick_f = test_tick,
	 .test_main_f = test_cap_initiator_unicast},
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	{.test_id = "cap_initiator_broadcast",
	 .test_post_init_f = test_init,
	 .test_tick_f = test_tick,
	 .test_main_f = test_cap_initiator_broadcast},
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
	BSTEST_END_MARKER};

struct bst_test_list *test_cap_initiator_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_initiator);
}

#else /* !(CONFIG_BT_CAP_INITIATOR) */

struct bst_test_list *test_cap_initiator_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_INITIATOR */
