/** @file
 *  @brief Bluetooth Media Control Profile (MCP) Controller role.
 *
 *  Copyright 2023 NXP
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/toolchain.h>

static struct bt_conn *default_conn;
static K_MUTEX_DEFINE(conn_lock);

static K_SEM_DEFINE(sem_discovery_done, 0U, 1U);

/* Take a temporary reference on the current connection under conn_lock so a concurrent
 * disconnect (which drops default_conn) cannot free it while a command uses it.
 */
static struct bt_conn *conn_get(void)
{
	struct bt_conn *conn;

	k_mutex_lock(&conn_lock, K_FOREVER);
	conn = (default_conn != NULL) ? bt_conn_ref(default_conn) : NULL;
	k_mutex_unlock(&conn_lock);

	return conn;
}

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	ARG_UNUSED(conn);

	if (err != 0) {
		printk("MCP: Discovery of MCS failed (%d)\n", err);
	} else {
		printk("MCP: Discovered MCS\n");
	}
	k_sem_give(&sem_discovery_done);
}

static void mcc_send_command_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	ARG_UNUSED(conn);

	if (err != 0) {
		printk("MCP: Command send failed (%d) - opcode: %u, param: %d\n",
			err, cmd->opcode, cmd->param);
	} else {
		printk("MCP: Successfully sent command (%d) - opcode: %u, param: %d\n",
			err, cmd->opcode, cmd->param);
	}
}

static struct bt_mcc_cb mcc_cb = {
	.discover_mcs = mcc_discover_mcs_cb,
	.send_cmd = mcc_send_command_cb,
};

static void mcp_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	k_mutex_lock(&conn_lock, K_FOREVER);
	if (conn == default_conn) {
		bt_conn_drop(&default_conn);
	}
	k_mutex_unlock(&conn_lock);
}

BT_CONN_CB_DEFINE(mcp_conn_callbacks) = {
	.disconnected = mcp_disconnected,
};

int mcp_ctlr_init(struct bt_conn *conn)
{
	int err;

	k_mutex_lock(&conn_lock, K_FOREVER);
	default_conn = bt_conn_ref(conn);
	k_mutex_unlock(&conn_lock);

	err = bt_mcc_init(&mcc_cb);
	if (err != 0) {
		return err;
	}

	err = bt_mcc_discover_mcs(conn, true);
	if (err == 0) {
		err = k_sem_take(&sem_discovery_done, K_FOREVER);
		__ASSERT_NO_MSG(err == 0);
	}
	return err;
}

int mcp_send_cmd(uint8_t mcp_opcode)
{
	struct bt_conn *conn = conn_get();
	struct mpl_cmd cmd;
	int err;

	if (conn == NULL) {
		printk("MCP: No connection\n");
		return -EINVAL;
	}

	cmd.opcode = mcp_opcode;
	cmd.use_param = false;

	err = bt_mcc_send_cmd(conn, &cmd);
	if (err != 0) {
		printk("MCP: Command failed: %d\n", err);
	}

	bt_conn_unref(conn);
	return err;
}
