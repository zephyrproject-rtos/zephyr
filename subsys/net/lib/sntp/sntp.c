/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_SNTP)
#define SYS_LOG_DOMAIN "net/sntp"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <net/sntp.h>
#include "sntp_pkt.h"

#define SNTP_LI_MAX 3
#define SNTP_VERSION_NUMBER 3
#define SNTP_MODE_CLIENT 3
#define SNTP_MODE_SERVER 4
#define SNTP_STRATUM_KOD 0 /* kiss-o'-death */
#define OFFSET_1970_JAN_1 2208988800

#define SNTP_CTX_SRV_SOCKADDR(ctx) (ctx->net_app_ctx.default_ctx->remote)

static void sntp_pkt_dump(struct sntp_pkt *pkt)
{
	if (!pkt) {
		return;
	}

	NET_DBG("li               %x", LVM_GET_LI(pkt->lvm));
	NET_DBG("vn               %x", LVM_GET_VN(pkt->lvm));
	NET_DBG("mode             %x", LVM_GET_MODE(pkt->lvm));
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

static s32_t parse_response(u8_t *data, u16_t len, u32_t orig_ts,
				u64_t *epoch_time)
{
	struct sntp_pkt *pkt = (struct sntp_pkt *)data;

	sntp_pkt_dump(pkt);

	if (ntohl(pkt->orig_tm_s) != orig_ts) {
		NET_DBG("Mismatch originate timestamp: %d, expect: %d",
			ntohl(pkt->orig_tm_s), orig_ts);
		return -EINVAL;
	}

	if (LVM_GET_LI(pkt->lvm) > SNTP_LI_MAX) {
		NET_DBG("Unexpected LI: %d", LVM_GET_LI(pkt->lvm));
		return -EINVAL;
	}

	if (LVM_GET_MODE(pkt->lvm) != SNTP_MODE_SERVER) {
		/* For unicast and manycast, server should return 4.
		 * For broadcast (which is not supported now), server should
		 * return 5.
		 */
		NET_DBG("Unexpected mode: %d", LVM_GET_MODE(pkt->lvm));
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

	if (epoch_time) {
		u32_t ts = ntohl(pkt->tx_tm_s);

		/* Check if most significant bit is set */
		if (ts & 0x80000000) {
			/* UTC time is reckoned from 0h 0m 0s UTC
			 * on 1 January 1900.
			 */
			if (ts >= OFFSET_1970_JAN_1) {
				*epoch_time = ts - OFFSET_1970_JAN_1;
			} else {
				return -EINVAL;
			}
		} else {
			/* UTC time is reckoned from 6h 28m 16s UTC
			 * on 7 February 2036.
			 */
			*epoch_time = ts + 0x100000000 - OFFSET_1970_JAN_1;
		}
	}

	return 0;
}

static void sntp_recv_cb(struct net_app_ctx *ctx, struct net_pkt *pkt,
			int status, void *user_data)
{
	struct sntp_ctx *sntp = (struct sntp_ctx *)user_data;
	struct sntp_pkt buf = { 0 };
	u64_t epoch_time = 0;
	u64_t tmp = 0;
	u16_t offset = 0;

	if (status < 0) {
		goto error_exit;
	}

	if (net_pkt_appdatalen(pkt) != sizeof(struct sntp_pkt)) {
		status = -EMSGSIZE;
		goto error_exit;
	}

	/* copy to buf */
	offset = net_pkt_get_len(pkt) - net_pkt_appdatalen(pkt);
	status = net_frag_linearize((u8_t *)&buf, sizeof(buf), pkt, offset,
				    sizeof(buf));
	if (status < 0) {
		goto error_exit;
	}

	status = parse_response((u8_t *)&buf, sizeof(buf),
				sntp->expected_orig_ts, &tmp);
	if (status == 0) {
		epoch_time = tmp;
	}

error_exit:
	if (sntp->cb) {
		sntp->cb(sntp, status, epoch_time, sntp->user_data);
	}

	net_pkt_unref(pkt);
}

static u32_t get_uptime_in_sec(void)
{
	u64_t time;

	k_enable_sys_clock_always_on();
	time = k_uptime_get_32();
	k_disable_sys_clock_always_on();

	return time / MSEC_PER_SEC;
}

int sntp_init(struct sntp_ctx *ctx, const char *srv_addr, u16_t srv_port,
	      u32_t timeout)
{
	int rv;

	if (!ctx) {
		return -EFAULT;
	}

	memset(ctx, 0, sizeof(struct sntp_ctx));

	rv = net_app_init_udp_client(&ctx->net_app_ctx, NULL, NULL, srv_addr,
				     srv_port, timeout, ctx);
	if (rv < 0) {
		NET_DBG("Failed to init udp client: %d", rv);
		return rv;
	}

	rv = net_app_set_cb(&ctx->net_app_ctx, NULL, sntp_recv_cb, NULL, NULL);
	if (rv < 0) {
		NET_DBG("Failed to set net app callback: %d", rv);
		return rv;
	}

	ctx->is_init = true;

	return 0;
}

int sntp_request(struct sntp_ctx *ctx,
		 u32_t timeout,
		 sntp_resp_cb_t callback,
		 void *user_data)
{
	struct sntp_pkt tx_pkt = { 0 };
	int rv = 0;

	if (!ctx) {
		return -EFAULT;
	}

	if (!ctx->is_init) {
		return -EINVAL;
	}

	ctx->cb = callback;
	ctx->user_data = user_data;

	/* prepare request pkt */
	LVM_SET_LI(tx_pkt.lvm, 0);
	LVM_SET_VN(tx_pkt.lvm, SNTP_VERSION_NUMBER);
	LVM_SET_MODE(tx_pkt.lvm, SNTP_MODE_CLIENT);
	ctx->expected_orig_ts = get_uptime_in_sec() + OFFSET_1970_JAN_1;
	tx_pkt.tx_tm_s = htonl(ctx->expected_orig_ts);

	rv = net_app_connect(&ctx->net_app_ctx, K_NO_WAIT);
	if (rv < 0) {
		NET_DBG("Failed to connect: %d", rv);
		return rv;
	}

	rv = net_app_send_buf(&ctx->net_app_ctx, (u8_t *)&tx_pkt,
			      sizeof(tx_pkt),
			      &SNTP_CTX_SRV_SOCKADDR(ctx),
			      sizeof(SNTP_CTX_SRV_SOCKADDR(ctx)),
			      timeout, NULL);
	return rv;
}

void sntp_close(struct sntp_ctx *ctx)
{
	if (!ctx || !ctx->is_init) {
		return;
	}

	net_app_close(&ctx->net_app_ctx);
	net_app_release(&ctx->net_app_ctx);
	ctx->is_init = false;
}
