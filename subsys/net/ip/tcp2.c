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
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/udp.h>
#include "ipv4.h"
#include "ipv6.h"
#include "connection.h"
#include "net_stats.h"
#include "net_private.h"
#include "tcp2_priv.h"

static int tcp_rto = CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT;
static int tcp_retries = 3;
static int tcp_window = NET_IPV6_MTU;

static sys_slist_t tcp_conns = SYS_SLIST_STATIC_INIT(&tcp_conns);

static K_MEM_SLAB_DEFINE(tcp_conns_slab, sizeof(struct tcp),
				CONFIG_NET_MAX_CONTEXTS, 4);

static void tcp_in(struct tcp *conn, struct net_pkt *pkt);
static size_t tcp_data_len(struct net_pkt *pkt);
int net_tcp_finalize(struct net_pkt *pkt);

int (*tcp_send_cb)(struct net_pkt *pkt) = NULL;
size_t (*tcp_recv_cb)(struct tcp *conn, struct net_pkt *pkt) = NULL;

static struct tcphdr *th_get(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(th_access, struct tcphdr);

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	/* net_pkt_ip_hdr_len(), net_pkt_ip_opts_len() account for IPv4/IPv6 */

	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
		     net_pkt_ip_opts_len(pkt));

	return net_pkt_get_data(pkt, &th_access);
}

static size_t tcp_endpoint_len(sa_family_t af)
{
	return (af == AF_INET) ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
}

static union tcp_endpoint *tcp_endpoint_new(struct net_pkt *pkt, int src)
{
	sa_family_t af = net_pkt_family(pkt);
	union tcp_endpoint *ep = tcp_calloc(1, tcp_endpoint_len(af));

	ep->sa.sa_family = af;

	switch (af) {
	case AF_INET: {
		struct net_ipv4_hdr *ip = (struct net_ipv4_hdr *)
			net_pkt_ip_data(pkt);
		struct tcphdr *th = th_get(pkt);

		ep->sin.sin_port = src ? th->th_sport : th->th_dport;

		ep->sin.sin_addr = src ? ip->src : ip->dst;

		break;
	}
	case AF_INET6: {
		struct net_ipv6_hdr *ip = (struct net_ipv6_hdr *)
			net_pkt_ip_data(pkt);
		struct tcphdr *th = th_get(pkt);

		ep->sin6.sin6_port = src ? th->th_sport : th->th_dport;

		ep->sin6.sin6_addr = src ? ip->src : ip->dst;

		break;
	}
	default:
		NET_ERR("Unknown address family: %hu", af);
	}

	return ep;
}

static char *tcp_endpoint_to_string(union tcp_endpoint *ep)
{
#define NBUFS 2
#define BUF_SIZE 80
	sa_family_t af = ep->sa.sa_family;
	static char buf[NBUFS][BUF_SIZE];
	char addr[INET6_ADDRSTRLEN];
	static int i;
	char *s = buf[++i % NBUFS];

	switch (af) {
	case 0:
		snprintk(s, BUF_SIZE, ":%hu", ntohs(ep->sin.sin_port));
		break;
	case AF_INET: case AF_INET6:
		net_addr_ntop(af, &ep->sin.sin_addr, addr, sizeof(addr));
		snprintk(s, BUF_SIZE, "%s:%hu", addr, ntohs(ep->sin.sin_port));
		break;
	default:
		s = NULL;
		NET_ERR("Unknown address family: %hu", af);
	}
#undef BUF_SIZE
#undef NBUFS
	return s;
}

static const char *tcp_flags(u8_t flags)
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

static const char *tcp_th(struct net_pkt *pkt)
{
#define BUF_SIZE 80
	static char buf[BUF_SIZE];
	int len = 0;
	struct tcphdr *th = th_get(pkt);

	buf[0] = '\0';

	if (th->th_off < 5) {
		len += snprintk(buf + len, BUF_SIZE - len,
				"bogus th_off: %hu", (u16_t)th->th_off);
		goto end;
	}

	len += snprintk(buf + len, BUF_SIZE - len,
			"%s Seq=%u", tcp_flags(th->th_flags), th_seq(th));

	if (th->th_flags & ACK) {
		len += snprintk(buf + len, BUF_SIZE - len,
				" Ack=%u", th_ack(th));
	}

	len += snprintk(buf + len, BUF_SIZE - len,
			" Len=%ld", (long)tcp_data_len(pkt));
end:
#undef BUF_SIZE
	return buf;
}

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

	if (net_send_data(pkt) < 0) {
		NET_ERR("net_send_data()");
		tcp_pkt_unref(pkt);
	}
out:
	tcp_pkt_unref(pkt);
}

static void tcp_send_queue_flush(struct tcp *conn)
{
	struct net_pkt *pkt;

	if (k_delayed_work_remaining_get(&conn->send_timer)) {
		k_delayed_work_cancel(&conn->send_timer);
	}

	while ((pkt = tcp_slist(&conn->send_queue, get,
				struct net_pkt, next))) {
		tcp_pkt_unref(pkt);
	}
}

static int tcp_conn_unref(struct tcp *conn)
{
	int ref_count = atomic_dec(&conn->ref_count) - 1;
	int key;

	NET_DBG("conn: %p, ref_count=%d", conn, ref_count);

	if (ref_count) {
		tp_out(net_context_get_family(conn->context), conn->iface,
		       "TP_TRACE", "event", "CONN_DELETE");
		goto out;
	}

	key = irq_lock();

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

	tcp_free(conn->src);
	tcp_free(conn->dst);

	memset(conn, 0, sizeof(*conn));

	sys_slist_find_and_remove(&tcp_conns, (sys_snode_t *)conn);

	k_mem_slab_free(&tcp_conns_slab, (void **)&conn);

	irq_unlock(key);
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

static void tcp_send_process(struct k_work *work)
{
	struct tcp *conn = CONTAINER_OF(work, struct tcp, send_timer);
	struct net_pkt *pkt = tcp_slist(&conn->send_queue, peek_head,
					struct net_pkt, next);

	NET_DBG("%s %s", log_strdup(tcp_th(pkt)), conn->in_retransmission ?
		"in_retransmission" : "");

	if (conn->in_retransmission) {
		if (conn->send_retries > 0) {
			tcp_send(tcp_pkt_clone(pkt));
			conn->send_retries--;
		} else {
			tcp_conn_unref(conn);
			conn = NULL;
		}
	} else {
		u8_t fl = th_get(pkt)->th_flags;
		bool forget = ACK == fl || PSH == fl || (ACK | PSH) == fl ||
			RST & fl;

		pkt = forget ? tcp_slist(&conn->send_queue, get, struct net_pkt,
						next) : tcp_pkt_clone(pkt);
		tcp_send(pkt);

		if (forget == false && !k_delayed_work_remaining_get(
				&conn->send_timer)) {
			conn->send_retries = tcp_retries;
			conn->in_retransmission = true;
		}
	}

	if (conn && conn->in_retransmission) {
		k_delayed_work_submit(&conn->send_timer, K_MSEC(tcp_rto));
	}
}

static void tcp_send_timer_cancel(struct tcp *conn)
{
	NET_ASSERT(conn->in_retransmission == true, "Not in retransmission");

	k_delayed_work_cancel(&conn->send_timer);

	{
		struct net_pkt *pkt = tcp_slist(&conn->send_queue, get,
						struct net_pkt, next);
		NET_DBG("%s", log_strdup(tcp_th(pkt)));
		tcp_pkt_unref(pkt);
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		conn->in_retransmission = false;
	} else {
		conn->send_retries = tcp_retries;
		k_delayed_work_submit(&conn->send_timer, K_MSEC(tcp_rto));
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
	_(TCP_FIN_WAIT1);
	_(TCP_FIN_WAIT2);
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
#define BUF_SIZE 80
	static char buf[BUF_SIZE];

	snprintk(buf, BUF_SIZE, "%s [%s Seq=%u Ack=%u]", pkt ? tcp_th(pkt) : "",
			tcp_state_to_str(conn->state, false),
			conn->seq, conn->ack);
#undef BUF_SIZE
	return buf;
}

static bool tcp_options_check(void *buf, ssize_t len)
{
	bool result = len > 0 && ((len % 4) == 0) ? true : false;
	u8_t *options = buf, opt, opt_len;

	NET_DBG("len=%zd", len);

	for ( ; len >= 2; options += opt_len, len -= opt_len) {
		opt = options[0];
		opt_len = (opt == TCPOPT_END || opt == TCPOPT_NOP) ?
			1 : options[1];

		NET_DBG("opt: %hu, opt_len: %hu", (u16_t)opt, (u16_t)opt_len);

		if (opt == TCPOPT_END) {
			break;
		} else if (opt == TCPOPT_NOP) {
			continue;
		}

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
			break;
		case TCPOPT_WINDOW:
			if (opt_len != 3) {
				result = false;
				goto end;
			}
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

static size_t tcp_data_len(struct net_pkt *pkt)
{
	struct tcphdr *th = th_get(pkt);
	size_t tcp_options_len = (th->th_off - 5) * 4;
	ssize_t len = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
		net_pkt_ip_opts_len(pkt) - sizeof(*th) - tcp_options_len;

	if (tcp_options_len && tcp_options_check((th + 1), tcp_options_len)
			== false) {
		len = 0;
	}

	return len > 0 ? len : 0;
}

static size_t tcp_data_get(struct tcp *conn, struct net_pkt *pkt)
{
	ssize_t len = tcp_data_len(pkt);

	if (tcp_recv_cb) {
		tcp_recv_cb(conn, pkt);
		goto out;
	}

	if (len > 0) {
		if (conn->context->recv_cb) {
			struct net_pkt *up = net_pkt_clone(pkt, K_NO_WAIT);

			net_pkt_cursor_init(up);
			net_pkt_set_overwrite(up, true);

			net_pkt_skip(up, net_pkt_get_len(up) - len);

			net_context_packet_received(
				(struct net_conn *)conn->context->conn_handler,
				up, NULL, NULL, conn->recv_user_data);
		}
	}
 out:
	return len;
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

static int tcp_header_add(struct tcp *conn, struct net_pkt *pkt, u8_t flags)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct tcphdr);
	struct tcphdr *th;

	th = (struct tcphdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!th) {
		return -ENOBUFS;
	}

	memset(th, 0, sizeof(struct tcphdr));

	th->th_sport = conn->src->sin.sin_port;
	th->th_dport = conn->dst->sin.sin_port;

	th->th_off = 5;
	th->th_flags = flags;
	th->th_win = htons(conn->win);
	th->th_seq = htonl(conn->seq);

	if (ACK & flags) {
		th->th_ack = htonl(conn->ack);
	}

	return net_pkt_set_data(pkt, &tcp_access);
}

static int ip_header_add(struct tcp *conn, struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		return net_context_create_ipv4_new(conn->context, pkt,
						&conn->src->sin.sin_addr,
						&conn->dst->sin.sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		return net_context_create_ipv6_new(conn->context, pkt,
						&conn->src->sin6.sin6_addr,
						&conn->dst->sin6.sin6_addr);
	}

	return -EINVAL;
}

static struct net_pkt *tcp_pkt_alloc(struct net_if *iface,
				     sa_family_t family, size_t len)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, len, family,
					IPPROTO_TCP, K_NO_WAIT);

#if IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)
	tp_pkt_alloc(pkt);
#endif
	return pkt;
}

static void tcp_out(struct tcp *conn, u8_t flags, ...)
{
	struct net_pkt *pkt;
	size_t len = 0;
	int r;

	pkt = tcp_pkt_alloc(conn->iface, net_context_get_family(conn->context),
			    sizeof(struct tcphdr));
	if (!pkt) {
		goto fail;
	}

	if (PSH & flags) {
		struct net_pkt *data_pkt;
		va_list ap;
		va_start(ap, flags);
		data_pkt = va_arg(ap, struct net_pkt *);
		va_end(ap);

		len = net_pkt_get_len(data_pkt);
		/* Append the data buffer to pkt */
		net_pkt_append_buffer(pkt, data_pkt->buffer);

		data_pkt->buffer = NULL;
		tcp_pkt_unref(data_pkt);
	}

	pkt->iface = conn->iface;

	r = ip_header_add(conn, pkt);
	if (r < 0) {
		goto fail;
	}

	r = tcp_header_add(conn, pkt, flags);
	if (r < 0) {
		goto fail;
	}

	r = tcp_finalize_pkt(pkt);
	if (r < 0) {
		goto fail;
	}

	if (len) {
		conn_seq(conn, + len);
	}

	NET_DBG("%s", log_strdup(tcp_th(pkt)));

	if (tcp_send_cb) {
		tcp_send_cb(pkt);
		goto out;
	}

	sys_slist_append(&conn->send_queue, &pkt->next);

	tcp_send_process((struct k_work *)&conn->send_timer);
out:
	return;

fail:
	if (pkt) {
		tcp_pkt_unref(pkt);
	}
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
		goto out;
	}

	memset(conn, 0, sizeof(*conn));

	k_mutex_init(&conn->lock);

	conn->state = TCP_LISTEN;

	conn->win = tcp_window;

	sys_slist_init(&conn->send_queue);

	k_delayed_work_init(&conn->send_timer, tcp_send_process);

	tcp_conn_ref(conn);

	sys_slist_append(&tcp_conns, (sys_snode_t *)conn);
out:
	NET_DBG("conn: %p", conn);

	return conn;
}

int net_tcp_get(struct net_context *context)
{
	int ret = 0, key = irq_lock();
	struct tcp *conn;

	conn = tcp_conn_alloc();
	if (conn == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	/* Mutually link the net_context and tcp connection */
	conn->context = context;
	context->tcp = conn;
out:
	irq_unlock(key);

	NET_DBG("context: %p (local: %s, remote: %s), conn: %p", context,
		log_strdup(tcp_endpoint_to_string((void *)&context->local)),
		log_strdup(tcp_endpoint_to_string((void *)&context->remote)),
		conn);

	return ret;
}

static bool tcp_endpoint_cmp(union tcp_endpoint *ep, struct net_pkt *pkt,
				int which)
{
	union tcp_endpoint *ep_new = tcp_endpoint_new(pkt, which);
	bool is_equal = memcmp(ep, ep_new, tcp_endpoint_len(ep->sa.sa_family)) ?
		false : true;

	tcp_free(ep_new);

	return is_equal;
}

static bool tcp_conn_cmp(struct tcp *conn, struct net_pkt *pkt)
{
	return tcp_endpoint_cmp(conn->src, pkt, DST) &&
		tcp_endpoint_cmp(conn->dst, pkt, SRC);
}

static struct tcp *tcp_conn_search(struct net_pkt *pkt)
{
	bool found = false;
	struct tcp *conn;

	SYS_SLIST_FOR_EACH_CONTAINER(&tcp_conns, conn, next) {

		if (NULL == conn->src || NULL == conn->dst) {
			continue;
		}

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

	if (th->th_flags & SYN && !(th->th_flags & ACK)) {
		struct tcp *conn_old = ((struct net_context *)user_data)->tcp;

		conn = tcp_conn_new(pkt);

		conn_old->context->remote = conn->dst->sa;

		conn_old->accept_cb(conn->context,
				    &conn_old->context->remote,
				    sizeof(struct sockaddr), 0,
				    conn_old->context);
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
	int ret;

	ret = net_context_get(af, SOCK_STREAM, IPPROTO_TCP, &context);
	if (ret < 0) {
		NET_ERR("net_context_get(): %d", ret);
		goto err;
	}

	conn = context->tcp;
	conn->iface = pkt->iface;

	net_context_set_family(conn->context, pkt->family);

	conn->dst = tcp_endpoint_new(pkt, SRC);
	conn->src = tcp_endpoint_new(pkt, DST);

	NET_DBG("conn: src: %s, dst: %s",
		log_strdup(tcp_endpoint_to_string(conn->src)),
		log_strdup(tcp_endpoint_to_string(conn->dst)));

	memcpy(&context->remote, conn->dst, sizeof(context->remote));
	context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	((struct sockaddr_in *)&context->local)->sin_family = af;

	NET_DBG("context: local: %s, remote: %s",
		log_strdup(tcp_endpoint_to_string((void *)&context->local)),
		log_strdup(tcp_endpoint_to_string((void *)&context->remote)));

	ret = net_conn_register(IPPROTO_TCP, af,
				&context->remote, (void *)&context->local,
				ntohs(conn->dst->sin.sin_port),/* local port */
				ntohs(conn->src->sin.sin_port),/* remote port */
				tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		NET_ERR("net_conn_register(): %d", ret);
		net_context_unref(context);
		conn = NULL;
		goto err;
	}
err:
	return conn;
}

/* TCP state machine, everything happens here */
static void tcp_in(struct tcp *conn, struct net_pkt *pkt)
{
	struct tcphdr *th = pkt ? th_get(pkt) : NULL;
	u8_t next = 0, fl = th ? th->th_flags : 0;
	size_t len;

	k_mutex_lock(&conn->lock, K_FOREVER);

	NET_DBG("%s", log_strdup(tcp_conn_state(conn, pkt)));

	if (th && th->th_off < 5) {
		tcp_out(conn, RST);
		conn_state(conn, TCP_CLOSED);
		goto next_state;
	}

	if (FL(&fl, &, RST)) {
		conn_state(conn, TCP_CLOSED);
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
		} else {
			tcp_out(conn, SYN);
			conn_seq(conn, + 1);
			next = TCP_SYN_SENT;
		}
		break;
	case TCP_SYN_RECEIVED:
		if (FL(&fl, &, ACK, th_ack(th) == conn->seq &&
				th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_ESTABLISHED;
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);
			if (len) {
				tcp_data_get(conn, pkt);
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
		if (FL(&fl, &, ACK, th && th_ack(th) == conn->seq)) {
			tcp_send_timer_cancel(conn);
			next = TCP_ESTABLISHED;
			net_context_set_state(conn->context,
					      NET_CONTEXT_CONNECTED);
			if (FL(&fl, &, PSH)) {
				tcp_data_get(conn, pkt);
			}
			if (FL(&fl, &, SYN)) {
				conn_ack(conn, th_seq(th) + 1);
				tcp_out(conn, ACK);
			}
		}
		break;
	case TCP_ESTABLISHED:
		/* full-close */
		if (th && FL(&fl, ==, (FIN | ACK), th_seq(th) == conn->ack)) {
			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_CLOSE_WAIT;
			break;
		}
		if (len) {
			if (th_seq(th) == conn->ack) {
				tcp_data_get(conn, pkt);
				conn_ack(conn, + len);
				tcp_out(conn, ACK);
			} else if (th_seq(th) < conn->ack) {
				tcp_out(conn, ACK); /* peer has resent */
			}
		}
		break; /* TODO: Catch all the rest here */
	case TCP_CLOSE_WAIT:
		tcp_out(conn, FIN | ACK);
		next = TCP_LAST_ACK;
		break;
	case TCP_LAST_ACK:
		if (FL(&fl, ==, ACK, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_CLOSED;
		}
		break;
	case TCP_CLOSED:
		fl = 0;
		tcp_conn_unref(conn);
		break;
	case TCP_TIME_WAIT:
	case TCP_CLOSING:
	case TCP_FIN_WAIT1:
	case TCP_FIN_WAIT2:
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

	k_mutex_unlock(&conn->lock);
}

/* close() has been called on the socket */
int net_tcp_put(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	NET_DBG("%s", conn ? log_strdup(tcp_conn_state(conn, NULL)) : "");

	if (conn) {
		conn->state = TCP_CLOSE_WAIT;
		tcp_in(conn, NULL);
	}

	net_context_unref(context);

	return 0;
}

int net_tcp_listen(struct net_context *context)
{
	/* when created, tcp connections are in state TCP_LISTEN */
	net_context_set_state(context, NET_CONTEXT_LISTENING);

	return 0;
}

int net_tcp_update_recv_wnd(struct net_context *context, s32_t delta)
{
	ARG_UNUSED(context);
	ARG_UNUSED(delta);

	return -EPROTONOSUPPORT;
}

/* net context wants to queue data for the TCP connection */
int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt)
{
	struct tcp *conn = context->tcp;
	int ret = 0;

	NET_DBG("conn: %p, len: %zu", conn, net_pkt_get_len(pkt));

	if (!conn || conn->state != TCP_ESTABLISHED) {
		ret = -ENOTCONN;
		goto out;
	}

	tcp_out(conn, PSH | ACK, pkt);
out:
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
		    u16_t remote_port, u16_t local_port,
		    s32_t timeout, net_context_connect_cb_t cb, void *user_data)
{
	struct tcp *conn;
	int ret;

	NET_DBG("context: %p, local: %s, remote: %s", context,
		log_strdup(tcp_endpoint_to_string((void *)local_addr)),
		log_strdup(tcp_endpoint_to_string((void *)remote_addr)));

	conn = context->tcp;
	conn->iface = net_context_get_iface(context);

	switch (net_context_get_family(context)) {
		const struct in_addr *ip4;
		const struct in6_addr *ip6;

	case AF_INET:
		conn->src = tcp_calloc(1, tcp_endpoint_len(AF_INET));
		conn->dst = tcp_calloc(1, tcp_endpoint_len(AF_INET));

		conn->src->sa.sa_family = AF_INET;
		conn->dst->sa.sa_family = AF_INET;

		conn->dst->sin.sin_port = remote_port;
		conn->src->sin.sin_port = local_port;

		/* we have to select the source address here as
		 * net_context_create_ipv4_new() is not called in the packet
		 * output chain
		 */
		ip4 = net_if_ipv4_select_src_addr(net_context_get_iface(context),
						  (struct in_addr *)remote_addr);
		conn->src->sin.sin_addr = *ip4;
		conn->dst->sa = *remote_addr;
		break;

	case AF_INET6:
		conn->src = tcp_calloc(1, tcp_endpoint_len(AF_INET6));
		conn->dst = tcp_calloc(1, tcp_endpoint_len(AF_INET6));

		memset(conn->src, 0, tcp_endpoint_len(AF_INET6));
		memset(conn->dst, 0, tcp_endpoint_len(AF_INET6));

		conn->src->sin6.sin6_family = AF_INET6;
		conn->dst->sin6.sin6_family = AF_INET6;

		conn->dst->sin6.sin6_port = remote_port;
		conn->src->sin6.sin6_port = local_port;

		ip6 = net_if_ipv6_select_src_addr(net_context_get_iface(context),
						  (struct in6_addr *)remote_addr);
		conn->src->sin6.sin6_addr = *ip6;
		conn->dst->sin6.sin6_addr = ((struct sockaddr_in6 *)remote_addr)->sin6_addr;
		break;

	default:
		return -EPROTONOSUPPORT;
	}

	NET_DBG("conn: %p, local: %s, remote: %s", conn,
		log_strdup(tcp_endpoint_to_string(conn->src)),
		log_strdup(tcp_endpoint_to_string(conn->dst)));

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	ret = net_conn_register(net_context_get_ip_proto(context),
				net_context_get_family(context),
				remote_addr, local_addr,
				ntohs(remote_port), ntohs(local_port),
				tcp_recv, context,
				&context->conn_handler);
	if (ret < 0) {
		return ret;
	}

	/* Input of a (nonexistent) packet with no flags set will cause
	 * a TCP connection to be established
	 */
	tcp_in(conn, NULL);

	return 0;
}

int net_tcp_accept(struct net_context *context, net_tcp_accept_cb_t cb,
		   void *user_data)
{
	struct tcp *conn = context->tcp;
	struct sockaddr local_addr = { };
	u16_t local_port, remote_port;

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

		if (conn == NULL && SYN == th->th_flags) {
			struct net_context *context =
				tcp_calloc(1, sizeof(struct net_context));
			net_tcp_get(context);
			net_context_set_family(context, pkt->family);
			conn = context->tcp;
			conn->dst = tcp_endpoint_new(pkt, SRC);
			conn->src = tcp_endpoint_new(pkt, DST);
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
				net_context_set_family(context, pkt->family);
				conn = context->tcp;
				conn->dst = tcp_endpoint_new(pkt, SRC);
				conn->src = tcp_endpoint_new(pkt, DST);
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
				struct net_pkt *pkt = tcp_pkt_alloc(0);
				struct net_buf *nb =
					net_pkt_get_frag(pkt, K_NO_WAIT);
				memcpy(net_buf_add(nb, len), buf, len);
				net_pkt_frag_insert(pkt, nb);
				net_tcp_queue_data(conn->context, pkt);
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

static void test_cb_register(sa_family_t family, u8_t proto, u16_t remote_port,
			     u16_t local_port, net_conn_cb_t cb)
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
}
