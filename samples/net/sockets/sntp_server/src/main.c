/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2025 L. Felten <lothar.felten@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sntp_demo, LOG_LEVEL_DBG);

#include <zephyr/net/sntp.h>
#include <zephyr/net/sntp_server.h>
#include <zephyr/sys/clock.h>
#include "net_sample_common.h"

#define FRAC2NS(x) (uint32_t)((((uint64_t)(x)) * NSEC_PER_SEC) >> 32)

/* Reference identifier of our clock source, RFC5905 Fig.12. Four ASCII
 * characters, left justified and zero padded.
 */
static const uint8_t sntp_ref_id[4] = {'X', 'D', 'E', 'V'};

int main(void)
{
	struct sntp_time t;
	struct timespec tp;
	int ret;

	LOG_INF("waiting for network");
	wait_for_network();

	/* Empty string literal, so the option was left at its default */
	if (sizeof(CONFIG_NET_SAMPLE_SNTP_UPSTREAM_ADDRESS) == 1) {
		LOG_WRN("No upstream SNTP server configured, serving unsynchronized time. "
			"Set CONFIG_NET_SAMPLE_SNTP_UPSTREAM_ADDRESS to synchronize.");
		return 0;
	}

	ret = sntp_simple(CONFIG_NET_SAMPLE_SNTP_UPSTREAM_ADDRESS,
			  CONFIG_NET_SAMPLE_SNTP_UPSTREAM_TIMEOUT_MS, &t);
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

	/* We are one hop away from the server we just queried, so we serve
	 * time at its stratum + 1. Assume a stratum 1 upstream here.
	 */
	sntp_server_clock_source(sntp_ref_id, 2, -6);
	LOG_INF("SNTP service ready");

	return 0;
}
