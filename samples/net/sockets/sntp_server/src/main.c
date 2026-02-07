/*
 * Copyright (c) 2025 Lothar Felten <lothar.felten@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sntp_demo, LOG_LEVEL_DBG);

#include <zephyr/net/sntp.h>
#include <zephyr/net/sntp_server.h>
#include "net_sample_common.h"

#define FRAC2NS(x) (uint32_t)((((uint64_t)x) * 1000000000ULL) >> 32);

int main(void)
{
	struct sntp_time t;
	struct timespec tp;
	int ret;

	LOG_INF("waiting for network");
	wait_for_network();

	ret = sntp_simple(CONFIG_NET_SAMPLE_SNTP_CLIENT_SERVER_ADDRESS, SYS_FOREVER_MS, &t);
	if (ret < 0) {
		LOG_ERR("SNTP client error (%d)", ret);
		return 0;
	}
	tp.tv_sec = t.seconds;
	tp.tv_nsec = FRAC2NS(t.fraction);
	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tp);
	if (ret < 0) {
		LOG_ERR("SNTP unable to set system time (%d)", ret);
		return 0;
	}

	sntp_server_clock_source("XDEV", 2, -6);
	LOG_INF("SNTP service ready");
	return 0;
}
