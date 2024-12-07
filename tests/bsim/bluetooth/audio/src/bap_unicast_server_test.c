/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bap_common.h"
#include "bap_stream_rx.h"
#include "bap_stream_tx.h"
#include "bstests.h"
#include "common.h"

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)

extern enum bst_result_t bst_result;

#define CHANNEL_COUNT_1 BIT(0)

#define PREF_CONTEXT (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA)

static const struct bt_audio_codec_cap lc3_codec_cap = {
	.path_id = BT_ISO_DATA_PATH_HCI,
	.id = BT_HCI_CODING_FORMAT_LC3,
	.cid = 0x0000U,
	.vid = 0x0000U,
	.data_len = (3 + 1) + (2 + 1) + (2 + 1) + (5 + 1) + (2 + 1),
	.data = {
			BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CAP_TYPE_FREQ,
					    BT_BYTES_LIST_LE16(BT_AUDIO_CODEC_CAP_FREQ_16KHZ)),
			BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CAP_TYPE_DURATION,
					    BT_AUDIO_CODEC_CAP_DURATION_10),
			BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CAP_TYPE_CHAN_COUNT, CHANNEL_COUNT_1),
			BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN,
					    BT_BYTES_LIST_LE16(40U), BT_BYTES_LIST_LE16(40U)),
			BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CAP_TYPE_FRAME_COUNT, 1U),
		},
	.meta_len = (5 + 1) + (LONG_META_LEN + 1U),
	.meta = {
			BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
					    BT_BYTES_LIST_LE32(PREF_CONTEXT)),
			BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, LONG_META),
		},
};

static struct audio_test_stream
	test_streams[CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT + CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT];

static const struct bt_bap_qos_cfg_pref qos_pref =
	BT_BAP_QOS_CFG_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),    /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	BT_BYTES_LIST_LE16(PREF_CONTEXT),
	BT_BYTES_LIST_LE16(PREF_CONTEXT),
	0x00, /* Metadata length */
};

static const struct bt_data unicast_server_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
};
static struct bt_le_ext_adv *ext_adv;

CREATE_FLAG(flag_stream_configured);
CREATE_FLAG(flag_stream_started);
static void print_ase_info(struct bt_bap_ep *ep, void *user_data)
{
	struct bt_bap_ep_info info;

	bt_bap_ep_get_info(ep, &info);
	printk("ASE info: id %u state %u dir %u\n", info.id, info.state, info.dir);
}

static struct bt_bap_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_streams); i++) {
		struct bt_bap_stream *stream = bap_stream_from_audio_test_stream(&test_streams[i]);

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int lc3_config(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_audio_codec_cfg *codec_cfg, struct bt_bap_stream **stream,
		      struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Config: conn %p ep %p dir %u\n", conn, ep, dir);

	print_codec_cfg(codec_cfg);

	*stream = stream_alloc();
	if (*stream == NULL) {
		printk("No test_streams available\n");
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_NO_MEM, BT_BAP_ASCS_REASON_NONE);
		return -ENOMEM;
	}

	printk("ASE Codec Config stream %p\n", *stream);

	bt_bap_unicast_server_foreach_ep(conn, print_ase_info, NULL);

	SET_FLAG(flag_stream_configured);

	*pref = qos_pref;

	return 0;
}

static int lc3_reconfig(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_audio_codec_cfg *codec_cfg,
			struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);

	print_codec_cfg(codec_cfg);
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	test_stream->tx_sdu_size = qos->sdu;

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
		      struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_len %zu\n", stream, meta_len);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static bool data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data->type)) {
		printk("Invalid metadata type %u or length %u\n", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);
		return false;
	}

	return true;
}

static int lc3_metadata(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_len %zu\n", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
}

static int lc3_disable(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Disable: stream %p\n", stream);

	return 0;
}

static int lc3_stop(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Stop: stream %p\n", stream);

	return 0;
}

static int lc3_release(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Release: stream %p\n", stream);

	return 0;
}

static struct bt_bap_unicast_server_register_param param = {
	CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT,
	CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT
};

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
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

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	printk("Enabled: stream %p\n", stream);

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
	printk("Started: stream %p\n", stream);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_register(stream);
		if (err != 0) {
			FAIL("Failed to register stream %p for TX: %d\n", stream, err);
			return;
		}
	}

	SET_FLAG(flag_stream_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stopped stream %p with reason 0x%02X\n", stream, reason);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_unregister(stream);
		if (err != 0) {
			FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
			return;
		}
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = bap_stream_rx_recv_cb,
	.sent = bap_stream_tx_sent_cb,
};

static void transceive_test_streams(void)
{
	struct bt_bap_stream *source_stream = NULL;
	struct bt_bap_stream *sink_stream = NULL;
	struct bt_bap_ep_info info;
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(test_streams); i++) {
		struct bt_bap_stream *stream = bap_stream_from_audio_test_stream(&test_streams[i]);

		if (stream->ep == NULL) {
			break;
		}

		while (true) {
			err = bt_bap_ep_get_info(stream->ep, &info);
			if (err != 0) {
				FAIL("Failed to get endpoint info for stream[%zu] %p: %d\n", i,
				     stream, err);
				return;
			}

			/* Ensure that all configured test_streams are in the streaming state before
			 * starting TX and RX
			 */
			if (info.state == BT_BAP_EP_STATE_STREAMING) {
				break;
			}

			k_sleep(K_MSEC(100));
		}

		if (info.dir == BT_AUDIO_DIR_SINK && sink_stream == NULL) {
			sink_stream = stream;
		} else if (info.dir == BT_AUDIO_DIR_SOURCE && source_stream == NULL) {
			source_stream = stream;
		}
	}

	if (source_stream != NULL) {
		struct audio_test_stream *test_stream =
			audio_test_stream_from_bap_stream(source_stream);

		/* Keep sending until we reach the minimum expected */
		while (test_stream->tx_cnt < MIN_SEND_COUNT) {
			k_sleep(K_MSEC(100));
		}
	}

	if (sink_stream != NULL) {
		printk("Waiting for data\n");
		WAIT_FOR_FLAG(flag_audio_received);
	}
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
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, (BT_AUDIO_LOCATION_FRONT_LEFT |
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

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					     BT_AUDIO_CONTEXT_TYPE_MEDIA |
						     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL);
	if (IS_ENABLED(CONFIG_BT_PAC_SNK) && err != 0) {
		FAIL("Failed to set sink supported contexts (err %d)\n", err);
		return;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					     BT_AUDIO_CONTEXT_TYPE_MEDIA |
						     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL);
	if (IS_ENABLED(CONFIG_BT_PAC_SNK) && err != 0) {
		FAIL("Failed to set sink available contexts (err %d)\n", err);
		return;
	}

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					     BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS);
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && err != 0) {
		FAIL("Failed to set source supported contexts (err %d)\n", err);
		return;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					     BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS);
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && err != 0) {
		FAIL("Failed to set source available contexts (err %d)\n", err);
		return;
	}

	printk("Available contexts successfully set\n");
}

static void init(void)
{
	static struct bt_pacs_cap cap = {
		.codec_cap = &lc3_codec_cap,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bap_stream_tx_init();

	err = bt_bap_unicast_server_register(&param);
	if (err != 0) {
		FAIL("Failed to register unicast server (err %d)\n", err);

		return;
	}

	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err != 0) {
		FAIL("Failed to register capabilities: %d", err);
		return;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap);
	if (err != 0) {
		FAIL("Failed to register capabilities: %d", err);
		return;
	}

	set_location();
	set_available_contexts();

	for (size_t i = 0; i < ARRAY_SIZE(test_streams); i++) {
		bt_bap_stream_cb_register(bap_stream_from_audio_test_stream(&test_streams[i]),
					  &stream_ops);
	}

	/* Create a connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &ext_adv);
	if (err != 0) {
		FAIL("Failed to create advertising set (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_set_data(ext_adv, unicast_server_ad, ARRAY_SIZE(unicast_server_ad),
				     NULL, 0);
	if (err != 0) {
		FAIL("Failed to set advertising data (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);
		return;
	}
	printk("Advertising started\n");
}

static void test_main(void)
{
	init();

	/* TODO: When babblesim supports ISO, wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_stream_configured);


	WAIT_FOR_FLAG(flag_stream_started);
	transceive_test_streams();
	WAIT_FOR_UNSET_FLAG(flag_connected);
	PASS("Unicast server passed\n");
}

static void restart_adv_cb(struct k_work *work)
{
	int err;

	printk("Restarting ext_adv after disconnect\n");

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		if (err != -EALREADY) {
			FAIL("Failed to start advertising set (err %d)\n", err);
			return;
		}
	}
}

static K_WORK_DEFINE(restart_adv_work, restart_adv_cb);

static void acl_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != default_conn) {
		return;
	}

	k_work_submit(&restart_adv_work);
}

static void test_main_acl_disconnect(void)
{
	struct bt_le_ext_adv *dummy_ext_adv[CONFIG_BT_MAX_CONN - 1];
	static struct bt_conn_cb conn_callbacks = {
		.disconnected = acl_disconnected,
	};

	init();

	stream_ops.recv = NULL; /* We do not care about data in this test */

	/* Create CONFIG_BT_MAX_CONN - 1 dummy advertising sets, to ensure that we only have 1 free
	 * connection when attempting to restart advertising, which should ensure that the
	 * bt_conn object is properly unref'ed by the stack
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(dummy_ext_adv); i++) {
		const struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
			(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN), BT_GAP_ADV_SLOW_INT_MAX,
			BT_GAP_ADV_SLOW_INT_MAX, NULL);
		int err;

		err = bt_le_ext_adv_create(&param, NULL, &dummy_ext_adv[i]);
		if (err != 0) {
			FAIL("Failed to create advertising set[%zu] (err %d)\n", i, err);
			return;
		}

		err = bt_le_ext_adv_start(dummy_ext_adv[i], BT_LE_EXT_ADV_START_DEFAULT);
		if (err != 0) {
			FAIL("Failed to start advertising set[%zu] (err %d)\n", i, err);
			return;
		}
	}

	bt_conn_cb_register(&conn_callbacks);

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_stream_configured);

	/* The client will reconnect */
	WAIT_FOR_UNSET_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_connected);
	PASS("Unicast server ACL disconnect  passed\n");
}

static const struct bst_test_instance test_unicast_server[] = {
	{
		.test_id = "unicast_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	{
		.test_id = "unicast_server_acl_disconnect",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_acl_disconnect,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_unicast_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_unicast_server);
}

#else /* !(CONFIG_BT_BAP_UNICAST_SERVER) */

struct bst_test_list *test_unicast_server_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_UNICAST_SERVER */
