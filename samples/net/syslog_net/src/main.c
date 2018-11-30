/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_syslog, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <net/net_core.h>
#include <net/net_pkt.h>

#define SLEEP_BETWEEN_PRINTS K_SECONDS(3)

void main(void)
{
	int count = K_SECONDS(60) / SLEEP_BETWEEN_PRINTS;

	/* Allow some setup time before starting to send data */
	k_sleep(SLEEP_BETWEEN_PRINTS);

	LOG_DBG("Starting");

	do {
		LOG_ERR("Error message");
		LOG_WRN("Warning message");
		LOG_INF("Info message");
		LOG_DBG("Debug message");

		k_sleep(SLEEP_BETWEEN_PRINTS);

	} while (count--);

	LOG_DBG("Stopping");
}
