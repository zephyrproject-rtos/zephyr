/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_BAP_SCAN_DELEGATOR
#include <zephyr/bluetooth/audio/bap.h>
#include "common.h"

extern enum bst_result_t bst_result;

#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define PA_SYNC_SKIP              5

static volatile bool g_cb;
static volatile bool g_pa_synced;
CREATE_FLAG(flag_broadcast_code_received);

struct sync_state {
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	bool pa_syncing;
	struct k_work_delayable pa_timer;
	struct bt_le_per_adv_sync *pa_sync;
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
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

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		/* Ensure that the following calculation does not overflow silently */
		__ASSERT(SYNC_RETRY_COUNT < 10,
			 "SYNC_RETRY_COUNT shall be less than 10");

		/* Add retries and convert to unit in 10's of ms */
		pa_timeout = ((uint32_t)pa_interval * SYNC_RETRY_COUNT) / 10;

		/* Enforce restraints */
		pa_timeout = CLAMP(pa_timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
				   BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static void pa_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sync_state *state = CONTAINER_OF(dwork, struct sync_state, pa_timer);

	state->pa_syncing = false;

	if (state->recv_state != NULL) {
		enum bt_bap_pa_state pa_state;

		if (state->recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
			pa_state = BT_BAP_PA_STATE_NO_PAST;
		} else {
			pa_state = BT_BAP_PA_STATE_FAILED;
		}

		bt_bap_scan_delegator_set_pa_state(state->recv_state->src_id,
						   pa_state);
	}

	FAIL("PA timeout\n");
}

static int pa_sync_past(struct bt_conn *conn,
			struct sync_state *state,
			uint16_t pa_interval)
{
	struct bt_le_per_adv_sync_transfer_param param = { 0 };
	int err;

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
	printk("Receive state with ID %u updated\n", recv_state->src_id);
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	struct sync_state *state;
	int err;

	printk("PA Sync request: past_avail %u, pa_interval 0x%04x\n: %p",
	       past_avail, pa_interval, recv_state);

	state = sync_state_get_or_new(recv_state);
	if (state == NULL) {
		FAIL("Could not get state");
		return -1;
	}

	state->recv_state = recv_state;

	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		/* Already syncing */
		/* TODO: Terminate existing sync and then sync to new?*/
		return -1;
	}

	if (past_avail) {
		err = pa_sync_past(conn, state, pa_interval);
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
			      const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	struct sync_state *state;

	printk("Broadcast code received for %p\n", recv_state);

	state = sync_state_get(recv_state);
	if (state == NULL) {
		FAIL("Could not get state\n");
		return;
	}

	(void)memcpy(state->broadcast_code, broadcast_code, BT_AUDIO_BROADCAST_CODE_SIZE);

	SET_FLAG(flag_broadcast_code_received);
}

static struct bt_bap_scan_delegator_cb scan_delegator_cb = {
	.recv_state_updated = recv_state_updated_cb,
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.broadcast_code = broadcast_code_cb,
};

static void pa_synced_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	struct sync_state *state;

	printk("PA %p synced\n", sync);

	state = sync_state_get_by_pa(sync);
	if (state == NULL) {
		FAIL("Could not get sync state from PA sync %p\n", sync);
		return;
	}

	k_work_cancel_delayable(&state->pa_timer);

	g_pa_synced = true;
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

	k_work_cancel_delayable(&state->pa_timer);

	g_pa_synced = false;
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced_cb,
	.term = pa_term_cb,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_bap_scan_delegator_register_cb(&scan_delegator_cb);
	bt_le_per_adv_sync_cb_register(&pa_sync_cb);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_COND(g_pa_synced);

	WAIT_FOR_FLAG(flag_broadcast_code_received);

	WAIT_FOR_COND(!g_pa_synced);

	PASS("BAP Scan Delegator passed\n");
}

static const struct bst_test_instance test_scan_delegator[] = {
	{
		.test_id = "bap_scan_delegator",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
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
