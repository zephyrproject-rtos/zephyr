/* udp.c - UDP specific code for throughput server */

/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_tp_server_udp
#define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/udp.h>

#include <net/net_app.h>

#include "common.h"

#define STATS_CHECK 1000

#define TYPE_SEQ_NUM 42

struct header {
	unsigned char type;
	unsigned char len;
	unsigned char value[0];
} __packed;

static struct net_app_ctx udp;

/* Note that both tcp and udp can share the same pool but in this
 * example the UDP context and TCP context have separate pools.
 */
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_SLAB_DEFINE(echo_tx_udp, 100);
NET_PKT_DATA_POOL_DEFINE(echo_data_udp, 200);

static struct k_mem_slab *tx_udp_slab(void)
{
	return &echo_tx_udp;
}

static struct net_buf_pool *data_udp_pool(void)
{
	return &echo_data_udp;
}
#else
#define tx_udp_slab NULL
#define data_udp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

static inline void set_dst_addr(sa_family_t family,
				struct net_pkt *pkt,
				struct sockaddr *dst_addr)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = udp_hdr->src_port;
	}
#endif /* CONFIG_NET_IPV6) */

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = udp_hdr->src_port;
	}
#endif /* CONFIG_NET_IPV6) */
}

static void udp_received(struct net_app_ctx *ctx,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	static char dbg[MAX_DBG_PRINT + 1];
	static u32_t prev_seq;
	static u32_t count;
	struct sockaddr dst_addr;
	struct header *hdr;
	sa_family_t family = net_pkt_family(pkt);
	socklen_t dst_len;
	u32_t pkt_len;
#if defined(SEND_REPLY)
	struct net_pkt *reply_pkt;
	int ret;
#endif

	snprintk(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	if (family == AF_INET6) {
		dst_len = sizeof(struct sockaddr_in6);
	} else {
		dst_len = sizeof(struct sockaddr_in);
	}

	set_dst_addr(family, pkt, &dst_addr);

#if defined(SEND_REPLY)
	reply_pkt = build_reply_pkt(dbg, ctx, pkt);
#endif

	pkt_len = net_pkt_appdatalen(pkt);

	tp_stats.bytes.recv += pkt_len;
	tp_stats.pkts.recv++;

	hdr = (struct header *)net_pkt_appdata(pkt);
	if (hdr->type == TYPE_SEQ_NUM) {
		u32_t seq;

		memcpy(&seq, hdr->value, sizeof(seq));
		seq = ntohl(seq);

		if (seq && prev_seq && seq != (prev_seq + 1)) {
			tp_stats.pkts.dropped += (seq - prev_seq);
		}

		prev_seq = seq;
	}

	net_pkt_unref(pkt);

#if defined(SEND_REPLY)
	if (!reply_pkt) {
		return;
	}

	pkt_len = net_pkt_appdatalen(reply_pkt);

	ret = net_app_send_pkt(ctx, reply_pkt, &dst_addr, dst_len, K_NO_WAIT,
			       UINT_TO_POINTER(pkt_len));
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);

		tp_stats.pkt.dropped++;
	} else {
		tp_stats.pkt.sent++;
	}
#endif

	/* Print statistics only periodically */
	if (count > STATS_CHECK) {
		print_statistics();
		count = 0;
	}

	count++;
}

void start_udp(void)
{
	int ret;

	ret = net_app_init_udp_server(&udp, NULL, MY_PORT, NULL);
	if (ret < 0) {
		NET_ERR("Cannot init UDP service at port %d", MY_PORT);
		return;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&udp, tx_udp_slab, data_udp_pool);
#endif

	ret = net_app_set_cb(&udp, NULL, udp_received, pkt_sent, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		net_app_release(&udp);
		return;
	}

	net_app_server_enable(&udp);

	ret = net_app_listen(&udp);
	if (ret < 0) {
		NET_ERR("Cannot wait connection (%d)", ret);
		net_app_release(&udp);
		return;
	}
}

void stop_udp(void)
{
	net_app_close(&udp);
	net_app_release(&udp);
}
