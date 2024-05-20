/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_ACCEPTOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include "common.h"
#include "bap_common.h"

extern enum bst_result_t bst_result;

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_GAME)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

static uint8_t csis_rank = 1;

static struct bt_audio_codec_cap codec_cap =
	BT_AUDIO_CODEC_CAP_LC3(BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_ANY,
			       BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2, CONTEXT);

static const struct bt_audio_codec_qos_pref unicast_qos_pref =
	BT_AUDIO_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, 0U, 60U, 10000U, 60000U, 10000U, 60000U);

#define UNICAST_CHANNEL_COUNT_1 BIT(0)

static struct bt_cap_stream
	unicast_streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];

CREATE_FLAG(flag_unicast_stream_started);
CREATE_FLAG(flag_gmap_discovered);

static void unicast_stream_enabled_cb(struct bt_bap_stream *stream)
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

static void unicast_stream_started_cb(struct bt_bap_stream *stream)
{
	printk("Started: stream %p\n", stream);
	SET_FLAG(flag_unicast_stream_started);
}

static struct bt_bap_stream_ops unicast_stream_ops = {
	.enabled = unicast_stream_enabled_cb,
	.started = unicast_stream_started_cb,
};

/* TODO: Expand with GMAP service data */
static const struct bt_data gmap_acceptor_ad[] = {
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

static bool data_func_cb(struct bt_data *data, void *user_data)
{
	struct bt_bap_ascs_rsp *rsp = (struct bt_bap_ascs_rsp *)user_data;

	if (!BT_AUDIO_METADATA_TYPE_IS_KNOWN(data->type)) {
		printk("Invalid metadata type %u or length %u\n", data->type, data->data_len);
		*rsp = BT_BAP_ASCS_RSP(BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED, data->type);

		return -EINVAL;
	}

	return true;
}

static int unicast_server_enable(struct bt_bap_stream *stream, const uint8_t meta[],
				 size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	printk("Enable: stream %p meta_len %zu\n", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
}

static int unicast_server_start(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	printk("Start: stream %p\n", stream);

	return 0;
}

static int unicast_server_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len, struct bt_bap_ascs_rsp *rsp)
{
	printk("Metadata: stream %p meta_len %zu\n", stream, meta_len);

	return bt_audio_data_parse(meta, meta_len, data_func_cb, rsp);
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
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, LOCATION);
		if (err != 0) {
			FAIL("Failed to set sink location (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC_LOC)) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, LOCATION);
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
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, CONTEXT);
		if (err != 0) {
			printk("Failed to set sink supported contexts (err %d)\n", err);

			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, CONTEXT);
		if (err != 0) {
			printk("Failed to set source supported contexts (err %d)\n", err);

			return err;
		}
	}

	printk("Supported contexts successfully set\n");

	return 0;
}

static void set_available_contexts(void)
{
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, CONTEXT);
	if (IS_ENABLED(CONFIG_BT_PAC_SNK) && err != 0) {
		FAIL("Failed to set sink available contexts (err %d)\n", err);
		return;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, CONTEXT);
	if (IS_ENABLED(CONFIG_BT_PAC_SRC) && err != 0) {
		FAIL("Failed to set source available contexts (err %d)\n", err);
		return;
	}

	printk("Available contexts successfully set\n");
}

static void gmap_discover_cb(struct bt_conn *conn, int err, enum bt_gmap_role role,
			     struct bt_gmap_feat features)
{
	enum bt_gmap_ugg_feat ugg_feat;

	if (err != 0) {
		FAIL("GMAP discovery (err %d)\n", err);
		return;
	}

	printk("GMAP discovered for conn %p:\n\trole 0x%02x\n\tugg_feat 0x%02x\n\tugt_feat "
	       "0x%02x\n\tbgs_feat 0x%02x\n\tbgr_feat 0x%02x\n",
	       conn, role, features.ugg_feat, features.ugt_feat, features.bgs_feat,
	       features.bgr_feat);

	if ((role & BT_GMAP_ROLE_UGG) == 0) {
		FAIL("Remote GMAP device is not a UGG\n");
		return;
	}

	ugg_feat = features.ugg_feat;
	if ((ugg_feat & BT_GMAP_UGG_FEAT_MULTIPLEX) == 0 ||
	    (ugg_feat & BT_GMAP_UGG_FEAT_96KBPS_SOURCE) == 0 ||
	    (ugg_feat & BT_GMAP_UGG_FEAT_MULTISINK) == 0) {
		FAIL("Remote GMAP device does not have expected UGG features: %d\n", ugg_feat);
		return;
	}

	SET_FLAG(flag_gmap_discovered);
}

static const struct bt_gmap_cb gmap_cb = {
	.discover = gmap_discover_cb,
};

static void discover_gmas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_gmap_discovered);

	err = bt_gmap_discover(conn);
	if (err != 0) {
		printk("Failed to discover GMAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_gmap_discovered);
}

static void test_main(void)
{
	/* TODO: Register all GMAP codec capabilities */
	const enum bt_gmap_role role = BT_GMAP_ROLE_UGT;
	const struct bt_gmap_feat features = {
		.ugt_feat = (BT_GMAP_UGT_FEAT_SOURCE | BT_GMAP_UGT_FEAT_80KBPS_SOURCE |
			     BT_GMAP_UGT_FEAT_SINK | BT_GMAP_UGT_FEAT_64KBPS_SINK |
			     BT_GMAP_UGT_FEAT_MULTIPLEX | BT_GMAP_UGT_FEAT_MULTISINK |
			     BT_GMAP_UGT_FEAT_MULTISOURCE),
	};
	static struct bt_pacs_cap unicast_cap = {
		.codec_cap = &codec_cap,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		const struct bt_csip_set_member_register_param csip_set_member_param = {
			.set_size = 2,
			.rank = csis_rank,
			.lockable = true,
			/* Using the CSIP_SET_MEMBER test sample SIRK */
			.set_sirk = {0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce, 0x22, 0xfd,
				     0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45},
		};

		err = bt_cap_acceptor_register(&csip_set_member_param, &csip_set_member);
		if (err != 0) {
			FAIL("CAP acceptor failed to register (err %d)\n", err);
			return;
		}
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &unicast_cap);
	if (err != 0) {
		FAIL("Capability register failed (err %d)\n", err);

		return;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &unicast_cap);
	if (err != 0) {
		FAIL("Capability register failed (err %d)\n", err);

		return;
	}

	err = bt_bap_unicast_server_register_cb(&unicast_server_cbs);
	if (err != 0) {
		FAIL("Failed to register unicast server callbacks (err %d)\n", err);

		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		bt_cap_stream_ops_register(&unicast_streams[i], &unicast_stream_ops);
	}

	set_supported_contexts();
	set_available_contexts();
	set_location();

	err = bt_gmap_register(role, features);
	if (err != 0) {
		FAIL("Failed to register GMAS (err %d)\n", err);

		return;
	}

	err = bt_gmap_cb_register(&gmap_cb);
	if (err != 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);

		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, gmap_acceptor_ad, ARRAY_SIZE(gmap_acceptor_ad),
			      NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	discover_gmas(default_conn);
	discover_gmas(default_conn); /* test that we can discover twice */

	WAIT_FOR_FLAG(flag_disconnected);

	PASS("GMAP UGT passed\n");
}

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "rank") == 0) {
			csis_rank = strtoul(argv[++argn], NULL, 10);
		} else {
			FAIL("Invalid arg: %s\n", arg);
		}
	}
}

static const struct bst_test_instance test_gmap_ugt[] = {
	{
		.test_id = "gmap_ugt",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gmap_ugt_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gmap_ugt);
}

#else /* !(CONFIG_BT_CAP_ACCEPTOR) */

struct bst_test_list *test_gmap_ugt_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_ACCEPTOR */
