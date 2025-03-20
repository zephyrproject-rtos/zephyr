/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

#include "init_clock_sntp.h"

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
static void sntp_resync_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sntp_resync_work_handle, sntp_resync_handler);
#endif

#if CONFIG_NET_CONFIG_SNTP_TIMEZONE
enum timezone global_tz = UTC_0;

void sntp_set_timezone(const enum timezone tz)
{
  global_tz = tz;
}
#endif

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
		goto end;
	}

#if CONFIG_NET_CONFIG_SNTP_TIMEZONE
  tspec.tv_sec = ts.seconds - (UTC_0 - global_tz) * 3600;
#else
  tspec.tv_sec = ts.seconds;
#endif
	tspec.tv_nsec = ((uint64_t)ts.fraction * (1000 * 1000 * 1000)) >> 32;
	res = clock_settime(CLOCK_REALTIME, &tspec);

end:
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
	k_work_reschedule(&sntp_resync_work_handle,
			  K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_INTERVAL));
#endif
	return res;
}

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
static void sntp_resync_handler(struct k_work *work)
{
	int res;

	ARG_UNUSED(work);

	res = net_init_clock_via_sntp();
	if (res < 0) {
		LOG_ERR("Cannot resync time using SNTP");
		return;
	}
	LOG_DBG("Time resynced using SNTP");
}
#endif /* CONFIG_NET_CONFIG_SNTP_INIT_RESYNC */

#if CONFIG_NET_CONFIG_SNTP_TIMEZONE
const char* get_timezone_string(const enum timezone tz)
{
  static const char* timezones[] = {
      "UTC-12", "UTC-11", "UTC-10", "UTC-9", "UTC-8", "UTC-7", "UTC-6", "UTC-5",
      "UTC-4", "UTC-3", "UTC-2", "UTC-1", "UTC+0", "UTC+1", "UTC+2", "UTC+3",
      "UTC+4", "UTC+5", "UTC+6", "UTC+7", "UTC+8", "UTC+9", "UTC+10", "UTC+11",
      "UTC+12", "UTC+13", "UTC+14"
  };
  return (tz >= 0 && tz < TIMEZONE_MAX) ? timezones[tz] : "Invalid timezone";
}
#endif
