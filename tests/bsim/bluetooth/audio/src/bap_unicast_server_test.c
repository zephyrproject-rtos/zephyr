/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP_UNICAST_SERVER)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include "common.h"
#include "bap_unicast_common.h"

extern enum bst_result_t bst_result;

#define CHANNEL_COUNT_1 BIT(0)

#define PREF_CONTEXT \
	(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA)

static struct bt_codec lc3_codec = {
	.path_id = BT_ISO_DATA_PATH_HCI,
	.id = BT_CODEC_LC3_ID,
	.cid = 0x0000U,
	.vid = 0x0000U,
	.data_count = 5U,
	.data = {
		BT_CODEC_DATA(BT_CODEC_LC3_FREQ,
			      (BT_CODEC_LC3_FREQ_16KHZ & 0xFFU),
			       ((BT_CODEC_LC3_FREQ_16KHZ >> 8) & 0xFFU)),
		BT_CODEC_DATA(BT_CODEC_LC3_DURATION, BT_CODEC_LC3_DURATION_10),
		BT_CODEC_DATA(BT_CODEC_LC3_CHAN_COUNT, CHANNEL_COUNT_1),
		BT_CODEC_DATA(BT_CODEC_LC3_FRAME_LEN,
			      (40U & 0xFFU), ((40U >> 8) & 0xFFU),
			      (40U & 0xFFU), ((40U >> 8) & 0xFFU)),
		BT_CODEC_DATA(BT_CODEC_LC3_FRAME_COUNT, 1U)
	},
	.meta_count = 2,
	.meta = {
		BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
			      (PREF_CONTEXT & 0xFFU),
			      ((PREF_CONTEXT >> 8) & 0xFFU)),
		BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR,
			      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
			      0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
			      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			      0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
			      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
			      0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			      0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
			      0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
			      0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
			      0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
			      0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
			      0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f)
	}
};

static struct bt_bap_stream streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];

static const struct bt_codec_qos_pref qos_pref =
	BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0x02, 10, 40000, 40000, 40000, 40000);

/* TODO: Expand with BAP data */
static const struct bt_data unicast_server_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
};

CREATE_FLAG(flag_stream_configured);

static void print_ase_info(struct bt_bap_ep *ep, void *user_data)
{
	struct bt_bap_ep_info info;

	bt_bap_ep_get_info(ep, &info);
	printk("ASE info: id %u state %u dir %u\n", info.id, info.state, info.dir);
}

static struct bt_bap_stream *stream_alloc(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_bap_stream *stream = &streams[i];

		if (!stream->conn) {
			return stream;
		}
	}

	return NULL;
}

static int lc3_config(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_codec *codec, struct bt_bap_stream **stream,
		      struct bt_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Config: conn %p ep %p dir %u\n", conn, ep, dir);

	print_codec(codec);

	*stream = stream_alloc();
	if (*stream == NULL) {
		printk("No streams available\n");
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
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref,
			struct bt_bap_ascs_rsp *rsp)
{
	printk("ASE Codec Reconfig: stream %p\n", stream);

	print_codec(codec);
	*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, BT_BAP_ASCS_REASON_NONE);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_qos(struct bt_bap_stream *stream, const struct bt_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp)
{
	printk("QoS: stream %p qos %p\n", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
		      size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_count %zu\n", stream, meta_count);

	return 0;
}

static int lc3_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
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
	case BT_AUDIO_METADATA_TYPE_VENDOR:   /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO:     /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

static int lc3_metadata(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_count %zu\n", stream, meta_count);

	for (size_t i = 0; i < meta_count; i++) {
		const struct bt_codec_data *data = &meta[i];

		if (!valid_metadata_type(data->data.type, data->data.data_len)) {
			printk("Invalid metadata type %u or length %u\n", data->data.type,
			       data->data.data_len);
			*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED,
					       data->data.type);
			return -EINVAL;
		}
	}

	return 0;
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

static void stream_recv(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			struct net_buf *buf)
{
	printk("Incoming audio on stream %p len %u\n", stream, buf->len);
}

static struct bt_bap_stream_ops stream_ops = {
	.enabled = stream_enabled_cb,
	.recv = stream_recv
};

static void init(void)
{
	static struct bt_pacs_cap cap = {
		.codec = &lc3_codec,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

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

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_bap_stream_cb_register(&streams[i], &stream_ops);
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, unicast_server_ad, ARRAY_SIZE(unicast_server_ad),
			      NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
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

static void test_main(void)
{
	init();

	set_location();
	set_available_contexts();

	/* TODO: When babblesim supports ISO, wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_stream_configured);
	PASS("Unicast server passed\n");
}

static const struct bst_test_instance test_unicast_server[] = {{.test_id = "unicast_server",
								.test_post_init_f = test_init,
								.test_tick_f = test_tick,
								.test_main_f = test_main},
							       BSTEST_END_MARKER};

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
