/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
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
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "bap_common.h"
#include "bap_stream_tx.h"
#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(cap_handover_central, LOG_LEVEL_DBG);

#if defined(CONFIG_BT_CAP_HANDOVER)

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

extern enum bst_result_t bst_result;

static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset broadcast_preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct cap_acceptor {
	struct audio_test_stream sink_stream;
	struct audio_test_stream source_stream;
	struct bt_bap_ep *unicast_sink_ep;
	struct bt_bap_ep *unicast_source_ep;
	struct bt_conn *conn;
} cap_acceptors[CONFIG_BT_MAX_CONN];

static size_t connected_conn_cnt;
static struct bt_cap_broadcast_source *broadcast_source;
static bt_addr_le_t remote_dev_addr;

CREATE_FLAG(flag_dev_found);
CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_codec_found);
CREATE_FLAG(flag_endpoint_found);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_handover_unicast_to_broadcast);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_source_discovered);
CREATE_FLAG(flag_broadcast_started);
CREATE_FLAG(flag_broadcast_stopped);

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

		LOG_DBG("Found CAS with CSIS %p", csis_inst);
	} else {
		LOG_DBG("Found CAS");
	}

	SET_FLAG(flag_discovered);
}

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err == 0) {
		LOG_DBG("BASS discover done with %u recv states", recv_state_count);
	} else {
		LOG_DBG("BASS discover failed (%d)", err);
	}

	SET_FLAG(flag_discovered);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		LOG_DBG("Failed to start (failing conn %p): %d", conn, err);
	} else {
		SET_FLAG(flag_started);
	}
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to stop (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_stopped);
}

static void unicast_to_broadcast_complete_cb(int err, struct bt_conn *conn,
					     struct bt_cap_unicast_group *unicast_group,
					     struct bt_cap_broadcast_source *source)
{
	if (err != 0) {
		FAIL("Failed to handover unicast to broadcast (failing conn %p): %d", conn, err);

		return;
	}

	broadcast_source = source;
	SET_FLAG(flag_handover_unicast_to_broadcast);
}

static void add_remote_sink(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	if (cap_acceptors[conn_index].unicast_sink_ep == NULL) {
		LOG_DBG("Acceptor[%u] %p: Sink ep %p", conn_index, conn, ep);
		cap_acceptors[conn_index].unicast_sink_ep = ep;
	} else {
		LOG_DBG("Could not add sink ep %p", ep);
	}
}

static void add_remote_source(const struct bt_conn *conn, struct bt_bap_ep *ep)
{
	const uint8_t conn_index = bt_conn_index(conn);

	if (cap_acceptors[conn_index].unicast_source_ep == NULL) {
		LOG_DBG("Acceptor[%u] %p: Source ep %p", conn_index, conn, ep);
		cap_acceptors[conn_index].unicast_source_ep = ep;
	} else {
		LOG_DBG("Could not add Source ep %p", ep);
	}
}

static void print_remote_codec(const struct bt_audio_codec_cap *codec_cap, enum bt_audio_dir dir)
{
	LOG_DBG("codec_cap %p dir 0x%02x", codec_cap, dir);

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
		LOG_DBG("Sink discover complete");

		SET_FLAG(flag_sink_discovered);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Source discover complete");

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

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_DBG("MTU exchanged");
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

	LOG_DBG("data->type %u", data->type);

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
	LOG_DBG("Device found: %s", addr_str);

	bt_addr_le_copy(&remote_dev_addr, addr);
	SET_FLAG(flag_dev_found);

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
		bt_addr_le_t addr;

		bt_addr_le_copy(&addr, info->addr);

		/* Check for CAS support in advertising data */
		bt_data_parse(buf, check_audio_support_and_connect_cb, (void *)&addr);
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	memset(&test_stream->last_info, 0, sizeof(test_stream->last_info));
	test_stream->rx_cnt = 0U;
	test_stream->valid_rx_cnt = 0U;
	test_stream->seq_num = 0U;
	test_stream->tx_cnt = 0U;

	LOG_DBG("Started stream %p", stream);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_register(stream);
		if (err != 0) {
			FAIL("Failed to register stream %p for TX: %d\n", stream, err);
			return;
		}
	}
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_DBG("Stopped stream %p with reason 0x%02X", stream, reason);

	if (bap_stream_tx_can_send(stream)) {
		int err;

		err = bap_stream_tx_unregister(stream);
		if (err != 0) {
			FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
			return;
		}
	}
}

static void broadcast_source_started_cb(struct bt_bap_broadcast_source *source)
{
	SET_FLAG(flag_broadcast_started);
}

static void broadcast_source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	SET_FLAG(flag_broadcast_stopped);
}

static void init(void)
{
	static struct bt_le_scan_cb scan_callbacks = {
		.recv = scan_recv_cb,
	};
	static struct bt_bap_broadcast_assistant_cb ba_cbs = {
		.discover = bap_broadcast_assistant_discover_cb,
	};
	static struct bt_cap_handover_cb cap_handover_cb = {
		.unicast_to_broadcast_complete = unicast_to_broadcast_complete_cb,
	};
	static struct bt_cap_initiator_cb cap_initiator_cb = {
		.unicast_discovery_complete = cap_discovery_complete_cb,
		.unicast_start_complete = unicast_start_complete_cb,
		.unicast_stop_complete = unicast_stop_complete_cb,
	};
	static struct bt_bap_unicast_client_cb unicast_client_cbs = {
		.discover = discover_cb,
		.pac_record = pac_record_cb,
		.endpoint = endpoint_cb,
	};
	static struct bt_bap_stream_ops stream_ops = {
		.started = stream_started_cb,
		.stopped = stream_stopped_cb,
		.sent = bap_stream_tx_sent_cb,
	};
	static struct bt_bap_broadcast_source_cb broadcast_source_cbs = {
		.started = broadcast_source_started_cb,
		.stopped = broadcast_source_stopped_cb,
	};

	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	LOG_DBG("Bluetooth initialized");
	bap_stream_tx_init();

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

	err = bt_cap_initiator_register_cb(&cap_initiator_cb);
	if (err != 0) {
		FAIL("Failed to register CAP initiator callbacks (err %d)\n", err);
		return;
	}

	err = bt_cap_handover_register_cb(&cap_handover_cb);
	if (err != 0) {
		FAIL("Failed to register CAP handover callbacks (err %d)\n", err);
		return;
	}

	err = bt_bap_broadcast_assistant_register_cb(&ba_cbs);
	if (err != 0) {
		FAIL("Failed to register broadcast assistant callbacks (err %d)\n");
		return;
	}

	err = bt_bap_broadcast_source_register_cb(&broadcast_source_cbs);
	if (err != 0) {
		FAIL("Failed to register broadcast assistant callbacks (err %d)\n");
		return;
	}

	ARRAY_FOR_EACH_PTR(cap_acceptors, acceptor) {
		bt_cap_stream_ops_register(
			cap_stream_from_audio_test_stream(&acceptor->sink_stream), &stream_ops);
		bt_cap_stream_ops_register(
			cap_stream_from_audio_test_stream(&acceptor->source_stream), &stream_ops);
	}
}

static void scan_and_connect(struct bt_conn **conn)
{
	int err;

	UNSET_FLAG(flag_dev_found);
	UNSET_FLAG(flag_connected);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_dev_found);
	LOG_DBG("Stopping scan");
	if (bt_le_scan_stop() != 0) {
		FAIL("Could not stop scan");
		return;
	}

	err = bt_conn_le_create(&remote_dev_addr, BT_CONN_LE_CREATE_CONN, BT_BAP_CONN_PARAM_RELAXED,
				conn);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
		return;
	}

	LOG_DBG("Scanning successfully started");
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
	cap_acceptors[conn_index].unicast_sink_ep = NULL;

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		LOG_DBG("Failed to discover sink: %d", err);
		return;
	}

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
	cap_acceptors[conn_index].unicast_source_ep = NULL;

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		LOG_DBG("Failed to discover sink: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_source_discovered);
	WAIT_FOR_FLAG(flag_endpoint_found);
	WAIT_FOR_FLAG(flag_codec_found);
}

static void discover_cas(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		LOG_DBG("Failed to discover CAS: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void discover_bass(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_bap_broadcast_assistant_discover(conn);
	if (err != 0) {
		FAIL("Failed to discover BASS on the sink (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void unicast_group_create(struct bt_cap_unicast_group **out_unicast_group)
{
	struct bt_cap_unicast_group_stream_param group_source_stream_params[CONFIG_BT_MAX_CONN];
	struct bt_cap_unicast_group_stream_param group_sink_stream_params[CONFIG_BT_MAX_CONN];
	struct bt_cap_unicast_group_stream_pair_param pair_params[CONFIG_BT_MAX_CONN];
	struct bt_cap_unicast_group_param group_param;
	int err;

	for (size_t i = 0U; i < connected_conn_cnt; i++) {
		group_sink_stream_params[i].qos_cfg = &unicast_preset_16_2_1.qos;
		group_sink_stream_params[i].stream =
			cap_stream_from_audio_test_stream(&cap_acceptors[i].sink_stream);
		group_source_stream_params[i].qos_cfg = &unicast_preset_16_2_1.qos;
		group_source_stream_params[i].stream =
			cap_stream_from_audio_test_stream(&cap_acceptors[i].source_stream);
		pair_params[i].tx_param = &group_sink_stream_params[i];
		pair_params[i].rx_param = &group_source_stream_params[i];
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = connected_conn_cnt;
	group_param.params = pair_params;

	err = bt_cap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void unicast_audio_start(struct bt_cap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param[2 * ARRAY_SIZE(cap_acceptors)];
	struct bt_cap_unicast_audio_start_param param;
	size_t stream_param_cnt = 0U;
	int err;

	for (size_t i = 0U; i < connected_conn_cnt; i++) {
		/* Sink param */
		stream_param[stream_param_cnt].member.member = cap_acceptors[i].conn;
		stream_param[stream_param_cnt].stream =
			cap_stream_from_audio_test_stream(&cap_acceptors[i].sink_stream);
		stream_param[stream_param_cnt].ep = cap_acceptors[i].unicast_sink_ep;
		stream_param[stream_param_cnt].codec_cfg = &unicast_preset_16_2_1.codec_cfg;
		stream_param_cnt++;

		/* source param */
		stream_param[stream_param_cnt].member.member = cap_acceptors[i].conn;
		stream_param[stream_param_cnt].stream =
			cap_stream_from_audio_test_stream(&cap_acceptors[i].source_stream);
		stream_param[stream_param_cnt].ep = cap_acceptors[i].unicast_source_ep;
		stream_param[stream_param_cnt].codec_cfg = &unicast_preset_16_2_1.codec_cfg;
		stream_param_cnt++;
	}

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = stream_param_cnt;
	param.stream_params = stream_param;

	UNSET_FLAG(flag_started);

	err = bt_cap_initiator_unicast_audio_start(&param);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_started);
	/* let other devices know we have started what we wanted */
	backchannel_sync_send_all();
}

static void handover_unicast_to_broadcast(struct bt_cap_unicast_group *unicast_group,
					  struct bt_le_ext_adv *ext_adv)
{
	static struct bt_cap_initiator_broadcast_stream_param
		stream_params[ARRAY_SIZE(cap_acceptors)];
	static struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	/* Struct containing the converted unicast group configuration */
	static struct bt_cap_initiator_broadcast_create_param create_param = {0};
	struct bt_cap_handover_unicast_to_broadcast_param param = {0};
	size_t stream_cnt = 0U;
	int err;

	ARRAY_FOR_EACH(stream_params, i) {
		const struct bt_bap_ep *ep =
			bap_stream_from_audio_test_stream(&cap_acceptors[i].sink_stream)->ep;
		struct bt_bap_ep_info ep_info;

		if (ep == NULL) {
			/* Not configured */
			continue;
		}

		err = bt_bap_ep_get_info(ep, &ep_info);

		if (err != 0) {
			FAIL("Failed to get endpoint info: %d", err);
			return;
		}

		if (ep_info.state != BT_BAP_EP_STATE_STREAMING) {
			/* Not streaming - Handover is only applied to streaming streams */
			continue;
		}

		stream_params[stream_cnt].stream =
			cap_stream_from_audio_test_stream(&cap_acceptors[i].sink_stream);
		stream_params[stream_cnt].data_len = 0U;
		stream_params[stream_cnt].data = NULL;

		stream_cnt++;
	}

	if (stream_cnt == 0U) {
		FAIL("No streams can be handed over");
		return;
	}

	subgroup_param.stream_count = stream_cnt;
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec_cfg = &broadcast_preset_16_2_1.codec_cfg;

	create_param.subgroup_count = 1U;
	create_param.subgroup_params = &subgroup_param;
	create_param.qos = &broadcast_preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = false;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.unicast_group = unicast_group;
	param.broadcast_create_param = &create_param;
	param.ext_adv = ext_adv;
	param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	param.broadcast_id = 0x123456;

	UNSET_FLAG(flag_handover_unicast_to_broadcast);

	err = bt_cap_handover_unicast_to_broadcast(&param);
	if (err != 0) {
		FAIL("Failed to handover unicast audio to broadcast: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_handover_unicast_to_broadcast);
	LOG_DBG("Handover procedure completed");
}

static void set_base_data(struct bt_le_ext_adv *ext_adv)
{
	struct bt_data per_ad;
	int err;

	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

	err = bt_cap_initiator_broadcast_get_base(broadcast_source, &base_buf);
	if (err != 0) {
		FAIL("Failed to get encoded BASE: %d\n", err);
		return;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;
	err = bt_le_per_adv_set_data(ext_adv, &per_ad, 1);
	if (err != 0) {
		FAIL("Failed to set periodic advertising data: %d\n", err);
		return;
	}
}

static void stop_broadcast(void)
{
	int err;

	/* Verify that it cannot be stopped twice */
	err = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
	if (err != 0) {
		FAIL("Failed to stop broadcast source: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_broadcast_stopped);

	err = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
	if (err != 0) {
		FAIL("Failed to delete broadcast source: %d\n", err);
		return;
	}

	broadcast_source = NULL;
}

static void test_main_cap_handover_unicast_to_broadcast(void)
{
	const size_t acceptor_cnt = get_dev_cnt() - 1; /* Assume all other devices are acceptors */
	struct bt_cap_unicast_group *unicast_group;
	struct bt_le_ext_adv *ext_adv;

	if (acceptor_cnt > CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT) {
		FAIL("Cannot run test with %zu acceptors and maximum %d broadcast streams",
		     CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
	}

	init();

	/* Connect to and do discovery on all CAP acceptors */
	for (size_t i = 0U; i < acceptor_cnt; i++) {
		scan_and_connect(&cap_acceptors[i].conn);

		WAIT_FOR_FLAG(flag_mtu_exchanged);

		discover_cas(cap_acceptors[i].conn);
		discover_bass(cap_acceptors[i].conn);

		discover_sink(cap_acceptors[i].conn);
		discover_source(cap_acceptors[i].conn);
	}

	unicast_group_create(&unicast_group);

	unicast_audio_start(unicast_group);

	/* Wait for acceptors to receive some data */
	backchannel_sync_wait_all();

	setup_broadcast_adv(&ext_adv);

	handover_unicast_to_broadcast(unicast_group, ext_adv);
	set_base_data(ext_adv);
	start_broadcast_adv(ext_adv);

	/* Wait for acceptors to receive some data */
	backchannel_sync_wait_all();

	stop_broadcast();

	PASS("CAP initiator handover unicast to broadcast passed\n");
}

static const struct bst_test_instance test_cap_handover_central[] = {
	{
		.test_id = "cap_handover_central",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_handover_unicast_to_broadcast,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_handover_central_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_handover_central);
}

#else /* !CONFIG_BT_CAP_HANDOVER */

struct bst_test_list *test_cap_handover_central_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_HANDOVER */
