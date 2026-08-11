/*
 * Copyright (c) 2023-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/ascs.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/bluetooth/audio/gmap_lc3_preset.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bap_stream_tx.h"
#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"
#include "bap_common.h"

LOG_MODULE_REGISTER(gmap_ugg_test);

#if defined(CONFIG_BT_GMAP)

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_GAME)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

#define GMAP_BROADCAST_AC_MAX_STREAM 2U

extern enum bst_result_t bst_result;
static const struct named_lc3_preset *snk_named_preset;
static const struct named_lc3_preset *src_named_preset;
static const struct named_lc3_preset *broadcast_named_preset;

struct gmap_broadcast_ac_param {
	char *name;
	size_t stream_cnt;
	size_t chan_cnt;
	const struct named_lc3_preset *named_preset;
};

static struct named_lc3_preset gmap_unicast_snk_presets[] = {
	{"32_1_gr", BT_GMAP_LC3_PRESET_32_1_GR(LOCATION, CONTEXT)},
	{"32_2_gr", BT_GMAP_LC3_PRESET_32_2_GR(LOCATION, CONTEXT)},
	{"48_1_gr", BT_GMAP_LC3_PRESET_48_1_GR(LOCATION, CONTEXT)},
	{"48_2_gr", BT_GMAP_LC3_PRESET_48_2_GR(LOCATION, CONTEXT)},
	{"48_3_gr", BT_GMAP_LC3_PRESET_48_3_GR(LOCATION, CONTEXT)},
	{"48_4_gr", BT_GMAP_LC3_PRESET_48_4_GR(LOCATION, CONTEXT)},
};

static struct named_lc3_preset gmap_unicast_src_presets[] = {
	{"16_1_gs", BT_GMAP_LC3_PRESET_16_1_GS(LOCATION, CONTEXT)},
	{"16_2_gs", BT_GMAP_LC3_PRESET_16_2_GS(LOCATION, CONTEXT)},
	{"32_1_gs", BT_GMAP_LC3_PRESET_32_1_GS(LOCATION, CONTEXT)},
	{"32_2_gs", BT_GMAP_LC3_PRESET_32_2_GS(LOCATION, CONTEXT)},
	{"48_1_gs", BT_GMAP_LC3_PRESET_48_1_GS(LOCATION, CONTEXT)},
	{"48_2_gs", BT_GMAP_LC3_PRESET_48_2_GS(LOCATION, CONTEXT)},
};

static struct named_lc3_preset gmap_broadcast_presets[] = {
	{"48_1_g", BT_GMAP_LC3_PRESET_48_1_G(LOCATION, CONTEXT)},
	{"48_2_g", BT_GMAP_LC3_PRESET_48_2_G(LOCATION, CONTEXT)},
	{"48_3_g", BT_GMAP_LC3_PRESET_48_3_G(LOCATION, CONTEXT)},
	{"48_4_g", BT_GMAP_LC3_PRESET_48_4_G(LOCATION, CONTEXT)},
};

struct named_lc3_preset named_preset;

static struct audio_test_stream broadcast_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];

static K_SEM_DEFINE(sem_stream_started, 0U, ARRAY_SIZE(broadcast_streams));
static K_SEM_DEFINE(sem_stream_stopped, 0U, ARRAY_SIZE(broadcast_streams));

CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_gmap_discovered);

const struct named_lc3_preset *gmap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						     const char *preset_arg)
{
	if (is_unicast) {
		if (dir == BT_AUDIO_DIR_SINK) {
			for (size_t i = 0U; i < ARRAY_SIZE(gmap_unicast_snk_presets); i++) {
				if (!strcmp(preset_arg, gmap_unicast_snk_presets[i].name)) {
					return &gmap_unicast_snk_presets[i];
				}
			}
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			for (size_t i = 0U; i < ARRAY_SIZE(gmap_unicast_src_presets); i++) {
				if (!strcmp(preset_arg, gmap_unicast_src_presets[i].name)) {
					return &gmap_unicast_src_presets[i];
				}
			}
		}
	} else {

		for (size_t i = 0U; i < ARRAY_SIZE(gmap_broadcast_presets); i++) {
			if (!strcmp(preset_arg, gmap_broadcast_presets[i].name)) {
				return &gmap_broadcast_presets[i];
			}
		}
	}

	return NULL;
}

static void stream_codec_configured_cb(struct bt_bap_stream *stream,
				       const struct bt_bap_qos_cfg_pref *pref)
{
	ARG_UNUSED(pref);

	LOG_INF("Configured stream %p", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */
}

static void stream_qos_configured_cb(struct bt_bap_stream *stream)
{
	LOG_INF("QoS set stream %p", stream);
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Enabled stream %p", stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	memset(&test_stream->last_info, 0, sizeof(test_stream->last_info));
	test_stream->rx_cnt = 0U;
	test_stream->valid_rx_cnt = 0U;
	test_stream->seq_num = 0U;
	test_stream->tx_cnt = 0U;
	UNSET_FLAG(test_stream->flag_audio_received);

	LOG_INF("Started stream %p", stream);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_register(stream);
		if (err != 0) {
			FAIL("Failed to register stream %p for TX: %d\n", stream, err);
			return;
		}
	}

	k_sem_give(&sem_stream_started);
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Metadata updated stream %p", stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Disabled stream %p", stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_INF("Stream %p stopped with reason 0x%02X", stream, reason);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_unregister(stream);
		if (err != 0) {
			FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
			return;
		}
	}

	k_sem_give(&sem_stream_stopped);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Released stream %p", stream);
}

static struct bt_bap_stream_ops stream_ops = {
	.codec_configured = stream_codec_configured_cb,
	.qos_configured = stream_qos_configured_cb,
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
	.sent = bap_stream_tx_sent_cb,
	.recv = bap_stream_rx_recv_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(tx);
	ARG_UNUSED(rx);

	LOG_INF("MTU exchanged");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void gmap_discover_cb(struct bt_conn *conn, int err, enum bt_gmap_role role,
			     struct bt_gmap_feat features)
{
	enum bt_gmap_ugt_feat ugt_feat;

	if (err != 0) {
		FAIL("gmap discovery (err %d)\n", err);
		return;
	}

	LOG_INF("GMAP discovered for conn %p:\trole 0x%02x\tugg_feat 0x%02x\tugt_feat "
	       "0x%02x\tbgs_feat 0x%02x\tbgr_feat 0x%02x",
	       conn, role, features.ugg_feat, features.ugt_feat, features.bgs_feat,
	       features.bgr_feat);

	if ((role & BT_GMAP_ROLE_UGT) == 0) {
		FAIL("Remote GMAP device is not a UGT\n");
		return;
	}

	ugt_feat = features.ugt_feat;
	if ((ugt_feat & BT_GMAP_UGT_FEAT_SOURCE) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_80KBPS_SOURCE) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_SINK) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_64KBPS_SINK) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_MULTIPLEX) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_MULTISINK) == 0 ||
	    (ugt_feat & BT_GMAP_UGT_FEAT_MULTISOURCE) == 0) {
		FAIL("Remote GMAP device does not have expected UGT features: %d\n", ugt_feat);
		return;
	}

	SET_FLAG(flag_gmap_discovered);
}

static const struct bt_gmap_cb gmap_cb = {
	.discover = gmap_discover_cb,
};

static void init(void)
{
	const struct bt_gmap_feat features = {
		.ugg_feat = (BT_GMAP_UGG_FEAT_MULTIPLEX | BT_GMAP_UGG_FEAT_96KBPS_SOURCE |
			     BT_GMAP_UGG_FEAT_MULTISINK),
	};
	const enum bt_gmap_role role = BT_GMAP_ROLE_UGG;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	LOG_INF("Bluetooth initialized");
	bap_stream_tx_init();

	bt_gatt_cb_register(&gatt_callbacks);
	err = bt_le_scan_cb_register(&common_scan_cb);
	if (err != 0) {
		FAIL("Failed to register scan callbacks (err %d)\n", err);
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_streams); i++) {
		bt_cap_stream_ops_register(cap_stream_from_audio_test_stream(&broadcast_streams[i]),
					   &stream_ops);
	}

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
}

static void deinit(void)
{
	int err;

	err = bt_gatt_cb_unregister(&gatt_callbacks);
	if (err != 0) {
		FAIL("Failed to unregister GATT callbacks (err %d)\n", err);
		return;
	}
}

static void scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Scanning successfully started");
	WAIT_FOR_FLAG(flag_connected);
}

static void discover_gmas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_gmap_discovered);

	err = bt_gmap_discover(conn);
	if (err != 0) {
		LOG_ERR("Failed to discover GMAS: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_gmap_discovered);
}

static void test_gmap(void)
{
	int err;

	init();

	UNSET_FLAG(flag_mtu_exchanged);

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	LOG_INF("Connected");

	update_security(default_conn);

	discover_gmas(default_conn);
	discover_gmas(default_conn); /* test that we can discover twice */

	backchannel_sync_send_all();

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect: %d\n", err);
		return;
	}

	PASS("GMAP UGG passed\n");
}

static void setup_extended_adv_data(struct bt_cap_broadcast_source *source,
				    struct bt_le_ext_adv *adv)
{
	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	uint32_t broadcast_id;
	int err;

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err != 0) {
		FAIL("Unable to generate broadcast ID: %d\n", err);
		return;
	}

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len;
	ext_ad.data = ad_buf.data;
	err = bt_le_ext_adv_set_data(adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		FAIL("Failed to set extended advertising data: %d\n", err);
		return;
	}

	/* Setup periodic advertising data */
	err = bt_cap_initiator_broadcast_get_base(source, &base_buf);
	if (err != 0) {
		FAIL("Failed to get encoded BASE: %d\n", err);
		return;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (err != 0) {
		FAIL("Failed to set periodic advertising data: %d\n", err);
		return;
	}
}

static void stop_and_delete_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Stop extended advertising */
	err = bt_le_per_adv_stop(adv);
	if (err != 0) {
		FAIL("Failed to stop periodic advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err != 0) {
		FAIL("Failed to stop extended advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err != 0) {
		FAIL("Failed to delete extended advertising: %d\n", err);
		return;
	}
}

static void broadcast_audio_start(struct bt_cap_broadcast_source *broadcast_source,
				  struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_cap_initiator_broadcast_audio_start(broadcast_source, adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	LOG_INF("Broadcast source created");
}

static void broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source,
				 size_t stream_count)
{
	int err;

	LOG_INF("Stopping broadcast source");

	err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	LOG_INF("Waiting for broadcast_streams to be stopped");
	for (size_t i = 0U; i < stream_count; i++) {
		err = k_sem_take(&sem_stream_stopped, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
	}

	LOG_INF("Broadcast source stopped");
}

static void broadcast_audio_delete(struct bt_cap_broadcast_source *broadcast_source)
{
	int err;

	LOG_INF("Deleting broadcast source");

	err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	LOG_INF("Broadcast source deleted");
}

static int test_gmap_ugg_broadcast_ac(const struct gmap_broadcast_ac_param *param)
{
	uint8_t stereo_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
				    BT_AUDIO_LOCATION_FRONT_RIGHT | BT_AUDIO_LOCATION_FRONT_LEFT)};
	uint8_t right_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC, BT_AUDIO_LOCATION_FRONT_RIGHT)};
	uint8_t left_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC, BT_AUDIO_LOCATION_FRONT_LEFT)};
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	struct bt_cap_initiator_broadcast_create_param create_param = {0};
	struct bt_cap_initiator_broadcast_stream_param
		stream_params[GMAP_BROADCAST_AC_MAX_STREAM] = {0};
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_qos_cfg qos;
	struct bt_le_ext_adv *adv;
	int err;

	for (size_t i = 0U; i < param->stream_cnt; i++) {
		stream_params[i].stream = cap_stream_from_audio_test_stream(&broadcast_streams[i]);

		if (param->stream_cnt == 1U) {
			stream_params[i].data_len = ARRAY_SIZE(stereo_data);
			stream_params[i].data = stereo_data;
		} else if (i == 0U) {
			stream_params[i].data_len = ARRAY_SIZE(left_data);
			stream_params[i].data = left_data;
		} else if (i == 1U) {
			stream_params[i].data_len = ARRAY_SIZE(right_data);
			stream_params[i].data = right_data;
		}
	}

	memcpy(&codec_cfg, &broadcast_named_preset->preset.codec_cfg, sizeof(codec_cfg));
	memcpy(&qos, &broadcast_named_preset->preset.qos, sizeof(qos));
	qos.sdu *= param->chan_cnt;

	subgroup_param.stream_count = param->stream_cnt;
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec_cfg = &codec_cfg;
	create_param.subgroup_count = 1U;
	create_param.subgroup_params = &subgroup_param;
	create_param.qos = &qos;

	init();
	setup_broadcast_adv(&adv);

	err = bt_cap_initiator_broadcast_audio_create(&create_param, &broadcast_source);
	if (err != 0) {
		FAIL("Failed to create broadcast source: %d\n", err);
		return -ENOEXEC;
	}

	for (size_t i = 0U; i < param->stream_cnt; i++) {
		struct audio_test_stream *test_stream = &broadcast_streams[i];

		test_stream->tx_sdu_size = create_param.qos->sdu;
	}

	broadcast_audio_start(broadcast_source, adv);
	setup_extended_adv_data(broadcast_source, adv);
	start_broadcast_adv(adv);

	/* Wait for all to be started */
	LOG_INF("Waiting for broadcast_streams to be started");
	for (size_t i = 0U; i < param->stream_cnt; i++) {
		err = k_sem_take(&sem_stream_started, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
	}

	/* Wait for other devices to have received what they wanted */
	backchannel_sync_wait_any();

	broadcast_audio_stop(broadcast_source, param->stream_cnt);

	broadcast_audio_delete(broadcast_source);
	broadcast_source = NULL;

	stop_and_delete_extended_adv(adv);
	adv = NULL;

	deinit();

	PASS("CAP initiator broadcast passed\n");

	return 0;
}

static void test_gmap_ac(const struct cap_unicast_ac_param *param)
{
	const struct bt_gmap_feat features = {
		.ugg_feat = (BT_GMAP_UGG_FEAT_MULTIPLEX | BT_GMAP_UGG_FEAT_96KBPS_SOURCE |
			     BT_GMAP_UGG_FEAT_MULTISINK),
	};
	const enum bt_gmap_role role = BT_GMAP_ROLE_UGG;
	int err;

	err = bt_gmap_register(role, features);
	if (err != 0) {
		FAIL("Failed to register GMAS (err %d)\n", err);

		return;
	}

	test_cap_initiator_unicast_ac(param);
}

static void test_gmap_ac_1(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_1",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_MONO_AUDIO,
		.conn_param[0].cis_param[0].has_src = false,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_2(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_2",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = false,
		.conn_param[0].cis_param[0].has_src = true,
		.conn_param[0].cis_param[0].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = NULL,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_3(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_3",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_MONO_AUDIO,
		.conn_param[0].cis_param[0].has_src = true,
		.conn_param[0].cis_param[0].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_4(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_4",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc =
			BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[0].cis_param[0].has_src = false,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_5(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_5",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc =
			BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[0].cis_param[0].has_src = true,
		.conn_param[0].cis_param[0].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_6_i(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_6_i",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 2U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = false,

		.conn_param[0].cis_param[1].has_snk = true,
		.conn_param[0].cis_param[1].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[0].cis_param[1].has_src = false,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_6_ii(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_6_ii",
		.conn_cnt = 2U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = false,

		.conn_param[1].cis_cnt = 1U,

		.conn_param[1].cis_param[0].has_snk = true,
		.conn_param[1].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[1].cis_param[0].has_src = false,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_7_ii(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_7_ii",
		.conn_cnt = 2U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_MONO_AUDIO,
		.conn_param[0].cis_param[0].has_src = false,

		.conn_param[1].cis_cnt = 1U,

		.conn_param[1].cis_param[0].has_snk = false,
		.conn_param[1].cis_param[0].has_src = true,
		.conn_param[1].cis_param[0].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_8_i(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_8_i",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 2U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = false,

		.conn_param[0].cis_param[1].has_snk = true,
		.conn_param[0].cis_param[1].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[0].cis_param[1].has_src = true,
		.conn_param[0].cis_param[1].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_8_ii(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_8_ii",
		.conn_cnt = 2U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = false,

		.conn_param[1].cis_cnt = 1U,

		.conn_param[1].cis_param[0].has_snk = true,
		.conn_param[1].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[1].cis_param[0].has_src = true,
		.conn_param[1].cis_param[0].src_loc = BT_AUDIO_LOCATION_MONO_AUDIO,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_11_i(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_11_i",
		.conn_cnt = 1U,

		.conn_param[0].cis_cnt = 2U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = true,
		.conn_param[0].cis_param[0].src_loc = BT_AUDIO_LOCATION_FRONT_LEFT,

		.conn_param[0].cis_param[1].has_snk = true,
		.conn_param[0].cis_param[1].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[0].cis_param[1].has_src = true,
		.conn_param[0].cis_param[1].src_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_11_ii(void)
{
	const struct cap_unicast_ac_param param = {
		.name = "ac_11_ii",
		.conn_cnt = 2U,

		.conn_param[0].cis_cnt = 1U,

		.conn_param[0].cis_param[0].has_snk = true,
		.conn_param[0].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_LEFT,
		.conn_param[0].cis_param[0].has_src = true,
		.conn_param[0].cis_param[0].src_loc = BT_AUDIO_LOCATION_FRONT_LEFT,

		.conn_param[1].cis_cnt = 1U,

		.conn_param[1].cis_param[0].has_snk = true,
		.conn_param[1].cis_param[0].snk_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,
		.conn_param[1].cis_param[0].has_src = true,
		.conn_param[1].cis_param[0].src_loc = BT_AUDIO_LOCATION_FRONT_RIGHT,

		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ac(&param);
}

static void test_gmap_ac_12(void)
{
	const struct gmap_broadcast_ac_param param = {
		.name = "ac_12",
		.stream_cnt = 1U,
		.chan_cnt = 1U,
		.named_preset = broadcast_named_preset,
	};

	test_gmap_ugg_broadcast_ac(&param);
}

#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= GMAP_BROADCAST_AC_MAX_STREAM
static void test_gmap_ac_13(void)
{
	const struct gmap_broadcast_ac_param param = {
		.name = "ac_13",
		.stream_cnt = 2U,
		.chan_cnt = 1U,
		.named_preset = broadcast_named_preset,
	};

	test_gmap_ugg_broadcast_ac(&param);
}
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= GMAP_BROADCAST_AC_MAX_STREAM */

static void test_gmap_ac_14(void)
{
	const struct gmap_broadcast_ac_param param = {
		.name = "ac_14",
		.stream_cnt = 1U,
		.chan_cnt = 2U,
		.named_preset = broadcast_named_preset,
	};

	test_gmap_ugg_broadcast_ac(&param);
}

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0U; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "sink_preset") == 0) {
			argn++;
			const char *preset_arg = argv[argn];

			snk_named_preset =
				gmap_get_named_preset(true, BT_AUDIO_DIR_SINK, preset_arg);
			if (snk_named_preset == NULL) {
				FAIL("Failed to get sink preset from %s\n", preset_arg);
			}
		} else if (strcmp(arg, "source_preset") == 0) {
			argn++;
			const char *preset_arg = argv[argn];

			src_named_preset =
				gmap_get_named_preset(true, BT_AUDIO_DIR_SOURCE, preset_arg);
			if (src_named_preset == NULL) {
				FAIL("Failed to get source preset from %s\n", preset_arg);
			}
		} else if (strcmp(arg, "broadcast_preset") == 0) {
			argn++;
			const char *preset_arg = argv[argn];

			broadcast_named_preset = gmap_get_named_preset(
				false, BT_AUDIO_DIR_SINK /* unused */, preset_arg);
			if (broadcast_named_preset == NULL) {
				FAIL("Failed to get broadcast preset from %s\n", preset_arg);
			}
		} else {
			FAIL("Invalid arg: %s\n", arg);
		}
	}
}

static const struct bst_test_instance test_gmap_ugg[] = {
	{
		.test_id = "gmap_ugg",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_1",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_1,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_2",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_2,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_3",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_3,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_4",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_4,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_5",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_5,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_6_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_6_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_6_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_6_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_7_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_7_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_8_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_8_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_8_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_8_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_11_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_11_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_11_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_11_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "gmap_ugg_ac_12",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_12,
		.test_args_f = test_args,
	},
#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= GMAP_BROADCAST_AC_MAX_STREAM
	{
		.test_id = "gmap_ugg_ac_13",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_13,
		.test_args_f = test_args,
	},
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT >= GMAP_BROADCAST_AC_MAX_STREAM */
	{
		.test_id = "gmap_ugg_ac_14",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_gmap_ac_14,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gmap_ugg_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gmap_ugg);
}

#else /* !(CONFIG_BT_GMAP) */

struct bst_test_list *test_gmap_ugg_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_GMAP */
