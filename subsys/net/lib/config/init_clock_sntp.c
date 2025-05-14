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
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
static void sntp_resync_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sntp_resync_work_handle, sntp_resync_handler);
#endif

BUILD_ASSERT(
	IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION) ||
	(sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) != 1),
	"SNTP server has to be configured, unless DHCPv4 is used to set it");

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
	if (sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) == 1) {
		/* Empty address, skip using SNTP via Kconfig defaults */
		return -EINVAL;
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

	tspec.tv_sec = ts.seconds;
	tspec.tv_nsec = ((uint64_t)ts.fraction * (1000 * 1000 * 1000)) >> 32;
	res = clock_settime(CLOCK_REALTIME, &tspec);

end:
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
	k_work_reschedule(
		&sntp_resync_work_handle,
		(res < 0) ? K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_ON_FAILURE_INTERVAL)
			  : K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_INTERVAL));
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

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_USE_CONNECTION_MANAGER
static void l4_event_handler(uint32_t mgmt_event, struct net_if *iface, void *info,
			     size_t info_length, void *user_data)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(info);
	ARG_UNUSED(info_length);
	ARG_UNUSED(user_data);

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		k_work_reschedule(&sntp_resync_work_handle, K_NO_WAIT);
	} else if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		k_work_cancel_delayable(&sntp_resync_work_handle);
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(sntp_init_event_handler,
				NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED,
				&l4_event_handler, NULL);
#endif /* CONFIG_LOG_BACKEND_NET_USE_CONNECTION_MANAGER */
