/**
 * @file
 * @brief Shell APIs for Bluetooth CSIP set coordinator
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "bt.h"

#include <zephyr/bluetooth/audio/csip.h>

static uint8_t members_found;
static struct k_work_delayable discover_members_timer;
static struct bt_conn *conns[CONFIG_BT_MAX_CONN];
static const struct bt_csip_set_coordinator_set_member *set_members[CONFIG_BT_MAX_CONN];
struct bt_csip_set_coordinator_csis_inst *cur_inst;
static bt_addr_le_t addr_found[CONFIG_BT_MAX_CONN];
static const struct bt_csip_set_coordinator_set_member *locked_members[CONFIG_BT_MAX_CONN];

static bool is_discovered(const bt_addr_le_t *addr)
{
	for (int i = 0; i < members_found; i++) {
		if (bt_addr_le_eq(addr, &addr_found[i])) {
			return true;
		}
	}
	return false;
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	uint8_t conn_index;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		shell_error(ctx_shell, "Failed to connect to %s (%u)",
			    addr, err);
		return;
	}

	conn_index = bt_conn_index(conn);

	shell_print(ctx_shell, "[%u]: Connected to %s", conn_index, addr);

	/* TODO: Handle RPAs */

	conns[conn_index] = bt_conn_ref(conn);
	shell_print(ctx_shell, "Member[%u] connected", conn_index);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	uint8_t conn_index = bt_conn_index(conn);

	bt_conn_unref(conns[conn_index]);
	conns[conn_index] = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb
};

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member,
			     int err, size_t set_count)
{
	uint8_t conn_index;

	if (err != 0) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	if (set_count == 0) {
		shell_warn(ctx_shell, "Device has no sets");
		return;
	}

	conn_index = bt_conn_index(conn);

	shell_print(ctx_shell, "Found %zu sets on member[%u]",
		    set_count, conn_index);

	for (size_t i = 0U; i < set_count; i++) {
		shell_print(ctx_shell, "CSIS[%zu]: %p", i, &member->insts[i]);
	}

	set_members[conn_index] = member;
}

static void csip_set_coordinator_lock_set_cb(int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set locked");
}

static void csip_set_coordinator_release_set_cb(int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set released");
}

static void csip_set_coordinator_ordered_access_cb(
	const struct bt_csip_set_coordinator_set_info *set_info, int err,
	bool locked, struct bt_csip_set_coordinator_set_member *member)
{
	if (err) {
		printk("Ordered access failed with err %d\n", err);
	} else if (locked) {
		printk("Cannot do ordered access as member %p is locked\n",
		       member);
	} else {
		printk("Ordered access procedure finished\n");
	}
}

static struct bt_csip_set_coordinator_cb cbs = {
	.lock_set = csip_set_coordinator_lock_set_cb,
	.release_set = csip_set_coordinator_release_set_cb,
	.discover = csip_discover_cb,
	.ordered_access = csip_set_coordinator_ordered_access_cb
};

static bool csip_set_coordinator_oap_cb(const struct bt_csip_set_coordinator_set_info *set_info,
					struct bt_csip_set_coordinator_set_member *members[],
					size_t count)
{
	for (size_t i = 0; i < count; i++) {
		printk("Ordered access for members[%zu]: %p\n", i, members[i]);
	}

	return true;
}

static bool csip_found(struct bt_data *data, void *user_data);
static void csip_set_coordinator_scan_recv(const struct bt_le_scan_recv_info *info,
					   struct net_buf_simple *ad)
{
	/* We're only interested in connectable events */
	if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		if (cur_inst != NULL) {
			bt_data_parse(ad, csip_found, (void *)info->addr);
		}
	}
}

static struct bt_le_scan_cb csip_set_coordinator_scan_callbacks = {
	.recv = csip_set_coordinator_scan_recv
};

static bool csip_found(struct bt_data *data, void *user_data)
{
	if (bt_csip_set_coordinator_is_set_member(cur_inst->info.set_sirk, data)) {
		bt_addr_le_t *addr = user_data;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		shell_print(ctx_shell, "Found CSIP advertiser with address %s",
			    addr_str);

		if (is_discovered(addr)) {
			shell_print(ctx_shell, "Set member already found");
			/* Stop parsing */
			return false;
		}

		bt_addr_le_copy(&addr_found[members_found++], addr);

		shell_print(ctx_shell, "Found member (%u / %u)",
			    members_found, cur_inst->info.set_size);

		if (members_found == cur_inst->info.set_size) {
			int err;

			(void)k_work_cancel_delayable(&discover_members_timer);

			bt_le_scan_cb_unregister(&csip_set_coordinator_scan_callbacks);

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

static void discover_members_timer_handler(struct k_work *work)
{
	int err;

	shell_error(ctx_shell, "Could not find all members (%u / %u)",
		    members_found, cur_inst->info.set_size);

	bt_le_scan_cb_unregister(&csip_set_coordinator_scan_callbacks);

	err = bt_le_scan_stop();
	if (err != 0) {
		shell_error(ctx_shell, "Failed to stop scan: %d", err);
	}
}

static int cmd_csip_set_coordinator_discover(const struct shell *sh,
					     size_t argc, char *argv[])
{
	char addr[BT_ADDR_LE_STR_LEN];
	static bool initialized;
	long member_index = 0;
	struct bt_conn *conn;
	int err;

	if (!initialized) {
		k_work_init_delayable(&discover_members_timer,
				      discover_members_timer_handler);
		bt_csip_set_coordinator_register_cb(&cbs);
		initialized = true;
	}

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 || member_index > ARRAY_SIZE(conns)) {
			shell_error(sh, "Invalid member_index %ld",
				    member_index);
			return -ENOEXEC;
		}
	}

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	conn = conns[member_index];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	shell_print(sh, "Discovering for member[%u] (%s)",
		    (uint8_t)member_index, addr);
	err = bt_csip_set_coordinator_discover(conn);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_discover_members(const struct shell *sh,
						     size_t argc, char *argv[])
{
	int err;

	cur_inst = (struct bt_csip_set_coordinator_csis_inst *)strtol(argv[1], NULL, 0);

	if (cur_inst == NULL) {
		shell_error(sh, "NULL set");
		return -EINVAL;
	}

	if (cur_inst->info.set_size > ARRAY_SIZE(set_members)) {
		/*
		 * TODO Handle case where set size is larger than
		 * number of possible connections
		 */
		shell_error(sh,
			    "Set size (%u) larger than max connections (%u)",
			    cur_inst->info.set_size, ARRAY_SIZE(set_members));
		return -EINVAL;
	}

	if (members_found > 1) {
		members_found = 1;
	}

	err = k_work_reschedule(&discover_members_timer,
				BT_CSIP_SET_COORDINATOR_DISCOVER_TIMER_VALUE);
	if (err < 0) { /* Can return 0, 1 and 2 for success */
		shell_error(sh,
			    "Could not schedule discover_members_timer %d",
			    err);
		return err;
	}

	bt_le_scan_cb_register(&csip_set_coordinator_scan_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		shell_error(sh, "Could not start scan: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_lock_set(const struct shell *sh,
					     size_t argc, char *argv[])
{
	int err;
	int conn_count = 0;

	if (cur_inst == NULL) {
		shell_error(sh, "No set selected");
		return -ENOEXEC;
	}

	for (size_t i = 0; i < ARRAY_SIZE(locked_members); i++) {
		if (set_members[i] != NULL) {
			locked_members[conn_count++] = set_members[i];
		}
	}

	err = bt_csip_set_coordinator_lock(locked_members, conn_count,
					   &cur_inst->info);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_release_set(const struct shell *sh,
						size_t argc, char *argv[])
{
	int err;
	int conn_count = 0;

	if (cur_inst == NULL) {
		shell_error(sh, "No set selected");
		return -ENOEXEC;
	}

	for (size_t i = 0; i < ARRAY_SIZE(locked_members); i++) {
		if (set_members[i] != NULL) {
			locked_members[conn_count++] = set_members[i];
		}
	}

	err = bt_csip_set_coordinator_release(locked_members, conn_count,
					      &cur_inst->info);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_ordered_access(const struct shell *sh,
						   size_t argc, char *argv[])
{
	int err;
	long member_count = (long)ARRAY_SIZE(set_members);
	const struct bt_csip_set_coordinator_set_member *members[ARRAY_SIZE(set_members)];

	if (argc > 1) {
		member_count = strtol(argv[1], NULL, 0);

		if (member_count < 0 || member_count > ARRAY_SIZE(members)) {
			shell_error(sh, "Invalid member count %ld",
				    member_count);
			return -ENOEXEC;
		}
	}

	for (size_t i = 0; i < (size_t)member_count; i++) {
		members[i] = set_members[i];
	}

	err = bt_csip_set_coordinator_ordered_access(members,
						     ARRAY_SIZE(members),
						     &cur_inst->info,
						     csip_set_coordinator_oap_cb);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_lock(const struct shell *sh, size_t argc,
					 char *argv[])
{
	int err;
	long member_index = 0;
	const struct bt_csip_set_coordinator_set_member *lock_member[1];

	if (cur_inst == NULL) {
		shell_error(sh, "No set selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 ||
		    member_index > ARRAY_SIZE(set_members)) {
			shell_error(sh, "Invalid member_index %ld",
				    member_index);
			return -ENOEXEC;
		}
	}

	lock_member[0] = set_members[member_index];

	err = bt_csip_set_coordinator_lock(lock_member, 1, &cur_inst->info);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator_release(const struct shell *sh, size_t argc,
					    char *argv[])
{
	int err;
	long member_index = 0;
	const struct bt_csip_set_coordinator_set_member *lock_member[1];

	if (cur_inst == NULL) {
		shell_error(sh, "No set selected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		member_index = strtol(argv[1], NULL, 0);

		if (member_index < 0 ||
		    member_index > ARRAY_SIZE(set_members)) {
			shell_error(sh, "Invalid member_index %ld",
				    member_index);
			return -ENOEXEC;
		}
	}

	lock_member[0] = set_members[member_index];

	err = bt_csip_set_coordinator_release(lock_member, 1, &cur_inst->info);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static int cmd_csip_set_coordinator(const struct shell *sh, size_t argc,
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

SHELL_STATIC_SUBCMD_SET_CREATE(csip_set_coordinator_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Run discover for CSIS on peer device [member_index]",
		      cmd_csip_set_coordinator_discover, 1, 1),
	SHELL_CMD_ARG(discover_members, NULL,
		      "Scan for set members <set_pointer>",
		      cmd_csip_set_coordinator_discover_members, 2, 0),
	SHELL_CMD_ARG(lock_set, NULL,
		      "Lock set",
		      cmd_csip_set_coordinator_lock_set, 1, 0),
	SHELL_CMD_ARG(release_set, NULL,
		      "Release set",
		      cmd_csip_set_coordinator_release_set, 1, 0),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock specific member [member_index]",
		      cmd_csip_set_coordinator_lock, 1, 1),
	SHELL_CMD_ARG(release, NULL,
		      "Release specific member [member_index]",
		      cmd_csip_set_coordinator_release, 1, 1),
	SHELL_CMD_ARG(ordered_access, NULL,
		      "Perform dummy ordered access procedure [member_count]",
		      cmd_csip_set_coordinator_ordered_access, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csip_set_coordinator, &csip_set_coordinator_cmds,
		       "Bluetooth csip_set_coordinator shell commands",
		       cmd_csip_set_coordinator, 1, 1);
