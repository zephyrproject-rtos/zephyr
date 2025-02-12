/** @file
 *  @brief Bluetooth Media Control Profile (MCP) Server role.
 *
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/media_proxy.h>

int mcp_server_init(void)
{
	int err;

	err = media_proxy_pl_init();

	return err;
}
