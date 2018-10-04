/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_zperf_tcp_recv
#define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>

#include <linker/sections.h>
#include <toolchain.h>

#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#define TCP_RX_FIBER_STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(zperf_tcp_rx_stack, TCP_RX_FIBER_STACK_SIZE);
static struct k_thread zperf_tcp_rx_thread_data;

#if defined(CONFIG_NET_IPV6)
static struct sockaddr_in6 *in6_addr_my;
#endif
#if defined(CONFIG_NET_IPV4)
static struct sockaddr_in *in4_addr_my;
#endif

static void tcp_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	const struct shell *shell = user_data;
	struct session *session;
	u32_t time;

	if (!pkt) {
		return;
	}

	time = k_cycle_get_32();

	session = get_session(pkt, SESSION_TCP);
	if (!session) {
		shell_fprintf(shell, SHELL_WARNING, "Cannot get a session!\n");
		return;
	}

	switch (session->state) {
	case STATE_NULL:
	case STATE_COMPLETED:
		shell_fprintf(shell, SHELL_NORMAL, "New session started\n");
		zperf_reset_session_stats(session);
		session->start_time = k_cycle_get_32();
		session->state = STATE_ONGOING;
		/* fall through */
	case STATE_ONGOING:
		session->counter++;
		session->length += net_pkt_appdatalen(pkt);

		if (status == 0) { /* EOF */
			u32_t rate_in_kbps;
			u32_t duration = HW_CYCLES_TO_USEC(
				time_delta(session->start_time, time));

			session->state = STATE_COMPLETED;

			/* Compute baud rate */
			if (duration != 0) {
				rate_in_kbps = (u32_t)
					(((u64_t)session->length *
					  (u64_t)8 *
					  (u64_t)USEC_PER_SEC) /
					 ((u64_t)duration * 1024));
			} else {
				rate_in_kbps = 0;
			}

			shell_fprintf(shell, SHELL_NORMAL,
				      "TCP session ended\n");

			shell_fprintf(shell, SHELL_NORMAL,
				      " Duration:\t\t");
			print_number(shell, duration, TIME_US, TIME_US_UNIT);
			shell_fprintf(shell, SHELL_NORMAL, "\n");

			shell_fprintf(shell, SHELL_NORMAL, " rate:\t\t\t");
			print_number(shell, rate_in_kbps, KBPS, KBPS_UNIT);
			shell_fprintf(shell, SHELL_NORMAL, "\n");
		}
		break;
	case STATE_LAST_PACKET_RECEIVED:
		break;
	default:
		shell_fprintf(shell, SHELL_WARNING, "Unsupported case\n");
	}

	net_pkt_unref(pkt);
}

static void tcp_accepted(struct net_context *context,
			 struct sockaddr *addr,
			 socklen_t addrlen,
			 int error,
			 void *user_data)
{
	const struct shell *shell = user_data;
	int ret;

	ret = net_context_recv(context, tcp_received, K_NO_WAIT, user_data);
	if (ret < 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot receive TCP packet (family %d)",
			      net_context_get_family(context));
	}
}

static void zperf_tcp_rx_thread(const struct shell *shell, int port)
{
	struct net_context *context4 = NULL;
	struct net_context *context6 = NULL;
	int ret, fail = 0;

	if (IS_ENABLED(CONFIG_NET_IPV4) && MY_IP4ADDR) {
		ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
				      &context4);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				     "Cannot get IPv4 TCP network context.\n");
			return;
		}

		ret = zperf_get_ipv4_addr(shell, MY_IP4ADDR,
					  &in4_addr_my->sin_addr);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IPv4\n");
			return;
		}

		shell_fprintf(shell, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

		in4_addr_my->sin_port = htons(port);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && MY_IP6ADDR) {
		ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
				      &context6);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				     "Cannot get IPv6 TCP network context.\n");
			return;
		}

		ret = zperf_get_ipv6_addr(shell, MY_IP6ADDR, MY_PREFIX_LEN_STR,
					  &in6_addr_my->sin6_addr);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IPv6\n");
			return;
		}

		shell_fprintf(shell, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

		in6_addr_my->sin6_port = htons(port);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && context6) {
		ret = net_context_bind(context6,
				       (struct sockaddr *)in6_addr_my,
				       sizeof(struct sockaddr_in6));
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot bind IPv6 TCP port %d (%d)\n",
				      ntohs(in6_addr_my->sin6_port), ret);
			fail++;
		}

		ret = net_context_listen(context6, 0);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot listen IPv6 TCP (%d)", ret);
			return;
		}

		ret = net_context_accept(context6, tcp_accepted, K_NO_WAIT,
					 NULL);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot receive IPv6 TCP packets (%d)",
				      ret);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && context4) {
		ret = net_context_bind(context4,
				       (struct sockaddr *)in4_addr_my,
				       sizeof(struct sockaddr_in));
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot bind IPv4 TCP port %d (%d)\n",
				      ntohs(in4_addr_my->sin_port), ret);
			fail++;
		}

		ret = net_context_listen(context4, 0);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot listen IPv4 TCP (%d)", ret);
			return;
		}

		ret = net_context_accept(context4, tcp_accepted, K_NO_WAIT,
					 (void *)shell);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot receive IPv4 TCP packets (%d)",
				      ret);
			return;
		}
	}

	if (fail > 1) {
		return;
	}

	k_sleep(K_FOREVER);
}

void zperf_tcp_receiver_init(const struct shell *shell, int port)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		in6_addr_my = zperf_get_sin6();
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		in4_addr_my = zperf_get_sin();
	}

	k_thread_create(&zperf_tcp_rx_thread_data, zperf_tcp_rx_stack,
			K_THREAD_STACK_SIZEOF(zperf_tcp_rx_stack),
			(k_thread_entry_t)zperf_tcp_rx_thread,
			(void *)shell, INT_TO_POINTER(port), 0,
			K_PRIO_COOP(7), 0, K_NO_WAIT);
}
