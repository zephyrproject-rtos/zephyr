/** @file
 *  @brief Bluetooth Media Control Profile (MCP) Controller role.
 *
 *  Copyright 2023 NXP
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static struct bt_conn *default_conn;

static K_SEM_DEFINE(sem_discovery_done, 0, 1);

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		printk("MCP: Discovery of MCS failed (%d)\n", err);
	} else {
		printk("MCP: Discovered MCS\n");
	}
	k_sem_give(&sem_discovery_done);
}

static void mcc_send_command_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	if (err) {
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

int mcp_ctlr_init(struct bt_conn *conn)
{
	int err;

	default_conn = bt_conn_ref(conn);

	err = bt_mcc_init(&mcc_cb);
	if (err != 0) {
		return err;
	}

	err = bt_mcc_discover_mcs(default_conn, true);
	if (err == 0) {
		k_sem_take(&sem_discovery_done, K_FOREVER);
	}
	return err;
}

int mcp_send_cmd(uint8_t mcp_opcode)
{
	int err;
	struct mpl_cmd cmd;

	cmd.opcode = mcp_opcode;
	cmd.use_param = false;

	if (default_conn == NULL) {
		printk("MCP: No connection\n");
		return -EINVAL;
	}

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		printk("MCP: Command failed: %d\n", err);
	}

	return err;
}
