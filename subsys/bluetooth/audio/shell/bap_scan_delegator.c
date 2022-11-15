/**
 * @file
 * @brief Shell APIs for Bluetooth BAP scan delegator
 *
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/bap.h>
#include "shell/bt.h"

#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define PA_SYNC_SKIP              5

struct sync_state {
	bool pa_syncing;
	bool past_avail;
	uint8_t src_id;
	uint16_t pa_interval;
	struct k_work_delayable pa_timer;
	struct bt_conn *conn;
	struct bt_le_per_adv_sync *pa_sync;
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
} sync_states[CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT];

static bool past_preference;

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

static struct sync_state *sync_state_new(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sync_states); i++) {
		if (sync_states[i].recv_state == NULL) {
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

		shell_info(ctx_shell, "PA timeout for %p", state->recv_state);
	}
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
		shell_info(ctx_shell, "Could not do PAST subscribe: %d", err);
	} else {
		shell_info(ctx_shell, "Syncing with PAST: %d", err);
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
		shell_info(ctx_shell, "Could not sync per adv: %d", err);
	} else {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&recv_state->addr, addr_str, sizeof(addr_str));
		shell_info(ctx_shell, "PA sync pending for addr %s", addr_str);
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

	shell_info(ctx_shell, "Deleting PA sync");

	err = bt_le_per_adv_sync_delete(state->pa_sync);
	if (err != 0) {
		shell_error(ctx_shell, "Could not delete per adv sync: %d",
			    err);
	} else {
		state->pa_syncing = false;
		state->pa_sync = NULL;
	}

	return err;
}

static void recv_state_updated_cb(struct bt_conn *conn,
				  const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	shell_info(ctx_shell, "Receive state with ID %u updated", recv_state->src_id);
}

static int pa_sync_req_cb(struct bt_conn *conn,
			  const struct bt_bap_scan_delegator_recv_state *recv_state,
			  bool past_avail, uint16_t pa_interval)
{
	struct sync_state *state;

	shell_info(ctx_shell,
		   "PA Sync request: past_avail %u, broadcast_id 0x%06X, pa_interval 0x%04x: %p",
		   past_avail, recv_state->broadcast_id, pa_interval,
		   recv_state);

	state = sync_state_get_or_new(recv_state);
	if (state == NULL) {
		shell_error(ctx_shell, "Could not get state");

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
		state->past_avail = past_preference;
		state->conn = bt_conn_ref(conn);
	} else {
		state->past_avail = false;
	}

	return 0;
}

static int pa_sync_term_req_cb(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct sync_state *state;

	shell_info(ctx_shell, "PA Sync term request for %p", recv_state);

	state = sync_state_get(recv_state);
	if (state == NULL) {
		shell_error(ctx_shell, "Could not get state");

		return -1;
	}

	return pa_sync_term(state);
}

static void broadcast_code_cb(struct bt_conn *conn,
			      const struct bt_bap_scan_delegator_recv_state *recv_state,
			      const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	struct sync_state *state;

	shell_info(ctx_shell, "Broadcast code received for %p", recv_state);

	state = sync_state_get(recv_state);
	if (state == NULL) {
		shell_error(ctx_shell, "Could not get state");

		return;
	}

	(void)memcpy(state->broadcast_code, broadcast_code, BT_AUDIO_BROADCAST_CODE_SIZE);
}

static int bis_sync_req_cb(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   const uint32_t bis_sync_req[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS])
{
	printk("BIS sync request received for %p\n", recv_state);

	for (int i = 0; i < BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS; i++) {
		printk("  [%d]: 0x%08x\n", i, bis_sync_req[i]);
	}

	return 0;
}

static struct bt_bap_scan_delegator_cb scan_delegator_cb = {
	.recv_state_updated = recv_state_updated_cb,
	.pa_sync_req = pa_sync_req_cb,
	.pa_sync_term_req = pa_sync_term_req_cb,
	.broadcast_code = broadcast_code_cb,
	.bis_sync_req = bis_sync_req_cb,
};

static void pa_synced_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	struct sync_state *state;

	shell_info(ctx_shell, "PA %p synced", sync);

	state = sync_state_get_by_pa(sync);
	if (state == NULL) {
		shell_info(ctx_shell,
			   "Could not get sync state from PA sync %p",
			   sync);
		return;
	}

	if (state->conn != NULL) {
		bt_conn_unref(state->conn);
		state->conn = NULL;
	}

	k_work_cancel_delayable(&state->pa_timer);
}

static void pa_term_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_term_info *info)
{
	struct sync_state *state;

	shell_info(ctx_shell, "PA %p sync terminated", sync);

	state = sync_state_get_by_pa(sync);
	if (state == NULL) {
		shell_error(ctx_shell,
			    "Could not get sync state from PA sync %p",
			    sync);
		return;
	}

	if (state->conn != NULL) {
		bt_conn_unref(state->conn);
		state->conn = NULL;
	}

	k_work_cancel_delayable(&state->pa_timer);
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced_cb,
	.term = pa_term_cb,
};

static int cmd_bap_scan_delegator_init(const struct shell *sh, size_t argc,
				       char **argv)
{
	bt_le_per_adv_sync_cb_register(&pa_sync_cb);
	bt_bap_scan_delegator_register_cb(&scan_delegator_cb);
	return 0;
}
static int cmd_bap_scan_delegator_set_past_pref(const struct shell *sh,
						size_t argc, char **argv)
{
	bool past_pref;
	int err;

	err = 0;

	past_pref = shell_strtobool(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse past_pref from %s", argv[1]);
		return -ENOEXEC;
	}

	past_preference = past_pref;

	return 0;
}

static int cmd_bap_scan_delegator_sync_pa(const struct shell *sh, size_t argc,
					  char **argv)
{
	struct sync_state *state;
	unsigned long src_id;
	int err;

	err = 0;

	src_id = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse src_id from %s", argv[1]);

		return -ENOEXEC;
	}

	if (src_id > UINT8_MAX) {
		shell_error(sh, "src_id shall be 0x00-0xff");

		return -ENOEXEC;
	}

	state = sync_state_get_by_src_id((uint8_t)src_id);
	if (state == NULL) {
		shell_error(ctx_shell, "Could not get state");

		return -ENOEXEC;
	}

	if (past_preference &&
	    state->past_avail &&
	    state->conn != NULL) {
		shell_info(sh, "Syncing with PAST");

		err = pa_sync_past(state->conn, state, state->pa_interval);
		if (err != 0) {
			err = bt_bap_scan_delegator_set_pa_state(src_id,
								 BT_BAP_PA_STATE_INFO_REQ);
			if (err != 0) {
				shell_error(sh,
					    "Failed to set INFO_REQ state: %d",
					    err);
			}

			return -ENOEXEC;
		}

	} else {
		shell_info(sh, "Syncing without PAST");
		err = pa_sync_no_past(state, state->pa_interval);
	}

	if (err != 0) {
		shell_error(sh, "Failed PA sync: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_bap_scan_delegator_add_src(const struct shell *sh, size_t argc,
					  char **argv)
{
	/* TODO: Add support to select which PA sync to BIG sync to */
	struct bt_le_per_adv_sync *pa_sync = per_adv_syncs[0];
	struct bt_bap_scan_delegator_subgroup *subgroup_param;
	struct bt_bap_scan_delegator_add_src_param param;
	unsigned long broadcast_id;
	struct sync_state *state;
	unsigned long enc_state;
	int err;

	err = 0;

	broadcast_id = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse broadcast_id from %s", argv[1]);

		return -EINVAL;
	}

	if (broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		shell_error(sh, "Invalid broadcast_id %lu", broadcast_id);

		return -EINVAL;
	}

	enc_state = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse enc_state from %s", argv[2]);

		return -EINVAL;
	}

	if (enc_state > BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		shell_error(sh, "Invalid enc_state %lu", enc_state);

		return -EINVAL;
	}

	/* TODO: Support multiple subgroups */
	subgroup_param = &param.subgroups[0];
	if (argc > 3) {
		unsigned long bis_sync;

		bis_sync = shell_strtoul(argv[3], 16, &err);
		if (err != 0) {
			shell_error(sh, "Failed to parse bis_sync from %s", argv[3]);

			return -EINVAL;
		}

		if (bis_sync > BT_BAP_BIS_SYNC_NO_PREF) {
			shell_error(sh, "Invalid bis_sync %lu", bis_sync);

			return -EINVAL;
		}
	} else {
		subgroup_param->bis_sync = 0U;
	}

	if (argc > 4) {
		subgroup_param->metadata_len = hex2bin(argv[4], strlen(argv[4]),
						       subgroup_param->metadata,
						       sizeof(subgroup_param->metadata));

		if (subgroup_param->metadata_len == 0U) {
			shell_error(sh, "Could not parse metadata");

			return -EINVAL;
		}
	} else {
		subgroup_param->metadata_len = 0U;
	}

	state = sync_state_new();
	if (state == NULL) {
		shell_error(ctx_shell, "Could not get new state");

		return -ENOEXEC;
	}

	param.pa_sync = pa_sync;
	param.encrypt_state = (enum bt_bap_big_enc_state)enc_state;
	param.broadcast_id = broadcast_id;
	param.num_subgroups = 1U;

	err = bt_bap_scan_delegator_add_src(&param);
	if (err < 0) {
		shell_error(ctx_shell, "Failed to add source: %d", err);

		return -ENOEXEC;
	}

	state->src_id = (uint8_t)err;

	return 0;
}

static int cmd_bap_scan_delegator_mod_src(const struct shell *sh, size_t argc,
					  char **argv)
{
	struct bt_bap_scan_delegator_subgroup *subgroup_param;
	struct bt_bap_scan_delegator_mod_src_param param;
	unsigned long broadcast_id;
	unsigned long enc_state;
	unsigned long src_id;
	int err;

	err = 0;

	src_id = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse src_id from %s", argv[1]);

		return -EINVAL;
	}

	if (src_id > UINT8_MAX) {
		shell_error(sh, "Invalid src_id %lu", src_id);

		return -EINVAL;
	}

	broadcast_id = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse broadcast_id from %s", argv[2]);

		return -EINVAL;
	}

	if (broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		shell_error(sh, "Invalid broadcast_id %lu", broadcast_id);

		return -EINVAL;
	}

	enc_state = shell_strtoul(argv[3], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse enc_state from %s", argv[3]);

		return -EINVAL;
	}

	if (enc_state > BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		shell_error(sh, "Invalid enc_state %lu", enc_state);

		return -EINVAL;
	}

	/* TODO: Support multiple subgroups */
	subgroup_param = &param.subgroups[0];
	if (argc > 4) {
		unsigned long bis_sync;

		bis_sync = shell_strtoul(argv[4], 16, &err);
		if (err != 0) {
			shell_error(sh, "Failed to parse bis_sync from %s", argv[4]);

			return -EINVAL;
		}

		if (bis_sync > BT_BAP_BIS_SYNC_NO_PREF) {
			shell_error(sh, "Invalid bis_sync %lu", bis_sync);

			return -EINVAL;
		}
	} else {
		subgroup_param->bis_sync = 0U;
	}

	if (argc > 5) {
		subgroup_param->metadata_len = hex2bin(argv[5], strlen(argv[5]),
						       subgroup_param->metadata,
						       sizeof(subgroup_param->metadata));

		if (subgroup_param->metadata_len == 0U) {
			shell_error(sh, "Could not parse metadata");

			return -EINVAL;
		}
	} else {
		subgroup_param->metadata_len = 0U;
	}


	param.src_id = (uint8_t)src_id;
	param.encrypt_state = (enum bt_bap_big_enc_state)enc_state;
	param.broadcast_id = broadcast_id;
	param.num_subgroups = 1U;

	err = bt_bap_scan_delegator_mod_src(&param);
	if (err < 0) {
		shell_error(ctx_shell, "Failed to modify source: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_bap_scan_delegator_rem_src(const struct shell *sh, size_t argc,
					  char **argv)
{
	unsigned long src_id;
	int err;

	err = 0;

	src_id = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse src_id from %s", argv[1]);

		return -EINVAL;
	}

	if (src_id > UINT8_MAX) {
		shell_error(sh, "Invalid src_id %lu", src_id);

		return -EINVAL;
	}

	err = bt_bap_scan_delegator_rem_src((uint8_t)src_id);
	if (err < 0) {
		shell_error(ctx_shell, "Failed to remove source source: %d",
			    err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_bap_scan_delegator_bis_synced(const struct shell *sh, size_t argc,
					 char **argv)
{
	uint32_t bis_syncs[CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
	unsigned long pa_sync_state;
	unsigned long bis_synced;
	unsigned long src_id;
	int result = 0;

	src_id = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse src_id: %d", result);

		return -ENOEXEC;
	}

	if (src_id > UINT8_MAX) {
		shell_error(sh, "Invalid src_id: %lu", src_id);

		return -ENOEXEC;
	}

	pa_sync_state = shell_strtoul(argv[2], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse pa_sync_state: %d", result);

		return -ENOEXEC;
	}

	if (pa_sync_state > BT_BAP_PA_STATE_NO_PAST) {
		shell_error(sh, "Invalid pa_sync_state %ld", pa_sync_state);

		return -ENOEXEC;
	}

	bis_synced = shell_strtoul(argv[3], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse bis_synced: %d", result);

		return -ENOEXEC;
	}

	if (bis_synced > UINT32_MAX) {
		shell_error(sh, "Invalid bis_synced %ld", bis_synced);

		return -ENOEXEC;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(bis_syncs); i++) {
		bis_syncs[i] = bis_synced;
	}

	result = bt_bap_scan_delegator_set_bis_sync_state(src_id, bis_syncs);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_bap_scan_delegator(const struct shell *sh, size_t argc,
				  char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bap_scan_delegator_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_bap_scan_delegator_init, 1, 0),
	SHELL_CMD_ARG(set_past_pref, NULL,
		      "Set PAST preference <true || false>",
		      cmd_bap_scan_delegator_set_past_pref, 2, 0),
	SHELL_CMD_ARG(sync_pa, NULL,
		      "Sync to PA <src_id>",
		      cmd_bap_scan_delegator_sync_pa, 2, 0),
	SHELL_CMD_ARG(add_src, NULL,
		      "Add a PA as source <broadcast_id> <enc_state> [bis_sync [metadata]]",
		      cmd_bap_scan_delegator_add_src, 3, 2),
	SHELL_CMD_ARG(mod_src, NULL,
		      "Modify source <src_id> <broadcast_id> <enc_state> [bis_sync [metadata]]",
		      cmd_bap_scan_delegator_mod_src, 4, 2),
	SHELL_CMD_ARG(rem_src, NULL,
		      "Remove source <src_id>",
		      cmd_bap_scan_delegator_rem_src, 2, 0),
	SHELL_CMD_ARG(synced, NULL,
		      "Set server scan state <src_id> <bis_syncs>",
		      cmd_bap_scan_delegator_bis_synced, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(bap_scan_delegator, &bap_scan_delegator_cmds,
		       "Bluetooth BAP scan delegator shell commands",
		       cmd_bap_scan_delegator, 1, 1);
