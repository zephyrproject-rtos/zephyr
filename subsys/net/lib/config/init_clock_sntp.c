/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

static int sntp_init_helper(struct sntp_time *tm)
{
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION
	struct net_if *iface = net_if_get_default();

	if (!net_ipv4_is_addr_unspecified(&iface->config.dhcpv4.ntp_addr)) {
		struct sockaddr_in sntp_addr = {0};

		sntp_addr.sin_family = AF_INET;
		sntp_addr.sin_addr.s_addr = iface->config.dhcpv4.ntp_addr.s_addr;
		return sntp_simple_addr((struct sockaddr *)&sntp_addr, sizeof(sntp_addr),
					CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, tm);
	}
	LOG_INF("SNTP address not set by DHCPv4, using Kconfig defaults");
#endif /* NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION */
	return sntp_simple(CONFIG_NET_CONFIG_SNTP_INIT_SERVER,
			   CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, tm);
}

int net_init_clock_via_sntp(void)
{
	struct sntp_time ts;
	struct timespec tspec;
	int res = sntp_init_helper(&ts);

	if (res < 0) {
		LOG_ERR("Cannot set time using SNTP");
		return res;
	}

	tspec.tv_sec = ts.seconds;
	tspec.tv_nsec = ((uint64_t)ts.fraction * (1000 * 1000 * 1000)) >> 32;
	res = clock_settime(CLOCK_REALTIME, &tspec);

	return 0;
}
