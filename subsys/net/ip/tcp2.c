/*
 * Copyright (c) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_tcp, CONFIG_NET_TCP_LOG_LEVEL);

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <random/rand32.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/udp.h>
#include "ipv4.h"
#include "ipv6.h"
#include "connection.h"
#include "net_stats.h"
#include "net_private.h"
#include "tcp2_priv.h"

#define ACK_TIMEOUT_MS CONFIG_NET_TCP_ACK_TIMEOUT
#define ACK_TIMEOUT K_MSEC(ACK_TIMEOUT_MS)
#define FIN_TIMEOUT_MS MSEC_PER_SEC
#define FIN_TIMEOUT K_MSEC(FIN_TIMEOUT_MS)

static int tcp_rto = CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT;
static int tcp_retries = CONFIG_NET_TCP_RETRY_COUNT;
static int tcp_window = NET_IPV6_MTU;

static sys_slist_t tcp_conns = SYS_SLIST_STATIC_INIT(&tcp_conns);

static K_MUTEX_DEFINE(tcp_lock);

static K_MEM_SLAB_DEFINE(tcp_conns_slab, sizeof(struct tcp),
				CONFIG_NET_MAX_CONTEXTS, 4);

static struct k_work_q tcp_work_q;
static K_KERNEL_STACK_DEFINE(work_q_stack, CONFIG_NET_TCP_WORKQ_STACK_SIZE);

static void tcp_in(struct tcp *conn, struct net_pkt *pkt);

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

	buf = net_pkt_get_frag(pkt, TCP_PKT_ALLOC_TIMEOUT);

	if (!buf || buf->size < len) {
		if (buf) {
			net_buf_unref(buf);
		}
		ret = -ENOBUFS;
		goto out;
	}

	net_buf_linearize(buf->data, buf->size, pkt->frags, pos, len);
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
			net_ipaddr_copy(&ep->sin.sin_addr,
					src == TCP_EP_SRC ?
							&ip->src : &ip->dst);
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
			net_ipaddr_copy(&ep->sin6.sin6_addr,
					src == TCP_EP_SRC ?
							&ip->src : &ip->dst);
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

		buf[len - 1] = '\0'; /* delete the last comma */
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
	 ((IS_ENABLED(CONFIG_NET_L2_BT) &&				\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_BLUETOOTH) ||	\
	  (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&			\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_IEEE802154) ||	\
	  (IS_ENABLED(CONFIG_NET_L2_CANBUS) &&				\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_CANBUS)))

static void tcp_send(struct net_pkt *pkt)
{
	NET_DBG("%s", log_strdup(tcp_th(pkt)));

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

static void tcp_send_queue_flush(struct tcp *conn)
{
	struct net_pkt *pkt;

	k_delayed_work_cancel(&conn->send_timer);

	while ((pkt = tcp_slist(conn, &conn->send_queue, get,
				struct net_pkt, next))) {
		tcp_pkt_unref(pkt);
	}
}

static int tcp_conn_unref(struct tcp *conn)
{
	int ref_count = atomic_get(&conn->ref_count);
	struct net_pkt *pkt;

	NET_DBG("conn: %p, ref_count=%d", conn, ref_count);

#if !defined(CONFIG_NET_TEST_PROTOCOL)
	if (conn->in_connect) {
		NET_DBG("conn: %p is waiting on connect semaphore", conn);
		tcp_send_queue_flush(conn);
		goto out;
	}
#endif /* CONFIG_NET_TEST_PROTOCOL */

	k_mutex_lock(&tcp_lock, K_FOREVER);

	ref_count = atomic_dec(&conn->ref_count) - 1;
	if (ref_count) {
		tp_out(net_context_get_family(conn->context), conn->iface,
		       "TP_TRACE", "event", "CONN_DELETE");
		goto unlock;
	}

	/* If there is any pending data, pass that to application */
	while ((pkt = k_fifo_get(&conn->recv_data, K_NO_WAIT)) != NULL) {
		if (net_context_packet_received(
			    (struct net_conn *)conn->context->conn_handler,
			    pkt, NULL, NULL, conn->recv_user_data) ==
		    NET_DROP) {
			/* Application is no longer there, unref the pkt */
			tcp_pkt_unref(pkt);
		}
	}

	if (conn->context->conn_handler) {
		net_conn_unregister(conn->context->conn_handler);
		conn->context->conn_handler = NULL;
	}

	if (conn->context->recv_cb) {
		conn->context->recv_cb(conn->context, NULL, NULL, NULL,
					-ECONNRESET, conn->recv_user_data);
	}

	conn->context->tcp = NULL;

	net_context_unref(conn->context);

	tcp_send_queue_flush(conn);

	k_delayed_work_cancel(&conn->send_data_timer);
	tcp_pkt_unref(conn->send_data);

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT) {
		tcp_pkt_unref(conn->queue_recv_data);
	}

	k_delayed_work_cancel(&conn->timewait_timer);
	k_delayed_work_cancel(&conn->fin_timer);

	sys_slist_find_and_remove(&tcp_conns, &conn->next);

	memset(conn, 0, sizeof(*conn));

	k_mem_slab_free(&tcp_conns_slab, (void **)&conn);

unlock:
	k_mutex_unlock(&tcp_lock);
out:
	return ref_count;
}

int net_tcp_unref(struct net_context *context)
{
	int ref_count = 0;

	NET_DBG("context: %p, conn: %p", context, context->tcp);

	if (context->tcp) {
		ref_count = tcp_conn_unref(context->tcp);
	}

	return ref_count;
}

static bool tcp_send_process_no_lock(struct tcp *conn)
{
	bool unref = false;
	struct net_pkt *pkt;

	pkt = tcp_slist(conn, &conn->send_queue, peek_head,
			struct net_pkt, next);
	if (!pkt) {
		goto out;
	}

	NET_DBG("%s %s", log_strdup(tcp_th(pkt)), conn->in_retransmission ?
		"in_retransmission" : "");

	if (conn->in_retransmission) {
		if (conn->send_retries > 0) {
			struct net_pkt *clone = tcp_pkt_clone(pkt);

			if (clone) {
				tcp_send(clone);
				conn->send_retries--;
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
			NET_ERR("net_pkt alloc failure");
			goto out;
		}

		tcp_send(pkt);

		if (forget == false && !k_delayed_work_remaining_get(
				&conn->send_timer)) {
			conn->send_retries = tcp_retries;
			conn->in_retransmission = true;
		}
	}

	if (conn->in_retransmission) {
		k_delayed_work_submit_to_queue(&tcp_work_q, &conn->send_timer,
					       K_MSEC(tcp_rto));
	}

out:
	return unref;
}

static void tcp_send_process(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, send_timer);
	bool unref;

	k_mutex_lock(&conn->lock, K_FOREVER);

	unref = tcp_send_process_no_lock(conn);

	k_mutex_unlock(&conn->lock);

	if (unref) {
		tcp_conn_unref(conn);
	}
}

static void tcp_send_timer_cancel(struct tcp *conn)
{
	if (conn->in_retransmission == false) {
		return;
	}

	k_delayed_work_cancel(&conn->send_timer);

	{
		struct net_pkt *pkt = tcp_slist(conn, &conn->send_queue, get,
						struct net_pkt, next);
		if (pkt) {
			NET_DBG("%s", log_strdup(tcp_th(pkt)));
			tcp_pkt_unref(pkt);
		}
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		conn->in_retransmission = false;
	} else {
		conn->send_retries = tcp_retries;
		k_delayed_work_submit_to_queue(&tcp_work_q, &conn->send_timer,
					       K_MSEC(tcp_rto));
	}
}

static const char *tcp_state_to_str(enum tcp_state state, bool prefix)
{
	const char *s = NULL;
#define _(_x) case _x: do { s = #_x; goto out; } while (0)
	switch (state) {
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

		if (opt == TCPOPT_END) {
			break;
		} else if (opt == TCPOPT_NOP) {
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
		NET_DBG("opt: %hu, opt_len: %hu", (uint16_t)opt, (uint16_t)opt_len);

		if (opt_len < 2 || opt_len > len) {
			result = false;
			break;
		}

		switch (opt) {
		case TCPOPT_MAXSEG:
			if (opt_len != 4) {
				result = false;
				goto end;
			}

			recv_options->mss =
				ntohs(UNALIGNED_GET((uint16_t *)(options + 2)));
			recv_options->mss_found = true;
			NET_DBG("MSS=%hu", recv_options->mss);
			break;
		case TCPOPT_WINDOW:
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

static size_t tcp_check_pending_data(struct tcp *conn, struct net_pkt *pkt,
				     size_t len)
{
	size_t pending_len = 0;

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT &&
	    !net_pkt_is_empty(conn->queue_recv_data)) {
		struct tcphdr *th = th_get(pkt);
		uint32_t expected_seq = th_seq(th) + len;
		uint32_t pending_seq;

		pending_seq = tcp_get_seq(conn->queue_recv_data->buffer);
		if (pending_seq == expected_seq) {
			pending_len = net_pkt_get_len(conn->queue_recv_data);

			NET_DBG("Found pending data seq %u len %zd",
				pending_seq, pending_len);
			net_buf_frag_add(pkt->buffer,
					 conn->queue_recv_data->buffer);
			conn->queue_recv_data->buffer = NULL;

			k_delayed_work_cancel(&conn->recv_queue_timer);
		}
	}

	return pending_len;
}

static int tcp_data_get(struct tcp *conn, struct net_pkt *pkt, size_t *len)
{
	int ret = 0;

	if (tcp_recv_cb) {
		tcp_recv_cb(conn, pkt);
		goto out;
	}

	if (conn->context->recv_cb) {
		struct net_pkt *up = tcp_pkt_clone(pkt);

		if (!up) {
			ret = -ENOBUFS;
			goto out;
		}

		/* If there is any out-of-order pending data, then pass it
		 * to the application here.
		 */
		*len += tcp_check_pending_data(conn, up, *len);

		net_pkt_cursor_init(up);
		net_pkt_set_overwrite(up, true);

		net_pkt_skip(up, net_pkt_get_len(up) - *len);

		/* Do not pass data to application with TCP conn
		 * locked as there could be an issue when the app tries
		 * to send the data and the conn is locked. So the recv
		 * data is placed in fifo which is flushed in tcp_in()
		 * after unlocking the conn
		 */
		k_fifo_put(&conn->recv_data, up);
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

static int tcp_out_ext(struct tcp *conn, uint8_t flags, struct net_pkt *data,
		       uint32_t seq)
{
	struct net_pkt *pkt;
	int ret = 0;

	pkt = tcp_pkt_alloc(conn, sizeof(struct tcphdr));
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

	ret = tcp_finalize_pkt(pkt);
	if (ret < 0) {
		tcp_pkt_unref(pkt);
		goto out;
	}

	NET_DBG("%s", log_strdup(tcp_th(pkt)));

	if (tcp_send_cb) {
		ret = tcp_send_cb(pkt);
		goto out;
	}

	sys_slist_append(&conn->send_queue, &pkt->next);

	if (tcp_send_process_no_lock(conn)) {
		tcp_conn_unref(conn);
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

static bool tcp_window_full(struct tcp *conn)
{
	bool window_full = !(conn->unacked_len < conn->send_win);

	NET_DBG("conn: %p window_full=%hu", conn, window_full);

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
 out:
	NET_DBG("unsent_len=%d", unsent_len);

	return unsent_len;
}

static int tcp_send_data(struct tcp *conn)
{
	int ret = 0;
	int pos, len;
	struct net_pkt *pkt;

	pos = conn->unacked_len;
	len = MIN3(conn->send_data_total - conn->unacked_len,
		   conn->send_win - conn->unacked_len,
		   conn_mss(conn));

	pkt = tcp_pkt_alloc(conn, len);
	if (!pkt) {
		NET_ERR("conn: %p packet allocation failed, len=%d", conn, len);
		ret = -ENOBUFS;
		goto out;
	}

	ret = tcp_pkt_peek(pkt, conn->send_data, pos, len);
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

		if (tcp_window_full(conn)) {
			subscribe = true;
			break;
		}

		ret = tcp_send_data(conn);
		if (ret < 0) {
			break;
		}
	}

	if (conn->unacked_len) {
		subscribe = true;
	}

	if (k_delayed_work_remaining_get(&conn->send_data_timer)) {
		subscribe = false;
	}

	/* If we have out-of-bufs case, then do not start retransmit timer
	 * yet. The socket layer will catch this and resend data if needed.
	 */
	if (ret == -ENOBUFS) {
		NET_DBG("No bufs, cancelling retransmit timer");
		k_delayed_work_cancel(&conn->send_data_timer);
	}

	if (subscribe) {
		conn->send_data_retries = 0;
		k_delayed_work_submit_to_queue(&tcp_work_q,
					       &conn->send_data_timer,
					       K_MSEC(tcp_rto));
	}
 out:
	return ret;
}

static void tcp_cleanup_recv_queue(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, recv_queue_timer);

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
	struct tcp *conn = CONTAINER_OF(work, struct tcp, send_data_timer);
	bool conn_unref = false;
	int ret;

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("send_data_retries=%hu", conn->send_data_retries);

	if (conn->send_data_retries >= tcp_retries) {
		NET_DBG("conn: %p close, data retransmissions exceeded", conn);
		conn_unref = true;
		goto out;
	}

	conn->data_mode = TCP_DATA_MODE_RESEND;
	conn->unacked_len = 0;

	ret = tcp_send_data(conn);
	if (ret == 0) {
		conn->send_data_retries++;

		if (conn->in_close && conn->send_data_total == 0) {
			NET_DBG("TCP connection in active close, "
				"not disposing yet (waiting %dms)",
				FIN_TIMEOUT_MS);
			k_delayed_work_submit_to_queue(&tcp_work_q,
						       &conn->fin_timer,
						       FIN_TIMEOUT);

			conn_state(conn, TCP_FIN_WAIT_1);

			ret = tcp_out_ext(conn, FIN | ACK, NULL,
				    conn->seq + conn->unacked_len);
			if (ret == 0) {
				conn_seq(conn, + 1);
			}

			goto out;
		}
	}

	k_delayed_work_submit_to_queue(&tcp_work_q, &conn->send_data_timer,
				       K_MSEC(tcp_rto));

 out:
	k_mutex_unlock(&conn->lock);

	if (conn_unref) {
		tcp_conn_unref(conn);
	}
}

static void tcp_timewait_timeout(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, timewait_timer);

	NET_DBG("conn: %p %s", conn, log_strdup(tcp_conn_state(conn, NULL)));

	/* Extra unref from net_tcp_put() */
	net_context_unref(conn->context);
}

static void tcp_establish_timeout(struct tcp *conn)
{
	NET_DBG("Did not receive %s in %dms", "ACK", ACK_TIMEOUT_MS);
	NET_DBG("conn: %p %s", conn, log_strdup(tcp_conn_state(conn, NULL)));

	(void)tcp_conn_unref(conn);
}

static void tcp_fin_timeout(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, fin_timer);

	if (conn->state == TCP_SYN_RECEIVED) {
		tcp_establish_timeout(conn);
		return;
	}

	NET_DBG("Did not receive %s in %dms", "FIN", FIN_TIMEOUT_MS);
	NET_DBG("conn: %p %s", conn, log_strdup(tcp_conn_state(conn, NULL)));

	/* Extra unref from net_tcp_put() */
	net_context_unref(conn->context);
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
	k_sem_init(&conn->connect_sem, 0, UINT_MAX);

	conn->in_connect = false;
	conn->state = TCP_LISTEN;
	conn->recv_win = tcp_window;
	conn->seq = (IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) ||
		     IS_ENABLED(CONFIG_NET_TEST)) ? 0 : sys_rand32_get();

	sys_slist_init(&conn->send_queue);

	k_delayed_work_init(&conn->send_timer, tcp_send_process);
	k_delayed_work_init(&conn->timewait_timer, tcp_timewait_timeout);
	k_delayed_work_init(&conn->fin_timer, tcp_fin_timeout);
	k_delayed_work_init(&conn->send_data_timer, tcp_resend_data);
	k_delayed_work_init(&conn->recv_queue_timer, tcp_cleanup_recv_queue);

	tcp_conn_ref(conn);

	sys_slist_append(&tcp_conns, &conn->next);
out:
	NET_DBG("conn: %p", conn);

	return conn;

fail:
	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT && conn->queue_recv_data) {
		tcp_pkt_unref(conn->queue_recv_data);
		conn->queue_recv_data = NULL;
	}

	k_mem_slab_free(&tcp_conns_slab, (void **)&conn);
	return NULL;
}

int net_tcp_get(struct net_context *context)
{
	int ret = 0;
	struct tcp *conn;

	k_mutex_lock(&tcp_lock, K_FOREVER);

	conn = tcp_conn_alloc();
	if (conn == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	/* Mutually link the net_context and tcp connection */
	conn->context = context;
	context->tcp = conn;
out:
	k_mutex_unlock(&tcp_lock);

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

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp_conns, conn, tmp, next) {

		found = tcp_conn_cmp(conn, pkt);
		if (found) {
			break;
		}
	}

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

		net_ipaddr_copy(&conn_old->context->remote, &conn->dst.sa);

		conn->accepted_conn = conn_old;
	}
 in:
	if (conn) {
		tcp_in(conn, pkt);
	}

	return NET_DROP;
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

	net_context_set_family(conn->context, net_pkt_family(pkt));

	if (tcp_endpoint_set(&conn->dst, pkt, TCP_EP_SRC) < 0) {
		net_context_unref(context);
		conn = NULL;
		goto err;
	}

	if (tcp_endpoint_set(&conn->src, pkt, TCP_EP_DST) < 0) {
		net_context_unref(context);
		conn = NULL;
		goto err;
	}

	NET_DBG("conn: src: %s, dst: %s",
		log_strdup(net_sprint_addr(conn->src.sa.sa_family,
				(const void *)&conn->src.sin.sin_addr)),
		log_strdup(net_sprint_addr(conn->dst.sa.sa_family,
				(const void *)&conn->dst.sin.sin_addr)));

	memcpy(&context->remote, &conn->dst, sizeof(context->remote));
	context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	net_sin_ptr(&context->local)->sin_family = af;

	local_addr.sa_family = net_context_get_family(context);

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_context_get_family(context) == AF_INET6) {
		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_context_get_family(context) == AF_INET) {
		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				      net_sin_ptr(&context->local)->sin_addr);
		}
	}

	ret = net_context_bind(context, &local_addr, sizeof(local_addr));
	if (ret < 0) {
		NET_DBG("Cannot bind accepted context, connection reset");
		net_context_unref(context);
		conn = NULL;
		goto err;
	}

	NET_DBG("context: local: %s, remote: %s",
		log_strdup(net_sprint_addr(
		      local_addr.sa_family,
		      (const void *)&net_sin(&local_addr)->sin_addr)),
		log_strdup(net_sprint_addr(
		      context->remote.sa_family,
		      (const void *)&net_sin(&context->remote)->sin_addr)));

	ret = net_conn_register(IPPROTO_TCP, af,
				&context->remote, &local_addr,
				ntohs(conn->dst.sin.sin_port),/* local port */
				ntohs(conn->src.sin.sin_port),/* remote port */
				tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		NET_ERR("net_conn_register(): %d", ret);
		net_context_unref(context);
		conn = NULL;
		goto err;
	}
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

static void print_seq_list(struct net_buf *buf)
{
	struct net_buf *tmp = buf;
	uint32_t seq;

	while (tmp) {
		seq = tcp_get_seq(tmp);

		NET_DBG("buf %p seq %u len %d", tmp, seq, tmp->len);

		tmp = tmp->frags;
	}
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
		print_seq_list(pkt->buffer);
	}

	if (!net_pkt_is_empty(conn->queue_recv_data)) {
		/* Place the data to correct place in the list. If the data
		 * would not be sequential, then drop this packet.
		 */
		uint32_t pending_seq;

		pending_seq = tcp_get_seq(conn->queue_recv_data->buffer);
		if (pending_seq == seq) {
			/* Put new data before the pending data */
			net_buf_frag_add(pkt->buffer,
					 conn->queue_recv_data->buffer);
			conn->queue_recv_data->buffer = pkt->buffer;
			inserted = true;
		} else {
			struct net_buf *last;

			last = net_buf_frag_last(conn->queue_recv_data->buffer);
			pending_seq = tcp_get_seq(last);

			if ((pending_seq + last->len) == seq_start) {
				/* Put new data after pending data */
				last->frags = pkt->buffer;
				inserted = true;
			}
		}

		if (IS_ENABLED(CONFIG_NET_TCP_LOG_LEVEL_DBG)) {
			if (inserted) {
				NET_DBG("All pending data: conn %p", conn);
				print_seq_list(conn->queue_recv_data->buffer);
			} else {
				NET_DBG("Cannot add new data to queue");
			}
		}
	} else {
		net_pkt_append_buffer(conn->queue_recv_data, pkt->buffer);
		inserted = true;
	}

	if (inserted) {
		/* We need to keep the received data but free the pkt */
		pkt->buffer = NULL;

		if (!k_delayed_work_pending(&conn->recv_queue_timer)) {
			k_delayed_work_submit_to_queue(&tcp_work_q,
						       &conn->recv_queue_timer,
						       K_MSEC(CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT));
		}
	}
}

static bool tcp_data_received(struct tcp *conn, struct net_pkt *pkt,
			      size_t *len)
{
	if (tcp_data_get(conn, pkt, len) < 0) {
		return false;
	}

	net_stats_update_tcp_seg_recv(conn->iface);
	conn_ack(conn, *len);
	tcp_out(conn, ACK);

	return true;
}

static void tcp_out_of_order_data(struct tcp *conn, struct net_pkt *pkt,
				  size_t data_len, uint32_t seq)
{
	size_t headers_len;

	headers_len = net_pkt_get_len(pkt) - data_len;

	/* Get rid of protocol headers from the data */
	if (tcp_pkt_pull(pkt, headers_len) < 0) {
		return;
	}

	/* We received out-of-order data. Try to queue it.
	 */
	tcp_queue_recv_data(conn, pkt, data_len, seq);
}

/* TCP state machine, everything happens here */
static void tcp_in(struct tcp *conn, struct net_pkt *pkt)
{
	struct tcphdr *th = pkt ? th_get(pkt) : NULL;
	uint8_t next = 0, fl = 0;
	bool do_close = false;
	size_t tcp_options_len = th ? (th_off(th) - 5) * 4 : 0;
	struct net_conn *conn_handler = NULL;
	struct net_pkt *recv_pkt;
	void *recv_user_data;
	struct k_fifo *recv_data_fifo;
	size_t len;
	int ret;

	if (th) {
		/* Currently we ignore ECN and CWR flags */
		fl = th_flags(th) & ~(ECN | CWR);
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("%s", log_strdup(tcp_conn_state(conn, pkt)));

	if (th && th_off(th) < 5) {
		tcp_out(conn, RST);
		conn_state(conn, TCP_CLOSED);
		goto next_state;
	}

	if (FL(&fl, &, RST)) {
		/* We only accept RST packet that has valid seq field. */
		if (!tcp_validate_seq(conn, th)) {
			net_stats_update_tcp_seg_rsterr(net_pkt_iface(pkt));
			k_mutex_unlock(&conn->lock);
			return;
		}

		net_stats_update_tcp_seg_rst(net_pkt_iface(pkt));
		conn_state(conn, TCP_CLOSED);
		goto next_state;
	}

	if (tcp_options_len && !tcp_options_check(&conn->recv_options, pkt,
						  tcp_options_len)) {
		NET_DBG("DROP: Invalid TCP option list");
		tcp_out(conn, RST);
		conn_state(conn, TCP_CLOSED);
		goto next_state;
	}

	if (th) {
		size_t max_win;

		conn->send_win = ntohs(th_win(th));

#if defined(CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE)
		if (CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE) {
			max_win = CONFIG_NET_TCP_MAX_SEND_WINDOW_SIZE;
		} else
#endif
		{
			/* Adjust the window so that we do not run out of bufs
			 * while waiting acks.
			 */
			max_win = (CONFIG_NET_BUF_TX_COUNT *
				   CONFIG_NET_BUF_DATA_SIZE) / 3;
		}

		max_win = MAX(max_win, NET_IPV6_MTU);
		if ((size_t)conn->send_win > max_win) {
			NET_DBG("Lowering send window from %zd to %zd",
				(size_t)conn->send_win, max_win);

			conn->send_win = max_win;
		}
	}

next_state:
	len = pkt ? tcp_data_len(pkt) : 0;

	switch (conn->state) {
	case TCP_LISTEN:
		if (FL(&fl, ==, SYN)) {
			conn_ack(conn, th_seq(th) + 1); /* capture peer's isn */
			tcp_out(conn, SYN | ACK);
			conn_seq(conn, + 1);
			next = TCP_SYN_RECEIVED;

			/* Close the connection if we do not receive ACK on time.
			 */
			k_delayed_work_submit_to_queue(&tcp_work_q,
						       &conn->establish_timer,
						       ACK_TIMEOUT);
		} else {
			tcp_out(conn, SYN);
			conn_seq(conn, + 1);
			next = TCP_SYN_SENT;
		}
		break;
	case TCP_SYN_RECEIVED:
		if (FL(&fl, &, ACK, th_ack(th) == conn->seq &&
				th_seq(th) == conn->ack)) {
			k_delayed_work_cancel(&conn->establish_timer);
			tcp_send_timer_cancel(conn);
			next = TCP_ESTABLISHED;
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);

			if (conn->accepted_conn) {
				conn->accepted_conn->accept_cb(
					conn->context,
					&conn->accepted_conn->context->remote,
					sizeof(struct sockaddr), 0,
					conn->accepted_conn->context);

				/* Make sure the accept_cb is only called once.
				 */
				conn->accepted_conn = NULL;
			}

			if (len) {
				if (tcp_data_get(conn, pkt, &len) < 0) {
					break;
				}
				conn_ack(conn, + len);
				tcp_out(conn, ACK);
			}
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
				if (tcp_data_get(conn, pkt, &len) < 0) {
					break;
				}
				conn_ack(conn, + len);
			}
			k_sem_give(&conn->connect_sem);
			next = TCP_ESTABLISHED;
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);
			tcp_out(conn, ACK);
		}
		break;
	case TCP_ESTABLISHED:
		/* full-close */
		if (th && FL(&fl, ==, (FIN | ACK), th_seq(th) == conn->ack)) {
			if (net_tcp_seq_cmp(th_ack(th), conn->seq) > 0) {
				uint32_t len_acked = th_ack(th) - conn->seq;

				conn_seq(conn, + len_acked);
			}

			conn_ack(conn, + 1);
			tcp_out(conn, FIN | ACK);
			next = TCP_LAST_ACK;
			break;
		} else if (th && FL(&fl, ==, FIN, th_seq(th) == conn->ack)) {
			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_CLOSE_WAIT;
			break;
		} else if (th && FL(&fl, ==, (FIN | ACK | PSH),
				    th_seq(th) == conn->ack)) {
			if (len) {
				if (tcp_data_get(conn, pkt, &len) < 0) {
					break;
				}
			}

			conn_ack(conn, + len + 1);
			tcp_out(conn, FIN | ACK);
			next = TCP_LAST_ACK;
			break;
		}

		if (th && net_tcp_seq_cmp(th_ack(th), conn->seq) > 0) {
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
				conn_state(conn, TCP_CLOSED);
				break;
			}

			conn->send_data_total -= len_acked;
			conn->unacked_len -= len_acked;
			conn_seq(conn, + len_acked);
			net_stats_update_tcp_seg_recv(conn->iface);

			conn_send_data_dump(conn);

			if (!k_delayed_work_remaining_get(&conn->send_data_timer)) {
				NET_DBG("conn: %p, Missing a subscription "
					"of the send_data queue timer", conn);
				break;
			}
			conn->send_data_retries = 0;
			k_delayed_work_cancel(&conn->send_data_timer);
			if (conn->data_mode == TCP_DATA_MODE_RESEND) {
				conn->unacked_len = 0;
			}
			conn->data_mode = TCP_DATA_MODE_SEND;

			/* We are closing the connection, send a FIN to peer */
			if (conn->in_close && conn->send_data_total == 0) {
				tcp_send_timer_cancel(conn);
				next = TCP_FIN_WAIT_1;

				tcp_out(conn, FIN | ACK);
				conn_seq(conn, + 1);
				break;
			}

			ret = tcp_send_queued_data(conn);
			if (ret < 0 && ret != -ENOBUFS) {
				tcp_out(conn, RST);
				conn_state(conn, TCP_CLOSED);
				break;
			}
		}

		if (th && len) {
			if (th_seq(th) == conn->ack) {
				if (!tcp_data_received(conn, pkt, &len)) {
					break;
				}
			} else if (net_tcp_seq_greater(conn->ack, th_seq(th))) {
				tcp_out(conn, ACK); /* peer has resent */

				net_stats_update_tcp_seg_ackerr(conn->iface);
			} else if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT) {
				tcp_out_of_order_data(conn, pkt, len,
						      th_seq(th));
			}
		}
		break;
	case TCP_CLOSE_WAIT:
		tcp_out(conn, FIN);
		next = TCP_LAST_ACK;
		break;
	case TCP_LAST_ACK:
		if (th && FL(&fl, ==, ACK, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_CLOSED;
		}
		break;
	case TCP_CLOSED:
		do_close = true;
		break;
	case TCP_FIN_WAIT_1:
		if (th && FL(&fl, ==, (FIN | ACK), th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_TIME_WAIT;
		} else if (th && FL(&fl, ==, FIN, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_CLOSING;
		} else if (th && FL(&fl, ==, ACK, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_FIN_WAIT_2;
		}
		break;
	case TCP_FIN_WAIT_2:
		if (th && (FL(&fl, ==, FIN, th_seq(th) == conn->ack) ||
			   FL(&fl, ==, FIN | ACK, th_seq(th) == conn->ack))) {
			/* Received FIN on FIN_WAIT_2, so cancel the timer */
			k_delayed_work_cancel(&conn->fin_timer);

			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_TIME_WAIT;
		}
		break;
	case TCP_CLOSING:
		if (th && FL(&fl, ==, ACK, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_TIME_WAIT;
		}
		break;
	case TCP_TIME_WAIT:
		k_delayed_work_submit_to_queue(&tcp_work_q,
					       &conn->timewait_timer,
					       K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
		break;
	default:
		NET_ASSERT(false, "%s is unimplemented",
			   tcp_state_to_str(conn->state, true));
	}

	if (next) {
		pkt = NULL;
		th = NULL;
		conn_state(conn, next);
		next = 0;
		goto next_state;
	}

	/* If the conn->context is not set, then the connection was already
	 * closed.
	 */
	if (conn->context) {
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

	/* We must not try to unref the connection while having a connection
	 * lock because the unref will try to acquire net_context lock and the
	 * application might have that lock held already, and that might lead
	 * to a deadlock.
	 */
	if (do_close) {
		tcp_conn_unref(conn);
	}
}

/* Active connection close: send FIN and go to FIN_WAIT_1 state */
int net_tcp_put(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	if (!conn) {
		return -ENOENT;
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("%s", conn ? log_strdup(tcp_conn_state(conn, NULL)) : "");
	NET_DBG("context %p %s", context,
		log_strdup(({ const char *state = net_context_state(context);
					state ? state : "<unknown>"; })));

	if (conn && conn->state == TCP_ESTABLISHED) {
		/* Send all remaining data if possible. */
		if (conn->send_data_total > 0) {
			NET_DBG("conn %p pending %zu bytes", conn,
				conn->send_data_total);
			conn->in_close = true;

			/* How long to wait until all the data has been sent?
			 */
			k_delayed_work_submit_to_queue(&tcp_work_q,
						       &conn->send_data_timer,
						       K_MSEC(tcp_rto));
		} else {
			int ret;

			NET_DBG("TCP connection in active close, not "
				"disposing yet (waiting %dms)", FIN_TIMEOUT_MS);
			k_delayed_work_submit_to_queue(&tcp_work_q,
						       &conn->fin_timer,
						       FIN_TIMEOUT);

			ret = tcp_out_ext(conn, FIN | ACK, NULL,
				    conn->seq + conn->unacked_len);
			if (ret == 0) {
				conn_seq(conn, + 1);
			}

			conn_state(conn, TCP_FIN_WAIT_1);
		}

		/* Make sure we do not delete the connection yet until we have
		 * sent the final ACK.
		 */
		net_context_ref(context);
	}

	k_mutex_unlock(&conn->lock);

	net_context_unref(context);

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
	ARG_UNUSED(context);
	ARG_UNUSED(delta);

	return -EPROTONOSUPPORT;
}

/* net_context queues the outgoing data for the TCP connection */
int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt)
{
	struct tcp *conn = context->tcp;
	struct net_buf *orig_buf = NULL;
	int ret = 0;
	size_t len;

	if (!conn || conn->state != TCP_ESTABLISHED) {
		return -ENOTCONN;
	}

	k_mutex_lock(&conn->lock, K_FOREVER);

	if (tcp_window_full(conn)) {
		/* Trigger resend if the timer is not active */
		/* TODO: use k_work_delayable for send_data_timer so we don't
		 * have to directly access the internals of the legacy object.
		 *
		 * NOTE: It is not permitted to access any fields of k_work or
		 * k_work_delayable directly.  This replacement does so, but
		 * only as a temporary workaround until the legacy
		 * k_delayed_work structure is replaced with k_work_delayable;
		 * at that point k_work_schedule() can be invoked to cause the
		 * work to be scheduled if it is not already scheduled.
		 *
		 * This solution diverges from the original, which would
		 * invoke the retransmit function directly here.  Because that
		 * function is given a k_work pointer, again this cannot be
		 * done without accessing the internal data of the
		 * k_work_delayable structure.
		 *
		 * The original inline retransmission could be supported by
		 * refactoring the work_handler to delegate to a function that
		 * takes conn directly, rather than the work item in which
		 * conn is embedded, and calling that function directly here
		 * and in the work handler.
		 */
		(void)k_work_schedule_for_queue(&tcp_work_q,
						&conn->send_data_timer.work, K_NO_WAIT);

		ret = -EAGAIN;
		goto out;
	}

	len = net_pkt_get_len(pkt);

	if (conn->send_data->buffer) {
		orig_buf = net_buf_frag_last(conn->send_data->buffer);
	}

	net_pkt_append_buffer(conn->send_data, pkt->buffer);
	conn->send_data_total += len;
	NET_DBG("conn: %p Queued %zu bytes (total %zu)", conn, len,
		conn->send_data_total);
	pkt->buffer = NULL;

	ret = tcp_send_queued_data(conn);
	if (ret < 0 && ret != -ENOBUFS) {
		tcp_conn_unref(conn);
		goto out;
	}

	if (ret == -ENOBUFS) {
		/* Restore the original data so that we do not resend the pkt
		 * data multiple times.
		 */
		conn->send_data_total -= len;

		if (orig_buf) {
			pkt->buffer = orig_buf->frags;
			orig_buf->frags = NULL;
		} else {
			pkt->buffer = conn->send_data->buffer;
			conn->send_data->buffer = NULL;
		}
	} else {
		/* We should not free the pkt if there was an error. It will be
		 * freed in net_context.c:context_sendto()
		 */
		tcp_pkt_unref(pkt);
	}
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
		log_strdup(net_sprint_addr(
			    local_addr->sa_family,
			    (const void *)&net_sin(local_addr)->sin_addr)),
		log_strdup(net_sprint_addr(
			    remote_addr->sa_family,
			    (const void *)&net_sin(remote_addr)->sin_addr)));

	conn = context->tcp;
	conn->iface = net_context_get_iface(context);

	switch (net_context_get_family(context)) {
		const struct in_addr *ip4;
		const struct in6_addr *ip6;

	case AF_INET:
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
		ip4 = net_if_ipv4_select_src_addr(
			net_context_get_iface(context),
			&net_sin(remote_addr)->sin_addr);
		conn->src.sin.sin_addr = *ip4;
		net_ipaddr_copy(&conn->dst.sin.sin_addr,
				&net_sin(remote_addr)->sin_addr);
		break;

	case AF_INET6:
		memset(&conn->src, 0, sizeof(struct sockaddr_in6));
		memset(&conn->dst, 0, sizeof(struct sockaddr_in6));

		conn->src.sin6.sin6_family = AF_INET6;
		conn->dst.sin6.sin6_family = AF_INET6;

		conn->dst.sin6.sin6_port = remote_port;
		conn->src.sin6.sin6_port = local_port;

		ip6 = net_if_ipv6_select_src_addr(
					net_context_get_iface(context),
					&net_sin6(remote_addr)->sin6_addr);
		conn->src.sin6.sin6_addr = *ip6;
		net_ipaddr_copy(&conn->dst.sin6.sin6_addr,
				&net_sin6(remote_addr)->sin6_addr);
		break;

	default:
		ret = -EPROTONOSUPPORT;
	}

	NET_DBG("conn: %p src: %s, dst: %s", conn,
		log_strdup(net_sprint_addr(conn->src.sa.sa_family,
				(const void *)&conn->src.sin.sin_addr)),
		log_strdup(net_sprint_addr(conn->dst.sa.sa_family,
				(const void *)&conn->dst.sin.sin_addr)));

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	ret = net_conn_register(net_context_get_ip_proto(context),
				net_context_get_family(context),
				remote_addr, local_addr,
				ntohs(remote_port), ntohs(local_port),
				tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		goto out;
	}

	/* Input of a (nonexistent) packet with no flags set will cause
	 * a TCP connection to be established
	 */
	tcp_in(conn, NULL);

	if (!IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)) {
		conn->in_connect = true;

		if (k_sem_take(&conn->connect_sem, timeout) != 0 &&
		    conn->state != TCP_ESTABLISHED) {
			conn->in_connect = false;
			tcp_conn_unref(conn);
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

	return net_conn_register(net_context_get_ip_proto(context),
				 local_addr.sa_family,
				 context->flags & NET_CONTEXT_REMOTE_ADDR_SET ?
				 &context->remote : NULL,
				 &local_addr,
				 remote_port, local_port,
				 tcp_recv, context,
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

int net_tcp_finalize(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_tcp_hdr *tcp_hdr;

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!tcp_hdr) {
		return -ENOBUFS;
	}

	tcp_hdr->chksum = 0U;

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		tcp_hdr->chksum = net_calc_chksum_tcp(pkt);
	}

	return net_pkt_set_data(pkt, &tcp_access);
}

struct net_tcp_hdr *net_tcp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *tcp_access)
{
	struct net_tcp_hdr *tcp_hdr;

	if (IS_ENABLED(CONFIG_NET_TCP_CHECKSUM) &&
			net_if_need_calc_rx_checksum(net_pkt_iface(pkt)) &&
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
			tcp_in(conn, pkt);
		}
	}

	return NET_DROP;
}

static size_t tp_tcp_recv_cb(struct tcp *conn, struct net_pkt *pkt)
{
	ssize_t len = tcp_data_len(pkt);
	struct net_pkt *up = tcp_pkt_clone(pkt);

	NET_DBG("pkt: %p, len: %zu", pkt, net_pkt_get_len(pkt));

	net_pkt_cursor_init(up);
	net_pkt_set_overwrite(up, true);

	net_pkt_pull(up, net_pkt_get_len(up) - len);

	net_tcp_queue_data(conn->context, up);

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
			tcp_in(conn, NULL);
		}
		if (is("CLOSE", tp->op)) {
			tp_trace = false;
			{
				struct net_context *context;

				conn = (void *)sys_slist_peek_head(&tcp_conns);
				context = conn->context;
				while (tcp_conn_unref(conn))
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
				struct net_pkt *data_pkt;

				data_pkt = tcp_pkt_alloc(conn, len);
				net_pkt_write(data_pkt, buf, len);
				net_pkt_cursor_init(data_pkt);
				net_tcp_queue_data(conn->context, data_pkt);
			}
		}
		break;
	case TP_CONFIG_REQUEST:
		tp_new_find_and_apply(tp_new, "tcp_rto", &tcp_rto, TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_retries", &tcp_retries,
					TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_window", &tcp_window,
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

	return NET_DROP;
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

uint16_t net_tcp_get_recv_mss(const struct tcp *conn)
{
	sa_family_t family = net_context_get_family(conn->context);

	if (family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_context_get_iface(conn->context);

		if (iface && net_if_get_mtu(iface) >= NET_IPV4TCPH_LEN) {
			/* Detect MSS based on interface MTU minus "TCP,IP
			 * header size"
			 */
			return net_if_get_mtu(iface) - NET_IPV4TCPH_LEN;
		}
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

		if (mss < NET_IPV6_MTU) {
			mss = NET_IPV6_MTU;
		}

		return mss;
	}
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

const char *net_tcp_state_str(enum tcp_state state)
{
	return tcp_state_to_str(state, false);
}

void net_tcp_init(void)
{
#if defined(CONFIG_NET_TEST_PROTOCOL)
	/* Register inputs for TTCN-3 based TCP2 sanity check */
	test_cb_register(AF_INET,  IPPROTO_TCP, 4242, 4242, tcp_input);
	test_cb_register(AF_INET6, IPPROTO_TCP, 4242, 4242, tcp_input);
	test_cb_register(AF_INET,  IPPROTO_UDP, 4242, 4242, tp_input);
	test_cb_register(AF_INET6, IPPROTO_UDP, 4242, 4242, tp_input);

	tcp_recv_cb = tp_tcp_recv_cb;
#endif

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

	/* Use private workqueue in order not to block the system work queue.
	 */
	k_work_q_start(&tcp_work_q, work_q_stack,
		       K_KERNEL_STACK_SIZEOF(work_q_stack),
		       THREAD_PRIORITY);

	k_thread_name_set(&tcp_work_q.thread, "tcp_work");
	NET_DBG("Workq started. Thread ID: %p", &tcp_work_q.thread);
}
