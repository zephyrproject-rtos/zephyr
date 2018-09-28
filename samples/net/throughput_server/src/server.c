/* server.c - Networking server for measuring networking throughput */

/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "tp-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdlib.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <net/net_app.h>

#include "common.h"

/* The startup time needs to be longish if DHCP is enabled as setting
 * DHCP up takes some time.
 */
#define APP_STARTUP_TIME K_SECONDS(20)

#define APP_BANNER "Running throughput server"

struct tp_stats tp_stats;

static struct k_sem quit_lock;

void quit(void)
{
	k_sem_give(&quit_lock);
}

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt)
{
	struct net_pkt *reply_pkt;
	struct net_buf *frag, *tmp;
	int header_len = 0, recv_len, reply_len;

	NET_DBG("%s received %d bytes", name, net_pkt_appdatalen(pkt));

	if (net_pkt_appdatalen(pkt) == 0) {
		return NULL;
	}

	reply_pkt = net_app_get_net_pkt(ctx, net_pkt_family(pkt), K_FOREVER);

	NET_ASSERT(reply_pkt);
	NET_ASSERT(net_pkt_family(reply_pkt) == net_pkt_family(pkt));

	recv_len = net_pkt_get_len(pkt);

	tmp = pkt->frags;

	/* If we have link layer headers, then get rid of them here. */
	if (recv_len != net_pkt_appdatalen(pkt)) {
		/* First fragment will contain IP header so move the data
		 * down in order to get rid of it.
		 */
		header_len = net_pkt_appdata(pkt) - tmp->data;

		NET_ASSERT(header_len < CONFIG_NET_BUF_DATA_SIZE);

		/* After this pull, the tmp->data points directly to application
		 * data.
		 */
		net_buf_pull(tmp, header_len);
	}

	net_pkt_set_appdatalen(reply_pkt, net_pkt_appdatalen(pkt));

	while (tmp) {
		frag = net_app_get_net_buf(ctx, reply_pkt, K_FOREVER);

		if (net_buf_headroom(tmp) == 0) {
			/* If there is no link layer headers in the
			 * received fragment, then get rid of that also
			 * in the sending fragment. We end up here
			 * if MTU is larger than fragment size, this
			 * is typical for ethernet.
			 */
			net_buf_push(frag, net_buf_headroom(frag));

			frag->len = 0; /* to make fragment empty */

			/* Make sure to set the reserve so that
			 * in sending side we add the link layer
			 * header if needed.
			 */
			net_pkt_set_ll_reserve(reply_pkt, 0);
		}

		NET_ASSERT_INFO(net_buf_tailroom(frag) >= tmp->len,
				"tail %zd longer than len %d",
				net_buf_tailroom(frag), tmp->len);

		memcpy(net_buf_add(frag, tmp->len), tmp->data, tmp->len);

		tmp = net_pkt_frag_del(pkt, NULL, tmp);
	}

	reply_len = net_pkt_get_len(reply_pkt);

	NET_ASSERT_INFO((recv_len - header_len) == reply_len,
			"Received %d bytes, sending %d bytes",
			recv_len - header_len, reply_len);

	return reply_pkt;
}

#define PRINT_STATS_SECS 15
#define PRINT_STATISTICS_INTERVAL K_SECONDS(PRINT_STATS_SECS)

static inline s64_t cmp_val(u64_t val1, u64_t val2)
{
	return (s64_t)(val1 - val2);
}

static inline void stats(void)
{
	static u64_t next_print;
	static bool first_print;
	static u32_t count;
	u64_t curr = k_uptime_get();
	s64_t cmp = cmp_val(curr, next_print);

	if (!next_print || (abs(cmp) > PRINT_STATISTICS_INTERVAL)) {
		if (!first_print) {
			first_print = true;
		} else {
			NET_INFO("[%u]", ++count);
			NET_INFO("Packets received %u", tp_stats.pkts.recv);
#if defined(SEND_REPLY)
			NET_INFO("Packets sent     %u", tp_stats.pkts.sent);
#endif
			NET_INFO("Packets dropped  %u", tp_stats.pkts.dropped);
			NET_INFO("Bytes received   %u", tp_stats.bytes.recv);
#if defined(SEND_REPLY)
			NET_INFO("Bytes sent       %u", tp_stats.bytes.sent);
#endif
			NET_INFO("Packets / period %u", tp_stats.pkts.recv -
				 tp_stats.pkts.prev_recv);
			NET_INFO("Packets / sec    %u", (tp_stats.pkts.recv -
						     tp_stats.pkts.prev_recv) /
				 PRINT_STATS_SECS);
		}

		next_print = curr + PRINT_STATISTICS_INTERVAL;

		tp_stats.pkts.prev_recv = tp_stats.pkts.recv;
	}
}

void print_statistics(void)
{
	/* In order to make the info print lines shorter, use shorter
	 * function name.
	 */
	stats();
}

void pkt_sent(struct net_app_ctx *ctx,
	     int status,
	     void *user_data_send,
	     void *user_data)
{
	if (!status) {
		NET_DBG("Sent %d bytes", POINTER_TO_UINT(user_data_send));
	}
}

static inline int init_app(void)
{
	k_sem_init(&quit_lock, 0, UINT_MAX);

	return 0;
}

void main(void)
{
	init_app();

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		start_udp();
	}

	k_sem_take(&quit_lock, K_FOREVER);

	NET_INFO("Stopping...");

	if (IS_ENABLED(CONFIG_NET_UDP)) {
		stop_udp();
	}
}
