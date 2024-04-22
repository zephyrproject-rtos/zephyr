/*
 * Copyright (c) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_tcp, CONFIG_NET_TCP_LOG_LEVEL);

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#if defined(CONFIG_NET_TCP_ISN_RFC6528)
#include <mbedtls/md5.h>
#endif
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/udp.h>
#include "ipv4.h"
#include "ipv6.h"
#include "connection.h"
#include "net_stats.h"
#include "net_private.h"
#include "tcp_internal.h"

#define ACK_TIMEOUT_MS tcp_max_timeout_ms
#define ACK_TIMEOUT K_MSEC(ACK_TIMEOUT_MS)
#define LAST_ACK_TIMEOUT_MS tcp_max_timeout_ms
#define LAST_ACK_TIMEOUT K_MSEC(LAST_ACK_TIMEOUT_MS)
#define FIN_TIMEOUT K_MSEC(tcp_max_timeout_ms)
#define ACK_DELAY K_MSEC(100)
#define ZWP_MAX_DELAY_MS 120000
#define DUPLICATE_ACK_RETRANSMIT_TRHESHOLD 3

static int tcp_rto = CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT;
static int tcp_retries = CONFIG_NET_TCP_RETRY_COUNT;
static int tcp_max_timeout_ms;
static int tcp_rx_window =
#if (CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE != 0)
	CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE;
#else
#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
	(CONFIG_NET_BUF_RX_COUNT * CONFIG_NET_BUF_DATA_SIZE) / 3;
#else
	CONFIG_NET_PKT_BUF_RX_DATA_POOL_SIZE / 3;
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */
#endif
static int tcp_tx_window =
#if (CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE != 0)
	CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE;
#else
#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
	(CONFIG_NET_BUF_TX_COUNT * CONFIG_NET_BUF_DATA_SIZE) / 3;
#else
	CONFIG_NET_PKT_BUF_TX_DATA_POOL_SIZE / 3;
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */
#endif
#ifdef CONFIG_NET_TCP_RANDOMIZED_RTO
#define TCP_RTO_MS (conn->rto)
#else
#define TCP_RTO_MS (tcp_rto)
#endif

/* Define the number of MSS sections the congestion window is initialized at */
#define TCP_CONGESTION_INITIAL_WIN 1
#define TCP_CONGESTION_INITIAL_SSTHRESH 3

static sys_slist_t tcp_conns = SYS_SLIST_STATIC_INIT(&tcp_conns);

static K_MUTEX_DEFINE(tcp_lock);

K_MEM_SLAB_DEFINE_STATIC(tcp_conns_slab, sizeof(struct tcp),
				CONFIG_NET_MAX_CONTEXTS, 4);

static struct k_work_q tcp_work_q;
static K_KERNEL_STACK_DEFINE(work_q_stack, CONFIG_NET_TCP_WORKQ_STACK_SIZE);

static enum net_verdict tcp_in(struct tcp *conn, struct net_pkt *pkt);
static bool is_destination_local(struct net_pkt *pkt);
static void tcp_out(struct tcp *conn, uint8_t flags);
static const char *tcp_state_to_str(enum tcp_state state, bool prefix);

int (*tcp_send_cb)(struct net_pkt *pkt) = NULL;
size_t (*tcp_recv_cb)(struct tcp *conn, struct net_pkt *pkt) = NULL;

static uint32_t tcp_get_seq(struct net_buf *buf)
{
	return *(uint32_t *)net_buf_user_data(buf);
}

static void tcp_set_seq(struct net_buf *buf, uint32_t seq)
{
	*(uint32_t *)net_buf_user_data(buf) = seq;
}

static int tcp_pkt_linearize(struct net_pkt *pkt, size_t pos, size_t len)
{
	struct net_buf *buf, *first = pkt->cursor.buf, *second = first->frags;
	int ret = 0;
	size_t len1, len2;

	if (net_pkt_get_len(pkt) < (pos + len)) {
		NET_ERR("Insufficient packet len=%zd (pos+len=%zu)",
			net_pkt_get_len(pkt), pos + len);
		ret = -EINVAL;
		goto out;
	}

	buf = net_pkt_get_frag(pkt, len, TCP_PKT_ALLOC_TIMEOUT);

	if (!buf || net_buf_max_len(buf) < len) {
		if (buf) {
			net_buf_unref(buf);
		}
		ret = -ENOBUFS;
		goto out;
	}

	net_buf_linearize(buf->data, net_buf_max_len(buf), pkt->frags, pos, len);
	net_buf_add(buf, len);

	len1 = first->len - (pkt->cursor.pos - pkt->cursor.buf->data);
	len2 = len - len1;

	first->len -= len1;

	while (len2) {
		size_t pull_len = MIN(second->len, len2);
		struct net_buf *next;

		len2 -= pull_len;
		net_buf_pull(second, pull_len);
		next = second->frags;
		if (second->len == 0) {
			net_buf_unref(second);
		}
		second = next;
	}

	buf->frags = second;
	first->frags = buf;
 out:
	return ret;
}

static struct tcphdr *th_get(struct net_pkt *pkt)
{
	size_t ip_len = net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt);
	struct tcphdr *th = NULL;
 again:
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, ip_len) != 0) {
		goto out;
	}

	if (!net_pkt_is_contiguous(pkt, sizeof(*th))) {
		if (tcp_pkt_linearize(pkt, ip_len, sizeof(*th)) < 0) {
			goto out;
		}

		goto again;
	}

	th = net_pkt_cursor_get_pos(pkt);
 out:
	return th;
}

static size_t tcp_endpoint_len(sa_family_t af)
{
	return (af == AF_INET) ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
}

static int tcp_endpoint_set(union tcp_endpoint *ep, struct net_pkt *pkt,
			    enum pkt_addr src)
{
	int ret = 0;

	switch (net_pkt_family(pkt)) {
	case AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			struct net_ipv4_hdr *ip = NET_IPV4_HDR(pkt);
			struct tcphdr *th;

			th = th_get(pkt);
			if (!th) {
				return -ENOBUFS;
			}

			memset(ep, 0, sizeof(*ep));

			ep->sin.sin_port = src == TCP_EP_SRC ? th_sport(th) :
							       th_dport(th);
			net_ipv4_addr_copy_raw((uint8_t *)&ep->sin.sin_addr,
					       src == TCP_EP_SRC ?
							ip->src : ip->dst);
			ep->sa.sa_family = AF_INET;
		} else {
			ret = -EINVAL;
		}

		break;

	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			struct net_ipv6_hdr *ip = NET_IPV6_HDR(pkt);
			struct tcphdr *th;

			th = th_get(pkt);
			if (!th) {
				return -ENOBUFS;
			}

			memset(ep, 0, sizeof(*ep));

			ep->sin6.sin6_port = src == TCP_EP_SRC ? th_sport(th) :
								 th_dport(th);
			net_ipv6_addr_copy_raw((uint8_t *)&ep->sin6.sin6_addr,
					       src == TCP_EP_SRC ?
							ip->src : ip->dst);
			ep->sa.sa_family = AF_INET6;
		} else {
			ret = -EINVAL;
		}

		break;

	default:
		NET_ERR("Unknown address family: %hu", net_pkt_family(pkt));
		ret = -EINVAL;
	}

	return ret;
}

static const char *tcp_flags(uint8_t flags)
{
#define BUF_SIZE 25 /* 6 * 4 + 1 */
	static char buf[BUF_SIZE];
	int len = 0;

	buf[0] = '\0';

	if (flags) {
		if (flags & SYN) {
			len += snprintk(buf + len, BUF_SIZE - len, "SYN,");
		}
		if (flags & FIN) {
			len += snprintk(buf + len, BUF_SIZE - len, "FIN,");
		}
		if (flags & ACK) {
			len += snprintk(buf + len, BUF_SIZE - len, "ACK,");
		}
		if (flags & PSH) {
			len += snprintk(buf + len, BUF_SIZE - len, "PSH,");
		}
		if (flags & RST) {
			len += snprintk(buf + len, BUF_SIZE - len, "RST,");
		}
		if (flags & URG) {
			len += snprintk(buf + len, BUF_SIZE - len, "URG,");
		}

		if (len > 0) {
			buf[len - 1] = '\0'; /* delete the last comma */
		}
	}
#undef BUF_SIZE
	return buf;
}

static size_t tcp_data_len(struct net_pkt *pkt)
{
	struct tcphdr *th = th_get(pkt);
	size_t tcp_options_len = (th_off(th) - 5) * 4;
	int len = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
		net_pkt_ip_opts_len(pkt) - sizeof(*th) - tcp_options_len;

	return len > 0 ? (size_t)len : 0;
}

static const char *tcp_th(struct net_pkt *pkt)
{
#define BUF_SIZE 80
	static char buf[BUF_SIZE];
	int len = 0;
	struct tcphdr *th = th_get(pkt);

	buf[0] = '\0';

	if (th_off(th) < 5) {
		len += snprintk(buf + len, BUF_SIZE - len,
				"bogus th_off: %hu", (uint16_t)th_off(th));
		goto end;
	}

	len += snprintk(buf + len, BUF_SIZE - len,
			"%s Seq=%u", tcp_flags(th_flags(th)), th_seq(th));

	if (th_flags(th) & ACK) {
		len += snprintk(buf + len, BUF_SIZE - len,
				" Ack=%u", th_ack(th));
	}

	len += snprintk(buf + len, BUF_SIZE - len,
			" Len=%ld", (long)tcp_data_len(pkt));
end:
#undef BUF_SIZE
	return buf;
}

#define is_6lo_technology(pkt)						\
	(IS_ENABLED(CONFIG_NET_IPV6) &&	net_pkt_family(pkt) == AF_INET6 && \
	 (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&			\
	  net_pkt_lladdr_dst(pkt)->type == NET_LINK_IEEE802154))

static void tcp_send(struct net_pkt *pkt)
{
	tcp_pkt_ref(pkt);

	if (tcp_send_cb) {
		if (tcp_send_cb(pkt) < 0) {
			NET_ERR("net_send_data()");
			tcp_pkt_unref(pkt);
		}
		goto out;
	}

	/* We must have special handling for some network technologies that
	 * tweak the IP protocol headers during packet sending. This happens
	 * with Bluetooth and IEEE 802.15.4 which use IPv6 header compression
	 * (6lo) and alter the sent network packet. So in order to avoid any
	 * corruption of the original data buffer, we must copy the sent data.
	 * For Bluetooth, its fragmentation code will even mangle the data
	 * part of the message so we need to copy those too.
	 */
	if (is_6lo_technology(pkt)) {
		struct net_pkt *new_pkt;

		new_pkt = tcp_pkt_clone(pkt);
		if (!new_pkt) {
			/* The caller of this func assumes that the net_pkt
			 * is consumed by this function. We call unref here
			 * so that the unref at the end of the func will
			 * free the net_pkt.
			 */
			tcp_pkt_unref(pkt);
			NET_WARN("net_pkt alloc failure");
			goto out;
		}

		if (net_send_data(new_pkt) < 0) {
			tcp_pkt_unref(new_pkt);
		}

		/* We simulate sending of the original pkt and unref it like
		 * the device driver would do.
		 */
		tcp_pkt_unref(pkt);
	} else {
		if (net_send_data(pkt) < 0) {
			NET_ERR("net_send_data()");
			tcp_pkt_unref(pkt);
		}
	}
out:
	tcp_pkt_unref(pkt);
}

static void tcp_derive_rto(struct tcp *conn)
{
#ifdef CONFIG_NET_TCP_RANDOMIZED_RTO
	/* Compute a randomized rto 1 and 1.5 times tcp_rto */
	uint32_t gain;
	uint8_t gain8;
	uint32_t rto;

	/* Getting random is computational expensive, so only use 8 bits */
	sys_rand_get(&gain8, sizeof(uint8_t));

	gain = (uint32_t)gain8;
	gain += 1 << 9;

	rto = (uint32_t)tcp_rto;
	rto = (gain * rto) >> 9;
	conn->rto = (uint16_t)rto;
#else
	ARG_UNUSED(conn);
#endif
}

#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE

/* Implementation according to RFC6582 */

static void tcp_new_reno_log(struct tcp *conn, char *step)
{
	NET_DBG("conn: %p, ca %s, cwnd=%d, ssthres=%d, fast_pend=%i",
		conn, step, conn->ca.cwnd, conn->ca.ssthresh,
		conn->ca.pending_fast_retransmit_bytes);
}

static void tcp_new_reno_init(struct tcp *conn)
{
	conn->ca.cwnd = conn_mss(conn) * TCP_CONGESTION_INITIAL_WIN;
	conn->ca.ssthresh = conn_mss(conn) * TCP_CONGESTION_INITIAL_SSTHRESH;
	conn->ca.pending_fast_retransmit_bytes = 0;
	tcp_new_reno_log(conn, "init");
}

static void tcp_new_reno_fast_retransmit(struct tcp *conn)
{
	if (conn->ca.pending_fast_retransmit_bytes == 0) {
		conn->ca.ssthresh = MAX(conn_mss(conn) * 2, conn->unacked_len / 2);
		/* Account for the lost segments */
		conn->ca.cwnd = conn_mss(conn) * 3 + conn->ca.ssthresh;
		conn->ca.pending_fast_retransmit_bytes = conn->unacked_len;
		tcp_new_reno_log(conn, "fast_retransmit");
	}
}

static void tcp_new_reno_timeout(struct tcp *conn)
{
	conn->ca.ssthresh = MAX(conn_mss(conn) * 2, conn->unacked_len / 2);
	conn->ca.cwnd = conn_mss(conn);
	tcp_new_reno_log(conn, "timeout");
}

/* For every duplicate ack increment the cwnd by mss */
static void tcp_new_reno_dup_ack(struct tcp *conn)
{
	int32_t new_win = conn->ca.cwnd;

	new_win += conn_mss(conn);
	conn->ca.cwnd = MIN(new_win, UINT16_MAX);
	tcp_new_reno_log(conn, "dup_ack");
}

static void tcp_new_reno_pkts_acked(struct tcp *conn, uint32_t acked_len)
{
	int32_t new_win = conn->ca.cwnd;
	int32_t win_inc = MIN(acked_len, conn_mss(conn));

	if (conn->ca.pending_fast_retransmit_bytes == 0) {
		if (conn->ca.cwnd < conn->ca.ssthresh) {
			new_win += win_inc;
		} else {
			/* Implement a div_ceil	to avoid rounding to 0 */
			new_win += ((win_inc * win_inc) + conn->ca.cwnd - 1) / conn->ca.cwnd;
		}
		conn->ca.cwnd = MIN(new_win, UINT16_MAX);
	} else {
		/* Check if it is still in fast recovery mode */
		if (conn->ca.pending_fast_retransmit_bytes <= acked_len) {
			conn->ca.pending_fast_retransmit_bytes = 0;
			conn->ca.cwnd = conn->ca.ssthresh;
		} else {
			conn->ca.pending_fast_retransmit_bytes -= acked_len;
			conn->ca.cwnd -= acked_len;
		}
	}
	tcp_new_reno_log(conn, "pkts_acked");
}

static void tcp_ca_init(struct tcp *conn)
{
	tcp_new_reno_init(conn);
}

static void tcp_ca_fast_retransmit(struct tcp *conn)
{
	tcp_new_reno_fast_retransmit(conn);
}

static void tcp_ca_timeout(struct tcp *conn)
{
	tcp_new_reno_timeout(conn);
}

static void tcp_ca_dup_ack(struct tcp *conn)
{
	tcp_new_reno_dup_ack(conn);
}

static void tcp_ca_pkts_acked(struct tcp *conn, uint32_t acked_len)
{
	tcp_new_reno_pkts_acked(conn, acked_len);
}
#else

static void tcp_ca_init(struct tcp *conn) { }

static void tcp_ca_fast_retransmit(struct tcp *conn) { }

static void tcp_ca_timeout(struct tcp *conn) { }

static void tcp_ca_dup_ack(struct tcp *conn) { }

static void tcp_ca_pkts_acked(struct tcp *conn, uint32_t acked_len) { }

#endif

#if defined(CONFIG_NET_TCP_KEEPALIVE)

static void tcp_send_keepalive_probe(struct k_work *work);

static void keep_alive_timer_init(struct tcp *conn)
{
	conn->keep_alive = false;
	conn->keep_idle = CONFIG_NET_TCP_KEEPIDLE_DEFAULT;
	conn->keep_intvl = CONFIG_NET_TCP_KEEPINTVL_DEFAULT;
	conn->keep_cnt = CONFIG_NET_TCP_KEEPCNT_DEFAULT;
	NET_DBG("keepalive timer init idle = %d, interval = %d, cnt = %d",
		conn->keep_idle, conn->keep_intvl, conn->keep_cnt);
	k_work_init_delayable(&conn->keepalive_timer, tcp_send_keepalive_probe);
}

static void keep_alive_param_copy(struct tcp *to, struct tcp *from)
{
	to->keep_alive = from->keep_alive;
	to->keep_idle = from->keep_idle;
	to->keep_intvl = from->keep_intvl;
	to->keep_cnt = from->keep_cnt;
}

static void keep_alive_timer_restart(struct tcp *conn)
{
	if (!conn->keep_alive || conn->state != TCP_ESTABLISHED) {
		return;
	}

	conn->keep_cur = 0;
	k_work_reschedule_for_queue(&tcp_work_q, &conn->keepalive_timer,
				    K_SECONDS(conn->keep_idle));
}

static void keep_alive_timer_stop(struct tcp *conn)
{
	k_work_cancel_delayable(&conn->keepalive_timer);
}

static int set_tcp_keep_alive(struct tcp *conn, const void *value, size_t len)
{
	int keep_alive;

	if (conn == NULL || value == NULL || len != sizeof(int)) {
		return -EINVAL;
	}

	keep_alive = *(int *)value;
	if ((keep_alive < 0) || (keep_alive > 1)) {
		return -EINVAL;
	}

	conn->keep_alive = (bool)keep_alive;

	if (keep_alive) {
		keep_alive_timer_restart(conn);
	} else {
		keep_alive_timer_stop(conn);
	}

	return 0;
}

static int set_tcp_keep_idle(struct tcp *conn, const void *value, size_t len)
{
	int keep_idle;

	if (conn == NULL || value == NULL || len != sizeof(int)) {
		return -EINVAL;
	}

	keep_idle = *(int *)value;
	if (keep_idle < 1) {
		return -EINVAL;
	}

	conn->keep_idle = keep_idle;

	keep_alive_timer_restart(conn);

	return 0;
}

static int set_tcp_keep_intvl(struct tcp *conn, const void *value, size_t len)
{
	int keep_intvl;

	if (conn == NULL || value == NULL || len != sizeof(int)) {
		return -EINVAL;
	}

	keep_intvl = *(int *)value;
	if (keep_intvl < 1) {
		return -EINVAL;
	}

	conn->keep_intvl = keep_intvl;

	keep_alive_timer_restart(conn);

	return 0;
}

static int set_tcp_keep_cnt(struct tcp *conn, const void *value, size_t len)
{
	int keep_cnt;

	if (conn == NULL || value == NULL || len != sizeof(int)) {
		return -EINVAL;
	}

	keep_cnt = *(int *)value;
	if (keep_cnt < 1) {
		return -EINVAL;
	}

	conn->keep_cnt = keep_cnt;

	keep_alive_timer_restart(conn);

	return 0;
}

static int get_tcp_keep_alive(struct tcp *conn, void *value, size_t *len)
{
	if (conn == NULL || value == NULL || len == NULL ||
	    *len != sizeof(int)) {
		return -EINVAL;
	}

	*((int *)value) = (int)conn->keep_alive;

	return 0;
}

static int get_tcp_keep_idle(struct tcp *conn, void *value, size_t *len)
{
	if (conn == NULL || value == NULL || len == NULL ||
	    *len != sizeof(int)) {
		return -EINVAL;
	}

	*((int *)value) = (int)conn->keep_idle;

	return 0;
}

static int get_tcp_keep_intvl(struct tcp *conn, void *value, size_t *len)
{
	if (conn == NULL || value == NULL || len == NULL ||
	    *len != sizeof(int)) {
		return -EINVAL;
	}

	*((int *)value) = (int)conn->keep_intvl;

	return 0;
}

static int get_tcp_keep_cnt(struct tcp *conn, void *value, size_t *len)
{
	if (conn == NULL || value == NULL || len == NULL ||
	    *len != sizeof(int)) {
		return -EINVAL;
	}

	*((int *)value) = (int)conn->keep_cnt;

	return 0;
}

#else /* CONFIG_NET_TCP_KEEPALIVE */

#define keep_alive_timer_init(...)
#define keep_alive_param_copy(...)
#define keep_alive_timer_restart(...)
#define keep_alive_timer_stop(...)
#define set_tcp_keep_alive(...) (-ENOPROTOOPT)
#define set_tcp_keep_idle(...) (-ENOPROTOOPT)
#define set_tcp_keep_intvl(...) (-ENOPROTOOPT)
#define set_tcp_keep_cnt(...) (-ENOPROTOOPT)
#define get_tcp_keep_alive(...) (-ENOPROTOOPT)
#define get_tcp_keep_idle(...) (-ENOPROTOOPT)
#define get_tcp_keep_intvl(...) (-ENOPROTOOPT)
#define get_tcp_keep_cnt(...) (-ENOPROTOOPT)

#endif /* CONFIG_NET_TCP_KEEPALIVE */

static void tcp_send_queue_flush(struct tcp *conn)
{
	struct net_pkt *pkt;

	k_work_cancel_delayable(&conn->send_timer);

	while ((pkt = tcp_slist(conn, &conn->send_queue, get,
				struct net_pkt, next))) {
		tcp_pkt_unref(pkt);
	}
}

static void tcp_conn_release(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, conn_release);
	struct net_pkt *pkt;

#if defined(CONFIG_NET_TEST)
	if (conn->test_closed_cb != NULL) {
		conn->test_closed_cb(conn, conn->test_user_data);
	}
#endif

	/* Application is no longer there, unref any remaining packets on the
	 * fifo (although there shouldn't be any at this point.)
	 */
	while ((pkt = k_fifo_get(&conn->recv_data, K_NO_WAIT)) != NULL) {
		tcp_pkt_unref(pkt);
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	if (conn->context->conn_handler) {
		net_conn_unregister(conn->context->conn_handler);
		conn->context->conn_handler = NULL;
	}

	/* As the TCP socket could be closed without connect being called,
	 * check if the address reference is done before releasing the address.
	 */
	if (conn->iface != NULL && conn->addr_ref_done) {
		net_if_addr_unref(conn->iface, conn->src.sa.sa_family,
				  conn->src.sa.sa_family == AF_INET ?
				  (const void *)&conn->src.sin.sin_addr :
				  (const void *)&conn->src.sin6.sin6_addr);
	}

	conn->context->tcp = NULL;
	conn->state = TCP_UNUSED;

	tcp_send_queue_flush(conn);

	(void)k_work_cancel_delayable(&conn->send_data_timer);
	tcp_pkt_unref(conn->send_data);

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT) {
		tcp_pkt_unref(conn->queue_recv_data);
	}

	(void)k_work_cancel_delayable(&conn->timewait_timer);
	(void)k_work_cancel_delayable(&conn->fin_timer);
	(void)k_work_cancel_delayable(&conn->persist_timer);
	(void)k_work_cancel_delayable(&conn->ack_timer);
	(void)k_work_cancel_delayable(&conn->send_timer);
	(void)k_work_cancel_delayable(&conn->recv_queue_timer);
	keep_alive_timer_stop(conn);

	k_mutex_unlock(&conn->lock);

	net_context_unref(conn->context);
	conn->context = NULL;

	k_mutex_lock(&tcp_lock, K_FOREVER);
	sys_slist_find_and_remove(&tcp_conns, &conn->next);
	k_mutex_unlock(&tcp_lock);

	k_mem_slab_free(&tcp_conns_slab, (void *)conn);
}

#if defined(CONFIG_NET_TEST)
void tcp_install_close_cb(struct net_context *ctx,
			  net_tcp_closed_cb_t cb,
			  void *user_data)
{
	NET_ASSERT(ctx->tcp != NULL);

	((struct tcp *)ctx->tcp)->test_closed_cb = cb;
	((struct tcp *)ctx->tcp)->test_user_data = user_data;
}
#endif

static int tcp_conn_unref(struct tcp *conn)
{
	int ref_count = atomic_get(&conn->ref_count);

	NET_DBG("conn: %p, ref_count=%d", conn, ref_count);

	k_mutex_lock(&conn->lock, K_FOREVER);

#if !defined(CONFIG_NET_TEST_PROTOCOL)
	if (conn->in_connect) {
		conn->in_connect = false;
		k_sem_reset(&conn->connect_sem);
	}
#endif /* CONFIG_NET_TEST_PROTOCOL */

	k_mutex_unlock(&conn->lock);

	ref_count = atomic_dec(&conn->ref_count) - 1;
	if (ref_count != 0) {
		tp_out(net_context_get_family(conn->context), conn->iface,
		       "TP_TRACE", "event", "CONN_DELETE");
		return ref_count;
	}

	/* Release the TCP context from the TCP workqueue. This will ensure,
	 * that all pending TCP works are cancelled properly, when the context
	 * is released.
	 */
	k_work_submit_to_queue(&tcp_work_q, &conn->conn_release);

	return ref_count;
}

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
#define tcp_conn_close(conn, status)				\
	tcp_conn_close_debug(conn, status, __func__, __LINE__)

static int tcp_conn_close_debug(struct tcp *conn, int status,
				const char *caller, int line)
#else
static int tcp_conn_close(struct tcp *conn, int status)
#endif
{
#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("conn: %p closed by TCP stack (%s():%d)", conn, caller, line);
#endif
	k_mutex_lock(&conn->lock, K_FOREVER);
	conn_state(conn, TCP_CLOSED);
	keep_alive_timer_stop(conn);
	k_mutex_unlock(&conn->lock);

	if (conn->in_connect) {
		if (conn->connect_cb) {
			conn->connect_cb(conn->context, status, conn->context->user_data);

			/* Make sure the connect_cb is only called once. */
			conn->connect_cb = NULL;
		}
	} else if (conn->context->recv_cb) {
		conn->context->recv_cb(conn->context, NULL, NULL, NULL,
				       status, conn->recv_user_data);
	}

	k_sem_give(&conn->tx_sem);

	return tcp_conn_unref(conn);
}

static bool tcp_send_process_no_lock(struct tcp *conn)
{
	bool unref = false;
	struct net_pkt *pkt;
	bool local = false;

	pkt = tcp_slist(conn, &conn->send_queue, peek_head,
			struct net_pkt, next);
	if (!pkt) {
		goto out;
	}

	NET_DBG("%s %s", tcp_th(pkt), conn->in_retransmission ?
		"in_retransmission" : "");

	if (conn->in_retransmission) {
		if (conn->send_retries > 0) {
			struct net_pkt *clone = tcp_pkt_clone(pkt);

			if (clone) {
				tcp_send(clone);
				conn->send_retries--;
			} else {
				NET_WARN("net_pkt alloc failure");
			}
		} else {
			unref = true;
			goto out;
		}
	} else {
		uint8_t fl = th_get(pkt)->th_flags;
		bool forget = ACK == fl || PSH == fl || (ACK | PSH) == fl ||
			RST & fl;

		pkt = forget ? tcp_slist(conn, &conn->send_queue, get,
					 struct net_pkt, next) :
			tcp_pkt_clone(pkt);
		if (!pkt) {
			NET_WARN("net_pkt alloc failure");
			goto out;
		}

		if (is_destination_local(pkt)) {
			local = true;
		}

		tcp_send(pkt);

		if (forget == false &&
		    !k_work_delayable_remaining_get(&conn->send_timer)) {
			conn->send_retries = tcp_retries;
			conn->in_retransmission = true;
		}
	}

	if (conn->in_retransmission) {
		k_work_reschedule_for_queue(&tcp_work_q, &conn->send_timer,
					    K_MSEC(TCP_RTO_MS));
	} else if (local && !sys_slist_is_empty(&conn->send_queue)) {
		k_work_reschedule_for_queue(&tcp_work_q, &conn->send_timer,
					    K_NO_WAIT);
	}

out:
	return unref;
}

static void tcp_send_process(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, send_timer);
	bool unref;

	k_mutex_lock(&conn->lock, K_FOREVER);

	unref = tcp_send_process_no_lock(conn);

	k_mutex_unlock(&conn->lock);

	if (unref) {
		tcp_conn_close(conn, -ETIMEDOUT);
	}
}

static void tcp_send_timer_cancel(struct tcp *conn)
{
	if (conn->in_retransmission == false) {
		return;
	}

	k_work_cancel_delayable(&conn->send_timer);

	{
		struct net_pkt *pkt = tcp_slist(conn, &conn->send_queue, get,
						struct net_pkt, next);
		if (pkt) {
			NET_DBG("%s", tcp_th(pkt));
			tcp_pkt_unref(pkt);
		}
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		conn->in_retransmission = false;
	} else {
		conn->send_retries = tcp_retries;
		k_work_reschedule_for_queue(&tcp_work_q, &conn->send_timer,
					    K_MSEC(TCP_RTO_MS));
	}
}

#if defined(CONFIG_NET_TCP_IPV6_ND_REACHABILITY_HINT)

static void tcp_nbr_reachability_hint(struct tcp *conn)
{
	int64_t now;
	struct net_if *iface;

	if (net_context_get_family(conn->context) != AF_INET6) {
		return;
	}

	now = k_uptime_get();
	iface = net_context_get_iface(conn->context);

	/* Ensure that Neighbor Reachability hints are rate-limited (using threshold
	 * of half of reachable time).
	 */
	if ((now - conn->last_nd_hint_time) > (net_if_ipv6_get_reachable_time(iface) / 2)) {
		net_ipv6_nbr_reachability_hint(iface, &conn->dst.sin6.sin6_addr);
		conn->last_nd_hint_time = now;
	}
}

#else /* CONFIG_NET_TCP_IPV6_ND_REACHABILITY_HINT */

#define tcp_nbr_reachability_hint(...)

#endif /* CONFIG_NET_TCP_IPV6_ND_REACHABILITY_HINT */

static const char *tcp_state_to_str(enum tcp_state state, bool prefix)
{
	const char *s = NULL;
#define _(_x) case _x: do { s = #_x; goto out; } while (0)
	switch (state) {
	_(TCP_UNUSED);
	_(TCP_LISTEN);
	_(TCP_SYN_SENT);
	_(TCP_SYN_RECEIVED);
	_(TCP_ESTABLISHED);
	_(TCP_FIN_WAIT_1);
	_(TCP_FIN_WAIT_2);
	_(TCP_CLOSE_WAIT);
	_(TCP_CLOSING);
	_(TCP_LAST_ACK);
	_(TCP_TIME_WAIT);
	_(TCP_CLOSED);
	}
#undef _
	NET_ASSERT(s, "Invalid TCP state: %u", state);
out:
	return prefix ? s : (s + 4);
}

static const char *tcp_conn_state(struct tcp *conn, struct net_pkt *pkt)
{
#define BUF_SIZE 160
	static char buf[BUF_SIZE];

	snprintk(buf, BUF_SIZE, "%s [%s Seq=%u Ack=%u]", pkt ? tcp_th(pkt) : "",
			tcp_state_to_str(conn->state, false),
			conn->seq, conn->ack);
#undef BUF_SIZE
	return buf;
}

static uint8_t *tcp_options_get(struct net_pkt *pkt, int tcp_options_len,
				uint8_t *buf, size_t buf_len)
{
	struct net_pkt_cursor backup;
	int ret;

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt) +
		     sizeof(struct tcphdr));
	ret = net_pkt_read(pkt, buf, MIN(tcp_options_len, buf_len));
	if (ret < 0) {
		buf = NULL;
	}

	net_pkt_cursor_restore(pkt, &backup);

	return buf;
}

static bool tcp_options_check(struct tcp_options *recv_options,
			      struct net_pkt *pkt, ssize_t len)
{
	uint8_t options_buf[40]; /* TCP header max options size is 40 */
	bool result = len > 0 && ((len % 4) == 0) ? true : false;
	uint8_t *options = tcp_options_get(pkt, len, options_buf,
					   sizeof(options_buf));
	uint8_t opt, opt_len;

	NET_DBG("len=%zd", len);

	recv_options->mss_found = false;
	recv_options->wnd_found = false;

	for ( ; options && len >= 1; options += opt_len, len -= opt_len) {
		opt = options[0];

		if (opt == NET_TCP_END_OPT) {
			break;
		} else if (opt == NET_TCP_NOP_OPT) {
			opt_len = 1;
			continue;
		} else {
			if (len < 2) { /* Only END and NOP can have length 1 */
				NET_ERR("Illegal option %d with length %zd",
					opt, len);
				result = false;
				break;
			}
			opt_len = options[1];
		}

		NET_DBG("opt: %hu, opt_len: %hu",
			(uint16_t)opt, (uint16_t)opt_len);

		if (opt_len < 2 || opt_len > len) {
			result = false;
			break;
		}

		switch (opt) {
		case NET_TCP_MSS_OPT:
			if (opt_len != 4) {
				result = false;
				goto end;
			}

			recv_options->mss =
				ntohs(UNALIGNED_GET((uint16_t *)(options + 2)));
			recv_options->mss_found = true;
			NET_DBG("MSS=%hu", recv_options->mss);
			break;
		case NET_TCP_WINDOW_SCALE_OPT:
			if (opt_len != 3) {
				result = false;
				goto end;
			}

			recv_options->window = opt;
			recv_options->wnd_found = true;
			break;
		default:
			continue;
		}
	}
end:
	if (false == result) {
		NET_WARN("Invalid TCP options");
	}

	return result;
}

static bool tcp_short_window(struct tcp *conn)
{
	int32_t threshold = MIN(conn_mss(conn), conn->recv_win_max / 2);

	if (conn->recv_win > threshold) {
		return false;
	}

	return true;
}

/**
 * @brief Update TCP receive window
 *
 * @param conn TCP network connection
 * @param delta Receive window delta
 *
 * @return 0 on success, -EINVAL
 *         if the receive window delta is out of bounds
 */
static int tcp_update_recv_wnd(struct tcp *conn, int32_t delta)
{
	int32_t new_win;
	bool short_win_before;
	bool short_win_after;

	new_win = conn->recv_win + delta;
	if (new_win < 0) {
		new_win = 0;
	} else if (new_win > conn->recv_win_max) {
		new_win = conn->recv_win_max;
	}

	short_win_before = tcp_short_window(conn);

	conn->recv_win = new_win;

	short_win_after = tcp_short_window(conn);

	if (short_win_before && !short_win_after &&
	    conn->state == TCP_ESTABLISHED) {
		k_work_cancel_delayable(&conn->ack_timer);
		tcp_out(conn, ACK);
	}

	return 0;
}

static size_t tcp_check_pending_data(struct tcp *conn, struct net_pkt *pkt,
				     size_t len)
{
	size_t pending_len = 0;

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT &&
	    !net_pkt_is_empty(conn->queue_recv_data)) {
		/* Some potentential cases:
		 * Note: MI = MAX_INT
		 * Packet | Queued| End off   | Gap size | Required handling
		 * Seq|Len|Seq|Len|           |          |
		 *  3 | 3 | 6 | 4 | 3+3-6=  0 | 6-3-3=0  | Append
		 *  3 | 4 | 6 | 4 | 3+4-6 = 1 | 6-3-4=-1 | Append, pull from queue
		 *  3 | 7 | 6 | 4 | 3+7-6 = 4 | 6-3-7=-4 | Drop queued data
		 *  3 | 8 | 6 | 4 | 3+8-6 = 5 | 6-3-8=-5 | Drop queued data
		 *  6 | 5 | 6 | 4 | 6+5-6 = 5 | 6-6-5=-5 | Drop queued data
		 *  6 | 4 | 6 | 4 | 6+4-6 = 4 | 6-6-4=-4 | Drop queued data / packet
		 * 10 | 2 | 6 | 4 | 10+2-6= 6 | 6-10-2=-6| Should not happen, dropping queue
		 *  7 | 4 | 6 | 4 | 7+4-6 = 5 | 6-7-4=-5 | Should not happen, dropping queue
		 * 11 | 2 | 6 | 4 | 11+2-6= 7 | 6-11-2=-7| Should not happen, dropping queue
		 *  2 | 3 | 6 | 4 | 2+3-6= MI | 6-2-3=1  | Keep queued data
		 */
		struct tcphdr *th = th_get(pkt);
		uint32_t expected_seq = th_seq(th) + len;
		uint32_t pending_seq;
		int32_t gap_size;
		uint32_t end_offset;

		pending_seq = tcp_get_seq(conn->queue_recv_data->buffer);
		end_offset = expected_seq - pending_seq;
		gap_size = (int32_t)(pending_seq - th_seq(th) - ((uint32_t)len));
		pending_len = net_pkt_get_len(conn->queue_recv_data);
		if (end_offset < pending_len) {
			if (end_offset) {
				net_pkt_remove_tail(pkt, end_offset);
				pending_len -= end_offset;
			}

			NET_DBG("Found pending data seq %u len %zd",
				expected_seq, pending_len);

			net_buf_frag_add(pkt->buffer,
					 conn->queue_recv_data->buffer);
			conn->queue_recv_data->buffer = NULL;

			k_work_cancel_delayable(&conn->recv_queue_timer);
		} else {
			/* Check if the queued data is just a section of the incoming data */
			if (gap_size <= 0) {
				net_buf_unref(conn->queue_recv_data->buffer);
				conn->queue_recv_data->buffer = NULL;

				k_work_cancel_delayable(&conn->recv_queue_timer);
			}

			pending_len = 0;
		}
	}

	return pending_len;
}

static enum net_verdict tcp_data_get(struct tcp *conn, struct net_pkt *pkt, size_t *len)
{
	enum net_verdict ret = NET_DROP;

	if (tcp_recv_cb) {
		tcp_recv_cb(conn, pkt);
		goto out;
	}

	if (conn->context->recv_cb) {
		/* If there is any out-of-order pending data, then pass it
		 * to the application here.
		 */
		*len += tcp_check_pending_data(conn, pkt, *len);

		net_pkt_cursor_init(pkt);
		net_pkt_set_overwrite(pkt, true);

		net_pkt_skip(pkt, net_pkt_get_len(pkt) - *len);

		tcp_update_recv_wnd(conn, -*len);

		/* Do not pass data to application with TCP conn
		 * locked as there could be an issue when the app tries
		 * to send the data and the conn is locked. So the recv
		 * data is placed in fifo which is flushed in tcp_in()
		 * after unlocking the conn
		 */
		k_fifo_put(&conn->recv_data, pkt);

		ret = NET_OK;
	}
 out:
	return ret;
}

static int tcp_finalize_pkt(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		return net_ipv4_finalize(pkt, IPPROTO_TCP);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		return net_ipv6_finalize(pkt, IPPROTO_TCP);
	}

	return -EINVAL;
}

static int tcp_header_add(struct tcp *conn, struct net_pkt *pkt, uint8_t flags,
			  uint32_t seq)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct tcphdr);
	struct tcphdr *th;

	th = (struct tcphdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!th) {
		return -ENOBUFS;
	}

	memset(th, 0, sizeof(struct tcphdr));

	UNALIGNED_PUT(conn->src.sin.sin_port, &th->th_sport);
	UNALIGNED_PUT(conn->dst.sin.sin_port, &th->th_dport);
	th->th_off = 5;

	if (conn->send_options.mss_found) {
		th->th_off++;
	}

	UNALIGNED_PUT(flags, &th->th_flags);
	UNALIGNED_PUT(htons(conn->recv_win), &th->th_win);
	UNALIGNED_PUT(htonl(seq), &th->th_seq);

	if (ACK & flags) {
		UNALIGNED_PUT(htonl(conn->ack), &th->th_ack);
	}

	return net_pkt_set_data(pkt, &tcp_access);
}

static int ip_header_add(struct tcp *conn, struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		return net_context_create_ipv4_new(conn->context, pkt,
						&conn->src.sin.sin_addr,
						&conn->dst.sin.sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		return net_context_create_ipv6_new(conn->context, pkt,
						&conn->src.sin6.sin6_addr,
						&conn->dst.sin6.sin6_addr);
	}

	return -EINVAL;
}

static int set_tcp_nodelay(struct tcp *conn, const void *value, size_t len)
{
	int no_delay_int;

	if (len != sizeof(int)) {
		return -EINVAL;
	}

	no_delay_int = *(int *)value;

	if ((no_delay_int < 0) || (no_delay_int > 1)) {
		return -EINVAL;
	}

	conn->tcp_nodelay = (bool)no_delay_int;

	return 0;
}

static int get_tcp_nodelay(struct tcp *conn, void *value, size_t *len)
{
	int no_delay_int = (int)conn->tcp_nodelay;

	*((int *)value) = no_delay_int;

	if (len) {
		*len = sizeof(int);
	}
	return 0;
}

static int net_tcp_set_mss_opt(struct tcp *conn, struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(mss_opt_access, struct tcp_mss_option);
	struct tcp_mss_option *mss;
	uint32_t recv_mss;

	mss = net_pkt_get_data(pkt, &mss_opt_access);
	if (!mss) {
		return -ENOBUFS;
	}

	recv_mss = net_tcp_get_supported_mss(conn);
	recv_mss |= (NET_TCP_MSS_OPT << 24) | (NET_TCP_MSS_SIZE << 16);

	UNALIGNED_PUT(htonl(recv_mss), (uint32_t *)mss);

	return net_pkt_set_data(pkt, &mss_opt_access);
}

static bool is_destination_local(struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_is_addr_loopback(
				(struct in_addr *)NET_IPV4_HDR(pkt)->dst) ||
		    net_ipv4_is_my_addr(
				(struct in_addr *)NET_IPV4_HDR(pkt)->dst)) {
			return true;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_is_addr_loopback(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->dst) ||
		    net_ipv6_is_my_addr(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
			return true;
		}
	}

	return false;
}

void net_tcp_reply_rst(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access_rst, struct tcphdr);
	struct tcphdr *th_pkt = th_get(pkt);
	struct tcphdr *th_rst;
	struct net_pkt *rst;
	int ret;

	if (th_pkt == NULL || (th_flags(th_pkt) & RST)) {
		/* Don't reply to a RST segment. */
		return;
	}

	rst = tcp_pkt_alloc_no_conn(pkt->iface, pkt->family,
				    sizeof(struct tcphdr));
	if (rst == NULL) {
		return;
	}

	/* IP header */
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		ret = net_ipv4_create(rst,
				      (struct in_addr *)NET_IPV4_HDR(pkt)->dst,
				      (struct in_addr *)NET_IPV4_HDR(pkt)->src);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		ret =  net_ipv6_create(rst,
				      (struct in6_addr *)NET_IPV6_HDR(pkt)->dst,
				      (struct in6_addr *)NET_IPV6_HDR(pkt)->src);
	} else {
		ret = -EINVAL;
	}

	if (ret < 0) {
		goto err;
	}

	/* TCP header */
	th_rst = (struct tcphdr *)net_pkt_get_data(rst, &tcp_access_rst);
	if (th_rst == NULL) {
		goto err;
	}

	memset(th_rst, 0, sizeof(struct tcphdr));

	UNALIGNED_PUT(th_pkt->th_dport, &th_rst->th_sport);
	UNALIGNED_PUT(th_pkt->th_sport, &th_rst->th_dport);
	th_rst->th_off = 5;

	if (th_flags(th_pkt) & ACK) {
		UNALIGNED_PUT(RST, &th_rst->th_flags);
		UNALIGNED_PUT(th_pkt->th_ack, &th_rst->th_seq);
	} else {
		uint32_t ack = ntohl(th_pkt->th_seq) + tcp_data_len(pkt);

		UNALIGNED_PUT(RST | ACK, &th_rst->th_flags);
		UNALIGNED_PUT(htonl(ack), &th_rst->th_ack);
	}

	ret = net_pkt_set_data(rst, &tcp_access_rst);
	if (ret < 0) {
		goto err;
	}

	ret = tcp_finalize_pkt(rst);
	if (ret < 0) {
		goto err;
	}

	NET_DBG("%s", tcp_th(rst));

	tcp_send(rst);

	return;

err:
	tcp_pkt_unref(rst);
}

static int tcp_out_ext(struct tcp *conn, uint8_t flags, struct net_pkt *data,
		       uint32_t seq)
{
	size_t alloc_len = sizeof(struct tcphdr);
	struct net_pkt *pkt;
	int ret = 0;

	if (conn->send_options.mss_found) {
		alloc_len += sizeof(uint32_t);
	}

	pkt = tcp_pkt_alloc(conn, alloc_len);
	if (!pkt) {
		ret = -ENOBUFS;
		goto out;
	}

	if (data) {
		/* Append the data buffer to the pkt */
		net_pkt_append_buffer(pkt, data->buffer);
		data->buffer = NULL;
	}

	ret = ip_header_add(conn, pkt);
	if (ret < 0) {
		tcp_pkt_unref(pkt);
		goto out;
	}

	ret = tcp_header_add(conn, pkt, flags, seq);
	if (ret < 0) {
		tcp_pkt_unref(pkt);
		goto out;
	}

	if (conn->send_options.mss_found) {
		ret = net_tcp_set_mss_opt(conn, pkt);
		if (ret < 0) {
			tcp_pkt_unref(pkt);
			goto out;
		}
	}

	ret = tcp_finalize_pkt(pkt);
	if (ret < 0) {
		tcp_pkt_unref(pkt);
		goto out;
	}

	if (tcp_send_cb) {
		ret = tcp_send_cb(pkt);
		goto out;
	}

	sys_slist_append(&conn->send_queue, &pkt->next);

	if (is_destination_local(pkt)) {
		/* If the destination is local, we have to let the current
		 * thread to finish with any state-machine changes before
		 * sending the packet, or it might lead to state inconsistencies
		 */
		k_work_schedule_for_queue(&tcp_work_q,
					  &conn->send_timer, K_NO_WAIT);
	} else if (tcp_send_process_no_lock(conn)) {
		tcp_conn_close(conn, -ETIMEDOUT);
	}
out:
	return ret;
}

static void tcp_out(struct tcp *conn, uint8_t flags)
{
	(void)tcp_out_ext(conn, flags, NULL /* no data */, conn->seq);
}

static int tcp_pkt_pull(struct net_pkt *pkt, size_t len)
{
	int total = net_pkt_get_len(pkt);
	int ret = 0;

	if (len > total) {
		ret = -EINVAL;
		goto out;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_pull(pkt, len);
	net_pkt_trim_buffer(pkt);
 out:
	return ret;
}

static int tcp_pkt_peek(struct net_pkt *to, struct net_pkt *from, size_t pos,
			size_t len)
{
	net_pkt_cursor_init(to);
	net_pkt_cursor_init(from);

	if (pos) {
		net_pkt_set_overwrite(from, true);
		net_pkt_skip(from, pos);
	}

	return net_pkt_copy(to, from, len);
}

static int tcp_pkt_append(struct net_pkt *pkt, const uint8_t *data, size_t len)
{
	size_t alloc_len = len;
	struct net_buf *buf = NULL;
	int ret = 0;

	if (pkt->buffer) {
		buf = net_buf_frag_last(pkt->buffer);

		if (len > net_buf_tailroom(buf)) {
			alloc_len -= net_buf_tailroom(buf);
		} else {
			alloc_len = 0;
		}
	}

	if (alloc_len > 0) {
		ret = net_pkt_alloc_buffer_raw(pkt, alloc_len,
					       TCP_PKT_ALLOC_TIMEOUT);
		if (ret < 0) {
			return -ENOBUFS;
		}
	}

	if (buf == NULL) {
		buf = pkt->buffer;
	}

	while (buf != NULL && len > 0) {
		size_t write_len = MIN(len, net_buf_tailroom(buf));

		net_buf_add_mem(buf, data, write_len);

		data += write_len;
		len -= write_len;
		buf = buf->frags;
	}

	NET_ASSERT(len == 0, "Not all bytes written");

	return ret;
}

static bool tcp_window_full(struct tcp *conn)
{
	bool window_full = (conn->send_data_total >= conn->send_win);

#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE
	window_full = window_full || (conn->send_data_total >= conn->ca.cwnd);
#endif

	if (window_full) {
		NET_DBG("conn: %p TX window_full", conn);
	}

	return window_full;
}

static int tcp_unsent_len(struct tcp *conn)
{
	int unsent_len;

	if (conn->unacked_len > conn->send_data_total) {
		NET_ERR("total=%zu, unacked_len=%d",
			conn->send_data_total, conn->unacked_len);
		unsent_len = -ERANGE;
		goto out;
	}

	unsent_len = conn->send_data_total - conn->unacked_len;
	if (conn->unacked_len >= conn->send_win) {
		unsent_len = 0;
	} else {
		unsent_len = MIN(unsent_len, conn->send_win - conn->unacked_len);

#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE
		if (conn->unacked_len >= conn->ca.cwnd) {
			unsent_len = 0;
		} else {
			unsent_len = MIN(unsent_len, conn->ca.cwnd - conn->unacked_len);
		}
#endif
	}
 out:
	return unsent_len;
}

static int tcp_send_data(struct tcp *conn)
{
	int ret = 0;
	int len;
	struct net_pkt *pkt;

	len = MIN(tcp_unsent_len(conn), conn_mss(conn));
	if (len < 0) {
		ret = len;
		goto out;
	}
	if (len == 0) {
		NET_DBG("conn: %p no data to send", conn);
		ret = -ENODATA;
		goto out;
	}

	pkt = tcp_pkt_alloc(conn, len);
	if (!pkt) {
		NET_ERR("conn: %p packet allocation failed, len=%d", conn, len);
		ret = -ENOBUFS;
		goto out;
	}

	ret = tcp_pkt_peek(pkt, conn->send_data, conn->unacked_len, len);
	if (ret < 0) {
		tcp_pkt_unref(pkt);
		ret = -ENOBUFS;
		goto out;
	}

	ret = tcp_out_ext(conn, PSH | ACK, pkt, conn->seq + conn->unacked_len);
	if (ret == 0) {
		conn->unacked_len += len;

		if (conn->data_mode == TCP_DATA_MODE_RESEND) {
			net_stats_update_tcp_resent(conn->iface, len);
			net_stats_update_tcp_seg_rexmit(conn->iface);
		} else {
			net_stats_update_tcp_sent(conn->iface, len);
			net_stats_update_tcp_seg_sent(conn->iface);
		}
	}

	/* The data we want to send, has been moved to the send queue so we
	 * can unref the head net_pkt. If there was an error, we need to remove
	 * the packet anyway.
	 */
	tcp_pkt_unref(pkt);

	conn_send_data_dump(conn);

 out:
	return ret;
}

/* Send all queued but unsent data from the send_data packet by packet
 * until the receiver's window is full. */
static int tcp_send_queued_data(struct tcp *conn)
{
	int ret = 0;
	bool subscribe = false;

	if (conn->data_mode == TCP_DATA_MODE_RESEND) {
		goto out;
	}

	while (tcp_unsent_len(conn) > 0) {
		/* Implement Nagle's algorithm */
		if ((conn->tcp_nodelay == false) && (conn->unacked_len > 0)) {
			/* If there is already pending data */
			if (tcp_unsent_len(conn) < conn_mss(conn)) {
				/* The number of bytes to be transmitted is less than an MSS,
				 * skip transmission for now.
				 * Wait for more data to be transmitted or all pending data
				 * being acknowledged.
				 */
				break;
			}
		}

		ret = tcp_send_data(conn);
		if (ret < 0) {
			break;
		}
	}

	if (conn->send_data_total) {
		subscribe = true;
	}

	if (k_work_delayable_remaining_get(&conn->send_data_timer)) {
		subscribe = false;
	}

	if (subscribe) {
		conn->send_data_retries = 0;
		k_work_reschedule_for_queue(&tcp_work_q, &conn->send_data_timer,
					    K_MSEC(TCP_RTO_MS));
	}
 out:
	return ret;
}

static void tcp_cleanup_recv_queue(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, recv_queue_timer);

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("Cleanup recv queue conn %p len %zd seq %u", conn,
		net_pkt_get_len(conn->queue_recv_data),
		tcp_get_seq(conn->queue_recv_data->buffer));

	net_buf_unref(conn->queue_recv_data->buffer);
	conn->queue_recv_data->buffer = NULL;

	k_mutex_unlock(&conn->lock);
}

static void tcp_resend_data(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, send_data_timer);
	bool conn_unref = false;
	int ret;
	int exp_tcp_rto;

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("send_data_retries=%hu", conn->send_data_retries);

	if (conn->send_data_retries >= tcp_retries) {
		NET_DBG("conn: %p close, data retransmissions exceeded", conn);
		conn_unref = true;
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_TCP_CONGESTION_AVOIDANCE) &&
	    (conn->send_data_retries == 0)) {
		tcp_ca_timeout(conn);
		if (tcp_window_full(conn)) {
			(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
		}
	}

	conn->data_mode = TCP_DATA_MODE_RESEND;
	conn->unacked_len = 0;

	ret = tcp_send_data(conn);
	conn->send_data_retries++;
	if (ret == 0) {
		if (conn->in_close && conn->send_data_total == 0) {
			NET_DBG("TCP connection in %s close, "
				"not disposing yet (waiting %dms)",
				"active", tcp_max_timeout_ms);
			k_work_reschedule_for_queue(&tcp_work_q,
						    &conn->fin_timer,
						    FIN_TIMEOUT);

			conn_state(conn, TCP_FIN_WAIT_1);

			ret = tcp_out_ext(conn, FIN | ACK, NULL,
					  conn->seq + conn->unacked_len);
			if (ret == 0) {
				conn_seq(conn, + 1);
			}

			keep_alive_timer_stop(conn);

			goto out;
		}
	} else if (ret == -ENODATA) {
		conn->data_mode = TCP_DATA_MODE_SEND;

		goto out;
	} else if (ret == -ENOBUFS) {
		NET_ERR("TCP failed to allocate buffer in retransmission");
	}

	exp_tcp_rto = TCP_RTO_MS;
	/* The last retransmit does not need to wait that long */
	if (conn->send_data_retries < tcp_retries) {
		/* Every retransmit, the retransmission timeout increases by a factor 1.5 */
		for (int i = 0; i < conn->send_data_retries; i++) {
			exp_tcp_rto += exp_tcp_rto >> 1;
		}
	}

	k_work_reschedule_for_queue(&tcp_work_q, &conn->send_data_timer,
				    K_MSEC(exp_tcp_rto));

 out:
	k_mutex_unlock(&conn->lock);

	if (conn_unref) {
		tcp_conn_close(conn, -ETIMEDOUT);
	}
}

static void tcp_timewait_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, timewait_timer);

	/* no need to acquire the conn->lock as there is nothing scheduled here */
	NET_DBG("conn: %p %s", conn, tcp_conn_state(conn, NULL));

	(void)tcp_conn_close(conn, -ETIMEDOUT);
}

static void tcp_establish_timeout(struct tcp *conn)
{
	NET_DBG("Did not receive %s in %dms", "ACK", ACK_TIMEOUT_MS);
	NET_DBG("conn: %p %s", conn, tcp_conn_state(conn, NULL));

	(void)tcp_conn_close(conn, -ETIMEDOUT);
}

static void tcp_fin_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, fin_timer);

	/* no need to acquire the conn->lock as there is nothing scheduled here */
	if (conn->state == TCP_SYN_RECEIVED) {
		tcp_establish_timeout(conn);
		return;
	}

	NET_DBG("Did not receive %s in %dms", "FIN", tcp_max_timeout_ms);
	NET_DBG("conn: %p %s", conn, tcp_conn_state(conn, NULL));

	(void)tcp_conn_close(conn, -ETIMEDOUT);
}

static void tcp_last_ack_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, fin_timer);

	NET_DBG("Did not receive %s in %dms", "last ACK", LAST_ACK_TIMEOUT_MS);
	NET_DBG("conn: %p %s", conn, tcp_conn_state(conn, NULL));

	(void)tcp_conn_close(conn, -ETIMEDOUT);
}

static void tcp_setup_last_ack_timer(struct tcp *conn)
{
	/* Just in case the last ack is lost, install a timer that will
	 * close the connection in that case. Use the fin_timer for that
	 * as the fin handling cannot be done in this passive close state.
	 * Instead of default tcp_fin_timeout() function, have a separate
	 * function to catch this last ack case.
	 */
	k_work_init_delayable(&conn->fin_timer, tcp_last_ack_timeout);

	NET_DBG("TCP connection in %s close, "
		"not disposing yet (waiting %dms)",
		"passive", LAST_ACK_TIMEOUT_MS);
	k_work_reschedule_for_queue(&tcp_work_q,
				    &conn->fin_timer,
				    LAST_ACK_TIMEOUT);
}

static void tcp_cancel_last_ack_timer(struct tcp *conn)
{
	k_work_cancel_delayable(&conn->fin_timer);
}

#if defined(CONFIG_NET_TCP_KEEPALIVE)
static void tcp_send_keepalive_probe(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, keepalive_timer);

	if (conn->state != TCP_ESTABLISHED) {
		NET_DBG("conn: %p TCP connection not established", conn);
		return;
	}

	if (!conn->keep_alive) {
		NET_DBG("conn: %p keepalive is not enabled", conn);
		return;
	}

	conn->keep_cur++;
	if (conn->keep_cur > conn->keep_cnt) {
		NET_DBG("conn: %p keepalive probe failed multiple times",
			conn);
		tcp_conn_close(conn, -ETIMEDOUT);
		return;
	}

	NET_DBG("conn: %p keepalive probe", conn);
	k_work_reschedule_for_queue(&tcp_work_q, &conn->keepalive_timer,
				    K_SECONDS(conn->keep_intvl));


	(void)tcp_out_ext(conn, ACK, NULL, conn->seq - 1);
}
#endif /* CONFIG_NET_TCP_KEEPALIVE */

static void tcp_send_zwp(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, persist_timer);

	k_mutex_lock(&conn->lock, K_FOREVER);

	(void)tcp_out_ext(conn, ACK, NULL, conn->seq - 1);

	tcp_derive_rto(conn);

	if (conn->send_win == 0) {
		uint64_t timeout = TCP_RTO_MS;

		/* Make sure the bitwise shift does not result in undefined behaviour */
		if (conn->zwp_retries < 63) {
			conn->zwp_retries++;
		}

		timeout <<= conn->zwp_retries;
		if (timeout == 0 || timeout > ZWP_MAX_DELAY_MS) {
			timeout = ZWP_MAX_DELAY_MS;
		}

		(void)k_work_reschedule_for_queue(
			&tcp_work_q, &conn->persist_timer, K_MSEC(timeout));
	}

	k_mutex_unlock(&conn->lock);
}

static void tcp_send_ack(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tcp *conn = CONTAINER_OF(dwork, struct tcp, ack_timer);

	k_mutex_lock(&conn->lock, K_FOREVER);

	tcp_out(conn, ACK);

	k_mutex_unlock(&conn->lock);
}

static void tcp_conn_ref(struct tcp *conn)
{
	int ref_count = atomic_inc(&conn->ref_count) + 1;

	NET_DBG("conn: %p, ref_count: %d", conn, ref_count);
}

static struct tcp *tcp_conn_alloc(void)
{
	struct tcp *conn = NULL;
	int ret;

	ret = k_mem_slab_alloc(&tcp_conns_slab, (void **)&conn, K_NO_WAIT);
	if (ret) {
		NET_ERR("Cannot allocate slab");
		goto out;
	}

	memset(conn, 0, sizeof(*conn));

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT) {
		conn->queue_recv_data = tcp_rx_pkt_alloc(conn, 0);
		if (conn->queue_recv_data == NULL) {
			NET_ERR("Cannot allocate %s queue for conn %p", "recv",
				conn);
			goto fail;
		}
	}

	conn->send_data = tcp_pkt_alloc(conn, 0);
	if (conn->send_data == NULL) {
		NET_ERR("Cannot allocate %s queue for conn %p", "send", conn);
		goto fail;
	}

	k_mutex_init(&conn->lock);
	k_fifo_init(&conn->recv_data);
	k_sem_init(&conn->connect_sem, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&conn->tx_sem, 1, 1);

	conn->in_connect = false;
	conn->state = TCP_LISTEN;
	conn->recv_win_max = tcp_rx_window;
	conn->recv_win = conn->recv_win_max;
	conn->send_win_max = MAX(tcp_tx_window, NET_IPV6_MTU);
	conn->send_win = conn->send_win_max;
	conn->tcp_nodelay = false;
	conn->addr_ref_done = false;
#ifdef CONFIG_NET_TCP_FAST_RETRANSMIT
	conn->dup_ack_cnt = 0;
#endif
#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE
	/* Initially set the congestion window at its max size, since only the MSS
	 * is available as soon as the connection is established
	 */
	conn->ca.cwnd = UINT16_MAX;
#endif

	/* The ISN value will be set when we get the connection attempt or
	 * when trying to create a connection.
	 */
	conn->seq = 0U;

	sys_slist_init(&conn->send_queue);

	k_work_init_delayable(&conn->send_timer, tcp_send_process);
	k_work_init_delayable(&conn->timewait_timer, tcp_timewait_timeout);
	k_work_init_delayable(&conn->fin_timer, tcp_fin_timeout);
	k_work_init_delayable(&conn->send_data_timer, tcp_resend_data);
	k_work_init_delayable(&conn->recv_queue_timer, tcp_cleanup_recv_queue);
	k_work_init_delayable(&conn->persist_timer, tcp_send_zwp);
	k_work_init_delayable(&conn->ack_timer, tcp_send_ack);
	k_work_init(&conn->conn_release, tcp_conn_release);
	keep_alive_timer_init(conn);

	tcp_conn_ref(conn);

	k_mutex_lock(&tcp_lock, K_FOREVER);
	sys_slist_append(&tcp_conns, &conn->next);
	k_mutex_unlock(&tcp_lock);
out:
	NET_DBG("conn: %p", conn);

	return conn;

fail:
	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT && conn->queue_recv_data) {
		tcp_pkt_unref(conn->queue_recv_data);
		conn->queue_recv_data = NULL;
	}

	k_mem_slab_free(&tcp_conns_slab, (void *)conn);
	return NULL;
}

int net_tcp_get(struct net_context *context)
{
	int ret = 0;
	struct tcp *conn;

	conn = tcp_conn_alloc();
	if (conn == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	/* Mutually link the net_context and tcp connection */
	conn->context = context;
	context->tcp = conn;

	return ret;
}

static bool tcp_endpoint_cmp(union tcp_endpoint *ep, struct net_pkt *pkt,
			     enum pkt_addr which)
{
	union tcp_endpoint ep_tmp;

	if (tcp_endpoint_set(&ep_tmp, pkt, which) < 0) {
		return false;
	}

	return !memcmp(ep, &ep_tmp, tcp_endpoint_len(ep->sa.sa_family));
}

static bool tcp_conn_cmp(struct tcp *conn, struct net_pkt *pkt)
{
	return tcp_endpoint_cmp(&conn->src, pkt, TCP_EP_DST) &&
		tcp_endpoint_cmp(&conn->dst, pkt, TCP_EP_SRC);
}

static struct tcp *tcp_conn_search(struct net_pkt *pkt)
{
	bool found = false;
	struct tcp *conn;
	struct tcp *tmp;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp_conns, conn, tmp, next) {
		found = tcp_conn_cmp(conn, pkt);
		if (found) {
			break;
		}
	}

	k_mutex_unlock(&tcp_lock);

	return found ? conn : NULL;
}

static struct tcp *tcp_conn_new(struct net_pkt *pkt);

static enum net_verdict tcp_recv(struct net_conn *net_conn,
				 struct net_pkt *pkt,
				 union net_ip_header *ip,
				 union net_proto_header *proto,
				 void *user_data)
{
	struct tcp *conn;
	struct tcphdr *th;
	enum net_verdict verdict = NET_DROP;

	ARG_UNUSED(net_conn);
	ARG_UNUSED(proto);

	conn = tcp_conn_search(pkt);
	if (conn) {
		goto in;
	}

	th = th_get(pkt);

	if (th_flags(th) & SYN && !(th_flags(th) & ACK)) {
		struct tcp *conn_old = ((struct net_context *)user_data)->tcp;

		conn = tcp_conn_new(pkt);
		if (!conn) {
			NET_ERR("Cannot allocate a new TCP connection");
			goto in;
		}

		conn->accepted_conn = conn_old;
	}
in:
	if (conn) {
		verdict = tcp_in(conn, pkt);
	} else {
		net_tcp_reply_rst(pkt);
	}

	return verdict;
}

static uint32_t seq_scale(uint32_t seq)
{
	return seq + (k_ticks_to_ns_floor32(k_uptime_ticks()) >> 6);
}

static uint8_t unique_key[16]; /* MD5 128 bits as described in RFC6528 */

static uint32_t tcpv6_init_isn(struct in6_addr *saddr,
			       struct in6_addr *daddr,
			       uint16_t sport,
			       uint16_t dport)
{
	struct {
		uint8_t key[sizeof(unique_key)];
		struct in6_addr saddr;
		struct in6_addr daddr;
		uint16_t sport;
		uint16_t dport;
	} buf = {
		.saddr = *(struct in6_addr *)saddr,
		.daddr = *(struct in6_addr *)daddr,
		.sport = sport,
		.dport = dport
	};

	uint8_t hash[16];
	static bool once;

	if (!once) {
		sys_rand_get(unique_key, sizeof(unique_key));
		once = true;
	}

	memcpy(buf.key, unique_key, sizeof(buf.key));

#if defined(CONFIG_NET_TCP_ISN_RFC6528)
	mbedtls_md5((const unsigned char *)&buf, sizeof(buf), hash);
#endif

	return seq_scale(UNALIGNED_GET((uint32_t *)&hash[0]));
}

static uint32_t tcpv4_init_isn(struct in_addr *saddr,
			       struct in_addr *daddr,
			       uint16_t sport,
			       uint16_t dport)
{
	struct {
		uint8_t key[sizeof(unique_key)];
		struct in_addr saddr;
		struct in_addr daddr;
		uint16_t sport;
		uint16_t dport;
	} buf = {
		.saddr = *(struct in_addr *)saddr,
		.daddr = *(struct in_addr *)daddr,
		.sport = sport,
		.dport = dport
	};

	uint8_t hash[16];
	static bool once;

	if (!once) {
		sys_rand_get(unique_key, sizeof(unique_key));
		once = true;
	}

	memcpy(buf.key, unique_key, sizeof(unique_key));

#if defined(CONFIG_NET_TCP_ISN_RFC6528)
	mbedtls_md5((const unsigned char *)&buf, sizeof(buf), hash);
#endif

	return seq_scale(UNALIGNED_GET((uint32_t *)&hash[0]));
}

static uint32_t tcp_init_isn(struct sockaddr *saddr, struct sockaddr *daddr)
{
	if (IS_ENABLED(CONFIG_NET_TCP_ISN_RFC6528)) {
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    saddr->sa_family == AF_INET6) {
			return tcpv6_init_isn(&net_sin6(saddr)->sin6_addr,
					      &net_sin6(daddr)->sin6_addr,
					      net_sin6(saddr)->sin6_port,
					      net_sin6(daddr)->sin6_port);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
			   saddr->sa_family == AF_INET) {
			return tcpv4_init_isn(&net_sin(saddr)->sin_addr,
					      &net_sin(daddr)->sin_addr,
					      net_sin(saddr)->sin_port,
					      net_sin(daddr)->sin_port);
		}
	}

	return sys_rand32_get();
}

/* Create a new tcp connection, as a part of it, create and register
 * net_context
 */
static struct tcp *tcp_conn_new(struct net_pkt *pkt)
{
	struct tcp *conn = NULL;
	struct net_context *context = NULL;
	sa_family_t af = net_pkt_family(pkt);
	struct sockaddr local_addr = { 0 };
	int ret;

	ret = net_context_get(af, SOCK_STREAM, IPPROTO_TCP, &context);
	if (ret < 0) {
		NET_ERR("net_context_get(): %d", ret);
		goto err;
	}

	conn = context->tcp;
	conn->iface = pkt->iface;
	tcp_derive_rto(conn);

	net_context_set_family(conn->context, net_pkt_family(pkt));

	if (tcp_endpoint_set(&conn->dst, pkt, TCP_EP_SRC) < 0) {
		net_context_put(context);
		conn = NULL;
		goto err;
	}

	if (tcp_endpoint_set(&conn->src, pkt, TCP_EP_DST) < 0) {
		net_context_put(context);
		conn = NULL;
		goto err;
	}

	NET_DBG("conn: src: %s, dst: %s",
		net_sprint_addr(conn->src.sa.sa_family,
				(const void *)&conn->src.sin.sin_addr),
		net_sprint_addr(conn->dst.sa.sa_family,
				(const void *)&conn->dst.sin.sin_addr));

	memcpy(&context->remote, &conn->dst, sizeof(context->remote));
	context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	net_sin_ptr(&context->local)->sin_family = af;

	local_addr.sa_family = net_context_get_family(context);

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_context_get_family(context) == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				&conn->src.sin6.sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_context_get_family(context) == AF_INET) {
		net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				&conn->src.sin.sin_addr);
	}

	ret = net_context_bind(context, &local_addr, sizeof(local_addr));
	if (ret < 0) {
		NET_DBG("Cannot bind accepted context, connection reset");
		net_context_put(context);
		conn = NULL;
		goto err;
	}

	/* The newly created context object for the new TCP client connection needs
	 * all four parameters of the tuple (local address, local port, remote
	 * address, remote port) to be properly identified. Remote address and port
	 * are already copied above from conn->dst. The call to net_context_bind
	 * with the prepared local_addr further copies the local address. However,
	 * this call wont copy the local port, as the bind would then fail due to
	 * an address/port reuse without the REUSEPORT option enables for both
	 * connections. Therefore, we copy the port after the bind call.
	 * It is safe to bind to this address/port combination, as the new TCP
	 * client connection is separated from the local listening connection
	 * by the specified remote address and remote port.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_context_get_family(context) == AF_INET6) {
		net_sin6_ptr(&context->local)->sin6_port = conn->src.sin6.sin6_port;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_context_get_family(context) == AF_INET) {
		net_sin_ptr(&context->local)->sin_port = conn->src.sin.sin_port;
	}

	if (!(IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) ||
	      IS_ENABLED(CONFIG_NET_TEST))) {
		conn->seq = tcp_init_isn(&local_addr, &context->remote);
	}

	NET_DBG("context: local: %s, remote: %s",
		net_sprint_addr(local_addr.sa_family,
				(const void *)&net_sin(&local_addr)->sin_addr),
		net_sprint_addr(context->remote.sa_family,
				(const void *)&net_sin(&context->remote)->sin_addr));

	ret = net_conn_register(IPPROTO_TCP, af,
				&context->remote, &local_addr,
				ntohs(conn->dst.sin.sin_port),/* local port */
				ntohs(conn->src.sin.sin_port),/* remote port */
				context, tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		NET_ERR("net_conn_register(): %d", ret);
		net_context_put(context);
		conn = NULL;
		goto err;
	}

	net_if_addr_ref(conn->iface, conn->dst.sa.sa_family,
			conn->src.sa.sa_family == AF_INET ?
			(const void *)&conn->src.sin.sin_addr :
			(const void *)&conn->src.sin6.sin6_addr);
	conn->addr_ref_done = true;

err:
	if (!conn) {
		net_stats_update_tcp_seg_conndrop(net_pkt_iface(pkt));
	}

	return conn;
}

static bool tcp_validate_seq(struct tcp *conn, struct tcphdr *hdr)
{
	return (net_tcp_seq_cmp(th_seq(hdr), conn->ack) >= 0) &&
		(net_tcp_seq_cmp(th_seq(hdr), conn->ack + conn->recv_win) < 0);
}

static int32_t tcp_compute_new_length(struct tcp *conn, struct tcphdr *hdr, size_t len,
				      bool fin_received)
{
	int32_t new_len = 0;

	if (len > 0) {
		/* Cases:
		 * - Data already received earlier: new_len <= 0
		 * - Partially new data new_len > 0
		 * - Out of order data new_len > 0,
		 *   should be checked by sequence number
		 */
		new_len = (int32_t)(len) - net_tcp_seq_cmp(conn->ack, th_seq(hdr));
		if (fin_received) {
			/* Add with one additional byte as the FIN flag has to be subtracted */
			new_len++;
		}
	}
	return new_len;
}

static enum tcp_state tcp_enter_time_wait(struct tcp *conn)
{
	tcp_send_timer_cancel(conn);
	/* Entering TIME-WAIT, so cancel the timer and start the TIME-WAIT timer */
	k_work_cancel_delayable(&conn->fin_timer);
	k_work_reschedule_for_queue(
		&tcp_work_q, &conn->timewait_timer,
		K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
	return TCP_TIME_WAIT;
}

static bool check_seq_list(struct net_buf *buf)
{
	struct net_buf *last = NULL;
	struct net_buf *tmp = buf;
	uint32_t seq;
	uint32_t next_seq = 0;
	bool result = true;

	while (tmp) {
		seq = tcp_get_seq(tmp);

		NET_DBG("buf %p seq %u len %d", tmp, seq, tmp->len);

		if (last != NULL) {
			if (next_seq != seq) {
				result = false;
			}
		}

		next_seq = seq + tmp->len;
		last = tmp;
		tmp = tmp->frags;
	}
	return result;
}

static void tcp_queue_recv_data(struct tcp *conn, struct net_pkt *pkt,
				size_t len, uint32_t seq)
{
	uint32_t seq_start = seq;
	bool inserted = false;
	struct net_buf *tmp;

	NET_DBG("conn: %p len %zd seq %u ack %u", conn, len, seq, conn->ack);

	tmp = pkt->buffer;

	tcp_set_seq(tmp, seq);
	seq += tmp->len;
	tmp = tmp->frags;

	while (tmp) {
		tcp_set_seq(tmp, seq);
		seq += tmp->len;
		tmp = tmp->frags;
	}

	if (IS_ENABLED(CONFIG_NET_TCP_LOG_LEVEL_DBG)) {
		NET_DBG("Queuing data: conn %p", conn);
	}

	if (!net_pkt_is_empty(conn->queue_recv_data)) {
		/* Place the data to correct place in the list. If the data
		 * would not be sequential, then drop this packet.
		 *
		 * Only work with subtractions between sequence numbers in uint32_t format
		 * to proper handle cases that are around the wrapping point.
		 */

		/* Some potentential cases:
		 * Note: MI = MAX_INT
		 * Packet | Queued| End off1  | Start off| End off2    | Required handling
		 * Seq|Len|Seq|Len|           |          |             |
		 *  3 | 3 | 6 | 4 | 3+3-6=  0 | NA       | NA          | Prepend
		 *  3 | 4 | 6 | 4 | 3+4-6 = 1 | NA       | NA          | Prepend, pull from buffer
		 *  3 | 7 | 6 | 4 | 3+7-6 = 4 | 6-3=3    | 6+4-3=7     | Drop queued data
		 *  3 | 8 | 6 | 4 | 3+8-6 = 5 | 6-3=3    | 6+4-3=7     | Drop queued data
		 *  6 | 5 | 6 | 4 | 6+5-6 = 5 | 6-6=0    | 6+4-6=4     | Drop queued data
		 *  6 | 4 | 6 | 4 | 6+4-6 = 4 | 6-6=0    | 6+4-6=4     | Drop queued data / packet
		 *  7 | 2 | 6 | 4 | 7+2-6 = 3 | 6-7=MI   | 6+4-7=3     | Drop packet
		 * 10 | 2 | 6 | 4 | 10+2-6= 6 | 6-10=MI-3| 6+4-10=0    | Append
		 *  7 | 4 | 6 | 4 | 7+4-6 = 5 | 6-7 =MI  | 6+4-7 =3    | Append, pull from packet
		 * 11 | 2 | 6 | 4 | 11+2-6= 7 | 6-11=MI-6| 6+4-11=MI-1 | Drop incoming packet
		 *  2 | 3 | 6 | 4 | 2+3-6= MI | 6-2=4    | 6+4-2=8     | Drop incoming packet
		 */

		uint32_t pending_seq;
		uint32_t start_offset;
		uint32_t end_offset;
		size_t pending_len;

		pending_seq = tcp_get_seq(conn->queue_recv_data->buffer);
		end_offset = seq - pending_seq;
		pending_len = net_pkt_get_len(conn->queue_recv_data);
		if (end_offset < pending_len) {
			if (end_offset < len) {
				if (end_offset) {
					net_pkt_remove_tail(pkt, end_offset);
				}

				/* Put new data before the pending data */
				net_buf_frag_add(pkt->buffer,
						 conn->queue_recv_data->buffer);
				NET_DBG("Adding at before queue, end_offset %i, pending_len %zu",
					end_offset, pending_len);
				conn->queue_recv_data->buffer = pkt->buffer;
				inserted = true;
			}
		} else {
			struct net_buf *last;

			last = net_buf_frag_last(conn->queue_recv_data->buffer);
			pending_seq = tcp_get_seq(last);

			start_offset = pending_seq - seq_start;
			/* Compute the offset w.r.t. the start point of the new packet */
			end_offset = (pending_seq + last->len) - seq_start;

			/* Check if queue start with within the within the new packet */
			if ((start_offset < len) && (end_offset <= len)) {
				/* The queued data is irrelevant since the new packet overlaps the
				 * new packet, take the new packet as contents
				 */
				net_buf_unref(conn->queue_recv_data->buffer);
				conn->queue_recv_data->buffer = pkt->buffer;
				inserted = true;
			} else {
				if (end_offset < len) {
					if (end_offset) {
						net_pkt_remove_tail(conn->queue_recv_data,
								    end_offset);
					}

					/* Put new data after pending data */
					NET_DBG("Adding at end of queue, start %i, end %i, len %zu",
						start_offset, end_offset, len);
					net_buf_frag_add(conn->queue_recv_data->buffer,
							 pkt->buffer);
					inserted = true;
				}
			}
		}

		if (inserted) {
			NET_DBG("All pending data: conn %p", conn);
			if (check_seq_list(conn->queue_recv_data->buffer) == false) {
				NET_ERR("Incorrect order in out of order sequence for conn %p",
					conn);
				/* error in sequence list, drop it */
				net_buf_unref(conn->queue_recv_data->buffer);
				conn->queue_recv_data->buffer = NULL;
			}
		} else {
			NET_DBG("Cannot add new data to queue");
		}
	} else {
		net_pkt_append_buffer(conn->queue_recv_data, pkt->buffer);
		inserted = true;
	}

	if (inserted) {
		/* We need to keep the received data but free the pkt */
		pkt->buffer = NULL;

		if (!k_work_delayable_is_pending(&conn->recv_queue_timer)) {
			k_work_reschedule_for_queue(
				&tcp_work_q, &conn->recv_queue_timer,
				K_MSEC(CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT));
		}
	}
}

static enum net_verdict tcp_data_received(struct tcp *conn, struct net_pkt *pkt,
					  size_t *len)
{
	enum net_verdict ret;

	if (*len == 0) {
		return NET_DROP;
	}

	ret = tcp_data_get(conn, pkt, len);

	net_stats_update_tcp_seg_recv(conn->iface);
	conn_ack(conn, *len);

	/* Delay ACK response in case of small window or missing PSH,
	 * as described in RFC 813.
	 */
	if (tcp_short_window(conn)) {
		k_work_schedule_for_queue(&tcp_work_q, &conn->ack_timer,
					  ACK_DELAY);
	} else {
		k_work_cancel_delayable(&conn->ack_timer);
		tcp_out(conn, ACK);
	}

	return ret;
}

static void tcp_out_of_order_data(struct tcp *conn, struct net_pkt *pkt,
				  size_t data_len, uint32_t seq)
{
	size_t headers_len;

	if (data_len == 0) {
		return;
	}

	headers_len = net_pkt_get_len(pkt) - data_len;

	/* Get rid of protocol headers from the data */
	if (tcp_pkt_pull(pkt, headers_len) < 0) {
		return;
	}

	/* We received out-of-order data. Try to queue it.
	 */
	tcp_queue_recv_data(conn, pkt, data_len, seq);
}

static void tcp_check_sock_options(struct tcp *conn)
{
	int sndbuf_opt = 0;
	int rcvbuf_opt = 0;

	if (IS_ENABLED(CONFIG_NET_CONTEXT_SNDBUF)) {
		(void)net_context_get_option(conn->context, NET_OPT_SNDBUF,
					     &sndbuf_opt, NULL);
	}

	if (IS_ENABLED(CONFIG_NET_CONTEXT_RCVBUF)) {
		(void)net_context_get_option(conn->context, NET_OPT_RCVBUF,
					     &rcvbuf_opt, NULL);
	}

	if (sndbuf_opt > 0 && sndbuf_opt != conn->send_win_max) {
		k_mutex_lock(&conn->lock, K_FOREVER);

		conn->send_win_max = sndbuf_opt;
		if (conn->send_win > conn->send_win_max) {
			conn->send_win = conn->send_win_max;
		}

		k_mutex_unlock(&conn->lock);
	}

	if (rcvbuf_opt > 0 && rcvbuf_opt != conn->recv_win_max) {
		int diff;

		k_mutex_lock(&conn->lock, K_FOREVER);

		diff = rcvbuf_opt - conn->recv_win_max;
		conn->recv_win_max = rcvbuf_opt;
		tcp_update_recv_wnd(conn, diff);

		k_mutex_unlock(&conn->lock);
	}
}

/* TCP state machine, everything happens here */
static enum net_verdict tcp_in(struct tcp *conn, struct net_pkt *pkt)
{
	struct tcphdr *th = pkt ? th_get(pkt) : NULL;
	uint8_t next = 0, fl = 0;
	bool do_close = false;
	bool connection_ok = false;
	size_t tcp_options_len = th ? (th_off(th) - 5) * 4 : 0;
	struct net_conn *conn_handler = NULL;
	struct net_pkt *recv_pkt;
	void *recv_user_data;
	struct k_fifo *recv_data_fifo;
	size_t len;
	int ret;
	int close_status = 0;
	enum net_verdict verdict = NET_DROP;

	if (th) {
		/* Currently we ignore ECN and CWR flags */
		fl = th_flags(th) & ~(ECN | CWR);
	}

	if (conn->state != TCP_SYN_SENT) {
		tcp_check_sock_options(conn);
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	/* Connection context was already freed. */
	if (conn->state == TCP_UNUSED) {
		k_mutex_unlock(&conn->lock);
		return NET_DROP;
	}

	NET_DBG("%s", tcp_conn_state(conn, pkt));

	if (th && th_off(th) < 5) {
		tcp_out(conn, RST);
		do_close = true;
		close_status = -ECONNRESET;
		goto out;
	}

	if (FL(&fl, &, RST)) {
		/* We only accept RST packet that has valid seq field. */
		if (!tcp_validate_seq(conn, th)) {
			net_stats_update_tcp_seg_rsterr(net_pkt_iface(pkt));
			k_mutex_unlock(&conn->lock);
			return NET_DROP;
		}

		/* Valid RST received. */
		verdict = NET_OK;
		net_stats_update_tcp_seg_rst(net_pkt_iface(pkt));
		do_close = true;
		close_status = -ECONNRESET;

		/* If we receive RST and ACK for the sent SYN, it means
		 * that there is no socket listening the port we are trying
		 * to connect to. Set the errno properly in this case.
		 */
		if (conn->in_connect) {
			fl = th_flags(th);
			if (FL(&fl, ==, RST | ACK)) {
				close_status = -ECONNREFUSED;
			}
		}

		goto out;
	}

	if (tcp_options_len && !tcp_options_check(&conn->recv_options, pkt,
						  tcp_options_len)) {
		NET_DBG("DROP: Invalid TCP option list");
		tcp_out(conn, RST);
		do_close = true;
		close_status = -ECONNRESET;
		goto out;
	}

	if (th && (conn->state != TCP_LISTEN) && (conn->state != TCP_SYN_SENT) &&
	    tcp_validate_seq(conn, th) && FL(&fl, &, SYN)) {
		/* According to RFC 793, ch 3.9 Event Processing, receiving SYN
		 * once the connection has been established is an error
		 * condition, reset should be sent and connection closed.
		 */
		NET_DBG("conn: %p, SYN received in %s state, dropping connection",
			conn, tcp_state_to_str(conn->state, false));
		net_stats_update_tcp_seg_drop(conn->iface);
		tcp_out(conn, RST);
		do_close = true;
		close_status = -ECONNRESET;
		goto out;
	}

	if (th) {
		conn->send_win = ntohs(th_win(th));
		if (conn->send_win > conn->send_win_max) {
			NET_DBG("Lowering send window from %u to %u",
				conn->send_win, conn->send_win_max);

			conn->send_win = conn->send_win_max;
		}

		if (conn->send_win == 0) {
			if (!k_work_delayable_is_pending(&conn->persist_timer)) {
				conn->zwp_retries = 0;
				(void)k_work_reschedule_for_queue(
					&tcp_work_q, &conn->persist_timer,
					K_MSEC(TCP_RTO_MS));
			}
		} else {
			(void)k_work_cancel_delayable(&conn->persist_timer);
		}

		if (tcp_window_full(conn)) {
			(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
		} else {
			k_sem_give(&conn->tx_sem);
		}
	}

next_state:
	len = pkt ? tcp_data_len(pkt) : 0;

	switch (conn->state) {
	case TCP_LISTEN:
		if (FL(&fl, ==, SYN)) {
			/* Make sure our MSS is also sent in the ACK */
			conn->send_options.mss_found = true;
			conn_ack(conn, th_seq(th) + 1); /* capture peer's isn */
			tcp_out(conn, SYN | ACK);
			conn->send_options.mss_found = false;
			conn_seq(conn, + 1);
			next = TCP_SYN_RECEIVED;

			/* Close the connection if we do not receive ACK on time.
			 */
			k_work_reschedule_for_queue(&tcp_work_q,
						    &conn->establish_timer,
						    ACK_TIMEOUT);
			verdict = NET_OK;
		} else {
			conn->send_options.mss_found = true;
			tcp_out(conn, SYN);
			conn->send_options.mss_found = false;
			conn_seq(conn, + 1);
			next = TCP_SYN_SENT;
			tcp_conn_ref(conn);
		}
		break;
	case TCP_SYN_RECEIVED:
		if (FL(&fl, &, ACK, th_ack(th) == conn->seq &&
				th_seq(th) == conn->ack)) {
			net_tcp_accept_cb_t accept_cb = NULL;
			struct net_context *context = NULL;

			if (conn->accepted_conn != NULL) {
				accept_cb = conn->accepted_conn->accept_cb;
				context = conn->accepted_conn->context;
				keep_alive_param_copy(conn, conn->accepted_conn);
			}

			k_work_cancel_delayable(&conn->establish_timer);
			tcp_send_timer_cancel(conn);
			tcp_conn_ref(conn);
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);

			/* Make sure the accept_cb is only called once. */
			conn->accepted_conn = NULL;

			if (accept_cb == NULL) {
				/* In case of no accept_cb registered,
				 * application will not take ownership of the
				 * connection. To prevent connection leak, unref
				 * the TCP context and put the connection into
				 * active close (TCP_FIN_WAIT_1).
				 */
				net_tcp_put(conn->context);
				break;
			}

			keep_alive_timer_restart(conn);

			net_ipaddr_copy(&conn->context->remote, &conn->dst.sa);

			/* Check if v4-mapping-to-v6 needs to be done for
			 * the accepted socket.
			 */
			if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
			    net_context_get_family(conn->context) == AF_INET &&
			    net_context_get_family(context) == AF_INET6 &&
			    !net_context_is_v6only_set(context)) {
				struct in6_addr mapped;

				net_ipv6_addr_create_v4_mapped(
					&net_sin(&conn->context->remote)->sin_addr,
					&mapped);
				net_ipaddr_copy(&net_sin6(&conn->context->remote)->sin6_addr,
						&mapped);

				net_sin6(&conn->context->remote)->sin6_family = AF_INET6;

				NET_DBG("Setting v4 mapped address %s",
					net_sprint_ipv6_addr(&mapped));

				/* Note that we cannot set the local address to IPv6 one
				 * as that is used to match the connection, and not just
				 * for printing. The remote address is only used for
				 * passing it to accept() and printing it by "net conn"
				 * command.
				 */
			}

			accept_cb(conn->context, &conn->context->remote,
				  net_context_get_family(context) == AF_INET6 ?
				  sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in),
				  0, context);

			next = TCP_ESTABLISHED;

			tcp_ca_init(conn);

			if (len) {
				verdict = tcp_data_get(conn, pkt, &len);
				if (verdict == NET_OK) {
					/* net_pkt owned by the recv fifo now */
					pkt = NULL;
				}

				conn_ack(conn, + len);
				tcp_out(conn, ACK);
			} else {
				verdict = NET_OK;
			}

			/* ACK for SYN | ACK has been received. This signilizes that
			 * the connection makes a "forward progress".
			 */
			tcp_nbr_reachability_hint(conn);
		}
		break;
	case TCP_SYN_SENT:
		/* if we are in SYN SENT and receive only a SYN without an
		 * ACK , shouldn't we go to SYN RECEIVED state? See Figure
		 * 6 of RFC 793
		 */
		if (FL(&fl, &, SYN | ACK, th && th_ack(th) == conn->seq)) {
			tcp_send_timer_cancel(conn);
			conn_ack(conn, th_seq(th) + 1);
			if (len) {
				verdict = tcp_data_get(conn, pkt, &len);
				if (verdict == NET_OK) {
					/* net_pkt owned by the recv fifo now */
					pkt = NULL;
				}

				conn_ack(conn, + len);
			} else {
				verdict = NET_OK;
			}

			next = TCP_ESTABLISHED;
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);
			tcp_ca_init(conn);
			tcp_out(conn, ACK);
			keep_alive_timer_restart(conn);

			/* The connection semaphore is released *after*
			 * we have changed the connection state. This way
			 * the application can send data and it is queued
			 * properly even if this thread is running in lower
			 * priority.
			 */
			connection_ok = true;

			/* ACK for SYN has been received. This signilizes that
			 * the connection makes a "forward progress".
			 */
			tcp_nbr_reachability_hint(conn);
		} else if (pkt) {
			net_tcp_reply_rst(pkt);
		}

		break;
	case TCP_ESTABLISHED:
		/* full-close */
		if (th && FL(&fl, &, FIN, th_seq(th) == conn->ack)) {
			bool acked = false;

			if (len) {
				verdict = tcp_data_get(conn, pkt, &len);
				if (verdict == NET_OK) {
					/* net_pkt owned by the recv fifo now */
					pkt = NULL;
				}
			} else {
				verdict = NET_OK;
			}

			conn_ack(conn, + len + 1);
			keep_alive_timer_stop(conn);

			if (FL(&fl, &, ACK)) {
				acked = true;

				if (net_tcp_seq_cmp(th_ack(th), conn->seq) > 0) {
					uint32_t len_acked = th_ack(th) - conn->seq;

					conn_seq(conn, + len_acked);
				}
			}

			if (acked) {
				tcp_out(conn, FIN | ACK);
				conn_seq(conn, + 1);
				tcp_setup_last_ack_timer(conn);
				next = TCP_LAST_ACK;
			} else {
				tcp_out(conn, ACK);
				next = TCP_CLOSE_WAIT;
			}

			break;
		}

		/* Whatever we've received, we know that peer is alive, so reset
		 * the keepalive timer.
		 */
		keep_alive_timer_restart(conn);

#ifdef CONFIG_NET_TCP_FAST_RETRANSMIT
		if (th && (net_tcp_seq_cmp(th_ack(th), conn->seq) == 0)) {
			/* Only if there is pending data, increment the duplicate ack count */
			if (conn->send_data_total > 0) {
				/* There could be also payload, only without payload account them */
				if (len == 0) {
					/* Increment the duplicate acc counter,
					 * but maximize the value
					 */
					conn->dup_ack_cnt = MIN(conn->dup_ack_cnt + 1,
						DUPLICATE_ACK_RETRANSMIT_TRHESHOLD + 1);
					tcp_ca_dup_ack(conn);
				}
			} else {
				conn->dup_ack_cnt = 0;
			}

			/* Only do fast retransmit when not already in a resend state */
			if ((conn->data_mode == TCP_DATA_MODE_SEND) &&
			    (conn->dup_ack_cnt == DUPLICATE_ACK_RETRANSMIT_TRHESHOLD)) {
				/* Apply a fast retransmit */
				int temp_unacked_len = conn->unacked_len;

				conn->unacked_len = 0;

				(void)tcp_send_data(conn);

				/* Restore the current transmission */
				conn->unacked_len = temp_unacked_len;

				tcp_ca_fast_retransmit(conn);
				if (tcp_window_full(conn)) {
					(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
				}
			}
		}
#endif
		NET_ASSERT((conn->send_data_total == 0) ||
			   k_work_delayable_is_pending(&conn->send_data_timer),
			   "conn: %p, Missing a subscription "
				"of the send_data queue timer", conn);

		if (th && (net_tcp_seq_cmp(th_ack(th), conn->seq) > 0)) {
			uint32_t len_acked = th_ack(th) - conn->seq;

			NET_DBG("conn: %p len_acked=%u", conn, len_acked);

			if ((conn->send_data_total < len_acked) ||
					(tcp_pkt_pull(conn->send_data,
						      len_acked) < 0)) {
				NET_ERR("conn: %p, Invalid len_acked=%u "
					"(total=%zu)", conn, len_acked,
					conn->send_data_total);
				net_stats_update_tcp_seg_drop(conn->iface);
				tcp_out(conn, RST);
				do_close = true;
				close_status = -ECONNRESET;
				break;
			}

#ifdef CONFIG_NET_TCP_FAST_RETRANSMIT
			/* New segment, reset duplicate ack counter */
			conn->dup_ack_cnt = 0;
#endif
			tcp_ca_pkts_acked(conn, len_acked);

			conn->send_data_total -= len_acked;
			if (conn->unacked_len < len_acked) {
				conn->unacked_len = 0;
			} else {
				conn->unacked_len -= len_acked;
			}

			if (!tcp_window_full(conn)) {
				k_sem_give(&conn->tx_sem);
			}

			conn_seq(conn, + len_acked);
			net_stats_update_tcp_seg_recv(conn->iface);

			/* Receipt of an acknowledgment that covers a sequence number
			 * not previously acknowledged indicates that the connection
			 * makes a "forward progress".
			 */
			tcp_nbr_reachability_hint(conn);

			conn_send_data_dump(conn);

			conn->send_data_retries = 0;
			if (conn->data_mode == TCP_DATA_MODE_RESEND) {
				conn->unacked_len = 0;
				tcp_derive_rto(conn);
			}
			conn->data_mode = TCP_DATA_MODE_SEND;
			if (conn->send_data_total > 0) {
				k_work_reschedule_for_queue(&tcp_work_q, &conn->send_data_timer,
					    K_MSEC(TCP_RTO_MS));
			}

			/* We are closing the connection, send a FIN to peer */
			if (conn->in_close && conn->send_data_total == 0) {
				tcp_send_timer_cancel(conn);
				next = TCP_FIN_WAIT_1;

				k_work_reschedule_for_queue(&tcp_work_q,
							    &conn->fin_timer,
							    FIN_TIMEOUT);

				tcp_out(conn, FIN | ACK);
				conn_seq(conn, + 1);
				verdict = NET_OK;
				keep_alive_timer_stop(conn);
				break;
			}

			ret = tcp_send_queued_data(conn);
			if (ret < 0 && ret != -ENOBUFS) {
				tcp_out(conn, RST);
				do_close = true;
				close_status = ret;
				verdict = NET_OK;
				break;
			}

			if (tcp_window_full(conn)) {
				(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
			}
		}

		if (th) {
			if (th_seq(th) == conn->ack) {
				if (len > 0) {
					verdict = tcp_data_received(conn, pkt, &len);
					if (verdict == NET_OK) {
						/* net_pkt owned by the recv fifo now */
						pkt = NULL;
					}
				} else {
					/* ACK, no data */
					verdict = NET_OK;
				}
			} else if (net_tcp_seq_greater(conn->ack, th_seq(th))) {
				/* This should handle the acknowledgements of keep alive
				 * packets and retransmitted data.
				 * RISK:
				 * There is a tiny risk of creating a ACK loop this way when
				 * both ends of the connection are out of order due to packet
				 * loss is a simulatanious bidirectional data flow.
				 */
				tcp_out(conn, ACK); /* peer has resent */

				net_stats_update_tcp_seg_ackerr(conn->iface);
				verdict = NET_OK;
			} else if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT) {
				tcp_out_of_order_data(conn, pkt, len,
						      th_seq(th));
				/* Send out a duplicated ACK */
				if ((len > 0) || FL(&fl, &, FIN)) {
					tcp_out(conn, ACK);
				}

				verdict = NET_OK;
			}
		}

		/* Check if there is any data left to retransmit possibly*/
		if (conn->send_data_total == 0) {
			conn->send_data_retries = 0;
			k_work_cancel_delayable(&conn->send_data_timer);
		}

		/* A lot could have happened to the transmission window check the situation here */
		if (tcp_window_full(conn)) {
			(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
		} else {
			k_sem_give(&conn->tx_sem);
		}

		break;
	case TCP_CLOSE_WAIT:
		tcp_out(conn, FIN);
		conn_seq(conn, + 1);
		next = TCP_LAST_ACK;
		tcp_setup_last_ack_timer(conn);
		break;
	case TCP_LAST_ACK:
		if (th && FL(&fl, ==, ACK, th_ack(th) == conn->seq)) {
			tcp_send_timer_cancel(conn);
			do_close = true;
			verdict = NET_OK;
			close_status = 0;

			/* Remove the last ack timer if we received it in time */
			tcp_cancel_last_ack_timer(conn);
		}
		break;
	case TCP_CLOSED:
		break;
	case TCP_FIN_WAIT_1:
		/*
		 * FIN1:
		 * Acknowledge path and sequence path are independent, treat them that way
		 * The table of incoming messages and their destination states:
		 * -   & -   -> TCP_FIN_WAIT_1
		 * FIN & -   -> TCP_CLOSING
		 * -   & ACK -> TCP_FIN_WAIT_2
		 * FIN & ACK -> TCP_TIME_WAIT
		 */
		if (th) {
			bool fin_acked = false;

			if (tcp_compute_new_length(conn, th, len, false) > 0) {
				/* We do not implement half closed sockets, therefore
				 * cannot accept new data in after sending our FIN, as
				 * we are in sequence can send a reset now.
				 */
				net_stats_update_tcp_seg_drop(conn->iface);

				next = tcp_enter_time_wait(conn);

				tcp_out(conn, RST);
				break;
			}
			if (FL(&fl, &, ACK, th_ack(th) == conn->seq)) {
				NET_DBG("conn %p: FIN acknowledged, going to FIN_WAIT_2 "
					"state seq %u, ack %u"
					, conn, conn->seq, conn->ack);
				tcp_send_timer_cancel(conn);
				fin_acked = true;
				next = TCP_FIN_WAIT_2;
				verdict = NET_OK;
			}

			/*
			 * There can also be data in the message, so compute with the length
			 * of the packet to check the sequence number of the FIN flag with the ACK
			 */
			if (FL(&fl, &, FIN, net_tcp_seq_cmp(th_seq(th) + len, conn->ack) == 0)) {
				conn_ack(conn, + 1);

				/* State path is dependent on if the acknowledge is in */
				if (fin_acked) {
					/* Already acknowledged, we can go further */
					NET_DBG("conn %p: FIN received, going to TIME WAIT", conn);

					next = tcp_enter_time_wait(conn);

					tcp_out(conn, ACK);
				} else {
					/* Fin not yet acknowledged, waiting for the ack in CLOSING
					 */
					NET_DBG("conn %p: FIN received, going to CLOSING as no "
						"ACK has been received", conn);
					tcp_send_timer_cancel(conn);
					tcp_out_ext(conn, FIN | ACK, NULL, conn->seq - 1);
					next = TCP_CLOSING;
				}
				verdict = NET_OK;
			} else {
				if (len > 0) {
					if (fin_acked) {
						/* Send out a duplicate ACK */
						tcp_send_timer_cancel(conn);
						tcp_out(conn, ACK);
					} else {
						/* In FIN1 state
						 * Send out a duplicate ACK, with the pending FIN
						 * flag
						 */
						tcp_send_timer_cancel(conn);
						tcp_out_ext(conn, FIN | ACK, NULL, conn->seq - 1);
					}
					verdict = NET_OK;
				}
			}
		}
		break;
	case TCP_FIN_WAIT_2:
		/*
		 * FIN2:
		 * Only FIN is relevant in this state, as our FIN was already acknowledged
		 * -   -> TCP_FIN_WAIT_2
		 * FIN -> TCP_TIME_WAIT
		 */
		if (th) {
			/* No tcp_send_timer_cancel call required here, as is has been called
			 * before entering this state, only allowed through the
			 * tcp_enter_time_wait function.
			 */

			/* Compute if there is new data after our close */
			if (tcp_compute_new_length(conn, th, len, false) > 0) {
				/* We do not implement half closed sockets, therefore
				 * cannot accept new data in after sending our FIN, as
				 * we are in sequence can send a reset now.
				 */
				net_stats_update_tcp_seg_drop(conn->iface);

				next = tcp_enter_time_wait(conn);

				tcp_out(conn, RST);
				break;
			}
			/*
			 * There can also be data in the message, so compute with the length
			 * of the packet to check the sequence number of the FIN flag with the ACK
			 */
			if (FL(&fl, &, FIN, net_tcp_seq_cmp(th_seq(th) + len, conn->ack) == 0)) {
				conn_ack(conn, + 1);
				NET_DBG("conn %p: FIN received, going to TIME WAIT", conn);

				next = tcp_enter_time_wait(conn);

				verdict = NET_OK;
				tcp_out(conn, ACK);
			} else {
				if (len > 0) {
					/* Send out a duplicate ACK */
					tcp_out(conn, ACK);
					verdict = NET_OK;
				}
			}
		}
		break;
	case TCP_CLOSING:
		if (th) {
			bool fin_acked = false;

			/*
			 * Closing:
			 * Our FIN has to be acknowledged
			 * -   -> TCP_CLOSING
			 * ACK -> TCP_TIME_WAIT
			 */
			int32_t new_len = tcp_compute_new_length(conn, th, len, true);

			if (new_len > 0) {
				/* This should not happen here, as no data can be send after
				 * the FIN flag has been send.
				 */
				NET_ERR("conn: %p, new bytes %u during CLOSING state "
					"sending reset", conn, new_len);
				net_stats_update_tcp_seg_drop(conn->iface);

				next = tcp_enter_time_wait(conn);

				tcp_out(conn, RST);
				break;
			}

			if (FL(&fl, &, ACK, th_ack(th) == conn->seq)) {
				NET_DBG("conn %p: FIN acknowledged, going to TIME WAIT "
					"state seq %u, ack %u"
					, conn, conn->seq, conn->ack);

				next = tcp_enter_time_wait(conn);
				fin_acked = true;

				verdict = NET_OK;
			}

			/*
			 * There can also be data in the message, so compute with the length
			 * of the packet to check with the ack
			 * Since the conn->ack was already incremented in TCP_FIN_WAIT_1
			 * add 1 in the comparison sequence
			 */
			if ((FL(&fl, &, FIN,
				net_tcp_seq_cmp(th_seq(th) + len + 1, conn->ack) == 0)) ||
			    (len > 0)) {
				tcp_send_timer_cancel(conn);
				if (fin_acked) {
					/* Send out a duplicate ACK */
					tcp_out(conn, ACK);
				} else {
					/* Send out a duplicate ACK, with the pending FIN
					 * flag
					 */
					tcp_out_ext(conn, FIN | ACK, NULL, conn->seq - 1);
				}
				verdict = NET_OK;
			}
		}
		break;
	case TCP_TIME_WAIT:
		if (th) {
			int32_t new_len = tcp_compute_new_length(conn, th, len, true);

			/* No tcp_send_timer_cancel call required here, as is has been called
			 * before entering this state, only allowed through the
			 * tcp_enter_time_wait function.
			 */

			if (new_len > 0) {
				/* This should not happen here, as no data can be send after
				 * the FIN flag has been send.
				 */
				NET_ERR("conn: %p, new bytes %u during TIME-WAIT state "
					"sending reset", conn, new_len);
				net_stats_update_tcp_seg_drop(conn->iface);

				tcp_out(conn, RST);
			} else {
				/* Acknowledge any FIN attempts, in case retransmission took
				 * place.
				 */
				if ((FL(&fl, &, FIN,
					net_tcp_seq_cmp(th_seq(th) + 1, conn->ack) == 0)) ||
				    (len > 0)) {
					tcp_out(conn, ACK);
					verdict = NET_OK;
				}
			}
		}
		break;
	default:
		NET_ASSERT(false, "%s is unimplemented",
			   tcp_state_to_str(conn->state, true));
	}

out:
	if (pkt) {
		if (verdict == NET_OK) {
			net_pkt_unref(pkt);
		}

		pkt = NULL;
	}

	if (next) {
		th = NULL;
		conn_state(conn, next);
		next = 0;

		if (connection_ok) {
			conn->in_connect = false;
			if (conn->connect_cb) {
				conn->connect_cb(conn->context, 0, conn->context->user_data);

				/* Make sure the connect_cb is only called once. */
				conn->connect_cb = NULL;
			}

			k_sem_give(&conn->connect_sem);
		}

		goto next_state;
	}

	if (conn->context) {
		/* If the conn->context is not set, then the connection was
		 * already closed.
		 */
		conn_handler = (struct net_conn *)conn->context->conn_handler;
	}

	recv_user_data = conn->recv_user_data;
	recv_data_fifo = &conn->recv_data;

	k_mutex_unlock(&conn->lock);

	/* Pass all the received data stored in recv fifo to the application.
	 * This is done like this so that we do not have any connection lock
	 * held.
	 */
	while (conn_handler && atomic_get(&conn->ref_count) > 0 &&
	       (recv_pkt = k_fifo_get(recv_data_fifo, K_NO_WAIT)) != NULL) {
		if (net_context_packet_received(conn_handler, recv_pkt, NULL,
						NULL, recv_user_data) ==
		    NET_DROP) {
			/* Application is no longer there, unref the pkt */
			tcp_pkt_unref(recv_pkt);
		}
	}

	/* Make sure we close the connection only once by checking connection
	 * state.
	 */
	if (do_close && conn->state != TCP_UNUSED && conn->state != TCP_CLOSED) {
		tcp_conn_close(conn, close_status);
	}

	return verdict;
}

/* Active connection close: send FIN and go to FIN_WAIT_1 state */
int net_tcp_put(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	if (!conn) {
		return -ENOENT;
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("%s", conn ? tcp_conn_state(conn, NULL) : "");
	NET_DBG("context %p %s", context,
		({ const char *state = net_context_state(context);
					state ? state : "<unknown>"; }));

	if (conn && (conn->state == TCP_ESTABLISHED ||
		     conn->state == TCP_SYN_RECEIVED)) {
		/* Send all remaining data if possible. */
		if (conn->send_data_total > 0) {
			NET_DBG("conn %p pending %zu bytes", conn,
				conn->send_data_total);
			conn->in_close = true;

			/* How long to wait until all the data has been sent?
			 */
			k_work_reschedule_for_queue(&tcp_work_q,
						    &conn->send_data_timer,
						    K_MSEC(TCP_RTO_MS));
		} else {
			int ret;

			NET_DBG("TCP connection in %s close, "
				"not disposing yet (waiting %dms)",
				"active", tcp_max_timeout_ms);
			k_work_reschedule_for_queue(&tcp_work_q,
						    &conn->fin_timer,
						    FIN_TIMEOUT);

			ret = tcp_out_ext(conn, FIN | ACK, NULL,
					  conn->seq + conn->unacked_len);
			if (ret == 0) {
				conn_seq(conn, + 1);
			}

			conn_state(conn, TCP_FIN_WAIT_1);

			keep_alive_timer_stop(conn);
		}
	} else if (conn && conn->in_connect) {
		conn->in_connect = false;
	}

	k_mutex_unlock(&conn->lock);

	tcp_conn_unref(conn);

	return 0;
}

int net_tcp_listen(struct net_context *context)
{
	/* when created, tcp connections are in state TCP_LISTEN */
	net_context_set_state(context, NET_CONTEXT_LISTENING);

	return 0;
}

int net_tcp_update_recv_wnd(struct net_context *context, int32_t delta)
{
	struct tcp *conn = context->tcp;
	int ret;

	if (!conn) {
		NET_ERR("context->tcp == NULL");
		return -EPROTOTYPE;
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	ret = tcp_update_recv_wnd((struct tcp *)context->tcp, delta);

	k_mutex_unlock(&conn->lock);

	return ret;
}

int net_tcp_queue(struct net_context *context, const void *data, size_t len,
		  const struct msghdr *msg)
{
	struct tcp *conn = context->tcp;
	size_t queued_len = 0;
	int ret = 0;

	if (!conn || conn->state != TCP_ESTABLISHED) {
		return -ENOTCONN;
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	/* If there is no space to transmit, try at a later time.
	 * The ZWP will make sure the window becomes available at
	 * some point in time.
	 */
	if (tcp_window_full(conn)) {
		ret = -EAGAIN;
		goto out;
	}

	if (msg) {
		len = 0;

		for (int i = 0; i < msg->msg_iovlen; i++) {
			len += msg->msg_iov[i].iov_len;
		}
	}

	/* Queue no more than TX window permits. It's guaranteed at this point
	 * that conn->send_data_total is less than conn->send_win, as it was
	 * verified in tcp_window_full() check above. As the connection mutex
	 * is held, their values shall not change since.
	 */
	len = MIN(conn->send_win - conn->send_data_total, len);

	if (msg) {
		for (int i = 0; i < msg->msg_iovlen; i++) {
			int iovlen = MIN(msg->msg_iov[i].iov_len, len);

			ret = tcp_pkt_append(conn->send_data,
					     msg->msg_iov[i].iov_base,
					     iovlen);
			if (ret < 0) {
				if (queued_len == 0) {
					goto out;
				} else {
					break;
				}
			}

			queued_len += iovlen;
			len -= iovlen;

			if (len == 0) {
				break;
			}
		}
	} else {
		ret = tcp_pkt_append(conn->send_data, data, len);
		if (ret < 0) {
			goto out;
		}

		queued_len = len;
	}

	conn->send_data_total += queued_len;

	/* Successfully queued data for transmission. Even if there's a transmit
	 * failure now (out-of-buf case), it can be ignored for now, retransmit
	 * timer will take care of queued data retransmission.
	 */
	ret = tcp_send_queued_data(conn);
	if (ret < 0 && ret != -ENOBUFS) {
		tcp_conn_close(conn, ret);
		goto out;
	}

	if (tcp_window_full(conn)) {
		(void)k_sem_take(&conn->tx_sem, K_NO_WAIT);
	}

	ret = queued_len;
out:
	k_mutex_unlock(&conn->lock);

	return ret;
}

/* net context is about to send out queued data - inform caller only */
int net_tcp_send_data(struct net_context *context, net_context_send_cb_t cb,
		      void *user_data)
{
	if (cb) {
		cb(context, 0, user_data);
	}

	return 0;
}

/* When connect() is called on a TCP socket, register the socket for incoming
 * traffic with net context and give the TCP packet receiving function, which
 * in turn will call tcp_in() to deliver the TCP packet to the stack
 */
int net_tcp_connect(struct net_context *context,
		    const struct sockaddr *remote_addr,
		    struct sockaddr *local_addr,
		    uint16_t remote_port, uint16_t local_port,
		    k_timeout_t timeout, net_context_connect_cb_t cb,
		    void *user_data)
{
	struct tcp *conn;
	int ret = 0;

	NET_DBG("context: %p, local: %s, remote: %s", context,
		net_sprint_addr(local_addr->sa_family,
				(const void *)&net_sin(local_addr)->sin_addr),
		net_sprint_addr(remote_addr->sa_family,
				(const void *)&net_sin(remote_addr)->sin_addr));

	conn = context->tcp;
	conn->iface = net_context_get_iface(context);
	tcp_derive_rto(conn);

	switch (net_context_get_family(context)) {
		const struct in_addr *ip4;
		const struct in6_addr *ip6;

	case AF_INET:
		if (!IS_ENABLED(CONFIG_NET_IPV4)) {
			ret = -EINVAL;
			goto out;
		}

		memset(&conn->src, 0, sizeof(struct sockaddr_in));
		memset(&conn->dst, 0, sizeof(struct sockaddr_in));

		conn->src.sa.sa_family = AF_INET;
		conn->dst.sa.sa_family = AF_INET;

		conn->dst.sin.sin_port = remote_port;
		conn->src.sin.sin_port = local_port;

		/* we have to select the source address here as
		 * net_context_create_ipv4_new() is not called in the packet
		 * output chain
		 */
		if (net_ipv4_is_addr_unspecified(
			&net_sin(local_addr)->sin_addr)) {
			ip4 = net_if_ipv4_select_src_addr(
				net_context_get_iface(context),
				&net_sin(remote_addr)->sin_addr);
			net_ipaddr_copy(&conn->src.sin.sin_addr, ip4);
		} else {
			net_ipaddr_copy(&conn->src.sin.sin_addr,
					&net_sin(local_addr)->sin_addr);
		}
		net_ipaddr_copy(&conn->dst.sin.sin_addr,
				&net_sin(remote_addr)->sin_addr);
		break;

	case AF_INET6:
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			ret = -EINVAL;
			goto out;
		}

		memset(&conn->src, 0, sizeof(struct sockaddr_in6));
		memset(&conn->dst, 0, sizeof(struct sockaddr_in6));

		conn->src.sin6.sin6_family = AF_INET6;
		conn->dst.sin6.sin6_family = AF_INET6;

		conn->dst.sin6.sin6_port = remote_port;
		conn->src.sin6.sin6_port = local_port;

		if (net_ipv6_is_addr_unspecified(
			&net_sin6(local_addr)->sin6_addr)) {
			ip6 = net_if_ipv6_select_src_addr(
				net_context_get_iface(context),
				&net_sin6(remote_addr)->sin6_addr);
			net_ipaddr_copy(&conn->src.sin6.sin6_addr, ip6);
		} else {
			net_ipaddr_copy(&conn->src.sin6.sin6_addr,
					&net_sin6(local_addr)->sin6_addr);
		}
		net_ipaddr_copy(&conn->dst.sin6.sin6_addr,
				&net_sin6(remote_addr)->sin6_addr);
		break;

	default:
		ret = -EPROTONOSUPPORT;
	}

	if (!(IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) ||
	      IS_ENABLED(CONFIG_NET_TEST))) {
		conn->seq = tcp_init_isn(&conn->src.sa, &conn->dst.sa);
	}

	NET_DBG("conn: %p src: %s, dst: %s", conn,
		net_sprint_addr(conn->src.sa.sa_family,
				(const void *)&conn->src.sin.sin_addr),
		net_sprint_addr(conn->dst.sa.sa_family,
				(const void *)&conn->dst.sin.sin_addr));

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	ret = net_conn_register(net_context_get_proto(context),
				net_context_get_family(context),
				remote_addr, local_addr,
				ntohs(remote_port), ntohs(local_port),
				context, tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		goto out;
	}

	net_if_addr_ref(conn->iface, conn->src.sa.sa_family,
			conn->src.sa.sa_family == AF_INET ?
			(const void *)&conn->src.sin.sin_addr :
			(const void *)&conn->src.sin6.sin6_addr);
	conn->addr_ref_done = true;

	conn->connect_cb = cb;
	context->user_data = user_data;

	/* Input of a (nonexistent) packet with no flags set will cause
	 * a TCP connection to be established
	 */
	conn->in_connect = !IS_ENABLED(CONFIG_NET_TEST_PROTOCOL);
	(void)tcp_in(conn, NULL);

	if (!IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)) {
		if ((K_TIMEOUT_EQ(timeout, K_NO_WAIT)) &&
		    conn->state != TCP_ESTABLISHED) {
			ret = -EINPROGRESS;
			goto out;
		} else if (k_sem_take(&conn->connect_sem, timeout) != 0 &&
			   conn->state != TCP_ESTABLISHED) {
			if (conn->in_connect) {
				conn->in_connect = false;
				tcp_conn_close(conn, -ETIMEDOUT);
			}

			ret = -ETIMEDOUT;
			goto out;
		}
		conn->in_connect = false;
	}
 out:
	NET_DBG("conn: %p, ret=%d", conn, ret);

	return ret;
}

int net_tcp_accept(struct net_context *context, net_tcp_accept_cb_t cb,
		   void *user_data)
{
	struct tcp *conn = context->tcp;
	struct sockaddr local_addr = { };
	uint16_t local_port, remote_port;

	if (!conn) {
		return -EINVAL;
	}

	NET_DBG("context: %p, tcp: %p, cb: %p", context, conn, cb);

	if (conn->state != TCP_LISTEN) {
		return -EINVAL;
	}

	conn->accept_cb = cb;
	local_addr.sa_family = net_context_get_family(context);

	switch (local_addr.sa_family) {
		struct sockaddr_in *in;
		struct sockaddr_in6 *in6;

	case AF_INET:
		if (!IS_ENABLED(CONFIG_NET_IPV4)) {
			return -EINVAL;
		}

		in = (struct sockaddr_in *)&local_addr;

		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&in->sin_addr,
					net_sin_ptr(&context->local)->sin_addr);
		}

		in->sin_port =
			net_sin((struct sockaddr *)&context->local)->sin_port;
		local_port = ntohs(in->sin_port);
		remote_port = ntohs(net_sin(&context->remote)->sin_port);

		break;

	case AF_INET6:
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			return -EINVAL;
		}

		in6 = (struct sockaddr_in6 *)&local_addr;

		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&in6->sin6_addr,
				net_sin6_ptr(&context->local)->sin6_addr);
		}

		in6->sin6_port =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;
		local_port = ntohs(in6->sin6_port);
		remote_port = ntohs(net_sin6(&context->remote)->sin6_port);

		break;

	default:
		return -EINVAL;
	}

	context->user_data = user_data;

	/* Remove the temporary connection handler and register
	 * a proper now as we have an established connection.
	 */
	net_conn_unregister(context->conn_handler);

	return net_conn_register(net_context_get_proto(context),
				 local_addr.sa_family,
				 context->flags & NET_CONTEXT_REMOTE_ADDR_SET ?
				 &context->remote : NULL,
				 &local_addr,
				 remote_port, local_port,
				 context, tcp_recv, context,
				 &context->conn_handler);
}

int net_tcp_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data)
{
	struct tcp *conn = context->tcp;

	NET_DBG("context: %p, cb: %p, user_data: %p", context, cb, user_data);

	context->recv_cb = cb;

	if (conn) {
		conn->recv_user_data = user_data;
	}

	return 0;
}

int net_tcp_finalize(struct net_pkt *pkt, bool force_chksum)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_tcp_hdr *tcp_hdr;

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!tcp_hdr) {
		return -ENOBUFS;
	}

	tcp_hdr->chksum = 0U;

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt)) || force_chksum) {
		tcp_hdr->chksum = net_calc_chksum_tcp(pkt);
		net_pkt_set_chksum_done(pkt, true);
	}

	return net_pkt_set_data(pkt, &tcp_access);
}

struct net_tcp_hdr *net_tcp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *tcp_access)
{
	struct net_tcp_hdr *tcp_hdr;

	if (IS_ENABLED(CONFIG_NET_TCP_CHECKSUM) &&
	    (net_if_need_calc_rx_checksum(net_pkt_iface(pkt)) ||
	     net_pkt_is_ip_reassembled(pkt)) &&
	    net_calc_chksum_tcp(pkt) != 0U) {
		NET_DBG("DROP: checksum mismatch");
		goto drop;
	}

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, tcp_access);
	if (tcp_hdr && !net_pkt_set_data(pkt, tcp_access)) {
		return tcp_hdr;
	}

drop:
	net_stats_update_tcp_seg_chkerr(net_pkt_iface(pkt));
	return NULL;
}

#if defined(CONFIG_NET_TEST_PROTOCOL)
static enum net_verdict tcp_input(struct net_conn *net_conn,
				  struct net_pkt *pkt,
				  union net_ip_header *ip,
				  union net_proto_header *proto,
				  void *user_data)
{
	struct tcphdr *th = th_get(pkt);
	enum net_verdict verdict = NET_DROP;

	if (th) {
		struct tcp *conn = tcp_conn_search(pkt);

		if (conn == NULL && SYN == th_flags(th)) {
			struct net_context *context =
				tcp_calloc(1, sizeof(struct net_context));
			net_tcp_get(context);
			net_context_set_family(context, net_pkt_family(pkt));
			conn = context->tcp;
			tcp_endpoint_set(&conn->dst, pkt, TCP_EP_SRC);
			tcp_endpoint_set(&conn->src, pkt, TCP_EP_DST);
			/* Make an extra reference, the sanity check suite
			 * will delete the connection explicitly
			 */
			tcp_conn_ref(conn);
		}

		if (conn) {
			conn->iface = pkt->iface;
			verdict = tcp_in(conn, pkt);
		}
	}

	return verdict;
}

static size_t tp_tcp_recv_cb(struct tcp *conn, struct net_pkt *pkt)
{
	ssize_t len = tcp_data_len(pkt);
	struct net_pkt *up = tcp_pkt_clone(pkt);

	NET_DBG("pkt: %p, len: %zu", pkt, net_pkt_get_len(pkt));

	net_pkt_cursor_init(up);
	net_pkt_set_overwrite(up, true);

	net_pkt_pull(up, net_pkt_get_len(up) - len);

	for (struct net_buf *buf = pkt->buffer; buf != NULL; buf = buf->frags) {
		net_tcp_queue(conn->context, buf->data, buf->len);
	}

	return len;
}

static ssize_t tp_tcp_recv(int fd, void *buf, size_t len, int flags)
{
	return 0;
}

static void tp_init(struct tcp *conn, struct tp *tp)
{
	struct tp out = {
		.msg = "",
		.status = "",
		.state = tcp_state_to_str(conn->state, true),
		.seq = conn->seq,
		.ack = conn->ack,
		.rcv = "",
		.data = "",
		.op = "",
	};

	*tp = out;
}

static void tcp_to_json(struct tcp *conn, void *data, size_t *data_len)
{
	struct tp tp;

	tp_init(conn, &tp);

	tp_encode(&tp, data, data_len);
}

enum net_verdict tp_input(struct net_conn *net_conn,
			  struct net_pkt *pkt,
			  union net_ip_header *ip_hdr,
			  union net_proto_header *proto,
			  void *user_data)
{
	struct net_udp_hdr *uh = net_udp_get_hdr(pkt, NULL);
	size_t data_len = ntohs(uh->len) - sizeof(*uh);
	struct tcp *conn = tcp_conn_search(pkt);
	size_t json_len = 0;
	struct tp *tp;
	struct tp_new *tp_new;
	enum tp_type type;
	bool responded = false;
	static char buf[512];
	enum net_verdict verdict = NET_DROP;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ip_opts_len(pkt) + sizeof(*uh));
	net_pkt_read(pkt, buf, data_len);
	buf[data_len] = '\0';
	data_len += 1;

	type = json_decode_msg(buf, data_len);

	data_len = ntohs(uh->len) - sizeof(*uh);

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ip_opts_len(pkt) + sizeof(*uh));
	net_pkt_read(pkt, buf, data_len);
	buf[data_len] = '\0';
	data_len += 1;

	switch (type) {
	case TP_CONFIG_REQUEST:
		tp_new = json_to_tp_new(buf, data_len);
		break;
	default:
		tp = json_to_tp(buf, data_len);
		break;
	}

	switch (type) {
	case TP_COMMAND:
		if (is("CONNECT", tp->op)) {
			tp_output(pkt->family, pkt->iface, buf, 1);
			responded = true;
			{
				struct net_context *context = tcp_calloc(1,
						sizeof(struct net_context));
				net_tcp_get(context);
				net_context_set_family(context,
						       net_pkt_family(pkt));
				conn = context->tcp;
				tcp_endpoint_set(&conn->dst, pkt, TCP_EP_SRC);
				tcp_endpoint_set(&conn->src, pkt, TCP_EP_DST);
				conn->iface = pkt->iface;
				tcp_conn_ref(conn);
			}
			conn->seq = tp->seq;
			verdict = tcp_in(conn, NULL);
		}
		if (is("CLOSE", tp->op)) {
			tp_trace = false;
			{
				struct net_context *context;

				conn = (void *)sys_slist_peek_head(&tcp_conns);
				context = conn->context;
				while (tcp_conn_close(conn, 0))
					;
				tcp_free(context);
			}
			tp_mem_stat();
			tp_nbuf_stat();
			tp_pkt_stat();
			tp_seq_stat();
		}
		if (is("CLOSE2", tp->op)) {
			struct tcp *conn =
				(void *)sys_slist_peek_head(&tcp_conns);
			net_tcp_put(conn->context);
		}
		if (is("RECV", tp->op)) {
#define HEXSTR_SIZE 64
			char hexstr[HEXSTR_SIZE];
			ssize_t len = tp_tcp_recv(0, buf, sizeof(buf), 0);

			tp_init(conn, tp);
			bin2hex(buf, len, hexstr, HEXSTR_SIZE);
			tp->data = hexstr;
			NET_DBG("%zd = tcp_recv(\"%s\")", len, tp->data);
			json_len = sizeof(buf);
			tp_encode(tp, buf, &json_len);
		}
		if (is("SEND", tp->op)) {
			ssize_t len = tp_str_to_hex(buf, sizeof(buf), tp->data);
			struct tcp *conn =
				(void *)sys_slist_peek_head(&tcp_conns);

			tp_output(pkt->family, pkt->iface, buf, 1);
			responded = true;
			NET_DBG("tcp_send(\"%s\")", tp->data);
			{
				net_tcp_queue(conn->context, buf, len);
			}
		}
		break;
	case TP_CONFIG_REQUEST:
		tp_new_find_and_apply(tp_new, "tcp_rto", &tcp_rto, TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_retries", &tcp_retries,
					TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_window", &tcp_rx_window,
					TP_INT);
		tp_new_find_and_apply(tp_new, "tp_trace", &tp_trace, TP_BOOL);
		break;
	case TP_INTROSPECT_REQUEST:
		json_len = sizeof(buf);
		conn = (void *)sys_slist_peek_head(&tcp_conns);
		tcp_to_json(conn, buf, &json_len);
		break;
	case TP_DEBUG_STOP: case TP_DEBUG_CONTINUE:
		tp_state = tp->type;
		break;
	default:
		NET_ASSERT(false, "Unimplemented tp command: %s", tp->msg);
	}

	if (json_len) {
		tp_output(pkt->family, pkt->iface, buf, json_len);
	} else if ((TP_CONFIG_REQUEST == type || TP_COMMAND == type)
			&& responded == false) {
		tp_output(pkt->family, pkt->iface, buf, 1);
	}

	return verdict;
}

static void test_cb_register(sa_family_t family, uint8_t proto, uint16_t remote_port,
			     uint16_t local_port, net_conn_cb_t cb)
{
	struct net_conn_handle *conn_handle = NULL;
	const struct sockaddr addr = { .sa_family = family, };

	int ret = net_conn_register(proto,
				    family,
				    &addr,	/* remote address */
				    &addr,	/* local address */
				    local_port,
				    remote_port,
				    NULL,
				    cb,
				    NULL,	/* user_data */
				    &conn_handle);
	if (ret < 0) {
		NET_ERR("net_conn_register(): %d", ret);
	}
}
#endif /* CONFIG_NET_TEST_PROTOCOL */

void net_tcp_foreach(net_tcp_cb_t cb, void *user_data)
{
	struct tcp *conn;
	struct tcp *tmp;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp_conns, conn, tmp, next) {
		if (atomic_get(&conn->ref_count) > 0) {
			k_mutex_unlock(&tcp_lock);
			cb(conn, user_data);
			k_mutex_lock(&tcp_lock, K_FOREVER);
		}
	}

	k_mutex_unlock(&tcp_lock);
}

uint16_t net_tcp_get_supported_mss(const struct tcp *conn)
{
	sa_family_t family = net_context_get_family(conn->context);

	if (family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_context_get_iface(conn->context);
		int mss = 0;

		if (iface && net_if_get_mtu(iface) >= NET_IPV4TCPH_LEN) {
			/* Detect MSS based on interface MTU minus "TCP,IP
			 * header size"
			 */
			mss = net_if_get_mtu(iface) - NET_IPV4TCPH_LEN;
		}

		if (mss == 0) {
			mss = NET_IPV4_MTU - NET_IPV4TCPH_LEN;
		}

		return mss;
#else
		return 0;
#endif /* CONFIG_NET_IPV4 */
	}
#if defined(CONFIG_NET_IPV6)
	else if (family == AF_INET6) {
		struct net_if *iface = net_context_get_iface(conn->context);
		int mss = 0;

		if (iface && net_if_get_mtu(iface) >= NET_IPV6TCPH_LEN) {
			/* Detect MSS based on interface MTU minus "TCP,IP
			 * header size"
			 */
			mss = net_if_get_mtu(iface) - NET_IPV6TCPH_LEN;
		}

		if (mss == 0) {
			mss = NET_IPV6_MTU - NET_IPV6TCPH_LEN;
		}

		return mss;
	}
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

int net_tcp_set_option(struct net_context *context,
		       enum tcp_conn_option option,
		       const void *value, size_t len)
{
	int ret = 0;

	NET_ASSERT(context);

	struct tcp *conn = context->tcp;

	NET_ASSERT(conn);

	k_mutex_lock(&conn->lock, K_FOREVER);

	switch (option) {
	case TCP_OPT_NODELAY:
		ret = set_tcp_nodelay(conn, value, len);
		break;
	case TCP_OPT_KEEPALIVE:
		ret = set_tcp_keep_alive(conn, value, len);
		break;
	case TCP_OPT_KEEPIDLE:
		ret = set_tcp_keep_idle(conn, value, len);
		break;
	case TCP_OPT_KEEPINTVL:
		ret = set_tcp_keep_intvl(conn, value, len);
		break;
	case TCP_OPT_KEEPCNT:
		ret = set_tcp_keep_cnt(conn, value, len);
		break;
	}

	k_mutex_unlock(&conn->lock);

	return ret;
}

int net_tcp_get_option(struct net_context *context,
		       enum tcp_conn_option option,
		       void *value, size_t *len)
{
	int ret = 0;

	NET_ASSERT(context);

	struct tcp *conn = context->tcp;

	NET_ASSERT(conn);

	k_mutex_lock(&conn->lock, K_FOREVER);

	switch (option) {
	case TCP_OPT_NODELAY:
		ret = get_tcp_nodelay(conn, value, len);
		break;
	case TCP_OPT_KEEPALIVE:
		ret = get_tcp_keep_alive(conn, value, len);
		break;
	case TCP_OPT_KEEPIDLE:
		ret = get_tcp_keep_idle(conn, value, len);
		break;
	case TCP_OPT_KEEPINTVL:
		ret = get_tcp_keep_intvl(conn, value, len);
		break;
	case TCP_OPT_KEEPCNT:
		ret = get_tcp_keep_cnt(conn, value, len);
		break;
	}

	k_mutex_unlock(&conn->lock);

	return ret;
}

const char *net_tcp_state_str(enum tcp_state state)
{
	return tcp_state_to_str(state, false);
}

struct k_sem *net_tcp_tx_sem_get(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	return &conn->tx_sem;
}

struct k_sem *net_tcp_conn_sem_get(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	return &conn->connect_sem;
}

void net_tcp_init(void)
{
	int i;
	int rto;
#if defined(CONFIG_NET_TEST_PROTOCOL)
	/* Register inputs for TTCN-3 based TCP sanity check */
	test_cb_register(AF_INET,  IPPROTO_TCP, 4242, 4242, tcp_input);
	test_cb_register(AF_INET6, IPPROTO_TCP, 4242, 4242, tcp_input);
	test_cb_register(AF_INET,  IPPROTO_UDP, 4242, 4242, tp_input);
	test_cb_register(AF_INET6, IPPROTO_UDP, 4242, 4242, tp_input);

	tcp_recv_cb = tp_tcp_recv_cb;
#endif

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NET_TCP_WORKER_PRIO)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NET_TCP_WORKER_PRIO)
#endif

	/* Use private workqueue in order not to block the system work queue.
	 */
	k_work_queue_start(&tcp_work_q, work_q_stack,
			   K_KERNEL_STACK_SIZEOF(work_q_stack), THREAD_PRIORITY,
			   NULL);

	/* Compute the largest possible retransmission timeout */
	tcp_max_timeout_ms = 0;
	rto = tcp_rto;
	for (i = 0; i < tcp_retries; i++) {
		tcp_max_timeout_ms += rto;
		rto += rto >> 1;
	}
	/* At the last timeout cicle */
	tcp_max_timeout_ms += tcp_rto;

	/* When CONFIG_NET_TCP_RANDOMIZED_RTO is active in can be worse case 1.5 times larger */
	if (IS_ENABLED(CONFIG_NET_TCP_RANDOMIZED_RTO)) {
		tcp_max_timeout_ms += tcp_max_timeout_ms >> 1;
	}

	k_thread_name_set(&tcp_work_q.thread, "tcp_work");
	NET_DBG("Workq started. Thread ID: %p", &tcp_work_q.thread);
}
