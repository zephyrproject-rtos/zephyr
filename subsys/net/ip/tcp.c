/** @file
 * @brief TCP handler
 *
 * Handle TCP connections.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2011-2015 by Andrey Butok. FNET Community.
 * Copyright 2008-2010 by Andrey Butok. Freescale Semiconductor, Inc.
 * Copyright 2003 by Alexey Shervashidze, Andrey Butok. Motorola SPS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_TCP)
#define SYS_LOG_DOMAIN "net/tcp"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/net_context.h>
#include <misc/byteorder.h>

#include "connection.h"
#include "net_private.h"

#include "ipv6.h"
#include "ipv4.h"
#include "tcp.h"
#include "net_stats.h"

#define ALLOC_TIMEOUT 500

/*
 * Each TCP connection needs to be tracked by net_context, so
 * we need to allocate equal number of control structures here.
 */
#define NET_MAX_TCP_CONTEXT CONFIG_NET_MAX_CONTEXTS
static struct net_tcp tcp_context[NET_MAX_TCP_CONTEXT];

/* 2MSL timeout, where "MSL" is arbitrarily 2 minutes in the RFC */
#if defined(CONFIG_NET_TCP_2MSL_TIME)
#define TIME_WAIT_MS K_SECONDS(CONFIG_NET_TCP_2MSL_TIME)
#else
#define TIME_WAIT_MS K_SECONDS(2 * 2 * 60)
#endif

struct tcp_segment {
	u32_t seq;
	u32_t ack;
	u16_t wnd;
	u8_t flags;
	u8_t optlen;
	void *options;
	struct sockaddr_ptr *src_addr;
	const struct sockaddr *dst_addr;
};

#if defined(CONFIG_NET_DEBUG_TCP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
static char upper_if_set(char chr, bool set)
{
	if (set) {
		return chr & ~0x20;
	}

	return chr | 0x20;
}

static void net_tcp_trace(struct net_pkt *pkt, struct net_tcp *tcp)
{
	struct net_tcp_hdr hdr, *tcp_hdr;
	u32_t rel_ack, ack;
	u8_t flags;

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return;
	}

	flags = NET_TCP_FLAGS(tcp_hdr);
	ack = sys_get_be32(tcp_hdr->ack);

	if (!tcp->sent_ack) {
		rel_ack = 0;
	} else {
		rel_ack = ack ? ack - tcp->sent_ack : 0;
	}

	NET_DBG("[%p] pkt %p src %u dst %u seq 0x%04x (%u) ack 0x%04x (%u/%u) "
		"flags %c%c%c%c%c%c win %u chk 0x%04x",
		tcp, pkt,
		ntohs(tcp_hdr->src_port),
		ntohs(tcp_hdr->dst_port),
		sys_get_be32(tcp_hdr->seq),
		sys_get_be32(tcp_hdr->seq),
		ack,
		ack,
		/* This tells how many bytes we are acking now */
		rel_ack,
		upper_if_set('u', flags & NET_TCP_URG),
		upper_if_set('a', flags & NET_TCP_ACK),
		upper_if_set('p', flags & NET_TCP_PSH),
		upper_if_set('r', flags & NET_TCP_RST),
		upper_if_set('s', flags & NET_TCP_SYN),
		upper_if_set('f', flags & NET_TCP_FIN),
		sys_get_be16(tcp_hdr->wnd),
		ntohs(tcp_hdr->chksum));
}
#else
#define net_tcp_trace(...)
#endif /* CONFIG_NET_DEBUG_TCP */

static inline u32_t retry_timeout(const struct net_tcp *tcp)
{
	return ((u32_t)1 << tcp->retry_timeout_shift) *
				CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT;
}

#define is_6lo_technology(pkt)						    \
	(IS_ENABLED(CONFIG_NET_IPV6) &&	net_pkt_family(pkt) == AF_INET6 &&  \
	 ((IS_ENABLED(CONFIG_NET_L2_BT) &&			    \
	   net_pkt_ll_dst(pkt)->type == NET_LINK_BLUETOOTH) ||		    \
	  (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&			    \
	   net_pkt_ll_dst(pkt)->type == NET_LINK_IEEE802154)))

/* The ref should not be done for Bluetooth and IEEE 802.15.4 which use
 * IPv6 header compression (6lo). For BT and 802.15.4 we copy the pkt
 * chain we are about to send so it is fine if the network driver
 * releases it. As we have our own copy of the sent data, we do not
 * need to take a reference of it. See also net_tcp_send_pkt().
 *
 * Note that this is macro so that we get information who called the
 * net_pkt_ref() if memory debugging is active.
 */
#define do_ref_if_needed(tcp, pkt)					\
	do {								\
		if (!is_6lo_technology(pkt)) {				\
			NET_DBG("[%p] ref pkt %p new ref %d (%s:%d)",	\
				tcp, pkt, pkt->ref + 1, __func__,	\
				__LINE__);				\
			pkt = net_pkt_ref(pkt);				\
		}							\
	} while (0)

static void abort_connection(struct net_tcp *tcp)
{
	struct net_context *ctx = tcp->context;

	NET_DBG("[%p] segment retransmission exceeds %d, resetting context %p",
		tcp, CONFIG_NET_TCP_RETRY_COUNT, ctx);

	if (ctx->recv_cb) {
		ctx->recv_cb(ctx, NULL, -ECONNRESET, tcp->recv_user_data);
	}

	net_context_unref(ctx);
}

static void tcp_retry_expired(struct k_work *work)
{
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp, retry_timer);
	struct net_pkt *pkt;

	/* Double the retry period for exponential backoff and resent
	 * the first (only the first!) unack'd packet.
	 */
	if (!sys_slist_is_empty(&tcp->sent_list)) {
		tcp->retry_timeout_shift++;

		if (tcp->retry_timeout_shift > CONFIG_NET_TCP_RETRY_COUNT) {
			abort_connection(tcp);
			return;
		}

		k_delayed_work_submit(&tcp->retry_timer, retry_timeout(tcp));

		pkt = CONTAINER_OF(sys_slist_peek_head(&tcp->sent_list),
				   struct net_pkt, sent_list);

		if (net_pkt_sent(pkt)) {
			do_ref_if_needed(tcp, pkt);
			net_pkt_set_sent(pkt, false);
		}

		net_pkt_set_queued(pkt, true);

		if (net_tcp_send_pkt(pkt) < 0 && !is_6lo_technology(pkt)) {
			NET_DBG("retry %u: [%p] pkt %p send failed",
				tcp->retry_timeout_shift, tcp, pkt);
			net_pkt_unref(pkt);
		} else {
			NET_DBG("retry %u: [%p] sent pkt %p",
				tcp->retry_timeout_shift, tcp, pkt);
			if (IS_ENABLED(CONFIG_NET_STATISTICS_TCP) &&
			    !is_6lo_technology(pkt)) {
				net_stats_update_tcp_seg_rexmit();
			}
		}
	} else if (IS_ENABLED(CONFIG_NET_TCP_TIME_WAIT)) {
		if (tcp->fin_sent && tcp->fin_rcvd) {
			NET_DBG("[%p] Closing connection (context %p)",
				tcp, tcp->context);
			net_context_unref(tcp->context);
		}
	}
}

struct net_tcp *net_tcp_alloc(struct net_context *context)
{
	int i, key;

	key = irq_lock();
	for (i = 0; i < NET_MAX_TCP_CONTEXT; i++) {
		if (!net_tcp_is_used(&tcp_context[i])) {
			tcp_context[i].flags |= NET_TCP_IN_USE;
			break;
		}
	}
	irq_unlock(key);

	if (i >= NET_MAX_TCP_CONTEXT) {
		return NULL;
	}

	memset(&tcp_context[i], 0, sizeof(struct net_tcp));

	tcp_context[i].flags = NET_TCP_IN_USE;
	tcp_context[i].state = NET_TCP_CLOSED;
	tcp_context[i].context = context;

	tcp_context[i].send_seq = tcp_init_isn();
	tcp_context[i].recv_max_ack = tcp_context[i].send_seq + 1u;
	tcp_context[i].recv_wnd = min(NET_TCP_MAX_WIN, NET_TCP_BUF_MAX_LEN);
	tcp_context[i].send_mss = NET_TCP_DEFAULT_MSS;

	tcp_context[i].accept_cb = NULL;

	k_delayed_work_init(&tcp_context[i].retry_timer, tcp_retry_expired);
	k_sem_init(&tcp_context[i].connect_wait, 0, UINT_MAX);

	return &tcp_context[i];
}

static void ack_timer_cancel(struct net_tcp *tcp)
{
	k_delayed_work_cancel(&tcp->ack_timer);
}

static void fin_timer_cancel(struct net_tcp *tcp)
{
	k_delayed_work_cancel(&tcp->fin_timer);
}

static void retry_timer_cancel(struct net_tcp *tcp)
{
	k_delayed_work_cancel(&tcp->retry_timer);
}

int net_tcp_release(struct net_tcp *tcp)
{
	struct net_pkt *pkt;
	struct net_pkt *tmp;
	int key;

	if (!PART_OF_ARRAY(tcp_context, tcp)) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp->sent_list, pkt, tmp,
					  sent_list) {
		sys_slist_remove(&tcp->sent_list, NULL, &pkt->sent_list);
		net_pkt_unref(pkt);
	}

	retry_timer_cancel(tcp);
	k_sem_reset(&tcp->connect_wait);

	ack_timer_cancel(tcp);
	fin_timer_cancel(tcp);

	net_tcp_change_state(tcp, NET_TCP_CLOSED);
	tcp->context = NULL;

	key = irq_lock();
	tcp->flags &= ~(NET_TCP_IN_USE | NET_TCP_RECV_MSS_SET);
	irq_unlock(key);

	NET_DBG("[%p] Disposed of TCP connection state", tcp);

	return 0;
}

static inline u8_t net_tcp_add_options(struct net_buf *header, size_t len,
				       void *data)
{
	u8_t optlen;

	memcpy(net_buf_add(header, len), data, len);

	/* Set the length (this value is saved in 4-byte words format) */
	if ((len & 0x3u) != 0u) {
		optlen = (len & 0xfffCu) + 4u;
	} else {
		optlen = len;
	}

	return optlen;
}

static int finalize_segment(struct net_context *context, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		return net_ipv4_finalize(context, pkt);
	} else
#endif
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		return net_ipv6_finalize(context, pkt);
	}
#endif
	{
	}

	return 0;
}

static struct net_pkt *prepare_segment(struct net_tcp *tcp,
				       struct tcp_segment *segment,
				       struct net_pkt *pkt)
{
	struct net_buf *header, *tail = NULL;
	struct net_context *context = tcp->context;
	struct net_tcp_hdr *tcp_hdr;
	u16_t dst_port, src_port;
	bool pkt_allocated;
	u8_t optlen = 0;

	NET_ASSERT(context);

	if (pkt) {
		/* TCP transmit data comes in with a pre-allocated
		 * net_pkt at the head (so that net_context_send can find
		 * the context), and the data after.  Rejigger so we
		 * can insert a TCP header cleanly
		 */
		tail = pkt->frags;
		pkt->frags = NULL;
		pkt_allocated = false;
	} else {
		pkt = net_pkt_get_tx(context, ALLOC_TIMEOUT);
		if (!pkt) {
			return NULL;
		}

		pkt_allocated = true;
	}

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		net_ipv4_create(context, pkt,
				net_sin_ptr(segment->src_addr)->sin_addr,
				&(net_sin(segment->dst_addr)->sin_addr));
		dst_port = net_sin(segment->dst_addr)->sin_port;
		src_port = ((struct sockaddr_in_ptr *)&context->local)->
								sin_port;
		NET_IPV4_HDR(pkt)->proto = IPPROTO_TCP;
	} else
#endif
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		net_ipv6_create(tcp->context, pkt,
				net_sin6_ptr(segment->src_addr)->sin6_addr,
				&(net_sin6(segment->dst_addr)->sin6_addr));
		dst_port = net_sin6(segment->dst_addr)->sin6_port;
		src_port = ((struct sockaddr_in6_ptr *)&context->local)->
								sin6_port;
		NET_IPV6_HDR(pkt)->nexthdr = IPPROTO_TCP;
	} else
#endif
	{
		NET_DBG("[%p] Protocol family %d not supported", tcp,
			net_pkt_family(pkt));

		if (pkt_allocated) {
			net_pkt_unref(pkt);
		} else {
			pkt->frags = tail;
		}

		return NULL;
	}

	header = net_pkt_get_data(context, ALLOC_TIMEOUT);
	if (!header) {
		if (pkt_allocated) {
			net_pkt_unref(pkt);
		} else {
			pkt->frags = tail;
		}

		return NULL;
	}

	net_pkt_frag_add(pkt, header);

	tcp_hdr = (struct net_tcp_hdr *)net_buf_add(header, NET_TCPH_LEN);

	if (segment->options && segment->optlen) {
		optlen = net_tcp_add_options(header, segment->optlen,
					segment->options);
	}

	tcp_hdr->offset = (NET_TCPH_LEN + optlen) << 2;

	tcp_hdr->src_port = src_port;
	tcp_hdr->dst_port = dst_port;
	sys_put_be32(segment->seq, tcp_hdr->seq);
	sys_put_be32(segment->ack, tcp_hdr->ack);
	tcp_hdr->flags = segment->flags;
	sys_put_be16(segment->wnd, tcp_hdr->wnd);
	tcp_hdr->urg[0] = 0;
	tcp_hdr->urg[1] = 0;

	if (tail) {
		net_pkt_frag_add(pkt, tail);
	}

	if (finalize_segment(context, pkt) < 0) {
		if (pkt_allocated) {
			net_pkt_unref(pkt);
		}

		return NULL;
	}

	net_tcp_trace(pkt, tcp);

	return pkt;
}

u32_t net_tcp_get_recv_wnd(const struct net_tcp *tcp)
{
	return tcp->recv_wnd;
}

int net_tcp_prepare_segment(struct net_tcp *tcp, u8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr_ptr *local,
			    const struct sockaddr *remote,
			    struct net_pkt **send_pkt)
{
	u32_t seq;
	u16_t wnd;
	struct tcp_segment segment = { 0 };

	if (!local) {
		local = &tcp->context->local;
	}

	seq = tcp->send_seq;

	if (flags & NET_TCP_ACK) {
		if (net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_1) {
			if (flags & NET_TCP_FIN) {
				/* FIN is used here only to determine which
				 * state to go to next; it's not to be used
				 * in the sent segment.
				 */
				flags &= ~NET_TCP_FIN;
				net_tcp_change_state(tcp, NET_TCP_TIME_WAIT);
			} else {
				net_tcp_change_state(tcp, NET_TCP_CLOSING);
			}
		} else if (net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_2) {
			net_tcp_change_state(tcp, NET_TCP_TIME_WAIT);
		} else if (net_tcp_get_state(tcp) == NET_TCP_CLOSE_WAIT) {
			tcp->flags |= NET_TCP_IS_SHUTDOWN;
			flags |= NET_TCP_FIN;
			net_tcp_change_state(tcp, NET_TCP_LAST_ACK);
		}
	}

	if (flags & NET_TCP_FIN) {
		tcp->flags |= NET_TCP_FINAL_SENT;
		/* RFC793 says about ACK bit: "Once a connection is
		 * established this is always sent." as teardown
		 * happens when connection is established, it must
		 * have ACK set.
		 */
		flags |= NET_TCP_ACK;
		seq++;

		if (net_tcp_get_state(tcp) == NET_TCP_ESTABLISHED ||
		    net_tcp_get_state(tcp) == NET_TCP_SYN_RCVD) {
			net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_1);
		}
	}

	wnd = net_tcp_get_recv_wnd(tcp);

	segment.src_addr = (struct sockaddr_ptr *)local;
	segment.dst_addr = remote;
	segment.seq = tcp->send_seq;
	segment.ack = tcp->send_ack;
	segment.flags = flags;
	segment.wnd = wnd;
	segment.options = options;
	segment.optlen = optlen;

	*send_pkt = prepare_segment(tcp, &segment, *send_pkt);
	if (!*send_pkt) {
		return -EINVAL;
	}

	tcp->send_seq = seq;

	if (net_tcp_seq_greater(tcp->send_seq, tcp->recv_max_ack)) {
		tcp->recv_max_ack = tcp->send_seq;
	}

	return 0;
}

static inline u32_t get_size(u32_t pos1, u32_t pos2)
{
	u32_t size;

	if (pos1 <= pos2) {
		size = pos2 - pos1;
	} else {
		size = NET_TCP_MAX_SEQ - pos1 + pos2 + 1;
	}

	return size;
}

#if defined(CONFIG_NET_IPV4)
#ifndef NET_IP_MAX_PACKET
#define NET_IP_MAX_PACKET (10 * 1024)
#endif

#define NET_IP_MAX_OPTIONS 40 /* Maximum option field length */

static inline size_t ip_max_packet_len(struct in_addr *dest_ip)
{
	ARG_UNUSED(dest_ip);

	return (NET_IP_MAX_PACKET - (NET_IP_MAX_OPTIONS +
		      sizeof(struct net_ipv4_hdr))) & (~0x3LU);
}
#else /* CONFIG_NET_IPV4 */
#define ip_max_packet_len(...) 0
#endif /* CONFIG_NET_IPV4 */

u16_t net_tcp_get_recv_mss(const struct net_tcp *tcp)
{
	sa_family_t family = net_context_get_family(tcp->context);

	if (family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_context_get_iface(tcp->context);

		if (iface && iface->mtu >= NET_IPV4TCPH_LEN) {
			/* Detect MSS based on interface MTU minus "TCP,IP
			 * header size"
			 */
			return iface->mtu - NET_IPV4TCPH_LEN;
		}
#else
		return 0;
#endif /* CONFIG_NET_IPV4 */
	}
#if defined(CONFIG_NET_IPV6)
	else if (family == AF_INET6) {
		struct net_if *iface = net_context_get_iface(tcp->context);
		int mss = 0;

		if (iface && iface->mtu >= NET_IPV6TCPH_LEN) {
			/* Detect MSS based on interface MTU minus "TCP,IP
			 * header size"
			 */
			mss = iface->mtu - NET_IPV6TCPH_LEN;
		}

		if (mss < NET_IPV6_MTU) {
			mss = NET_IPV6_MTU;
		}

		return mss;
	}
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

static void net_tcp_set_syn_opt(struct net_tcp *tcp, u8_t *options,
				u8_t *optionlen)
{
	u32_t recv_mss;

	*optionlen = 0;

	if (!(tcp->flags & NET_TCP_RECV_MSS_SET)) {
		recv_mss = net_tcp_get_recv_mss(tcp);
		tcp->flags |= NET_TCP_RECV_MSS_SET;
	} else {
		recv_mss = 0;
	}

	recv_mss |= (NET_TCP_MSS_OPT << 24) | (NET_TCP_MSS_SIZE << 16);
	UNALIGNED_PUT(htonl(recv_mss),
		      (u32_t *)(options + *optionlen));

	*optionlen += NET_TCP_MSS_SIZE;
}

int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_pkt **pkt)
{
	u8_t options[NET_TCP_MAX_OPT_SIZE];
	u8_t optionlen;

	switch (net_tcp_get_state(tcp)) {
	case NET_TCP_SYN_RCVD:
		/* In the SYN_RCVD state acknowledgment must be with the
		 * SYN flag.
		 */
		net_tcp_set_syn_opt(tcp, options, &optionlen);

		return net_tcp_prepare_segment(tcp, NET_TCP_SYN | NET_TCP_ACK,
					       options, optionlen, NULL, remote,
					       pkt);
	case NET_TCP_FIN_WAIT_1:
	case NET_TCP_LAST_ACK:
		/* In the FIN_WAIT_1 and LAST_ACK states acknowledgment must
		 * be with the FIN flag.
		 */
		return net_tcp_prepare_segment(tcp, NET_TCP_FIN | NET_TCP_ACK,
					       0, 0, NULL, remote, pkt);
	default:
		return net_tcp_prepare_segment(tcp, NET_TCP_ACK, 0, 0, NULL,
					       remote, pkt);
	}

	return -EINVAL;
}

int net_tcp_prepare_reset(struct net_tcp *tcp,
			  const struct sockaddr *remote,
			  struct net_pkt **pkt)
{
	struct tcp_segment segment = { 0 };

	if ((net_context_get_state(tcp->context) != NET_CONTEXT_UNCONNECTED) &&
	    (net_tcp_get_state(tcp) != NET_TCP_SYN_SENT) &&
	    (net_tcp_get_state(tcp) != NET_TCP_TIME_WAIT)) {
		/* Send the reset segment always with acknowledgment. */
		segment.ack = tcp->send_ack;
		segment.flags = NET_TCP_RST | NET_TCP_ACK;
		segment.seq = tcp->send_seq;
		segment.src_addr = &tcp->context->local;
		segment.dst_addr = remote;
		segment.wnd = 0;
		segment.options = NULL;
		segment.optlen = 0;

		*pkt = prepare_segment(tcp, &segment, NULL);
	}

	return 0;
}

const char *net_tcp_state_str(enum net_tcp_state state)
{
#if defined(CONFIG_NET_DEBUG_TCP)
	switch (state) {
	case NET_TCP_CLOSED:
		return "CLOSED";
	case NET_TCP_LISTEN:
		return "LISTEN";
	case NET_TCP_SYN_SENT:
		return "SYN_SENT";
	case NET_TCP_SYN_RCVD:
		return "SYN_RCVD";
	case NET_TCP_ESTABLISHED:
		return "ESTABLISHED";
	case NET_TCP_CLOSE_WAIT:
		return "CLOSE_WAIT";
	case NET_TCP_LAST_ACK:
		return "LAST_ACK";
	case NET_TCP_FIN_WAIT_1:
		return "FIN_WAIT_1";
	case NET_TCP_FIN_WAIT_2:
		return "FIN_WAIT_2";
	case NET_TCP_TIME_WAIT:
		return "TIME_WAIT";
	case NET_TCP_CLOSING:
		return "CLOSING";
	}
#else /* CONFIG_NET_DEBUG_TCP */
	ARG_UNUSED(state);
#endif /* CONFIG_NET_DEBUG_TCP */

	return "";
}

int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt)
{
	struct net_conn *conn = (struct net_conn *)context->conn_handler;
	size_t data_len = net_pkt_get_len(pkt);
	int ret;

	NET_DBG("[%p] Queue %p len %zd", context->tcp, pkt, data_len);

	/* Set PSH on all packets, our window is so small that there's
	 * no point in the remote side trying to finesse things and
	 * coalesce packets.
	 */
	ret = net_tcp_prepare_segment(context->tcp, NET_TCP_PSH | NET_TCP_ACK,
				      NULL, 0, NULL, &conn->remote_addr, &pkt);
	if (ret) {
		return ret;
	}

	context->tcp->send_seq += data_len;

	net_stats_update_tcp_sent(data_len);

	sys_slist_append(&context->tcp->sent_list, &pkt->sent_list);

	/* We need to restart retry_timer if it is stopped. */
	if (k_delayed_work_remaining_get(&context->tcp->retry_timer) == 0) {
		k_delayed_work_submit(&context->tcp->retry_timer,
				      retry_timeout(context->tcp));
	}

	do_ref_if_needed(context->tcp, pkt);

	return 0;
}

int net_tcp_send_pkt(struct net_pkt *pkt)
{
	struct net_context *ctx = net_pkt_context(pkt);
	struct net_tcp_hdr hdr, *tcp_hdr;
	bool calc_chksum = false;

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		NET_ERR("Packet %p does not contain TCP header", pkt);
		return -EMSGSIZE;
	}

	if (sys_get_be32(tcp_hdr->ack) != ctx->tcp->send_ack) {
		sys_put_be32(ctx->tcp->send_ack, tcp_hdr->ack);
		calc_chksum = true;
	}

	/* The data stream code always sets this flag, because
	 * existing stacks (Linux, anyway) seem to ignore data packets
	 * without a valid-but-already-transmitted ACK.  But set it
	 * anyway if we know we need it just to sanify edge cases.
	 */
	if (ctx->tcp->sent_ack != ctx->tcp->send_ack &&
		(tcp_hdr->flags & NET_TCP_ACK) == 0) {
		tcp_hdr->flags |= NET_TCP_ACK;
		calc_chksum = true;
	}

	if (calc_chksum) {
		net_tcp_set_chksum(pkt, pkt->frags);
	}

	if (tcp_hdr->flags & NET_TCP_FIN) {
		ctx->tcp->fin_sent = 1;
	}

	ctx->tcp->sent_ack = ctx->tcp->send_ack;

	/* As we modified the header, we need to write it back.
	 */
	net_tcp_set_hdr(pkt, tcp_hdr);

	/* We must have special handling for some network technologies that
	 * tweak the IP protocol headers during packet sending. This happens
	 * with Bluetooth and IEEE 802.15.4 which use IPv6 header compression
	 * (6lo) and alter the sent network packet. So in order to avoid any
	 * corruption of the original data buffer, we must copy the sent data.
	 * For Bluetooth, its fragmentation code will even mangle the data
	 * part of the message so we need to copy those too.
	 */
	if (is_6lo_technology(pkt)) {
		struct net_pkt *new_pkt, *check_pkt;
		int ret;
		bool pkt_in_slist = false;

		/*
		 * There are users of this function that don't add pkt to TCP
		 * sent_list. (See send_ack() in net_context.c) In these cases,
		 * we should avoid the extra 6lowpan specific buffer copy
		 * below.
		 */
		SYS_SLIST_FOR_EACH_CONTAINER(&ctx->tcp->sent_list,
					     check_pkt, sent_list) {
			if (check_pkt == pkt) {
				pkt_in_slist = true;
				break;
			}
		}

		if (pkt_in_slist) {
			new_pkt = net_pkt_clone(pkt, ALLOC_TIMEOUT);
			if (!new_pkt) {
				return -ENOMEM;
			}

			/* This function is called from net_context.c and if we
			 * return < 0, the caller will unref the original pkt.
			 * This would leak the new_pkt so remove it here.
			 */
			ret = net_send_data(new_pkt);
			if (ret < 0) {
				net_pkt_unref(new_pkt);
			} else {
				net_stats_update_tcp_seg_rexmit();
			}

			return ret;
		}
	}

	return net_send_data(pkt);
}

static void restart_timer(struct net_tcp *tcp)
{
	if (!sys_slist_is_empty(&tcp->sent_list)) {
		tcp->flags |= NET_TCP_RETRYING;
		tcp->retry_timeout_shift = 0;
		k_delayed_work_submit(&tcp->retry_timer, retry_timeout(tcp));
	} else if (IS_ENABLED(CONFIG_NET_TCP_TIME_WAIT)) {
		if (tcp->fin_sent && tcp->fin_rcvd) {
			/* We know sent_list is empty, which means if
			 * fin_sent is true it must have been ACKd
			 */
			k_delayed_work_submit(&tcp->retry_timer, TIME_WAIT_MS);
			net_context_ref(tcp->context);
		}
	} else {
		k_delayed_work_cancel(&tcp->retry_timer);
		tcp->flags &= ~NET_TCP_RETRYING;
	}
}

int net_tcp_send_data(struct net_context *context)
{
	struct net_pkt *pkt;

	/* For now, just send all queued data synchronously.  Need to
	 * add window handling and retry/ACK logic.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&context->tcp->sent_list, pkt, sent_list) {
		/* Do not resend packets that were sent by expire timer */
		if (net_pkt_queued(pkt)) {
			NET_DBG("[%p] Skipping pkt %p because it was already "
				"sent.", context->tcp, pkt);
			continue;
		}

		if (!net_pkt_sent(pkt)) {
			int ret;

			NET_DBG("[%p] Sending pkt %p (%zd bytes)", context->tcp,
				pkt, net_pkt_get_len(pkt));

			ret = net_tcp_send_pkt(pkt);
			if (ret < 0 && !is_6lo_technology(pkt)) {
				NET_DBG("[%p] pkt %p not sent (%d)",
					context->tcp, pkt, ret);
				net_pkt_unref(pkt);
			}

			net_pkt_set_queued(pkt, true);
		}
	}

	return 0;
}

void net_tcp_ack_received(struct net_context *ctx, u32_t ack)
{
	struct net_tcp *tcp = ctx->tcp;
	sys_slist_t *list = &ctx->tcp->sent_list;
	sys_snode_t *head;
	struct net_pkt *pkt;
	u32_t seq;
	bool valid_ack = false;

	if (IS_ENABLED(CONFIG_NET_STATISTICS_TCP) &&
	    sys_slist_is_empty(list)) {
		net_stats_update_tcp_seg_ackerr();
	}

	while (!sys_slist_is_empty(list)) {
		struct net_tcp_hdr hdr, *tcp_hdr;

		head = sys_slist_peek_head(list);
		pkt = CONTAINER_OF(head, struct net_pkt, sent_list);

		tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
		if (!tcp_hdr) {
			/* The pkt does not contain TCP header, this should
			 * not happen.
			 */
			NET_ERR("pkt %p has no TCP header", pkt);
			sys_slist_remove(list, NULL, head);
			net_pkt_unref(pkt);
			continue;
		}

		seq = sys_get_be32(tcp_hdr->seq) + net_pkt_appdatalen(pkt) - 1;

		if (!net_tcp_seq_greater(ack, seq)) {
			net_stats_update_tcp_seg_ackerr();
			break;
		}

		if (tcp_hdr->flags & NET_TCP_FIN) {
			enum net_tcp_state s = net_tcp_get_state(tcp);

			if (s == NET_TCP_FIN_WAIT_1) {
				net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_2);
			} else if (s == NET_TCP_CLOSING) {
				net_tcp_change_state(tcp, NET_TCP_TIME_WAIT);
			}
		}

		sys_slist_remove(list, NULL, head);
		net_pkt_unref(pkt);
		valid_ack = true;
	}

	/* No need to re-send stuff we are closing down */
	if (valid_ack && net_tcp_get_state(tcp) == NET_TCP_ESTABLISHED) {
		/* Restart the timer on a valid inbound ACK.  This
		 * isn't quite the same behavior as per-packet retry
		 * timers, but is close in practice (it starts retries
		 * one timer period after the connection "got stuck")
		 * and avoids the need to track per-packet timers or
		 * sent times.
		 */
		restart_timer(ctx->tcp);

		/* And, if we had been retrying, mark all packets
		 * untransmitted and then resend them.  The stalled
		 * pipe is uncorked again.
		 */
		if (ctx->tcp->flags & NET_TCP_RETRYING) {
			SYS_SLIST_FOR_EACH_CONTAINER(&ctx->tcp->sent_list, pkt,
						     sent_list) {
				if (net_pkt_sent(pkt)) {
					do_ref_if_needed(ctx->tcp, pkt);
					net_pkt_set_sent(pkt, false);
				}
			}

			net_tcp_send_data(ctx);
		}
	}
}

void net_tcp_init(void)
{
}

#if defined(CONFIG_NET_DEBUG_TCP)
static void validate_state_transition(enum net_tcp_state current,
				      enum net_tcp_state new)
{
	static const u16_t valid_transitions[] = {
		[NET_TCP_CLOSED] = 1 << NET_TCP_LISTEN |
			1 << NET_TCP_SYN_SENT,
		[NET_TCP_LISTEN] = 1 << NET_TCP_SYN_RCVD |
			1 << NET_TCP_SYN_SENT,
		[NET_TCP_SYN_RCVD] = 1 << NET_TCP_FIN_WAIT_1 |
			1 << NET_TCP_ESTABLISHED |
			1 << NET_TCP_LISTEN |
			1 << NET_TCP_CLOSED,
		[NET_TCP_SYN_SENT] = 1 << NET_TCP_CLOSED |
			1 << NET_TCP_ESTABLISHED |
			1 << NET_TCP_SYN_RCVD |
			1 << NET_TCP_CLOSED,
		[NET_TCP_ESTABLISHED] = 1 << NET_TCP_CLOSE_WAIT |
			1 << NET_TCP_FIN_WAIT_1 |
			1 << NET_TCP_CLOSED,
		[NET_TCP_CLOSE_WAIT] = 1 << NET_TCP_LAST_ACK |
			1 << NET_TCP_CLOSED,
		[NET_TCP_LAST_ACK] = 1 << NET_TCP_CLOSED,
		[NET_TCP_FIN_WAIT_1] = 1 << NET_TCP_CLOSING |
			1 << NET_TCP_FIN_WAIT_2 |
			1 << NET_TCP_TIME_WAIT |
			1 << NET_TCP_CLOSED,
		[NET_TCP_FIN_WAIT_2] = 1 << NET_TCP_TIME_WAIT |
			1 << NET_TCP_CLOSED,
		[NET_TCP_CLOSING] = 1 << NET_TCP_TIME_WAIT |
			1 << NET_TCP_CLOSED,
		[NET_TCP_TIME_WAIT] = 1 << NET_TCP_CLOSED
	};

	if (!(valid_transitions[current] & 1 << new)) {
		NET_DBG("Invalid state transition: %s (%d) => %s (%d)",
			net_tcp_state_str(current), current,
			net_tcp_state_str(new), new);
	}
}
#endif /* CONFIG_NET_DEBUG_TCP */

void net_tcp_change_state(struct net_tcp *tcp,
			  enum net_tcp_state new_state)
{
	NET_ASSERT(tcp);

	if (net_tcp_get_state(tcp) == new_state) {
		return;
	}

	NET_ASSERT(new_state >= NET_TCP_CLOSED &&
		   new_state <= NET_TCP_CLOSING);

	NET_DBG("[%p] state %s (%d) => %s (%d)",
		tcp, net_tcp_state_str(tcp->state), tcp->state,
		net_tcp_state_str(new_state), new_state);

#if defined(CONFIG_NET_DEBUG_TCP)
	validate_state_transition(tcp->state, new_state);
#endif /* CONFIG_NET_DEBUG_TCP */

	tcp->state = new_state;

	if (net_tcp_get_state(tcp) != NET_TCP_CLOSED) {
		return;
	}

	if (!tcp->context) {
		return;
	}

	/* Remove any port handlers if we are closing */
	if (tcp->context->conn_handler) {
		net_tcp_unregister(tcp->context->conn_handler);
		tcp->context->conn_handler = NULL;
	}

	if (tcp->accept_cb) {
		tcp->accept_cb(tcp->context,
			       &tcp->context->remote,
			       sizeof(struct sockaddr),
			       -ENETRESET,
			       tcp->context->user_data);
	}
}

void net_tcp_foreach(net_tcp_cb_t cb, void *user_data)
{
	int i, key;

	key = irq_lock();

	for (i = 0; i < NET_MAX_TCP_CONTEXT; i++) {
		if (!net_tcp_is_used(&tcp_context[i])) {
			continue;
		}

		irq_unlock(key);

		cb(&tcp_context[i], user_data);

		key = irq_lock();
	}

	irq_unlock(key);
}

bool net_tcp_validate_seq(struct net_tcp *tcp, struct net_pkt *pkt)
{
	struct net_tcp_hdr hdr, *tcp_hdr;

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	return (net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
				tcp->send_ack) >= 0) &&
		(net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
				 tcp->send_ack
					+ net_tcp_get_recv_wnd(tcp)) < 0);
}

struct net_tcp_hdr *net_tcp_get_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr)
{
	struct net_tcp_hdr *tcp_hdr;
	struct net_buf *frag;
	u16_t pos;

	tcp_hdr = net_pkt_tcp_data(pkt);
	if (net_tcp_header_fits(pkt, tcp_hdr)) {
		return tcp_hdr;
	}

	frag = net_frag_read(pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt),
			     &pos, sizeof(hdr->src_port),
			     (u8_t *)&hdr->src_port);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->dst_port),
			     (u8_t *)&hdr->dst_port);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->seq), hdr->seq);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->ack), hdr->ack);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->offset);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->flags);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->wnd), hdr->wnd);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->urg), hdr->urg);

	if (!frag && pos == 0xffff) {
		/* If the pkt is compressed, then this is the typical outcome
		 * so no use printing error in this case.
		 */
		if (IS_ENABLED(CONFIG_NET_DEBUG_TCP) &&
		    !is_6lo_technology(pkt)) {
			NET_ASSERT(frag);
		}

		return NULL;
	}

	return hdr;
}

struct net_tcp_hdr *net_tcp_set_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr)
{
	struct net_buf *frag;
	u16_t pos;

	if (net_tcp_header_fits(pkt, hdr)) {
		return hdr;
	}

	frag = net_pkt_write(pkt, pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt),
			     &pos, sizeof(hdr->src_port),
			     (u8_t *)&hdr->src_port, ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->dst_port),
			     (u8_t *)&hdr->dst_port, ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->seq), hdr->seq,
			     ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->ack), hdr->ack,
			     ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->offset),
			     &hdr->offset, ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->flags),
			     &hdr->flags, ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->wnd), hdr->wnd,
			     ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum, ALLOC_TIMEOUT);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->urg), hdr->urg,
			     ALLOC_TIMEOUT);

	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

u16_t net_tcp_get_chksum(struct net_pkt *pkt, struct net_buf *frag)
{
	struct net_tcp_hdr *hdr;
	u16_t chksum;
	u16_t pos;

	hdr = net_pkt_tcp_data(pkt);
	if (net_tcp_header_fits(pkt, hdr)) {
		return hdr->chksum;
	}

	frag = net_frag_read(frag,
			     net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     2 + 2 + 4 + 4 + /* src + dst + seq + ack */
			     1 + 1 + 2 /* offset + flags + wnd */,
			     &pos, sizeof(chksum), (u8_t *)&chksum);
	NET_ASSERT(frag);

	return chksum;
}

struct net_buf *net_tcp_set_chksum(struct net_pkt *pkt, struct net_buf *frag)
{
	struct net_tcp_hdr *hdr;
	u16_t chksum = 0;
	u16_t pos;

	hdr = net_pkt_tcp_data(pkt);
	if (net_tcp_header_fits(pkt, hdr)) {
		hdr->chksum = 0;
		hdr->chksum = ~net_calc_chksum_tcp(pkt);

		return frag;
	}

	/* We need to set the checksum to 0 first before the calc */
	frag = net_pkt_write(pkt, frag,
			     net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     2 + 2 + 4 + 4 + /* src + dst + seq + ack */
			     1 + 1 + 2 /* offset + flags + wnd */,
			     &pos, sizeof(chksum), (u8_t *)&chksum,
			     ALLOC_TIMEOUT);

	chksum = ~net_calc_chksum_tcp(pkt);

	frag = net_pkt_write(pkt, frag, pos - 2, &pos, sizeof(chksum),
			     (u8_t *)&chksum, ALLOC_TIMEOUT);

	NET_ASSERT(frag);

	return frag;
}

int net_tcp_parse_opts(struct net_pkt *pkt, int opt_totlen,
		       struct net_tcp_options *opts)
{
	struct net_buf *frag = pkt->frags;
	u16_t pos = net_pkt_ip_hdr_len(pkt)
		  + net_pkt_ipv6_ext_len(pkt)
		  + sizeof(struct net_tcp_hdr);
	u8_t opt, optlen;

	/* TODO: this should be done for each TCP pkt, on reception */
	if (pos + opt_totlen > net_pkt_get_len(pkt)) {
		NET_ERR("Truncated pkt len: %d, expected: %d",
			(int)net_pkt_get_len(pkt), pos + opt_totlen);
		return -EINVAL;
	}

	while (opt_totlen) {
		frag = net_frag_read(frag, pos, &pos, sizeof(opt), &opt);
		opt_totlen--;

		/* https://www.iana.org/assignments/tcp-parameters/tcp-parameters.xhtml#tcp-parameters-1 */
		/* "Options 0 and 1 are exactly one octet which is their
		 * kind field.  All other options have their one octet
		 * kind field, followed by a one octet length field,
		 * followed by length-2 octets of option data."
		 */
		if (opt == NET_TCP_END_OPT) {
			break;
		} else if (opt == NET_TCP_NOP_OPT) {
			continue;
		}

		if (!opt_totlen) {
			optlen = 0;
			goto error;
		}

		frag = net_frag_read(frag, pos, &pos, sizeof(optlen), &optlen);
		opt_totlen--;
		if (optlen < 2) {
			goto error;
		}

		/* Subtract opt/optlen size now to avoid doing this
		 * repeatedly.
		 */
		optlen -= 2;
		if (opt_totlen < optlen) {
			goto error;
		}

		switch (opt) {
		case NET_TCP_MSS_OPT:
			if (optlen != 2) {
				goto error;
			}
			frag = net_frag_read_be16(frag, pos, &pos,
						  &opts->mss);
			break;
		default:
			frag = net_frag_skip(frag, pos, &pos, optlen);
			break;
		}

		opt_totlen -= optlen;
	}

	return 0;

error:
	NET_ERR("Invalid TCP opt: %d len: %d", opt, optlen);
	return -EINVAL;
}
