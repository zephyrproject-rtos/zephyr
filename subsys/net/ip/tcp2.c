/*
 * Copyright (c) 2018-2019 Intel Corporation
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
#include "connection.h"
#include "net_stats.h"
#include "net_private.h"
#include "tcp2_priv.h"

static int tcp_rto = 500; /* Retransmission timeout, msec */
static int tcp_retries = 3;
static int tcp_window = NET_IPV6_MTU;
static bool tcp_echo;

static sys_slist_t tcp_conns = SYS_SLIST_STATIC_INIT(&tcp_conns);

static K_MEM_SLAB_DEFINE(tcp_conns_slab, sizeof(struct tcp),
				CONFIG_NET_MAX_CONTEXTS, 4);

NET_BUF_POOL_DEFINE(tcp_nbufs, 64/*count*/, 128/*size*/, 0, NULL);

static void tcp_in(struct tcp *conn, struct net_pkt *pkt);

int (*tcp_send_cb)(struct net_pkt *pkt) = NULL;

/* TODO: IPv4 options may enlarge the IPv4 header */
static struct tcphdr *th_get(struct net_pkt *pkt)
{
	struct tcphdr *th = NULL;
	ssize_t len;

	if (pkt == NULL) {
		goto out;
	}

	len = net_pkt_get_len(pkt);

	switch (pkt->family) {
	case AF_INET:
		if (len < (sizeof(struct net_ipv4_hdr) +
				sizeof(struct tcphdr))) {
			NET_WARN("Undersized IPv4 packet: %zd byte(s)", len);
			goto out;
		}
		th = (struct tcphdr *)(ip_get(pkt) + 1);
		break;
	case AF_INET6:
		if (len < (sizeof(struct net_ipv6_hdr) +
				sizeof(struct tcphdr))) {
			NET_WARN("Undersized IPv6 packet: %zd byte(s)", len);
			goto out;
		}
		th = (struct tcphdr *)((u8_t *)ip6_get(pkt) + 1);
		break;
	default:
		break;
	}
out:
	return th;
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
		struct net_ipv4_hdr *ip = ip_get(pkt);
		struct tcphdr *th = th_get(pkt);

		ep->sin.sin_port = src ? th->th_sport : th->th_dport;

		ep->sin.sin_addr = src ? ip->src : ip->dst;

		break;
	}
	case AF_INET6: {
		struct net_ipv6_hdr *ip = (void *)ip_get(pkt);
		struct tcphdr *th = (void *)(ip + 1);

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
		snprintf(s, BUF_SIZE, ":%hu", ntohs(ep->sin.sin_port));
		break;
	case AF_INET: case AF_INET6:
		net_addr_ntop(af, &ep->sin.sin_addr, addr, sizeof(addr));
		snprintf(s, BUF_SIZE, "%s:%hu", addr, ntohs(ep->sin.sin_port));
		break;
	default:
		s = NULL;
		NET_ERR("Unknown address family: %hu", af);
	}
#undef BUF_SIZE
#undef NBUFS
	return s;
}

static const char *tcp_flags(u8_t fl)
{
#define BUF_SIZE 80
	static char buf[BUF_SIZE];
	size_t buf_size = BUF_SIZE;
	char *s = buf;
	*s = '\0';

	if (fl) {
		if (fl & SYN) {
			s += snprintf(s, buf_size, "SYN,");
			buf_size -= s - buf;
		}
		if (fl & FIN) {
			s += snprintf(s, buf_size, "FIN,");
			buf_size -= s - buf;
		}
		if (fl & ACK) {
			s += snprintf(s, buf_size, "ACK,");
			buf_size -= s - buf;
		}
		if (fl & PSH) {
			s += snprintf(s, buf_size, "PSH,");
			buf_size -= s - buf;
		}
		if (fl & RST) {
			s += snprintf(s, buf_size, "RST,");
			buf_size -= s - buf;
		}
		if (fl & URG) {
			s += snprintf(s, buf_size, "URG,");
			buf_size -= s - buf;
		}
		s[strlen(s) - 1] = '\0';
		s--;
	}
#undef BUF_SIZE
	return buf;
}

static const char *tcp_th(struct net_pkt *pkt)
{
#define BUF_SIZE 80
	static char buf[BUF_SIZE];
	size_t buf_size = BUF_SIZE;
	char *s = buf;
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct tcphdr *th = th_get(pkt);
	u8_t fl = th->th_flags;
	ssize_t data_len = ntohs(ip->len) - sizeof(*ip) - th->th_off * 4;

	*s = '\0';

	if (th->th_off < 5) {
		s += snprintf(s, buf_size, "Bogus th_off: %hu", th->th_off);
		buf_size -= s - buf;
		goto end;
	}

	if (fl) {
		if (fl & SYN) {
			s += snprintf(s, buf_size, "SYN=%u,", th_seq(th));
			buf_size -= s - buf;
		}
		if (fl & FIN) {
			s += snprintf(s, buf_size, "FIN=%u,", th_seq(th));
			buf_size -= s - buf;
		}
		if (fl & ACK) {
			s += snprintf(s, buf_size, "ACK=%u,", th_ack(th));
			buf_size -= s - buf;
		}
		if (fl & PSH) {
			s += snprintf(s, buf_size, "PSH,");
			buf_size -= s - buf;
		}
		if (fl & RST) {
			s += snprintf(s, buf_size, "RST,");
			buf_size -= s - buf;
		}
		if (fl & URG) {
			s += snprintf(s, buf_size, "URG,");
			buf_size -= s - buf;
		}
		s[strlen(s) - 1] = '\0';
		s--;
	}

	if (data_len) {
		s += snprintf(s, buf_size, ", len=%zd", data_len);
		buf_size -= s - buf;
	}

	if (((bool)(PSH & fl)) != (data_len > 0)) {
		NET_WARN("Invalid TCP packet: %s, data_len=%zd", buf, data_len);
	}
end:
	return buf;
#undef BUF_SIZE
}

static void tcp_send(struct net_pkt *pkt)
{
	NET_DBG("%s", tcp_th(pkt));

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

	if (is_timer_subscribed(&conn->send_timer)) {
		k_timer_stop(&conn->send_timer);
	}

	while ((pkt = tcp_slist(&conn->send_queue, get,
				struct net_pkt, next))) {
		tcp_pkt_unref(pkt);
	}
}

static void tcp_win_free(struct tcp_win *w, const char *name)
{
	struct net_buf *buf;

	while ((buf = tcp_slist(&w->bufs, get, struct net_buf, user_data))) {
		NET_DBG("%s %p len=%d", name, buf, buf->len);
		tcp_nbuf_unref(buf);
	}

	tcp_free(w);
}

static int tcp_conn_unref(struct tcp *conn)
{
	int ref_count = atomic_dec(&conn->ref_count) - 1;
	int key;

	NET_DBG("conn: %p, ref_count=%d", conn, ref_count);

	if (ref_count) {
		tp_out(conn->iface, "TP_TRACE", "event", "CONN_DELETE");
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

	tcp_win_free(conn->snd, "SND");
	tcp_win_free(conn->rcv, "RCV");

	tcp_free(conn->src);
	tcp_free(conn->dst);

	sys_slist_find_and_remove(&tcp_conns, (sys_snode_t *)conn);

	k_mem_slab_free(&tcp_conns_slab, (void **)&conn);

	memset(conn, 0, sizeof(*conn));

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

static void tcp_send_process(struct k_timer *timer)
{
	struct tcp *conn = k_timer_user_data_get(timer);
	struct net_pkt *pkt = tcp_slist(&conn->send_queue, peek_head,
					struct net_pkt, next);

	NET_DBG("%s %s", tcp_th(pkt), conn->in_retransmission ?
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

		if (forget == false && is_timer_subscribed(
				&conn->send_timer) == false) {
			conn->send_retries = tcp_retries;
			conn->in_retransmission = true;
		}
	}

	if (conn && conn->in_retransmission) {
		k_timer_start(&conn->send_timer, K_MSEC(tcp_rto), 0);
	}
}

static void tcp_send_timer_cancel(struct tcp *conn)
{
	NET_ASSERT_INFO(conn->in_retransmission == true,
			"Not in retransmission");

	k_timer_stop(&conn->send_timer);

	{
		struct net_pkt *pkt = tcp_slist(&conn->send_queue, get,
						struct net_pkt, next);
		NET_DBG("%s", tcp_th(pkt));
		tcp_pkt_unref(pkt);
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		conn->in_retransmission = false;
	} else {
		conn->send_retries = tcp_retries;
		k_timer_start(&conn->send_timer, K_MSEC(tcp_rto), 0);
	}
}

static struct tcp_win *tcp_win_new(void)
{
	struct tcp_win *w = tcp_calloc(1, sizeof(struct tcp_win));

	sys_slist_init(&w->bufs);

	return w;
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
	NET_ASSERT_INFO(s, "Invalid TCP state: %u", state);
out:
	return prefix ? s : (s + 4);
}

static void tcp_win_append(struct tcp_win *w, const char *name,
				const void *data, size_t len)
{
	struct net_buf *buf = tcp_nbuf_alloc(&tcp_nbufs, len);
	size_t prev_len = w->len;

	NET_ASSERT_INFO(len, "Zero length data");

	memcpy(net_buf_add(buf, len), data, len);

	sys_slist_append(&w->bufs, (void *)&buf->user_data);

	w->len += len;

	NET_DBG("%s %p %zu->%zu byte(s)", name, buf, prev_len, w->len);
}

static struct net_buf *tcp_win_peek(struct tcp_win *w, const char *name,
					size_t len)
{
	struct net_buf *out = tcp_nbuf_alloc(&tcp_nbufs, len);
	struct net_buf *buf = tcp_slist(&w->bufs, peek_head, struct net_buf,
					user_data);
	while (buf) {

		if (len <= 0) {
			break;
		}

		memcpy(net_buf_add(out, buf->len), buf->data, buf->len);

		len -= buf->len;

		buf = tcp_slist((sys_snode_t *)&buf->user_data, peek_next,
				struct net_buf, user_data);
	}

	NET_ASSERT_INFO(len == 0, "Unfulfilled request, len: %zu", len);

	NET_DBG("%s len=%zu", name, net_buf_frags_len(out));

	return out;
}

static const char *tcp_conn_state(struct tcp *conn, struct net_pkt *pkt)
{
#define BUF_SIZE 64
	static char buf[BUF_SIZE];

	snprintf(buf, BUF_SIZE, "%s %s %u/%u", pkt ? tcp_th(pkt) : "",
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
		opt_len = options[1];

		NET_DBG("opt: %hu, opt_len: %hu", opt, opt_len);

		if (opt == TCPOPT_PAD) {
			break;
		}
		if (opt == TCPOPT_NOP) {
			opt_len = 1;
		} else if (opt_len < 2 || opt_len > len) {
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
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct tcphdr *th = th_get(pkt);
	u8_t off = th->th_off;
	ssize_t data_len = ntohs(ip->len) - sizeof(*ip) - off * 4;

	if (off > 5 && false == tcp_options_check((th + 1), (off - 5) * 4)) {
		data_len = 0;
	}

	return data_len > 0 ? data_len : 0;
}

static size_t tcp_data_get(struct tcp *conn, struct net_pkt *pkt)
{
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct tcphdr *th = th_get(pkt);
	ssize_t len = tcp_data_len(pkt);

	if (len > 0) {
		void *buf = tcp_malloc(len);

		net_pkt_skip(pkt, sizeof(*ip) + th->th_off * 4);

		net_pkt_read(pkt, buf, len);

		tcp_win_append(conn->rcv, "RCV", buf, len);

		if (tcp_echo) {
			tcp_win_append(conn->snd, "SND", buf, len);
		}

		tcp_free(buf);

		if (conn->context->recv_cb) {
			struct net_pkt *up = net_pkt_clone(pkt, K_NO_WAIT);

			net_pkt_cursor_init(up);
			net_pkt_set_overwrite(up, true);
			net_pkt_skip(up, 40);

			net_context_packet_received(
				(struct net_conn *)conn->context->conn_handler,
				up, NULL, NULL, conn->recv_user_data);
		}
	}

	return len;
}

static void tcp_adj(struct net_pkt *pkt, int req_len)
{
	struct net_ipv4_hdr *ip = ip_get(pkt);
	u16_t len = ntohs(ip->len) + req_len;

	ip->len = htons(len);
}

static struct net_pkt *tcp_pkt_make(struct tcp *conn, u8_t flags)
{
	const size_t len = 40;
	struct net_pkt *pkt = tcp_pkt_alloc(len);
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct tcphdr *th = (void *) (ip + 1);

	memset(ip, 0, len);

	ip->vhl = 0x45;
	ip->ttl = 64;
	ip->proto = IPPROTO_TCP;
	ip->len = htons(len);

	ip->src = conn->src->sin.sin_addr;
	ip->dst = conn->dst->sin.sin_addr;

	th->th_sport = conn->src->sin.sin_port;
	th->th_dport = conn->dst->sin.sin_port;

	th->th_off = 5;
	th->th_flags = flags;
	th->th_win = htons(conn->win);
	th->th_seq = htonl(conn->seq);

	if (ACK & flags) {
		th->th_ack = htonl(conn->ack);
	}

	pkt->iface = conn->iface;

	return pkt;
}

static u32_t sum(void *data, size_t len)
{
	u32_t s = 0;

	for ( ; len > 1; len -= 2, data = (u8_t *)data + 2) {
		s += *((u16_t *)data);
	}

	if (len) {
		s += *((u8_t *)data);
	}

	return s;
}

static uint16_t cs(int32_t s)
{
	return ~((s & 0xFFFF) + (s >> 16));
}

static void tcp_csum(struct net_pkt *pkt)
{
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct tcphdr *th = (void *) (ip + 1);
	u16_t len = ntohs(ip->len) - 20;
	u32_t s;

	ip->chksum = cs(sum(ip, sizeof(*ip)));

	s = sum(&ip->src, sizeof(struct in_addr) * 2);
	s += ntohs(ip->proto + len);

	th->th_sum = 0;
	s += sum(th, len);

	th->th_sum = cs(s);
}

static struct net_pkt *tcp_pkt_linearize(struct net_pkt *pkt)
{
	struct net_pkt *new = tcp_pkt_alloc(0);
	struct net_buf *tmp, *buf = net_pkt_get_frag(new, K_NO_WAIT);

	for (tmp = pkt->frags; tmp; tmp = tmp->frags) {
		memcpy(net_buf_add(buf, tmp->len), tmp->data, tmp->len);
	}

	net_pkt_frag_add(new, buf);

	new->iface = pkt->iface;

	tcp_pkt_unref(pkt);

	return new;
}

static void tcp_chain_free(struct net_buf *head)
{
	struct net_buf *next;

	for ( ; head; head = next) {
		next = head->frags;
		head->frags = NULL;
		tcp_nbuf_unref(head);
	}
}

static void tcp_chain(struct net_pkt *pkt, struct net_buf *head)
{
	struct net_buf *buf;

	for ( ; head; head = head->frags) {
		buf = net_pkt_get_frag(pkt, K_NO_WAIT);
		memcpy(net_buf_add(buf, head->len), head->data, head->len);
		net_pkt_frag_add(pkt, buf);
	}
}

static void tcp_out(struct tcp *conn, u8_t flags, ...)
{
	struct net_pkt *pkt = tcp_pkt_make(conn, flags);

	if (PSH & flags) {
		size_t len = conn->snd->len;
		struct net_buf *buf = tcp_win_peek(conn->snd, "SND", len);

		{
			va_list ap;
			ssize_t *out_len;

			va_start(ap, flags);
			out_len = va_arg(ap, ssize_t *);
			*out_len = len;
			va_end(ap);
		}

		tcp_chain(pkt, buf);

		tcp_chain_free(buf);

		tcp_adj(pkt, len);
	}

	pkt = tcp_pkt_linearize(pkt);

	tcp_csum(pkt);

	NET_DBG("%s", tcp_th(pkt));

	if (tcp_send_cb) {
		tcp_send_cb(pkt);
		goto out;
	}

	sys_slist_append(&conn->send_queue, &pkt->next);

	tcp_send_process(&conn->send_timer);
out:
	return;
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

	conn->state = TCP_LISTEN;

	conn->win = tcp_window;

	conn->rcv = tcp_win_new();
	conn->snd = tcp_win_new();

	sys_slist_init(&conn->send_queue);

	k_timer_init(&conn->send_timer, tcp_send_process, NULL);
	k_timer_user_data_set(&conn->send_timer, conn);

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
		tcp_endpoint_to_string((void *)&context->local),
		tcp_endpoint_to_string((void *)&context->remote), conn);

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

void tcp_input(struct net_pkt *pkt)
{
	struct tcphdr *th = /*tp_tap_input(pkt) ? NULL :*/ th_get(pkt);

	if (th) {
		struct tcp *conn = tcp_conn_search(pkt);

		if (conn == NULL && SYN == th->th_flags) {
			struct net_context *context =
				tcp_calloc(1, sizeof(struct net_context));
			net_tcp_get(context);
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
}

static struct tcp *tcp_conn_new(struct net_pkt *pkt);

static enum net_verdict tcp_pkt_received(struct net_conn *net_conn,
						struct net_pkt *pkt,
						union net_ip_header *ip,
						union net_proto_header *proto,
						void *user_data)
{
	struct tcp *conn = ((struct net_context *)user_data)->tcp;
	u8_t vhl = ip->ipv4->vhl;

	ARG_UNUSED(net_conn);
	ARG_UNUSED(proto);

	if (vhl != 0x45) {
		NET_ERR("conn: %p, Unsupported IP version: 0x%hx", conn, vhl);
		goto out;
	}

	NET_DBG("conn: %p, %s", conn, tcp_th(pkt));

	if (conn && TCP_LISTEN == conn->state) {
		struct tcp *conn_old = conn;

		conn = tcp_conn_new(pkt);

		conn->context->iface = conn_old->context->iface;
		conn->context->user_data = conn_old->context->user_data;

		conn_old->context->remote = conn->dst->sa;

		conn_old->accept_cb(conn->context,
					&conn_old->context->remote,
					sizeof(struct sockaddr), 0,
					conn_old->context);
	}

	if (conn) {
		tcp_in(conn, pkt);
	}
out:
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

	conn->dst = tcp_endpoint_new(pkt, SRC);
	conn->src = tcp_endpoint_new(pkt, DST);

	NET_DBG("conn: src: %s, dst: %s", tcp_endpoint_to_string(conn->src),
		tcp_endpoint_to_string(conn->dst));

	memcpy(&context->remote, conn->dst, sizeof(context->remote));
	context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	((struct sockaddr_in *)&context->local)->sin_family = af;

	NET_DBG("context: local: %s, remote: %s",
		tcp_endpoint_to_string((void *)&context->local),
		tcp_endpoint_to_string((void *)&context->remote));

	ret = net_conn_register(IPPROTO_TCP, af,
				&context->remote, (void *)&context->local,
				ntohs(conn->dst->sin.sin_port),/* local port */
				ntohs(conn->src->sin.sin_port),/* remote port */
				tcp_pkt_received, context,
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
	struct tcphdr *th = th_get(pkt);
	u8_t next = 0, fl = th ? th->th_flags : 0;

	NET_DBG("%s", tcp_conn_state(conn, pkt));

	if (th && th->th_off < 5) {
		tcp_out(conn, RST);
		conn_state(conn, TCP_CLOSED);
		goto next_state;
	}

	if (FL(&fl, &, RST)) {
		conn_state(conn, TCP_CLOSED);
	}
next_state:
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
		if (FL(&fl, &, ACK, th_ack(th) == conn->seq)) {
			tcp_send_timer_cancel(conn);
			next = TCP_ESTABLISHED;
			if (FL(&fl, &, PSH)) {
				tcp_data_get(conn, pkt);
			}
		}
		break;
	case TCP_SYN_SENT:
		/* if we are in SYN SENT and receive only a SYN without an
		 * ACK , shouldn't we go to SYN RECEIVED state? See Figure
		 * 6 of RFC 793
		 */
		if (FL(&fl, &, ACK, th_seq(th) == conn->ack)) {
			tcp_send_timer_cancel(conn);
			next = TCP_ESTABLISHED;
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
		net_context_set_state(conn->context, NET_CONTEXT_CONNECTED);
		if (!th && conn->snd->len) { /* TODO: Out of the loop */
			ssize_t data_len;

			tcp_out(conn, PSH, &data_len);
			conn_seq(conn, + data_len);
			break;
		}
		/* full-close */
		if (FL(&fl, ==, (FIN | ACK), th_seq(th) == conn->ack)) {
			conn_ack(conn, + 1);
			tcp_out(conn, ACK);
			next = TCP_CLOSE_WAIT;
			break;
		}
		if (FL(&fl, &, PSH, th_seq(th) < conn->ack)) {
			tcp_out(conn, ACK); /* peer has resent */
			break;
		}
		if (FL(&fl, &, PSH, th_seq(th) > conn->ack)) {
			tcp_out(conn, RST);
			next = TCP_CLOSED;
			break;
		}
		/* Non piggybacking version for clarity now */
		if (FL(&fl, &, PSH, th_seq(th) == conn->ack)) {
			ssize_t len = tcp_data_get(conn, pkt);

			if (len) {
				conn_ack(conn, + len);
				tcp_out(conn, ACK);

				if (tcp_echo) { /* TODO: Out of the loop? */
					tcp_out(conn, PSH, &len);
					conn_seq(conn, + len);
				}
			} else {
				tcp_out(conn, RST);
				next = TCP_CLOSED;
				break;
			}
		}
		if (FL(&fl, ==, ACK, th_ack(th) == conn->seq)) {
			tcp_win_free(conn->snd, "SND");
			conn->snd = tcp_win_new();
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
		NET_ASSERT_INFO(false, "%s is unimplemented",
				tcp_state_to_str(conn->state, true));
	}

	if (fl) {
		th = NULL;
		NET_WARN("Unconsumed flags: %s (%s) %s",
			 log_strdup(tcp_flags(fl)),
			 log_strdup(tcp_th(pkt)),
			 log_strdup(tcp_conn_state(conn, NULL)));
		tcp_out(conn, RST);
		conn_state(conn, TCP_CLOSED);
		next = 0;
		goto next_state;
	}

	if (next) {
		th = NULL;
		conn_state(conn, next);
		next = 0;
		goto next_state;
	}
}

static ssize_t _tcp_send(struct tcp *conn, const void *buf, size_t len,
				int flags)
{
	tcp_win_append(conn->snd, "SND", buf, len);

	tcp_in(conn, NULL);

	return len;
}

/* close() has been called on the socket */
int net_tcp_put(struct net_context *context)
{
	struct tcp *conn = context->tcp;

	NET_DBG("%s", conn ? tcp_conn_state(conn, NULL) : "");

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

int net_tcp_queue(struct net_context *context, const void *buf, size_t len,
		  const struct msghdr *msghdr)
{
	struct tcp *conn = context->tcp;
	ssize_t ret = 0;

	NET_DBG("conn: %p, buf: %p, len: %zu", conn, buf, len);

	if (conn == NULL) {
		ret = -ESHUTDOWN;
		goto out;
	}

	if (msghdr && msghdr->msg_iovlen > 0) {
		int i;

		for (i = 0; i < msghdr->msg_iovlen; i++) {
			ret = _tcp_send(conn, msghdr->msg_iov[i].iov_base,
					msghdr->msg_iov[i].iov_len, 0);

			if (ret < 0) {
				break;
			}
		}
	} else {
		ret = _tcp_send(conn, buf, len, 0);
	}
out:
	NET_DBG("conn: %p, ret: %zd", conn, ret);

	return ret;
}

/* net context wants to queue data for the TCP connection - not used */
int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt)
{
	ARG_UNUSED(context);
	ARG_UNUSED(pkt);

	return 0;
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
	struct tcp *conn = context->tcp;
	int ret;

	switch (net_context_get_family(context)) {
	case AF_INET:
		net_sin(&conn->src->sa)->sin_port = local_port;
		net_sin(&conn->dst->sa)->sin_port = remote_port;
		break;

	case AF_INET6:
		net_sin6(&conn->src->sa)->sin6_port = local_port;
		net_sin6(&conn->dst->sa)->sin6_port = remote_port;
		break;

	default:
		return -EPROTONOSUPPORT;
	}

	conn->src->sa = *local_addr;
	conn->dst->sa = *remote_addr;

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	ret = net_conn_register(net_context_get_ip_proto(context),
				net_context_get_family(context),
				remote_addr, local_addr,
				ntohs(remote_port), ntohs(local_port),
				tcp_pkt_received, context,
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

	NET_DBG("context: %p, tcp: %p, cb: %p", context, conn, cb);

	conn->accept_cb = cb;

	if (!conn || conn->state != TCP_LISTEN) {
		return -EINVAL;
	}

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
				 tcp_pkt_received, context,
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

void net_tcp_init(void)
{
	/* nothing to do here */
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
static sys_slist_t tp_q = SYS_SLIST_STATIC_INIT(&tp_q);

static struct net_buf *tcp_win_pop(struct tcp_win *w, const char *name,
					size_t len)
{
	struct net_buf *buf, *out = NULL;

	NET_ASSERT_INFO(len, "Invalid request, len: %zu", len);

	NET_ASSERT_INFO(len <= w->len, "Insufficient window length, "
			"len: %zu, req: %zu", w->len, len);
	while (len) {

		buf = tcp_slist(&w->bufs, get, struct net_buf, user_data);

		w->len -= buf->len;

		out = out ? net_buf_frag_add(out, buf) : buf;

		len -= buf->len;
	}

	NET_ASSERT_INFO(len == 0, "Unfulfilled request, len: %zu", len);

	NET_DBG("%s len=%zu", name, net_buf_frags_len(out));

	return out;
}

static ssize_t tcp_recv(int fd, void *buf, size_t len, int flags)
{
	struct tcp *conn = (void *)sys_slist_peek_head(&tcp_conns);
	ssize_t bytes_received = conn->rcv->len;
	struct net_buf *data = tcp_win_pop(conn->rcv, "RCV", bytes_received);

	NET_ASSERT_INFO(bytes_received <= len, "Unimplemented");

	net_buf_linearize(buf, len, data, 0, net_buf_frags_len(data));

	tcp_chain_free(data);

	return bytes_received;
}

static void tcp_step(void)
{
	struct net_pkt *pkt = (void *) sys_slist_get(&tp_q);

	if (pkt) {
		struct tcp *conn = tcp_conn_search(pkt);

		if (conn == NULL) {
			/* conn = tcp_conn_new(pkt); */
		}

		tcp_in(conn, pkt);
	}
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

bool tp_input(struct net_pkt *pkt)
{
	struct net_ipv4_hdr *ip = ip_get(pkt);
	struct net_udp_hdr *uh = (void *) (ip + 1);
	size_t data_len = ntohs(uh->len) - sizeof(*uh);
	struct tcp *conn = tcp_conn_search(pkt);
	size_t json_len = 0;
	struct tp *tp;
	struct tp_new *tp_new;
	enum tp_type type;
	bool responded = false;
	static char buf[512];

	if (ip->proto != IPPROTO_UDP || 4242 != ntohs(uh->dst_port)) {
		return false;
	}

	net_pkt_skip(pkt, sizeof(*ip) + sizeof(*uh));
	net_pkt_read(pkt, buf, data_len);
	buf[data_len] = '\0';
	data_len += 1;

	type = json_decode_msg(buf, data_len);

	data_len = ntohs(uh->len) - sizeof(*uh);
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, sizeof(*ip) + sizeof(*uh));
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
			u8_t data_to_send[128];
			size_t len = tp_str_to_hex(data_to_send,
						sizeof(data_to_send), tp->data);
			tp_output(pkt->iface, buf, 1);
			responded = true;

			{
				struct net_context *context = tcp_calloc(1,
						sizeof(struct net_context));
				net_tcp_get(context);
				conn = context->tcp;
				conn->dst = tcp_endpoint_new(pkt, SRC);
				conn->src = tcp_endpoint_new(pkt, DST);
				conn->iface = pkt->iface;
				tcp_conn_ref(conn);
			}
			conn->seq = tp->seq;
			if (len > 0) {
				tcp_win_append(conn->snd, "SND", data_to_send,
						len);
			}
			tcp_in(conn, NULL);
		}
		if (is("CLOSE", tp->op)) {
			tp_trace = false;
			{
				struct net_context *context;

				conn = (void *)sys_slist_peek_head(&tcp_conns);
				context = conn->context;
				tcp_conn_unref(conn);
				tcp_conn_unref(conn);
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
			ssize_t len = tcp_recv(0, buf, sizeof(buf), 0);

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

			tp_output(pkt->iface, buf, 1);
			responded = true;
			NET_DBG("tcp_send(\"%s\")", tp->data);
			_tcp_send(conn, buf, len, 0);
		}
		break;
	case TP_CONFIG_REQUEST:
		tp_new_find_and_apply(tp_new, "tcp_rto", &tcp_rto, TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_retries", &tcp_retries,
					TP_INT);
		tp_new_find_and_apply(tp_new, "tcp_window", &tcp_window,
					TP_INT);
		tp_new_find_and_apply(tp_new, "tp_trace", &tp_trace, TP_BOOL);
		tp_new_find_and_apply(tp_new, "tcp_echo", &tcp_echo, TP_BOOL);
		break;
	case TP_INTROSPECT_REQUEST:
		json_len = sizeof(buf);
		conn = (void *)sys_slist_peek_head(&tcp_conns);
		tcp_to_json(conn, buf, &json_len);
		break;
	case TP_DEBUG_STOP: case TP_DEBUG_CONTINUE:
		tp_state = tp->type;
		break;
	case TP_DEBUG_STEP:
		tcp_step();
		break;
	default:
		NET_ASSERT_INFO(false, "Unimplemented tp command: %s", tp->msg);
	}

	if (json_len) {
		tp_output(pkt->iface, buf, json_len);
	} else if ((TP_CONFIG_REQUEST == type || TP_COMMAND == type)
			&& responded == false) {
		tp_output(pkt->iface, buf, 1);
	}

	return true;
}
#endif /* end of IS_ENABLED(CONFIG_NET_TEST_PROTOCOL) */
