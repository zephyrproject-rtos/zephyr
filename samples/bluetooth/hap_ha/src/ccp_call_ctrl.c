/** @file
 *  @brief Bluetooth Call Control Profile (CCP) Call Controller role.
 *
 *  Copyright (c) 2020 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/tbs.h>

enum {
	CCP_FLAG_PROFILE_CONNECTED,
	CCP_FLAG_GTBS_DISCOVER,
	CCP_FLAG_CCID_READ,
	CCP_FLAG_STATUS_FLAGS_READ,
	CCP_FLAG_CALL_STATE_READ,

	CCP_FLAG_NUM,
};

static ATOMIC_DEFINE(flags, CCP_FLAG_NUM)[CONFIG_BT_MAX_CONN];

static int process_profile_connection(struct bt_conn *conn)
{
	atomic_t *flags_for_conn = flags[bt_conn_index(conn)];
	int err = 0;

	if (!atomic_test_and_set_bit(flags_for_conn, CCP_FLAG_GTBS_DISCOVER)) {
		err = bt_tbs_client_discover(conn);
		if (err != 0) {
			printk("bt_tbs_client_discover (err %d)\n", err);
		}
	} else if (!atomic_test_and_set_bit(flags_for_conn, CCP_FLAG_CCID_READ)) {
		err = bt_tbs_client_read_ccid(conn, BT_TBS_GTBS_INDEX);
		if (err != 0) {
			printk("bt_tbs_client_read_ccid (err %d)\n", err);
		}
	} else if (!atomic_test_and_set_bit(flags_for_conn, CCP_FLAG_STATUS_FLAGS_READ)) {
		err = bt_tbs_client_read_status_flags(conn, BT_TBS_GTBS_INDEX);
		if (err != 0) {
			printk("bt_tbs_client_read_status_flags (err %d)\n", err);
		}
	} else if (!atomic_test_and_set_bit(flags_for_conn, CCP_FLAG_CALL_STATE_READ)) {
		err = bt_tbs_client_read_call_state(conn, BT_TBS_GTBS_INDEX);
		if (err != 0) {
			printk("bt_tbs_client_read_call_state (err %d)\n", err);
		}
	} else if (!atomic_test_and_set_bit(flags_for_conn, CCP_FLAG_PROFILE_CONNECTED)) {
		printk("CCP Profile connected\n");
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %d)\n", err);
		return;
	}

	atomic_set(flags[bt_conn_index(conn)], 0);

	if (process_profile_connection(conn) != 0) {
		printk("Profile connection failed");
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
};

static void discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count, bool gtbs_found)
{
	if (!gtbs_found) {
		printk("Failed to discover GTBS\n");
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	if (!atomic_test_bit(flags[bt_conn_index(conn)], CCP_FLAG_PROFILE_CONNECTED)) {
		process_profile_connection(conn);
	}
}

static void ccid_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint32_t value)
{
	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	if (!atomic_test_bit(flags[bt_conn_index(conn)], CCP_FLAG_PROFILE_CONNECTED)) {
		process_profile_connection(conn);
	}
}

static void status_flags_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint32_t value)
{
	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	if (!atomic_test_bit(flags[bt_conn_index(conn)], CCP_FLAG_PROFILE_CONNECTED)) {
		process_profile_connection(conn);
	}
}

static void call_state_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_count,
			  const struct bt_tbs_client_call_state *call_states)
{
	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err) {
		printk("%s (err %d)\n", __func__, err);
		return;
	}

	if (!atomic_test_bit(flags[bt_conn_index(conn)], CCP_FLAG_PROFILE_CONNECTED)) {
		process_profile_connection(conn);
	}
}

struct bt_tbs_client_cb tbs_client_cb = {
	.discover = discover_cb,
	.ccid = ccid_cb,
	.status_flags = status_flags_cb,
	.call_state = call_state_cb,
};

int ccp_call_ctrl_init(void)
{
	bt_tbs_client_register_cb(&tbs_client_cb);

	return 0;
}
