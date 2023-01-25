/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

static struct bt_audio_stream g_streams[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_codec *g_remote_codecs[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_ep *g_sinks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];

/* Mandatory support preset by both client and server */
static struct bt_audio_lc3_preset preset_16_2_1 =
	BT_AUDIO_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					   BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

CREATE_FLAG(flag_connected);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_stream_codec_configured);
static atomic_t flag_stream_qos_configured;
CREATE_FLAG(flag_stream_enabled);
CREATE_FLAG(flag_stream_released);

static void stream_configured(struct bt_audio_stream *stream,
			      const struct bt_codec_qos_pref *pref)
{
	printk("Configured stream %p\n", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */

	SET_FLAG(flag_stream_codec_configured);
}

static void stream_qos_set(struct bt_audio_stream *stream)
{
	printk("QoS set stream %p\n", stream);

	atomic_inc(&flag_stream_qos_configured);
}

static void stream_enabled(struct bt_audio_stream *stream)
{
	printk("Enabled stream %p\n", stream);

	SET_FLAG(flag_stream_enabled);
}

static void stream_started(struct bt_audio_stream *stream)
{
	printk("Started stream %p\n", stream);
}

static void stream_metadata_updated(struct bt_audio_stream *stream)
{
	printk("Metadata updated stream %p\n", stream);
}

static void stream_disabled(struct bt_audio_stream *stream)
{
	printk("Disabled stream %p\n", stream);
}

static void stream_stopped(struct bt_audio_stream *stream)
{
	printk("Stopped stream %p\n", stream);
}

static void stream_released(struct bt_audio_stream *stream)
{
	printk("Released stream %p\n", stream);

	SET_FLAG(flag_stream_released);
}

static struct bt_audio_stream_ops stream_ops = {
	.configured = stream_configured,
	.qos_set = stream_qos_set,
	.enabled = stream_enabled,
	.started = stream_started,
	.metadata_updated = stream_metadata_updated,
	.disabled = stream_disabled,
	.stopped = stream_stopped,
	.released = stream_released,
};

static void unicast_client_location_cb(struct bt_conn *conn,
				       enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	printk("dir %u loc %X\n", dir, loc);
}

static void available_contexts_cb(struct bt_conn *conn,
				  enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	printk("snk ctx %u src ctx %u\n", snk_ctx, src_ctx);
}

const struct bt_audio_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
};

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	g_sinks[index] = ep;
}

static void add_remote_codec(struct bt_codec *codec, int index,
			     enum bt_audio_dir dir)
{
	printk("#%u: codec %p dir 0x%02x\n", index, codec, dir);

	print_codec(codec);

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		g_remote_codecs[index] = codec;
	}
}

static void discover_sink_cb(struct bt_conn *conn,
			    struct bt_codec *codec,
			    struct bt_audio_ep *ep,
			    struct bt_audio_discover_params *params)
{
	static bool codec_found;
	static bool endpoint_found;

	if (params->err != 0) {
		FAIL("Discovery failed: %d\n", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
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

	printk("Discover complete\n");

	(void)memset(params, 0, sizeof(*params));

	if (endpoint_found && codec_found) {
		SET_FLAG(flag_sink_discovered);
	} else {
		FAIL("Did not discover endpoint and codec\n");
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		bt_conn_unref(default_conn);
		default_conn = NULL;

		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	SET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

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

	for (size_t i = 0; i < ARRAY_SIZE(g_streams); i++) {
		g_streams[i].ops = &stream_ops;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_audio_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		FAIL("Failed to register client callbacks: %d", err);
		return;
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

static void exchange_mtu(void)
{
	WAIT_FOR_FLAG(flag_mtu_exchanged);
}

static void discover_sink(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sink_cb;
	params.dir = BT_AUDIO_DIR_SINK;

	err = bt_audio_discover(default_conn, &params);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_sink_discovered);
}

static int codec_configure_stream(struct bt_audio_stream *stream,
			    struct bt_audio_ep *ep)
{
	int err;

	UNSET_FLAG(flag_stream_codec_configured);

	err = bt_audio_stream_config(default_conn, stream, ep,
				     &preset_16_2_1.codec);
	if (err != 0) {
		FAIL("Could not configure stream: %d\n", err);
		return err;
	}

	WAIT_FOR_FLAG(flag_stream_codec_configured);

	return 0;
}

static void codec_configure_streams(size_t stream_cnt)
{
	for (size_t i = 0U; i < stream_cnt; i++) {
		struct bt_audio_stream *stream = &g_streams[i];
		int err;

		if (g_sinks[i] == NULL) {
			break;
		}

		err = codec_configure_stream(stream, g_sinks[i]);
		if (err != 0) {
			FAIL("Unable to configure stream[%zu]: %d", i, err);
			return;
		}
	}
}

static void qos_configure_streams(struct bt_audio_unicast_group *unicast_group,
				  size_t stream_cnt)
{
	int err;

	UNSET_FLAG(flag_stream_qos_configured);

	err = bt_audio_stream_qos(default_conn, unicast_group);
	if (err != 0) {
		FAIL("Unable to QoS configure streams: %d", err);

		return;
	}

	while (atomic_get(&flag_stream_qos_configured) != stream_cnt) {
		(void)k_sleep(K_MSEC(1));
	}
}

static int enable_stream(struct bt_audio_stream *stream)
{
	int err;

	UNSET_FLAG(flag_stream_enabled);

	err = bt_audio_stream_enable(stream, NULL, 0);
	if (err != 0) {
		FAIL("Could not enable stream: %d\n", err);

		return err;
	}

	WAIT_FOR_FLAG(flag_stream_enabled);

	return 0;
}

static void enable_streams(size_t stream_cnt)
{
	for (size_t i = 0U; i < stream_cnt; i++) {
		struct bt_audio_stream *stream = &g_streams[i];
		int err;

		err = enable_stream(stream);
		if (err != 0) {
			FAIL("Unable to enable stream[%zu]: %d",
			     i, err);

			return;
		}
	}
}

static size_t release_streams(size_t stream_cnt)
{
	for (size_t i = 0; i < stream_cnt; i++) {
		int err;

		if (g_sinks[i] == NULL) {
			break;
		}

		UNSET_FLAG(flag_stream_released);

		err = bt_audio_stream_release(&g_streams[i]);
		if (err != 0) {
			FAIL("Unable to release stream[%zu]: %d", i, err);
			return 0;
		}

		WAIT_FOR_FLAG(flag_stream_released);
	}

	return stream_cnt;
}


static size_t create_unicast_group(struct bt_audio_unicast_group **unicast_group)
{
	struct bt_audio_unicast_group_stream_pair_param pair_params[ARRAY_SIZE(g_streams)];
	struct bt_audio_unicast_group_stream_param stream_params[ARRAY_SIZE(g_streams)];
	struct bt_audio_unicast_group_param param;
	size_t stream_cnt = 0;
	int err;

	for (stream_cnt = 0U;
	     stream_cnt < MIN(ARRAY_SIZE(g_sinks), ARRAY_SIZE(g_streams));
	     stream_cnt++) {
		if (g_sinks[stream_cnt] == NULL) {
			break;
		}

		stream_params[stream_cnt].stream = &g_streams[stream_cnt];
		stream_params[stream_cnt].qos = &preset_16_2_1.qos;
		pair_params[stream_cnt].rx_param = NULL;
		pair_params[stream_cnt].tx_param = &stream_params[stream_cnt];
	}

	if (stream_cnt == 0U) {
		FAIL("No streams added to group");

		return 0;
	}

	param.params = pair_params;
	param.params_count = stream_cnt;
	param.packing = BT_ISO_PACKING_SEQUENTIAL;

	/* Require controller support for CIGs */
	printk("Creating unicast group\n");
	err = bt_audio_unicast_group_create(&param, unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);

		return 0;
	}

	return stream_cnt;
}

static void delete_unicast_group(struct bt_audio_unicast_group *unicast_group)
{
	int err;

	/* Require controller support for CIGs */
	err = bt_audio_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Unable to delete unicast group: %d", err);
		return;
	}
}

static void test_main(void)
{
	const unsigned int iterations = 3;

	init();

	scan_and_connect();

	exchange_mtu();

	discover_sink();

	/* Run the stream setup multiple time to ensure states are properly
	 * set and reset
	 */
	for (unsigned int i = 0U; i < iterations; i++) {
		struct bt_audio_unicast_group *unicast_group;
		size_t stream_cnt;

		printk("\n########### Running iteration #%u\n\n", i);

		printk("Creating unicast group\n");
		stream_cnt = create_unicast_group(&unicast_group);

		printk("Codec configuring streams\n");
		codec_configure_streams(stream_cnt);

		printk("QoS configuring streams\n");
		qos_configure_streams(unicast_group, stream_cnt);

		printk("Enabling streams\n");
		enable_streams(stream_cnt);

		/* TODO: When babblesim supports CIS connection start Audio streams */

		release_streams(stream_cnt);

		/* Test removing streams from group after creation */
		printk("Deleting unicast group\n");
		delete_unicast_group(unicast_group);
		unicast_group = NULL;
	}


	PASS("Unicast client passed\n");
}

static const struct bst_test_instance test_unicast_client[] = {
	{
		.test_id = "unicast_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_unicast_client);
}

#else /* !(CONFIG_BT_AUDIO_UNICAST_CLIENT) */

struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
