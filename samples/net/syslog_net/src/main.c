/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_syslog, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <logging/log_backend.h>

#include <stdlib.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_LOG_BACKEND_NET), "syslog backend not enabled");

#define SLEEP_BETWEEN_PRINTS 3

#if IS_ENABLED(CONFIG_LOG_BACKEND_NET)
extern const struct log_backend *log_backend_net_get(void);
#endif

void main(void)
{
	int i, count, sleep;

	LOG_DBG("Starting");

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_NET_AUTOSTART)) {
		/* Example how to start the backend if autostart is disabled.
		 * This is useful if the application needs to wait the network
		 * to be fully up before the syslog-net is able to work.
		 */
		const struct log_backend *backend = log_backend_net_get();

		if (!log_backend_is_active(backend)) {
			if (backend->api->init != NULL) {
				backend->api->init();
			}

			log_backend_activate(backend, NULL);
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
}
