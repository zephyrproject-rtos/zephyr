/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"
#include "zperf_session.h"

/* Get some useful debug routings from net_private.h, requires
 * that NET_LOG_ENABLED is set.
 */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "ipv6.h" /* to get infinite lifetime */


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

static struct sockaddr_in6 in6_addr_my = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(MY_SRC_PORT),
};

static struct sockaddr_in6 in6_addr_dst = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(DEF_PORT),
};

static struct sockaddr_in in4_addr_dst = {
	.sin_family = AF_INET,
	.sin_port = htons(DEF_PORT),
};

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(MY_SRC_PORT),
};

static struct in6_addr shell_ipv6;

static struct in_addr shell_ipv4;

#define DEVICE_NAME "zperf shell"

const uint32_t TIME_US[] = { 60 * 1000 * 1000, 1000 * 1000, 1000, 0 };
const char *TIME_US_UNIT[] = { "m", "s", "ms", "us" };
const uint32_t KBPS[] = { 1000, 0 };
const char *KBPS_UNIT[] = { "Mbps", "Kbps" };
const uint32_t K[] = { 1000 * 1000, 1000, 0 };
const char *K_UNIT[] = { "M", "K", "" };

static void print_number(const struct shell *sh, uint32_t value,
			 const uint32_t *divisor_arr, const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	uint32_t dec, radix;

	unit = units;
	divisor = divisor_arr;

	while (value < *divisor) {
		divisor++;
		unit++;
	}

	if (*divisor != 0U) {
		radix = value / *divisor;
		dec = (value % *divisor) * 100U / *divisor;
		shell_fprintf(sh, SHELL_NORMAL, "%u.%s%u %s", radix,
			      (dec < 10) ? "0" : "", dec, *unit);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%u %s", value, *unit);
	}
}

static void print_number_64(const struct shell *sh, uint64_t value,
			 const uint32_t *divisor_arr, const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	uint32_t dec;
	uint64_t radix;

	unit = units;
	divisor = divisor_arr;

	while (value < *divisor) {
		divisor++;
		unit++;
	}

	if (*divisor != 0U) {
		radix = value / *divisor;
		dec = (value % *divisor) * 100U / *divisor;
		shell_fprintf(sh, SHELL_NORMAL, "%llu.%s%u %s", radix,
			      (dec < 10) ? "0" : "", dec, *unit);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%llu %s", value, *unit);
	}
}

static long parse_number(const char *string, const uint32_t *divisor_arr,
			 const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	char *suffix;
	long dec;
	int cmp;

	dec = strtoul(string, &suffix, 10);
	unit = units;
	divisor = divisor_arr;

	do {
		cmp = strncasecmp(suffix, *unit++, 1);
	} while (cmp != 0 && *++divisor != 0U);

	return (*divisor == 0U) ? dec : dec * *divisor;
}

static int parse_ipv6_addr(const struct shell *sh, char *host, char *port,
			   struct sockaddr_in6 *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET6, host, &addr->sin6_addr);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Invalid IPv6 address %s\n", host);
		return -EINVAL;
	}

	addr->sin6_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin6_port) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Invalid port %s\n", port);
		return -EINVAL;
	}

	return 0;
}

static int parse_ipv4_addr(const struct shell *sh, char *host, char *port,
			   struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Invalid IPv4 address %s\n", host);
		return -EINVAL;
	}

	addr->sin_port = htons(strtoul(port, NULL, 10));
	if (!addr->sin_port) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Invalid port %s\n", port);
		return -EINVAL;
	}

	return 0;
}

static int zperf_bind_host(const struct shell *sh,
			   size_t argc, char *argv[],
			   struct zperf_download_params *param)
{
	int ret;

	/* Parse options */
	if (argc >= 2) {
		param->port = strtoul(argv[1], NULL, 10);
	} else {
		param->port = DEF_PORT;
	}

	if (argc >= 3) {
		char *addr_str = argv[2];
		struct sockaddr addr;

		memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot parse address \"%s\"\n",
				      addr_str);
			return ret;
		}

		memcpy(&param->addr, &addr, sizeof(struct sockaddr));
	}

	return 0;
}

static int cmd_setip(const struct shell *sh, size_t argc, char *argv[])
{
	int start = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		if (argc != 3) {
			shell_help(sh);
			return -ENOEXEC;
		}

		if (zperf_get_ipv6_addr(argv[start + 1], argv[start + 2],
					&shell_ipv6) < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to set %s address (%s disabled)\n", "IPv6", "IPv4");
			return 0;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Setting IP address %s\n",
			      net_sprint_ipv6_addr(&shell_ipv6));
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		if (argc != 2) {
			shell_help(sh);
			return -ENOEXEC;
		}

		if (zperf_get_ipv4_addr(argv[start + 1], &shell_ipv4) < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to set %s address (%s disabled)\n", "IPv4", "IPv6");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Setting IP address %s\n",
			      net_sprint_ipv4_addr(&shell_ipv4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		if (net_addr_pton(AF_INET6, argv[start + 1], &shell_ipv6) < 0) {
			if (argc != 2) {
				shell_help(sh);
				return -ENOEXEC;
			}

			if (zperf_get_ipv4_addr(argv[start + 1],
						&shell_ipv4) < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to set %s address\n", "IPv4");
				return -ENOEXEC;
			}

			shell_fprintf(sh, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv4_addr(&shell_ipv4));
		} else {
			if (argc != 3) {
				shell_help(sh);
				return -ENOEXEC;
			}

			if (zperf_get_ipv6_addr(argv[start + 1],
						argv[start + 2], &shell_ipv6) < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to set %s address\n", "IPv6");
				return -ENOEXEC;
			}

			shell_fprintf(sh, SHELL_NORMAL,
				      "Setting IP address %s\n",
				      net_sprint_ipv6_addr(&shell_ipv6));
		}
	}

	return 0;
}

static void udp_session_cb(enum zperf_status status,
			   struct zperf_results *result,
			   void *user_data)
{
	const struct shell *sh = user_data;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		shell_fprintf(sh, SHELL_NORMAL, "New session started.\n");
		break;

	case ZPERF_SESSION_FINISHED: {
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				((result->total_len * 8ULL * USEC_PER_SEC) /
				 (result->time_in_us * 1000ULL));
		} else {
			rate_in_kbps = 0U;
		}

		shell_fprintf(sh, SHELL_NORMAL, "End of session!\n");

		shell_fprintf(sh, SHELL_NORMAL, " duration:\t\t");
		print_number_64(sh, result->time_in_us, TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, " received packets:\t%u\n",
			      result->nb_packets_rcvd);
		shell_fprintf(sh, SHELL_NORMAL, " nb packets lost:\t%u\n",
			      result->nb_packets_lost);
		shell_fprintf(sh, SHELL_NORMAL, " nb packets outorder:\t%u\n",
			      result->nb_packets_outorder);

		shell_fprintf(sh, SHELL_NORMAL, " jitter:\t\t\t");
		print_number(sh, result->jitter_in_us, TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, " rate:\t\t\t");
		print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		break;
	}

	case ZPERF_SESSION_ERROR:
		shell_fprintf(sh, SHELL_ERROR, "UDP session error.\n");
		break;
	}
}

/*
 * parse download options with '-'
 * return < 0 if parse error
 * return 0 if no '-' options
 * return > 0 num of argc we parsed
 * and following parse starts from this num
 */
static int shell_cmd_download(const struct shell *sh, size_t argc,
			      char *argv[],
			      struct zperf_download_params *param)
{
	int opt_cnt = 0;
	size_t i;

	for (i = 1; i < argc; ++i) {
		if (*argv[i] != '-') {
			break;
		}

		switch (argv[i][1]) {
		case 'I':
			/*
			 * IFNAMSIZ by default CONFIG_NET_INTERFACE_NAME_LEN
			 * is at least 1 so no overflow risk here
			 */
			i++;
			if (i >= argc) {
				shell_fprintf(sh, SHELL_WARNING,
					      "-I <interface name>\n");
				return -ENOEXEC;
			}
			(void)memset(param->if_name, 0x0, IFNAMSIZ);
			strncpy(param->if_name, argv[i], IFNAMSIZ - 1);

			opt_cnt += 2;
			break;

		default:
			shell_fprintf(sh, SHELL_WARNING,
				      "Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	return opt_cnt;
}

static int cmd_udp_download_stop(const struct shell *sh, size_t argc,
				 char *argv[])
{
	int ret;

	ret = zperf_udp_download_stop();
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "UDP server not running!\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "UDP server stopped\n");

	return 0;
}

static int cmd_udp_download(const struct shell *sh, size_t argc,
			    char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		struct zperf_download_params param = { 0 };
		int ret;
		int start;

		start = shell_cmd_download(sh, argc, argv, &param);
		if (start < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to parse option.\n");
			return -ENOEXEC;
		}

		ret = zperf_bind_host(sh, argc - start, &argv[start], &param);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to bind host.\n");
			shell_help(sh);
			return -ENOEXEC;
		}

		ret = zperf_udp_download(&param, udp_session_cb, (void *)sh);
		if (ret == -EALREADY) {
			shell_fprintf(sh, SHELL_WARNING,
				      "UDP server already started!\n");
			return -ENOEXEC;
		} else if (ret < 0) {
			shell_fprintf(sh, SHELL_ERROR,
				      "Failed to start UDP server!\n");
			return -ENOEXEC;
		}

		k_yield();

		shell_fprintf(sh, SHELL_NORMAL,
			      "UDP server started on port %u\n", param.port);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static void shell_udp_upload_print_stats(const struct shell *sh,
					 struct zperf_results *results)
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		unsigned int rate_in_kbps, client_rate_in_kbps;

		shell_fprintf(sh, SHELL_NORMAL, "-\nUpload completed!\n");

		if (results->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				((results->total_len * 8 * USEC_PER_SEC) /
				 (results->time_in_us * 1000U));
		} else {
			rate_in_kbps = 0U;
		}

		if (results->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)results->nb_packets_sent *
				  (uint64_t)results->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 (results->client_time_in_us * 1000U));
		} else {
			client_rate_in_kbps = 0U;
		}

		if (!rate_in_kbps) {
			shell_fprintf(sh, SHELL_ERROR,
				      "LAST PACKET NOT RECEIVED!!!\n");
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Statistics:\t\tserver\t(client)\n");
		shell_fprintf(sh, SHELL_NORMAL, "Duration:\t\t");
		print_number_64(sh, results->time_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\t(");
		print_number_64(sh, results->client_time_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, ")\n");

		shell_fprintf(sh, SHELL_NORMAL, "Num packets:\t\t%u\t(%u)\n",
			      results->nb_packets_rcvd,
			      results->nb_packets_sent);

		shell_fprintf(sh, SHELL_NORMAL,
			      "Num packets out order:\t%u\n",
			      results->nb_packets_outorder);
		shell_fprintf(sh, SHELL_NORMAL, "Num packets lost:\t%u\n",
			      results->nb_packets_lost);

		shell_fprintf(sh, SHELL_NORMAL, "Jitter:\t\t\t");
		print_number(sh, results->jitter_in_us, TIME_US,
			     TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t\t");
		print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\t(");
		print_number(sh, client_rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, ")\n");
	}
}

static void shell_tcp_upload_print_stats(const struct shell *sh,
					 struct zperf_results *results)
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		unsigned int client_rate_in_kbps;

		shell_fprintf(sh, SHELL_NORMAL, "-\nUpload completed!\n");

		if (results->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)results->nb_packets_sent *
				  (uint64_t)results->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 (results->client_time_in_us * 1000U));
		} else {
			client_rate_in_kbps = 0U;
		}

		shell_fprintf(sh, SHELL_NORMAL, "Duration:\t");
		print_number_64(sh, results->client_time_in_us,
			     TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");
		shell_fprintf(sh, SHELL_NORMAL, "Num packets:\t%u\n",
			      results->nb_packets_sent);
		shell_fprintf(sh, SHELL_NORMAL,
			      "Num errors:\t%u (retry or fail)\n",
			      results->nb_packets_errors);
		shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t");
		print_number(sh, client_rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");
	}
}

static void udp_upload_cb(enum zperf_status status,
			  struct zperf_results *result,
			  void *user_data)
{
	const struct shell *sh = user_data;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		break;

	case ZPERF_SESSION_FINISHED: {
		shell_udp_upload_print_stats(sh, result);
		break;
	}

	case ZPERF_SESSION_ERROR:
		shell_fprintf(sh, SHELL_ERROR, "UDP upload failed\n");
		break;
	}
}

static void tcp_upload_cb(enum zperf_status status,
			  struct zperf_results *result,
			  void *user_data)
{
	const struct shell *sh = user_data;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		break;

	case ZPERF_SESSION_FINISHED: {
		shell_tcp_upload_print_stats(sh, result);
		break;
	}

	case ZPERF_SESSION_ERROR:
		shell_fprintf(sh, SHELL_ERROR, "TCP upload failed\n");
		break;
	}
}

static int ping_handler(struct net_icmp_ctx *ctx,
			struct net_pkt *pkt,
			struct net_icmp_ip_hdr *ip_hdr,
			struct net_icmp_hdr *icmp_hdr,
			void *user_data)
{
	struct k_sem *sem_wait = user_data;

	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(icmp_hdr);

	k_sem_give(sem_wait);

	return 0;
}

static void send_ping(const struct shell *sh,
		      struct in6_addr *addr,
		      int timeout_ms)
{
	static struct k_sem sem_wait;
	struct sockaddr_in6 dest_addr = { 0 };
	struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_ECHO_REPLY, 0, ping_handler);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "Cannot send ping (%d)\n", ret);
		return;
	}

	memcpy(&dest_addr.sin6_addr, addr, sizeof(struct in6_addr));

	k_sem_init(&sem_wait, 0, 1);

	(void)net_icmp_send_echo_request(&ctx,
					 net_if_get_default(),
					 (struct sockaddr *)&dest_addr,
					 NULL, &sem_wait);

	ret = k_sem_take(&sem_wait, K_MSEC(timeout_ms));
	if (ret == -EAGAIN) {
		shell_fprintf(sh, SHELL_WARNING, "ping %s timeout\n",
			      net_sprint_ipv6_addr(addr));
	}

	(void)net_icmp_cleanup_ctx(&ctx);
}

static int execute_upload(const struct shell *sh,
			  const struct zperf_upload_params *param,
			  bool is_udp, bool async)
{
	struct zperf_results results = { 0 };
	int ret;

	shell_fprintf(sh, SHELL_NORMAL, "Duration:\t");
	print_number_64(sh, (uint64_t)param->duration_ms * USEC_PER_MSEC, TIME_US,
		     TIME_US_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\n");
	shell_fprintf(sh, SHELL_NORMAL, "Packet size:\t%u bytes\n",
		      param->packet_size);
	shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t%u kbps\n",
		      param->rate_kbps);
	shell_fprintf(sh, SHELL_NORMAL, "Starting...\n");

	if (IS_ENABLED(CONFIG_NET_IPV6) && param->peer_addr.sa_family == AF_INET6) {
		struct sockaddr_in6 *ipv6 =
				(struct sockaddr_in6 *)&param->peer_addr;
		/* For IPv6, we should make sure that neighbor discovery
		 * has been done for the peer. So send ping here, wait
		 * some time and start the test after that.
		 */
		send_ping(sh, &ipv6->sin6_addr, MSEC_PER_SEC);
	}

	if (is_udp && IS_ENABLED(CONFIG_NET_UDP)) {
		uint32_t packet_duration =
			zperf_packet_duration(param->packet_size, param->rate_kbps);

		shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t");
		print_number(sh, param->rate_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		if (packet_duration > 1000U) {
			shell_fprintf(sh, SHELL_NORMAL, "Packet duration %u ms\n",
				      (unsigned int)(packet_duration / 1000U));
		} else {
			shell_fprintf(sh, SHELL_NORMAL, "Packet duration %u us\n",
				      (unsigned int)packet_duration);
		}

		if (async) {
			ret = zperf_udp_upload_async(param, udp_upload_cb,
						     (void *)sh);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_ERROR,
					"Failed to start UDP async upload (%d)\n", ret);
				return ret;
			}
		} else {
			ret = zperf_udp_upload(param, &results);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_ERROR,
					"UDP upload failed (%d)\n", ret);
				return ret;
			}

			shell_udp_upload_print_stats(sh, &results);
		}
	} else {
		if (!IS_ENABLED(CONFIG_NET_UDP)) {
			shell_fprintf(sh, SHELL_INFO,
				      "UDP not supported\n");
		}
	}

	if (!is_udp && IS_ENABLED(CONFIG_NET_TCP)) {
		if (async) {
			ret = zperf_tcp_upload_async(param, tcp_upload_cb,
						     (void *)sh);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_ERROR,
					"Failed to start TCP async upload (%d)\n", ret);
				return ret;
			}
		} else {
			ret = zperf_tcp_upload(param, &results);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_ERROR,
					"TCP upload failed (%d)\n", ret);
				return ret;
			}

			shell_tcp_upload_print_stats(sh, &results);
		}
	} else {
		if (!IS_ENABLED(CONFIG_NET_TCP)) {
			shell_fprintf(sh, SHELL_INFO,
				      "TCP not supported\n");
		}
	}

	return 0;
}

static int parse_arg(size_t *i, size_t argc, char *argv[])
{
	int res = -1;
	const char *str = argv[*i] + 2;
	char *endptr;

	if (*str == 0) {
		if (*i + 1 >= argc) {
			return -1;
		}

		*i += 1;
		str = argv[*i];
	}

	errno = 0;
	if (strncmp(str, "0x", 2) == 0) {
		res = strtol(str, &endptr, 16);
	} else {
		res = strtol(str, &endptr, 10);
	}

	if (errno || (endptr == str)) {
		return -1;
	}

	return res;
}

static int shell_cmd_upload(const struct shell *sh, size_t argc,
			     char *argv[], enum net_ip_protocol proto)
{
	struct zperf_upload_params param = { 0 };
	struct sockaddr_in6 ipv6 = { .sin6_family = AF_INET6 };
	struct sockaddr_in ipv4 = { .sin_family = AF_INET };
	char *port_str;
	bool async = false;
	bool is_udp;
	int start = 0;
	size_t opt_cnt = 0;

	param.options.priority = -1;
	is_udp = proto == IPPROTO_UDP;

	/* Parse options */
	for (size_t i = 1; i < argc; ++i) {
		if (*argv[i] != '-') {
			break;
		}

		switch (argv[i][1]) {
		case 'S': {
			int tos = parse_arg(&i, argc, argv);

			if (tos < 0 || tos > UINT8_MAX) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			param.options.tos = tos;
			opt_cnt += 2;
			break;
		}

		case 'a':
			async = true;
			opt_cnt += 1;
			break;

		case 'n':
			if (is_udp) {
				shell_fprintf(sh, SHELL_WARNING,
					      "UDP does not support -n option\n");
				return -ENOEXEC;
			}
			param.options.tcp_nodelay = 1;
			opt_cnt += 1;
			break;

#ifdef CONFIG_NET_CONTEXT_PRIORITY
		case 'p':
			param.options.priority = parse_arg(&i, argc, argv);
			if (param.options.priority < 0 ||
			    param.options.priority > UINT8_MAX) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			opt_cnt += 2;
			break;
#endif /* CONFIG_NET_CONTEXT_PRIORITY */

		case 'I':
			i++;
			if (i >= argc) {
				shell_fprintf(sh, SHELL_WARNING,
					      "-I <interface name>\n");
				return -ENOEXEC;
			}
			(void)memset(param.if_name, 0x0, IFNAMSIZ);
			strncpy(param.if_name, argv[i], IFNAMSIZ - 1);

			opt_cnt += 2;
			break;

		default:
			shell_fprintf(sh, SHELL_WARNING,
				      "Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	start += opt_cnt;
	argc -= opt_cnt;

	if (argc < 2) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Not enough parameters.\n");

		if (is_udp) {
			if (IS_ENABLED(CONFIG_NET_UDP)) {
				shell_help(sh);
				return -ENOEXEC;
			}
		} else {
			if (IS_ENABLED(CONFIG_NET_TCP)) {
				shell_help(sh);
				return -ENOEXEC;
			}
		}

		return -ENOEXEC;
	}

	if (argc > 2) {
		port_str = argv[start + 2];
		shell_fprintf(sh, SHELL_NORMAL,
			      "Remote port is %s\n", port_str);
	} else {
		port_str = DEF_PORT_STR;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		if (parse_ipv6_addr(sh, argv[start + 1], port_str,
				    &ipv6) < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Please specify the IP address of the "
				      "remote server.\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_WARNING, "Connecting to %s\n",
			      net_sprint_ipv6_addr(&ipv6.sin6_addr));

		memcpy(&param.peer_addr, &ipv6, sizeof(ipv6));
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		if (parse_ipv4_addr(sh, argv[start + 1], port_str,
				    &ipv4) < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Please specify the IP address of the "
				      "remote server.\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL, "Connecting to %s\n",
			      net_sprint_ipv4_addr(&ipv4.sin_addr));

		memcpy(&param.peer_addr, &ipv4, sizeof(ipv4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		if (parse_ipv6_addr(sh, argv[start + 1], port_str,
				    &ipv6) < 0) {
			if (parse_ipv4_addr(sh, argv[start + 1], port_str,
					    &ipv4) < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Please specify the IP address "
					      "of the remote server.\n");
				return -ENOEXEC;
			}

			shell_fprintf(sh, SHELL_NORMAL,
				      "Connecting to %s\n",
				      net_sprint_ipv4_addr(&ipv4.sin_addr));

			memcpy(&param.peer_addr, &ipv4, sizeof(ipv4));
		} else {
			shell_fprintf(sh, SHELL_NORMAL,
				      "Connecting to %s\n",
				      net_sprint_ipv6_addr(&ipv6.sin6_addr));

			memcpy(&param.peer_addr, &ipv6, sizeof(ipv6));
		}
	}

	if (argc > 3) {
		param.duration_ms = MSEC_PER_SEC * strtoul(argv[start + 3],
							   NULL, 10);
	} else {
		param.duration_ms = MSEC_PER_SEC * 1;
	}

	if (argc > 4) {
		param.packet_size = parse_number(argv[start + 4], K, K_UNIT);
	} else {
		param.packet_size = 256U;
	}

	if (argc > 5) {
		param.rate_kbps =
			(parse_number(argv[start + 5], K, K_UNIT) + 999) / 1000;
	} else {
		param.rate_kbps = 10U;
	}

	return execute_upload(sh, &param, is_udp, async);
}

static int cmd_tcp_upload(const struct shell *sh, size_t argc, char *argv[])
{
	return shell_cmd_upload(sh, argc, argv, IPPROTO_TCP);
}

static int cmd_udp_upload(const struct shell *sh, size_t argc, char *argv[])
{
	return shell_cmd_upload(sh, argc, argv, IPPROTO_UDP);
}

static int shell_cmd_upload2(const struct shell *sh, size_t argc,
			     char *argv[], enum net_ip_protocol proto)
{
	struct zperf_upload_params param = { 0 };
	sa_family_t family;
	uint8_t is_udp;
	bool async = false;
	int start = 0;
	size_t opt_cnt = 0;

	is_udp = proto == IPPROTO_UDP;

	/* Parse options */
	for (size_t i = 1; i < argc; ++i) {
		if (*argv[i] != '-') {
			break;
		}

		switch (argv[i][1]) {
		case 'S': {
			int tos = parse_arg(&i, argc, argv);

			if (tos < 0 || tos > UINT8_MAX) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			param.options.tos = tos;
			opt_cnt += 2;
			break;
		}

		case 'a':
			async = true;
			opt_cnt += 1;
			break;

		case 'n':
			if (is_udp) {
				shell_fprintf(sh, SHELL_WARNING,
					      "UDP does not support -n option\n");
				return -ENOEXEC;
			}
			param.options.tcp_nodelay = 1;
			opt_cnt += 1;
			break;

#ifdef CONFIG_NET_CONTEXT_PRIORITY
		case 'p':
			param.options.priority = parse_arg(&i, argc, argv);
			if (param.options.priority == -1 ||
			    param.options.priority > UINT8_MAX) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			opt_cnt += 2;
			break;
#endif /* CONFIG_NET_CONTEXT_PRIORITY */

		case 'I':
			i++;
			if (i >= argc) {
				shell_fprintf(sh, SHELL_WARNING,
					      "-I <interface name>\n");
				return -ENOEXEC;
			}
			(void)memset(param.if_name, 0x0, IFNAMSIZ);
			strncpy(param.if_name, argv[i], IFNAMSIZ - 1);

			opt_cnt += 2;
			break;

		default:
			shell_fprintf(sh, SHELL_WARNING,
				      "Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	start += opt_cnt;
	argc -= opt_cnt;

	if (argc < 2) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Not enough parameters.\n");

		if (is_udp) {
			if (IS_ENABLED(CONFIG_NET_UDP)) {
				shell_help(sh);
				return -ENOEXEC;
			}
		} else {
			if (IS_ENABLED(CONFIG_NET_TCP)) {
				shell_help(sh);
				return -ENOEXEC;
			}
		}

		return -ENOEXEC;
	}

	family = !strcmp(argv[start + 1], "v4") ? AF_INET : AF_INET6;

	if (family == AF_INET6) {
		if (net_ipv6_is_addr_unspecified(&in6_addr_dst.sin6_addr)) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Invalid destination IPv6 address.\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Connecting to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_dst.sin6_addr));

		memcpy(&param.peer_addr, &in6_addr_dst, sizeof(in6_addr_dst));
	} else {
		if (net_ipv4_is_addr_unspecified(&in4_addr_dst.sin_addr)) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Invalid destination IPv4 address.\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "Connecting to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_dst.sin_addr));

		memcpy(&param.peer_addr, &in4_addr_dst, sizeof(in4_addr_dst));
	}

	if (argc > 2) {
		param.duration_ms = MSEC_PER_SEC * strtoul(argv[start + 2],
							   NULL, 10);
	} else {
		param.duration_ms = MSEC_PER_SEC * 1;
	}

	if (argc > 3) {
		param.packet_size = parse_number(argv[start + 3], K, K_UNIT);
	} else {
		param.packet_size = 256U;
	}

	if (argc > 4) {
		param.rate_kbps =
			(parse_number(argv[start + 4], K, K_UNIT) + 999) / 1000;
	} else {
		param.rate_kbps = 10U;
	}

	return execute_upload(sh, &param, is_udp, async);
}

static int cmd_tcp_upload2(const struct shell *sh, size_t argc,
			   char *argv[])
{
	return shell_cmd_upload2(sh, argc, argv, IPPROTO_TCP);
}

static int cmd_udp_upload2(const struct shell *sh, size_t argc,
			   char *argv[])
{
	return shell_cmd_upload2(sh, argc, argv, IPPROTO_UDP);
}

static int cmd_tcp(const struct shell *sh, size_t argc, char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_INFO, "TCP support is not enabled. "
		      "Set CONFIG_NET_TCP=y in your config file.\n");

	return -ENOTSUP;
}

static int cmd_udp(const struct shell *sh, size_t argc, char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_UDP)) {
		shell_help(sh);
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_INFO, "UDP support is not enabled. "
		      "Set CONFIG_NET_UDP=y in your config file.\n");

	return -ENOTSUP;
}

static int cmd_connectap(const struct shell *sh, size_t argc, char *argv[])
{
	shell_fprintf(sh, SHELL_INFO,
		      "Zephyr has not been built with Wi-Fi support.\n");

	return 0;
}

static void tcp_session_cb(enum zperf_status status,
			   struct zperf_results *result,
			   void *user_data)
{
	const struct shell *sh = user_data;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		shell_fprintf(sh, SHELL_NORMAL, "New TCP session started.\n");
		break;

	case ZPERF_SESSION_FINISHED: {
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				((result->total_len * 8ULL * USEC_PER_SEC) /
				 (result->time_in_us * 1000ULL));
		} else {
			rate_in_kbps = 0U;
		}

		shell_fprintf(sh, SHELL_NORMAL, "TCP session ended\n");

		shell_fprintf(sh, SHELL_NORMAL, " Duration:\t\t");
		print_number_64(sh, result->time_in_us, TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, " rate:\t\t\t");
		print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		break;
	}

	case ZPERF_SESSION_ERROR:
		shell_fprintf(sh, SHELL_ERROR, "TCP session error.\n");
		break;
	}
}

static int cmd_tcp_download_stop(const struct shell *sh, size_t argc,
				 char *argv[])
{
	int ret;

	ret = zperf_tcp_download_stop();
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "TCP server not running!\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "TCP server stopped\n");

	return 0;
}

static int cmd_tcp_download(const struct shell *sh, size_t argc,
			    char *argv[])
{
	if (IS_ENABLED(CONFIG_NET_TCP)) {
		struct zperf_download_params param = { 0 };
		int ret;
		int start;

		start = shell_cmd_download(sh, argc, argv, &param);
		if (start < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to parse option.\n");
			return -ENOEXEC;
		}

		ret = zperf_bind_host(sh, argc - start, &argv[start], &param);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Unable to bind host.\n");
			shell_help(sh);
			return -ENOEXEC;
		}

		ret = zperf_tcp_download(&param, tcp_session_cb, (void *)sh);
		if (ret == -EALREADY) {
			shell_fprintf(sh, SHELL_WARNING,
				      "TCP server already started!\n");
			return -ENOEXEC;
		} else if (ret < 0) {
			shell_fprintf(sh, SHELL_ERROR,
				      "Failed to start TCP server!\n");
			return -ENOEXEC;
		}

		shell_fprintf(sh, SHELL_NORMAL,
			      "TCP server started on port %u\n", param.port);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static int cmd_version(const struct shell *sh, size_t argc, char *argv[])
{
	shell_fprintf(sh, SHELL_NORMAL, "Version: %s\nConfig: %s\n",
		      ZPERF_VERSION, CONFIG);

	return 0;
}

void zperf_shell_init(void)
{
	int ret;

	if (IS_ENABLED(MY_IP6ADDR_SET) && MY_IP6ADDR) {
		ret = net_addr_pton(AF_INET6, MY_IP6ADDR,
				    &in6_addr_my.sin6_addr);
		if (ret < 0) {
			NET_WARN("Unable to set %s address\n", "IPv6");
		} else {
			NET_INFO("Setting IP address %s",
				 net_sprint_ipv6_addr(&in6_addr_my.sin6_addr));
		}

		ret = net_addr_pton(AF_INET6, DST_IP6ADDR,
				    &in6_addr_dst.sin6_addr);
		if (ret < 0) {
			NET_WARN("Unable to set destination %s address %s",
				 "IPv6",
				 DST_IP6ADDR ? DST_IP6ADDR
					     : "(not set)");
		} else {
			NET_INFO("Setting destination IP address %s",
				 net_sprint_ipv6_addr(&in6_addr_dst.sin6_addr));
		}
	}

	if (IS_ENABLED(MY_IP4ADDR_SET) && MY_IP4ADDR) {
		ret = net_addr_pton(AF_INET, MY_IP4ADDR,
				    &in4_addr_my.sin_addr);
		if (ret < 0) {
			NET_WARN("Unable to set %s address\n", "IPv4");
		} else {
			NET_INFO("Setting IP address %s",
				 net_sprint_ipv4_addr(&in4_addr_my.sin_addr));
		}

		ret = net_addr_pton(AF_INET, DST_IP4ADDR,
				    &in4_addr_dst.sin_addr);
		if (ret < 0) {
			NET_WARN("Unable to set destination %s address %s",
				 "IPv4",
				  DST_IP4ADDR ? DST_IP4ADDR
					      : "(not set)");
		} else {
			NET_INFO("Setting destination IP address %s",
				 net_sprint_ipv4_addr(&in4_addr_dst.sin_addr));
		}
	}
}

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_tcp_download,
	SHELL_CMD(stop, NULL, "Stop TCP server\n", cmd_tcp_download_stop),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_tcp,
	SHELL_CMD(upload, NULL,
		  "[<options>] <dest ip> <dest port> <duration> <packet size>[K]\n"
		  "<options>     command options (optional): [-S tos -a]\n"
		  "<dest ip>     IP destination\n"
		  "<dest port>   port destination\n"
		  "<duration>    of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "Available options:\n"
		  "-S tos: Specify IPv4/6 type of service\n"
		  "-a: Asynchronous call (shell will not block for the upload)\n"
		  "-n: Disable Nagle's algorithm\n"
#ifdef CONFIG_NET_CONTEXT_PRIORITY
		  "-p: Specify custom packet priority\n"
#endif /* CONFIG_NET_CONTEXT_PRIORITY */
		  "Example: tcp upload 192.0.2.2 1111 1 1K\n"
		  "Example: tcp upload 2001:db8::2\n",
		  cmd_tcp_upload),
	SHELL_CMD(upload2, NULL,
		  "[<options>] v6|v4 <duration> <packet size>[K] <baud rate>[K|M]\n"
		  "<options>     command options (optional): [-S tos -a]\n"
		  "<v6|v4>:      Use either IPv6 or IPv4\n"
		  "<duration>    Duration of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "Available options:\n"
		  "-S tos: Specify IPv4/6 type of service\n"
		  "-a: Asynchronous call (shell will not block for the upload)\n"
#ifdef CONFIG_NET_CONTEXT_PRIORITY
		  "-p: Specify custom packet priority\n"
#endif /* CONFIG_NET_CONTEXT_PRIORITY */
		  "Example: tcp upload2 v6 1 1K\n"
		  "Example: tcp upload2 v4\n"
		  "-n: Disable Nagle's algorithm\n"
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
	SHELL_CMD(download, &zperf_cmd_tcp_download,
		  "[<port>]:  Server port to listen on/connect to\n"
		  "[<host>]:  Bind to <host>, an interface address\n"
		  "Example: tcp download 5001 192.168.0.1\n",
		  cmd_tcp_download),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_udp_download,
	SHELL_CMD(stop, NULL, "Stop UDP server\n", cmd_udp_download_stop),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(zperf_cmd_udp,
	SHELL_CMD(upload, NULL,
		  "[<options>] <dest ip> [<dest port> <duration> <packet size>[K] "
							"<baud rate>[K|M]]\n"
		  "<options>     command options (optional): [-S tos -a]\n"
		  "<dest ip>     IP destination\n"
		  "<dest port>   port destination\n"
		  "<duration>    of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "<baud rate>   Baudrate in kilobyte or megabyte\n"
		  "Available options:\n"
		  "-S tos: Specify IPv4/6 type of service\n"
		  "-a: Asynchronous call (shell will not block for the upload)\n"
#ifdef CONFIG_NET_CONTEXT_PRIORITY
		  "-p: Specify custom packet priority\n"
#endif /* CONFIG_NET_CONTEXT_PRIORITY */
		  "-I: Specify host interface name\n"
		  "Example: udp upload 192.0.2.2 1111 1 1K 1M\n"
		  "Example: udp upload 2001:db8::2\n",
		  cmd_udp_upload),
	SHELL_CMD(upload2, NULL,
		  "[<options>] v6|v4 [<duration> <packet size>[K] <baud rate>[K|M]]\n"
		  "<options>     command options (optional): [-S tos -a]\n"
		  "<v6|v4>:      Use either IPv6 or IPv4\n"
		  "<duration>    Duration of the test in seconds\n"
		  "<packet size> Size of the packet in byte or kilobyte "
							"(with suffix K)\n"
		  "<baud rate>   Baudrate in kilobyte or megabyte\n"
		  "Available options:\n"
		  "-S tos: Specify IPv4/6 type of service\n"
		  "-a: Asynchronous call (shell will not block for the upload)\n"
#ifdef CONFIG_NET_CONTEXT_PRIORITY
		  "-p: Specify custom packet priority\n"
#endif /* CONFIG_NET_CONTEXT_PRIORITY */
		  "-I: Specify host interface name\n"
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
	SHELL_CMD(download, &zperf_cmd_udp_download,
		  "[<options>] command options (optional): [-I eth0]\n"
		  "[<port>]:  Server port to listen on/connect to\n"
		  "[<host>]:  Bind to <host>, an interface address\n"
		  "Available options:\n"
		  "-I <interface name>: Specify host interface name\n"
		  "Example: udp download 5001 192.168.0.1\n",
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
