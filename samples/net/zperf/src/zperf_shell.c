/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_zperf_sample, LOG_LEVEL_DBG);

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include <net/net_ip.h>
#include <net/net_core.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

/* Get some useful debug routings from net_private.h, requires
 * that NET_LOG_ENABLED is set.
 */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "ipv6.h" /* to get infinite lifetime */

#define DEVICE_NAME "zperf shell"

static const char *CONFIG =
		"unified"
#if defined(CONFIG_WIFI)
		" wifi"
#endif
#if defined(CONFIG_NET_L2_ETHERNET)
		" ethernet"
#endif
#if defined(CONFIG_NET_IPV4)
		" ipv4"
#endif
#if defined(CONFIG_NET_IPV6)
		" ipv6"
#endif
		"";

#define MY_SRC_PORT 50000
#define DEF_PORT 5001
#define DEF_PORT_STR STRINGIFY(DEF_PORT)
#define WAIT_CONNECT K_SECONDS(2) /* in ms */

static struct in6_addr ipv6;

static struct sockaddr_in6 in6_addr_my = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(MY_SRC_PORT),
};

static struct sockaddr_in6 in6_addr_dst = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(DEF_PORT),
};

struct sockaddr_in6 *zperf_get_sin6(void)
{
	return &in6_addr_my;
}

static struct in_addr ipv4;

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(MY_SRC_PORT),
};

static struct sockaddr_in in4_addr_dst = {
	.sin_family = AF_INET,
	.sin_port = htons(DEF_PORT),
};

struct sockaddr_in *zperf_get_sin(void)
{
	return &in4_addr_my;
}

static void zperf_init(const struct shell *shell);

static void do_init(const struct shell *shell)
{
	static bool init_ok;

	if (!init_ok) {
		zperf_init(shell);
		init_ok = true;
	}
}

static int parse_ipv6_addr(const struct shell *shell, char *host, char *port,
			   struct sockaddr_in6 *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET6, host, &addr->sin6_addr);
	if (ret < 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid IPv6 address %s\n", host);
		return -EINVAL;
	}

	addr->sin6_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin6_port) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid port %s\n", port);
		return -EINVAL;
	}

	return 0;
}

int zperf_get_ipv6_addr(const struct shell *shell, char *host,
			char *prefix_str, struct in6_addr *addr)
{
	struct net_if_ipv6_prefix *prefix;
	struct net_if_addr *ifaddr;
	int prefix_len;
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET6, host, addr);
	if (ret < 0) {
		return -EINVAL;
	}

	prefix_len = strtoul(prefix_str, NULL, 10);

	ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
				      addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot set IPv6 address %s\n", host);
		return -EINVAL;
	}

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					addr, prefix_len,
					NET_IPV6_ND_INFINITE_LIFETIME);
	if (!prefix) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot set IPv6 prefix %s\n", prefix_str);
		return -EINVAL;
	}

	return 0;
}

static int parse_ipv4_addr(const struct shell *shell, char *host, char *port,
			   struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid IPv4 address %s\n", host);
		return -EINVAL;
	}

	addr->sin_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin_port) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid port %s\n", port);
		return -EINVAL;
	}

	return 0;
}

int zperf_get_ipv4_addr(const struct shell *shell, char *host,
			struct in_addr *addr)
{
	struct net_if_addr *ifaddr;
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, addr);
	if (ret < 0) {
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(net_if_get_default(),
				      addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot set IPv4 address %s\n", host);
		return -EINVAL;
	}

	return 0;
}

const struct in_addr *zperf_get_default_if_in4_addr(void)
{
#if CONFIG_NET_IPV4
	return net_if_ipv4_select_src_addr(NULL,
					   net_ipv4_unspecified_address());
#else
	return NULL;
#endif
}

const struct in6_addr *zperf_get_default_if_in6_addr(void)
{
#if CONFIG_NET_IPV6
	return net_if_ipv6_select_src_addr(NULL,
					   net_ipv6_unspecified_address());
#else
	return NULL;
#endif
}

static int cmd_setip(const struct shell *shell, size_t argc, char *argv[])
{
	int start = 0;

	do_init(shell);

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		if (argc != 3) {
			shell_help(shell);
			return -ENOEXEC;
		}

		if (zperf_get_ipv6_addr(shell, argv[start + 1], argv[start + 2],
					&ipv6) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP\n");
			return 0;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Setting IP address %s\n",
			      net_sprint_ipv6_addr(&ipv6));
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		if (argc != 2) {
			shell_help(shell);
			return -ENOEXEC;
		}

		if (zperf_get_ipv4_addr(shell, argv[start + 1], &ipv4) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP\n");
			return -ENOEXEC;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Setting IP address %s\n",
			      net_sprint_ipv4_addr(&ipv4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		if (net_addr_pton(AF_INET6, argv[start + 1], &ipv6) < 0) {
			if (argc != 2) {
				shell_help(shell);
				return -ENOEXEC;
			}

			if (zperf_get_ipv4_addr(shell, argv[start + 1],
						&ipv4) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to set IP\n");
				return -ENOEXEC;
			}

			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv4_addr(&ipv4));
		} else {
			if (argc != 3) {
				shell_help(shell);
				return -ENOEXEC;
			}

			if (zperf_get_ipv6_addr(shell, argv[start + 1],
						argv[start + 2], &ipv6) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to set IP\n");
				return -ENOEXEC;
			}

			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv6_addr(&ipv6));
		}
	}

	return 0;
}

static int cmd_udp_download(const struct shell *shell, size_t argc,
			    char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		static bool udp_stopped = true;
		int port, start = 0;

		do_init(shell);

		if (argc >= 2) {
			port = strtoul(argv[start + 1], NULL, 10);
		} else {
			port = DEF_PORT;
		}

		if (!udp_stopped) {
			shell_fprintf(shell, SHELL_WARNING,
				      "UDP server already started!\n");
			return -ENOEXEC;
		}

		zperf_udp_receiver_init(shell, port);

		k_yield();

		udp_stopped = false;

		shell_fprintf(shell, SHELL_NORMAL,
			      "UDP server started on port %u\n", port);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static void shell_udp_upload_print_stats(const struct shell *shell,
					 struct zperf_results *results)
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		unsigned int rate_in_kbps, client_rate_in_kbps;

		shell_fprintf(shell, SHELL_NORMAL, "-\nUpload completed!\n");

		if (results->time_in_us != 0U) {
			rate_in_kbps = (u32_t)
				(((u64_t)results->nb_bytes_sent *
				  (u64_t)8 * (u64_t)USEC_PER_SEC) /
				 ((u64_t)results->time_in_us * 1024U));
		} else {
			rate_in_kbps = 0U;
		}

		if (results->client_time_in_us != 0U) {
			client_rate_in_kbps = (u32_t)
				(((u64_t)results->nb_packets_sent *
				  (u64_t)results->packet_size * (u64_t)8 *
				  (u64_t)USEC_PER_SEC) /
				 ((u64_t)results->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}

		if (!rate_in_kbps) {
			shell_fprintf(shell, SHELL_ERROR,
				      "LAST PACKET NOT RECEIVED!!!\n");
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Statistics:\t\tserver\t(client)\n");
		shell_fprintf(shell, SHELL_NORMAL, "Duration:\t\t");
		print_number(shell, results->time_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\t(");
		print_number(shell, results->client_time_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, ")\n");

		shell_fprintf(shell, SHELL_NORMAL, "Num packets:\t\t%u\t(%u)\n",
			      results->nb_packets_rcvd,
			      results->nb_packets_sent);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Num packets out order:\t%u\n",
			      results->nb_packets_outorder);
		shell_fprintf(shell, SHELL_NORMAL, "Num packets lost:\t%u\n",
			      results->nb_packets_lost);

		shell_fprintf(shell, SHELL_NORMAL, "Jitter:\t\t\t");
		print_number(shell, results->jitter_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\n");

		shell_fprintf(shell, SHELL_NORMAL, "Rate:\t\t\t");
		print_number(shell, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\t(");
		print_number(shell, client_rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, ")\n");
	}
}

static void shell_tcp_upload_print_stats(const struct shell *shell,
					 struct zperf_results *results)
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		unsigned int client_rate_in_kbps;

		shell_fprintf(shell, SHELL_NORMAL, "-\nUpload completed!\n");

		if (results->client_time_in_us != 0U) {
			client_rate_in_kbps = (u32_t)
				(((u64_t)results->nb_packets_sent *
				  (u64_t)results->packet_size * (u64_t)8 *
				  (u64_t)USEC_PER_SEC) /
				 ((u64_t)results->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}

		shell_fprintf(shell, SHELL_NORMAL, "Duration:\t");
		print_number(shell, results->client_time_in_us,
			     TIME_US, TIME_US_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\n");
		shell_fprintf(shell, SHELL_NORMAL, "Num packets:\t%u\n",
			      results->nb_packets_sent);
		shell_fprintf(shell, SHELL_NORMAL,
			      "Num errors:\t%u (retry or fail)\n",
			      results->nb_packets_errors);
		shell_fprintf(shell, SHELL_NORMAL, "Rate:\t\t");
		print_number(shell, client_rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\n");
	}
}

static int setup_contexts(const struct shell *shell,
			  struct net_context **context6,
			  struct net_context **context4,
			  sa_family_t family,
			  struct sockaddr_in6 *ipv6,
			  struct sockaddr_in *ipv4,
			  int port,
			  bool is_udp,
			  char *argv0)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = net_context_get(AF_INET6,
				      is_udp ? SOCK_DGRAM : SOCK_STREAM,
				      is_udp ? IPPROTO_UDP : IPPROTO_TCP,
				      context6);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot get IPv6 network context (%d)\n",
				      ret);
			return -ENOEXEC;
		}

		ipv6->sin6_port = htons(port);
		ipv6->sin6_family = AF_INET6;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_context_get(AF_INET,
				      is_udp ? SOCK_DGRAM : SOCK_STREAM,
				      is_udp ? IPPROTO_UDP : IPPROTO_TCP,
				      context4);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot get IPv4 network context (%d)\n",
				      ret);
			return -ENOEXEC;
		}

		ipv4->sin_port = htons(port);
		ipv4->sin_family = AF_INET;
	}

	if (family == AF_INET6 && *context6) {
		ret = net_context_bind(*context6,
				       (struct sockaddr *)ipv6,
				       sizeof(struct sockaddr_in6));
		if (ret < 0) {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Cannot bind IPv6 port %d (%d)",
				      ntohs(ipv6->sin6_port), ret);
			return -ENOEXEC;
		}
	}

	if (family == AF_INET && *context4) {
		ret = net_context_bind(*context4,
				       (struct sockaddr *)ipv4,
				       sizeof(struct sockaddr_in));
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot bind IPv4 port %d (%d)",
				      ntohs(ipv4->sin_port), ret);
			return -ENOEXEC;
		}
	}

	if (!(*context6) && !(*context4)) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Fail to retrieve network context(s)\n");
		return -ENOEXEC;
	}

	return 0;
}

static int execute_upload(const struct shell *shell,
			  struct net_context *context6,
			  struct net_context *context4,
			  sa_family_t family,
			  struct sockaddr_in6 *ipv6,
			  struct sockaddr_in *ipv4,
			  bool is_udp,
			  int port,
			  char *argv0,
			  unsigned int duration_in_ms,
			  unsigned int packet_size,
			  unsigned int rate_in_kbps)
{
	struct zperf_results results = { };
	int ret;

	shell_fprintf(shell, SHELL_NORMAL, "Duration:\t");
	print_number(shell, duration_in_ms * USEC_PER_MSEC, TIME_US,
		     TIME_US_UNIT);
	shell_fprintf(shell, SHELL_NORMAL, "\n");
	shell_fprintf(shell, SHELL_NORMAL, "Packet size:\t%u bytes\n",
		      packet_size);
	shell_fprintf(shell, SHELL_NORMAL, "Rate:\t\t%u kbps\n",
		      rate_in_kbps);
	shell_fprintf(shell, SHELL_NORMAL, "Starting...\n");

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6 && context6) {
		/* For IPv6, we should make sure that neighbor discovery
		 * has been done for the peer. So send ping here, wait
		 * some time and start the test after that.
		 */
		net_icmpv6_send_echo_request(net_if_get_default(),
					     &ipv6->sin6_addr, 0, 0, NULL, 0);

		k_sleep(K_SECONDS(1));
	}

	if (is_udp && IS_ENABLED(CONFIG_NET_UDP)) {
		shell_fprintf(shell, SHELL_NORMAL, "Rate:\t\t");
		print_number(shell, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(shell, SHELL_NORMAL, "\n");

		if (family == AF_INET6 && context6) {
			ret = net_context_connect(context6,
						  (struct sockaddr *)ipv6,
						  sizeof(*ipv6),
						  NULL,
						  K_NO_WAIT,
						  NULL);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "IPv6 connect failed (%d)\n",
					      ret);
				goto out;
			}

			zperf_udp_upload(shell, context6, port, duration_in_ms,
					 packet_size, rate_in_kbps, &results);
			shell_udp_upload_print_stats(shell, &results);
		}

		if (family == AF_INET && context4) {
			ret = net_context_connect(context4,
						  (struct sockaddr *)ipv4,
						  sizeof(*ipv4),
						  NULL,
						  K_NO_WAIT,
						  NULL);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_NORMAL,
					      "IPv4 connect failed (%d)\n",
					      ret);
				goto out;
			}

			zperf_udp_upload(shell, context4, port, duration_in_ms,
					 packet_size, rate_in_kbps, &results);
			shell_udp_upload_print_stats(shell, &results);
		}
	} else {
		if (!IS_ENABLED(CONFIG_NET_UDP)) {
			shell_fprintf(shell, SHELL_INFO,
				      "UDP not supported\n");
		}
	}

	if (!is_udp && IS_ENABLED(CONFIG_NET_TCP)) {
		if (family == AF_INET6 && context6) {
			ret = net_context_connect(context6,
						  (struct sockaddr *)ipv6,
						  sizeof(*ipv6),
						  NULL,
						  WAIT_CONNECT,
						  NULL);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "IPv6 connect failed (%d)\n",
					      ret);
				goto out;
			}

			/* We either upload using IPv4 or IPv6, not both at
			 * the same time.
			 */
			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				net_context_put(context4);
			}

			zperf_tcp_upload(shell, context6, duration_in_ms,
					 packet_size, &results);

			shell_tcp_upload_print_stats(shell, &results);

			return 0;
		}

		if (family == AF_INET && context4) {
			ret = net_context_connect(context4,
						  (struct sockaddr *)ipv4,
						  sizeof(*ipv4),
						  NULL,
						  WAIT_CONNECT,
						  NULL);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "IPv4 connect failed (%d)\n",
					      ret);
				goto out;
			}

			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				net_context_put(context6);
			}

			zperf_tcp_upload(shell, context4, duration_in_ms,
					 packet_size, &results);

			shell_tcp_upload_print_stats(shell, &results);

			return 0;
		}
	} else {
		if (!IS_ENABLED(CONFIG_NET_TCP)) {
			shell_fprintf(shell, SHELL_INFO,
				      "TCP not supported\n");
		}
	}

out:
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_context_put(context6);
	}
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_context_put(context4);
	}

	return 0;
}

static int shell_cmd_upload(const struct shell *shell, size_t argc,
			     char *argv[], enum net_ip_protocol proto)
{
	struct sockaddr_in6 ipv6 = { .sin6_family = AF_INET6 };
	struct sockaddr_in ipv4 = { .sin_family = AF_INET };
	struct net_context *context6 = NULL, *context4 = NULL;
	sa_family_t family = AF_UNSPEC;
	unsigned int duration_in_ms, packet_size, rate_in_kbps;
	char *port_str;
	u16_t port;
	bool is_udp;
	int start = 0;

	is_udp = proto == IPPROTO_UDP;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Not enough parameters.\n");

		if (is_udp) {
			if (IS_ENABLED(CONFIG_NET_UDP)) {
				shell_help(shell);
				return -ENOEXEC;
			}
		} else {
			if (IS_ENABLED(CONFIG_NET_TCP)) {
				shell_help(shell);
				return -ENOEXEC;
			}
		}

		return -ENOEXEC;
	}

	if (argc > 2) {
		port = strtoul(argv[start + 2], NULL, 10);
		shell_fprintf(shell, SHELL_NORMAL,
			      "Remote port is %u\n", port);
		port_str = argv[start + 2];
	} else {
		port = DEF_PORT;
		port_str = DEF_PORT_STR;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		if (parse_ipv6_addr(shell, argv[start + 1], port_str,
				    &ipv6) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Please specify the IP address of the "
				      "remote server.\n");
			return -ENOEXEC;
		}

		shell_fprintf(shell, SHELL_WARNING, "Connecting to %s\n",
			      net_sprint_ipv6_addr(&ipv6.sin6_addr));

		family = AF_INET6;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		if (parse_ipv4_addr(shell, argv[start + 1], port_str,
				    &ipv4) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Please specify the IP address of the "
				      "remote server.\n");
			return -ENOEXEC;
		}

		shell_fprintf(shell, SHELL_NORMAL, "Connecting to %s\n",
			      net_sprint_ipv4_addr(&ipv4.sin_addr));

		family = AF_INET;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		if (parse_ipv6_addr(shell, argv[start + 1], port_str,
				    &ipv6) < 0) {
			if (parse_ipv4_addr(shell, argv[start + 1], port_str,
					    &ipv4) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Please specify the IP address "
					      "of the remote server.\n");
				return -ENOEXEC;
			}

			shell_fprintf(shell, SHELL_NORMAL,
				      "Connecting to %s\n",
				      net_sprint_ipv4_addr(&ipv4.sin_addr));

			family = AF_INET;
		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Connecting to %s\n",
				      net_sprint_ipv6_addr(&ipv6.sin6_addr));

			family = AF_INET6;
		}
	}

	if (setup_contexts(shell, &context6, &context4, family, &in6_addr_my,
			   &in4_addr_my, port, is_udp, argv[start]) < 0) {
		return -ENOEXEC;
	}

	if (argc > 3) {
		duration_in_ms = K_SECONDS(strtoul(argv[start + 3], NULL, 10));
	} else {
		duration_in_ms = K_SECONDS(1);
	}

	if (argc > 4) {
		packet_size = parse_number(argv[start + 4], K, K_UNIT);
	} else {
		packet_size = 256U;
	}

	if (argc > 5) {
		rate_in_kbps =
			(parse_number(argv[start + 5], K, K_UNIT) +
			 1023) / 1024;
	} else {
		rate_in_kbps = 10U;
	}

	return execute_upload(shell, context6, context4, family, &ipv6, &ipv4,
			      is_udp, port, argv[start], duration_in_ms,
			      packet_size, rate_in_kbps);
}

static int cmd_tcp_upload(const struct shell *shell, size_t argc, char *argv[])
{
	do_init(shell);

	return shell_cmd_upload(shell, argc, argv, IPPROTO_TCP);
}

static int cmd_udp_upload(const struct shell *shell, size_t argc, char *argv[])
{
	do_init(shell);

	return shell_cmd_upload(shell, argc, argv, IPPROTO_UDP);
}

static int shell_cmd_upload2(const struct shell *shell, size_t argc,
			     char *argv[], enum net_ip_protocol proto)
{
	struct net_context *context6 = NULL, *context4 = NULL;
	u16_t port = DEF_PORT;
	unsigned int duration_in_ms, packet_size, rate_in_kbps;
	sa_family_t family;
	u8_t is_udp;
	int start = 0;

	is_udp = proto == IPPROTO_UDP;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Not enough parameters.\n");

		if (is_udp) {
			if (IS_ENABLED(CONFIG_NET_UDP)) {
				shell_help(shell);
				return -ENOEXEC;
			}
		} else {
			if (IS_ENABLED(CONFIG_NET_TCP)) {
				shell_help(shell);
				return -ENOEXEC;
			}
		}

		return -ENOEXEC;
	}

	family = !strcmp(argv[start + 1], "v4") ? AF_INET : AF_INET6;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		in6_addr_my.sin6_port = htons(port);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		in4_addr_my.sin_port = htons(port);
	}

	if (family == AF_INET6) {
		if (net_ipv6_is_addr_unspecified(&in6_addr_my.sin6_addr)) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Invalid local IPv6 address.\n");
			return -ENOEXEC;
		}

		if (net_ipv6_is_addr_unspecified(&in6_addr_dst.sin6_addr)) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Invalid destination IPv6 address.\n");
			return -ENOEXEC;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Connecting to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_dst.sin6_addr));
	} else {
		if (net_ipv4_is_addr_unspecified(&in4_addr_my.sin_addr)) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Invalid local IPv4 address.\n");
			return -ENOEXEC;
		}

		if (net_ipv4_is_addr_unspecified(&in4_addr_dst.sin_addr)) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Invalid destination IPv4 address.\n");
			return -ENOEXEC;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Connecting to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_dst.sin_addr));
	}

	if (setup_contexts(shell, &context6, &context4, family, &in6_addr_my,
			   &in4_addr_my, port, is_udp, argv[start]) < 0) {
		return -ENOEXEC;
	}

	if (argc > 2) {
		duration_in_ms = K_SECONDS(strtoul(argv[start + 2], NULL, 10));
	} else {
		duration_in_ms = K_SECONDS(1);
	}

	if (argc > 3) {
		packet_size = parse_number(argv[start + 3], K, K_UNIT);
	} else {
		packet_size = 256U;
	}

	if (argc > 4) {
		rate_in_kbps =
			(parse_number(argv[start + 4], K, K_UNIT) + 1023) /
			1024;
	} else {
		rate_in_kbps = 10U;
	}

	return execute_upload(shell, context6, context4, family, &in6_addr_dst,
			      &in4_addr_dst, is_udp, port, argv[start],
			      duration_in_ms, packet_size, rate_in_kbps);
}

static int cmd_tcp_upload2(const struct shell *shell, size_t argc,
			   char *argv[])
{
	do_init(shell);

	return shell_cmd_upload2(shell, argc, argv, IPPROTO_TCP);
}

static int cmd_udp_upload2(const struct shell *shell, size_t argc,
			   char *argv[])
{
	do_init(shell);

	return shell_cmd_upload2(shell, argc, argv, IPPROTO_UDP);
}

static int cmd_tcp(const struct shell *shell, size_t argc, char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		do_init(shell);

		shell_help(shell);
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_INFO, "TCP support is not enabled. "
		      "Set CONFIG_NET_TCP=y in your config file.\n");

	return -ENOTSUP;
}

static int cmd_udp(const struct shell *shell, size_t argc, char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		do_init(shell);

		shell_help(shell);
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_INFO, "UDP support is not enabled. "
		      "Set CONFIG_NET_UDP=y in your config file.\n");

	return -ENOTSUP;
}

static int cmd_connectap(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_INFO,
		      "Zephyr has not been built with Wi-Fi support.\n");

	return 0;
}

static bool tcp_running;

void zperf_tcp_stopped(void)
{
	tcp_running = false;
}

void zperf_tcp_started(void)
{
	tcp_running = true;
}

static int cmd_tcp_download(const struct shell *shell, size_t argc,
			    char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		int port;

		do_init(shell);

		if (argc >= 2) {
			port = strtoul(argv[1], NULL, 10);
		} else {
			port = DEF_PORT;
		}

		if (tcp_running) {
			shell_fprintf(shell, SHELL_WARNING,
				      "TCP server already started!\n");
			return -ENOEXEC;
		}

		zperf_tcp_receiver_init(shell, port);

		shell_fprintf(shell, SHELL_NORMAL,
			      "TCP server started on port %u\n", port);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static int cmd_version(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Version: %s\nConfig: %s\n",
		      VERSION, CONFIG);

	return 0;
}

static void zperf_init(const struct shell *shell)
{
	int ret;

	shell_fprintf(shell, SHELL_NORMAL, "\n");

	if (IS_ENABLED(CONFIG_NET_IPV6) && MY_IP6ADDR) {
		if (zperf_get_ipv6_addr(shell, MY_IP6ADDR, MY_PREFIX_LEN_STR,
					&ipv6) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP\n");
		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv6_addr(&ipv6));

			net_ipaddr_copy(&in6_addr_my.sin6_addr, &ipv6);
		}

		ret = net_addr_pton(AF_INET6, DST_IP6ADDR,
				    &in6_addr_dst.sin6_addr);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP %s\n",
				      DST_IP6ADDR);
		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting destination IP address %s\n",
				      net_sprint_ipv6_addr(
					      &in6_addr_dst.sin6_addr));
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && MY_IP4ADDR) {
		if (zperf_get_ipv4_addr(shell, MY_IP4ADDR, &ipv4) < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP\n");
		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv4_addr(&ipv4));

			net_ipaddr_copy(&in4_addr_my.sin_addr, &ipv4);
		}

		ret = net_addr_pton(AF_INET, DST_IP4ADDR,
				    &in4_addr_dst.sin_addr);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Unable to set IP %s\n",
				      DST_IP4ADDR);
		} else {
			shell_fprintf(shell, SHELL_NORMAL,
				      "Setting destination IP address %s\n",
				      net_sprint_ipv4_addr(
					      &in4_addr_dst.sin_addr));
		}
	}

	shell_fprintf(shell, SHELL_NORMAL, "\n");

	zperf_session_init();
}

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_tcp,
	SHELL_CMD(upload, NULL,
		  "<dest ip> <dest port> <duration> <packet size>[K]\n"
		  "<dest ip>     IP destination\n"
		  "<dest port>   port destination\n"
		  "<duration>    of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "Example: tcp upload 192.0.2.2 1111 1 1K\n"
		  "Example: tcp upload 2001:db8::2\n",
		  cmd_tcp_upload),
	SHELL_CMD(upload2, NULL,
		  "v6|v4 <duration> <packet size>[K] <baud rate>[K|M]\n"
		  "<v6|v4>:      Use either IPv6 or IPv4\n"
		  "<duration>    Duration of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "Example: tcp upload2 v6 1 1K\n"
		  "Example: tcp upload2 v4\n"
#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR_SET)
		  "Default IPv6 address is " MY_IP6ADDR
		  ", destination [" DST_IP6ADDR "]:" DEF_PORT_STR "\n"
#endif
#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR_SET)
		  "Default IPv4 address is " MY_IP4ADDR
		  ", destination " DST_IP4ADDR ":" DEF_PORT_STR "\n"
#endif
		  ,
		  cmd_tcp_upload2),
	SHELL_CMD(download, NULL,
		  "[<port>]\n"
		  "Example: tcp download 5001\n",
		  cmd_tcp_download),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_udp,
	SHELL_CMD(upload, NULL,
		  "<dest ip> [<dest port> <duration> <packet size>[K] "
							"<baud rate>[K|M]]\n"
		  "<dest ip>     IP destination\n"
		  "<dest port>   port destination\n"
		  "<duration>    of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "<baud rate>   Baudrate in kilobyte or megabyte\n"
		  "Example: udp upload 192.0.2.2 1111 1 1K 1M\n"
		  "Example: udp upload 2001:db8::2\n",
		  cmd_udp_upload),
	SHELL_CMD(upload2, NULL,
		  "v6|v4 [<duration> <packet size>[K] <baud rate>[K|M]]\n"
		  "<v6|v4>:      Use either IPv6 or IPv4\n"
		  "<duration>    Duration of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "<baud rate>   Baudrate in kilobyte or megabyte\n"
		  "Example: udp upload2 v4 1 1K 1M\n"
		  "Example: udp upload2 v6\n"
#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR_SET)
		  "Default IPv6 address is " MY_IP6ADDR
		  ", destination [" DST_IP6ADDR "]:" DEF_PORT_STR "\n"
#endif
#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR_SET)
		  "Default IPv4 address is " MY_IP4ADDR
		  ", destination " DST_IP4ADDR ":" DEF_PORT_STR "\n"
#endif
		  ,
		  cmd_udp_upload2),
	SHELL_CMD(download, NULL,
		  "[<port>]\n"
		  "Example: udp download 5001\n",
		  cmd_udp_download),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_commands,
	SHELL_CMD(connectap, NULL,
		  "Connect to AP",
		  cmd_connectap),
	SHELL_CMD(setip, NULL,
		  "Set IP address\n"
		  "<my ip> <prefix len>\n"
		  "Example setip 2001:db8::2 64\n"
		  "Example setip 192.0.2.2\n",
		  cmd_setip),
	SHELL_CMD(tcp, &zperf_cmd_tcp,
		  "Upload/Download TCP data",
		  cmd_tcp),
	SHELL_CMD(udp, &zperf_cmd_udp,
		  "Upload/Download UDP data",
		  cmd_udp),
	SHELL_CMD(version, NULL,
		  "Zperf version",
		  cmd_version),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(zperf, &zperf_commands, "Zperf commands", NULL);
