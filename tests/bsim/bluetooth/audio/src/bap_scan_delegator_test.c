/*
 * Copyright (c) 2021-2025 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_BAP_SCAN_DELEGATOR
extern enum bst_result_t bst_result;

#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */
#define PA_SYNC_SKIP              5

CREATE_FLAG(flag_pa_synced);
CREATE_FLAG(flag_pa_terminated);
CREATE_FLAG(flag_broadcast_code_received);
CREATE_FLAG(flag_recv_state_updated);
CREATE_FLAG(flag_bis_sync_requested);
CREATE_FLAG(flag_bis_sync_term_requested);
CREATE_FLAG(flag_broadcast_source_added);
CREATE_FLAG(flag_broadcast_source_modified);
CREATE_FLAG(flag_broadcast_source_removed);
CREATE_FLAG(flag_remove_source_rejected);

static volatile uint32_t g_broadcast_id;
static bool reject_control_op;

struct sync_state {
	uint8_t src_id;
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	bool pa_syncing;
	struct k_work_delayable pa_timer;
	struct bt_le_per_adv_sync *pa_sync;
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
	uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
} sync_states[CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT];

static struct sync_state *sync_state_get(const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		if (sync_states[i].recv_state == recv_state) {
			return &sync_states[i];
		}
	}

	return NULL;
}

static struct sync_state *sync_state_get_or_new(
	const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct sync_state *free_state = NULL;

	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		if (sync_states[i].recv_state == NULL &&
		    free_state == NULL) {
			free_state = &sync_states[i];

			if (recv_state == NULL) {
				return free_state;
			}
		}

		if (sync_states[i].recv_state == recv_state) {
			return &sync_states[i];
		}
	}

	return free_state;
}

static struct sync_state *sync_state_get_by_pa(struct bt_le_per_adv_sync *sync)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		if (sync_states[i].pa_sync == sync) {
			return &sync_states[i];
		}
	}

	return NULL;
}

static struct sync_state *sync_state_get_by_src_id(uint8_t src_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		if (sync_states[i].src_id == src_id) {
			return &sync_states[i];
		}
	}

	return NULL;
}

static void pa_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sync_state *state = CONTAINER_OF(dwork, struct sync_state, pa_timer);

	state->pa_syncing = false;

	if (state->recv_state != NULL) {
		enum bt_bap_pa_state pa_state;
		int err;

		if (state->recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
			pa_state = BT_BAP_PA_STATE_NO_PAST;
		} else {
			pa_state = BT_BAP_PA_STATE_FAILED;
		}

		err = bt_bap_scan_delegator_set_pa_state(state->recv_state->src_id, pa_state);
		if (err != 0) {
			FAIL("Could not set PA sync state: %d\n", err);
			return;
		}
	}

	FAIL("PA timeout\n");
}

static int pa_sync_past(struct bt_conn *conn,
			struct sync_state *state,
			uint16_t pa_interval)
{
	struct bt_le_per_adv_sync_transfer_param param = { 0 };
	int err;

	param.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_FILTER_DUPLICATES;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(pa_interval);

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err != 0) {
		printk("Could not do PAST subscribe: %d\n", err);
	} else {
		printk("Syncing with PAST: %d\n", err);
		state->pa_syncing = true;
		k_work_init_delayable(&state->pa_timer, pa_timer_handler);
		(void)k_work_reschedule(&state->pa_timer,
					K_MSEC(param.timeout * 10));
	}

	return err;
}

static int pa_sync_no_past(struct sync_state *state,
			    uint16_t pa_interval)
{
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	struct bt_le_per_adv_sync_param param = { 0 };
	int err;

	recv_state = state->recv_state;
	state->src_id = recv_state->src_id;

	bt_addr_le_copy(&param.addr, &recv_state->addr);
	param.sid = recv_state->adv_sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(pa_interval);

	/* TODO: Validate that the advertise is broadcasting the same
	 * broadcast_id that the receive state has
	 */
	err = bt_le_per_adv_sync_create(&param, &state->pa_sync);
	if (err != 0) {
		printk("Could not sync per adv: %d\n", err);
	} else {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&recv_state->addr, addr_str, sizeof(addr_str));
		printk("PA sync pending for addr %s\n", addr_str);
		state->pa_syncing = true;
		k_work_init_delayable(&state->pa_timer, pa_timer_handler);
		(void)k_work_reschedule(&state->pa_timer,
					K_MSEC(param.timeout * 10));
	}

	return err;
}

static int pa_sync_term(struct sync_state *state)
{
	int err;

	(void)k_work_cancel_delayable(&state->pa_timer);

	if (state->pa_sync == NULL) {
		return -1;
	}

	printk("Deleting PA sync\n");

	err = bt_le_per_adv_sync_delete(state->pa_sync);
	if (err != 0) {
		FAIL("Could not delete per adv sync: %d\n", err);
	} else {
		state->pa_syncing = false;
		state->pa_sync = NULL;
	}

	return err;
}

static void recv_state_updated_cb(struct bt_conn *conn,
				  const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct sync_state *state;

	printk("Receive state with ID %u updated\n", recv_state->src_id);

	state = sync_state_get_by_src_id(recv_state->src_id);
	if (state == NULL) {
		FAIL("Could not get state");
		return;
	}

	if (state->recv_state != NULL) {
		if (state->recv_state != recv_state) {
			FAIL("Sync state receive state mismatch: %p - %p",
			     state->recv_state, recv_state);
			return;
		}
	} else {
		state->recv_state = recv_state;
	}

	SET_FLAG(flag_recv_state_updated);
}

static void reset_cp_flags(void)
{
	UNSET_FLAG(flag_broadcast_source_added);
	UNSET_FLAG(flag_broadcast_source_modified);
	UNSET_FLAG(flag_broadcast_source_removed);
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	struct sync_state *state;
	int err;

	reset_cp_flags();
	printk("PA Sync request: past_avail %u, pa_interval 0x%04x\n: %p",
	       past_avail, pa_interval, recv_state);

	state = sync_state_get_or_new(recv_state);
	if (state == NULL) {
		FAIL("Could not get state");
		return -1;
	}

	state->recv_state = recv_state;
	state->src_id = recv_state->src_id;

	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		/* Already syncing */
		/* TODO: Terminate existing sync and then sync to new?*/
		return -1;
	}

	if (past_avail) {
		err = pa_sync_past(conn, state, pa_interval);
		if (err == 0) {
			err = bt_bap_scan_delegator_set_pa_state(state->recv_state->src_id,
								 BT_BAP_PA_STATE_INFO_REQ);
			if (err != 0) {
				FAIL("Could not set PA sync state: %d\n", err);
				return err;
			}
		}
	} else {
		err = pa_sync_no_past(state, pa_interval);
	}

	return err;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct sync_state *state;

	printk("PA Sync term request for %p\n", recv_state);

	state = sync_state_get(recv_state);
	if (state == NULL) {
		FAIL("Could not get state\n");
		return -1;
	}

	return pa_sync_term(state);
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	struct sync_state *state;

	printk("Broadcast code received for %p\n", recv_state);

	state = sync_state_get(recv_state);
	if (state == NULL) {
		FAIL("Could not get state\n");
		return;
	}

	(void)memcpy(state->broadcast_code, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);

	SET_FLAG(flag_broadcast_code_received);
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	struct sync_state *state;
	bool sync_bis;

	printk("BIS sync request received for %p\n", recv_state);
	for (int i = 0; i < CONFIG_BT_BAP_BASS_MAX_SUBGROUPS; i++) {
		if (bis_sync_req[i]) {
			sync_bis = true;
		}

		printk("  [%d]: 0x%08x\n", i, bis_sync_req[i]);
	}

	state = sync_state_get(recv_state);
	if (state == NULL) {
		FAIL("Could not get state\n");
		return -1;
	}

	(void)memcpy(state->bis_sync_req, bis_sync_req,
		     sizeof(state->bis_sync_req));

	if (sync_bis) {
		SET_FLAG(flag_bis_sync_requested);
	} else {
		SET_FLAG(flag_bis_sync_term_requested);
	}

	return 0;
}

static int add_source_cb(struct bt_conn *conn,
	const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	printk("Add Source callback: src_id=%u\n", recv_state->src_id);
	SET_FLAG(flag_broadcast_source_added);
	return 0;
}

static int modify_source_cb(struct bt_conn *conn,
	   const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	printk("Modify Source callback: src_id=%u\n", recv_state->src_id);
	SET_FLAG(flag_broadcast_source_modified);
	return 0;
}

static int remove_source_cb(struct bt_conn *conn, uint8_t src_id)
{
	printk("Remove Source callback: src_id=%u\n", src_id);

	if (reject_control_op) {
		SET_FLAG(flag_remove_source_rejected);
		return BT_ATT_ERR_WRITE_REQ_REJECTED;
	}

	SET_FLAG(flag_broadcast_source_removed);
	return 0;
}

static struct bt_bap_scan_delegator_cb scan_delegator_cb = {
	.recv_state_updated = recv_state_updated_cb,
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.broadcast_code = broadcast_code_cb,
	.bis_sync_req = bis_sync_req_cb,
	.add_source = add_source_cb,
	.modify_source = modify_source_cb,
	.remove_source = remove_source_cb
};

static void pa_synced_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	struct sync_state *state;

	printk("PA %p synced\n", sync);

	if (info->conn) { /* if from PAST */
		for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
			if (!sync_states[i].pa_sync) {
				sync_states[i].pa_sync = sync;
			}
		}
	}

	state = sync_state_get_by_pa(sync);
	if (state == NULL) {
		FAIL("Could not get sync state from PA sync %p\n", sync);
		return;
	}

	if (state->recv_state != NULL) {
		int err;

		err = bt_bap_scan_delegator_set_pa_state(state->src_id, BT_BAP_PA_STATE_SYNCED);
		if (err != 0) {
			FAIL("Could not set PA sync state: %d\n", err);
			return;
		}
	}

	k_work_cancel_delayable(&state->pa_timer);

	SET_FLAG(flag_pa_synced);
}

static void pa_term_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_term_info *info)
{
	struct sync_state *state;

	printk("PA %p sync terminated\n", sync);

	state = sync_state_get_by_pa(sync);
	if (state == NULL) {
		FAIL("Could not get sync state from PA sync %p\n", sync);
		return;
	}

	if (state->recv_state != NULL) {
		int err;

		err = bt_bap_scan_delegator_set_pa_state(state->src_id, BT_BAP_PA_STATE_NOT_SYNCED);
		if (err != 0) {
			FAIL("Could not set PA sync state: %d\n", err);
			return;
		}
	}

	k_work_cancel_delayable(&state->pa_timer);

	SET_FLAG(flag_pa_terminated);
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced_cb,
	.term = pa_term_cb,
};

static bool broadcast_source_found(struct bt_data *data, void *user_data)
{
	struct bt_le_per_adv_sync_param sync_create_param = { 0 };
	const struct bt_le_scan_recv_info *info = user_data;
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_16 adv_uuid;
	struct sync_state *state;
	int err;


	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO) != 0) {
		return true;
	}

	g_broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
	bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));

	printk("Found BAP broadcast source with address %s and ID 0x%06X\n",
	       addr_str, g_broadcast_id);

	state = sync_state_get_or_new(NULL);
	if (state == NULL) {
		FAIL("Failed to get sync state");
		return true;
	}

	printk("Creating Periodic Advertising Sync\n");
	bt_addr_le_copy(&sync_create_param.addr, info->addr);
	sync_create_param.sid = info->sid;
	sync_create_param.timeout = 0xa;

	err = bt_le_per_adv_sync_create(&sync_create_param,
					&state->pa_sync);
	if (err != 0) {
		FAIL("Could not create PA sync (err %d)\n", err);
		return true;
	}

	state->pa_syncing = true;

	return false;
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info,
			 struct net_buf_simple *ad)
{
	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0 ||
	    info->interval == 0) {
		return;
	}

	bt_data_parse(ad, broadcast_source_found, (void *)info);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv_cb
};

static int add_source(struct sync_state *state)
{
	struct bt_bap_scan_delegator_add_src_param param;
	struct bt_le_per_adv_sync_info sync_info;
	int res;

	UNSET_FLAG(flag_recv_state_updated);

	res = bt_le_per_adv_sync_get_info(state->pa_sync, &sync_info);
	if (res != 0) {
		FAIL("Failed to get PA sync info: %d)\n", res);
		return true;
	}

	bt_addr_le_copy(&param.addr, &sync_info.addr);
	param.sid = sync_info.sid;
	param.pa_state = BT_BAP_PA_STATE_SYNCED;
	param.encrypt_state = BT_BAP_BIG_ENC_STATE_NO_ENC;
	param.broadcast_id = g_broadcast_id;
	param.num_subgroups = 1U;

	for (uint8_t i = 0U; i < param.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup_param = &param.subgroups[i];

		subgroup_param->bis_sync = BT_BAP_BIS_SYNC_NO_PREF;
		subgroup_param->metadata_len = 0U;
		(void)memset(subgroup_param->metadata, 0, sizeof(subgroup_param->metadata));
	}

	res = bt_bap_scan_delegator_add_src(&param);
	if (res < 0) {
		return res;
	}

	state->src_id = (uint8_t)res;

	WAIT_FOR_FLAG(flag_recv_state_updated);

	return res;
}

static void add_all_sources(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		struct sync_state *state = &sync_states[i];

		if (state->pa_sync != NULL) {
			int res;

			printk("[%zu]: Adding source\n", i);

			res = add_source(state);
			if (res < 0) {
				FAIL("[%zu]: Add source failed (err %d)\n", i, res);
				return;
			}

			printk("[%zu]: Source added with id %u\n",
			       i, state->src_id);
		}
	}
}

static int mod_source(struct sync_state *state)
{
	const uint16_t pref_context = BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL;
	struct bt_bap_scan_delegator_mod_src_param param;
	const uint8_t pref_context_metadata[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT,
				    BT_BYTES_LIST_LE16(pref_context)),
	};
	int err;

	UNSET_FLAG(flag_recv_state_updated);

	param.src_id = state->src_id;
	param.encrypt_state = BT_BAP_BIG_ENC_STATE_NO_ENC;
	param.broadcast_id = g_broadcast_id;
	param.num_subgroups = 1U;

	for (uint8_t i = 0U; i < param.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup_param = &param.subgroups[i];

		subgroup_param->bis_sync = 0U;
		subgroup_param->metadata_len = sizeof(pref_context_metadata);
		(void)memcpy(subgroup_param->metadata, pref_context_metadata,
			     subgroup_param->metadata_len);
	}

	err = bt_bap_scan_delegator_mod_src(&param);
	if (err < 0) {
		return err;
	}

	WAIT_FOR_FLAG(flag_recv_state_updated);

	return 0;
}

static void mod_all_sources(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		struct sync_state *state = &sync_states[i];

		if (state->pa_sync != NULL) {
			int err;

			printk("[%zu]: Modifying source\n", i);

			err = mod_source(state);
			if (err < 0) {
				FAIL("[%zu]: Modify source failed (err %d)\n", i, err);
				return;
			}

			printk("[%zu]: Source id modifed %u\n",
			       i, state->src_id);
		}
	}
}

static int remove_source(struct sync_state *state)
{
	int err;

	UNSET_FLAG(flag_recv_state_updated);

	/* We don't actually need to sync to the BIG/BISes */
	err = bt_bap_scan_delegator_rem_src(state->src_id);
	if (err) {
		return err;
	}

	state->recv_state = NULL;

	WAIT_FOR_FLAG(flag_recv_state_updated);

	return 0;
}

static void remove_all_sources(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		struct sync_state *state = &sync_states[i];

		if (state->recv_state != NULL) {
			int err;

			printk("[%zu]: Removing source\n", i);

			err = remove_source(state);
			if (err) {
				FAIL("[%zu]: Remove source failed (err %d)\n", err);
				return;
			}

			printk("[%zu]: Source removed with id %u\n",
			       i, state->src_id);

			printk("Terminating PA sync\n");
			err = pa_sync_term(state);
			if (err) {
				FAIL("[%zu]: PA sync term failed (err %d)\n", err);
				return;
			}
		}
	}
}

static int sync_broadcast(struct sync_state *state)
{
	int err;

	UNSET_FLAG(flag_recv_state_updated);

	if (!TEST_FLAG(flag_bis_sync_requested)) {
		/* If we have not received a sync request, set a value ourselves */
		for (size_t i = 0U; i < ARRAY_SIZE(state->bis_sync_req); i++) {
			state->bis_sync_req[i] = BIT(i);
		}
	}

	/* We don't actually need to sync to the BIG/BISes */
	err = bt_bap_scan_delegator_set_bis_sync_state(state->src_id, state->bis_sync_req);
	if (err) {
		return err;
	}

	WAIT_FOR_FLAG(flag_recv_state_updated);

	return 0;
}

static void sync_all_broadcasts(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		struct sync_state *state = &sync_states[i];

		if (state->pa_sync != NULL) {
			int res;

			printk("[%zu]: Setting broadcast sync state\n", i);

			res = sync_broadcast(state);
			if (res < 0) {
				FAIL("[%zu]: Broadcast sync state set failed (err %d)\n", i, res);
				return;
			}

			printk("[%zu]: Broadcast sync state set\n", i);
		}
	}
}

static int common_init(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	err = bt_bap_scan_delegator_register(&scan_delegator_cb);
	if (err) {
		FAIL("Scan delegator register failed (err %d)\n", err);
		return err;
	}

	bt_le_per_adv_sync_cb_register(&pa_sync_cb);

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);

	return 0;
}

static void test_main_client_sync(void)
{
	int err;

	err = common_init();
	if (err) {
		FAIL("common init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_broadcast_source_added);
	/* Wait for broadcast assistant to request us to sync to PA */
	printk("Waiting for flag_pa_synced\n");
	WAIT_FOR_FLAG(flag_pa_synced);

	/* Mod all sources by modifying the metadata */
	mod_all_sources();

	WAIT_FOR_FLAG(flag_broadcast_source_modified);
	/* Wait for broadcast assistant to tell us to BIS sync */
	printk("Waiting for flag_bis_sync_requested\n");
	WAIT_FOR_FLAG(flag_bis_sync_requested);

	/* Set the BIS sync state */
	sync_all_broadcasts();

	/* Wait for broadcast assistant to send us broadcast code */
	printk("Waiting for flag_broadcast_code_received\n");
	WAIT_FOR_FLAG(flag_broadcast_code_received);

	/* Wait for broadcast assistant to remove source and terminate PA sync */
	printk("Waiting for flag_pa_terminated\n");
	WAIT_FOR_FLAG(flag_pa_terminated);

	WAIT_FOR_FLAG(flag_broadcast_source_removed);
	PASS("BAP Scan Delegator Client Sync passed\n");
}

static void test_main_server_sync_client_rem(void)
{
	int err;

	err = common_init();
	if (err) {
		FAIL("common init failed (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scan (%d)", err);
		return;
	}

	/* Wait for PA to sync */
	printk("Waiting for flag_pa_synced\n");
	WAIT_FOR_FLAG(flag_pa_synced);

	/* Add PAs as receive state sources */
	add_all_sources();

	/* Wait for broadcast assistant to send us broadcast code */
	printk("Waiting for flag_broadcast_code_received\n");
	WAIT_FOR_FLAG(flag_broadcast_code_received);

	/* Mod all sources by modifying the metadata */
	mod_all_sources();

	/* Set the BIS sync state */
	sync_all_broadcasts();

	/* Enable rejection for the first remove source request */
	reject_control_op = true;

	WAIT_FOR_FLAG(flag_remove_source_rejected);

	/* Disable rejection for subsequent remove source requests */
	reject_control_op = false;
	/* For for client to remove source and thus terminate the PA */
	printk("Waiting for flag_pa_terminated\n");
	WAIT_FOR_FLAG(flag_pa_terminated);

	PASS("BAP Scan Delegator Server Sync Client Remove passed\n");
}

static void test_main_server_sync_server_rem(void)
{
	int err;

	err = common_init();
	if (err) {
		FAIL("common init failed (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scan (%d)", err);
		return;
	}

	/* Wait for PA to sync */
	printk("Waiting for flag_pa_synced\n");
	WAIT_FOR_FLAG(flag_pa_synced);

	/* Add PAs as receive state sources */
	add_all_sources();

	/* Wait for broadcast assistant to send us broadcast code */
	printk("Waiting for flag_broadcast_code_received\n");
	WAIT_FOR_FLAG(flag_broadcast_code_received);

	/* Mod all sources by modifying the metadata */
	mod_all_sources();

	/* Set the BIS sync state */
	sync_all_broadcasts();

	/* Remote all sources, causing PA sync term request to trigger */
	remove_all_sources();

	/* Wait for PA sync to be terminated */
	printk("Waiting for flag_pa_terminated\n");
	WAIT_FOR_FLAG(flag_pa_terminated);

	PASS("BAP Scan Delegator Server Sync Server Remove passed\n");
}

static const struct bst_test_instance test_scan_delegator[] = {
	{
		.test_id = "bap_scan_delegator_client_sync",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_client_sync,
	},
	{
		.test_id = "bap_scan_delegator_server_sync_client_rem",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_server_sync_client_rem,
	},
	{
		.test_id = "bap_scan_delegator_server_sync_server_rem",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_server_sync_server_rem,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_scan_delegator_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_scan_delegator);
}
#else
struct bst_test_list *test_scan_delegator_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR */
