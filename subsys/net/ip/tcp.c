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

#include <logging/log.h>
LOG_MODULE_REGISTER(net_tcp, CONFIG_NET_TCP_LOG_LEVEL);

#include <kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/net_context.h>
#include <sys/byteorder.h>

#include "connection.h"
#include "net_private.h"

#include "ipv6.h"
#include "ipv4.h"
#include "tcp_internal.h"
#include "net_stats.h"

#define ALLOC_TIMEOUT K_MSEC(500)

static int net_tcp_queue_pkt(struct net_context *context, struct net_pkt *pkt);

/*
 * Each TCP connection needs to be tracked by net_context, so
 * we need to allocate equal number of control structures here.
 */
#define NET_MAX_TCP_CONTEXT CONFIG_NET_MAX_CONTEXTS
static struct net_tcp tcp_context[NET_MAX_TCP_CONTEXT];

static struct tcp_backlog_entry {
	struct net_tcp *tcp;
	uint32_t send_seq;
	uint32_t send_ack;
	struct k_delayed_work ack_timer;
	struct sockaddr remote;
	uint16_t send_mss;
} tcp_backlog[CONFIG_NET_TCP_BACKLOG_SIZE];

#if defined(CONFIG_NET_TCP_ACK_TIMEOUT)
#define ACK_TIMEOUT_MS CONFIG_NET_TCP_ACK_TIMEOUT
#define ACK_TIMEOUT K_MSEC(ACK_TIMEOUT_MS)
#else
#define ACK_TIMEOUT_MS (1 * MSEC_PER_SEC)
#define ACK_TIMEOUT K_MSEC(MSEC_PER_SEC)
#endif

#define FIN_TIMEOUT_MS (1 * MSEC_PER_SEC)
#define FIN_TIMEOUT K_MSEC(MSEC_PER_SEC)

/* Declares a wrapper function for a net_conn callback that refs the
 * context around the invocation (to protect it from premature
 * deletion).  Long term would be nice to see this feature be part of
 * the connection type itself, but right now it has opaque "user_data"
 * pointers and doesn't understand what a net_context is.
 */
#define NET_CONN_CB(name) \
	static enum net_verdict _##name(struct net_conn *conn,		\
					struct net_pkt *pkt,		\
					union net_ip_header *ip_hdr,	\
					union net_proto_header *proto_hdr, \
					void *user_data);		\
	static enum net_verdict name(struct net_conn *conn,		\
				     struct net_pkt *pkt,		\
				     union net_ip_header *ip_hdr,	\
				     union net_proto_header *proto_hdr,	\
				     void *user_data)			\
	{								\
		enum net_verdict result;				\
									\
		net_context_ref(user_data);				\
		result = _##name(conn, pkt, ip_hdr,			\
				 proto_hdr, user_data);			\
		net_context_unref(user_data);				\
		return result;						\
	}								\
	static enum net_verdict _##name(struct net_conn *conn,		\
					struct net_pkt *pkt,		\
					union net_ip_header *ip_hdr,	\
					union net_proto_header *proto_hdr, \
					void *user_data)		\


struct tcp_segment {
	uint32_t seq;
	uint32_t ack;
	uint16_t wnd;
	uint8_t flags;
	uint8_t optlen;
	void *options;
	struct sockaddr_ptr *src_addr;
	const struct sockaddr *dst_addr;
};

static char upper_if_set(char chr, bool set)
{
	if (set) {
		return chr & ~0x20;
	}

	return chr | 0x20;
}

static void net_tcp_trace(struct net_pkt *pkt,
			  struct net_tcp *tcp,
			  struct net_tcp_hdr *tcp_hdr)
{
	uint32_t rel_ack, ack;
	uint8_t flags;

	if (CONFIG_NET_TCP_LOG_LEVEL < LOG_LEVEL_DBG) {
		return;
	}

	flags = NET_TCP_FLAGS(tcp_hdr);
	ack = sys_get_be32(tcp_hdr->ack);

	if (!tcp->sent_ack) {
		rel_ack = 0U;
	} else {
		rel_ack = ack ? ack - tcp->sent_ack : 0;
	}

	NET_DBG("[%p] pkt %p src %u dst %u",
		tcp, pkt,
		ntohs(tcp_hdr->src_port),
		ntohs(tcp_hdr->dst_port));

	NET_DBG("  seq 0x%04x (%u) ack 0x%04x (%u/%u)",
		sys_get_be32(tcp_hdr->seq),
		sys_get_be32(tcp_hdr->seq),
		ack,
		ack,
		/* This tells how many bytes we are acking now */
		rel_ack);

	NET_DBG("  flags %c%c%c%c%c%c",
		upper_if_set('u', flags & NET_TCP_URG),
		upper_if_set('a', flags & NET_TCP_ACK),
		upper_if_set('p', flags & NET_TCP_PSH),
		upper_if_set('r', flags & NET_TCP_RST),
		upper_if_set('s', flags & NET_TCP_SYN),
		upper_if_set('f', flags & NET_TCP_FIN));

	NET_DBG("  win %u chk 0x%04x",
		sys_get_be16(tcp_hdr->wnd),
		ntohs(tcp_hdr->chksum));
}

static inline k_timeout_t retry_timeout(const struct net_tcp *tcp)
{
	return K_MSEC(((uint32_t)1 << tcp->retry_timeout_shift) *
		      CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT);
}

#define is_6lo_technology(pkt)						\
	(IS_ENABLED(CONFIG_NET_IPV6) &&	net_pkt_family(pkt) == AF_INET6 &&  \
	 ((IS_ENABLED(CONFIG_NET_L2_BT) &&				\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_BLUETOOTH) ||	\
	  (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&			\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_IEEE802154) ||	\
	  (IS_ENABLED(CONFIG_NET_L2_CANBUS) &&			\
	   net_pkt_lladdr_dst(pkt)->type == NET_LINK_CANBUS)))

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
				tcp, pkt, atomic_get(&pkt->atomic_ref) + 1, \
				__func__, __LINE__);			\
			pkt = net_pkt_ref(pkt);				\
		}							\
	} while (0)

static void abort_connection(struct net_tcp *tcp)
{
	struct net_context *ctx = tcp->context;

	NET_DBG("[%p] segment retransmission exceeds %d, resetting context %p",
		tcp, CONFIG_NET_TCP_RETRY_COUNT, ctx);

	if (ctx->recv_cb) {
		ctx->recv_cb(ctx, NULL, NULL, NULL, -ECONNRESET,
			     tcp->recv_user_data);
	}

	net_context_unref(ctx);
}

static void tcp_retry_expired(struct k_work *work)
{
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp, retry_timer);
	struct net_pkt *pkt;

	/* Double the retry period for exponential backoff and resend
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

		if (k_work_pending(net_pkt_work(pkt))) {
			/* If the packet is still pending in TX queue, then do
			 * not try to resend it again. This can happen if the
			 * device is so busy that the TX thread has not yet
			 * finished previous sending of this packet.
			 */
			NET_DBG("[%p] pkt %p still pending in TX queue",
				tcp, pkt);
			return;
		}

		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS)) {
			/* If we have enabled net_pkt TXTIME statistics, and we
			 * about to re-send already sent net_pkt, then reset
			 * the net_pkt start time as otherwise the TX average
			 * will be wrong (as it would be calculated from when
			 * the packet was created).
			 */
			struct net_ptp_time tp = {
				.nanosecond = k_cycle_get_32(),
			};

			net_pkt_set_timestamp(pkt, &tp);
		}

		net_pkt_set_queued(pkt, true);
		net_pkt_set_tcp_1st_msg(pkt, false);

		/* The ref here is for the initial reference which was lost
		 * when the pkt was sent. Typically the ref count should be 2
		 * at this point if the pkt is being sent by the driver.
		 */
		if (!is_6lo_technology(pkt)) {
			net_pkt_ref(pkt);
		}

		if (net_tcp_send_pkt(pkt) < 0 && !is_6lo_technology(pkt)) {
			NET_DBG("retry %u: [%p] pkt %p send failed",
				tcp->retry_timeout_shift, tcp, pkt);
			/* Undo the ref done above */
			net_pkt_unref(pkt);
		} else {
			NET_DBG("retry %u: [%p] sent pkt %p",
				tcp->retry_timeout_shift, tcp, pkt);
			if (IS_ENABLED(CONFIG_NET_STATISTICS_TCP) &&
			    !is_6lo_technology(pkt)) {
				net_stats_update_tcp_seg_rexmit(
							net_pkt_iface(pkt));
			}
		}
	} else if (CONFIG_NET_TCP_TIME_WAIT_DELAY != 0) {
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

	(void)memset(&tcp_context[i], 0, sizeof(struct net_tcp));

	tcp_context[i].flags = NET_TCP_IN_USE;
	tcp_context[i].state = NET_TCP_CLOSED;
	tcp_context[i].context = context;

	tcp_context[i].send_seq = tcp_init_isn();
	tcp_context[i].recv_wnd = MIN(NET_TCP_MAX_WIN, NET_TCP_BUF_MAX_LEN);
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

static void timewait_timer_cancel(struct net_tcp *tcp)
{
	k_delayed_work_cancel(&tcp->timewait_timer);
}

int net_tcp_release(struct net_tcp *tcp)
{
	struct net_pkt *pkt;
	struct net_pkt *tmp;
	unsigned int key;

	if (!PART_OF_ARRAY(tcp_context, tcp)) {
		return -EINVAL;
	}

	retry_timer_cancel(tcp);
	k_sem_reset(&tcp->connect_wait);

	ack_timer_cancel(tcp);
	fin_timer_cancel(tcp);
	timewait_timer_cancel(tcp);

	net_tcp_change_state(tcp, NET_TCP_CLOSED);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tcp->sent_list, pkt, tmp,
					  sent_list) {
		int refcount;

		sys_slist_remove(&tcp->sent_list, NULL, &pkt->sent_list);

		/* The packet might get freed when sending it, so if that is
		 * so, just skip it.
		 */
		if (atomic_get(&pkt->atomic_ref) == 0) {
			continue;
		}

		/* Make sure we undo the reference done in net_tcp_queue_pkt()
		 */
		net_pkt_unref(pkt);

		/* Release the packet fully unless it is still pending */
		refcount = atomic_get(&pkt->atomic_ref);
		if (refcount > 0) {
			/* If the pkt was already placed to TX queue, let
			 * it go as it will be released by L2 after it is
			 * sent.
			 */
			if (k_work_pending(net_pkt_work(pkt)) ||
			    net_pkt_sent(pkt)) {
				refcount--;
			}

			while (refcount) {
				net_pkt_unref(pkt);
				refcount--;
			}
		}
	}

	tcp->context = NULL;

	key = irq_lock();
	tcp->flags &= ~(NET_TCP_IN_USE | NET_TCP_RECV_MSS_SET);
	irq_unlock(key);

	NET_DBG("[%p] Disposed of TCP connection state", tcp);

	return 0;
}

static int finalize_segment(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		return net_ipv4_finalize(pkt, IPPROTO_TCP);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		return net_ipv6_finalize(pkt, IPPROTO_TCP);
	}

	return -EINVAL;
}

static int prepare_segment(struct net_tcp *tcp,
			   struct tcp_segment *segment,
			   struct net_pkt *pkt,
			   struct net_pkt **out_pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_context *context = tcp->context;
	struct net_buf *tail = NULL;
	struct net_tcp_hdr *tcp_hdr;
	uint16_t dst_port, src_port;
	bool pkt_allocated;
	uint8_t optlen = 0U;
	int status;

	NET_ASSERT(context);

	if (pkt) {
		/* TCP transmit data comes in with a pre-allocated
		 * net_pkt at the head (so that net_context_send can find
		 * the context), and the data after.  Rejigger so we
		 * can insert a TCP header cleanly
		 */
		tail = pkt->buffer;
		pkt->buffer = NULL;
		pkt_allocated = false;

		status = net_pkt_alloc_buffer(pkt, segment->optlen,
					      IPPROTO_TCP, ALLOC_TIMEOUT);
		if (status) {
			goto fail;
		}
	} else {
		pkt = net_pkt_alloc_with_buffer(net_context_get_iface(context),
						segment->optlen,
						net_context_get_family(context),
						IPPROTO_TCP, ALLOC_TIMEOUT);
		if (!pkt) {
			return -ENOMEM;
		}

		net_pkt_set_context(pkt, context);
		pkt_allocated = true;
	}

	net_pkt_set_tcp_1st_msg(pkt, true);
	net_pkt_set_sent(pkt, false);

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		status = net_context_create_ipv4_new(context, pkt,
				net_sin_ptr(segment->src_addr)->sin_addr,
				&(net_sin(segment->dst_addr)->sin_addr));
		if (status < 0) {
			goto fail;
		}

		dst_port = net_sin(segment->dst_addr)->sin_port;
		src_port = ((struct sockaddr_in_ptr *)&context->local)->
								sin_port;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		status = net_context_create_ipv6_new(context, pkt,
				net_sin6_ptr(segment->src_addr)->sin6_addr,
				&(net_sin6(segment->dst_addr)->sin6_addr));
		if (status < 0) {
			goto fail;
		}

		dst_port = net_sin6(segment->dst_addr)->sin6_port;
		src_port = ((struct sockaddr_in6_ptr *)&context->local)->
								sin6_port;
	} else {
		NET_DBG("[%p] Protocol family %d not supported", tcp,
			net_pkt_family(pkt));

		status = -EINVAL;
		goto fail;
	}

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!tcp_hdr) {
		status = -ENOBUFS;
		goto fail;
	}

	if (segment->options && segment->optlen) {
		/* Set the length (this value is saved in 4-byte words format)
		 */
		if ((segment->optlen & 0x3u) != 0u) {
			optlen = (segment->optlen & 0xfffCu) + 4u;
		} else {
			optlen = segment->optlen;
		}
	}

	memset(tcp_hdr, 0, NET_TCPH_LEN);

	tcp_hdr->src_port = src_port;
	tcp_hdr->dst_port = dst_port;
	sys_put_be32(segment->seq, tcp_hdr->seq);
	sys_put_be32(segment->ack, tcp_hdr->ack);
	tcp_hdr->offset   = (NET_TCPH_LEN + optlen) << 2;
	tcp_hdr->flags    = segment->flags;
	sys_put_be16(segment->wnd, tcp_hdr->wnd);
	tcp_hdr->chksum   = 0U;
	tcp_hdr->urg[0]   = 0U;
	tcp_hdr->urg[1]   = 0U;

	net_pkt_set_data(pkt, &tcp_access);

	if (optlen && net_pkt_write(pkt, segment->options, segment->optlen)) {
		goto fail;
	}

	if (tail) {
		net_pkt_append_buffer(pkt, tail);
	}

	status = finalize_segment(pkt);
	if (status < 0) {
		if (pkt_allocated) {
			net_pkt_unref(pkt);
		}

		return status;
	}

	net_tcp_trace(pkt, tcp, tcp_hdr);

	*out_pkt = pkt;

	return 0;

fail:
	if (pkt_allocated) {
		net_pkt_unref(pkt);
	} else {
		net_buf_unref(pkt->buffer);
		pkt->buffer = tail;
	}

	return status;
}

uint32_t net_tcp_get_recv_wnd(const struct net_tcp *tcp)
{
	return tcp->recv_wnd;
}

int net_tcp_prepare_segment(struct net_tcp *tcp, uint8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr_ptr *local,
			    const struct sockaddr *remote,
			    struct net_pkt **send_pkt)
{
	struct tcp_segment segment = { 0 };
	uint32_t seq;
	uint16_t wnd;
	int status;

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

	status = prepare_segment(tcp, &segment, *send_pkt, send_pkt);
	if (status < 0) {
		return status;
	}

	tcp->send_seq = seq;

	return 0;
}

static inline uint32_t get_size(uint32_t pos1, uint32_t pos2)
{
	uint32_t size;

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

uint16_t net_tcp_get_recv_mss(const struct net_tcp *tcp)
{
	sa_family_t family = net_context_get_family(tcp->context);

	if (family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_context_get_iface(tcp->context);

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
		struct net_if *iface = net_context_get_iface(tcp->context);
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

static void net_tcp_set_syn_opt(struct net_tcp *tcp, uint8_t *options,
				uint8_t *optionlen)
{
	uint32_t recv_mss;

	*optionlen = 0U;

	if (!(tcp->flags & NET_TCP_RECV_MSS_SET)) {
		recv_mss = net_tcp_get_recv_mss(tcp);
		tcp->flags |= NET_TCP_RECV_MSS_SET;
	} else {
		recv_mss = 0U;
	}

	recv_mss |= (NET_TCP_MSS_OPT << 24) | (NET_TCP_MSS_SIZE << 16);
	UNALIGNED_PUT(htonl(recv_mss),
		      (uint32_t *)(options + *optionlen));

	*optionlen += NET_TCP_MSS_SIZE;
}

int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_pkt **pkt)
{
	uint8_t options[NET_TCP_MAX_OPT_SIZE];
	uint8_t optionlen;

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

static inline void copy_sockaddr_to_sockaddr_ptr(struct net_tcp *tcp,
						 const struct sockaddr *local,
						 struct sockaddr_ptr *addr)
{
	(void)memset(addr, 0, sizeof(struct sockaddr_ptr));

#if defined(CONFIG_NET_IPV4)
	if (local->sa_family == AF_INET) {
		net_sin_ptr(addr)->sin_family = AF_INET;
		net_sin_ptr(addr)->sin_port = net_sin(local)->sin_port;
		net_sin_ptr(addr)->sin_addr = &net_sin(local)->sin_addr;
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (local->sa_family == AF_INET6) {
		net_sin6_ptr(addr)->sin6_family = AF_INET6;
		net_sin6_ptr(addr)->sin6_port = net_sin6(local)->sin6_port;
		net_sin6_ptr(addr)->sin6_addr = &net_sin6(local)->sin6_addr;
	}
#endif
}

int net_tcp_prepare_reset(struct net_tcp *tcp,
			  const struct sockaddr *local,
			  const struct sockaddr *remote,
			  struct net_pkt **pkt)
{
	struct tcp_segment segment = { 0 };
	int status = 0;
	struct sockaddr_ptr src_addr_ptr;

	if ((net_context_get_state(tcp->context) != NET_CONTEXT_UNCONNECTED) &&
	    (net_tcp_get_state(tcp) != NET_TCP_SYN_SENT) &&
	    (net_tcp_get_state(tcp) != NET_TCP_TIME_WAIT)) {
		/* Send the reset segment always with acknowledgment. */
		segment.ack = tcp->send_ack;
		segment.flags = NET_TCP_RST | NET_TCP_ACK;
		segment.seq = tcp->send_seq;

		if (!local) {
			segment.src_addr = &tcp->context->local;
		} else {
			copy_sockaddr_to_sockaddr_ptr(tcp, local,
						      &src_addr_ptr);
			segment.src_addr = &src_addr_ptr;
		}

		segment.dst_addr = remote;
		segment.wnd = 0U;
		segment.options = NULL;
		segment.optlen = 0U;

		status = prepare_segment(tcp, &segment, NULL, pkt);
	}

	return status;
}

const char *net_tcp_state_str(enum net_tcp_state state)
{
#if (CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG) || defined(CONFIG_NET_SHELL)
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
#else
	ARG_UNUSED(state);
#endif

	return "";
}

int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt)
{
	struct net_conn *conn = (struct net_conn *)context->conn_handler;
	size_t data_len = net_pkt_get_len(pkt);
	int ret;

	NET_DBG("[%p] Queue %p len %zd", context->tcp, pkt, data_len);

	if (net_context_get_state(context) != NET_CONTEXT_CONNECTED) {
		return -ENOTCONN;
	}

	NET_ASSERT(context->tcp);
	if (context->tcp->flags & NET_TCP_IS_SHUTDOWN) {
		return -ESHUTDOWN;
	}

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

	net_stats_update_tcp_sent(net_pkt_iface(pkt), data_len);

	return net_tcp_queue_pkt(context, pkt);
}

/* This function is the sole point of *adding* packets to tcp->sent_list,
 * and should remain such.
 */
static int net_tcp_queue_pkt(struct net_context *context, struct net_pkt *pkt)
{
	sys_slist_append(&context->tcp->sent_list, &pkt->sent_list);

	/* We need to restart retry_timer if it is stopped. */
	if (k_delayed_work_remaining_get(&context->tcp->retry_timer) == 0) {
		k_delayed_work_submit(&context->tcp->retry_timer,
				      retry_timeout(context->tcp));
	}

	/* Increase the ref count so that we do not lose the packet and
	 * can resend later if needed. The pkt will be released after we
	 * have received the ACK or the TCP stream is removed. This is only
	 * done for non-6lo technologies that will keep the data until ACK
	 * is received or timeout happens.
	 */
	do_ref_if_needed(context->tcp, pkt);

	return 0;
}

int net_tcp_send_pkt(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_context *ctx = net_pkt_context(pkt);
	struct net_tcp_hdr *tcp_hdr;
	bool calc_chksum = false;
	int ret;

	if (!ctx || !ctx->tcp) {
		NET_ERR("%scontext is not set on pkt %p",
			!ctx ? "" : "TCP ", pkt);
		return -EINVAL;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ip_opts_len(pkt))) {
		return -EMSGSIZE;
	}

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!tcp_hdr) {
		NET_ERR("Packet %p does not contain TCP header", pkt);
		return -EMSGSIZE;
	}

	if (sys_get_be32(tcp_hdr->ack) != ctx->tcp->send_ack) {
		sys_put_be32(ctx->tcp->send_ack, tcp_hdr->ack);
		tcp_hdr->chksum = 0U;
		calc_chksum = true;
	}

	/* The data stream code always sets this flag, because
	 * existing stacks (Linux, anyway) seem to ignore data packets
	 * without a valid-but-already-transmitted ACK.  But set it
	 * anyway if we know we need it just to sanify edge cases.
	 */
	if (ctx->tcp->sent_ack != ctx->tcp->send_ack &&
		(tcp_hdr->flags & NET_TCP_ACK) == 0U) {
		tcp_hdr->flags |= NET_TCP_ACK;
		tcp_hdr->chksum = 0U;
		calc_chksum = true;
	}

	/* As we modified the header, we need to write it back.
	 */
	net_pkt_set_data(pkt, &tcp_access);

	if (calc_chksum) {
		net_pkt_cursor_init(pkt);
		net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ip_opts_len(pkt));

		/* No need to get tcp_hdr again */
		tcp_hdr->chksum = net_calc_chksum_tcp(pkt);

		net_pkt_set_data(pkt, &tcp_access);
	}

	if (tcp_hdr->flags & NET_TCP_FIN) {
		ctx->tcp->fin_sent = 1U;
	}

	ctx->tcp->sent_ack = ctx->tcp->send_ack;

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
				net_stats_update_tcp_seg_rexmit(
							net_pkt_iface(pkt));
				net_pkt_set_sent(pkt, true);
			}

			return ret;
		}
	}

	ret = net_send_data(pkt);
	if (ret == 0) {
		net_pkt_set_sent(pkt, true);
	}

	return ret;
}

static void flush_queue(struct net_context *context)
{
	(void)net_tcp_send_data(context, NULL, NULL);
}

static void restart_timer(struct net_tcp *tcp)
{
	if (!sys_slist_is_empty(&tcp->sent_list)) {
		tcp->flags |= NET_TCP_RETRYING;
		tcp->retry_timeout_shift = 0U;
		k_delayed_work_submit(&tcp->retry_timer, retry_timeout(tcp));
	} else if (CONFIG_NET_TCP_TIME_WAIT_DELAY != 0 &&
			(tcp->fin_sent && tcp->fin_rcvd)) {
		/* We know sent_list is empty, which means if
		 * fin_sent is true it must have been ACKd
		 */
		k_delayed_work_submit(&tcp->retry_timer,
				      K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
		net_context_ref(tcp->context);
	} else {
		k_delayed_work_cancel(&tcp->retry_timer);
		tcp->flags &= ~NET_TCP_RETRYING;
	}
}

int net_tcp_send_data(struct net_context *context, net_context_send_cb_t cb,
		      void *user_data)
{
	struct net_pkt *pkt;
	int ret;

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

		/* If this pkt is the first one (not a resend), then we do
		 * not need to increase the ref count as it is 1 already.
		 * For a resent packet, the ref count is only 1 atm, and
		 * the packet would be freed in driver if we do not increase
		 * it here. This is only done for non-6lo technologies where
		 * we keep the original packet (by referencing it) for possible
		 * re-send (if ACK is not received on time).
		 */
		if (!is_6lo_technology(pkt)) {
			if (!net_pkt_tcp_1st_msg(pkt)) {
				net_pkt_ref(pkt);
			}
		}

		NET_DBG("[%p] Sending pkt %p (%zd bytes)", context->tcp,
			pkt, net_pkt_get_len(pkt));

		ret = net_tcp_send_pkt(pkt);
		if (ret < 0) {
			NET_DBG("[%p] pkt %p not sent (%d)",
				context->tcp, pkt, ret);
			if (!is_6lo_technology(pkt)) {
				net_pkt_unref(pkt);
			}

			return ret;
		}

		net_pkt_set_queued(pkt, true);
		net_pkt_set_tcp_1st_msg(pkt, false);
	}

	/* Just make the callback synchronously even if it didn't
	 * go over the wire.  In theory it would be nice to track
	 * specific ACK locations in the stream and make the
	 * callback at that time, but there's nowhere to store the
	 * user_data value right now.
	 */
	if (cb) {
		cb(context, 0, user_data);
	}

	return 0;
}

bool net_tcp_ack_received(struct net_context *ctx, uint32_t ack)
{
	struct net_tcp *tcp = ctx->tcp;
	sys_slist_t *list = &ctx->tcp->sent_list;
	sys_snode_t *head;
	struct net_pkt *pkt;
	bool valid_ack = false;

	if (net_tcp_seq_greater(ack, ctx->tcp->send_seq)) {
		NET_ERR("ctx %p: ACK for unsent data", ctx);
		net_stats_update_tcp_seg_ackerr(net_context_get_iface(ctx));
		/* RFC 793 doesn't say that invalid ack sequence is an error
		 * in the general case, but we implement tighter checking,
		 * and consider entire packet invalid.
		 */
		return false;
	}

	while (!sys_slist_is_empty(list)) {
		NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
		struct net_tcp_hdr *tcp_hdr;
		uint32_t last_seq;
		uint32_t seq_len;

		head = sys_slist_peek_head(list);
		pkt = CONTAINER_OF(head, struct net_pkt, sent_list);

		net_pkt_cursor_init(pkt);
		net_pkt_set_overwrite(pkt, true);

		if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
				 net_pkt_ip_opts_len(pkt))) {
			sys_slist_remove(list, NULL, head);
			net_pkt_unref(pkt);
			continue;
		}

		tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt,
								 &tcp_access);
		if (!tcp_hdr) {
			/* The pkt does not contain TCP header, this should
			 * not happen.
			 */
			NET_ERR("pkt %p has no TCP header", pkt);
			sys_slist_remove(list, NULL, head);
			net_pkt_unref(pkt);
			continue;
		}

		net_pkt_acknowledge_data(pkt, &tcp_access);
		seq_len = net_pkt_remaining_data(pkt);

		/* Each of SYN and FIN flags are counted
		 * as one sequence number.
		 */
		if (tcp_hdr->flags & NET_TCP_SYN) {
			seq_len += 1U;
		}
		if (tcp_hdr->flags & NET_TCP_FIN) {
			seq_len += 1U;
		}

		/* Last sequence number in this packet. */
		last_seq = sys_get_be32(tcp_hdr->seq) + seq_len - 1;

		/* Ack number should be strictly greater to acknowledged numbers
		 * below it. For example, ack no. 10 acknowledges all numbers up
		 * to and including 9.
		 */
		if (!net_tcp_seq_greater(ack, last_seq)) {
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

		NET_DBG("[%p] Received ACK pkt %p (len %zd bytes)", ctx->tcp,
			pkt, net_pkt_get_len(pkt));

		sys_slist_remove(list, NULL, head);

		/* If we receive a valid ACK, then we need to undo the ref
		 * set in net_tcp_queue_pkt() (when using non-6lo technology)
		 * or the ref set in packet creation (for 6lo packet) in order
		 * to release the pkt.
		 */
		net_pkt_set_sent(pkt, false);
		net_pkt_unref(pkt);

		valid_ack = true;
	}

	/* Restart the timer (if needed) on a valid inbound ACK.  This isn't
	 * quite the same behavior as per-packet retry timers, but is close in
	 * practice (it starts retries one timer period after the connection
	 * "got stuck") and avoids the need to track per-packet timers or
	 * sent times.
	 */
	if (valid_ack) {
		restart_timer(ctx->tcp);

		/* Flush anything pending. This is important as if there
		 * is FIN waiting in the queue, it gets sent asap.
		 */
		flush_queue(ctx);
	}

	return true;
}

void net_tcp_init(void)
{
}

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
static void validate_state_transition(enum net_tcp_state current,
				      enum net_tcp_state new)
{
	static const uint16_t valid_transitions[] = {
		[NET_TCP_CLOSED] = 1 << NET_TCP_LISTEN |
			1 << NET_TCP_SYN_SENT |
			/* Initial transition from closed->established when
			 * socket is accepted.
			 */
			1 << NET_TCP_ESTABLISHED,
		[NET_TCP_LISTEN] = 1 << NET_TCP_SYN_RCVD |
			1 << NET_TCP_SYN_SENT |
			1 << NET_TCP_CLOSED,
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
#else
static inline void validate_state_transition(enum net_tcp_state current,
					     enum net_tcp_state new)
{
	ARG_UNUSED(current);
	ARG_UNUSED(new);
}
#endif

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

	validate_state_transition(tcp->state, new_state);

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

bool net_tcp_validate_seq(struct net_tcp *tcp, struct net_tcp_hdr *tcp_hdr)
{
	return (net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
				tcp->send_ack) >= 0) &&
		(net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
				 tcp->send_ack
					+ net_tcp_get_recv_wnd(tcp)) < 0);
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

int net_tcp_parse_opts(struct net_pkt *pkt, int opt_totlen,
		       struct net_tcp_options *opts)
{
	uint8_t opt, optlen;

	while (opt_totlen) {
		if (net_pkt_read_u8(pkt, &opt)) {
			optlen = 0U;
			goto error;
		}

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
			optlen = 0U;
			goto error;
		}

		if (net_pkt_read_u8(pkt, &optlen) || optlen < 2) {
			goto error;
		}

		opt_totlen--;

		/* Subtract opt/optlen size now to avoid doing this
		 * repeatedly.
		 */
		optlen -= 2U;
		if (opt_totlen < optlen) {
			goto error;
		}

		switch (opt) {
		case NET_TCP_MSS_OPT:
			if (optlen != 2U) {
				goto error;
			}

			if (net_pkt_read_be16(pkt, &opts->mss)) {
				goto error;
			}

			break;
		default:
			if (net_pkt_skip(pkt, optlen)) {
				goto error;
			}

			break;
		}

		opt_totlen -= optlen;
	}

	return 0;

error:
	NET_ERR("Invalid TCP opt: %d len: %d", opt, optlen);
	return -EINVAL;
}

int net_tcp_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data)
{
	NET_ASSERT(context->tcp);

	if (context->tcp->flags & NET_TCP_IS_SHUTDOWN) {
		return -ESHUTDOWN;
	} else if (net_context_get_state(context) != NET_CONTEXT_CONNECTED) {
		return -ENOTCONN;
	}

	context->recv_cb = cb;
	context->tcp->recv_user_data = user_data;

	return 0;
}

static void queue_fin(struct net_context *ctx)
{
	struct net_pkt *pkt = NULL;
	bool flush = false;
	int ret;

	ret = net_tcp_prepare_segment(ctx->tcp, NET_TCP_FIN, NULL, 0,
				      NULL, &ctx->remote, &pkt);
	if (ret || !pkt) {
		return;
	}

	if (sys_slist_is_empty(&ctx->tcp->sent_list)) {
		flush = true;
	}

	net_tcp_queue_pkt(ctx, pkt);

	if (flush) {
		flush_queue(ctx);
	}
}

int net_tcp_put(struct net_context *context)
{
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		if (net_context_get_state(context) == NET_CONTEXT_CONNECTED
		    && context->tcp
		    && !context->tcp->fin_rcvd) {
			NET_DBG("TCP connection in active close, not "
				"disposing yet (waiting %dms)", FIN_TIMEOUT_MS);
			k_delayed_work_submit(&context->tcp->fin_timer,
					      FIN_TIMEOUT);
			queue_fin(context);
			return 0;
		}

		/* A listening context is only used to establish connections.
		 * Since once the connection is established it is not handled
		 * directly by the listening context but rather by the child it
		 * spawned, it is not needed to send FIN when closing such
		 * contexts.
		 */
		if (context->tcp &&
		    net_context_get_state(context) == NET_CONTEXT_LISTENING) {
			net_context_unref(context);
			return 0;
		}

		if (context->tcp &&
		    net_tcp_get_state(context->tcp) == NET_TCP_SYN_SENT) {
			net_context_unref(context);
		}

		return -ENOTCONN;
	}

	return -EOPNOTSUPP;
}

int net_tcp_listen(struct net_context *context)
{
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_tcp_change_state(context->tcp, NET_TCP_LISTEN);
		net_context_set_state(context, NET_CONTEXT_LISTENING);

		return 0;
	}

	return -EOPNOTSUPP;
}

int net_tcp_update_recv_wnd(struct net_context *context, int32_t delta)
{
	int32_t new_win;

	if (!context->tcp) {
		NET_ERR("context->tcp == NULL");
		return -EPROTOTYPE;
	}

	new_win = context->tcp->recv_wnd + delta;
	if (new_win < 0 || new_win > UINT16_MAX) {
		return -EINVAL;
	}

	context->tcp->recv_wnd = new_win;

	return 0;
}

static int send_reset(struct net_context *context, struct sockaddr *local,
		      struct sockaddr *remote);

static void backlog_ack_timeout(struct k_work *work)
{
	struct tcp_backlog_entry *backlog =
		CONTAINER_OF(work, struct tcp_backlog_entry, ack_timer);

	NET_DBG("Did not receive ACK in %dms", ACK_TIMEOUT_MS);

	/* TODO: If net_context is bound to unspecified IPv6 address
	 * and some port number, local address is not available.
	 * RST packet might be invalid. Cache local address
	 * and use it in RST message preparation.
	 */
	send_reset(backlog->tcp->context, NULL, &backlog->remote);

	(void)memset(backlog, 0, sizeof(struct tcp_backlog_entry));
}

static void tcp_copy_ip_addr_from_hdr(sa_family_t family,
				      union net_ip_header *ip_hdr,
				      struct net_tcp_hdr *tcp_hdr,
				      struct sockaddr *addr,
				      bool is_src_addr)
{
	uint16_t port;

	if (is_src_addr) {
		port = tcp_hdr->src_port;
	} else {
		port = tcp_hdr->dst_port;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		struct sockaddr_in *addr4 = net_sin(addr);

		if (is_src_addr) {
			net_ipaddr_copy(&addr4->sin_addr, &ip_hdr->ipv4->src);
		} else {
			net_ipaddr_copy(&addr4->sin_addr, &ip_hdr->ipv4->dst);
		}

		addr4->sin_port = port;
		addr->sa_family = AF_INET;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		struct sockaddr_in6 *addr6 = net_sin6(addr);

		if (is_src_addr) {
			net_ipaddr_copy(&addr6->sin6_addr, &ip_hdr->ipv6->src);
		} else {
			net_ipaddr_copy(&addr6->sin6_addr, &ip_hdr->ipv6->dst);
		}

		addr6->sin6_port = port;
		addr->sa_family = AF_INET6;
	}
}

static int tcp_backlog_find(struct net_pkt *pkt,
			    union net_ip_header *ip_hdr,
			    struct net_tcp_hdr *tcp_hdr,
			    int *empty_slot)
{
	int i, empty = -1;

	for (i = 0; i < CONFIG_NET_TCP_BACKLOG_SIZE; i++) {
		if (tcp_backlog[i].tcp == NULL && empty < 0) {
			empty = i;
			continue;
		}

		if (net_pkt_family(pkt) != tcp_backlog[i].remote.sa_family) {
			continue;
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) &&
		    net_pkt_family(pkt) == AF_INET) {
			if (net_sin(&tcp_backlog[i].remote)->sin_port !=
			    tcp_hdr->src_port) {
				continue;
			}

			if (memcmp(&net_sin(&tcp_backlog[i].remote)->sin_addr,
				   &ip_hdr->ipv4->src,
				   sizeof(struct in_addr))) {
				continue;
			}
		} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    net_pkt_family(pkt) == AF_INET6) {
			if (net_sin6(&tcp_backlog[i].remote)->sin6_port !=
			    tcp_hdr->src_port) {
				continue;
			}

			if (memcmp(&net_sin6(&tcp_backlog[i].remote)->sin6_addr,
				   &ip_hdr->ipv6->src,
				   sizeof(struct in6_addr))) {
				continue;
			}
		}

		return i;
	}

	if (empty_slot) {
		*empty_slot = empty;
	}

	return -EADDRNOTAVAIL;
}

static int tcp_backlog_syn(struct net_pkt *pkt,
			   union net_ip_header *ip_hdr,
			   struct net_tcp_hdr *tcp_hdr,
			   struct net_context *context,
			   uint16_t send_mss)
{
	int empty_slot = -1;

	if (tcp_backlog_find(pkt, ip_hdr, tcp_hdr, &empty_slot) >= 0) {
		return -EADDRINUSE;
	}

	if (empty_slot < 0) {
		return -ENOSPC;
	}

	tcp_backlog[empty_slot].tcp = context->tcp;

	tcp_copy_ip_addr_from_hdr(net_pkt_family(pkt), ip_hdr, tcp_hdr,
				  &tcp_backlog[empty_slot].remote, true);

	tcp_backlog[empty_slot].send_seq = context->tcp->send_seq;
	tcp_backlog[empty_slot].send_ack = context->tcp->send_ack;
	tcp_backlog[empty_slot].send_mss = send_mss;

	k_delayed_work_init(&tcp_backlog[empty_slot].ack_timer,
			    backlog_ack_timeout);
	k_delayed_work_submit(&tcp_backlog[empty_slot].ack_timer, ACK_TIMEOUT);

	return 0;
}

static int tcp_backlog_ack(struct net_pkt *pkt,
			   union net_ip_header *ip_hdr,
			   struct net_tcp_hdr *tcp_hdr,
			   struct net_context *context)
{
	int r;

	r = tcp_backlog_find(pkt, ip_hdr, tcp_hdr, NULL);
	if (r < 0) {
		return r;
	}

	/* Sent SEQ + 1 needs to be the same as the received ACK */
	if (tcp_backlog[r].send_seq + 1 != sys_get_be32(tcp_hdr->ack)) {
		return -EINVAL;
	}

	memcpy(&context->remote, &tcp_backlog[r].remote,
		sizeof(struct sockaddr));
	context->tcp->send_seq = tcp_backlog[r].send_seq + 1;
	context->tcp->send_ack = tcp_backlog[r].send_ack;
	context->tcp->send_mss = tcp_backlog[r].send_mss;

	k_delayed_work_cancel(&tcp_backlog[r].ack_timer);
	(void)memset(&tcp_backlog[r], 0, sizeof(struct tcp_backlog_entry));

	return 0;
}

static int tcp_backlog_rst(struct net_pkt *pkt,
			   union net_ip_header *ip_hdr,
			   struct net_tcp_hdr *tcp_hdr)
{
	int r;

	r = tcp_backlog_find(pkt, ip_hdr, tcp_hdr, NULL);
	if (r < 0) {
		return r;
	}

	/* The ACK sent needs to be the same as the received SEQ */
	if (tcp_backlog[r].send_ack != sys_get_be32(tcp_hdr->seq)) {
		return -EINVAL;
	}

	k_delayed_work_cancel(&tcp_backlog[r].ack_timer);
	(void)memset(&tcp_backlog[r], 0, sizeof(struct tcp_backlog_entry));

	return 0;
}

static void handle_fin_timeout(struct k_work *work)
{
	struct net_tcp *tcp =
		CONTAINER_OF(work, struct net_tcp, fin_timer);

	NET_DBG("Did not receive FIN in %dms", FIN_TIMEOUT_MS);

	net_context_unref(tcp->context);
}

static void handle_ack_timeout(struct k_work *work)
{
	/* This means that we did not receive ACK response in time. */
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp, ack_timer);

	NET_DBG("Did not receive ACK in %dms while in %s", ACK_TIMEOUT_MS,
		net_tcp_state_str(net_tcp_get_state(tcp)));

	if (net_tcp_get_state(tcp) == NET_TCP_LAST_ACK) {
		/* We did not receive the last ACK on time. We can only
		 * close the connection at this point. We will not send
		 * anything to peer in this last state, but will go directly
		 * to to CLOSED state.
		 */
		net_tcp_change_state(tcp, NET_TCP_CLOSED);

		if (tcp->context->recv_cb) {
			tcp->context->recv_cb(tcp->context, NULL, NULL, NULL,
					      0, tcp->recv_user_data);
		}

		net_context_unref(tcp->context);
	}
}

static void handle_timewait_timeout(struct k_work *work)
{
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp,
					   timewait_timer);

	NET_DBG("Timewait expired in %dms", CONFIG_NET_TCP_TIME_WAIT_DELAY);

	if (net_tcp_get_state(tcp) == NET_TCP_TIME_WAIT) {
		net_tcp_change_state(tcp, NET_TCP_CLOSED);

		if (tcp->context->recv_cb) {
			tcp->context->recv_cb(tcp->context, NULL, NULL, NULL,
					      0, tcp->recv_user_data);
		}

		net_context_unref(tcp->context);
	}
}

int net_tcp_get(struct net_context *context)
{
	context->tcp = net_tcp_alloc(context);
	if (!context->tcp) {
		NET_ASSERT(context->tcp, "Cannot allocate TCP context");
		return -ENOBUFS;
	}

	k_delayed_work_init(&context->tcp->ack_timer, handle_ack_timeout);
	k_delayed_work_init(&context->tcp->fin_timer, handle_fin_timeout);
	k_delayed_work_init(&context->tcp->timewait_timer,
			    handle_timewait_timeout);

	return 0;
}

int net_tcp_unref(struct net_context *context)
{
	int i;

	if (!context->tcp)
		return 0;

	/* Clear the backlog for this TCP context. */
	for (i = 0; i < CONFIG_NET_TCP_BACKLOG_SIZE; i++) {
		if (tcp_backlog[i].tcp != context->tcp) {
			continue;
		}

		k_delayed_work_cancel(&tcp_backlog[i].ack_timer);
		(void)memset(&tcp_backlog[i], 0, sizeof(tcp_backlog[i]));
	}

	net_tcp_release(context->tcp);
	context->tcp = NULL;

	return 0;
}

/** **/

#define net_tcp_print_recv_info(str, pkt, port)				\
	if (IS_ENABLED(CONFIG_NET_TCP_LOG_LEVEL_DBG)) {			\
		if (net_pkt_family(pkt) == AF_INET6) {	\
			NET_DBG("%s received from %s port %d", str,	\
				log_strdup(net_sprint_ipv6_addr(	\
					     &NET_IPV6_HDR(pkt)->src)), \
				ntohs(port));				\
		} else if (net_pkt_family(pkt) == AF_INET) {\
			NET_DBG("%s received from %s port %d", str,	\
				log_strdup(net_sprint_ipv4_addr(	\
					     &NET_IPV4_HDR(pkt)->src)), \
				ntohs(port));				\
		}							\
	}

#define net_tcp_print_send_info(str, pkt, port)				\
	if (IS_ENABLED(CONFIG_NET_TCP_LOG_LEVEL_DBG)) {			\
		if (net_pkt_family(pkt) == AF_INET6) {		\
			NET_DBG("%s sent to %s port %d", str,		\
				log_strdup(net_sprint_ipv6_addr(	\
					     &NET_IPV6_HDR(pkt)->dst)), \
				ntohs(port));				\
		} else if (net_pkt_family(pkt) == AF_INET) {	\
			NET_DBG("%s sent to %s port %d", str,		\
				log_strdup(net_sprint_ipv4_addr(	\
					     &NET_IPV4_HDR(pkt)->dst)), \
				ntohs(port));				\
		}							\
	}

static void print_send_info(struct net_pkt *pkt,
			    const char *msg, const struct sockaddr *remote)
{
	if (CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		uint16_t port = 0U;

		if (IS_ENABLED(CONFIG_NET_IPV4) &&
		    net_pkt_family(pkt) == AF_INET) {
			struct sockaddr_in *addr4 = net_sin(remote);

			port = addr4->sin_port;
		}

		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    net_pkt_family(pkt) == AF_INET6) {
			struct sockaddr_in6 *addr6 = net_sin6(remote);

			port = addr6->sin6_port;
		}

		net_tcp_print_send_info(msg, pkt, port);
	}
}

/* Send SYN or SYN/ACK. */
static inline int send_syn_segment(struct net_context *context,
				       const struct sockaddr_ptr *local,
				       const struct sockaddr *remote,
				       int flags, const char *msg)
{
	struct net_pkt *pkt = NULL;
	int ret;
	uint8_t options[NET_TCP_MAX_OPT_SIZE];
	uint8_t optionlen = 0U;

	if (flags == NET_TCP_SYN) {
		net_tcp_set_syn_opt(context->tcp, options, &optionlen);
	}

	ret = net_tcp_prepare_segment(context->tcp, flags, options, optionlen,
				      local, remote, &pkt);
	if (ret) {
		return ret;
	}

	print_send_info(pkt, msg, remote);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
		return ret;
	}

	net_pkt_set_sent(pkt, true);
	context->tcp->send_seq++;

	return ret;
}

static inline int send_syn(struct net_context *context,
			   const struct sockaddr *remote)
{
	net_tcp_change_state(context->tcp, NET_TCP_SYN_SENT);

	return send_syn_segment(context, NULL, remote, NET_TCP_SYN, "SYN");
}

static inline int send_syn_ack(struct net_context *context,
			       struct sockaddr_ptr *local,
			       struct sockaddr *remote)
{
	return send_syn_segment(context, local, remote,
				    NET_TCP_SYN | NET_TCP_ACK,
				    "SYN_ACK");
}

static int send_ack(struct net_context *context,
		    struct sockaddr *remote, bool force)
{
	struct net_pkt *pkt = NULL;
	int ret;

	/* Something (e.g. a data transmission under the user
	 * callback) already sent the ACK, no need
	 */
	if (!force && context->tcp->send_ack == context->tcp->sent_ack) {
		return 0;
	}

	ret = net_tcp_prepare_ack(context->tcp, remote, &pkt);
	if (ret) {
		return ret;
	}

	print_send_info(pkt, "ACK", remote);

	ret = net_tcp_send_pkt(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

static int send_reset(struct net_context *context,
		      struct sockaddr *local,
		      struct sockaddr *remote)
{
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_reset(context->tcp, local, remote, &pkt);
	if (ret || !pkt) {
		return ret;
	}

	print_send_info(pkt, "RST", remote);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	net_pkt_set_sent(pkt, true);
	return ret;
}

static uint16_t adjust_data_len(struct net_pkt *pkt, struct net_tcp_hdr *tcp_hdr,
			     uint16_t data_len)
{
	uint8_t offset = tcp_hdr->offset >> 4;

	/* We need to adjust the length of the data part if there
	 * are TCP options.
	 */
	if ((offset << 2) > sizeof(struct net_tcp_hdr)) {
		net_pkt_skip(pkt, (offset << 2) -
			     sizeof(struct net_tcp_hdr));

		data_len -= (offset << 2) - sizeof(struct net_tcp_hdr);
	}

	return data_len;
}

/* This is called when we receive data after the connection has been
 * established. The core TCP logic is located here.
 *
 * Prototype:
 * enum net_verdict tcp_established(struct net_conn *conn,
 *				    union net_ip_header *ip_hdr,
 *				    union net_proto_header *proto_hdr,
 *				    struct net_pkt *pkt,
 *                                  void *user_data)
 */
NET_CONN_CB(tcp_established)
{
	struct net_context *context = (struct net_context *)user_data;
	struct net_tcp_hdr *tcp_hdr = proto_hdr->tcp;
	enum net_verdict ret = NET_OK;
	bool do_not_send_ack = false;
	uint8_t tcp_flags;
	uint16_t data_len;

	k_mutex_lock(&context->lock, K_FOREVER);

	NET_ASSERT(context && context->tcp);

	if (net_tcp_get_state(context->tcp) < NET_TCP_ESTABLISHED) {
		NET_ERR("Context %p in wrong state %d",
			context, net_tcp_get_state(context->tcp));
		ret = NET_DROP;
		goto unlock;
	}

	net_tcp_print_recv_info("DATA", pkt, tcp_hdr->src_port);

	tcp_flags = NET_TCP_FLAGS(tcp_hdr);

	if (net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
			    context->tcp->send_ack) < 0) {
		/* Peer sent us packet we've already seen. Apparently,
		 * our ack was lost.
		 */

		/* RFC793 specifies that "highest" (i.e. current from our PoV)
		 * ack # value can/should be sent, so we just force resend.
		 */
resend_ack:
		send_ack(context, &conn->remote_addr, true);
		ret = NET_DROP;
		goto unlock;
	}

	if (net_tcp_seq_cmp(sys_get_be32(tcp_hdr->seq),
			    context->tcp->send_ack) > 0) {
		/* Don't try to reorder packets.  If it doesn't
		 * match the next segment exactly, drop and wait for
		 * retransmit
		 */
		ret = NET_DROP;
		goto unlock;
	}

	/*
	 * If we receive RST here, we close the socket. See RFC 793 chapter
	 * called "Reset Processing" for details.
	 */
	if (tcp_flags & NET_TCP_RST) {
		/* We only accept RST packet that has valid seq field. */
		if (!net_tcp_validate_seq(context->tcp, tcp_hdr)) {
			net_stats_update_tcp_seg_rsterr(net_pkt_iface(pkt));
			ret = NET_DROP;
			goto unlock;
		}

		net_stats_update_tcp_seg_rst(net_pkt_iface(pkt));

		net_tcp_print_recv_info("RST", pkt, tcp_hdr->src_port);

		if (context->recv_cb) {
			context->recv_cb(context, NULL, NULL, NULL, -ECONNRESET,
					 context->tcp->recv_user_data);
		}

		net_context_unref(context);

		ret = NET_DROP;
		goto unlock;
	}

	/* Handle TCP state transition */
	if (tcp_flags & NET_TCP_ACK) {
		if (!net_tcp_ack_received(context,
					  sys_get_be32(tcp_hdr->ack))) {
			ret = NET_DROP;
			goto unlock;
		}

		/* TCP state might be changed after maintaining the sent pkt
		 * list, e.g., an ack of FIN is received.
		 */

		if (net_tcp_get_state(context->tcp)
			   == NET_TCP_FIN_WAIT_1) {
			/* Active close: step to FIN_WAIT_2 */
			net_tcp_change_state(context->tcp, NET_TCP_FIN_WAIT_2);
		} else if (net_tcp_get_state(context->tcp)
			   == NET_TCP_LAST_ACK) {
			/* Passive close: step to CLOSED */
			net_tcp_change_state(context->tcp, NET_TCP_CLOSED);
			/* Release the pkt before clean up */
			net_pkt_unref(pkt);
			goto clean_up;
		}
	}

	if (tcp_flags & NET_TCP_FIN) {
		if (net_tcp_get_state(context->tcp) == NET_TCP_ESTABLISHED) {
			/* Passive close: step to CLOSE_WAIT */
			net_tcp_change_state(context->tcp, NET_TCP_CLOSE_WAIT);

			/* We should receive ACK next in order to get rid of
			 * LAST_ACK state that we are entering in a short while.
			 * But we need to be prepared to NOT to receive it as
			 * otherwise the connection would be stuck forever.
			 */
			k_delayed_work_submit(&context->tcp->ack_timer,
					      ACK_TIMEOUT);

			net_context_set_closing(context, true);
		} else if (net_tcp_get_state(context->tcp)
			   == NET_TCP_FIN_WAIT_2) {
			/* Received FIN on FIN_WAIT_2, so cancel the timer */
			k_delayed_work_cancel(&context->tcp->fin_timer);
			/* Active close: step to TIME_WAIT */
			net_tcp_change_state(context->tcp, NET_TCP_TIME_WAIT);
		}

		context->tcp->fin_rcvd = 1U;
	}

	if (!IS_ENABLED(CONFIG_NET_TCP_AUTO_ACCEPT) &&
	    net_context_is_accepting(context)) {
		data_len = 0;
		do_not_send_ack = true;
	} else {
		data_len = net_pkt_remaining_data(pkt);
	}

	if (data_len > net_tcp_get_recv_wnd(context->tcp)) {
		/* In case we have zero window, we should still accept
		 * Zero Window Probes from peer, which per convention
		 * come with len=1. Note that normally we need to check
		 * for net_tcp_get_recv_wnd(context->tcp) == 0, but
		 * given the if above, we know that if data_len == 1,
		 * then net_tcp_get_recv_wnd(context->tcp) can be only 0
		 * here.
		 */
		if (data_len == 1U) {
			goto resend_ack;
		}

		NET_ERR("Context %p: overflow of recv window (%d vs %d), "
			"pkt dropped",
			context, net_tcp_get_recv_wnd(context->tcp), data_len);
		ret = NET_DROP;
		goto unlock;
	}

	/* If the pkt has data, notify the recv callback which should
	 * release the pkt. Otherwise, release the pkt immediately.
	 */
	if (data_len > 0) {
		data_len = adjust_data_len(pkt, tcp_hdr, data_len);

		ret = net_context_packet_received(conn, pkt, ip_hdr, proto_hdr,
						  context->tcp->recv_user_data);
	} else if (data_len == 0U) {
		net_pkt_unref(pkt);
	}

	if (do_not_send_ack == false) {
		/* Increment the ack */
		context->tcp->send_ack += data_len;
		if (tcp_flags & NET_TCP_FIN) {
			context->tcp->send_ack += 1U;
		}

		send_ack(context, &conn->remote_addr, false);
	}

clean_up:
	if (net_tcp_get_state(context->tcp) == NET_TCP_TIME_WAIT) {
		k_delayed_work_submit(&context->tcp->timewait_timer,
				      K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
	}

	if (net_tcp_get_state(context->tcp) == NET_TCP_CLOSED) {
		if (context->recv_cb) {
			context->recv_cb(context, NULL, NULL, NULL, 0,
					 context->tcp->recv_user_data);
		}

		net_context_unref(context);
	}

unlock:
	k_mutex_unlock(&context->lock);

	return ret;
}

/*
 * Prototype:
 * enum net_verdict tcp_synack_received(struct net_conn *conn,
 *					struct net_pkt *pkt,
 *				        union net_ip_header *ip_hdr,
 *				        union net_proto_header *proto_hdr,
 *					void *user_data)
 */
NET_CONN_CB(tcp_synack_received)
{
	struct net_context *context = (struct net_context *)user_data;
	struct net_tcp_hdr *tcp_hdr = proto_hdr->tcp;
	int ret;

	NET_ASSERT(context && context->tcp);

	switch (net_tcp_get_state(context->tcp)) {
	case NET_TCP_SYN_SENT:
		net_context_set_iface(context, net_pkt_iface(pkt));
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, net_tcp_get_state(context->tcp));
		return NET_DROP;
	}

	net_pkt_set_context(pkt, context);

	NET_ASSERT(net_pkt_iface(pkt));

	if (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_RST) {
		/* We only accept RST packet that has valid seq field. */
		if (!net_tcp_validate_seq(context->tcp, tcp_hdr)) {
			net_stats_update_tcp_seg_rsterr(net_pkt_iface(pkt));
			return NET_DROP;
		}

		net_stats_update_tcp_seg_rst(net_pkt_iface(pkt));

		k_sem_give(&context->tcp->connect_wait);

		if (context->connect_cb) {
			context->connect_cb(context, -ECONNREFUSED,
					    context->user_data);
		}

		return NET_DROP;
	}

	if (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_SYN) {
		context->tcp->send_ack =
			sys_get_be32(tcp_hdr->seq) + 1;
	}
	/*
	 * If we receive SYN, we send SYN-ACK and go to SYN_RCVD state.
	 */
	if (NET_TCP_FLAGS(tcp_hdr) == (NET_TCP_SYN | NET_TCP_ACK)) {
		/* Remove the temporary connection handler and register
		 * a proper now as we have an established connection.
		 */
		struct sockaddr local_addr;
		struct sockaddr remote_addr;

		tcp_copy_ip_addr_from_hdr(net_pkt_family(pkt), ip_hdr, tcp_hdr,
					  &remote_addr, true);
		tcp_copy_ip_addr_from_hdr(net_pkt_family(pkt), ip_hdr, tcp_hdr,
					  &local_addr, false);

		net_tcp_unregister(context->conn_handler);

		ret = net_tcp_register(net_pkt_family(pkt),
				       &remote_addr,
				       &local_addr,
				       ntohs(tcp_hdr->src_port),
				       ntohs(tcp_hdr->dst_port),
				       tcp_established,
				       context,
				       &context->conn_handler);
		if (ret < 0) {
			NET_DBG("Cannot register TCP handler (%d)", ret);
			send_reset(context, &local_addr, &remote_addr);
			return NET_DROP;
		}

		net_tcp_change_state(context->tcp, NET_TCP_ESTABLISHED);
		net_context_set_state(context, NET_CONTEXT_CONNECTED);

		send_ack(context, &remote_addr, false);

		k_sem_give(&context->tcp->connect_wait);

		if (context->connect_cb) {
			context->connect_cb(context, 0, context->user_data);
		}
	}

	return NET_DROP;
}

static void get_sockaddr_ptr(union net_ip_header *ip_hdr,
			     struct net_tcp_hdr *tcp_hdr,
			     sa_family_t family,
			     struct sockaddr_ptr *addr)
{
	(void)memset(addr, 0, sizeof(*addr));

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		struct sockaddr_in_ptr *addr4 = net_sin_ptr(addr);

		addr4->sin_family = AF_INET;
		addr4->sin_port = tcp_hdr->dst_port;
		addr4->sin_addr = &ip_hdr->ipv4->dst;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		struct sockaddr_in6_ptr *addr6 = net_sin6_ptr(addr);

		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = tcp_hdr->dst_port;
		addr6->sin6_addr = &ip_hdr->ipv6->dst;
	}
}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static inline void copy_pool_vars(struct net_context *new_context,
				  struct net_context *listen_context)
{
	new_context->tx_slab = listen_context->tx_slab;
	new_context->data_pool = listen_context->data_pool;
}
#else
#define copy_pool_vars(...)
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

/* This callback is called when we are waiting connections and we receive
 * a packet. We need to check if we are receiving proper msg (SYN) here.
 * The ACK could also be received, in which case we have an established
 * connection.
 *
 * Prototype:
 * enum net_verdict tcp_syn_rcvd(struct net_conn *conn,
 *			         struct net_pkt *pkt,
 *			         union net_ip_header *ip_hdr,
 *			         union net_proto_header *proto_hdr,
 *			         void *user_data)
 */
NET_CONN_CB(tcp_syn_rcvd)
{
	struct net_context *context = (struct net_context *)user_data;
	struct net_tcp_hdr *tcp_hdr = proto_hdr->tcp;
	struct net_tcp *tcp;
	struct sockaddr_ptr pkt_src_addr;
	struct sockaddr local_addr;
	struct sockaddr remote_addr;

	NET_ASSERT(context && context->tcp);

	tcp = context->tcp;

	switch (net_tcp_get_state(tcp)) {
	case NET_TCP_LISTEN:
		net_context_set_iface(context, net_pkt_iface(pkt));
		break;
	case NET_TCP_SYN_RCVD:
		if (net_pkt_iface(pkt) != net_context_get_iface(context)) {
			return NET_DROP;
		}
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, tcp->state);
		return NET_DROP;
	}

	net_pkt_set_context(pkt, context);

	NET_ASSERT(net_pkt_iface(pkt));

	tcp_copy_ip_addr_from_hdr(net_pkt_family(pkt), ip_hdr, tcp_hdr,
				  &remote_addr, true);
	tcp_copy_ip_addr_from_hdr(net_pkt_family(pkt), ip_hdr, tcp_hdr,
				  &local_addr, false);

	/*
	 * If we receive SYN, we send SYN-ACK and go to SYN_RCVD state.
	 */
	if (NET_TCP_FLAGS(tcp_hdr) == NET_TCP_SYN) {
		struct net_tcp_options tcp_opts = {
			.mss = NET_TCP_DEFAULT_MSS,
		};
		int opt_totlen;
		int r;

		net_tcp_print_recv_info("SYN", pkt, tcp_hdr->src_port);

		opt_totlen = NET_TCP_HDR_LEN(tcp_hdr)
			     - sizeof(struct net_tcp_hdr);
		/* We expect MSS option to be present (opt_totlen > 0),
		 * so call unconditionally.
		 */
		if (net_tcp_parse_opts(pkt, opt_totlen, &tcp_opts) < 0) {
			return NET_DROP;
		}

		net_tcp_change_state(tcp, NET_TCP_SYN_RCVD);

		/* Set TCP seq and ack which are then stored in the backlog */
		context->tcp->send_seq = tcp_init_isn();
		context->tcp->send_ack =
			sys_get_be32(tcp_hdr->seq) + 1;

		/* Get MSS from TCP options here*/

		r = tcp_backlog_syn(pkt, ip_hdr, tcp_hdr,
				    context, tcp_opts.mss);
		if (r < 0) {
			if (r == -EADDRINUSE) {
				NET_DBG("TCP connection already exists");
			} else {
				NET_DBG("No free TCP backlog entries");
			}

			return NET_DROP;
		}

		get_sockaddr_ptr(ip_hdr, tcp_hdr,
				 net_context_get_family(context),
				 &pkt_src_addr);
		send_syn_ack(context, &pkt_src_addr, &remote_addr);
		net_pkt_unref(pkt);
		return NET_OK;
	}

	/*
	 * See RFC 793 chapter 3.4 "Reset Processing" and RFC 793, page 65
	 * for more details.
	 */
	if (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_RST) {

		if (tcp_backlog_rst(pkt, ip_hdr, tcp_hdr) < 0) {
			net_stats_update_tcp_seg_rsterr(net_pkt_iface(pkt));
			return NET_DROP;
		}

		net_stats_update_tcp_seg_rst(net_pkt_iface(pkt));

		net_tcp_print_recv_info("RST", pkt, tcp_hdr->src_port);

		return NET_DROP;
	}

	/*
	 * If we receive ACK, we go to ESTABLISHED state.
	 */
	if (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_ACK) {
		struct net_context *new_context;
		socklen_t addrlen;
		int ret;

		net_tcp_print_recv_info("ACK", pkt, tcp_hdr->src_port);

		if (!context->tcp->accept_cb) {
			NET_DBG("No accept callback, connection reset.");
			goto reset;
		}

		/* We create a new context that starts to wait data.
		 */
		ret = net_context_get(net_pkt_family(pkt),
				      SOCK_STREAM, IPPROTO_TCP,
				      &new_context);
		if (ret < 0) {
			NET_DBG("Cannot get accepted context, "
				"connection reset");
			goto conndrop;
		}

		ret = tcp_backlog_ack(pkt, ip_hdr, tcp_hdr, new_context);
		if (ret < 0) {
			NET_DBG("Cannot find context from TCP backlog");

			net_context_unref(new_context);

			goto conndrop;
		}

		ret = net_context_bind(new_context, &local_addr,
				       sizeof(local_addr));
		if (ret < 0) {
			NET_DBG("Cannot bind accepted context, "
				"connection reset");
			net_context_unref(new_context);
			goto conndrop;
		}

		new_context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

		memcpy(&new_context->remote, &remote_addr,
		       sizeof(remote_addr));

		ret = net_tcp_register(net_pkt_family(pkt),
			       &new_context->remote,
			       &local_addr,
			       ntohs(net_sin(&new_context->remote)->sin_port),
			       ntohs(net_sin(&local_addr)->sin_port),
			       tcp_established,
			       new_context,
			       &new_context->conn_handler);
		if (ret < 0) {
			NET_DBG("Cannot register accepted TCP handler (%d)",
				ret);
			net_context_unref(new_context);
			goto conndrop;
		}

		/* Swap the newly-created TCP states with the one that
		 * was used to establish this connection. The old TCP
		 * must be listening to accept other connections.
		 */
		copy_pool_vars(new_context, context);

		net_tcp_change_state(tcp, NET_TCP_LISTEN);

		net_tcp_change_state(new_context->tcp, NET_TCP_ESTABLISHED);

		/* Mark the new context to be still accepting so that we
		 * can do proper cleanup if connection is closed before
		 * we have called accept()
		 */
		net_context_set_accepting(new_context, true);

		net_context_set_state(new_context, NET_CONTEXT_CONNECTED);

		if (new_context->remote.sa_family == AF_INET) {
			addrlen = sizeof(struct sockaddr_in);
		} else if (new_context->remote.sa_family == AF_INET6) {
			addrlen = sizeof(struct sockaddr_in6);
		} else {
			NET_ASSERT(false, "Invalid protocol family %d",
				   new_context->remote.sa_family);
			net_context_unref(new_context);
			return NET_DROP;
		}

		context->tcp->accept_cb(new_context,
					&new_context->remote,
					addrlen,
					0,
					context->user_data);
		net_pkt_unref(pkt);
		return NET_OK;
	}

	return NET_DROP;

conndrop:
	net_stats_update_tcp_seg_conndrop(net_pkt_iface(pkt));

reset:
	send_reset(tcp->context, &local_addr, &remote_addr);

	return NET_DROP;
}

int net_tcp_accept(struct net_context *context,
		   net_tcp_accept_cb_t cb,
		   void *user_data)
{
	struct sockaddr local_addr;
	struct sockaddr *laddr = NULL;
	uint16_t lport = 0U;
	int ret;

	NET_ASSERT(context->tcp);

	if (net_tcp_get_state(context->tcp) != NET_TCP_LISTEN) {
		NET_DBG("Context %p in wrong state %d, should be %d",
			context, context->tcp->state, NET_TCP_LISTEN);
		return -EINVAL;
	}

	if (cb == NULL) {
		/* The context is being shut down */
		if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
			context->tcp->accept_cb = NULL;
			return 0;
		}
	}

	local_addr.sa_family = net_context_get_family(context);

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);

			laddr = &local_addr;
		}

		net_sin6(&local_addr)->sin6_port = lport =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				      net_sin_ptr(&context->local)->sin_addr);

			laddr = &local_addr;
		}

		net_sin(&local_addr)->sin_port = lport =
			net_sin((struct sockaddr *)&context->local)->sin_port;
	}
#endif /* CONFIG_NET_IPV4 */

	ret = net_tcp_register(net_context_get_family(context),
			       context->flags & NET_CONTEXT_REMOTE_ADDR_SET ?
			       &context->remote : NULL,
			       laddr,
			       ntohs(net_sin(&context->remote)->sin_port),
			       ntohs(lport),
			       tcp_syn_rcvd,
			       context,
			       &context->conn_handler);
	if (ret < 0) {
		return ret;
	}

	context->user_data = user_data;

	/* accept callback is only valid for TCP contexts */
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		context->tcp->accept_cb = cb;
	}

	return 0;
}

int net_tcp_connect(struct net_context *context,
		    const struct sockaddr *addr,
		    struct sockaddr *laddr,
		    uint16_t rport,
		    uint16_t lport,
		    k_timeout_t timeout,
		    net_context_connect_cb_t cb,
		    void *user_data)
{
	int ret;

	NET_ASSERT(context->tcp);

	if (net_context_get_type(context) != SOCK_STREAM) {
		return -ENOTSUP;
	}

	/* We need to register a handler, otherwise the SYN-ACK
	 * packet would not be received.
	 */
	ret = net_tcp_register(net_context_get_family(context),
			       addr,
			       laddr,
			       ntohs(rport),
			       ntohs(lport),
			       tcp_synack_received,
			       context,
			       &context->conn_handler);
	if (ret < 0) {
		return ret;
	}

	context->connect_cb = cb;
	context->user_data = user_data;

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	send_syn(context, addr);

	/* in tcp_synack_received() we give back this semaphore */
	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    k_sem_take(&context->tcp->connect_wait, timeout)) {
		return -ETIMEDOUT;
	}

	return 0;
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
