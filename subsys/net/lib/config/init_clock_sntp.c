/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <errno.h>
#include <string.h>
#include <time.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
static void sntp_resync_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sntp_resync_work_handle, sntp_resync_handler);
#define RESYNC_FAILED_INTERVAL K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_ON_FAILURE_INTERVAL)
#define RESYNC_INTERVAL K_SECONDS(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC_INTERVAL)
static K_MUTEX_DEFINE(sntp_resync_lock);
/* Variables protected under sntp_resync_lock */
struct sntp_resync_state {
	unsigned int generation;
	bool enabled;
	bool async_dns_in_progress;
	uint16_t dns_id;
	bool async_sntp_in_progress;
};
static struct sntp_resync_state resync_state = {
	.enabled = !IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_USE_CONNECTION_MANAGER),
};
static void sntp_resync_cancel(const struct sntp_resync_state *state);
#endif

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME
static K_MUTEX_DEFINE(sntp_server_lock);

/* Server set by the application. Written from any thread, under the lock. */
static char sntp_server_cfg[CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN + 1];
#endif

BUILD_ASSERT(
	IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION) ||
	IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME) ||
	(sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) != 1),
	"SNTP server has to be configured, unless DHCPv4 or net_config_sntp_set_server is used to set it");

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME
int net_config_sntp_set_server(const char *server)
{
	if (server != NULL && strlen(server) > CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	k_mutex_lock(&sntp_server_lock, K_FOREVER);
	if (server != NULL) {
		strncpy(sntp_server_cfg, server, CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN);
		sntp_server_cfg[CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN] = '\0';
	} else {
		sntp_server_cfg[0] = '\0';
	}
	k_mutex_unlock(&sntp_server_lock);

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (resync_state.enabled) {
		struct sntp_resync_state state;

		++resync_state.generation;
		state = resync_state;
		/* Temporarily clear enabled flag to prevent callbacks from re-enqueuing work that
		 * should be cancelled.
		 */
		resync_state.enabled = false;
		/* Cancel currently running resync */
		sntp_resync_cancel(&state);
		if (resync_state.generation == state.generation) {
			/* Reset state after cancellation */
			resync_state.enabled = true;
			++resync_state.generation;
			/* Use the new server right away. */
			k_work_reschedule(&sntp_resync_work_handle, K_NO_WAIT);
		}
	}
	k_mutex_unlock(&sntp_resync_lock);
#endif

	return 0;
}

static const char *
sntp_runtime_server(char user_buf[static CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN + 1])
{
	k_mutex_lock(&sntp_server_lock, K_FOREVER);
	strncpy(user_buf, sntp_server_cfg, CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN);
	k_mutex_unlock(&sntp_server_lock);
	user_buf[CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN] = '\0';

	return user_buf[0] != '\0' ? user_buf : NULL;
}
#endif

static int sntp_init_helper(struct sntp_time *tm)
{
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME
	char server_buf[CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN + 1];
	const char *server = sntp_runtime_server(server_buf);

	if (server != NULL) {
		return sntp_simple(server, CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, tm);
	}
#endif /* CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME */
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION
	struct net_if *iface = net_if_get_default();

	if (!net_ipv4_is_addr_unspecified(&iface->config.dhcpv4.ntp_addr)) {
		struct net_sockaddr_in sntp_addr = {0};

		sntp_addr.sin_family = NET_AF_INET;
		sntp_addr.sin_addr.s_addr = iface->config.dhcpv4.ntp_addr.s_addr;
		return sntp_simple_addr((struct net_sockaddr *)&sntp_addr, sizeof(sntp_addr),
					CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, tm);
	}
#endif /* NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION */
	if (sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) == 1) {
		/* Empty address, skip using SNTP via Kconfig defaults */
		return -EINVAL;
	}
#if IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION) ||                            \
	IS_ENABLED(CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME)
	LOG_INF("SNTP address not set by DHCPv4 or net_config_sntp_set_server, using Kconfig "
		"defaults");
#endif
	return sntp_simple(CONFIG_NET_CONFIG_SNTP_INIT_SERVER,
			   CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, tm);
}

#ifdef CONFIG_NET_CONFIG_CLOCK_SNTP_SET_RTC
static int timespec_to_rtc_time(const struct timespec *in, struct rtc_time *out)
{
	if (gmtime_r(&in->tv_sec, rtc_time_to_tm(out)) == NULL) {
		return -EINVAL;
	}

	out->tm_nsec = in->tv_nsec;

	return 0;
}

static void sntp_set_rtc(const struct timespec *tspec)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_rtc));
	struct rtc_time rtctime;
	int res;

	if (!device_is_ready(dev)) {
		return;
	}

	if (timespec_to_rtc_time(tspec, &rtctime) != 0) {
		LOG_ERR("Convert timespec to set RTC failed");
		return;
	}

	res = rtc_set_time(dev, &rtctime);
	if (res != 0) {
		LOG_ERR("Set RTC failed: %d", res);
	}
}
#else
static void sntp_set_rtc(const struct timespec *tspec)
{
	ARG_UNUSED(tspec);
}
#endif

static int sntp_set_clocks(struct sntp_time *ts)
{
	struct timespec tspec;
	int ret;

	tspec.tv_sec = ts->seconds;
	tspec.tv_nsec = ((uint64_t)ts->fraction * NSEC_PER_SEC) >> 32;
	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tspec);
	if (ret < 0) {
		LOG_ERR("Setting sys clock failed (%d)", ret);
	}

	sntp_set_rtc(&tspec);

	LOG_DBG("Time synced using SNTP, SNTP Time: %" PRIu64, ts->seconds);

	return ret;
}

int net_config_init_clock_via_sntp(void)
{
	struct sntp_time ts;
	int res = sntp_init_helper(&ts);

	if (res < 0) {
		LOG_ERR("Cannot set time using SNTP: %d", res);
		goto end;
	}

	res = sntp_set_clocks(&ts);

end:
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (resync_state.enabled) {
		k_work_reschedule(&sntp_resync_work_handle,
				  (res < 0) ? RESYNC_FAILED_INTERVAL : RESYNC_INTERVAL);
	}
	k_mutex_unlock(&sntp_resync_lock);
#endif
	return res;
}

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_RESYNC
#define SNTP_SERVER_PORT 123

static void sntp_async_service_handler(struct net_socket_service_event *pev);
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(sntp_service_async, sntp_async_service_handler, 1);

static void sntp_async_timeout(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sntp_async_timeout_work, sntp_async_timeout);

static struct sntp_ctx sntp_async_ctx;
static struct net_sockaddr_storage sntp_addr;
static net_socklen_t sntp_addrlen;

/* Copy of sntp_server_cfg owned by resync work */
static char sntp_server_active[CONFIG_NET_CONFIG_SNTP_INIT_SERVER_MAX_LEN + 1];

static void sntp_async_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_WRN("SNTP query timed out");

	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	sntp_close_async(&sntp_service_async);

	resync_state.async_sntp_in_progress = false;
	if (resync_state.enabled) {
		/* No response, reschedule */
		k_work_reschedule(&sntp_resync_work_handle, RESYNC_FAILED_INTERVAL);
	}
	k_mutex_unlock(&sntp_resync_lock);
}

static void sntp_async_service_handler(struct net_socket_service_event *pev)
{
	struct sntp_time ts;
	int ret;

	ret = sntp_read_async(pev, &ts);
	if (ret < 0) {
		LOG_ERR("Failed to read SNTP response (%d)", ret);
		goto out;
	}

	ret = sntp_set_clocks(&ts);

out:
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	/* Close the service */
	sntp_close_async(&sntp_service_async);

	k_work_cancel_delayable(&sntp_async_timeout_work);

	resync_state.async_sntp_in_progress = false;
	if (resync_state.enabled && (ret < 0)) {
		k_work_reschedule(&sntp_resync_work_handle, RESYNC_FAILED_INTERVAL);
	}
	k_mutex_unlock(&sntp_resync_lock);
}

static int sntp_query_async(struct sntp_resync_state *state, struct net_sockaddr *addr,
			    net_socklen_t addrlen)
{
	int ret;

	ret = sntp_init_async(&sntp_async_ctx, addr, addrlen, &sntp_service_async);
	if (ret < 0) {
		LOG_ERR("Failed to initialize SNTP context (%d)", ret);
		goto end;
	}

	ret = sntp_send_async(&sntp_async_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to send SNTP query (%d)", ret);
		sntp_close_async(&sntp_service_async);
		goto end;
	}

	k_work_reschedule(&sntp_async_timeout_work,
			  K_MSEC(CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT));
	state->async_sntp_in_progress = true;

end:
	return ret;
}

static void dns_result_cb(enum dns_resolve_status status,
			  struct dns_addrinfo *info,
			  void *user_data)
{
	int ret;
	const char *server = user_data;
	bool need_resched = false;
	struct sntp_resync_state state;

	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (!resync_state.enabled) {
		resync_state.async_dns_in_progress = false;
		k_mutex_unlock(&sntp_resync_lock);
		return;
	}
	state = resync_state;
	k_mutex_unlock(&sntp_resync_lock);

	if (status == DNS_EAI_CANCELED || status == DNS_EAI_FAIL) {
		/* If IPv4 query failed, try IPv6. Otherwise, just schedule next retry. */
		if (sntp_addr.ss_family == NET_AF_INET && IS_ENABLED(CONFIG_NET_IPV6)) {
			sntp_addr.ss_family = NET_AF_INET6;
			sntp_addrlen = 0;

			ret = dns_get_addr_info(server, DNS_QUERY_TYPE_AAAA, &state.dns_id,
						dns_result_cb, (void *)server,
						CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT);
			if (ret == 0) {
				goto out;
			}
		}

		LOG_WRN("DNS query timed out");
		state.async_dns_in_progress = false;
		need_resched = true;
		goto out;
	}

	if (status == DNS_EAI_ALLDONE) {
		/* If address was found, schedule SNTP query, if that fails schedule next retry. */
		state.async_dns_in_progress = false;
		ret = sntp_query_async(&state, net_sad(&sntp_addr), sntp_addrlen);
		if (ret < 0) {
			need_resched = true;
		}

		goto out;
	}

	if (status == DNS_EAI_INPROGRESS && info != NULL) {
		if (sntp_addrlen > 0) {
			/* Already got address, skip others. */
			return;
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) && info->ai_family == NET_AF_INET) {
			struct net_sockaddr *ai_addr = net_sad(&info->ai_addr_storage);

			sntp_addrlen = info->ai_addrlen;
			sntp_addr.ss_family = NET_AF_INET;
			net_ipv4_addr_copy_raw(net_sin(net_sad(&sntp_addr))->sin_addr.s4_addr,
					       net_sin(ai_addr)->sin_addr.s4_addr);
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && info->ai_family == NET_AF_INET6) {
			struct net_sockaddr *ai_addr = net_sad(&info->ai_addr_storage);

			sntp_addrlen = info->ai_addrlen;
			sntp_addr.ss_family = NET_AF_INET6;
			net_ipv6_addr_copy_raw(net_sin6(net_sad(&sntp_addr))->sin6_addr.s6_addr,
					       net_sin6(ai_addr)->sin6_addr.s6_addr);
		} else {
			/* Ignore. */
			return;
		}

		(void)net_port_set_default(net_sad(&sntp_addr), SNTP_SERVER_PORT);
	}

out:
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (state.generation != resync_state.generation) {
		/* Discard local changes, sntp resync was cancelled externally */
		k_mutex_unlock(&sntp_resync_lock);
		return;
	}
	if (need_resched) {
		k_work_reschedule(&sntp_resync_work_handle, RESYNC_FAILED_INTERVAL);
	}
	resync_state.dns_id = state.dns_id;
	resync_state.async_dns_in_progress = state.async_dns_in_progress;
	resync_state.async_sntp_in_progress = state.async_sntp_in_progress;
	k_mutex_unlock(&sntp_resync_lock);
}

static int dns_query_async(struct sntp_resync_state *state, const char *server)
{
	enum dns_query_type type;
	int ret;

	memset(&sntp_addr, 0, sizeof(sntp_addr));
	sntp_addrlen = 0;

	if (IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			sntp_addr.ss_family = NET_AF_INET;
			type = DNS_QUERY_TYPE_A;
		} else {
			sntp_addr.ss_family = NET_AF_INET6;
			type = DNS_QUERY_TYPE_AAAA;
		}

		ret = dns_get_addr_info(server, type, &state->dns_id, dns_result_cb, (void *)server,
					CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT);
		if (ret < 0) {
			state->async_dns_in_progress = false;
			LOG_ERR("Failed to initiate DNS query for SNTP server (%d)", ret);
		} else {
			state->async_dns_in_progress = true;
		}
		return ret;
	}

	/* Fallback for IP address string. */
	if (net_ipaddr_parse(server,
			     strlen(server),
			     net_sad(&sntp_addr))) {
		if (IS_ENABLED(CONFIG_NET_IPV4) && sntp_addr.ss_family == NET_AF_INET) {
			sntp_addrlen = sizeof(struct net_sockaddr_in);
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && sntp_addr.ss_family == NET_AF_INET6) {
			sntp_addrlen = sizeof(struct net_sockaddr_in6);
		} else {
			/* Unsupported family */
			return -EINVAL;
		}

		ret = net_port_set_default(net_sad(&sntp_addr), SNTP_SERVER_PORT);
		if (ret < 0) {
			return ret;
		}

		ret = sntp_query_async(state, net_sad(&sntp_addr), sntp_addrlen);
	} else {
		LOG_ERR("Failed to parse SNTP server address, enable CONFIG_DNS_RESOLVER");
		ret = -EINVAL;
	}

	return ret;
}

static void sntp_resync_handler(struct k_work *work)
{
	int ret = 0;
	struct sntp_resync_state state;

	ARG_UNUSED(work);

	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (!resync_state.enabled) {
		k_mutex_unlock(&sntp_resync_lock);
		return;
	}
	state = resync_state;
	k_mutex_unlock(&sntp_resync_lock);

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME
	const char *server = sntp_runtime_server(sntp_server_active);

	if (server != NULL) {
		ret = dns_query_async(&state, server);
		goto out;
	}
#endif /* CONFIG_NET_CONFIG_SNTP_INIT_SERVER_RUNTIME */
#ifdef CONFIG_NET_CONFIG_SNTP_INIT_SERVER_USE_DHCPV4_OPTION
	struct net_if *iface = net_if_get_default();

	if (!net_ipv4_is_addr_unspecified(&iface->config.dhcpv4.ntp_addr)) {
		sntp_addrlen = sizeof(struct net_sockaddr_in);
		sntp_addr.ss_family = NET_AF_INET;
		net_ipv4_addr_copy_raw(net_sin(net_sad(&sntp_addr))->sin_addr.s4_addr,
					       iface->config.dhcpv4.ntp_addr.s4_addr);
		net_sin(net_sad(&sntp_addr))->sin_port = net_htons(SNTP_SERVER_PORT);

		ret = sntp_query_async(&state, net_sad(&sntp_addr), sntp_addrlen);
		if (ret < 0) {
			LOG_ERR("Cannot set time using SNTP: %d", ret);
		}

		goto out;
	}

#endif
	if (sizeof(CONFIG_NET_CONFIG_SNTP_INIT_SERVER) == 1) {
		/* Empty address, skip using SNTP via Kconfig defaults */
		ret = -EINVAL;
		goto out;
	}

	ret = dns_query_async(&state, CONFIG_NET_CONFIG_SNTP_INIT_SERVER);

out:
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (state.generation != resync_state.generation) {
		/* Discard local changes, sntp resync was cancelled externally */
		k_mutex_unlock(&sntp_resync_lock);
		return;
	}
	k_work_reschedule(&sntp_resync_work_handle,
			  (ret < 0) ? RESYNC_FAILED_INTERVAL : RESYNC_INTERVAL);
	resync_state.dns_id = state.dns_id;
	resync_state.async_dns_in_progress = state.async_dns_in_progress;
	resync_state.async_sntp_in_progress = state.async_sntp_in_progress;
	k_mutex_unlock(&sntp_resync_lock);
}

/* Caller must hold sntp_resync_lock. This function releases and reacquires the lock. */
static void sntp_resync_cancel(const struct sntp_resync_state *state)
{
	resync_state.async_sntp_in_progress = false;
	resync_state.async_dns_in_progress = false;
	k_mutex_unlock(&sntp_resync_lock);
	if (IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		if (state->async_dns_in_progress) {
			dns_cancel_addr_info(state->dns_id);
		}
	}
	if (state->async_sntp_in_progress) {
		sntp_close_async(&sntp_service_async);
	}
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (state->generation == resync_state.generation) {
		/* Only cancel work if generation still matches, otherwise we could undo the work of
		 * an sntp_resync_enable called while we released the lock.
		 */
		k_work_cancel_delayable(&sntp_async_timeout_work);
		k_work_cancel_delayable(&sntp_resync_work_handle);
	}
}
#endif /* CONFIG_NET_CONFIG_SNTP_INIT_RESYNC */

#ifdef CONFIG_NET_CONFIG_SNTP_INIT_USE_CONNECTION_MANAGER
/* Disable resync and cancel running work and async contexts */
static void sntp_resync_disable(void)
{
	struct sntp_resync_state state;

	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	++resync_state.generation;
	state = resync_state;
	resync_state.enabled = false;
	sntp_resync_cancel(&state);
	k_mutex_unlock(&sntp_resync_lock);
}

static void sntp_resync_enable(void)
{
	k_mutex_lock(&sntp_resync_lock, K_FOREVER);
	if (!resync_state.enabled) {
		resync_state.enabled = true;
		++resync_state.generation;
		k_work_reschedule(&sntp_resync_work_handle, K_NO_WAIT);
	}
	k_mutex_unlock(&sntp_resync_lock);
}

static void l4_event_handler(uint64_t mgmt_event, struct net_if *iface, void *info,
			     size_t info_length, void *user_data)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(info);
	ARG_UNUSED(info_length);
	ARG_UNUSED(user_data);

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		sntp_resync_enable();
	} else if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		sntp_resync_disable();
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(sntp_init_event_handler,
				NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED,
				&l4_event_handler, NULL);
#endif /* CONFIG_LOG_BACKEND_NET_USE_CONNECTION_MANAGER */
