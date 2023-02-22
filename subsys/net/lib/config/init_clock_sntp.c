/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

int net_init_clock_via_sntp(void)
{
	int res = sntp_set_system_time(CONFIG_NET_CONFIG_SNTP_INIT_SERVER,
				       CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT);

	if (res < 0) {
		LOG_ERR("Cannot set time using SNTP");
		return res;
	}

	return 0;
}
