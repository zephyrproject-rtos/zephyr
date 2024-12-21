/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/lc3.h>
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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "bstests.h"
#include "common.h"
#include "bap_common.h"

#if defined(CONFIG_BT_CAP_INITIATOR_UNICAST)
#define UNICAST_SINK_SUPPORTED (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0)
#define UNICAST_SRC_SUPPORTED  (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0)

#define CAP_AC_MAX_CONN   2U
#define CAP_AC_MAX_SNK    (2U * CAP_AC_MAX_CONN)
#define CAP_AC_MAX_SRC    (2U * CAP_AC_MAX_CONN)
#define CAP_AC_MAX_PAIR   MAX(CAP_AC_MAX_SNK, CAP_AC_MAX_SRC)
#define CAP_AC_MAX_STREAM (CAP_AC_MAX_SNK + CAP_AC_MAX_SRC)

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

struct cap_initiator_ac_param {
	char *name;
	size_t conn_cnt;
	size_t snk_cnt[CAP_AC_MAX_CONN];
	size_t src_cnt[CAP_AC_MAX_CONN];
	size_t snk_chan_cnt;
	size_t src_chan_cnt;
	const struct named_lc3_preset *snk_named_preset;
	const struct named_lc3_preset *src_named_preset;
};

extern enum bst_result_t bst_result;

static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct bt_cap_stream unicast_client_sink_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_cap_stream
	unicast_client_source_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_bap_ep
	*unicast_sink_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep
	*unicast_source_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct unicast_stream unicast_streams[CAP_AC_MAX_STREAM];
static struct bt_conn *connected_conns[CAP_AC_MAX_CONN];
static size_t connected_conn_cnt;
static const struct named_lc3_preset *snk_named_preset;
static const struct named_lc3_preset *src_named_preset;
static struct bt_cap_stream *non_idle_streams[CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT];
static size_t non_idle_streams_cnt;

CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_codec_found);
CREATE_FLAG(flag_endpoint_found);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_start_failed);
CREATE_FLAG(flag_start_timeout);
CREATE_FLAG(flag_updated);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_source_discovered);

static const struct named_lc3_preset lc3_unicast_presets[] = {
	{"8_1_1", BT_BAP_LC3_UNICAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1", BT_BAP_LC3_UNICAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1", BT_BAP_LC3_UNICAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1", BT_BAP_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1", BT_BAP_LC3_UNICAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1", BT_BAP_LC3_UNICAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1", BT_BAP_LC3_UNICAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1", BT_BAP_LC3_UNICAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_BAP_LC3_UNICAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_BAP_LC3_UNICAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1", BT_BAP_LC3_UNICAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1", BT_BAP_LC3_UNICAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1", BT_BAP_LC3_UNICAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1", BT_BAP_LC3_UNICAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1", BT_BAP_LC3_UNICAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1", BT_BAP_LC3_UNICAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2", BT_BAP_LC3_UNICAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2", BT_BAP_LC3_UNICAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2", BT_BAP_LC3_UNICAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2", BT_BAP_LC3_UNICAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2", BT_BAP_LC3_UNICAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2", BT_BAP_LC3_UNICAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2", BT_BAP_LC3_UNICAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2", BT_BAP_LC3_UNICAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_BAP_LC3_UNICAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_BAP_LC3_UNICAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2", BT_BAP_LC3_UNICAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2", BT_BAP_LC3_UNICAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2", BT_BAP_LC3_UNICAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2", BT_BAP_LC3_UNICAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2", BT_BAP_LC3_UNICAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2", BT_BAP_LC3_UNICAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

static void unicast_stream_configured(struct bt_bap_stream *stream,
				      const struct bt_bap_qos_cfg_pref *pref)
{
	struct bt_cap_stream *cap_stream = cap_stream_from_bap_stream(stream);
	printk("Configured stream %p\n", stream);

	for (size_t i = 0U; i < ARRAY_SIZE(non_idle_streams); i++) {
		if (non_idle_streams[i] == NULL) {
			non_idle_streams[i] = cap_stream;
			non_idle_streams_cnt++;
			return;
		}
	}

	FAIL("Could not store cap_stream in non_idle_streams\n");

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
	printk("Stopped stream %p with reason 0x%02X\n", stream, reason);
}

static void unicast_stream_released(struct bt_bap_stream *stream)
{
	struct bt_cap_stream *cap_stream = cap_stream_from_bap_stream(stream);

	printk("Released stream %p\n", stream);

	for (size_t i = 0U; i < ARRAY_SIZE(non_idle_streams); i++) {
		if (non_idle_streams[i] == cap_stream) {
			non_idle_streams[i] = NULL;
			non_idle_streams_cnt--;
			return;
		}
	}

	FAIL("Could not find cap_stream in non_idle_streams\n");
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
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Failed to discover CAS: %d", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_discovered);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err == -ECANCELED) {
		SET_FLAG(flag_start_timeout);
	} else if (err != 0) {
		printk("Failed to start (failing conn %p): %d\n", conn, err);
		SET_FLAG(flag_start_failed);
	} else {
		SET_FLAG(flag_started);
	}
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to update (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_updated);
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
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

static void add_remote_sink(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_sink_eps[conn_index]); i++) {
		if (unicast_sink_eps[conn_index][i] == NULL) {
			printk("Conn[%u] %p: Sink #%zu: ep %p\n", conn_index, conn, i, ep);
			unicast_sink_eps[conn_index][i] = ep;
			return;
		}
	}

	FAIL("Could not add sink ep\n");
}

static void add_remote_source(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	for (size_t i = 0U; i < ARRAY_SIZE(unicast_source_eps[conn_index]); i++) {
		if (unicast_source_eps[conn_index][i] == NULL) {
			printk("Conn[%u] %p: Source #%zu: ep %p\n", conn_index, conn, i, ep);
			unicast_source_eps[conn_index][i] = ep;
			return;
		}
	}

	FAIL("Could not add source ep\n");
}

static void print_remote_codec(const struct bt_audio_codec_cap *codec_cap, enum bt_audio_dir dir)
{
	printk("codec_cap %p dir 0x%02x\n", codec_cap, dir);

	print_codec_cap(codec_cap);
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	print_remote_codec(codec_cap, dir);
	SET_FLAG(flag_codec_found);
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		FAIL("Discovery failed: %d\n", err);
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		printk("Sink discover complete\n");

		SET_FLAG(flag_sink_discovered);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		printk("Source discover complete\n");

		SET_FLAG(flag_source_discovered);
	} else {
		FAIL("Invalid dir: %u\n", dir);
	}
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(conn, ep);
		SET_FLAG(flag_endpoint_found);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		add_remote_source(conn, ep);
		SET_FLAG(flag_endpoint_found);
	} else {
		FAIL("Invalid param dir: %u\n", dir);
	}
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.discover = discover_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static bool check_audio_support_and_connect_cb(struct bt_data *data, void *user_data)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *addr = user_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;
	int err;

	printk("data->type %u\n", data->type);

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		return true; /* Continue parsing to next AD data type */
	}

	/* We are looking for the CAS service data */
	uuid_val = sys_get_le16(data->data);
	uuid = BT_UUID_DECLARE_16(uuid_val);
	if (bt_uuid_cmp(uuid, BT_UUID_CAS) != 0) {
		return true; /* Continue parsing to next AD data type */
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s\n", addr_str);

	printk("Stopping scan\n");
	if (bt_le_scan_stop()) {
		FAIL("Could not stop scan");
		return false;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MIN,
						 0, BT_GAP_MS_TO_CONN_TIMEOUT(4000)),
				&connected_conns[connected_conn_cnt]);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
	}

	return false; /* Stop parsing */
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, info->addr);
	if (conn != NULL) {
		/* Already connected to this device */
		bt_conn_unref(conn);
		return;
	}

	/* Check for connectable, extended advertising */
	if (((info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0) &&
	    ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) != 0) {
		/* Check for TMAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect_cb, (void *)info->addr);
	}
}

static void init(void)
{
	static struct bt_le_scan_cb scan_callbacks = {
		.recv = scan_recv_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);
	err = bt_le_scan_cb_register(&scan_callbacks);
	if (err != 0) {
		FAIL("Failed to register scan callbacks (err %d)\n", err);
		return;
	}

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		FAIL("Failed to register BAP unicast client callbacks (err %d)\n", err);
		return;
	}

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_client_sink_streams); i++) {
		bt_cap_stream_ops_register(&unicast_client_sink_streams[i], &unicast_stream_ops);
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_client_source_streams); i++) {
		bt_cap_stream_ops_register(&unicast_client_source_streams[i], &unicast_stream_ops);
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_streams); i++) {
		bt_cap_stream_ops_register(&unicast_streams[i].stream, &unicast_stream_ops);
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

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
	connected_conn_cnt++;
}

static void discover_sink(struct bt_conn *conn)
{
	const uint8_t conn_index = bt_conn_index(conn);
	int err;

	UNSET_FLAG(flag_sink_discovered);
	UNSET_FLAG(flag_codec_found);
	UNSET_FLAG(flag_endpoint_found);

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	memset(unicast_sink_eps[conn_index], 0, sizeof(unicast_sink_eps[conn_index]));

	WAIT_FOR_FLAG(flag_sink_discovered);
	WAIT_FOR_FLAG(flag_endpoint_found);
	WAIT_FOR_FLAG(flag_codec_found);
}

static void discover_source(struct bt_conn *conn)
{
	const uint8_t conn_index = bt_conn_index(conn);
	int err;

	UNSET_FLAG(flag_source_discovered);
	UNSET_FLAG(flag_codec_found);
	UNSET_FLAG(flag_endpoint_found);

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	memset(unicast_source_eps[conn_index], 0, sizeof(unicast_source_eps[conn_index]));

	WAIT_FOR_FLAG(flag_source_discovered);
	WAIT_FOR_FLAG(flag_endpoint_found);
	WAIT_FOR_FLAG(flag_codec_found);
}

static void discover_cas_inval(struct bt_conn *conn)
{
	int err;

	/* Test if it handles concurrent request for same connection */
	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	err = bt_cap_initiator_unicast_discover(conn);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_discover while previous discovery has not completed "
		     "did not fail\n");
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void discover_cas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void unicast_group_create(struct bt_bap_unicast_group **out_unicast_group)
{
	struct bt_bap_unicast_group_stream_param group_source_stream_params;
	struct bt_bap_unicast_group_stream_param group_sink_stream_params;
	struct bt_bap_unicast_group_stream_pair_param pair_params;
	struct bt_bap_unicast_group_param group_param;
	int err;

	group_sink_stream_params.qos = &unicast_preset_16_2_1.qos;
	group_sink_stream_params.stream = &unicast_client_sink_streams[0].bap_stream;
	group_source_stream_params.qos = &unicast_preset_16_2_1.qos;
	group_source_stream_params.stream = &unicast_client_source_streams[0].bap_stream;
	pair_params.tx_param = &group_sink_stream_params;
	pair_params.rx_param = &group_source_stream_params;

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = 1;
	group_param.params = &pair_params;

	err = bt_bap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void unicast_audio_start(struct bt_bap_unicast_group *unicast_group, bool wait)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param[2];
	struct bt_cap_unicast_audio_start_param param;
	int err;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = ARRAY_SIZE(stream_param);
	param.stream_params = stream_param;
	stream_param[0].member.member = default_conn;
	stream_param[0].stream = &unicast_client_sink_streams[0];
	stream_param[0].ep = unicast_sink_eps[bt_conn_index(default_conn)][0];
	stream_param[0].codec_cfg = &unicast_preset_16_2_1.codec_cfg;

	stream_param[1].member.member = default_conn;
	stream_param[1].stream = &unicast_client_source_streams[0];
	stream_param[1].ep = unicast_source_eps[bt_conn_index(default_conn)][0];
	stream_param[1].codec_cfg = &unicast_preset_16_2_1.codec_cfg;

	UNSET_FLAG(flag_started);

	err = bt_cap_initiator_unicast_audio_start(&param);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	if (wait) {
		WAIT_FOR_FLAG(flag_started);
	}
}

static void unicast_audio_update_inval(void)
{
	struct bt_audio_codec_cfg invalid_codec = BT_AUDIO_CODEC_LC3_CONFIG(
		BT_AUDIO_CODEC_CFG_FREQ_16KHZ, BT_AUDIO_CODEC_CFG_DURATION_10,
		BT_AUDIO_LOCATION_FRONT_LEFT, 40U, 1, BT_AUDIO_CONTEXT_TYPE_MEDIA);
	struct bt_cap_unicast_audio_update_stream_param stream_params[1] = {0};
	struct bt_cap_unicast_audio_update_param param = {0};
	int err;

	stream_params[0].stream = &unicast_client_sink_streams[0];
	stream_params[0].meta = unicast_preset_16_2_1.codec_cfg.meta;
	stream_params[0].meta_len = unicast_preset_16_2_1.codec_cfg.meta_len;
	param.count = ARRAY_SIZE(stream_params);
	param.stream_params = stream_params;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	err = bt_cap_initiator_unicast_audio_update(NULL);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with NULL params did not fail\n");
		return;
	}

	param.count = 0U;
	err = bt_cap_initiator_unicast_audio_update(&param);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with 0 param count did not fail\n");
		return;
	}

	/* Clear metadata so that it does not contain the mandatory stream context */
	param.count = ARRAY_SIZE(stream_params);
	memset(&invalid_codec.meta, 0, sizeof(invalid_codec.meta));
	stream_params[0].meta = invalid_codec.meta;

	err = bt_cap_initiator_unicast_audio_update(&param);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with invalid Codec metadata did not "
		     "fail\n");
		return;
	}
}

static void unicast_audio_update(void)
{
	struct bt_cap_unicast_audio_update_stream_param stream_params[2] = {0};
	struct bt_cap_unicast_audio_update_param param = {0};
	uint8_t new_meta[] = {
		3,
		BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT,
		BT_BYTES_LIST_LE16(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED),
		LONG_META_LEN,
		BT_AUDIO_METADATA_TYPE_VENDOR,
		LONG_META,
	};
	int err;

	stream_params[0].stream = &unicast_client_sink_streams[0];
	stream_params[0].meta = new_meta;
	stream_params[0].meta_len = ARRAY_SIZE(new_meta);

	stream_params[1].stream = &unicast_client_source_streams[0];
	stream_params[1].meta = new_meta;
	stream_params[1].meta_len = ARRAY_SIZE(new_meta);

	param.count = ARRAY_SIZE(stream_params);
	param.stream_params = stream_params;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	UNSET_FLAG(flag_updated);

	err = bt_cap_initiator_unicast_audio_update(&param);
	if (err != 0) {
		FAIL("Failed to update unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_updated);
	printk("READ LONG META\n");
}

static void unicast_audio_stop(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_stop_param param;
	int err;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = non_idle_streams_cnt;
	param.streams = non_idle_streams;
	param.release = false;

	/* Stop without release first to verify that we enter the QoS Configured state */
	UNSET_FLAG(flag_stopped);
	printk("Stopping without relasing\n");

	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err != 0) {
		FAIL("Failed to stop unicast audio without release: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_stopped);

	/* Verify that it cannot be stopped twice */
	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_stop without release with already-stopped "
		     "streams did not fail\n");
		return;
	}

	/* Stop with release first to verify that we enter the idle state */
	UNSET_FLAG(flag_stopped);
	param.release = true;
	printk("Relasing\n");

	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err != 0) {
		FAIL("Failed to stop unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_stopped);

	/* Verify that it cannot be stopped twice */
	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_stop with already-stopped streams did not "
		     "fail\n");
		return;
	}
}

static void unicast_audio_cancel(void)
{
	int err;

	err = bt_cap_initiator_unicast_audio_cancel();
	if (err != 0) {
		FAIL("Failed to cancel unicast audio: %d\n", err);
		return;
	}
}

static void unicast_group_delete_inval(void)
{
	int err;

	err = bt_bap_unicast_group_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_unicast_group_delete with NULL group did not fail\n");
		return;
	}
}

static void unicast_group_delete(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}

	/* Verify that it cannot be deleted twice */
	err = bt_bap_unicast_group_delete(unicast_group);
	if (err == 0) {
		FAIL("bt_bap_unicast_group_delete with already-deleted unicast group did not "
		     "fail\n");
		return;
	}
}

static void test_main_cap_initiator_unicast(void)
{
	struct bt_bap_unicast_group *unicast_group;
	const size_t iterations = 2;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas(default_conn);
	discover_cas(default_conn); /* test that we can discover twice */

	discover_sink(default_conn);
	discover_source(default_conn);

	for (size_t i = 0U; i < iterations; i++) {
		printk("\nRunning iteration i=%zu\n\n", i);
		unicast_group_create(&unicast_group);

		for (size_t j = 0U; j < iterations; j++) {
			printk("\nRunning iteration j=%zu\n\n", i);

			unicast_audio_start(unicast_group, true);

			unicast_audio_update();

			unicast_audio_stop(unicast_group);
		}

		unicast_group_delete(unicast_group);
		unicast_group = NULL;
	}

	PASS("CAP initiator unicast passed\n");
}

static void test_main_cap_initiator_unicast_inval(void)
{
	struct bt_bap_unicast_group *unicast_group;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas_inval(default_conn);
	discover_cas(default_conn);

	discover_sink(default_conn);
	discover_source(default_conn);

	unicast_group_create(&unicast_group);

	unicast_audio_start(unicast_group, true);

	unicast_audio_update_inval();
	unicast_audio_update();

	unicast_audio_stop(unicast_group);

	unicast_group_delete_inval();
	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	PASS("CAP initiator unicast inval passed\n");
}

static void test_cap_initiator_unicast_timeout(void)
{
	struct bt_bap_unicast_group *unicast_group;
	const k_timeout_t timeout = K_SECONDS(10);
	const size_t iterations = 2;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas(default_conn);

	discover_sink(default_conn);
	discover_source(default_conn);

	unicast_group_create(&unicast_group);

	for (size_t j = 0U; j < iterations; j++) {
		printk("\nRunning iteration #%zu\n\n", j);
		unicast_audio_start(unicast_group, false);

		k_sleep(timeout);
		if ((bool)atomic_get(&flag_started)) {
			FAIL("Unexpected start complete\n");
		} else {
			unicast_audio_cancel();
		}

		WAIT_FOR_FLAG(flag_start_timeout);

		unicast_audio_stop(unicast_group);
	}

	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	PASS("CAP initiator unicast timeout passed\n");
}

static void set_invalid_metadata_type(uint8_t type)
{
	const uint8_t val = 0xFF;
	int err;

	err = bt_audio_codec_cfg_meta_set_val(&unicast_preset_16_2_1.codec_cfg, type, &val,
					      sizeof(val));
	if (err < 0) {
		FAIL("Failed to set invalid metadata type: %d\n", err);
		return;
	}
}

static void unset_invalid_metadata_type(uint8_t type)
{
	int err;

	err = bt_audio_codec_cfg_meta_unset_val(&unicast_preset_16_2_1.codec_cfg, type);
	if (err < 0) {
		FAIL("Failed to unset invalid metadata type: %d\n", err);
		return;
	}
}

static void test_cap_initiator_unicast_ase_error(void)
{
	struct bt_bap_unicast_group *unicast_group;
	const uint8_t inval_type = 0xFD;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas(default_conn);
	discover_sink(default_conn);
	discover_source(default_conn);

	unicast_group_create(&unicast_group);

	set_invalid_metadata_type(inval_type);

	/* With invalid metadata type, start should fail */
	unicast_audio_start(unicast_group, false);
	WAIT_FOR_FLAG(flag_start_failed);

	/* Remove invalid type and retry */
	unset_invalid_metadata_type(inval_type);

	/* Without invalid metadata type, start should pass */
	unicast_audio_start(unicast_group, true);

	unicast_audio_stop(unicast_group);

	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	PASS("CAP initiator unicast ASE error passed\n");
}

static const struct named_lc3_preset *cap_get_named_preset(const char *preset_arg)
{
	for (size_t i = 0U; i < ARRAY_SIZE(lc3_unicast_presets); i++) {
		if (strcmp(preset_arg, lc3_unicast_presets[i].name) == 0) {
			return &lc3_unicast_presets[i];
		}
	}

	return NULL;
}

static int cap_initiator_ac_create_unicast_group(const struct cap_initiator_ac_param *param,
						 struct unicast_stream *snk_uni_streams[],
						 size_t snk_cnt,
						 struct unicast_stream *src_uni_streams[],
						 size_t src_cnt,
						 struct bt_bap_unicast_group **unicast_group)
{
	struct bt_bap_unicast_group_stream_param snk_group_stream_params[CAP_AC_MAX_SNK] = {0};
	struct bt_bap_unicast_group_stream_param src_group_stream_params[CAP_AC_MAX_SRC] = {0};
	struct bt_bap_unicast_group_stream_pair_param pair_params[CAP_AC_MAX_PAIR] = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	struct bt_bap_qos_cfg *snk_qos[CAP_AC_MAX_SNK];
	struct bt_bap_qos_cfg *src_qos[CAP_AC_MAX_SRC];
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t pair_cnt = 0U;

	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_qos[i] = &snk_uni_streams[i]->qos;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_qos[i] = &src_uni_streams[i]->qos;
	}

	/* Create Group
	 *
	 * First setup the individual stream parameters and then match them in pairs by connection
	 * and direction
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_group_stream_params[i].qos = snk_qos[i];
		snk_group_stream_params[i].stream = &snk_uni_streams[i]->stream.bap_stream;
	}
	for (size_t i = 0U; i < src_cnt; i++) {
		src_group_stream_params[i].qos = src_qos[i];
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

static int cap_initiator_ac_cap_unicast_start(const struct cap_initiator_ac_param *param,
					      struct unicast_stream *snk_uni_streams[],
					      size_t snk_cnt,
					      struct unicast_stream *src_uni_streams[],
					      size_t src_cnt,
					      struct bt_bap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_start_stream_param stream_params[CAP_AC_MAX_STREAM] = {0};
	struct bt_audio_codec_cfg *snk_codec_cfgs[CAP_AC_MAX_SNK] = {0};
	struct bt_audio_codec_cfg *src_codec_cfgs[CAP_AC_MAX_SRC] = {0};
	struct bt_cap_stream *snk_cap_streams[CAP_AC_MAX_SNK] = {0};
	struct bt_cap_stream *src_cap_streams[CAP_AC_MAX_SRC] = {0};
	struct bt_cap_unicast_audio_start_param start_param = {0};
	struct bt_bap_ep *snk_eps[CAP_AC_MAX_SNK] = {0};
	struct bt_bap_ep *src_eps[CAP_AC_MAX_SRC] = {0};
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t stream_cnt = 0U;
	size_t snk_ep_cnt = 0U;
	size_t src_ep_cnt = 0U;

	for (size_t i = 0U; i < param->conn_cnt; i++) {
		const uint8_t conn_index = bt_conn_index(connected_conns[i]);
#if UNICAST_SINK_SUPPORTED
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			snk_eps[snk_ep_cnt] = unicast_sink_eps[conn_index][j];
			if (snk_eps[snk_ep_cnt] == NULL) {
				FAIL("No sink[%u][%zu] endpoint available\n", conn_index, j);

				return -ENODEV;
			}
			snk_ep_cnt++;
		}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SRC_SUPPORTED
		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			src_eps[src_ep_cnt] = unicast_source_eps[conn_index][j];
			if (src_eps[src_ep_cnt] == NULL) {
				FAIL("No source[%u][%zu] endpoint available\n", conn_index, j);

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
		FAIL("Source  endpoint and stream count mismatch: %zu != %zu\n", src_ep_cnt,
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

	return bt_cap_initiator_unicast_audio_start(&start_param);
}

static int cap_initiator_ac_unicast(const struct cap_initiator_ac_param *param,
				    struct bt_bap_unicast_group **unicast_group)
{
	/* Allocate params large enough for any params, but only use what is required */
	struct unicast_stream *snk_uni_streams[CAP_AC_MAX_SNK];
	struct unicast_stream *src_uni_streams[CAP_AC_MAX_SRC];
	size_t snk_cnt = 0;
	size_t src_cnt = 0;
	int err;

	if (param->conn_cnt > CAP_AC_MAX_CONN) {
		FAIL("Invalid conn_cnt: %zu\n", param->conn_cnt);

		return -EINVAL;
	}

	for (size_t i = 0; i < param->conn_cnt; i++) {
		/* Verify conn values */
		if (param->snk_cnt[i] > CAP_AC_MAX_SNK) {
			FAIL("Invalid param->snk_cnt[%zu]: %zu\n", i, param->snk_cnt[i]);

			return -EINVAL;
		}

		if (param->src_cnt[i] > CAP_AC_MAX_SRC) {
			FAIL("Invalid param->src_cnt[%zu]: %zu\n", i, param->src_cnt[i]);

			return -EINVAL;
		}
	}

	/* Set all endpoints from multiple connections in a single array, and verify that the known
	 * endpoints matches the audio configuration
	 */
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

		if (param->snk_named_preset == NULL) {
			FAIL("No sink preset available\n");
			return -EINVAL;
		}

		copy_unicast_stream_preset(snk_uni_streams[i], param->snk_named_preset);

		/* Some audio configuration requires multiple sink channels,
		 * so multiply the SDU based on the channel count
		 */
		snk_uni_streams[i]->qos.sdu *= param->snk_chan_cnt;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_uni_streams[i] = &unicast_streams[i + snk_cnt];

		if (param->src_named_preset == NULL) {
			FAIL("No sink preset available\n");
			return -EINVAL;
		}

		copy_unicast_stream_preset(src_uni_streams[i], param->src_named_preset);

		/* Some audio configuration requires multiple source channels,
		 * so multiply the SDU based on the channel count
		 */
		src_uni_streams[i]->qos.sdu *= param->src_chan_cnt;
	}

	err = cap_initiator_ac_create_unicast_group(param, snk_uni_streams, snk_cnt,
						    src_uni_streams, src_cnt, unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);

		return err;
	}

	UNSET_FLAG(flag_started);

	printk("Starting %zu streams for %s\n", snk_cnt + src_cnt, param->name);
	err = cap_initiator_ac_cap_unicast_start(param, snk_uni_streams, snk_cnt, src_uni_streams,
						 src_cnt, *unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n\n", err);

		return err;
	}

	WAIT_FOR_FLAG(flag_started);

	return 0;
}

static void test_cap_initiator_ac(const struct cap_initiator_ac_param *param)
{
	struct bt_bap_unicast_group *unicast_group;

	printk("Running test for %s with Sink Preset %s and Source Preset %s\n", param->name,
	       param->snk_named_preset != NULL ? param->snk_named_preset->name : "None",
	       param->src_named_preset != NULL ? param->src_named_preset->name : "None");

	if (param->conn_cnt > CAP_AC_MAX_CONN) {
		FAIL("Invalid conn_cnt: %zu\n", param->conn_cnt);
		return;
	}

	if (param->snk_named_preset == NULL && param->src_named_preset == NULL) {
		FAIL("No presets available\n");
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
	}

	cap_initiator_ac_unicast(param, &unicast_group);

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

	PASS("CAP initiator passed for %s with Sink Preset %s and Source Preset %s\n", param->name,
	     param->snk_named_preset != NULL ? param->snk_named_preset->name : "None",
	     param->src_named_preset != NULL ? param->src_named_preset->name : "None");
}

static void test_cap_initiator_ac_1(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_1",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_2(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_2",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
		.snk_named_preset = NULL,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_3(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_3",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_4(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_4",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 2U,
		.src_chan_cnt = 0U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_5(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_5",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 2U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_6_i(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_6_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_6_ii(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_6_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {0U, 0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = NULL,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_7_i(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_7_i",
		.conn_cnt = 1U,
		/*  TODO: These should be in different CIS but will be in the same currently */
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_7_ii(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_7_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 0U},
		.src_cnt = {0U, 1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_8_i(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_8_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_8_ii(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_8_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_9_i(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_9_i",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {2U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
		.snk_named_preset = NULL,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_9_ii(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_9_ii",
		.conn_cnt = 2U,
		.snk_cnt = {0U, 0U},
		.src_cnt = {1U, 1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
		.snk_named_preset = NULL,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}
static void test_cap_initiator_ac_10(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_10",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 2U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_11_i(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_11_i",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {2U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_cap_initiator_ac_11_ii(void)
{
	const struct cap_initiator_ac_param param = {
		.name = "ac_11_ii",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
		.snk_named_preset = snk_named_preset,
		.src_named_preset = src_named_preset,
	};

	test_cap_initiator_ac(&param);
}

static void test_args(int argc, char *argv[])
{
	for (size_t argn = 0; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "sink_preset") == 0) {
			const char *preset_arg = argv[++argn];

			snk_named_preset = cap_get_named_preset(preset_arg);
			if (snk_named_preset == NULL) {
				FAIL("Failed to get sink preset from %s\n", preset_arg);
			}
		} else if (strcmp(arg, "source_preset") == 0) {
			const char *preset_arg = argv[++argn];

			src_named_preset = cap_get_named_preset(preset_arg);
			if (src_named_preset == NULL) {
				FAIL("Failed to get source preset from %s\n", preset_arg);
			}
		} else {
			FAIL("Invalid arg: %s\n", arg);
		}
	}
}

static const struct bst_test_instance test_cap_initiator_unicast[] = {
	{
		.test_id = "cap_initiator_unicast",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_initiator_unicast,
	},
	{
		.test_id = "cap_initiator_unicast_timeout",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_unicast_timeout,
	},
	{
		.test_id = "cap_initiator_unicast_ase_error",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_unicast_ase_error,
	},
	{
		.test_id = "cap_initiator_unicast_inval",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_initiator_unicast_inval,
	},
	{
		.test_id = "cap_initiator_ac_1",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_1,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_2",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_2,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_3",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_3,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_4",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_4,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_5",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_5,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_6_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_6_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_6_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_6_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_7_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_7_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_7_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_7_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_8_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_8_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_8_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_8_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_9_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_9_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_9_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_9_ii,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_10",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_10,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_11_i",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_11_i,
		.test_args_f = test_args,
	},
	{
		.test_id = "cap_initiator_ac_11_ii",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_ac_11_ii,
		.test_args_f = test_args,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_initiator_unicast_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_initiator_unicast);
}

#else /* !CONFIG_BT_CAP_INITIATOR_UNICAST */

struct bst_test_list *test_cap_initiator_unicast_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_INITIATOR_UNICAST */
