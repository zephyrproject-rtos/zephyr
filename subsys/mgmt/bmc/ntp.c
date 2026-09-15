/*
 * Based on samples/net/sockets/sntp_client/src/main.c
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <netdb.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/socket.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define NTP_SERVER_PORT      123
#define NTP_QUERY_TIMEOUT_MS 4000

/*
 * The initial interval is short so that the clock is set as soon as the
 * network comes up, then it backs off once a sync succeeded.
 */
#define NTP_RETRY_INTERVAL_INITIAL K_SECONDS(20)
#define NTP_RETRY_INTERVAL         K_SECONDS(60)
#define NTP_RESYNC_INTERVAL        K_HOURS(6)

static K_SEM_DEFINE(ntp_wakeup, 0, 1);
static bool ntp_synced;
static bool ntp_running;

static int sntp_set_clocks(struct sntp_time *ts)
{
	struct timespec tspec;
	int ret;

	tspec.tv_sec = ts->seconds;
	tspec.tv_nsec = ((uint64_t)ts->fraction * NSEC_PER_SEC) >> 32;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tspec);
	if (ret < 0) {
		LOG_ERR("Setting the system clock failed (err=%d)", ret);
		return ret;
	}

	LOG_INF("Time synchronised to NTP");

	ret = bmc_rtc_set_from_clock();
	if (ret < 0 && ret != -ENOTSUP) {
		/* Not fatal, the system clock is what the services use. */
		LOG_WRN("Updating the RTC from the system clock failed (err=%d)", ret);
	}

	return 0;
}

static int ntp_resolve(const char *host, struct sockaddr *addr, socklen_t *addrlen)
{
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM,
	};
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(host, NULL, &hints, &res);
	if (ret != 0) {
		LOG_ERR("Could not resolve %s (err=%d)", host, ret);
		return -EHOSTUNREACH;
	}

	*addr = *res->ai_addr;
	*addrlen = res->ai_addrlen;

	freeaddrinfo(res);

	net_sin(addr)->sin_port = htons(NTP_SERVER_PORT);

	return 0;
}

static int ntp_sync_once(void)
{
	const char *server = bmc_config_ntp_server();
	struct sntp_time s_time;
	struct sntp_ctx ctx;
	struct sockaddr addr;
	socklen_t addrlen;
	int ret;

	ret = ntp_resolve(server, &addr, &addrlen);
	if (ret < 0) {
		return ret;
	}

	ret = sntp_init(&ctx, &addr, addrlen);
	if (ret < 0) {
		LOG_ERR("Could not init the SNTP context (err=%d)", ret);
		return ret;
	}

	ret = sntp_query(&ctx, NTP_QUERY_TIMEOUT_MS, &s_time);
	sntp_close(&ctx);

	if (ret < 0) {
		LOG_ERR("SNTP request to %s failed (err=%d)", server, ret);
		return ret;
	}

	return sntp_set_clocks(&s_time);
}

/*
 * SNTP resolves a name and waits for a UDP reply, both of which block for a
 * long time, so this runs on its own thread rather than on the system
 * workqueue.
 */
static void ntp_thread(void *p1, void *p2, void *p3)
{
	k_timeout_t interval = NTP_RETRY_INTERVAL_INITIAL;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		(void)k_sem_take(&ntp_wakeup, interval);

		if (!ntp_running) {
			interval = K_FOREVER;
			continue;
		}

		if (ntp_sync_once() == 0) {
			ntp_synced = true;
			interval = NTP_RESYNC_INTERVAL;
		} else {
			interval = ntp_synced ? NTP_RESYNC_INTERVAL : NTP_RETRY_INTERVAL;
		}
	}
}

K_THREAD_DEFINE(bmc_ntp_thread_id, CONFIG_BMC_NTP_THREAD_STACK_SIZE, ntp_thread, NULL, NULL, NULL,
		CONFIG_BMC_NTP_THREAD_PRIORITY, 0, -1);

bool bmc_ntp_is_synced(void)
{
	return ntp_synced;
}

int bmc_ntp_start(void)
{
	ntp_running = true;
	k_sem_give(&ntp_wakeup);

	return 0;
}

int bmc_ntp_stop(void)
{
	ntp_running = false;

	return 0;
}

static int bmc_ntp_init(void)
{
	k_thread_name_set(bmc_ntp_thread_id, "bmc_ntp");
	k_thread_start(bmc_ntp_thread_id);

	if (bmc_config_use_ntp()) {
		return bmc_ntp_start();
	}

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_ntp, BMC_INIT_PHASE_SERVICE, bmc_ntp_init, true);
