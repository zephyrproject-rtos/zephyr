/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "net/zperf"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>

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
#if defined(CONFIG_WLAN)
		" wlan"
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

#if defined(PROFILER)
#include "profiler.h"
#endif

#define MY_SRC_PORT 50000
#define DEF_PORT 5001
#define WAIT_CONNECT (2 * 1000) /* in ms */

#if defined(CONFIG_NET_IPV6)
static struct in6_addr ipv6;
#endif

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

#if defined(CONFIG_NET_IPV4)
static struct in_addr ipv4;
#endif

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

#if defined(CONFIG_NET_IPV6)
static int parse_ipv6_addr(char *host, char *port,
			   struct sockaddr_in6 *addr,
			   const char *str)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET6, host, &addr->sin6_addr);
	if (ret < 0) {
		printk("[%s] Error! Invalid IPv6 address %s\n", str, host);
		return -EINVAL;
	}

	addr->sin6_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin6_port) {
		printk("[%s] Error! Invalid port %s\n", str, port);
		return -EINVAL;
	}

	return 0;
}

int zperf_get_ipv6_addr(char *host, char *prefix_str, struct in6_addr *addr,
			const char *str)
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
		printk("[%s] Error! Cannot set IPv6 address\n", str);
		return -EINVAL;
	}

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					addr, prefix_len,
					NET_IPV6_ND_INFINITE_LIFETIME);
	if (!prefix) {
		printk("[%s] Error! Cannot set IPv6 prefix\n", str);
		return -EINVAL;
	}

	return 0;
}
#endif

#if defined(CONFIG_NET_IPV4)
static int parse_ipv4_addr(char *host, char *port,
			   struct sockaddr_in *addr,
			   const char *str)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		printk("[%s] Error! Invalid IPv4 address %s\n", str, host);
		return -EINVAL;
	}

	addr->sin_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin_port) {
		printk("[%s] Error! Invalid port %s\n", str, port);
		return -EINVAL;
	}

	return 0;
}

int zperf_get_ipv4_addr(char *host, struct in_addr *addr, const char *str)
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
		printk("[%s] Error! Cannot set IPv4 address\n", str);
		return -EINVAL;
	}

	return 0;
}
#endif

static int shell_cmd_setip(int argc, char *argv[])
{
	int start = 0;

	if (!strcmp(argv[0], "zperf")) {
		start++;
		argc--;
	}

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	if (argc != 3) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_SETIP);
		printk("Usage:\t%s <my ip> <prefix len>\n", CMD_STR_SETIP);
		printk("\nExample %s 2001:db8::2 64\n", CMD_STR_SETIP);
		return -1;
	}

	if (zperf_get_ipv6_addr(argv[start + 1], argv[start + 2], &ipv6,
				CMD_STR_SETIP) < 0) {
		printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
		return 0;
	}

	printk("[%s] Setting IP address %s\n", CMD_STR_SETIP,
	       net_sprint_ipv6_addr(&ipv6));
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	if (argc != 2) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_SETIP);
		printk("Usage:\t%s <my ip>\n", CMD_STR_SETIP);
		printk("\nExample %s 10.237.164.178\n", CMD_STR_SETIP);
		return -1;
	}

	if (zperf_get_ipv4_addr(argv[start + 1], &ipv4, CMD_STR_SETIP) < 0) {
		printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
		return 0;
	}

	printk("[%s] Setting IP address %s\n", CMD_STR_SETIP,
	       net_sprint_ipv4_addr(&ipv4));
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	if (net_addr_pton(AF_INET6, argv[start + 1],
			  &ipv6) < 0) {
		if (argc != 2) {
			/* Print usage */
			printk("\n%s:\n", CMD_STR_SETIP);
			printk("Usage:\t%s <my ip>\n", CMD_STR_SETIP);
			printk("\nExample %s 10.237.164.178\n", CMD_STR_SETIP);
			printk("Example %s 2001:db8::1 64\n", CMD_STR_SETIP);
			return -1;
		}

		if (zperf_get_ipv4_addr(argv[start + 1], &ipv4,
					CMD_STR_SETIP) < 0) {
			printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
			return 0;
		}

		printk("[%s] Setting IP address %s\n", CMD_STR_SETIP,
		       net_sprint_ipv4_addr(&ipv4));
	} else {
		if (argc != 3) {
			/* Print usage */
			printk("\n%s:\n", CMD_STR_SETIP);
			printk("Usage:\t%s <my ip> <prefix len>\n",
			       CMD_STR_SETIP);
			printk("\nExample %s 2001:db8::2 64\n", CMD_STR_SETIP);
			printk("Example %s 10.237.164.178\n", CMD_STR_SETIP);
			return -1;
		}

		if (zperf_get_ipv6_addr(argv[start + 1], argv[start + 2], &ipv6,
					CMD_STR_SETIP) < 0) {
			printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
			return 0;
		}

		printk("[%s] Setting IP address %s\n", CMD_STR_SETIP,
		       net_sprint_ipv6_addr(&ipv6));
	}
#endif

	return 0;
}

#if defined(CONFIG_NET_UDP)
static int shell_cmd_udp_download(int argc, char *argv[])
{
	static bool udp_stopped = true;
	int port, start = 0;

	if (!strcmp(argv[0], "zperf")) {
		start++;
		argc--;
	}

	if (argc == 1) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_UDP_DOWNLOAD);
		printk("Usage:\t%s <port>\n", CMD_STR_UDP_DOWNLOAD);
		printk("\nExample %s 5001\n", CMD_STR_UDP_DOWNLOAD);
		return -1;
	}

	if (argc > 1) {
		port = strtoul(argv[start + 1], NULL, 10);
	} else {
		port = DEF_PORT;
	}

	if (!udp_stopped) {
		printk("[%s] ERROR! UDP server already started!\n",
			CMD_STR_UDP_DOWNLOAD);
		return -1;
	}

	zperf_receiver_init(port);

	k_yield();

	udp_stopped = false;

	printk("[%s] UDP server started on port %u\n", CMD_STR_UDP_DOWNLOAD,
	       port);

	return 0;
}
#endif

#if defined(CONFIG_NET_UDP)
static void shell_udp_upload_usage(void)
{
	/* Print usage */
	printk("\n%s:\n", CMD_STR_UDP_UPLOAD);
	printk("Usage:\t%s <dest ip> <dest port> <duration> <packet "
	       "size>[K] <baud rate>[K|M]\n", CMD_STR_UDP_UPLOAD);
	printk("\t<dest ip>:\tIP destination\n");
	printk("\t<dest port>:\tUDP destination port\n");
	printk("\t<duration>:\tDuration of the test in seconds\n");
	printk("\t<packet size>:\tSize of the packet in byte or kilobyte "
	       "(with suffix K)\n");
	printk("\t<baud rate>:\tBaudrate in kilobyte or megabyte\n");
	printk("\nExample %s 10.237.164.178 1111 1 1K 1M\n",
	       CMD_STR_UDP_UPLOAD);
}

static void shell_udp_upload2_usage(void)
{
	/* Print usage */
	printk("\n%s:\n", CMD_STR_UDP_UPLOAD2);
	printk("Usage:\t%s v6|v4 <duration> <packet "
	       "size>[K] <baud rate>[K|M]\n", CMD_STR_UDP_UPLOAD2);
	printk("\t<v6|v4>:\tUse either IPv6 or IPv4\n");
	printk("\t<duration>:\tDuration of the test in seconds\n");
	printk("\t<packet size>:\tSize of the packet in byte or kilobyte "
	       "(with suffix K)\n");
	printk("\t<baud rate>:\tBaudrate in kilobyte or megabyte\n");
	printk("\nExample %s v6 1 1K 1M\n",
	       CMD_STR_UDP_UPLOAD2);
#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR)
	printk("\nDefault IPv6 address is %s, destination [%s]:%d\n",
	       MY_IP6ADDR, DST_IP6ADDR, DEF_PORT);
#endif
#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR)
	printk("\nDefault IPv4 address is %s, destination %s:%d\n",
	       MY_IP4ADDR, DST_IP4ADDR, DEF_PORT);
#endif
}
#endif

#if defined(CONFIG_NET_TCP)
static void shell_tcp_upload_usage(void)
{
	/* Print usage */
	printk("\n%s:\n", CMD_STR_TCP_UPLOAD);
	printk("Usage:\t%s <dest ip> <dest port> <duration> <packet "
	       "size>[K]\n", CMD_STR_TCP_UPLOAD);
	printk("\t<dest ip>:\tIP destination\n");
	printk("\t<dest port>:\tport destination\n");
	printk("\t<duration>:\t of the test in seconds\n");
	printk("\t<packet size>:\tSize of the packet in byte or kilobyte "
	       "(with suffix K)\n");
	printk("\nExample %s 10.237.164.178 1111 1 1K 1M\n",
	       CMD_STR_TCP_UPLOAD);
}

static void shell_tcp_upload2_usage(void)
{
	/* Print usage */
	printk("\n%s:\n", CMD_STR_TCP_UPLOAD2);
	printk("Usage:\t%s v6|v4 <duration> <packet "
	       "size>[K] <baud rate>[K|M]\n", CMD_STR_TCP_UPLOAD2);
	printk("\t<v6|v4>:\tUse either IPv6 or IPv4\n");
	printk("\t<duration>:\tDuration of the test in seconds\n");
	printk("\t<packet size>:\tSize of the packet in byte or kilobyte "
	       "(with suffix K)\n");
	printk("\t<baud rate>:\tBaudrate in kilobyte or megabyte\n");
	printk("\nExample %s v6 1 1K 1M\n",
	       CMD_STR_TCP_UPLOAD2);
#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR)
	printk("\nDefault IPv6 address is %s, destination [%s]:%d\n",
	       MY_IP6ADDR, DST_IP6ADDR, DEF_PORT);
#endif
#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR)
	printk("\nDefault IPv4 address is %s, destination %s:%d\n",
	       MY_IP4ADDR, DST_IP4ADDR, DEF_PORT);
#endif
}
#endif

#if defined(CONFIG_NET_UDP)
static void shell_udp_upload_print_stats(struct zperf_results *results)
{
	unsigned int rate_in_kbps, client_rate_in_kbps;

	printk("[%s] Upload completed!\n", CMD_STR_UDP_UPLOAD);

	if (results->time_in_us != 0) {
		rate_in_kbps = (u32_t)
			(((u64_t)results->nb_bytes_sent *
			  (u64_t)8 * (u64_t)USEC_PER_SEC) /
			 ((u64_t)results->time_in_us * 1024));
	} else {
		rate_in_kbps = 0;
	}

	if (results->client_time_in_us != 0) {
		client_rate_in_kbps = (u32_t)
			(((u64_t)results->nb_packets_sent *
			  (u64_t)results->packet_size * (u64_t)8 *
			  (u64_t)USEC_PER_SEC) /
			 ((u64_t)results->client_time_in_us * 1024));
	} else {
		client_rate_in_kbps = 0;
	}

	if (!rate_in_kbps) {
		printk("[%s] LAST PACKET NOT RECEIVED!!!\n",
		       CMD_STR_UDP_UPLOAD);
	}

	printk("[%s] statistics:\t\tserver\t(client)\n", CMD_STR_UDP_UPLOAD);
	printk("[%s] duration:\t\t\t", CMD_STR_UDP_UPLOAD);
	print_number(results->time_in_us, TIME_US, TIME_US_UNIT);
	printk("\t(");
	print_number(results->client_time_in_us, TIME_US, TIME_US_UNIT);
	printk(")\n");

	printk("[%s] nb packets:\t\t%u\t(%u)\n", CMD_STR_UDP_UPLOAD,
	       results->nb_packets_rcvd,
	       results->nb_packets_sent);

	printk("[%s] nb packets outorder:\t%u\n", CMD_STR_UDP_UPLOAD,
	       results->nb_packets_outorder);
	printk("[%s] nb packets lost:\t\t%u\n", CMD_STR_UDP_UPLOAD,
	       results->nb_packets_lost);

	printk("[%s] jitter:\t\t\t", CMD_STR_UDP_UPLOAD);
	print_number(results->jitter_in_us, TIME_US, TIME_US_UNIT);
	printk("\n");

	printk("[%s] rate:\t\t\t", CMD_STR_UDP_UPLOAD);
	print_number(rate_in_kbps, KBPS, KBPS_UNIT);
	printk("\t(");
	print_number(client_rate_in_kbps, KBPS, KBPS_UNIT);
	printk(")\n");
}
#endif

#if defined(CONFIG_NET_TCP)
static void shell_tcp_upload_print_stats(struct zperf_results *results)
{
	unsigned int client_rate_in_kbps;

	printk("[%s] Upload completed!\n", CMD_STR_TCP_UPLOAD);

	if (results->client_time_in_us != 0) {
		client_rate_in_kbps = (u32_t)
			(((u64_t)results->nb_packets_sent *
			  (u64_t)results->packet_size * (u64_t)8 *
			  (u64_t)USEC_PER_SEC) /
			 ((u64_t)results->client_time_in_us * 1024));
	} else {
		client_rate_in_kbps = 0;
	}

	printk("[%s] duration:\t", CMD_STR_TCP_UPLOAD);
	print_number(results->client_time_in_us, TIME_US, TIME_US_UNIT);
	printk("\n");
	printk("[%s] nb packets:\t%u\n", CMD_STR_TCP_UPLOAD,
	       results->nb_packets_sent);
	printk("[%s] nb sending errors (retry or fail):\t%u\n",
	       CMD_STR_TCP_UPLOAD, results->nb_packets_errors);
	printk("[%s] rate:\t", CMD_STR_TCP_UPLOAD);
	print_number(client_rate_in_kbps, KBPS, KBPS_UNIT);
	printk("\n");
}
#endif

static int setup_contexts(struct net_context **context6,
			  struct net_context **context4,
			  sa_family_t family,
			  struct sockaddr_in6 *ipv6,
			  struct sockaddr_in *ipv4,
			  int port,
			  bool is_udp,
			  char *argv0)
{
	int ret;

#if defined(CONFIG_NET_IPV6)
	ret = net_context_get(AF_INET6,
			      is_udp ? SOCK_DGRAM : SOCK_STREAM,
			      is_udp ? IPPROTO_UDP : IPPROTO_TCP,
			      context6);
	if (ret < 0) {
		printk("[%s] Cannot get IPv6 network context (%d)\n",
		       argv0, ret);
		return -1;
	}

	ipv6->sin6_port = htons(port);
	ipv6->sin6_family = AF_INET6;
#endif

#if defined(CONFIG_NET_IPV4)
	ret = net_context_get(AF_INET,
			      is_udp ? SOCK_DGRAM : SOCK_STREAM,
			      is_udp ? IPPROTO_UDP : IPPROTO_TCP,
			      context4);
	if (ret < 0) {
		printk("[%s] Cannot get IPv4 network context (%d)\n",
		       argv0, ret);
		return -1;
	}

	ipv4->sin_port = htons(port);
	ipv4->sin_family = AF_INET;
#endif

	if (family == AF_INET6 && *context6) {
		ret = net_context_bind(*context6,
				       (struct sockaddr *)ipv6,
				       sizeof(struct sockaddr_in6));
		if (ret < 0) {
			printk("[%s] Cannot bind IPv6 port %d (%d)",
			       argv0, ntohs(ipv6->sin6_port), ret);
			return -1;
		}
	}

	if (family == AF_INET && *context4) {
		ret = net_context_bind(*context4,
				       (struct sockaddr *)ipv4,
				       sizeof(struct sockaddr_in));
		if (ret < 0) {
			printk("[%s] Cannot bind IPv4 port %d (%d)",
			       argv0, ntohs(ipv4->sin_port), ret);
			return -1;
		}
	}

	if (!(*context6) && !(*context4)) {
		printk("[%s] ERROR! Fail to retrieve network context(s)\n",
		       argv0);
		return -1;
	}

	return 0;
}

static int execute_upload(struct net_context *context6,
			  struct net_context *context4,
			  sa_family_t family,
			  struct sockaddr_in6 *ipv6,
			  struct sockaddr_in *ipv4,
			  bool is_udp,
			  char *argv0,
			  unsigned int duration_in_ms,
			  unsigned int packet_size,
			  unsigned int rate_in_kbps)
{
	struct zperf_results results = { };
	int ret;

	printk("[%s] duration:\t\t", argv0);
	print_number(duration_in_ms * USEC_PER_MSEC, TIME_US, TIME_US_UNIT);
	printk("\n");
	printk("[%s] packet size:\t%u bytes\n", argv0, packet_size);
	printk("[%s] start...\n", argv0);

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6 && context6) {
		/* For IPv6, we should make sure that neighbor discovery
		 * has been done for the peer. So send ping here, wait
		 * some time and start the test after that.
		 */
		net_icmpv6_send_echo_request(net_if_get_default(),
					     &ipv6->sin6_addr, 0, 0);

		k_sleep(1 * MSEC_PER_SEC);
	}
#endif

	if (is_udp) {
#if defined(CONFIG_NET_UDP)
		printk("[%s] rate:\t\t", argv0);
		print_number(rate_in_kbps, KBPS, KBPS_UNIT);
		printk("\n");

		if (family == AF_INET6 && context6) {
			ret = net_context_connect(context6,
						  (struct sockaddr *)ipv6,
						  sizeof(*ipv6),
						  NULL,
						  K_NO_WAIT,
						  NULL);
			if (ret < 0) {
				printk("[%s] IPv6 connect failed (%d)\n",
				       argv0, ret);
				goto out;
			}

			zperf_udp_upload(context6, duration_in_ms, packet_size,
					 rate_in_kbps, &results);
			shell_udp_upload_print_stats(&results);
		}

		if (family == AF_INET && context4) {
			ret = net_context_connect(context4,
						  (struct sockaddr *)ipv4,
						  sizeof(*ipv4),
						  NULL,
						  K_NO_WAIT,
						  NULL);
			if (ret < 0) {
				printk("[%s] IPv4 connect failed (%d)\n",
				       argv0, ret);
				goto out;
			}

			zperf_udp_upload(context4, duration_in_ms, packet_size,
					 rate_in_kbps, &results);
			shell_udp_upload_print_stats(&results);
		}
#else
		printk("[%s] UDP not supported\n", argv0);
#endif
	} else {
#if defined(CONFIG_NET_TCP)
		if (family == AF_INET6 && context6) {
			ret = net_context_connect(context6,
						  (struct sockaddr *)ipv6,
						  sizeof(*ipv6),
						  NULL,
						  WAIT_CONNECT,
						  NULL);
			if (ret < 0) {
				printk("[%s] IPv6 connect failed (%d)\n",
				       argv0, ret);
				goto out;
			}

			/* We either upload using IPv4 or IPv6, not both at
			 * the same time.
			 */
			net_context_put(context4);

			zperf_tcp_upload(context6, duration_in_ms,
					 packet_size, &results);

			shell_tcp_upload_print_stats(&results);

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
				printk("[%s] IPv4 connect failed (%d)\n",
				       argv0, ret);
				goto out;
			}

			net_context_put(context6);

			zperf_tcp_upload(context4, duration_in_ms,
					 packet_size, &results);

			shell_tcp_upload_print_stats(&results);

			return 0;
		}
#else
		printk("[%s] TCP not supported\n", argv0);
#endif
	}

out:
	net_context_put(context6);
	net_context_put(context4);

	return 0;
}

static int shell_cmd_upload(int argc, char *argv[])
{
	struct sockaddr_in6 ipv6 = { .sin6_family = AF_INET6 };
	struct sockaddr_in ipv4 = { .sin_family = AF_INET };
	struct net_context *context6 = NULL, *context4 = NULL;
	sa_family_t family = AF_UNSPEC;
	unsigned int duration_in_ms, packet_size, rate_in_kbps;
	u16_t port;
	bool is_udp;
	int start = 0;

	if (!strcmp(argv[0], "zperf")) {
		start++;
		argc--;
	}

	is_udp = !strcmp(argv[start], CMD_STR_UDP_UPLOAD) ? 1 : 0;

	if (argc == 1) {
		if (is_udp) {
#if defined(CONFIG_NET_UDP)
			shell_udp_upload_usage();
#endif
		} else {
#if defined(CONFIG_NET_TCP)
			shell_tcp_upload_usage();
#endif
		}

		return -1;
	}

#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
	if (parse_ipv6_addr(argv[start + 1], argv[start + 2],
			    &ipv6, argv[start]) < 0) {
		printk("[%s] ERROR! Please specify the IP address of the "
			"remote server\n",  argv[start]);
		return -1;
	}

	printk("[%s] Connecting to %s\n", argv[start],
	       net_sprint_ipv6_addr(&ipv6.sin6_addr));

	family = AF_INET6;
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	if (parse_ipv4_addr(argv[start + 1], argv[start + 2],
			    &ipv4, argv[start]) < 0) {
		printk("[%s] ERROR! Please specify the IP address of the "
		       "remote server\n",  argv[start]);
		return -1;
	}

	printk("[%s] Connecting to %s\n", argv[start],
	       net_sprint_ipv4_addr(&ipv4.sin_addr));

	family = AF_INET;
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	if (parse_ipv6_addr(argv[start + 1], argv[start + 2],
			    &ipv6, argv[start]) < 0) {
		if (parse_ipv4_addr(argv[start + 1], argv[start + 2],
				    &ipv4, argv[start]) < 0) {
			printk("[%s] ERROR! Please specify the IP address "
			       "of the remote server\n",  argv[start]);
			return -1;
		}

		printk("[%s] Connecting to %s\n", argv[start],
		       net_sprint_ipv4_addr(&ipv4.sin_addr));

		family = AF_INET;
	} else {
		printk("[%s] Connecting to %s\n", argv[start],
		       net_sprint_ipv6_addr(&ipv6.sin6_addr));

		family = AF_INET6;
	}
#endif

	if (argc > 2) {
		port = strtoul(argv[start + 2], NULL, 10);
		printk("[%s] Remote port is %u\n", argv[start], port);
	} else {
		port = DEF_PORT;
	}

	if (setup_contexts(&context6, &context4, family, &in6_addr_my,
			   &in4_addr_my, port, is_udp, argv[start]) < 0) {
		return -1;
	}

	if (argc > 3) {
		duration_in_ms = strtoul(argv[start + 3], NULL, 10) *
			MSEC_PER_SEC;
	} else {
		duration_in_ms = 1000;
	}

	if (argc > 4) {
		packet_size = parse_number(argv[start + 4], K, K_UNIT);
	} else {
		packet_size = 256;
	}

	if (argc > 5) {
		rate_in_kbps =
			(parse_number(argv[start + 5], K, K_UNIT) +
			 1023) / 1024;
	} else {
		rate_in_kbps = 10;
	}

	return execute_upload(context6, context4, family, &ipv6, &ipv4,
			      is_udp, argv[start], duration_in_ms,
			      packet_size, rate_in_kbps);
}

static int shell_cmd_upload2(int argc, char *argv[])
{
	struct net_context *context6 = NULL, *context4 = NULL;
	u16_t port = DEF_PORT;
	unsigned int duration_in_ms, packet_size, rate_in_kbps;
	sa_family_t family;
	u8_t is_udp;
	int start = 0;

	if (!strcmp(argv[0], "zperf")) {
		start++;
		argc--;
	}

	is_udp = !strcmp(argv[start], CMD_STR_UDP_UPLOAD2) ? 1 : 0;

	if (argc == 1) {
		if (is_udp) {
#if defined(CONFIG_NET_UDP)
			shell_udp_upload2_usage();
#endif
		} else {
#if defined(CONFIG_NET_TCP)
			shell_tcp_upload2_usage();
#endif
		}

		return -1;
	}

	family = !strcmp(argv[start + 1], "v4") ? AF_INET : AF_INET6;

#if defined(CONFIG_NET_IPV6)
	in6_addr_my.sin6_port = htons(port);
#endif

#if defined(CONFIG_NET_IPV4)
	in4_addr_my.sin_port = htons(port);
#endif

	if (family == AF_INET6) {
		if (net_is_ipv6_addr_unspecified(&in6_addr_my.sin6_addr)) {
			printk("[%s] Invalid local IPv6 address\n",
			       argv[start]);
			return -1;
		}

		if (net_is_ipv6_addr_unspecified(&in6_addr_dst.sin6_addr)) {
			printk("[%s] Invalid destination IPv6 address\n",
			       argv[start]);
			return -1;
		}

		printk("[%s] Connecting to %s\n", argv[start],
		       net_sprint_ipv6_addr(&in6_addr_dst.sin6_addr));
	} else {
		if (net_is_ipv4_addr_unspecified(&in4_addr_my.sin_addr)) {
			printk("[%s] Invalid local IPv4 address\n",
			       argv[start]);
			return -1;
		}

		if (net_is_ipv4_addr_unspecified(&in4_addr_dst.sin_addr)) {
			printk("[%s] Invalid destination IPv4 address\n",
			       argv[start]);
			return -1;
		}

		printk("[%s] Connecting to %s\n", argv[start],
		       net_sprint_ipv4_addr(&in4_addr_dst.sin_addr));
	}

	if (setup_contexts(&context6, &context4, family, &in6_addr_my,
			   &in4_addr_my, port, is_udp, argv[start]) < 0) {
		return -1;
	}

	if (argc > 1) {
		duration_in_ms = strtoul(argv[start + 2], NULL, 10) *
			MSEC_PER_SEC;
	} else {
		duration_in_ms = 1000;
	}

	if (argc > 2) {
		packet_size = parse_number(argv[start + 3], K, K_UNIT);
	} else {
		packet_size = 256;
	}

	if (argc > 3) {
		rate_in_kbps =
			(parse_number(argv[start + 4], K, K_UNIT) + 1023) /
			1024;
	} else {
		rate_in_kbps = 10;
	}

	return execute_upload(context6, context4, family, &in6_addr_dst,
			      &in4_addr_dst, is_udp, argv[start],
			      duration_in_ms, packet_size, rate_in_kbps);
}

static int shell_cmd_connectap(int argc, char *argv[])
{
	printk("[%s] Zephyr has not been built with Wi-Fi support.\n",
		CMD_STR_CONNECTAP);

	return 0;
}

#if defined(CONFIG_NET_TCP)
static int shell_cmd_tcp_download(int argc, char *argv[])
{
	static bool tcp_stopped = true;
	int port;

	if (argc == 1) {
		/* Print usage */
		printk("\n[%s]:\n", CMD_STR_TCP_DOWNLOAD);
		printk("Usage:\t%s <port>\n", CMD_STR_TCP_DOWNLOAD);
		printk("\nExample %s 5001\n", CMD_STR_TCP_DOWNLOAD);
		return -1;
	}

	if (argc > 1) {
		port = strtoul(argv[1], NULL, 10);
	} else {
		port = DEF_PORT;
	}

	if (!tcp_stopped) {
		printk("[%s] ERROR! TCP server already started!\n",
			CMD_STR_TCP_DOWNLOAD);
		return -1;
	}

	zperf_tcp_receiver_init(port);

	tcp_stopped = false;

	printk("[%s] TCP server started on port %u\n", CMD_STR_TCP_DOWNLOAD,
	       port);

	return 0;
}
#endif

static int shell_cmd_version(int argc, char *argv[])
{
	printk("\nzperf [%s]: %s config: %s\n", CMD_STR_VERSION, VERSION,
	       CONFIG);

	return 0;
}

static void zperf_init(void)
{
#if defined(MY_IP6ADDR) || defined(MY_IP4ADDR)
	int ret;

	printk("\n");
#endif

#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR)
	if (zperf_get_ipv6_addr(MY_IP6ADDR, MY_PREFIX_LEN_STR,
				&ipv6, __func__) < 0) {
		printk("[%s] ERROR! Unable to set IP\n", __func__);
	} else {
		printk("[%s] Setting IP address %s\n", __func__,
		       net_sprint_ipv6_addr(&ipv6));

		net_ipaddr_copy(&in6_addr_my.sin6_addr, &ipv6);
	}

	ret = net_addr_pton(AF_INET6, DST_IP6ADDR,
			    &in6_addr_dst.sin6_addr);
	if (ret < 0) {
		printk("[%s] ERROR! Unable to set IP %s\n", __func__,
			DST_IP6ADDR);
	} else {
		printk("[%s] Setting destination IP address %s\n", __func__,
		       net_sprint_ipv6_addr(&in6_addr_dst.sin6_addr));
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR)
	if (zperf_get_ipv4_addr(MY_IP4ADDR, &ipv4, __func__) < 0) {
		printk("[%s] ERROR! Unable to set IP\n", __func__);
	} else {
		printk("[%s] Setting IP address %s\n", __func__,
		       net_sprint_ipv4_addr(&ipv4));

		net_ipaddr_copy(&in4_addr_my.sin_addr, &ipv4);
	}

	ret = net_addr_pton(AF_INET, DST_IP4ADDR,
			    &in4_addr_dst.sin_addr);
	if (ret < 0) {
		printk("[%s] ERROR! Unable to set IP %s\n", __func__,
			DST_IP4ADDR);
	} else {
		printk("[%s] Setting destination IP address %s\n", __func__,
		       net_sprint_ipv4_addr(&in4_addr_dst.sin_addr));
	}
#endif

	zperf_session_init();
}

#define MY_SHELL_MODULE "zperf"
struct shell_cmd commands[] = {
	{ CMD_STR_SETIP, shell_cmd_setip },
	{ CMD_STR_CONNECTAP, shell_cmd_connectap },
	{ CMD_STR_VERSION, shell_cmd_version },
#if defined(CONFIG_NET_UDP)
	{ CMD_STR_UDP_UPLOAD, shell_cmd_upload },
	/* Same as upload command but no need to specify the addresses */
	{ CMD_STR_UDP_UPLOAD2, shell_cmd_upload2 },
	{ CMD_STR_UDP_DOWNLOAD, shell_cmd_udp_download },
#endif
#if defined(CONFIG_NET_TCP)
	{ CMD_STR_TCP_UPLOAD, shell_cmd_upload },
	{ CMD_STR_TCP_UPLOAD2, shell_cmd_upload2 },
	{ CMD_STR_TCP_DOWNLOAD, shell_cmd_tcp_download },
#endif
#if defined(PROFILER)
	PROF_CMD,
#endif
	{ NULL, NULL }
};

void main(void)
{
	shell_cmd_version(0, NULL);
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
	shell_register_default_module(MY_SHELL_MODULE);

	zperf_init();

#if PROFILER
	while (1) {
		k_sleep(5 * MSEC_PER_SEC);
		prof_flush();
	}
#else
	k_sleep(K_FOREVER);
#endif
}
