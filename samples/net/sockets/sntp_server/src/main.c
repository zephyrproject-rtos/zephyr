/*
 * Copyright (c) 2025 Lothar Felten <lothar.felten@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sntp_demo, LOG_LEVEL_DBG);

#include <zephyr/net/sntp_server.h>
#include <zephyr/net/sntp.h>
#include "net_sample_common.h"

int main(void)
{
	struct sntp_time t;
	struct timespec tp;
	int ret;

	LOG_INF("waiting for network");
	wait_for_network();
	ret = sntp_simple("ptbtime1.ptb.de", SYS_FOREVER_MS, &t);
	if (ret < 0) {
		LOG_ERR("SNTP client error (%d)", ret);
		return 0;
	}
	tp.tv_sec = t.seconds;
	tp.tv_nsec = t.fraction / 1000;
	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tp);
	if (ret < 0) {
		LOG_ERR("SNTP unable to set system time (%d)", ret);
		return 0;
	}

	sntp_server_clock_source("XDEV", 2, -6);
	return 0;
}
