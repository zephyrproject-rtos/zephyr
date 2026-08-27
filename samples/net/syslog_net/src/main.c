/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_syslog, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_net.h>
#include <zephyr/logging/log_ctrl.h>

#include <stdlib.h>

#include "net_sample_common.h"

BUILD_ASSERT(IS_ENABLED(CONFIG_LOG_BACKEND_NET), "syslog backend not enabled");

#define SLEEP_BETWEEN_PRINTS 3

int main(void)
{
	int i, count, sleep;

	LOG_DBG("Starting");

	wait_for_network();

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_NET_AUTOSTART)) {
		/* Example how to start the backend if autostart is disabled.
		 * This is useful if the application needs to wait the network
		 * to be fully up before the syslog-net is able to work.
		 */
		const struct log_backend *backend = log_backend_net_get();

		if (!log_backend_is_active(backend)) {

			/* Specifying an address by calling this function will
			 * override the value given to LOG_BACKEND_NET_SERVER.
			   It can also be called at any other time after the backend
			   is started. The net context will be released and
			   restarted with the newly specified address.
			 */
			if (strlen(CONFIG_LOG_BACKEND_NET_SERVER) == 0) {
				log_backend_net_set_addr(CONFIG_NET_SAMPLE_SERVER_RUNTIME);
			}
			log_backend_init(backend);
			log_backend_enable(backend, backend->cb->ctx, CONFIG_LOG_MAX_LEVEL);
		}
	}

	if (CONFIG_NET_SAMPLE_SEND_ITERATIONS) {
		count = CONFIG_NET_SAMPLE_SEND_ITERATIONS;
		sleep = 500;

		/* This will let the Docker process to start listening */
		k_msleep(1500);

		LOG_DBG("Sending total %d messages", 4 * count);
	} else {
		count = 60 / SLEEP_BETWEEN_PRINTS;
		sleep = SLEEP_BETWEEN_PRINTS * MSEC_PER_SEC;
	}

	/* Allow some setup time before starting to send data */
	k_msleep(sleep);

	i = count;

	do {
		LOG_ERR("Error message (%d)", i);
		LOG_WRN("Warning message (%d)", i);
		LOG_INF("Info message (%d)", i);
		LOG_DBG("Debug message (%d)", i);

		k_msleep(sleep);

	} while (--i);

	LOG_DBG("Stopped after %d msg", count);

	if (CONFIG_NET_SAMPLE_SEND_ITERATIONS) {
		/* We get here when doing Docker based testing */
		k_msleep(1000);
		exit(0);
	}
	return 0;
}
