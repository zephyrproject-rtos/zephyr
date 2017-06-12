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

#define INIT_RETRY_MS 200

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
	u8_t flags = NET_TCP_FLAGS(pkt);
	u32_t rel_ack, ack;

	ack = sys_get_be32(NET_TCP_HDR(pkt)->ack);

	if (!tcp->sent_ack) {
		rel_ack = 0;
	} else {
		rel_ack = ack ? ack - tcp->sent_ack : 0;
	}

	NET_DBG("pkt %p src %u dst %u seq 0x%04x (%u) ack 0x%04x (%u/%u) "
		"flags %c%c%c%c%c%c win %u chk 0x%04x",
		pkt,
		ntohs(NET_TCP_HDR(pkt)->src_port),
		ntohs(NET_TCP_HDR(pkt)->dst_port),
		sys_get_be32(NET_TCP_HDR(pkt)->seq),
		sys_get_be32(NET_TCP_HDR(pkt)->seq),
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
		sys_get_be16(NET_TCP_HDR(pkt)->wnd),
		ntohs(NET_TCP_HDR(pkt)->chksum));
}
#else
#define net_tcp_trace(...)
#endif /* CONFIG_NET_DEBUG_TCP */

static inline u32_t init_isn(void)
{
	/* Randomise initial seq number */
	return sys_rand32_get();
}

static inline u32_t retry_timeout(const struct net_tcp *tcp)
{
	return ((u32_t)1 << tcp->retry_timeout_shift) * INIT_RETRY_MS;
}

#define is_6lo_technology(pkt)						    \
	(IS_ENABLED(CONFIG_NET_IPV6) &&	net_pkt_family(pkt) == AF_INET6 &&  \
	 ((IS_ENABLED(CONFIG_NET_L2_BLUETOOTH) &&			    \
	   net_pkt_ll_dst(pkt)->type == NET_LINK_BLUETOOTH) ||		    \
	  (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&			    \
	   net_pkt_ll_dst(pkt)->type == NET_LINK_IEEE802154)))

static inline void do_ref_if_needed(struct net_pkt *pkt)
{
	/* The ref should not be done for Bluetooth and IEEE 802.15.4 which use
	 * IPv6 header compression (6lo). For BT and 802.15.4 we copy the pkt
	 * chain we are about to send so it is fine if the network driver
	 * releases it. As we have our own copy of the sent data, we do not
	 * need to take a reference of it. See also net_tcp_send_pkt().
	 */
	if (!is_6lo_technology(pkt)) {
		pkt = net_pkt_ref(pkt);
	}
}

static void abort_connection(struct net_tcp *tcp)
{
	struct net_context *ctx = tcp->context;

	NET_DBG("Segment retransmission exceeds %d, resetting context %p",
		CONFIG_NET_TCP_RETRY_COUNT, ctx);

	if (ctx->recv_cb) {
		ctx->recv_cb(ctx, NULL, -ECONNRESET, tcp->recv_user_data);
	}

	net_context_unref(ctx);
}

static void tcp_retry_expired(struct k_timer *timer)
{
	struct net_tcp *tcp = CONTAINER_OF(timer, struct net_tcp, retry_timer);
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

		k_timer_start(&tcp->retry_timer, retry_timeout(tcp), 0);

		pkt = CONTAINER_OF(sys_slist_peek_head(&tcp->sent_list),
				   struct net_pkt, sent_list);

		do_ref_if_needed(pkt);
		if (net_tcp_send_pkt(pkt) < 0 && !is_6lo_technology(pkt)) {
			net_pkt_unref(pkt);
		} else {
			if (IS_ENABLED(CONFIG_NET_STATISTICS_TCP) &&
			    !is_6lo_technology(pkt)) {
				net_stats_update_tcp_seg_rexmit();
			}
		}
	} else if (IS_ENABLED(CONFIG_NET_TCP_TIME_WAIT)) {
		if (tcp->fin_sent && tcp->fin_rcvd) {
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

	tcp_context[i].send_seq = init_isn();
	tcp_context[i].recv_max_ack = tcp_context[i].send_seq + 1u;

	tcp_context[i].accept_cb = NULL;

	k_timer_init(&tcp_context[i].retry_timer, tcp_retry_expired, NULL);
	k_sem_init(&tcp_context[i].connect_wait, 0, UINT_MAX);

	return &tcp_context[i];
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

	tcp->ack_timer_cancelled = true;
	k_delayed_work_cancel(&tcp->ack_timer);
	k_timer_stop(&tcp->retry_timer);
	k_sem_reset(&tcp->connect_wait);

	net_tcp_change_state(tcp, NET_TCP_CLOSED);
	tcp->context = NULL;

	key = irq_lock();
	tcp->flags &= ~(NET_TCP_IN_USE | NET_TCP_RECV_MSS_SET);
	irq_unlock(key);

	NET_DBG("Disposed of TCP connection state");

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
	struct net_tcp_hdr *tcphdr;
	struct net_context *context = tcp->context;
	u16_t dst_port, src_port;
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
	} else {
		pkt = net_pkt_get_tx(context, ALLOC_TIMEOUT);
		if (!pkt) {
			return NULL;
		}
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
		NET_DBG("Protocol family %d not supported",
			net_pkt_family(pkt));
		net_pkt_unref(pkt);
		return NULL;
	}

	header = net_pkt_get_data(context, ALLOC_TIMEOUT);
	if (!header) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_pkt_frag_add(pkt, header);

	tcphdr = (struct net_tcp_hdr *)net_buf_add(header, NET_TCPH_LEN);

	if (segment->options && segment->optlen) {
		optlen = net_tcp_add_options(header, segment->optlen,
					segment->options);
	}

	tcphdr->offset = (NET_TCPH_LEN + optlen) << 2;

	tcphdr->src_port = src_port;
	tcphdr->dst_port = dst_port;
	sys_put_be32(segment->seq, tcphdr->seq);
	sys_put_be32(segment->ack, tcphdr->ack);
	tcphdr->flags = segment->flags;
	sys_put_be16(segment->wnd, tcphdr->wnd);
	tcphdr->urg[0] = 0;
	tcphdr->urg[1] = 0;

	if (tail) {
		net_pkt_frag_add(pkt, tail);
	}

	if (finalize_segment(context, pkt) < 0) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_tcp_trace(pkt, tcp);

	return pkt;
}

static inline u32_t get_recv_wnd(struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);

	/* We don't queue received data inside the stack, we hand off
	 * packets to synchronous callbacks (who can queue if they
	 * want, but it's not our business).  So the available window
	 * size is always the same.  There are two configurables to
	 * check though.
	 */
	return min(NET_TCP_MAX_WIN, NET_TCP_BUF_MAX_LEN);
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
		/* FIXME: We apparently miss increment in another
		 * transition of the state machine, so have to
		 * adjust seq no by 2 here. This is required for
		 * Linux to detect active close on server side, and
		 * to make Wireshark happy about sequence numbers.
		 */
		seq += 2;

		if (net_tcp_get_state(tcp) == NET_TCP_ESTABLISHED ||
		    net_tcp_get_state(tcp) == NET_TCP_SYN_RCVD) {
			net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_1);
		}
	}

	if (flags & NET_TCP_SYN) {
		seq++;
	}

	wnd = get_recv_wnd(tcp);

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
		return 1280;
	}
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

static void net_tcp_set_syn_opt(struct net_tcp *tcp, u8_t *options,
				u8_t *optionlen)
{
	u16_t recv_mss;

	*optionlen = 0;

	if (!(tcp->flags & NET_TCP_RECV_MSS_SET)) {
		recv_mss = net_tcp_get_recv_mss(tcp);
		tcp->flags |= NET_TCP_RECV_MSS_SET;
	} else {
		recv_mss = 0;
	}

	UNALIGNED_PUT(htonl((u32_t)recv_mss | NET_TCP_MSS_HEADER),
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
		tcp->send_seq--;

		net_tcp_set_syn_opt(tcp, options, &optionlen);

		return net_tcp_prepare_segment(tcp, NET_TCP_SYN | NET_TCP_ACK,
					       options, optionlen, NULL, remote,
					       pkt);
	case NET_TCP_FIN_WAIT_1:
	case NET_TCP_LAST_ACK:
		/* In the FIN_WAIT_1 and LAST_ACK states acknowledgment must
		 * be with the FIN flag.
		 */
		tcp->send_seq--;

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

const char * const net_tcp_state_str(enum net_tcp_state state)
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
	if (k_timer_remaining_get(&context->tcp->retry_timer) == 0) {
		k_timer_start(&context->tcp->retry_timer,
			      retry_timeout(context->tcp), 0);
	}

	do_ref_if_needed(pkt);

	return 0;
}

int net_tcp_send_pkt(struct net_pkt *pkt)
{
	struct net_context *ctx = net_pkt_context(pkt);
	struct net_tcp_hdr *tcphdr = NET_TCP_HDR(pkt);

	sys_put_be32(ctx->tcp->send_ack, tcphdr->ack);

	/* The data stream code always sets this flag, because
	 * existing stacks (Linux, anyway) seem to ignore data packets
	 * without a valid-but-already-transmitted ACK.  But set it
	 * anyway if we know we need it just to sanify edge cases.
	 */
	if (ctx->tcp->sent_ack != ctx->tcp->send_ack) {
		tcphdr->flags |= NET_TCP_ACK;
	}

	if (tcphdr->flags & NET_TCP_FIN) {
		ctx->tcp->fin_sent = 1;
	}

	ctx->tcp->sent_ack = ctx->tcp->send_ack;

	net_pkt_set_sent(pkt, true);

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
			new_pkt = net_pkt_get_tx(ctx, ALLOC_TIMEOUT);
			if (!new_pkt) {
				return -ENOMEM;
			}

			memcpy(new_pkt, pkt, sizeof(struct net_pkt));
			new_pkt->frags = net_pkt_copy_all(pkt, 0,
							  ALLOC_TIMEOUT);
			if (!new_pkt->frags) {
				net_pkt_unref(new_pkt);
				return -ENOMEM;
			}

			NET_DBG("Copied %zu bytes from %p to %p",
				net_pkt_get_len(new_pkt), pkt, new_pkt);

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
		k_timer_start(&tcp->retry_timer, retry_timeout(tcp), 0);
	} else if (IS_ENABLED(CONFIG_NET_TCP_TIME_WAIT)) {
		if (tcp->fin_sent && tcp->fin_rcvd) {
			/* We know sent_list is empty, which means if
			 * fin_sent is true it must have been ACKd
			 */
			k_timer_start(&tcp->retry_timer, TIME_WAIT_MS, 0);
			net_context_ref(tcp->context);
		}
	} else {
		k_timer_stop(&tcp->retry_timer);
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
		if (!net_pkt_sent(pkt)) {
			if (net_tcp_send_pkt(pkt) < 0 &&
			    !is_6lo_technology(pkt)) {
				net_pkt_unref(pkt);
			}
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
	struct net_tcp_hdr *tcphdr;
	u32_t seq;
	bool valid_ack = false;

	if (IS_ENABLED(CONFIG_NET_STATISTICS_TCP) &&
	    sys_slist_is_empty(list)) {
		net_stats_update_tcp_seg_ackerr();
	}

	while (!sys_slist_is_empty(list)) {
		head = sys_slist_peek_head(list);
		pkt = CONTAINER_OF(head, struct net_pkt, sent_list);
		tcphdr = NET_TCP_HDR(pkt);

		seq = sys_get_be32(tcphdr->seq) + net_pkt_appdatalen(pkt) - 1;

		if (!net_tcp_seq_greater(ack, seq)) {
			net_stats_update_tcp_seg_ackerr();
			break;
		}

		if (tcphdr->flags & NET_TCP_FIN) {
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

	if (valid_ack) {
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
			struct net_pkt *pkt;

			SYS_SLIST_FOR_EACH_CONTAINER(&ctx->tcp->sent_list, pkt,
						     sent_list) {
				if (net_pkt_sent(pkt)) {
					do_ref_if_needed(pkt);
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

	NET_DBG("state@%p %s (%d) => %s (%d)",
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
	return !net_tcp_seq_greater(tcp->send_ack + get_recv_wnd(tcp),
				    sys_get_be32(NET_TCP_HDR(pkt)->seq));
}
