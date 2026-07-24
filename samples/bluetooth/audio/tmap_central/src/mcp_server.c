/** @file
 *  @brief Bluetooth Media Control Profile (MCP) Server role.
 *
 *  Copyright 2023 NXP
 *  Copyright (c) 2026 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/mcp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int mcp_server_init(void)
{
	int err;

	err = bt_mcp_media_control_server_register();

	return err;
}
