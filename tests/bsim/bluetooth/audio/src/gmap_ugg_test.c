/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/bluetooth/audio/gmap_lc3_preset.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bstests.h"
#include "common.h"
#include "bap_common.h"

#if defined(CONFIG_BT_GMAP)
/* Zephyr Controller works best while Extended Advertising interval to be a multiple
 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
 * Broadcast ISO radio events.
 */
#define BT_LE_EXT_ADV_CUSTOM \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, \
				0x0080, 0x0080, NULL)

#define BT_LE_PER_ADV_CUSTOM \
		BT_LE_PER_ADV_PARAM(0x0048, \
				    0x0048, \
				    BT_LE_PER_ADV_OPT_NONE)

#define UNICAST_SINK_SUPPORTED (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0)
#define UNICAST_SRC_SUPPORTED  (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0)

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_GAME)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

#define GMAP_BROADCAST_AC_MAX_STREAM 2
#define GMAP_UNICAST_AC_MAX_CONN     2U
#define GMAP_UNICAST_AC_MAX_SNK      (2U * GMAP_UNICAST_AC_MAX_CONN)
#define GMAP_UNICAST_AC_MAX_SRC      (2U * GMAP_UNICAST_AC_MAX_CONN)
#define GMAP_UNICAST_AC_MAX_PAIR     MAX(GMAP_UNICAST_AC_MAX_SNK, GMAP_UNICAST_AC_MAX_SRC)
#define GMAP_UNICAST_AC_MAX_STREAM   (GMAP_UNICAST_AC_MAX_SNK + GMAP_UNICAST_AC_MAX_SRC)

#define MAX_ISO_CHAN_COUNT 2U
#define ISO_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED       (ISO_ENQUEUE_COUNT * MAX_ISO_CHAN_COUNT)

BUILD_ASSERT(
	CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	"CONFIG_BT_ISO_TX_BUF_COUNT should be at least ISO_ENQUEUE_COUNT * MAX_ISO_CHAN_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool, TOTAL_BUF_NEEDED, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

extern enum bst_result_t bst_result;
static const struct named_lc3_preset *snk_named_preset;
static const struct named_lc3_preset *src_named_preset;
static const struct named_lc3_preset *broadcast_named_preset;

struct gmap_unicast_ac_param {
	char *name;
	size_t conn_cnt;
	size_t snk_cnt[GMAP_UNICAST_AC_MAX_CONN];
	size_t src_cnt[GMAP_UNICAST_AC_MAX_CONN];
	size_t snk_chan_cnt;
	const struct named_lc3_preset *snk_named_preset;
	const struct named_lc3_preset *src_named_preset;
};
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
static struct unicast_stream unicast_streams[GMAP_UNICAST_AC_MAX_STREAM];
static struct bt_cap_stream *started_unicast_streams[GMAP_UNICAST_AC_MAX_STREAM];
static size_t started_unicast_streams_cnt;
static struct bt_bap_ep
	*sink_eps[GMAP_UNICAST_AC_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep
	*source_eps[GMAP_UNICAST_AC_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_conn *connected_conns[GMAP_UNICAST_AC_MAX_CONN];
static size_t connected_conn_cnt;

static K_SEM_DEFINE(sem_stream_started, 0U,
		    MAX(ARRAY_SIZE(unicast_streams), ARRAY_SIZE(broadcast_streams)));
static K_SEM_DEFINE(sem_stream_stopped, 0U,
		    MAX(ARRAY_SIZE(unicast_streams), ARRAY_SIZE(broadcast_streams)));

CREATE_FLAG(flag_cas_discovered);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_updated);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_source_discovered);
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

static void stream_sent_cb(struct bt_bap_stream *bap_stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(bap_stream);
	struct bt_cap_stream *cap_stream = cap_stream_from_audio_test_stream(test_stream);
	struct net_buf *buf;
	int ret;

	if (!test_stream->tx_active) {
		return;
	}

	if ((test_stream->tx_cnt % 100U) == 0U) {
		printk("[%zu]: Stream %p sent with seq_num %u\n", test_stream->tx_cnt, cap_stream,
		       test_stream->seq_num);
	}

	if (test_stream->tx_sdu_size > CONFIG_BT_ISO_TX_MTU) {
		FAIL("Invalid SDU %u for the MTU: %d", test_stream->tx_sdu_size,
		     CONFIG_BT_ISO_TX_MTU);
		return;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n", bap_stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, mock_iso_data, test_stream->tx_sdu_size);
	ret = bt_cap_stream_send(cap_stream, buf, test_stream->seq_num++);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		net_buf_unref(buf);

		/* Only fail if tx is active (may fail if we are disabling the stream) */
		if (test_stream->tx_active) {
			FAIL("Unable to broadcast data on %p: %d\n", cap_stream, ret);
		}

		return;
	}

	test_stream->tx_cnt++;
}

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_qos_pref *pref)
{
	printk("Configured stream %p\n", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	printk("QoS set stream %p\n", stream);
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	printk("Enabled stream %p\n", stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	printk("Started stream %p\n", stream);
	k_sem_give(&sem_stream_started);
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	printk("Metadata updated stream %p\n", stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	printk("Disabled stream %p\n", stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);
	k_sem_give(&sem_stream_stopped);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	printk("Released stream %p\n", stream);
}

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured_cb,
	.qos_set = stream_qos_set_cb,
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
	.sent = stream_sent_cb,
};

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Failed to discover CAS: %d\n", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			FAIL("Failed to discover CAS CSIS\n");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_cas_discovered);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to start (failing conn %p): %d\n", conn, err);

		return;
	}

	SET_FLAG(flag_started);
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to update (failing conn %p): %d\n", conn, err);

		return;
	}

	SET_FLAG(flag_updated);
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to stop (failing conn %p): %d\n", conn, err);

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

static void add_remote_sink_ep(struct bt_conn *conn, struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sink_eps[bt_conn_index(conn)]); i++) {
		if (sink_eps[bt_conn_index(conn)][i] == NULL) {
			printk("Conn %p: Sink #%zu: ep %p\n", conn, i, ep);
			sink_eps[bt_conn_index(conn)][i] = ep;
			break;
		}
	}
}

static void add_remote_source_ep(struct bt_conn *conn, struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(source_eps[bt_conn_index(conn)]); i++) {
		if (source_eps[bt_conn_index(conn)][i] == NULL) {
			printk("Conn %p: Source #%zu: ep %p\n", conn, i, ep);
			source_eps[bt_conn_index(conn)][i] = ep;
			break;
		}
	}
}

static void bap_pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			      const struct bt_audio_codec_cap *codec_cap)
{
	printk("conn %p codec_cap %p dir 0x%02x\n", conn, codec_cap, dir);

	print_codec_cap(codec_cap);
}

static void bap_endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink_ep(conn, ep);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		add_remote_source_ep(conn, ep);
	}
}

static void bap_discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		FAIL("Discovery failed for dir %u: %d\n", dir, err);
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		printk("Sink discover complete\n");
		SET_FLAG(flag_sink_discovered);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		printk("Source discover complete\n");
		SET_FLAG(flag_source_discovered);
	}
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.pac_record = bap_pac_record_cb,
	.endpoint = bap_endpoint_cb,
	.discover = bap_discover_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
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

	printk("GMAP discovered for conn %p:\n\trole 0x%02x\n\tugg_feat 0x%02x\n\tugt_feat "
	       "0x%02x\n\tbgs_feat 0x%02x\n\tbgr_feat 0x%02x\n",
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

	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		FAIL("Failed to register BAP callbacks (err %d)\n", err);
		return;
	}

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		bt_cap_stream_ops_register(&unicast_streams[i].stream, &stream_ops);
	}

	for (size_t i = 0; i < ARRAY_SIZE(broadcast_streams); i++) {
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

static void gmap_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			      struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (conn != NULL) {
		/* Already connected to this device */
		bt_conn_unref(conn);
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		FAIL("RSSI too low");
		return;
	}

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return;
	}

	err = bt_conn_le_create(
		addr, BT_CONN_LE_CREATE_CONN,
		BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MIN, 0, 400),
		&connected_conns[connected_conn_cnt]);
	if (err) {
		FAIL("Could not connect to peer: %d", err);
	}
}

static void scan_and_connect(void)
{
	int err;

	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, gmap_device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
	connected_conn_cnt++;
}

static void discover_sink(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_sink_discovered);

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_sink_discovered);
}

static void discover_source(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_source_discovered);

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		printk("Failed to discover source: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_source_discovered);
}

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

static void discover_cas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_cas_discovered);

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_cas_discovered);
}

static int gmap_unicast_ac_create_unicast_group(const struct gmap_unicast_ac_param *param,
						struct unicast_stream *snk_uni_streams[],
						size_t snk_cnt,
						struct unicast_stream *src_uni_streams[],
						size_t src_cnt,
						struct bt_bap_unicast_group **unicast_group)
{
	struct bt_bap_unicast_group_stream_param
		snk_group_stream_params[GMAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_bap_unicast_group_stream_param
		src_group_stream_params[GMAP_UNICAST_AC_MAX_SRC] = {0};
	struct bt_bap_unicast_group_stream_pair_param pair_params[GMAP_UNICAST_AC_MAX_PAIR] = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t pair_cnt = 0U;

	/* Create Group
	 *
	 * First setup the individual stream parameters and then match them in pairs by connection
	 * and direction
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_group_stream_params[i].qos = &snk_uni_streams[i]->qos;
		snk_group_stream_params[i].stream = &snk_uni_streams[i]->stream.bap_stream;
	}
	for (size_t i = 0U; i < src_cnt; i++) {
		src_group_stream_params[i].qos = &src_uni_streams[i]->qos;
		src_group_stream_params[i].stream = &src_uni_streams[i]->stream.bap_stream;
	}

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0; j < MAX(param->snk_cnt[i], param->src_cnt[i]); j++) {
			if (param->snk_cnt[i] > j) {
				pair_params[pair_cnt].tx_param =
					&snk_group_stream_params[snk_stream_cnt++];
			} else {
				pair_params[pair_cnt].tx_param = NULL;
			}

			if (param->src_cnt[i] > j) {
				pair_params[pair_cnt].rx_param =
					&src_group_stream_params[src_stream_cnt++];
			} else {
				pair_params[pair_cnt].rx_param = NULL;
			}

			pair_cnt++;
		}
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params = pair_params;
	group_param.params_count = pair_cnt;

	return bt_bap_unicast_group_create(&group_param, unicast_group);
}

static int gmap_ac_cap_unicast_start(const struct gmap_unicast_ac_param *param,
				     struct unicast_stream *snk_uni_streams[], size_t snk_cnt,
				     struct unicast_stream *src_uni_streams[], size_t src_cnt,
				     struct bt_bap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_start_stream_param stream_params[GMAP_UNICAST_AC_MAX_STREAM] = {
		0};
	struct bt_audio_codec_cfg *snk_codec_cfgs[GMAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_audio_codec_cfg *src_codec_cfgs[GMAP_UNICAST_AC_MAX_SRC] = {0};
	struct bt_cap_stream *snk_cap_streams[GMAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_cap_stream *src_cap_streams[GMAP_UNICAST_AC_MAX_SRC] = {0};
	struct bt_cap_unicast_audio_start_param start_param = {0};
	struct bt_bap_ep *snk_eps[GMAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_bap_ep *src_eps[GMAP_UNICAST_AC_MAX_SRC] = {0};
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t stream_cnt = 0U;
	size_t snk_ep_cnt = 0U;
	size_t src_ep_cnt = 0U;
	int err;

	for (size_t i = 0U; i < param->conn_cnt; i++) {
#if UNICAST_SINK_SUPPORTED
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			snk_eps[snk_ep_cnt] = sink_eps[bt_conn_index(connected_conns[i])][j];
			if (snk_eps[snk_ep_cnt] == NULL) {
				FAIL("No sink[%zu][%zu] endpoint available\n", i, j);

				return -ENODEV;
			}
			snk_ep_cnt++;
		}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SRC_SUPPORTED
		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			src_eps[src_ep_cnt] = source_eps[bt_conn_index(connected_conns[i])][j];
			if (src_eps[src_ep_cnt] == NULL) {
				FAIL("No source[%zu][%zu] endpoint available\n", i, j);

				return -ENODEV;
			}
			src_ep_cnt++;
		}
#endif /* UNICAST_SRC_SUPPORTED > 0 */
	}

	if (snk_ep_cnt != snk_cnt) {
		FAIL("Sink endpoint and stream count mismatch: %zu != %zu\n", snk_ep_cnt, snk_cnt);

		return -EINVAL;
	}

	if (src_ep_cnt != src_cnt) {
		FAIL("Source endpoint and stream count mismatch: %zu != %zu\n", src_ep_cnt,
		     src_cnt);

		return -EINVAL;
	}

	/* Setup arrays of parameters based on the preset for easier access. This also copies the
	 * preset so that we can modify them (e.g. update the metadata)
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_cap_streams[i] = &snk_uni_streams[i]->stream;
		snk_codec_cfgs[i] = &snk_uni_streams[i]->codec_cfg;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_cap_streams[i] = &src_uni_streams[i]->stream;
		src_codec_cfgs[i] = &src_uni_streams[i]->codec_cfg;
	}

	/* CAP Start */
	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param =
				&stream_params[stream_cnt];

			stream_param->member.member = connected_conns[i];
			stream_param->codec_cfg = snk_codec_cfgs[snk_stream_cnt];
			stream_param->ep = snk_eps[snk_stream_cnt];
			stream_param->stream = snk_cap_streams[snk_stream_cnt];

			snk_stream_cnt++;
			stream_cnt++;

			/* If we have more than 1 connection or stream in one direction, we set the
			 * location bit accordingly
			 */
			if (param->conn_cnt > 1U || param->snk_cnt[i] > 1U) {
				const int err = bt_audio_codec_cfg_set_chan_allocation(
					stream_param->codec_cfg, (enum bt_audio_location)BIT(i));

				if (err < 0) {
					FAIL("Failed to set channel allocation: %d\n", err);
					return err;
				}
			}
		}

		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param =
				&stream_params[stream_cnt];

			stream_param->member.member = connected_conns[i];
			stream_param->codec_cfg = src_codec_cfgs[src_stream_cnt];
			stream_param->ep = src_eps[src_stream_cnt];
			stream_param->stream = src_cap_streams[src_stream_cnt];

			src_stream_cnt++;
			stream_cnt++;

			/* If we have more than 1 connection or stream in one direction, we set the
			 * location bit accordingly
			 */
			if (param->conn_cnt > 1U || param->src_cnt[i] > 1U) {
				const int err = bt_audio_codec_cfg_set_chan_allocation(
					stream_param->codec_cfg, (enum bt_audio_location)BIT(i));

				if (err < 0) {
					FAIL("Failed to set channel allocation: %d\n", err);
					return err;
				}
			}
		}
	}

	start_param.stream_params = stream_params;
	start_param.count = stream_cnt;
	start_param.type = BT_CAP_SET_TYPE_AD_HOC;

	err = bt_cap_initiator_unicast_audio_start(&start_param);
	if (err == 0) {
		for (size_t i = 0U; i < start_param.count; i++) {
			started_unicast_streams[i] = start_param.stream_params[i].stream;
		}

		started_unicast_streams_cnt = start_param.count;
	}

	return err;
}

static int gmap_ac_unicast(const struct gmap_unicast_ac_param *param,
			   struct bt_bap_unicast_group **unicast_group)
{
	/* Allocate params large enough for any params, but only use what is required */
	struct unicast_stream *snk_uni_streams[GMAP_UNICAST_AC_MAX_SNK];
	struct unicast_stream *src_uni_streams[GMAP_UNICAST_AC_MAX_SRC];
	size_t snk_cnt = 0;
	size_t src_cnt = 0;
	int err;

	if (param->conn_cnt > GMAP_UNICAST_AC_MAX_CONN) {
		FAIL("Invalid conn_cnt: %zu\n", param->conn_cnt);

		return -EINVAL;
	}

	for (size_t i = 0; i < param->conn_cnt; i++) {
		/* Verify conn values */
		if (param->snk_cnt[i] > GMAP_UNICAST_AC_MAX_SNK) {
			FAIL("Invalid conn_snk_cnt[%zu]: %zu\n", i, param->snk_cnt[i]);

			return -EINVAL;
		}

		if (param->src_cnt[i] > GMAP_UNICAST_AC_MAX_SRC) {
			FAIL("Invalid conn_src_cnt[%zu]: %zu\n", i, param->src_cnt[i]);

			return -EINVAL;
		}
	}

	/* Get total count of sink and source streams to setup */
	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			snk_cnt++;
		}

		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			src_cnt++;
		}
	}

	/* Setup arrays of parameters based on the preset for easier access. This also copies the
	 * preset so that we can modify them (e.g. update the metadata)
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_uni_streams[i] = &unicast_streams[i];

		copy_unicast_stream_preset(snk_uni_streams[i], param->snk_named_preset);

		/* Some audio configuration requires multiple sink channels,
		 * so multiply the SDU based on the channel count
		 */
		snk_uni_streams[i]->qos.sdu *= param->snk_chan_cnt;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_uni_streams[i] = &unicast_streams[i + snk_cnt];
		copy_unicast_stream_preset(src_uni_streams[i], param->src_named_preset);
	}

	err = gmap_unicast_ac_create_unicast_group(param, snk_uni_streams, snk_cnt, src_uni_streams,
						   src_cnt, unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);

		return err;
	}

	UNSET_FLAG(flag_started);

	printk("Starting %zu streams for %s\n", snk_cnt + src_cnt, param->name);
	err = gmap_ac_cap_unicast_start(param, snk_uni_streams, snk_cnt, src_uni_streams, src_cnt,
					*unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n\n", err);

		return err;
	}

	WAIT_FOR_FLAG(flag_started);

	return 0;
}

static void unicast_audio_stop(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_stop_param param;
	int err;

	UNSET_FLAG(flag_stopped);

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = started_unicast_streams_cnt;
	param.streams = started_unicast_streams;
	param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_stopped);

	started_unicast_streams_cnt = 0U;
	memset(started_unicast_streams, 0, sizeof(started_unicast_streams));
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

static void test_gmap_ugg_unicast_ac(const struct gmap_unicast_ac_param *param)
{
	struct bt_bap_unicast_group *unicast_group;

	printk("Running test for %s with Sink Preset %s and Source Preset %s\n", param->name,
	       param->snk_named_preset != NULL ? param->snk_named_preset->name : "None",
	       param->src_named_preset != NULL ? param->src_named_preset->name : "None");

	if (param->conn_cnt > GMAP_UNICAST_AC_MAX_CONN) {
		FAIL("Invalid conn_cnt: %zu\n", param->conn_cnt);
		return;
	}

	init();

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		UNSET_FLAG(flag_mtu_exchanged);

		scan_and_connect();

		WAIT_FOR_FLAG(flag_mtu_exchanged);

		printk("Connected %zu/%zu\n", i + 1, param->conn_cnt);
	}

	if (connected_conn_cnt < param->conn_cnt) {
		FAIL("Only %zu/%u connected devices, please connect additional devices for this "
		     "audio configuration\n",
		     connected_conn_cnt, param->conn_cnt);
		return;
	}

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		discover_cas(connected_conns[i]);

		if (param->snk_cnt[i] > 0U) {
			discover_sink(connected_conns[i]);
		}

		if (param->src_cnt[i] > 0U) {
			discover_source(connected_conns[i]);
		}

		discover_gmas(connected_conns[i]);
		discover_gmas(connected_conns[i]); /* test that we can discover twice */
	}

	gmap_ac_unicast(param, &unicast_group);

	unicast_audio_stop(unicast_group);

	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		const int err =
			bt_conn_disconnect(connected_conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		if (err != 0) {
			FAIL("Failed to disconnect conn[%zu]: %d\n", i, err);
		}

		bt_conn_unref(connected_conns[i]);
		connected_conns[i] = NULL;
	}

	PASS("GMAP UGG passed for %s with Sink Preset %s and Source Preset %s\n", param->name,
	     param->snk_named_preset != NULL ? param->snk_named_preset->name : "None",
	     param->src_named_preset != NULL ? param->src_named_preset->name : "None");
}

static void setup_extended_adv(struct bt_le_ext_adv **adv)
{
	int err;

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CUSTOM, NULL, adv);
	if (err != 0) {
		FAIL("Unable to create extended advertising set: %d\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_CUSTOM);
	if (err) {
		FAIL("Failed to set periodic advertising parameters: %d\n", err);
		return;
	}
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

	err = bt_cap_initiator_broadcast_get_id(source, &broadcast_id);
	if (err != 0) {
		FAIL("Unable to get broadcast ID: %d\n", err);
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

static void start_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		FAIL("Failed to start extended advertising: %d\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		FAIL("Failed to enable periodic advertising: %d\n", err);
		return;
	}
}

static void stop_and_delete_extended_adv(struct bt_le_ext_adv *adv)
{
	int err;

	/* Stop extended advertising */
	err = bt_le_per_adv_stop(adv);
	if (err) {
		FAIL("Failed to stop periodic advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		FAIL("Failed to stop extended advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
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

	printk("Broadcast source created\n");
}

static void broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source,
				 size_t stream_count)
{
	int err;

	printk("Stopping broadcast source\n");

	for (size_t i = 0U; i < ARRAY_SIZE(broadcast_streams); i++) {
		broadcast_streams[i].tx_active = false;
	}

	err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for broadcast_streams to be stopped\n");
	for (size_t i = 0U; i < stream_count; i++) {
		k_sem_take(&sem_stream_stopped, K_FOREVER);
	}

	printk("Broadcast source stopped\n");
}

static void broadcast_audio_delete(struct bt_cap_broadcast_source *broadcast_source)
{
	int err;

	printk("Deleting broadcast source\n");

	err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	printk("Broadcast source deleted\n");
}

static int test_gmap_ugg_broadcast_ac(const struct gmap_broadcast_ac_param *param)
{
	uint8_t stereo_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
				    BT_AUDIO_LOCATION_FRONT_RIGHT | BT_AUDIO_LOCATION_FRONT_LEFT)};
	uint8_t right_data[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
						    BT_AUDIO_LOCATION_FRONT_RIGHT)};
	uint8_t left_data[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
						   BT_AUDIO_LOCATION_FRONT_LEFT)};
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	struct bt_cap_initiator_broadcast_create_param create_param = {0};
	struct bt_cap_initiator_broadcast_stream_param
		stream_params[GMAP_BROADCAST_AC_MAX_STREAM] = {0};
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_audio_codec_qos qos;
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
	setup_extended_adv(&adv);

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
	start_extended_adv(adv);

	/* Wait for all to be started */
	printk("Waiting for broadcast_streams to be started\n");
	for (size_t i = 0U; i < param->stream_cnt; i++) {
		k_sem_take(&sem_stream_started, K_FOREVER);
	}

	/* Initialize sending */
	printk("Starting sending\n");
	for (size_t i = 0U; i < param->stream_cnt; i++) {
		struct audio_test_stream *test_stream = &broadcast_streams[i];

		test_stream->tx_active = true;

		for (unsigned int j = 0U; j < ISO_ENQUEUE_COUNT; j++) {
			stream_sent_cb(bap_stream_from_audio_test_stream(test_stream));
		}
	}

	/* Wait for other devices to have received what they wanted */
	backchannel_sync_wait_any();

	broadcast_audio_stop(broadcast_source, param->stream_cnt);

	broadcast_audio_delete(broadcast_source);
	broadcast_source = NULL;

	stop_and_delete_extended_adv(adv);
	adv = NULL;

	PASS("CAP initiator broadcast passed\n");

	return 0;
}

static void test_gmap_ac_1(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_1",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_2(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_2",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = NULL,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_3(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_3",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_4(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_4",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 2U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_5(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_5",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 2U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_6_i(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_6_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_6_ii(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_6_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {0U, 0U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_7_ii(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_7_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 0U},
		.src_cnt = {0U, 1U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_8_i(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_8_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_8_ii(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_8_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 0U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_11_i(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_11_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {2U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
}

static void test_gmap_ac_11_ii(void)
{
	const struct gmap_unicast_ac_param param = {
		.name = "ac_11_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 1U},
		.snk_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_gmap_ugg_unicast_ac(&param);
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
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "sink_preset") == 0) {
			const char *preset_arg = argv[++argn];

			snk_named_preset =
				gmap_get_named_preset(true, BT_AUDIO_DIR_SINK, preset_arg);
			if (snk_named_preset == NULL) {
				FAIL("Failed to get sink preset from %s\n", preset_arg);
			}
		} else if (strcmp(arg, "source_preset") == 0) {
			const char *preset_arg = argv[++argn];

			src_named_preset =
				gmap_get_named_preset(true, BT_AUDIO_DIR_SOURCE, preset_arg);
			if (src_named_preset == NULL) {
				FAIL("Failed to get source preset from %s\n", preset_arg);
			}
		} else if (strcmp(arg, "broadcast_preset") == 0) {
			const char *preset_arg = argv[++argn];

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
