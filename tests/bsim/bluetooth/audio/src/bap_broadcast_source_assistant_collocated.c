/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "bap_common.h"
#include "bap_stream_tx.h"
#include "bstests.h"
#include "common.h"
#include "bsim_args_runner.h"

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) && defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)

extern enum bst_result_t bst_result;

/* Broadcast Source variables */
static struct audio_test_stream broadcast_source_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static struct bt_bap_broadcast_source *broadcast_source;
static struct bt_le_ext_adv *broadcast_adv;
static uint32_t broadcast_id;

static struct bt_bap_lc3_preset preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static uint8_t bis_codec_data[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
			    BT_BYTES_LIST_LE32(BT_AUDIO_LOCATION_FRONT_CENTER)),
};

CREATE_FLAG(flag_source_started);
CREATE_FLAG(flag_source_stopped);

static K_SEM_DEFINE(sem_stream_started, 0U, ARRAY_SIZE(broadcast_source_streams));
static K_SEM_DEFINE(sem_stream_stopped, 0U, ARRAY_SIZE(broadcast_source_streams));

/* Broadcast Assistant variables */
CREATE_FLAG(flag_discovery_complete);
CREATE_FLAG(flag_write_complete);
CREATE_FLAG(flag_recv_state_updated);
CREATE_FLAG(flag_recv_state_removed);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_broadcast_code_requested);
CREATE_FLAG(flag_recv_state_updated_with_bis_sync);
CREATE_FLAG(flag_pa_info_req);

static struct bt_le_per_adv_sync *g_pa_sync;
static volatile uint8_t g_recv_state_count;
static struct bt_bap_scan_delegator_recv_state recv_state;

/* Streaming parameters */
#define STREAM_CNT   1U
#define SUBGROUP_CNT 1U
#define BIS_INDEX    1U

/*
 * Broadcast Source callbacks and helpers
 */

static void stream_started_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);
	int err;

	test_stream->seq_num = 0U;
	test_stream->tx_cnt = 0U;

	err = bap_stream_tx_register(stream);
	if (err != 0) {
		FAIL("Failed to register stream %p for TX: %d\n", stream, err);
		return;
	}

	printk("Stream %p started\n", stream);
	k_sem_give(&sem_stream_started);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	printk("Stream %p stopped with reason 0x%02X\n", stream, reason);

	err = bap_stream_tx_unregister(stream);
	if (err != 0) {
		FAIL("Failed to unregister stream %p for TX: %d\n", stream, err);
		return;
	}

	k_sem_give(&sem_stream_stopped);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.sent = bap_stream_tx_sent_cb,
};

static void source_started_cb(struct bt_bap_broadcast_source *source)
{
	printk("Broadcast source %p started\n", source);
	SET_FLAG(flag_source_started);
}

static void source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	printk("Broadcast source %p stopped with reason 0x%02X\n", source, reason);
	UNSET_FLAG(flag_source_started);
	SET_FLAG(flag_source_stopped);
}

static struct bt_bap_broadcast_source_cb broadcast_source_cbs = {
	.started = source_started_cb,
	.stopped = source_stopped_cb,
};

/*
 * Broadcast Assistant callbacks
 */

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err != 0) {
		FAIL("BASS discover failed (%d)\n", err);
		return;
	}

	printk("BASS discover done with %u recv states\n", recv_state_count);
	g_recv_state_count = recv_state_count;
	SET_FLAG(flag_discovery_complete);
}

static void
bap_broadcast_assistant_recv_state_cb(struct bt_conn *conn, int err,
				      const struct bt_bap_scan_delegator_recv_state *state)
{
	if (err != 0) {
		FAIL("BASS recv state read failed (%d)\n", err);
		return;
	}

	if (state == NULL) {
		return;
	}

	printk("BASS recv state: src_id %u, pa_sync_state %u, encrypt_state %u, num_subgroups %u\n",
	       state->src_id, state->pa_sync_state, state->encrypt_state, state->num_subgroups);

	if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BCODE_REQ) {
		SET_FLAG(flag_broadcast_code_requested);
	}

	if (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		SET_FLAG(flag_pa_info_req);
	}

	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		printk("\t[%d]: BIS sync 0x%08x\n", i, subgroup->bis_sync);

		if (subgroup->bis_sync != 0) {
			SET_FLAG(flag_recv_state_updated_with_bis_sync);
		}
	}

	memcpy(&recv_state, state, sizeof(recv_state));
	SET_FLAG(flag_recv_state_updated);
}

static void bap_broadcast_assistant_recv_state_removed_cb(struct bt_conn *conn, uint8_t src_id)
{
	printk("BASS recv state %u removed\n", src_id);
	SET_FLAG(flag_recv_state_removed);
}

static void bap_broadcast_assistant_add_src_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS add source failed (%d)\n", err);
		return;
	}

	printk("BASS add source successful\n");
	SET_FLAG(flag_write_complete);
}

static void bap_broadcast_assistant_mod_src_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS modify source failed (%d)\n", err);
		return;
	}

	printk("BASS modify source successful\n");
	SET_FLAG(flag_write_complete);
}

static void bap_broadcast_assistant_broadcast_code_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS broadcast code failed (%d)\n", err);
		return;
	}

	printk("BASS broadcast code successful\n");
	SET_FLAG(flag_write_complete);
}

static void bap_broadcast_assistant_rem_src_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS remove source failed (%d)\n", err);
		return;
	}

	printk("BASS remove source successful\n");
	SET_FLAG(flag_write_complete);
}

static struct bt_bap_broadcast_assistant_cb broadcast_assistant_cbs = {
	.discover = bap_broadcast_assistant_discover_cb,
	.recv_state = bap_broadcast_assistant_recv_state_cb,
	.recv_state_removed = bap_broadcast_assistant_recv_state_removed_cb,
	.add_src = bap_broadcast_assistant_add_src_cb,
	.mod_src = bap_broadcast_assistant_mod_src_cb,
	.broadcast_code = bap_broadcast_assistant_broadcast_code_cb,
	.rem_src = bap_broadcast_assistant_rem_src_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged: %u/%u\n", tx, rx);
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

/*
 * Broadcast Source setup functions
 */

static int setup_broadcast_source(bool encryption)
{
	struct bt_bap_broadcast_source_stream_param stream_params[STREAM_CNT];
	struct bt_bap_broadcast_source_subgroup_param subgroup_params[SUBGROUP_CNT];
	struct bt_bap_broadcast_source_param create_param;
	int err;

	for (size_t i = 0U; i < STREAM_CNT; i++) {
		stream_params[i].stream =
			bap_stream_from_audio_test_stream(&broadcast_source_streams[i]);
		bt_bap_stream_cb_register(stream_params[i].stream, &stream_ops);
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data);
		stream_params[i].data = bis_codec_data;
#endif
	}

	for (size_t i = 0U; i < SUBGROUP_CNT; i++) {
		subgroup_params[i].params_count = STREAM_CNT;
		subgroup_params[i].params = stream_params;
		subgroup_params[i].codec_cfg = &preset_16_2_1.codec_cfg;
	}

	create_param.params_count = SUBGROUP_CNT;
	create_param.params = subgroup_params;
	create_param.qos = &preset_16_2_1.qos;
	create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	create_param.encryption = encryption;
	if (encryption) {
		memcpy(create_param.broadcast_code, BROADCAST_CODE, BT_ISO_BROADCAST_CODE_SIZE);
	}

	printk("Creating broadcast source (encryption=%d)\n", encryption);
	err = bt_bap_broadcast_source_create(&create_param, &broadcast_source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d\n", err);
		return err;
	}

	for (size_t i = 0U; i < STREAM_CNT; i++) {
		broadcast_source_streams[i].tx_sdu_size = preset_16_2_1.qos.sdu;
	}

	return 0;
}

static int setup_extended_adv(void)
{
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);
	struct bt_data ext_ad;
	struct bt_data per_ad;
	int err;

	setup_broadcast_adv(&broadcast_adv);

	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err != 0) {
		FAIL("Unable to generate broadcast ID: %d\n", err);
		return err;
	}

	/* Mask to 24-bit */
	broadcast_id &= 0xFFFFFFU;

	printk("Broadcast ID: 0x%06X\n", broadcast_id);

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);
	ext_ad.type = BT_DATA_SVC_DATA16;
	ext_ad.data_len = ad_buf.len;
	ext_ad.data = ad_buf.data;

	err = bt_le_ext_adv_set_data(broadcast_adv, &ext_ad, 1, NULL, 0);
	if (err != 0) {
		FAIL("Failed to set extended advertising data: %d\n", err);
		return err;
	}

	/* Setup periodic advertising data (BASE) */
	err = bt_bap_broadcast_source_get_base(broadcast_source, &base_buf);
	if (err != 0) {
		FAIL("Failed to get BASE: %d\n", err);
		return err;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;

	err = bt_le_per_adv_set_data(broadcast_adv, &per_ad, 1);
	if (err != 0) {
		FAIL("Failed to set periodic advertising data: %d\n", err);
		return err;
	}

	start_broadcast_adv(broadcast_adv);

	return 0;
}

static void start_broadcast_source(void)
{
	int err;

	printk("Starting broadcast source\n");
	err = bt_bap_broadcast_source_start(broadcast_source, broadcast_adv);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for streams to start */
	for (size_t i = 0U; i < STREAM_CNT; i++) {
		k_sem_take(&sem_stream_started, K_FOREVER);
	}

	WAIT_FOR_FLAG(flag_source_started);
	printk("Broadcast source started\n");
}

static void stop_broadcast_source(void)
{
	int err;

	printk("Stopping broadcast source\n");
	err = bt_bap_broadcast_source_stop(broadcast_source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for streams to stop */
	for (size_t i = 0U; i < STREAM_CNT; i++) {
		k_sem_take(&sem_stream_stopped, K_FOREVER);
	}

	WAIT_FOR_FLAG(flag_source_stopped);
	printk("Broadcast source stopped\n");
}

static void delete_broadcast_source(void)
{
	int err;

	err = bt_bap_broadcast_source_delete(broadcast_source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d\n", err);
		return;
	}

	broadcast_source = NULL;

	/* Stop and delete advertising */
	err = bt_le_per_adv_stop(broadcast_adv);
	if (err != 0) {
		FAIL("Failed to stop periodic advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_stop(broadcast_adv);
	if (err != 0) {
		FAIL("Failed to stop extended advertising: %d\n", err);
		return;
	}

	err = bt_le_ext_adv_delete(broadcast_adv);
	if (err != 0) {
		FAIL("Failed to delete extended advertising: %d\n", err);
		return;
	}

	broadcast_adv = NULL;
	printk("Broadcast source deleted\n");
}

/*
 * Broadcast Assistant helper functions
 */

static void discover_bass(void)
{
	int err;

	printk("Discovering BASS\n");
	UNSET_FLAG(flag_discovery_complete);

	err = bt_bap_broadcast_assistant_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover BASS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovery_complete);
	printk("BASS discovered with %u receive states\n", g_recv_state_count);
}

static void add_broadcast_source(void)
{
	struct bt_bap_broadcast_assistant_add_src_param add_src_param = {0};
	struct bt_bap_bass_subgroup subgroup = {0};
	struct bt_le_ext_adv_info adv_info;
	int err;

	err = bt_le_ext_adv_get_info(broadcast_adv, &adv_info);
	if (err != 0) {
		FAIL("Failed to get adv info: %d\n", err);
		return;
	}

	printk("Adding broadcast source to scan delegator\n");
	UNSET_FLAG(flag_write_complete);
	UNSET_FLAG(flag_recv_state_updated);

	bt_addr_le_copy(&add_src_param.addr, adv_info.addr);
	add_src_param.adv_sid = adv_info.sid;
	add_src_param.pa_sync = false; /* Will be set via modify_source */
	add_src_param.broadcast_id = broadcast_id;
	add_src_param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	add_src_param.num_subgroups = SUBGROUP_CNT;
	add_src_param.subgroups = &subgroup;
	subgroup.bis_sync = 0;
	subgroup.metadata_len = 0;

	err = bt_bap_broadcast_assistant_add_src(default_conn, &add_src_param);
	if (err != 0) {
		FAIL("Failed to add source: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	WAIT_FOR_FLAG(flag_recv_state_updated);
	printk("Broadcast source added (src_id=%u)\n", recv_state.src_id);
}

static void modify_source_sync_bis(bool pa_sync, uint32_t bis_sync)
{
	struct bt_bap_broadcast_assistant_mod_src_param mod_src_param = {0};
	struct bt_bap_bass_subgroup subgroup = {0};
	int err;

	printk("Modifying source to sync BIS (pa_sync=%d)\n", pa_sync);
	UNSET_FLAG(flag_write_complete);
	UNSET_FLAG(flag_recv_state_updated);
	UNSET_FLAG(flag_recv_state_updated_with_bis_sync);

	mod_src_param.src_id = recv_state.src_id;
	mod_src_param.pa_sync = pa_sync;
	mod_src_param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	mod_src_param.num_subgroups = SUBGROUP_CNT;
	mod_src_param.subgroups = &subgroup;
	subgroup.bis_sync = bis_sync;
	subgroup.metadata_len = 0;

	err = bt_bap_broadcast_assistant_mod_src(default_conn, &mod_src_param);
	if (err != 0) {
		FAIL("Failed to modify source: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Source modified\n");
}

static void set_broadcast_code(void)
{
	int err;

	printk("Setting broadcast code\n");
	UNSET_FLAG(flag_write_complete);

	err = bt_bap_broadcast_assistant_set_broadcast_code(default_conn, recv_state.src_id,
							    BROADCAST_CODE);
	if (err != 0) {
		FAIL("Failed to set broadcast code: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Broadcast code set\n");
}

static void remove_broadcast_source(void)
{
	int err;

	printk("Removing broadcast source from scan delegator\n");
	UNSET_FLAG(flag_write_complete);
	UNSET_FLAG(flag_recv_state_removed);

	err = bt_bap_broadcast_assistant_rem_src(default_conn, recv_state.src_id);
	if (err != 0) {
		FAIL("Failed to remove source: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	WAIT_FOR_FLAG(flag_recv_state_removed);
	printk("Broadcast source removed\n");
}

static void wait_for_bis_sync(void)
{
	printk("Waiting for BIS sync\n");
	WAIT_FOR_FLAG(flag_recv_state_updated_with_bis_sync);
	printk("BIS synced\n");
}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
static void create_pa_sync(void)
{
	struct bt_le_per_adv_sync_param param = {0};
	struct bt_le_ext_adv_info adv_info;
	int err;

	err = bt_le_ext_adv_get_info(broadcast_adv, &adv_info);
	if (err != 0) {
		FAIL("Failed to get adv info: %d\n", err);
		return;
	}

	bt_addr_le_copy(&param.addr, adv_info.addr);
	param.sid = adv_info.sid;
	param.skip = 0;
	param.timeout = 1000; /* 10 seconds */

	printk("Creating PA sync to own broadcast (addr %s, SID %u)\n",
	       bt_addr_le_str(adv_info.addr), adv_info.sid);

	err = bt_le_per_adv_sync_create(&param, &g_pa_sync);
	if (err != 0) {
		FAIL("Failed to create PA sync: %d\n", err);
		return;
	}

	printk("PA sync created\n");
}

static void transfer_pa_sync_info(void)
{
	int err;

	printk("Transferring PA sync info via PAST\n");

	/* Since we ARE the broadcast source, use bt_le_per_adv_set_info_transfer */
	err = bt_le_per_adv_set_info_transfer(broadcast_adv, default_conn, BT_UUID_BASS_VAL);
	if (err != 0) {
		FAIL("Failed to transfer PA sync info: %d\n", err);
		return;
	}

	printk("PA sync info transferred\n");
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */

/*
 * Common initialization
 */

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed: %d\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bap_stream_tx_init();

	err = bt_bap_broadcast_source_register_cb(&broadcast_source_cbs);
	if (err != 0) {
		FAIL("Failed to register broadcast source callbacks: %d\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);
	bt_bap_broadcast_assistant_register_cb(&broadcast_assistant_cbs);
	bt_le_scan_cb_register(&common_scan_cb);
}

static void connect_to_scan_delegator(void)
{
	int err;

	printk("Scanning for scan delegator\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	printk("Connected to scan delegator\n");

	WAIT_FOR_FLAG(flag_mtu_exchanged);
	printk("MTU exchanged\n");

	update_security(default_conn);
}

static void update_conn_params(void)
{
	/* Set connection parameters to something that is not a multiple of the PA interval or ISO
	 * interval to avoid issues with e.g. PAST.
	 * 35 ms fits nicely as it is not a multiple of neither BT_BAP_ADV_PARAM_BROADCAST_SLOW,
	 * nor 10 ms for ISO
	 */
	int err;

	UNSET_FLAG(flag_conn_updated);
	err = bt_conn_le_param_update(default_conn,
				      BT_LE_CONN_PARAM(BT_GAP_MS_TO_CONN_INTERVAL(35),
						       BT_GAP_MS_TO_CONN_INTERVAL(35), 0,
						       BT_GAP_MS_TO_CONN_TIMEOUT(4000)));
	if (err != 0) {
		FAIL("Failed to update connection parameters %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_conn_updated);
}

static void disconnect_from_scan_delegator(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_disconnected);
	printk("Disconnected\n");
}

/*
 * Test case: Collocated source+assistant for use with broadcast_sink_with_assistant
 * This test uses backchannel sync to coordinate with a broadcast sink that receives audio.
 */
static void test_main_for_broadcast_sink(void)
{
	init();

	/* Step 1: Setup and start broadcast source (no encryption) */
	if (setup_broadcast_source(false) != 0) {
		return;
	}
	if (setup_extended_adv() != 0) {
		return;
	}
	start_broadcast_source();

	/* Step 2: Connect to broadcast sink */
	connect_to_scan_delegator();

	update_conn_params();

	/* Step 3: Discover BASS and add ourselves as broadcast source */
	discover_bass();
	add_broadcast_source();

	/* Step 4: Modify source to request PA sync and BIS sync */
	modify_source_sync_bis(true, BT_ISO_BIS_INDEX_BIT(BIS_INDEX));

	/* Step 5: Wait for BIS sync confirmation */
	wait_for_bis_sync();

	/* Step 6: Wait for sink to receive data (backchannel sync) */
	printk("Waiting for sink to receive data\n");
	backchannel_sync_wait_all();

	/* Step 7: Remove source and request cleanup */
	modify_source_sync_bis(false, 0);
	remove_broadcast_source();
	disconnect_from_scan_delegator();

	/* Step 8: Wait for sink to finish cleanup (backchannel sync) */
	printk("Waiting for sink to finish cleanup\n");
	backchannel_sync_wait_all();

	/* Step 9: Stop and delete broadcast source */
	stop_broadcast_source();
	delete_broadcast_source();

	PASS("Collocated Source+Assistant for Broadcast Sink test passed\n");
}

/*
 * Test case: Collocated source+assistant with encryption for use with broadcast_sink
 */
static void test_main_for_broadcast_sink_encrypted(void)
{
	init();

	/* Step 1: Setup and start encrypted broadcast source */
	if (setup_broadcast_source(true) != 0) {
		return;
	}
	if (setup_extended_adv() != 0) {
		return;
	}
	start_broadcast_source();

	/* Step 2: Connect to broadcast sink */
	connect_to_scan_delegator();

	update_conn_params();

	/* Step 3: Discover BASS and add ourselves as broadcast source */
	discover_bass();
	add_broadcast_source();

	/* Step 4: Modify source to request PA sync and BIS sync */
	modify_source_sync_bis(true, BT_ISO_BIS_INDEX_BIT(BIS_INDEX));

	/* Step 5: Wait for broadcast code request */
	printk("Waiting for broadcast code request\n");
	WAIT_FOR_FLAG(flag_broadcast_code_requested);

	/* Step 6: Set broadcast code */
	set_broadcast_code();

	/* Step 7: Wait for BIS sync confirmation */
	wait_for_bis_sync();

	/* Step 8: Wait for sink to receive data (backchannel sync) */
	printk("Waiting for sink to receive data\n");
	backchannel_sync_wait_all();

	/* Step 9: Remove source and request cleanup */
	modify_source_sync_bis(false, 0);
	remove_broadcast_source();
	disconnect_from_scan_delegator();

	/* Step 10: Wait for sink to finish cleanup (backchannel sync) */
	printk("Waiting for sink to finish cleanup\n");
	backchannel_sync_wait_all();

	/* Step 11: Stop and delete broadcast source */
	stop_broadcast_source();
	delete_broadcast_source();

	PASS("Collocated Source+Assistant Encrypted for Broadcast Sink test passed\n");
}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
/*
 * Test case: Collocated source+assistant with PAST for use with broadcast_sink
 */
static void test_main_for_broadcast_sink_past(void)
{
	init();

	/* Step 1: Setup and start broadcast source (no encryption) */
	if (setup_broadcast_source(false) != 0) {
		return;
	}
	if (setup_extended_adv() != 0) {
		return;
	}
	start_broadcast_source();

	/* Step 2: Connect to broadcast sink */
	connect_to_scan_delegator();

	update_conn_params();

	/* Step 3: Discover BASS */
	discover_bass();

	/* Step 4: Create PA sync to our own broadcast (needed for PAST) */
	create_pa_sync();

	/* Step 5: Add ourselves as broadcast source */
	add_broadcast_source();

	/* Step 6: Modify source to request PA sync and BIS sync */
	modify_source_sync_bis(true, BT_ISO_BIS_INDEX_BIT(BIS_INDEX));

	/* Step 7: Wait for INFO_REQ state and transfer PA info via PAST */
	printk("Waiting for PA INFO_REQ\n");
	WAIT_FOR_FLAG(flag_pa_info_req);
	transfer_pa_sync_info();

	/* Step 8: Wait for BIS sync confirmation */
	wait_for_bis_sync();

	/* Step 9: Wait for sink to receive data (backchannel sync) */
	printk("Waiting for sink to receive data\n");
	backchannel_sync_wait_all();

	/* Step 10: Remove source and request cleanup */
	modify_source_sync_bis(false, 0);
	remove_broadcast_source();
	disconnect_from_scan_delegator();

	/* Step 11: Wait for sink to finish cleanup (backchannel sync) */
	printk("Waiting for sink to finish cleanup\n");
	backchannel_sync_wait_all();

	/* Step 12: Stop and delete broadcast source */
	stop_broadcast_source();
	delete_broadcast_source();

	PASS("Collocated Source+Assistant PAST for Broadcast Sink test passed\n");
}

/*
 * Test case: Collocated source+assistant with encryption and PAST for use with broadcast_sink
 */
static void test_main_for_broadcast_sink_encrypted_past(void)
{
	init();

	/* Step 1: Setup and start encrypted broadcast source */
	if (setup_broadcast_source(true) != 0) {
		return;
	}
	if (setup_extended_adv() != 0) {
		return;
	}
	start_broadcast_source();

	/* Step 2: Connect to broadcast sink */
	connect_to_scan_delegator();

	update_conn_params();

	/* Step 3: Discover BASS */
	discover_bass();

	/* Step 4: Create PA sync to our own broadcast (needed for PAST) */
	create_pa_sync();

	/* Step 5: Add ourselves as broadcast source */
	add_broadcast_source();

	/* Step 6: Modify source to request PA sync and BIS sync */
	modify_source_sync_bis(true, BT_ISO_BIS_INDEX_BIT(BIS_INDEX));

	/* Step 7: Wait for INFO_REQ state and transfer PA info via PAST */
	printk("Waiting for PA INFO_REQ\n");
	WAIT_FOR_FLAG(flag_pa_info_req);
	transfer_pa_sync_info();

	/* Step 8: Wait for broadcast code request */
	printk("Waiting for broadcast code request\n");
	WAIT_FOR_FLAG(flag_broadcast_code_requested);

	/* Step 9: Set broadcast code */
	set_broadcast_code();

	/* Step 10: Wait for BIS sync confirmation */
	wait_for_bis_sync();

	/* Step 11: Wait for sink to receive data (backchannel sync) */
	printk("Waiting for sink to receive data\n");
	backchannel_sync_wait_all();

	/* Step 12: Remove source and request cleanup */
	modify_source_sync_bis(false, 0);
	remove_broadcast_source();
	disconnect_from_scan_delegator();

	/* Step 13: Wait for sink to finish cleanup (backchannel sync) */
	printk("Waiting for sink to finish cleanup\n");
	backchannel_sync_wait_all();

	/* Step 14: Stop and delete broadcast source */
	stop_broadcast_source();
	delete_broadcast_source();

	PASS("Collocated Source+Assistant Encrypted+PAST for Broadcast Sink test passed\n");
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */

static const struct bst_test_instance test_collocated[] = {
	{
		.test_id = "bap_broadcast_source_assistant_for_broadcast_sink",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_for_broadcast_sink,
	},
	{
		.test_id = "bap_broadcast_source_assistant_for_broadcast_sink_encrypted",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_for_broadcast_sink_encrypted,
	},
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	{
		.test_id = "bap_broadcast_source_assistant_for_broadcast_sink_past",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_for_broadcast_sink_past,
	},
	{
		.test_id = "bap_broadcast_source_assistant_for_broadcast_sink_encrypted_past",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_for_broadcast_sink_encrypted_past,
	},
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */
	BSTEST_END_MARKER,
};

struct bst_test_list *
test_bap_broadcast_source_assistant_collocated_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_collocated);
}

#else /* CONFIG_BT_BAP_BROADCAST_SOURCE && CONFIG_BT_BAP_BROADCAST_ASSISTANT */

struct bst_test_list *
test_bap_broadcast_source_assistant_collocated_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE && CONFIG_BT_BAP_BROADCAST_ASSISTANT */
