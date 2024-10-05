/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp, CONFIG_SNTP_LOG_LEVEL);

#include <zephyr/net/sntp.h>
#include "sntp_pkt.h"
#include <limits.h>

#define SNTP_LI_MAX 3
#define SNTP_VERSION_NUMBER 3
#define SNTP_MODE_CLIENT 3
#define SNTP_MODE_SERVER 4
#define SNTP_STRATUM_KOD 0 /* kiss-o'-death */
#define OFFSET_1970_JAN_1 2208988800

static void sntp_pkt_dump(struct sntp_pkt *pkt)
{
	if (!pkt) {
		return;
	}

	NET_DBG("li               %x", pkt->li);
	NET_DBG("vn               %x", pkt->vn);
	NET_DBG("mode             %x", pkt->mode);
	NET_DBG("stratum:         %x", pkt->stratum);
	NET_DBG("poll:            %x", pkt->poll);
	NET_DBG("precision:       %x", pkt->precision);
	NET_DBG("root_delay:      %x", ntohl(pkt->root_delay));
	NET_DBG("root_dispersion: %x", ntohl(pkt->root_dispersion));
	NET_DBG("ref_id:          %x", ntohl(pkt->ref_id));
	NET_DBG("ref_tm_s:        %x", ntohl(pkt->ref_tm_s));
	NET_DBG("ref_tm_f:        %x", ntohl(pkt->ref_tm_f));
	NET_DBG("orig_tm_s:       %x", ntohl(pkt->orig_tm_s));
	NET_DBG("orig_tm_f:       %x", ntohl(pkt->orig_tm_f));
	NET_DBG("rx_tm_s:         %x", ntohl(pkt->rx_tm_s));
	NET_DBG("rx_tm_f:         %x", ntohl(pkt->rx_tm_f));
	NET_DBG("tx_tm_s:         %x", ntohl(pkt->tx_tm_s));
	NET_DBG("tx_tm_f:         %x", ntohl(pkt->tx_tm_f));
}

#if defined(CONFIG_SNTP_UNCERTAINTY)
static int64_t q16_16_s_to_ll_us(uint32_t t)
{
	return (int64_t)(t >> 16) * (int64_t)USEC_PER_SEC +
	       (((int64_t)(t & 0xFFFF) * (int64_t)USEC_PER_SEC) >> 16);
}

static int64_t q32_32_s_to_ll_us(uint32_t t_s, uint32_t t_f)
{
	return (uint64_t)t_s * USEC_PER_SEC + (((uint64_t)t_f * (uint64_t)USEC_PER_SEC) >> 32);
}
#endif

static int32_t parse_response(uint8_t *data, uint16_t len, struct sntp_time *expected_orig_ts,
			      struct sntp_time *res)
{
	struct sntp_pkt *pkt = (struct sntp_pkt *)data;
	uint32_t ts;

	sntp_pkt_dump(pkt);

	if (ntohl(pkt->orig_tm_s) != expected_orig_ts->seconds ||
	    ntohl(pkt->orig_tm_f) != expected_orig_ts->fraction) {
		NET_DBG("Mismatch originate timestamp: %d.%09d, expect: %llu.%09u",
			ntohl(pkt->orig_tm_s), ntohl(pkt->orig_tm_f), expected_orig_ts->seconds,
			expected_orig_ts->fraction);
		return -ERANGE;
	}

	if (pkt->mode != SNTP_MODE_SERVER) {
		/* For unicast and manycast, server should return 4.
		 * For broadcast (which is not supported now), server should
		 * return 5.
		 */
		NET_DBG("Unexpected mode: %d", pkt->mode);
		return -EINVAL;
	}

	if (pkt->stratum == SNTP_STRATUM_KOD) {
		NET_DBG("kiss-o'-death stratum");
		return -EBUSY;
	}

	if (ntohl(pkt->tx_tm_s) == 0 && ntohl(pkt->tx_tm_f) == 0) {
		NET_DBG("zero transmit timestamp");
		return -EINVAL;
	}

#if defined(CONFIG_SNTP_UNCERTAINTY)

	int64_t dest_ts_us = k_ticks_to_us_near64(k_uptime_ticks());
	int64_t orig_ts_us =
		q32_32_s_to_ll_us(expected_orig_ts->seconds, expected_orig_ts->fraction);

	int64_t rx_ts_us = q32_32_s_to_ll_us(ntohl(pkt->rx_tm_s), ntohl(pkt->rx_tm_f));
	int64_t tx_ts_us = q32_32_s_to_ll_us(ntohl(pkt->tx_tm_s), ntohl(pkt->tx_tm_f));

	if (rx_ts_us > tx_ts_us || orig_ts_us > dest_ts_us) {
		NET_DBG("Invalid timestamps from SNTP server");
		return -EINVAL;
	}

	int64_t d_us = (dest_ts_us - orig_ts_us) - (tx_ts_us - rx_ts_us);
	int64_t clk_offset_us = ((rx_ts_us - orig_ts_us) + (tx_ts_us - dest_ts_us)) / 2;
	int64_t root_dispersion_us = q16_16_s_to_ll_us(ntohl(pkt->root_dispersion));
	int64_t root_delay_us = q16_16_s_to_ll_us(ntohl(pkt->root_delay));
	uint32_t precision_us;

	if (pkt->precision <= 0) {
		precision_us = (uint32_t)(USEC_PER_SEC + USEC_PER_SEC / 2) >> -pkt->precision;
	} else if (pkt->precision <= 10) {
		precision_us = (uint32_t)(USEC_PER_SEC + USEC_PER_SEC / 2) << pkt->precision;
	} else {
		NET_DBG("SNTP packet precision out of range: %d", pkt->precision);
		return -EINVAL;
	}

	res->uptime_us = dest_ts_us;
	res->seconds = (res->uptime_us + clk_offset_us) / USEC_PER_SEC;
	res->fraction = (res->uptime_us + clk_offset_us) % USEC_PER_SEC;
	res->uncertainty_us = (d_us + root_delay_us + precision_us) / 2 + root_dispersion_us;
#else
	res->fraction = ntohl(pkt->tx_tm_f);
	res->seconds = ntohl(pkt->tx_tm_s);
#endif
	ts = ntohl(pkt->tx_tm_s);

	/* Check if most significant bit is set */
	if (ts & 0x80000000) {
		/* UTC time is reckoned from 0h 0m 0s UTC
		 * on 1 January 1900.
		 */
		if (ts >= OFFSET_1970_JAN_1) {
			res->seconds -= OFFSET_1970_JAN_1;
		} else {
			return -EINVAL;
		}
	} else {
		/* UTC time is reckoned from 6h 28m 16s UTC
		 * on 7 February 2036.
		 */
		res->seconds += 0x100000000ULL - OFFSET_1970_JAN_1;
	}

	return 0;
}

int sntp_init(struct sntp_ctx *ctx, struct sockaddr *addr, socklen_t addr_len)
{
	int ret;

	if (!ctx || !addr) {
		return -EFAULT;
	}

	memset(ctx, 0, sizeof(struct sntp_ctx));

	ctx->sock.fd = zsock_socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (ctx->sock.fd < 0) {
		NET_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = zsock_connect(ctx->sock.fd, addr, addr_len);
	if (ret < 0) {
		(void)zsock_close(ctx->sock.fd);
		NET_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	ctx->sock.fds[ctx->sock.nfds].fd = ctx->sock.fd;
	ctx->sock.fds[ctx->sock.nfds].events = ZSOCK_POLLIN;
	ctx->sock.nfds++;

	return 0;
}

static int sntp_query_send(struct sntp_ctx *ctx)
{
	struct sntp_pkt tx_pkt = { 0 };
	int64_t ts_us = 0;

	/* prepare request pkt */
	tx_pkt.li = 0;
	tx_pkt.vn = SNTP_VERSION_NUMBER;
	tx_pkt.mode = SNTP_MODE_CLIENT;
	ts_us = k_ticks_to_us_near64(k_uptime_ticks());
	ctx->expected_orig_ts.seconds = ts_us / USEC_PER_SEC;
	ctx->expected_orig_ts.fraction = (ts_us % USEC_PER_SEC) * (UINT32_MAX / USEC_PER_SEC);
	tx_pkt.tx_tm_s = htonl(ctx->expected_orig_ts.seconds);
	tx_pkt.tx_tm_f = htonl(ctx->expected_orig_ts.fraction);

	return zsock_send(ctx->sock.fd, (uint8_t *)&tx_pkt, sizeof(tx_pkt), 0);
}

int sntp_query(struct sntp_ctx *ctx, uint32_t timeout, struct sntp_time *ts)
{
	int ret = 0;

	if (!ctx || !ts) {
		return -EFAULT;
	}

	ret = sntp_query_send(ctx);
	if (ret < 0) {
		NET_ERR("Failed to send over UDP socket %d", ret);
		return ret;
	}

	return sntp_recv_response(ctx, timeout, ts);
}

int sntp_recv_response(struct sntp_ctx *ctx, uint32_t timeout,
			      struct sntp_time *ts)
{
	struct sntp_pkt buf = { 0 };
	int status;
	int rcvd;

	status = zsock_poll(ctx->sock.fds, ctx->sock.nfds, timeout);
	if (status < 0) {
		NET_ERR("Error in poll:%d", errno);
		return -errno;
	}

	if (status == 0) {
		return -ETIMEDOUT;
	}

	rcvd = zsock_recv(ctx->sock.fd, (uint8_t *)&buf, sizeof(buf), 0);
	if (rcvd < 0) {
		return -errno;
	}

	if (rcvd != sizeof(struct sntp_pkt)) {
		return -EMSGSIZE;
	}

	status = parse_response((uint8_t *)&buf, sizeof(buf),
				&ctx->expected_orig_ts,
				ts);
	return status;
}

void sntp_close(struct sntp_ctx *ctx)
{
	if (ctx) {
		(void)zsock_close(ctx->sock.fd);
	}
}

#ifdef CONFIG_NET_SOCKETS_SERVICE

int sntp_init_async(struct sntp_ctx *ctx, struct sockaddr *addr, socklen_t addr_len,
		    const struct net_socket_service_desc *service)
{
	int ret;

	/* Validate service pointer */
	if (service == NULL) {
		return -EFAULT;
	}
	/* Standard init */
	ret = sntp_init(ctx, addr, addr_len);
	if (ret < 0) {
		return ret;
	}
	/* Attach socket to socket service */
	ret = net_socket_service_register(service, ctx->sock.fds, ctx->sock.nfds, ctx);
	if (ret < 0) {
		NET_ERR("Failed to register socket %d", ret);
		/* Cleanup init on register failure*/
		sntp_close(ctx);
	}
	return ret;
}

int sntp_send_async(struct sntp_ctx *ctx)
{
	int ret = 0;

	if (!ctx) {
		return -EFAULT;
	}

	ret = sntp_query_send(ctx);
	if (ret < 0) {
		NET_ERR("Failed to send over UDP socket %d", ret);
		return ret;
	}
	return 0;
}

int sntp_read_async(struct net_socket_service_event *event, struct sntp_time *ts)
{
	struct sntp_ctx *ctx = event->user_data;
	struct sntp_pkt buf = {0};
	int rcvd;

	rcvd = zsock_recv(ctx->sock.fd, (uint8_t *)&buf, sizeof(buf), 0);
	if (rcvd < 0) {
		return -errno;
	}

	if (rcvd != sizeof(struct sntp_pkt)) {
		return -EMSGSIZE;
	}

	return parse_response((uint8_t *)&buf, sizeof(buf), &ctx->expected_orig_ts, ts);
}

void sntp_close_async(const struct net_socket_service_desc *service)
{
	struct sntp_ctx *ctx = service->pev->user_data;
	/* Detach socket from socket service */
	net_socket_service_unregister(service);
	/* CLose the socket */
	if (ctx) {
		(void)zsock_close(ctx->sock.fd);
	}
}

#endif /* CONFIG_NET_SOCKETS_SERVICE */
