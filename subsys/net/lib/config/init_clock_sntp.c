/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <errno.h>
#include <net/sntp.h>
#include <posix/time.h>

int net_init_clock_via_sntp(void)
{
	struct sntp_time ts;
	struct timespec tspec;
	int res = sntp_simple(CONFIG_NET_CONFIG_SNTP_INIT_SERVER,
			      CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, &ts);

	if (res < 0) {
		LOG_ERR("Cannot set time using SNTP");
		return res;
	}

	tspec.tv_sec = ts.seconds;
	tspec.tv_nsec = ((u64_t)ts.fraction * (1000 * 1000 * 1000)) >> 32;
	res = clock_settime(CLOCK_REALTIME, &tspec);

	return 0;
}
