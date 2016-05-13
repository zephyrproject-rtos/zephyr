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

#define DEVICE_NAME "zperf shell"

static const char *CONFIG = ""
#ifdef CONFIG_MICROKERNEL
		"microkernel "
#else
		"nanokernel "
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

static struct net_addr in_addr_my = {
#ifdef CONFIG_NETWORKING_WITH_IPV6
	.family = AF_INET6,
	.in6_addr = IN6ADDR_ANY_INIT
#else
	.family = AF_INET,
	.in_addr = { { { 0 } } }
#endif
};

#define MY_SRC_PORT 50000
#define DEF_PORT 5001

static void shell_cmd_setip(int argc, char *argv[])
{
#ifdef CONFIG_NETWORKING_WITH_IPV4
	uint32_t value[4] = {0};

	if (argc != 2) {
		/* Print usage */
		printk("\nsetip:\n");
		printk("Usage:\tsetip <my ip>\n");
		printk("\nExample setip 10.237.164.178\n");
		return;
	}

	if (parseIpString(argv[1], value) == 0) {
		printk("[setip] Setting IP address %d.%d.%d.%d\n",
				value[0], value[1], value[2], value[3]);
		uip_ipaddr_t uip_ipaddr_local;

		uip_ipaddr(&uip_ipaddr_local,
				value[0],
				value[1],
				value[2],
				value[3]);
		uip_sethostaddr(&uip_ipaddr_local);
	} else {
		printk("[setip] ERROR! Unable to set IP\n");
	}
#endif
}

static void shell_cmd_udp_download(int argc, char *argv[])
{
	static bool udp_stopped = true;
	int port;

	if (argc == 1) {
		/* Print usage */
		printk("\nudp.download:\n");
		printk("Usage:\tudp.download <port>\n");
		printk("\nExample udp.download 5001\n");
		return;
	}

	if (argc > 1) {
		port = strtoul(argv[1], NULL, 10);
	} else {
		port = DEF_PORT;
	}

	if (udp_stopped == false) {
		printk("[udp.download] ERROR! UDP server already started!\n");
		return;
	}

	zperf_receiver_init(port);
	udp_stopped = false;
	printk("[udp.download] UDP server started on port %u\n", port);
}

static void shell_cmd_udp_upload(int argc, char *argv[])
{
	int value[IP_INDEX_MAX] = { 0 };
	static struct net_addr net_addr_remote = { .family = AF_INET, .in_addr = { {
			{ 192, 168, 43, 181 } } } };
	unsigned int duration_in_ms, packet_size, rate_in_kbps, client_rate_in_kbps;
	uint16_t port;
	zperf_results results = { 0 };
	struct net_context *net_context;

	if (argc == 1) {
		/* Print usage */
		printk("\nudp.upload:\n");
		printk(
				"Usage:\tudp.upload <dest ip> <dest port> <duration> <packet "
				"size>[K] <baud rate>[K|M]\n");
		printk("\t<dest ip>:\tIP destination\n");
		printk("\t<dest port>:\tport destination\n");
		printk("\t<duration>:\t of the test in seconds\n");
		printk(
				"\t<packet size>:\tSize of the paket in byte or kilobyte "
				"(with suffix K)\n");
		printk("\t<baud rate>:\tBaudrate in kilobyte or megabyte\n");
		printk("\nExample udp.upload 10.237.164.178 1111 1 1K 1M\n");
		return;
	}

	if (argc > 1 && parseIpString(argv[1], value) == 0) {
		printk("[udp.upload] Remote IP address is ");
		print_address(value);
		printk("\n");
#ifdef CONFIG_NETWORKING_WITH_IPV6
		for (int i = 0; i < IP_INDEX_MAX; i++) {
			net_addr_remote.in6_addr.s6_addr[i] = (value[i] & 0xFF00) >> 8;
			net_addr_remote.in6_addr.s6_addr[i] = value[i] & 0xFF;
		}
#else
		net_addr_remote.in_addr.s4_addr[0] = value[0];
		net_addr_remote.in_addr.s4_addr[1] = value[1];
		net_addr_remote.in_addr.s4_addr[2] = value[2];
		net_addr_remote.in_addr.s4_addr[3] = value[3];
#endif
	} else {
		printk(
				"[udp.upload] ERROR! Please specify the IP address of the"
				" remote server\n");
	}

	if (argc > 2) {
		port = strtoul(argv[2], NULL, 10);
		printk("[udp.upload] Remote port is %u\n", port);
	} else {
		port = DEF_PORT;
	}

	net_context = net_context_get(IPPROTO_UDP, &net_addr_remote, port,
			&in_addr_my, MY_SRC_PORT);

	if (!net_context) {
		printk("[udp.upload] ERROR! Fail to retrieve a net context\n");
		return;
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
	printk("[udp.upload] duration:\t\t");
	print_number(duration_in_ms * USEC_PER_MSEC, TIME_US, TIME_US_UNIT);
	printk("\n");

	printk("[udp.upload] packet size:\t%u bytes\n", packet_size);

	printk("[udp.upload] rate:\t\t");
	print_number(rate_in_kbps, KBPS, KBPS_UNIT);
	printk("\n");

	printk("[udp.upload] start...\n");

	/* Perform upload test */
	zperf_upload(net_context, duration_in_ms, packet_size, rate_in_kbps,
			&results);

	printk("[udp.upload] completed!\n");

	/* Print results */
	if (results.time_in_us != 0)
		rate_in_kbps = (uint32_t) (((uint64_t) results.nb_bytes_sent
				* (uint64_t) 8 * (uint64_t) USEC_PER_SEC)
				/ ((uint64_t) results.time_in_us * 1024));
	else
		rate_in_kbps = 0;

	if (results.client_time_in_us != 0)
		client_rate_in_kbps = (uint32_t) (((uint64_t) results.nb_packets_sent
				* (uint64_t) packet_size * (uint64_t) 8
				* (uint64_t) USEC_PER_SEC)
				/ ((uint64_t) results.client_time_in_us * 1024));
	else
		client_rate_in_kbps = 0;

	if (!rate_in_kbps)
		printk("[udp.upload] LAST PACKET NOT RECEIVED!!!\n");

	printk("[udp.upload] statistic:\t\t\tserver\t(client)\n");
	printk("[udp.upload] duration:\t\t\t");
	print_number(results.time_in_us, TIME_US, TIME_US_UNIT);
	printk("\t(");
	print_number(results.client_time_in_us, TIME_US, TIME_US_UNIT);
	printk(")\n");

	printk("[udp.upload] nb packets:\t\t%u\t(%u)\n", results.nb_packets_rcvd,
			results.nb_packets_sent);

	printk("[udp.upload] nb packets outorder:\t%u\n",
			results.nb_packets_outorder);
	printk("[udp.upload] nb packets lost:\t\t%u\n", results.nb_packets_lost);

	printk("[udp.upload] jitter:\t\t\t");
	print_number(results.jitter_in_us, TIME_US, TIME_US_UNIT);
	printk("\n");

	printk("[udp.upload] rate:\t\t\t");
	print_number(rate_in_kbps, KBPS, KBPS_UNIT);
	printk("\t(");
	print_number(client_rate_in_kbps, KBPS, KBPS_UNIT);
	printk(")\n");

	/* release net context */
	net_context_put(net_context);
}

#ifdef CONFIG_NETWORKING_WITH_TCP
static void shell_cmd_connectap(int argc, char *argv[])
{
	printk("[connectap] Zephyr has not been built with Wi-Fi support.\n");
}

static void shell_cmd_tcp_upload(int argc, char *argv[])
{
	printk("[connectap] Zephyr doesn't support TCP client yet.\n");
}
#endif /* CONFIG_NETWORKING_WITH_TCP */

static void shell_cmd_tcp_download(int argc, char *argv[])
{
	static bool tcp_stopped = true;
	int port;

	if (argc == 1) {
		/* Print usage */
		printk("\ntcp.download:\n");
		printk("Usage:\ttcp.download <port>\n");
		printk("\nExample tcp.download 5001\n");
		return;
	}

	if (argc > 1) {
		port = strtoul(argv[1], NULL, 10);
	} else {
		port = DEF_PORT;
	}

	if (tcp_stopped == false) {
		printk("[tcp.download] ERROR! TCP server already started!\n");
		return;
	}

	zperf_tcp_receiver_init(port);
	tcp_stopped = false;
	printk("[tcp.download] TCP server started on port %u\n", port);
}

static void shell_cmd_version(int argc, char *argv[])
{
	printk("\nzperf version: %s config: %s\n", VERSION, CONFIG);
}

static void zperf_init(void)
{
	zperf_session_init();
}

struct shell_cmd commands[] = {
		{ "setip", shell_cmd_setip },
		{ "connectap", shell_cmd_connectap },
		{ "version", shell_cmd_version },
		{ "udp.upload", shell_cmd_udp_upload },
		{ "udp.download", shell_cmd_udp_download },
#ifdef CONFIG_NETWORKING_WITH_TCP
		{ "tcp.upload", shell_cmd_tcp_upload },
		{ "tcp.download", shell_cmd_tcp_download },
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
	shell_init("zperf> ", commands);
	net_init();
	zperf_init();
}
