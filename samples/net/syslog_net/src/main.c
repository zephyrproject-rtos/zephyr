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

	NET_DBG("Starting");

	do {
		NET_ERR("Error message");
		NET_WARN("Warning message");
		NET_INFO("Info message");
		NET_DBG("Debug message");

		k_sleep(SLEEP_BETWEEN_PRINTS);

	} while (count--);

	NET_DBG("Stopping");
}
