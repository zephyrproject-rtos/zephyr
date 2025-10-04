/*
 * Copyright (c) 2025 Lothar Felten <lothar.felten@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_sntp, CONFIG_SNTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/clock.h>
#include "sntp_pkt.h"

#define SNTP_SERVER_RX_BUFFER_SIZE 1280

struct sntp_server_data {
	int sock;
	char recv_buffer[SNTP_SERVER_RX_BUFFER_SIZE];
};

static struct sntp_pkt sntp_reply;

#if defined(CONFIG_NET_IPV4)
struct sntp_server_data data_ipv4;
static void sntp_udp4_thread(void);

K_THREAD_DEFINE(udp4_thread_id, CONFIG_SNTP_SERVER_STACK_SIZE, sntp_udp4_thread, NULL, NULL, NULL,
		CONFIG_SNTP_SERVER_THREAD_PRIORITY, 0, 0);
#endif

#if defined(CONFIG_NET_IPV6)
struct sntp_server_data data_ipv6;
static void sntp_udp6_thread(void);

K_THREAD_DEFINE(udp6_thread_id, CONFIG_SNTP_SERVER_STACK_SIZE, sntp_udp6_thread, NULL, NULL, NULL,
		CONFIG_SNTP_SERVER_THREAD_PRIORITY, 0, 0);
#endif

static int sntp_start_server(struct sntp_server_data *data, struct sockaddr *bind_addr,
			     socklen_t bind_addrlen)
{
	int ret;

	sntp_reply.li = SNTP_LEAP_INDICATOR_CLOCK_INVALID;
	sntp_reply.mode = SNTP_MODE_SERVER;
	sntp_reply.stratum = SNTP_STRATUM_KOD;
	sntp_reply.mode = SNTP_MODE_SERVER;
	strncpy((char *)(&sntp_reply.ref_id), "X", sizeof(sntp_reply.ref_id));
	data->sock = socket(bind_addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (data->sock < 0) {
		LOG_ERR("Failed to create UDP socket: %d", errno);
		return -errno;
	}

	if (bind_addr->sa_family == AF_INET6) {
		int val;

		val = IPV6_PREFER_SRC_PUBLIC | IPV6_PREFER_SRC_TMP;
		(void)setsockopt(data->sock, IPPROTO_IPV6, IPV6_ADDR_PREFERENCES, &val,
				 sizeof(val));
	}

	ret = bind(data->sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		NET_ERR("Failed to bind UDP socket: %d", errno);
		ret = -errno;
	}

	return ret;
}

static int sntp_process_packet(struct sntp_server_data *data)
{
	int ret = 0;
	int rx;
	struct sockaddr client_addr;
	socklen_t client_addr_len;
	struct sntp_pkt *pkt;
	struct timespec systime;

	client_addr_len = sizeof(client_addr);
	rx = recvfrom(data->sock, data->recv_buffer, sizeof(data->recv_buffer), 0, &client_addr,
		      &client_addr_len);
	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &systime);
	if (rx < 0) {
		/* Socket error */
		NET_ERR("UDP Connection error %d", errno);
		ret = -errno;
		return ret;
	}
	if (rx != sizeof(struct sntp_pkt)) {
		NET_ERR("SNTP server malformed message");
		return -EPROTO;
	}
	if (ret < 0) {
		NET_ERR("SNTP server: system clock error");
		return ret;
	}

	pkt = (struct sntp_pkt *)data->recv_buffer;
	sntp_reply.mode = SNTP_MODE_SERVER;
	sntp_reply.vn = pkt->vn; /* copy from request */
	sntp_reply.li = SNTP_LEAP_INDICATOR_NONE;
	sntp_reply.poll = pkt->poll; /* copy from request */
	sntp_reply.root_delay = 0;
	sntp_reply.root_dispersion = 0;
	sntp_reply.ref_tm_s = 0;
	sntp_reply.orig_tm_s = pkt->tx_tm_s;
	sntp_reply.orig_tm_f = pkt->tx_tm_f;
	sntp_reply.rx_tm_s = htonl(systime.tv_sec + OFFSET_1970_JAN_1);
	sntp_reply.rx_tm_f = htonl((systime.tv_nsec));
	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, &systime);
	if (ret < 0) {
		NET_ERR("SNTP server: system clock error");
		return ret;
	}
	sntp_reply.tx_tm_s = htonl(systime.tv_sec + OFFSET_1970_JAN_1);
	sntp_reply.tx_tm_f = htonl((systime.tv_nsec));
	sntp_pkt_dump(&sntp_reply);
	ret = sendto(data->sock, &sntp_reply, sizeof(struct sntp_pkt), 0, &client_addr,
		     client_addr_len);
	if (ret < 0) {
		NET_ERR("UDP: Failed to send %d", errno);
		return -EIO;
	}
	return ret;
}

static void sntp_udp4_thread(void)
{
	int ret;
	struct sockaddr_in addr4;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(123);

	ret = sntp_start_server(&data_ipv4, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		return;
	}

	while (ret == 0) {
		sntp_process_packet(&data_ipv4);
		if (ret < 0) {
			LOG_ERR("error processing data(%d)", ret);
		}
	}
}

static void sntp_udp6_thread(void)
{
	int ret;
	struct sockaddr_in6 addr6;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(123);

	ret = sntp_start_server(&data_ipv6, (struct sockaddr *)&addr6, sizeof(addr6));
	if (ret < 0) {
		return;
	}

	while (ret == 0) {
		ret = sntp_process_packet(&data_ipv6);
		if (ret < 0) {
			LOG_ERR("error processing data(%d)", ret);
		}
	}
}

int sntp_server_clock_source(unsigned char *refid, unsigned char stratum, char precision)
{
	sntp_reply.stratum = stratum;
	strncpy((char *)(&sntp_reply.ref_id), refid, sizeof(sntp_reply.ref_id));
	sntp_reply.precision = precision;
	return 0;
}
