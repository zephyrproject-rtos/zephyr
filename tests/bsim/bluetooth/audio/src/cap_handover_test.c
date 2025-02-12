/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>

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

#include "bap_common.h"
#include "bap_stream_rx.h"
#include "bstests.h"
#include "common.h"

#if defined(CONFIG_BT_CAP_HANDOVER_SUPPORTED)

#define CONTEXT  (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#define LOCATION (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)

extern enum bst_result_t bst_result;

static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_bap_lc3_preset broadcast_preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct audio_test_stream sink_streams[MIN(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
						 CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)];
static struct audio_test_stream source_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_bap_ep
	*unicast_sink_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep
	*unicast_source_eps[CONFIG_BT_MAX_CONN][CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN];
static size_t connected_conn_cnt;

CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_codec_found);
CREATE_FLAG(flag_endpoint_found);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_handover_unicast_to_broadcast);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);
CREATE_FLAG(flag_source_discovered);

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

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err == 0) {
		printk("BASS discover done with %u recv states\n", recv_state_count);
	} else {
		printk("BASS discover failed (%d)\n", err);
	}

	SET_FLAG(flag_discovered);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		printk("Failed to start (failing conn %p): %d\n", conn, err);
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

static void unicast_to_broadcast_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to handover unicast to broadcast (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_handover_unicast_to_broadcast);
}

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

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_BAP_CONN_PARAM_RELAXED,
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
	static struct bt_bap_broadcast_assistant_cb ba_cbs = {
		.discover = bap_broadcast_assistant_discover_cb,
	};
	static struct bt_cap_initiator_cb cap_cb = {
		.unicast_discovery_complete = cap_discovery_complete_cb,
		.unicast_start_complete = unicast_start_complete_cb,
		.unicast_stop_complete = unicast_stop_complete_cb,
		.unicast_to_broadcast_complete = unicast_to_broadcast_complete_cb,
	};
	static struct bt_bap_unicast_client_cb unicast_client_cbs = {
		.discover = discover_cb,
		.pac_record = pac_record_cb,
		.endpoint = endpoint_cb,
	};

	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

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

	err = bt_bap_broadcast_assistant_register_cb(&ba_cbs);
	if (err != 0) {
		FAIL("Failed to register broadcast assistant callbacks (err %d)\n");
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
	struct bt_cap_unicast_group_stream_param group_source_stream_params;
	struct bt_cap_unicast_group_stream_param group_sink_stream_params;
	struct bt_cap_unicast_group_stream_pair_param pair_params;
	struct bt_cap_unicast_group_param group_param;
	int err;

	group_sink_stream_params.qos_cfg = &unicast_preset_16_2_1.qos;
	group_sink_stream_params.stream = cap_stream_from_audio_test_stream(&sink_streams[0]);
	group_source_stream_params.qos_cfg = &unicast_preset_16_2_1.qos;
	group_source_stream_params.stream = cap_stream_from_audio_test_stream(&source_streams[0]);
	pair_params.tx_param = &group_sink_stream_params;
	pair_params.rx_param = &group_source_stream_params;

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = 1;
	group_param.params = &pair_params;

	err = bt_cap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void unicast_audio_start(struct bt_cap_unicast_group *unicast_group)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param[2];
	struct bt_cap_unicast_audio_start_param param;
	int err;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = ARRAY_SIZE(stream_param);
	param.stream_params = stream_param;
	stream_param[0].member.member = default_conn;
	stream_param[0].stream = cap_stream_from_audio_test_stream(&sink_streams[0]);
	stream_param[0].ep = unicast_sink_eps[bt_conn_index(default_conn)][0];
	stream_param[0].codec_cfg = &unicast_preset_16_2_1.codec_cfg;

	stream_param[1].member.member = default_conn;
	stream_param[1].stream = cap_stream_from_audio_test_stream(&source_streams[0]);
	stream_param[1].ep = unicast_source_eps[bt_conn_index(default_conn)][0];
	stream_param[1].codec_cfg = &unicast_preset_16_2_1.codec_cfg;

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
					  struct bt_le_ext_adv *ext_adv,
					  struct bt_cap_broadcast_source **source)
{
	static struct bt_cap_initiator_broadcast_stream_param stream_params[MIN(
		ARRAY_SIZE(sink_streams), CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT)] = {};
	static struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {};
	/* Struct containing the converted unicast group configuration */
	static struct bt_cap_initiator_broadcast_create_param create_param = {};
	struct bt_cap_unicast_to_broadcast_param param = {};
	size_t stream_cnt = 0U;
	int err;

	ARRAY_FOR_EACH(stream_params, i) {
		struct bt_bap_ep_info ep_info;
		struct bt_bap_ep *ep = bap_stream_from_audio_test_stream(&sink_streams[i])->ep;

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
			cap_stream_from_audio_test_stream(&sink_streams[i]);
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
	param.sid = 0x00;
	param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	param.broadcast_id = 0x123456;

	UNSET_FLAG(flag_handover_unicast_to_broadcast);

	err = bt_cap_initiator_unicast_to_broadcast(&param, source);
	if (err != 0) {
		FAIL("Failed to handover unicast audio to broadcast: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_handover_unicast_to_broadcast);
	printk("Handover procedure completed\n");
}

static void test_main_cap_handover_unicast_to_broadcast(void)
{
	struct bt_cap_broadcast_source *broadcast_source;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_le_ext_adv *ext_adv;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas(default_conn);
	discover_bass(default_conn);

	discover_sink(default_conn);
	discover_source(default_conn);

	unicast_group_create(&unicast_group);

	UNSET_FLAG(flag_audio_received);

	unicast_audio_start(unicast_group);

	setup_broadcast_adv(&ext_adv);

	handover_unicast_to_broadcast(unicast_group, ext_adv, &broadcast_source);

	PASS("CAP initiator handover unicast to broadcast passed\n");
}

static const struct bst_test_instance test_cap_handover[] = {
	{
		.test_id = "cap_handover_unicast_to_broadcast",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_handover_unicast_to_broadcast,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_handover_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_handover);
}

#else /* !CONFIG_BT_CAP_HANDOVER_SUPPORTED */

struct bst_test_list *test_cap_handover_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_HANDOVER_SUPPORTED */
