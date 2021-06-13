/**
 * @file
 * @brief Shell APIs for Bluetooth CSIP
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#include "bt.h"

#include "../host/audio/csip.h"

static uint8_t members_found;
static struct k_work_delayable discover_members_timer;
static struct bt_csip_set_member set_members[CONFIG_BT_MAX_CONN];
struct bt_csip_set_t *cur_set;
const struct bt_csip_set_member *locked_members[CONFIG_BT_MAX_CONN];

static bool is_discovered(const bt_addr_le_t *addr)
{
	for (int i = 0; i < members_found; i++) {
		if (!bt_addr_le_cmp(addr, &set_members[i].addr)) {
			return true;
		}
	}
	return false;
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		shell_error(ctx_shell, "Failed to connect to %s (%u)",
			    addr, err);
		return;
	}

	shell_print(ctx_shell, "[%u]: Connected to %s",
		    bt_conn_index(conn), addr);

	/* TODO: Handle RPAs */

	if (members_found == 0) {
		shell_print(ctx_shell, "Assuming member[0] connected");
		set_members[0].conn = conn;
		bt_addr_le_copy(&set_members[0].addr, bt_conn_get_dst(conn));
		members_found = 1;
		return;
	}

	for (int i = 0; i < members_found; i++) {
		if (bt_addr_le_cmp(bt_conn_get_dst(conn),
				   &set_members[i].addr) == 0) {
			set_members[i].conn = conn;
			shell_print(ctx_shell, "Member[%u] connected", i);
			return;
		}
	}
	shell_warn(ctx_shell, "[%u] connected but was not member of set",
		   bt_conn_index(conn));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
};

static void csis_discover_cb(struct bt_conn *conn, int err, uint8_t set_count)
{
	if (err) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	if (set_count == 0) {
		shell_warn(ctx_shell, "Device has no sets");
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(set_members); i++) {
		if (set_members[i].conn == conn) {
			shell_print(ctx_shell, "Found %u sets on member[%u]",
				set_count, i);
		}
	}
}

static void csip_discover_sets_cb(struct bt_conn *conn, int err,
				  uint8_t set_count, struct bt_csip_set_t *sets)
{
	if (err) {
		shell_error(ctx_shell, "Discover sets failed (%d)", err);
		return;
	}

	for (int i = 0; i < set_count; i++) {
		shell_print(ctx_shell, "Set size %d (pointer: %p)",
			    sets[i].set_size, &sets[i]);
	}

	for (int i = 0; i < ARRAY_SIZE(set_members); i++) {
		struct bt_csip_set_member *member;

		member = &set_members[i];

		if (member->conn == conn) {
			for (int j = 0; j < set_count; j++) {
				memcpy(&member->sets[j], &sets[j],
				       sizeof(sets[j]));
			}
		}
	}
}

static void csip_lock_set_cb(int err)
{
	if (err) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set locked");
}

static void csip_release_set_cb(int err)
{
	if (err) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set released");
}

static void csip_lock_cb(struct bt_conn *conn, int err, uint8_t inst_idx)
{
	if (err) {
		shell_error(ctx_shell, "Device (index 0x%02x) lock failed (%d)",
			    inst_idx, err);
	}

	shell_print(ctx_shell, "Device (index 0x%02x) locked", inst_idx);
}

static void csip_release_cb(struct bt_conn *conn, int err, uint8_t inst_idx)
{
	if (err) {
		shell_error(ctx_shell, "Device (index 0x%02x) release "
			    "failed (%d)", inst_idx, err);
	}

	shell_print(ctx_shell, "Device (index 0x%02x) released", inst_idx);
}

static void csip_lock_get_cb(struct bt_conn *conn, int err, uint8_t inst_idx,
			     bool locked)
{
	if (err) {
		shell_error(ctx_shell, "Device (index 0x%02x) lock get "
			    "failed (%d)", inst_idx, err);
	}

	shell_print(ctx_shell, "Device (index 0x%02x) lock value %d",
		    inst_idx, locked);
}

static struct bt_csip_cb_t cbs = {
	.lock_set = csip_lock_set_cb,
	.release_set = csip_release_set_cb,
	.sets = csip_discover_sets_cb,
	.discover = csis_discover_cb,
	.lock = csip_lock_cb,
	.release = csip_release_cb,
	.lock_read = csip_lock_get_cb
};

static bool csis_found(struct bt_data *data, void *user_data)
{
	if (bt_csip_is_set_member(cur_set->set_sirk.value, data)) {
		bt_addr_le_t *addr = user_data;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		shell_print(ctx_shell, "Found CSIS advertiser with address %s",
			    addr_str);

		if (is_discovered(addr)) {
			shell_print(ctx_shell, "Set member already found");
			/* Stop parsing */
			return false;
		}

		bt_addr_le_copy(&set_members[members_found++].addr, addr);

		shell_print(ctx_shell, "Found member (%u / %u)",
			    members_found, cur_set->set_size);

		if (members_found == cur_set->set_size) {
			int err;

			(void)k_work_cancel_delayable(&discover_members_timer);
			err = bt_le_scan_stop();

			if (err != 0) {
				shell_error(ctx_shell,
					    "Failed to stop scan: %d",
					    err);
			}
		}

		/* Stop parsing */
		return false;
	}
	/* Continue parsing */
	return true;
}

static void csip_scan_recv(const struct bt_le_scan_recv_info *info,
			   struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		if (cur_set != NULL) {
			bt_data_parse(ad, csis_found, (void *)info->addr);
		}
	}
}

static struct bt_le_scan_cb csip_scan_callbacks = {
	.recv = csip_scan_recv
};

static void discover_members_timer_handler(struct k_work *work)
{
	shell_error(ctx_shell, "Could not find all members (%u / %u)",
		    members_found, cur_set->set_size);
	bt_le_scan_stop();
}

static int cmd_csip_init(const struct shell *shell, size_t argc, char *argv[])
{
	static bool initialized;

	if (!initialized) {
		k_work_init_delayable(&discover_members_timer,
				      discover_members_timer_handler);
		bt_le_scan_cb_register(&csip_scan_callbacks);
		bt_csip_register_cb(&cbs);
		bt_conn_cb_register(&conn_callbacks);

		initialized = true;
		return 0;
	} else {
		return -EALREADY;
	}
}

static int cmd_csip_discover(const struct shell *shell, size_t argc,
				    char *argv[])
{
	int result;
	long member_index = 0;

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > CONFIG_BT_MAX_CONN) {
			shell_error(shell, "Invalid member_index %u",
				    member_index);
			return -ENOEXEC;
		}
	}

	if (!ctx_shell) {
		ctx_shell = shell;
	}

	shell_print(shell, "Discovering for member[%u]", member_index);
	result = bt_csip_discover(set_members[member_index].conn, true);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_csip_discover_sets(const struct shell *shell, size_t argc,
					 char *argv[])
{
	int result;
	long member_index = 0;

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > CONFIG_BT_MAX_CONN) {
			shell_error(shell, "Invalid member_index %u",
				    member_index);
			return -ENOEXEC;
		}
	}

	result = bt_csip_discover_sets(set_members[member_index].conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_discover_members(const struct shell *shell,
					    size_t argc, char *argv[])
{
	int result;

	cur_set = (struct bt_csip_set_t *)strtol(argv[1], NULL, 0);

	if (cur_set == NULL) {
		shell_error(shell, "NULL set");
		return -EINVAL;
	}

	if (cur_set->set_size > CONFIG_BT_MAX_CONN) {
		/*
		 * TODO Handle case where set size is larger than
		 * number of possible connections
		 */
		shell_error(shell,
			    "Set size (%u) larger than max connections (%u)",
			    cur_set->set_size, CONFIG_BT_MAX_CONN);
		return -EINVAL;
	}

	if (members_found > 1) {
		members_found = 1;
	}

	result = k_work_reschedule(&discover_members_timer,
				   CSIP_DISCOVER_TIMER_VALUE);
	if (result < 0) { /* Can return 0, 1 and 2 for success */
		shell_error(shell,
			    "Could not schedule discover_members_timer %d",
			    result);
		return result;
	}

	result = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (result != 0) {
		shell_error(shell, "Could not start scan: %d", result);
	}

	return result;
}

static int cmd_csip_lock_set(const struct shell *shell, size_t argc,
				    char *argv[])
{
	int result;
	int conn_count = 0;

	if (cur_set == NULL) {
		shell_error(shell, "No set selected");
		return -ENOEXEC;
	}

	for (int i = 0; i < ARRAY_SIZE(locked_members); i++) {
		if (set_members[i].conn != NULL) {
			locked_members[conn_count++] = &set_members[i];
		}
	}

	result = bt_csip_lock(locked_members, conn_count, cur_set);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_release_set(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int result;
	int conn_count = 0;

	if (cur_set == NULL) {
		shell_error(shell, "No set selected");
		return -ENOEXEC;
	}

	for (int i = 0; i < ARRAY_SIZE(locked_members); i++) {
		if (set_members[i].conn != NULL) {
			locked_members[conn_count++] = &set_members[i];
		}
	}

	result = bt_csip_release(locked_members, conn_count, cur_set);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_lock_get(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;
	long idx = 0;
	long member_index = 0;

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > CONFIG_BT_MAX_CONN) {
			shell_error(shell, "Invalid member_index %u",
				    member_index);
			return -ENOEXEC;
		}
	}

	if (argc > 2) {
		idx = strtol(argv[2], NULL, 0);

		if (idx < 0 || idx > CONFIG_BT_CSIP_MAX_CSIS_INSTANCES) {
			shell_error(shell, "Invalid index %u", idx);
			return -ENOEXEC;
		}
	}

	result = bt_csip_lock_get(set_members[member_index].conn, (uint8_t)idx);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_csip_lock(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	long member_index = 0;
	const struct bt_csip_set_member *lock_member[1];

	if (cur_set == NULL) {
		shell_error(shell, "No set selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > CONFIG_BT_MAX_CONN) {
			shell_error(shell, "Invalid member_index %u",
				    member_index);
			return -ENOEXEC;
		}
	}

	lock_member[0] = &set_members[member_index];

	result = bt_csip_lock(lock_member, 1, cur_set);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_csip_release(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;
	long member_index = 0;
	const struct bt_csip_set_member *lock_member[1];

	if (cur_set == NULL) {
		shell_error(shell, "No set selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > CONFIG_BT_MAX_CONN) {
			shell_error(shell, "Invalid member_index %u",
				    member_index);
			return -ENOEXEC;
		}
	}

	lock_member[0] = &set_members[member_index];

	result = bt_csip_release(lock_member, 1, cur_set);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_csip(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(csip_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize CSIP",
		      cmd_csip_init, 1, 1),
	SHELL_CMD_ARG(discover, NULL,
		      "Run discover for CSIS on peer device [member_index]",
		      cmd_csip_discover, 1, 1),
	SHELL_CMD_ARG(discover_sets, NULL,
		      "Read all set values on connected device [member_index]",
		      cmd_csip_discover_sets, 1, 1),
	SHELL_CMD_ARG(discover_members, NULL,
		      "Scan for set members <set_pointer>",
		      cmd_csip_discover_members, 2, 0),
	SHELL_CMD_ARG(lock_set, NULL,
		      "Lock set",
		      cmd_csip_lock_set, 1, 0),
	SHELL_CMD_ARG(release_set, NULL,
		      "Release set",
		      cmd_csip_release_set, 1, 0),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock specific member [member_index]",
		      cmd_csip_lock, 1, 1),
	SHELL_CMD_ARG(release, NULL,
		      "Release specific member [member_index]",
		      cmd_csip_release, 1, 1),
	SHELL_CMD_ARG(lock_get, NULL,
		      "Get the lock value of the specific member and instance "
		      "[member_index [inst_idx]]",
		      cmd_csip_lock_get, 1, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csip, &csip_cmds, "Bluetooth CSIP shell commands",
		       cmd_csip, 1, 1);
