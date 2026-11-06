/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ftp_client_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/conn_mgr_monitor.h>
#include "net_sample_common.h"

int main(void)
{
	LOG_INF("Starting FTP client sample");

	LOG_INF("Waiting for network...");
	(void)conn_mgr_wait_ready(CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6, K_FOREVER);
	LOG_INF("Network ready");

	LOG_INF("Network connectivity established.");
	LOG_INF("You can interact with FTP server via shell.");

	k_sleep(K_FOREVER);

	return 0;
}
