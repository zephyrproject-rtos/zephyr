/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <misc/printk.h>
#include <misc/shell.h>
#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

#if defined(CONFIG_NETWORKING_WITH_IPV6)
#include <contiki/ipv6/uip-ds6.h>
#endif

#define DEVICE_NAME "zperf shell"

static const char *CONFIG =
#ifdef CONFIG_WLAN
		"wlan "
#endif
#ifdef CONFIG_ETH_DW
		"ethernet "
#endif
#ifdef CONFIG_NETWORKING_WITH_IPV4
		"ipv4 "
#endif
#ifdef CONFIG_NETWORKING_WITH_IPV6
		"ipv6 "
#endif
		"";
#ifdef CONFIG_NETWORKING_WITH_IPV6
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }
#define DST_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64
#endif

#ifdef PROFILER
#include "profiler.h"
#endif

static struct net_addr in_addr_my = {
#ifdef CONFIG_NETWORKING_WITH_IPV6
	.family = AF_INET6,
	.in6_addr = MY_IPADDR /* IN6ADDR_ANY_INIT */
#else
	.family = AF_INET,
	.in_addr = { { { 0 } } }
#endif
};

static struct net_addr in_addr_dst = {
#ifdef CONFIG_NETWORKING_WITH_IPV6
	.family = AF_INET6,
	.in6_addr = DST_IPADDR /* IN6ADDR_ANY_INIT */
#else
	.family = AF_INET,
	.in_addr = { { { 192, 168, 43, 181 } } }
#endif
};

#define MY_SRC_PORT 50000
#define DEF_PORT 5001

static int shell_cmd_setip(int argc, char *argv[])
{
	uint32_t value[IP_INDEX_MAX] = { 0 };

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	int prefix;

	if (argc != 3) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_SETIP);
		printk("Usage:\t%s <my ip> <prefix len>\n", CMD_STR_SETIP);
		printk("\nExample %s 2001:db8::2 64\n", CMD_STR_SETIP);
		return -1;
	}

	if (parseIpString(argv[1], value) != 0) {
		printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
		return 0;
	}

	prefix = strtoul(argv[2], NULL, 10);

	for (int i = 0; i < IP_INDEX_MAX; i++) {
		in_addr_my.in6_addr.s6_addr[2*i] = (value[i] & 0xFF00) >> 8;
		in_addr_my.in6_addr.s6_addr[2*i+1] = value[i] & 0xFF;
	}

	uip_ds6_addr_add((uip_ipaddr_t *)&in_addr_my.in6_addr, 0, ADDR_MANUAL);
	uip_ds6_prefix_add((uip_ipaddr_t *)&in_addr_my.in6_addr, prefix, 0);

#else
	if (argc != 2) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_SETIP);
		printk("Usage:\t%s <my ip>\n", CMD_STR_SETIP);
		printk("\nExample %s 10.237.164.178\n", CMD_STR_SETIP);
		return -1;
	}

	if (parseIpString(argv[1], value) != 0) {
		printk("[%s] ERROR! Unable to set IP\n", CMD_STR_SETIP);
		return -1;
	}

	uip_ipaddr_t uip_ipaddr_local;

	uip_ipaddr(&uip_ipaddr_local,
			value[0],
			value[1],
			value[2],
			value[3]);
	uip_sethostaddr(&uip_ipaddr_local);
#endif

	printk("[%s] Setting IP address ", CMD_STR_SETIP);
	print_address(value);
	printk("\n");

	return 0;
}

static int shell_cmd_udp_download(int argc, char *argv[])
{
	static bool udp_stopped = true;
	int port;

	if (argc == 1) {
		/* Print usage */
		printk("\n%s:\n", CMD_STR_UDP_DOWNLOAD);
		printk("Usage:\t%s <port>\n", CMD_STR_UDP_DOWNLOAD);
		printk("\nExample %s 5001\n", CMD_STR_UDP_DOWNLOAD);
		return -1;
	}

	if (argc > 1) {
		port = strtoul(argv[1], NULL, 10);
	} else {
		port = DEF_PORT;
	}

	if (udp_stopped == false) {
		printk("[%s] ERROR! UDP server already started!\n",
			CMD_STR_UDP_DOWNLOAD);
		return -1;
	}

	zperf_receiver_init(port);
	udp_stopped = false;
	printk("[%s] UDP server started on port %u\n", CMD_STR_UDP_DOWNLOAD, port);

	return 0;
}

static void shell_udp_upload_usage(void)
{
		/* Print usage */
		printk("\n%s:\n", CMD_STR_UDP_UPLOAD);
		printk(
				"Usage:\t%s <dest ip> <dest port> <duration> <packet "
				"size>[K] <baud rate>[K|M]\n", CMD_STR_UDP_UPLOAD);
		printk("\t<dest ip>:\tIP destination\n");
		printk("\t<dest port>:\tport destination\n");
		printk("\t<duration>:\t of the test in seconds\n");
		printk(
				"\t<packet size>:\tSize of the packet in byte or kilobyte "
				"(with suffix K)\n");
		printk("\t<baud rate>:\tBaudrate in kilobyte or megabyte\n");
		printk("\nExample %s 10.237.164.178 1111 1 1K 1M\n", CMD_STR_UDP_UPLOAD);
}

static void shell_tcp_upload_usage(void)
{
		/* Print usage */
		printk("\n%s:\n", CMD_STR_TCP_UPLOAD);
		printk(
				"Usage:\t%s <dest ip> <dest port> <duration> <packet "
				"size>[K]\n", CMD_STR_TCP_UPLOAD);
		printk("\t<dest ip>:\tIP destination\n");
		printk("\t<dest port>:\tport destination\n");
		printk("\t<duration>:\t of the test in seconds\n");
		printk(
				"\t<packet size>:\tSize of the packet in byte or kilobyte "
				"(with suffix K)\n");
		printk("\nExample %s 10.237.164.178 1111 1 1K 1M\n", CMD_STR_TCP_UPLOAD);
}

static void shell_udp_upload_print_stats(zperf_results *results)
{
	unsigned int rate_in_kbps, client_rate_in_kbps;

	printk("[%s] Upload completed!\n", CMD_STR_UDP_UPLOAD);

	if (results->time_in_us != 0)
		rate_in_kbps = (uint32_t) (((uint64_t) results->nb_bytes_sent
				* (uint64_t) 8 * (uint64_t) USEC_PER_SEC)
				/ ((uint64_t) results->time_in_us * 1024));
	else
		rate_in_kbps = 0;

	if (results->client_time_in_us != 0)
		client_rate_in_kbps = (uint32_t) (((uint64_t) results->nb_packets_sent
				* (uint64_t) results->packet_size * (uint64_t) 8
				* (uint64_t) USEC_PER_SEC)
				/ ((uint64_t) results->client_time_in_us * 1024));
	else
		client_rate_in_kbps = 0;

	if (!rate_in_kbps)
		printk("[%s] LAST PACKET NOT RECEIVED!!!\n", CMD_STR_UDP_UPLOAD);

	printk("[%s] statistic:\t\t\tserver\t(client)\n", CMD_STR_UDP_UPLOAD);
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

static void shell_tcp_upload_print_stats(zperf_results *results)
{
	unsigned int client_rate_in_kbps;

	printk("[%s] Upload completed!\n", CMD_STR_TCP_UPLOAD);

	if (results->client_time_in_us != 0)
		client_rate_in_kbps = (uint32_t) (((uint64_t) results->nb_packets_sent
				* (uint64_t) results->packet_size * (uint64_t) 8
				* (uint64_t) USEC_PER_SEC)
				/ ((uint64_t) results->client_time_in_us * 1024));
	else
		client_rate_in_kbps = 0;

	printk("[%s] duration:\t", CMD_STR_TCP_UPLOAD);
	print_number(results->client_time_in_us, TIME_US, TIME_US_UNIT);
	printk("\n");
	printk("[%s] nb packets:\t%u\n", CMD_STR_UDP_UPLOAD,
			results->nb_packets_sent);
	printk("[%s] nb sending errors (retry or fail):\t%u\n", CMD_STR_UDP_UPLOAD,
			results->nb_packets_errors);
	printk("[%s] rate:\t", CMD_STR_UDP_UPLOAD);
	print_number(client_rate_in_kbps, KBPS, KBPS_UNIT);
	printk("\n");
}

static int shell_cmd_upload(int argc, char *argv[])
{
	int value[IP_INDEX_MAX] = { 0 };
	unsigned int duration_in_ms, packet_size, rate_in_kbps;
	uint16_t port;
	zperf_results results = { 0 };
	struct net_context *net_context;
	uint8_t is_udp = !strcmp(argv[0], CMD_STR_UDP_UPLOAD) ? 1 : 0;

	if (argc == 1) {
		is_udp ? shell_udp_upload_usage() : shell_tcp_upload_usage();
		return -1;
	}

	if (argc > 1 && parseIpString(argv[1], value) == 0) {
		printk("[%s] Remote IP address is ", argv[0]);
		print_address(value);
		printk("\n");
#ifdef CONFIG_NETWORKING_WITH_IPV6
		for (int i = 0; i < IP_INDEX_MAX; i++) {
			in_addr_dst.in6_addr.s6_addr[2*i] = (value[i] & 0xFF00) >> 8;
			in_addr_dst.in6_addr.s6_addr[2*i+1] = value[i] & 0xFF;
		}
#else
		in_addr_dst.in_addr.s4_addr[0] = value[0];
		in_addr_dst.in_addr.s4_addr[1] = value[1];
		in_addr_dst.in_addr.s4_addr[2] = value[2];
		in_addr_dst.in_addr.s4_addr[3] = value[3];
#endif
	} else {
		printk(
			"[%s] ERROR! Please specify the IP address of the"
			" remote server\n",  argv[0]);
			return -1;
	}

	if (argc > 2) {
		port = strtoul(argv[2], NULL, 10);
		printk("[%s] Remote port is %u\n", argv[0], port);
	} else {
		port = DEF_PORT;
	}

	net_context = net_context_get(is_udp ? IPPROTO_UDP : IPPROTO_TCP,
			&in_addr_dst, port,
			&in_addr_my, MY_SRC_PORT);

	if (!net_context) {
		printk("[%s] ERROR! Fail to retrieve a net context\n", argv[0]);
		return -1;
	}

	if (argc > 3)
		duration_in_ms = strtoul(argv[3], NULL, 10) * MSEC_PER_SEC;
	else
		duration_in_ms = 1000;

	if (argc > 4)
		packet_size = parse_number(argv[4], K, K_UNIT);
	else
		packet_size = 256;

	if (argc > 5)
		rate_in_kbps = (parse_number(argv[5], K, K_UNIT) + 1023) / 1024;
	else
		rate_in_kbps = 10;

	/* Print settings */
	printk("[%s] duration:\t\t", argv[0]);
	print_number(duration_in_ms * USEC_PER_MSEC, TIME_US, TIME_US_UNIT);
	printk("\n");

	printk("[%s] packet size:\t%u bytes\n", argv[0], packet_size);

	printk("[%s] start...\n", argv[0]);

	if (is_udp) {
		printk("[%s] rate:\t\t", argv[0]);
		print_number(rate_in_kbps, KBPS, KBPS_UNIT);
		printk("\n");
		zperf_udp_upload(net_context, duration_in_ms, packet_size, rate_in_kbps,
		&results);
		shell_udp_upload_print_stats(&results);
	} else {
		zperf_tcp_upload(net_context, duration_in_ms, packet_size, &results);
		shell_tcp_upload_print_stats(&results);
	}

	/* release net context */
	net_context_put(net_context);

	return 0;
}

static int shell_cmd_connectap(int argc, char *argv[])
{
	printk("[%s] Zephyr has not been built with Wi-Fi support.\n",
		CMD_STR_CONNECTAP);

	return 0;
}

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

	if (tcp_stopped == false) {
		printk("[%s] ERROR! TCP server already started!\n",
			CMD_STR_TCP_DOWNLOAD);
		return -1;
	}

	zperf_tcp_receiver_init(port);
	tcp_stopped = false;
	printk("[%s] TCP server started on port %u\n", CMD_STR_TCP_DOWNLOAD, port);

	return 0;
}

static int shell_cmd_version(int argc, char *argv[])
{
	printk("\nzperf [%s]: %s config: %s\n", CMD_STR_VERSION, VERSION, CONFIG);

	return 0;
}

static void zperf_init(void)
{
	zperf_session_init();
}

#define MY_SHELL_MODULE "zperf"
struct shell_cmd commands[] = {
		{ CMD_STR_SETIP, shell_cmd_setip },
		{ CMD_STR_CONNECTAP, shell_cmd_connectap },
		{ CMD_STR_VERSION, shell_cmd_version },
		{ CMD_STR_UDP_UPLOAD, shell_cmd_upload },
		{ CMD_STR_UDP_DOWNLOAD, shell_cmd_udp_download },
#ifdef CONFIG_NETWORKING_WITH_TCP
		{ CMD_STR_TCP_UPLOAD, shell_cmd_upload },
		{ CMD_STR_TCP_DOWNLOAD, shell_cmd_tcp_download },
#endif
#ifdef PROFILER
		PROF_CMD,
#endif
		{ NULL, NULL } };

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
{
#else
void main(void)
{
#endif
	shell_cmd_version(0, NULL);
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
	shell_register_default_module(MY_SHELL_MODULE);
	net_init();
	zperf_init();

#if PROFILER
	while (1) {
		task_sleep(5 * sys_clock_ticks_per_sec);
		prof_flush();
	}
#endif
}
