/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ftp_client_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include "net_sample_common.h"

int main(void)
{
	LOG_INF("Starting FTP client sample");

	wait_for_network();

	LOG_INF("Network connectivity established.");
	LOG_INF("You can interact with FTP server via shell.");

	k_sleep(K_FOREVER);

	return 0;
}
