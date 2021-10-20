/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP) && defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

static struct bt_audio_stream g_streams[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_audio_capability *g_remote_capabilities[CONFIG_BT_BAP_PAC_COUNT];
static struct bt_audio_ep *g_sinks[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_conn *g_conn;

/* Mandatory support preset by both client and server */
static struct bt_audio_lc3_preset preset_16_2_1 = BT_AUDIO_LC3_UNICAST_PRESET_16_2_1;

CREATE_FLAG(flag_connected);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_stream_configured);
CREATE_FLAG(flag_stream_qos);
CREATE_FLAG(flag_stream_enabled);

static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p cap %p\n", conn, ep, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(g_streams); i++) {
		/* If the connection is NULL, the stream is free */
		if (g_streams[i].conn == NULL) {
			return &g_streams[i];
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

	SET_FLAG(flag_stream_configured);

	return 0;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	SET_FLAG(flag_stream_qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream, uint8_t meta_count,
		      struct bt_codec_data *meta)
{
	printk("Enable: stream %p meta_count %u\n", stream, meta_count);

	SET_FLAG(flag_stream_enabled);

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static int lc3_metadata(struct bt_audio_stream *stream, uint8_t meta_count,
			struct bt_codec_data *meta)
{
	printk("Metadata: stream %p meta_count %u\n", stream, meta_count);

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

static struct bt_audio_capability caps[] = {
	{
		.type = BT_AUDIO_SOURCE,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000),
		.codec = &preset_16_2_1.codec,
		.ops = &lc3_ops,
	}
};

static void stream_connected(struct bt_audio_stream *stream)
{
	printk("Audio Stream %p connected\n", stream);
}

static void stream_disconnected(struct bt_audio_stream *stream, uint8_t reason)
{
	printk("Audio Stream %p disconnected (reason 0x%02x)\n",
	       stream, reason);
}

static struct bt_audio_stream_ops stream_ops = {
	.connected	= stream_connected,
	.disconnected	= stream_disconnected,
};

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	printk("Sink #%u: ep %p\n", index, ep);

	g_sinks[index] = ep;
}

static void add_remote_capability(struct bt_audio_capability *cap, int index)
{
	printk("#%u: cap %p type 0x%02x\n", index, cap, cap->type);

	print_codec(cap->codec);

	if (cap->type != BT_AUDIO_SINK && cap->type != BT_AUDIO_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_BAP_PAC_COUNT) {
		g_remote_capabilities[index] = cap;
	}
}

static void discover_sink_cb(struct bt_conn *conn,
			    struct bt_audio_capability *cap,
			    struct bt_audio_ep *ep,
			    struct bt_audio_discover_params *params)
{
	static bool capability_found;
	static bool endpoint_found;

	if (params->err != 0) {
		FAIL("Discovery failed: %d\n", params->err);
		return;
	}

	if (cap != NULL) {
		add_remote_capability(cap, params->num_caps);
		capability_found = true;
		return;
	}

	if (ep != NULL) {
		if (params->type == BT_AUDIO_SINK) {
			add_remote_sink(ep, params->num_eps);
			endpoint_found = true;
		} else {
			FAIL("Invalid param type: %u\n", params->type);
		}

		return;
	}

	printk("Discover complete\n");

	(void)memset(params, 0, sizeof(*params));

	if (endpoint_found && capability_found) {
		SET_FLAG(flag_sink_discovered);
	} else {
		FAIL("Did not discover endpoint and capabilities\n");
	}
}

static void gatt_mtu_cb(struct bt_conn *conn, uint8_t err,
		   struct bt_gatt_exchange_params *params)
{
	if (err != 0) {
		FAIL("Failed to exchange MTU (%u)\n", err);
		return;
	}

	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		bt_conn_unref(conn);
		g_conn = NULL;

		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	g_conn = conn;
	SET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(caps); i++) {
		err = bt_audio_capability_register(&caps[i]);
		if (err != 0) {
			FAIL("Register capabilities[%zu] failed: %d", i, err);
			return;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(g_streams); i++) {
		g_streams[i].ops = &stream_ops;
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
	struct bt_gatt_exchange_params mtu_params = {
		.func = gatt_mtu_cb
	};
	int err;

	err = bt_gatt_exchange_mtu(g_conn, &mtu_params);
	if (err != 0) {
		FAIL("Failed to exchange MTU %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_mtu_exchanged);
}

static void discover_sink(void)
{
	static struct bt_audio_discover_params params;
	int err;

	params.func = discover_sink_cb;
	params.type = BT_AUDIO_SINK;

	err = bt_audio_discover(g_conn, &params);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_sink_discovered);
}

static struct bt_audio_stream *configure_stream(struct bt_audio_ep *ep)
{
	struct bt_audio_stream *stream;

	UNSET_FLAG(flag_stream_configured);

	stream = bt_audio_stream_config(g_conn, ep, g_remote_capabilities[0],
					&preset_16_2_1.codec);
	if (stream == NULL) {
		FAIL("Could not configure stream\n");
		return NULL;
	}

	WAIT_FOR_FLAG(flag_stream_configured);

	return stream;
}

static void test_main(void)
{
	struct bt_audio_stream *configured_streams[CONFIG_BT_BAP_ASE_SNK_COUNT];
	struct bt_audio_unicast_group *unicast_group;
	uint8_t stream_cnt;
	int err;

	init();

	scan_and_connect();

	exchange_mtu();

	discover_sink();

	printk("Configuring streams\n");
	memset(configured_streams, 0, sizeof(configured_streams));
	for (stream_cnt = 0; stream_cnt < ARRAY_SIZE(g_sinks); stream_cnt++) {
		if (g_sinks[stream_cnt] == NULL) {
			break;
		}

		configured_streams[stream_cnt] = configure_stream(g_sinks[stream_cnt]);
	}

	if (configured_streams[0] == NULL) {
		FAIL("No streams configured");
		return;
	}

	printk("Creating unicast group\n");
	err = bt_audio_unicast_group_create(g_streams, stream_cnt,
					    &unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);
		return;
	}

	/* TODO: When babblesim supports ISO setup Audio streams */

	printk("Deleting unicast group\n");
	err = bt_audio_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Unable to delete unicast group: %d", err);
		return;
	}
	unicast_group = NULL;

	/* Recreate unicast group to verify that it's possible */
	printk("Recreating unicast group\n");
	err = bt_audio_unicast_group_create(g_streams, stream_cnt,
					    &unicast_group);
	if (err != 0) {
		FAIL("Unable to create unicast group: %d", err);
		return;
	}

	printk("Deleting unicast group\n");
	err = bt_audio_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Unable to delete unicast group: %d", err);
		return;
	}
	unicast_group = NULL;

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

#else /* !(CONFIG_BT_BAP && CONFIG_BT_AUDIO_UNICAST_CLIENT) */

struct bst_test_list *test_unicast_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP && CONFIG_BT_AUDIO_UNICAST_CLIENT */
