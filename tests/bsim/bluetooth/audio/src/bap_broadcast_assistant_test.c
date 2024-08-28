/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "../../../../../subsys/bluetooth/host/hci_core.h"
#include "common.h"
#include "bap_common.h"
#include "bstests.h"

#ifdef CONFIG_BT_BAP_BROADCAST_ASSISTANT

extern enum bst_result_t bst_result;

/* BASS variables */
static volatile uint8_t g_src_id;
static volatile uint32_t g_broadcast_id;
static volatile uint8_t g_recv_state_count;
CREATE_FLAG(flag_discovery_complete);
CREATE_FLAG(flag_write_complete);
CREATE_FLAG(flag_cb_called);
CREATE_FLAG(flag_broadcaster_found);
CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_pa_terminated);
CREATE_FLAG(flag_state_synced);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_recv_state_read);
CREATE_FLAG(flag_recv_state_updated);
CREATE_FLAG(flag_recv_state_updated_with_bis_sync);
CREATE_FLAG(flag_recv_state_removed);
CREATE_FLAG(flag_broadcast_code_requested);
CREATE_FLAG(flag_incorrect_broadcast_code);

/* Broadcaster variables */
static bt_addr_le_t g_broadcaster_addr;
static struct bt_le_scan_recv_info g_broadcaster_info;
static struct bt_le_per_adv_sync *g_pa_sync;

static uint8_t metadata[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_VENDOR, LONG_META)};

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

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

static void bap_broadcast_assistant_scan_cb(const struct bt_le_scan_recv_info *info,
				uint32_t broadcast_id)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("Scan Recv: [DEVICE]: %s, broadcast_id 0x%06X, "
	       "interval (ms) %u), SID 0x%x, RSSI %i\n",
	       le_addr, broadcast_id, info->interval * 5 / 4,
	       info->sid, info->rssi);

	(void)memcpy(&g_broadcaster_info, info, sizeof(g_broadcaster_info));
	bt_addr_le_copy(&g_broadcaster_addr, info->addr);
	g_broadcast_id = broadcast_id;
	SET_FLAG(flag_broadcaster_found);
}

static bool metadata_entry(struct bt_data *data, void *user_data)
{
	char metadata[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE] = {0};

	(void)bin2hex(data->data, data->data_len, metadata, sizeof(metadata));

	printk("\t\tMetadata length %u, type %u, data: %s\n",
	       data->data_len, data->type, metadata);

	return true;
}

static void bap_broadcast_assistant_recv_state_cb(
	struct bt_conn *conn, int err,
	const struct bt_bap_scan_delegator_recv_state *state)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char bad_code[BT_AUDIO_BROADCAST_CODE_SIZE * 2 + 1];

	if (err != 0) {
		FAIL("BASS recv state read failed (%d)\n", err);
		return;
	}

	SET_FLAG(flag_recv_state_read);

	if (state == NULL) {
		/* Empty receive state */
		return;
	}

	bt_addr_le_to_str(&state->addr, le_addr, sizeof(le_addr));
	(void)bin2hex(state->bad_code, BT_AUDIO_BROADCAST_CODE_SIZE, bad_code,
		      sizeof(bad_code));
	printk("BASS recv state: src_id %u, addr %s, sid %u, sync_state %u, encrypt_state %u%s%s\n",
	       state->src_id, le_addr, state->adv_sid, state->pa_sync_state, state->encrypt_state,
	       state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE ? ", bad code: " : "",
	       bad_code);

	if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BCODE_REQ) {
		SET_FLAG(flag_broadcast_code_requested);
	} else if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		SET_FLAG(flag_incorrect_broadcast_code);
		if (memcmp(state->bad_code, INCORRECT_BROADCAST_CODE,
			   BT_AUDIO_BROADCAST_CODE_SIZE) != 0) {
			FAIL("Bad code is not what we sent");
			return;
		}

		for (uint8_t i = 0; i < state->num_subgroups; i++) {
			const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

			if (subgroup->bis_sync != BT_BAP_BIS_SYNC_FAILED) {
				FAIL("Invalid BIS sync value 0x%08X for failed sync",
				     subgroup->bis_sync);
				return;
			}
		}
	}

	for (uint8_t i = 0; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];
		struct net_buf_simple buf;

		printk("\t[%d]: BIS sync %u, metadata_len %u\n",
		       i, subgroup->bis_sync, subgroup->metadata_len);

		net_buf_simple_init_with_data(&buf, (void *)subgroup->metadata,
					      subgroup->metadata_len);
		bt_data_parse(&buf, metadata_entry, NULL);

		if (subgroup->bis_sync != 0) {
			SET_FLAG(flag_recv_state_updated_with_bis_sync);
		}
	}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	if (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		err = bt_le_per_adv_sync_transfer(g_pa_sync, conn,
						  BT_UUID_BASS_VAL);
		if (err != 0) {
			FAIL("Could not transfer periodic adv sync: %d\n", err);
			return;
		}
	}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER */

	if (state->pa_sync_state == BT_BAP_PA_STATE_SYNCED) {
		SET_FLAG(flag_state_synced);
	}

	g_src_id = state->src_id;
	SET_FLAG(flag_cb_called);

	SET_FLAG(flag_recv_state_updated);
}

static void bap_broadcast_assistant_recv_state_removed_cb(struct bt_conn *conn, uint8_t src_id)
{
	printk("BASS recv state %u removed\n", src_id);
	SET_FLAG(flag_cb_called);

	SET_FLAG(flag_recv_state_removed);
}

static void bap_broadcast_assistant_scan_start_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS scan start failed (%d)\n", err);
		return;
	}

	printk("BASS scan start successful\n");
	SET_FLAG(flag_write_complete);
}

static void bap_broadcast_assistant_scan_stop_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		FAIL("BASS scan stop failed (%d)\n", err);
		return;
	}

	printk("BASS scan stop successful\n");
	SET_FLAG(flag_write_complete);
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
	.scan = bap_broadcast_assistant_scan_cb,
	.recv_state = bap_broadcast_assistant_recv_state_cb,
	.recv_state_removed = bap_broadcast_assistant_recv_state_removed_cb,
	.scan_start = bap_broadcast_assistant_scan_start_cb,
	.scan_stop = bap_broadcast_assistant_scan_stop_cb,
	.add_src = bap_broadcast_assistant_add_src_cb,
	.mod_src = bap_broadcast_assistant_mod_src_cb,
	.broadcast_code = bap_broadcast_assistant_broadcast_code_cb,
	.rem_src = bap_broadcast_assistant_rem_src_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->interval,
	       info->interval * 5 / 4, phy2str(info->phy));

	SET_FLAG(flag_pa_synced);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	SET_FLAG(flag_pa_terminated);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
};

static void test_exchange_mtu(void)
{
	WAIT_FOR_FLAG(flag_mtu_exchanged);
	printk("MTU exchanged\n");
}

static void test_bass_discover(void)
{
	int err;

	printk("Discovering BASS\n");
	UNSET_FLAG(flag_discovery_complete);
	err = bt_bap_broadcast_assistant_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover BASS %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovery_complete);

	/* Verify that we can discover again */
	UNSET_FLAG(flag_discovery_complete);
	err = bt_bap_broadcast_assistant_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover BASS for the second time: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovery_complete);
	printk("Discovery complete\n");
}

static void test_bass_read_receive_states(void)
{
	for (uint8_t i = 0U; i < g_recv_state_count; i++) {
		int err;

		UNSET_FLAG(flag_recv_state_read);
		err = bt_bap_broadcast_assistant_read_recv_state(default_conn, i);
		if (err != 0) {
			FAIL("Failed to read receive state with idx %u: %d\n",
			     i, err);
			return;
		}

		WAIT_FOR_FLAG(flag_recv_state_read);
	}

	printk("Receive state read complete\n");
}

static void test_bass_scan_start(void)
{
	int err;

	printk("Starting scan\n");
	UNSET_FLAG(flag_write_complete);
	err = bt_bap_broadcast_assistant_scan_start(default_conn, true);
	if (err != 0) {
		FAIL("Could not write scan start to BASS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	WAIT_FOR_FLAG(flag_broadcaster_found);
	printk("Scan started\n");
}

static void test_bass_scan_stop(void)
{
	int err;

	printk("Stopping scan\n");
	UNSET_FLAG(flag_write_complete);
	err = bt_bap_broadcast_assistant_scan_stop(default_conn);
	if (err != 0) {
		FAIL("Could not write scan stop to BASS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Scan stopped\n");
}

static void test_bass_create_pa_sync(void)
{
	int err;
	struct bt_le_per_adv_sync_param sync_create_param = { 0 };

	printk("Creating Periodic Advertising Sync...\n");
	bt_addr_le_copy(&sync_create_param.addr, &g_broadcaster_addr);
	sync_create_param.sid = g_broadcaster_info.sid;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &g_pa_sync);
	if (err != 0) {
		FAIL("Could not create PA syncs (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_pa_synced);
	printk("PA synced\n");
}

static void test_bass_add_source(void)
{
	int err;
	struct bt_bap_broadcast_assistant_add_src_param add_src_param = { 0 };
	struct bt_bap_bass_subgroup subgroup = { 0 };

	printk("Adding source\n");
	UNSET_FLAG(flag_write_complete);
	UNSET_FLAG(flag_cb_called);
	bt_addr_le_copy(&add_src_param.addr, &g_broadcaster_addr);
	add_src_param.adv_sid = g_broadcaster_info.sid;
	add_src_param.num_subgroups = 1;
	add_src_param.pa_interval = g_broadcaster_info.interval;
	add_src_param.pa_sync = false;
	add_src_param.broadcast_id = g_broadcast_id;
	add_src_param.subgroups = &subgroup;
	subgroup.bis_sync = 0;
	subgroup.metadata_len = 0;
	err = bt_bap_broadcast_assistant_add_src(default_conn, &add_src_param);
	if (err != 0) {
		FAIL("Could not add source (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_cb_called);
	WAIT_FOR_FLAG(flag_write_complete);
	printk("Source added\n");
}

static void test_bass_mod_source(void)
{
	int err;
	struct bt_bap_broadcast_assistant_mod_src_param mod_src_param = { 0 };
	struct bt_bap_bass_subgroup subgroup = { 0 };

	printk("Modify source\n");
	UNSET_FLAG(flag_cb_called);
	UNSET_FLAG(flag_write_complete);
	mod_src_param.src_id = g_src_id;
	mod_src_param.num_subgroups = 1;
	mod_src_param.pa_sync = true;
	mod_src_param.subgroups = &subgroup;
	mod_src_param.pa_interval = g_broadcaster_info.interval;
	subgroup.bis_sync = BT_ISO_BIS_INDEX_BIT(1) | BT_ISO_BIS_INDEX_BIT(2); /* Indexes 1 and 2 */
	subgroup.metadata_len = 0;

	err = bt_bap_broadcast_assistant_mod_src(default_conn, &mod_src_param);
	if (err != 0) {
		FAIL("Could not modify source (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_cb_called);
	WAIT_FOR_FLAG(flag_write_complete);
	printk("Source added, waiting for server to PA sync\n");
	WAIT_FOR_FLAG(flag_state_synced)
	printk("Server PA synced\n");
}

static void test_bass_mod_source_long_meta(void)
{
	int err;
	struct bt_bap_broadcast_assistant_mod_src_param mod_src_param = { 0 };
	struct bt_bap_bass_subgroup subgroup = { 0 };

	printk("Long write\n");
	UNSET_FLAG(flag_cb_called);
	UNSET_FLAG(flag_write_complete);
	mod_src_param.src_id = g_src_id;
	mod_src_param.num_subgroups = 1;
	mod_src_param.pa_sync = true;
	mod_src_param.subgroups = &subgroup;
	mod_src_param.pa_interval = g_broadcaster_info.interval;
	subgroup.bis_sync = BT_ISO_BIS_INDEX_BIT(1) | BT_ISO_BIS_INDEX_BIT(2);

	subgroup.metadata_len = sizeof(metadata);
	memcpy(subgroup.metadata, metadata, sizeof(metadata));
	err = bt_bap_broadcast_assistant_mod_src(default_conn, &mod_src_param);
	if (err != 0) {
		FAIL("Could not modify source (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_cb_called);
	WAIT_FOR_FLAG(flag_write_complete);
	printk("Source added, waiting for server to PA sync\n");
	WAIT_FOR_FLAG(flag_state_synced)
	printk("Server PA synced\n");
}

static void test_bass_broadcast_code(const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	int err;

	printk("Adding broadcast code\n");
	UNSET_FLAG(flag_write_complete);
	err = bt_bap_broadcast_assistant_set_broadcast_code(default_conn, g_src_id, broadcast_code);
	if (err != 0) {
		FAIL("Could not add broadcast code (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Broadcast code added\n");
}

static void test_bass_remove_source(void)
{
	int err;

	printk("Removing source\n");
	UNSET_FLAG(flag_cb_called);
	UNSET_FLAG(flag_write_complete);
	err = bt_bap_broadcast_assistant_rem_src(default_conn, g_src_id);
	if (err != 0) {
		FAIL("Could not remove source (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_cb_called);
	WAIT_FOR_FLAG(flag_write_complete);
	printk("Source removed\n");
}

static int common_init(void)
{
	int err;

	err = bt_enable(NULL);

	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return err;
	}

	bt_gatt_cb_register(&gatt_callbacks);
	bt_bap_broadcast_assistant_register_cb(&broadcast_assistant_cbs);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	bt_le_scan_cb_register(&common_scan_cb);

	printk("Starting scan\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return err;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	test_exchange_mtu();
	test_bass_discover();
	test_bass_read_receive_states();

	return 0;
}

static void test_main_client_sync(void)
{
	int err;

	err = common_init();
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	test_bass_scan_start();
	test_bass_scan_stop();
	test_bass_create_pa_sync();
	test_bass_add_source();
	test_bass_mod_source();
	test_bass_mod_source_long_meta();
	test_bass_broadcast_code(BROADCAST_CODE);

	printk("Waiting for receive state with BIS sync\n");
	WAIT_FOR_FLAG(flag_recv_state_updated_with_bis_sync);

	test_bass_remove_source();

	PASS("BAP Broadcast Assistant Client Sync Passed\n");
}

static void test_main_client_sync_incorrect_code(void)
{
	int err;

	err = common_init();
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	test_bass_scan_start();
	test_bass_scan_stop();
	test_bass_create_pa_sync();
	test_bass_add_source();
	test_bass_mod_source();
	WAIT_FOR_FLAG(flag_broadcast_code_requested);
	test_bass_broadcast_code(INCORRECT_BROADCAST_CODE);
	WAIT_FOR_FLAG(flag_incorrect_broadcast_code);

	test_bass_remove_source();

	PASS("BAP Broadcast Assistant Client Sync Passed\n");
}

static void test_main_server_sync_client_rem(void)
{
	int err;

	err = common_init();
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_recv_state_updated);

	test_bass_broadcast_code(BROADCAST_CODE);

	printk("Waiting for receive state with BIS sync\n");
	WAIT_FOR_FLAG(flag_recv_state_updated_with_bis_sync);

	test_bass_remove_source();

	PASS("BAP Broadcast Assistant Server Sync Passed\n");
}

static void test_main_server_sync_server_rem(void)
{
	int err;

	err = common_init();
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_recv_state_updated);

	test_bass_broadcast_code(BROADCAST_CODE);

	printk("Waiting for receive state with BIS sync\n");
	WAIT_FOR_FLAG(flag_recv_state_updated_with_bis_sync);

	WAIT_FOR_FLAG(flag_recv_state_removed);

	PASS("BAP Broadcast Assistant Server Sync Passed\n");
}

static const struct bst_test_instance test_bass[] = {
	{
		.test_id = "bap_broadcast_assistant_client_sync",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_client_sync,
	},
	{
		.test_id = "bap_broadcast_assistant_client_sync_incorrect_code",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_client_sync_incorrect_code,
	},
	{
		.test_id = "bap_broadcast_assistant_server_sync_client_rem",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_server_sync_client_rem,
	},
	{
		.test_id = "bap_broadcast_assistant_server_sync_server_rem",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_server_sync_server_rem,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bap_broadcast_assistant_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_bass);
}

#else

struct bst_test_list *test_bap_broadcast_assistant_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
