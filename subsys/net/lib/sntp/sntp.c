/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_sntp, CONFIG_SNTP_LOG_LEVEL);

#include <net/sntp.h>
#include "sntp_pkt.h"

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

	NET_DBG("li               %x", SNTP_GET_LI(pkt->lvm));
	NET_DBG("vn               %x", SNTP_GET_VN(pkt->lvm));
	NET_DBG("mode             %x", SNTP_GET_MODE(pkt->lvm));
	NET_DBG("stratum:         %x", pkt->stratum);
	NET_DBG("poll:            %x", pkt->poll);
	NET_DBG("precision:       %x", pkt->precision);
	NET_DBG("root_delay:      %x", pkt->root_delay);
	NET_DBG("root_dispersion: %x", pkt->root_dispersion);
	NET_DBG("ref_id:          %x", pkt->ref_id);
	NET_DBG("ref_tm_s:        %x", pkt->ref_tm_s);
	NET_DBG("ref_tm_f:        %x", pkt->ref_tm_f);
	NET_DBG("orig_tm_s:       %x", pkt->orig_tm_s);
	NET_DBG("orig_tm_f:       %x", pkt->orig_tm_f);
	NET_DBG("rx_tm_s:         %x", pkt->rx_tm_s);
	NET_DBG("rx_tm_f:         %x", pkt->rx_tm_f);
	NET_DBG("tx_tm_s:         %x", pkt->tx_tm_s);
	NET_DBG("tx_tm_f:         %x", pkt->tx_tm_f);
}

static int32_t parse_response(uint8_t *data, uint16_t len, uint32_t orig_ts,
			    struct sntp_time *time)
{
	struct sntp_pkt *pkt = (struct sntp_pkt *)data;
	uint32_t ts;

	sntp_pkt_dump(pkt);

	if (ntohl(pkt->orig_tm_s) != orig_ts) {
		NET_DBG("Mismatch originate timestamp: %d, expect: %d",
			ntohl(pkt->orig_tm_s), orig_ts);
		return -EINVAL;
	}

	if (SNTP_GET_MODE(pkt->lvm) != SNTP_MODE_SERVER) {
		/* For unicast and manycast, server should return 4.
		 * For broadcast (which is not supported now), server should
		 * return 5.
		 */
		NET_DBG("Unexpected mode: %d", SNTP_GET_MODE(pkt->lvm));
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

	time->fraction = ntohl(pkt->tx_tm_f);
	ts = ntohl(pkt->tx_tm_s);

	/* Check if most significant bit is set */
	if (ts & 0x80000000) {
		/* UTC time is reckoned from 0h 0m 0s UTC
		 * on 1 January 1900.
		 */
		if (ts >= OFFSET_1970_JAN_1) {
			time->seconds = ts - OFFSET_1970_JAN_1;
		} else {
			return -EINVAL;
		}
	} else {
		/* UTC time is reckoned from 6h 28m 16s UTC
		 * on 7 February 2036.
		 */
		time->seconds = ts + 0x100000000ULL - OFFSET_1970_JAN_1;
	}

	return 0;
}

static int sntp_recv_response(struct sntp_ctx *sntp, uint32_t timeout,
			      struct sntp_time *time)
{
	struct sntp_pkt buf = { 0 };
	int status;
	int rcvd;

	status = poll(sntp->sock.fds, sntp->sock.nfds, timeout);
	if (status < 0) {
		NET_ERR("Error in poll:%d", errno);
		return -errno;
	}

	if (status == 0) {
		return -ETIMEDOUT;
	}

	rcvd = recv(sntp->sock.fd, (uint8_t *)&buf, sizeof(buf), 0);
	if (rcvd < 0) {
		return -errno;
	}

	if (rcvd != sizeof(struct sntp_pkt)) {
		return -EMSGSIZE;
	}

	status = parse_response((uint8_t *)&buf, sizeof(buf),
				sntp->expected_orig_ts,
				time);
	return status;
}

static uint32_t get_uptime_in_sec(void)
{
	uint64_t time;

	time = k_uptime_get_32();

	return time / MSEC_PER_SEC;
}

int sntp_init(struct sntp_ctx *ctx, struct sockaddr *addr, socklen_t addr_len)
{
	int ret;

	if (!ctx || !addr) {
		return -EFAULT;
	}

	memset(ctx, 0, sizeof(struct sntp_ctx));

	ctx->sock.fd = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (ctx->sock.fd < 0) {
		NET_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(ctx->sock.fd, addr, addr_len);
	if (ret < 0) {
		(void)close(ctx->sock.fd);
		NET_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	ctx->sock.fds[ctx->sock.nfds].fd = ctx->sock.fd;
	ctx->sock.fds[ctx->sock.nfds].events = POLLIN;
	ctx->sock.nfds++;

	return 0;
}

int sntp_request(struct sntp_ctx *ctx, uint32_t timeout, uint64_t *epoch_time)
{
	struct sntp_time time;
	int res = sntp_query(ctx, timeout, &time);

	*epoch_time = time.seconds;

	return res;
}

int sntp_query(struct sntp_ctx *ctx, uint32_t timeout, struct sntp_time *time)
{
	struct sntp_pkt tx_pkt = { 0 };
	int ret = 0;

	if (!ctx || !time) {
		return -EFAULT;
	}

	/* prepare request pkt */
	SNTP_SET_LI(tx_pkt.lvm, 0);
	SNTP_SET_VN(tx_pkt.lvm, SNTP_VERSION_NUMBER);
	SNTP_SET_MODE(tx_pkt.lvm, SNTP_MODE_CLIENT);
	ctx->expected_orig_ts = get_uptime_in_sec() + OFFSET_1970_JAN_1;
	tx_pkt.tx_tm_s = htonl(ctx->expected_orig_ts);

	ret = send(ctx->sock.fd, (uint8_t *)&tx_pkt, sizeof(tx_pkt), 0);
	if (ret < 0) {
		NET_ERR("Failed to send over UDP socket %d", ret);
		return ret;
	}

	return sntp_recv_response(ctx, timeout, time);
}

void sntp_close(struct sntp_ctx *ctx)
{
	if (ctx) {
		(void)close(ctx->sock.fd);
	}
}
