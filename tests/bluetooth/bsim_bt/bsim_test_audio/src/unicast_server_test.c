/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP) && defined(CONFIG_BT_AUDIO_UNICAST_SERVER)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

/* Mandatory support preset by both client and server */
static struct lc3_preset preset_16_2_1 =
	LC3_PRESET("16_2_1",
		   BT_CODEC_LC3_CONFIG_16_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(40u, 2u, 10u, 40000u));

static struct bt_audio_chan channels[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];

/* TODO: Expand with BAP data */
static const struct bt_data unicast_server_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
};

CREATE_FLAG(flag_connected);
CREATE_FLAG(flag_chan_configured);

static struct bt_audio_chan *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	printk("ASE Codec Config: conn %p ep %p cap %p\n", conn, ep, cap);

	print_codec(codec);

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		struct bt_audio_chan *chan = &channels[i];

		if (chan->conn == NULL) {
			printk("ASE Codec Config chan %p\n", chan);
			SET_FLAG(flag_chan_configured);
			return chan;
		}
	}

	FAIL("No channels available\n");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	printk("ASE Codec Reconfig: chan %p cap %p\n", chan, cap);

	print_codec(codec);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	printk("QoS: chan %p qos %p\n", chan, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_chan *chan, uint8_t meta_count,
		      struct bt_codec_data *meta)
{
	printk("Enable: chan %p meta_count %u\n", chan, meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_chan *chan)
{
	printk("Start: chan %p\n", chan);

	return 0;
}

static int lc3_metadata(struct bt_audio_chan *chan, uint8_t meta_count,
			struct bt_codec_data *meta)
{
	printk("Metadata: chan %p meta_count %u\n", chan, meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_chan *chan)
{
	printk("Disable: chan %p\n", chan);

	return 0;
}

static int lc3_stop(struct bt_audio_chan *chan)
{
	printk("Stop: chan %p\n", chan);

	return 0;
}

static int lc3_release(struct bt_audio_chan *chan)
{
	printk("Release: chan %p\n", chan);

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

static void chan_connected(struct bt_audio_chan *chan)
{
	printk("Audio Channel %p connected\n", chan);
}

static void chan_disconnected(struct bt_audio_chan *chan, uint8_t reason)
{
	printk("Audio Channel %p disconnected (reason 0x%02x)\n", chan, reason);
}

static void chan_recv(struct bt_audio_chan *chan, struct net_buf *buf)
{
	printk("Incoming audio on channel %p len %u\n", chan, buf->len);
}

static struct bt_audio_chan_ops chan_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
	.recv = chan_recv
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
	SET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void init(void)
{
	static struct bt_audio_capability caps = {
		.type = BT_AUDIO_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000),
		.codec = &preset_16_2_1.codec,
		.ops = &lc3_ops,
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

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		bt_audio_chan_cb_register(&channels[i], &chan_ops);
	}


	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, unicast_server_ad,
			      ARRAY_SIZE(unicast_server_ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}
}

static void test_main(void)
{
	init();

	/* TODO: When babblesim supports ISO, wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_chan_configured);
	PASS("Unicast server passed\n");
}

static const struct bst_test_instance test_unicast_server[] = {
	{
		.test_id = "unicast_server",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_unicast_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_unicast_server);
}

#else /* !(CONFIG_BT_BAP && CONFIG_BT_AUDIO_UNICAST_SERVER) */

struct bst_test_list *test_unicast_server_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP && CONFIG_BT_AUDIO_UNICAST_SERVER */
