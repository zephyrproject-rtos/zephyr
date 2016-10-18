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
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_NET_DEBUG_TCP)
#define SYS_LOG_DOMAIN "net/tcp"
#define NET_DEBUG 1
#endif

#include <nanokernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/net_context.h>

#include "connection.h"
#include "net_private.h"

#include "ipv6.h"
#include "ipv4.h"
#include "tcp.h"

/*
 * Each TCP connection needs to be tracked by net_context, so
 * we need to allocate equal number of control structures here.
 */
#define NET_MAX_TCP_CONTEXT CONFIG_NET_MAX_CONTEXTS
static struct net_tcp tcp_context[NET_MAX_TCP_CONTEXT];

static struct nano_sem tcp_lock;

struct tcp_segment {
	uint32_t seq;
	uint32_t ack;
	uint16_t wnd;
	uint8_t flags;
	uint8_t optlen;
	void *options;
	struct net_buf *data;
	struct sockaddr_ptr *src_addr;
	const struct sockaddr *dst_addr;
};

#if NET_DEBUG > 0
static inline uint32_t a2u32(uint8_t *a)
{
	return  a[0] << 24 | a[1] << 16 | a[2] << 8  | a[3];
}

static inline uint32_t a2u16(uint8_t *a)
{
	return a[0] << 8 | a[1];
}

static void net_tcp_trace(char *str, struct net_buf *buf)
{
	NET_INFO("%s[TCP header]", str);
	NET_INFO("|(SrcPort)         %5u |(DestPort)      %5u |",
		 ntohs(NET_TCP_BUF(buf)->src_port),
		 ntohs(NET_TCP_BUF(buf)->dst_port));
	NET_INFO("|(Sequence number)                 0x%010x |",
		 a2u32(NET_TCP_BUF(buf)->seq));
	NET_INFO("|(ACK number)                      0x%010x |",
		 a2u32(NET_TCP_BUF(buf)->ack));
	NET_INFO("|(HL) %2u |(F)  %u%u%u%u%u%u |(Window)           %5u |",
		 (NET_TCP_BUF(buf)->offset >> 4) * 4,
		 NET_TCP_BUF(buf)->flags >> 5 & 1,
		 NET_TCP_BUF(buf)->flags >> 4 & 1,
		 NET_TCP_BUF(buf)->flags >> 3 & 1,
		 NET_TCP_BUF(buf)->flags >> 2 & 1,
		 NET_TCP_BUF(buf)->flags >> 1 & 1,
		 NET_TCP_BUF(buf)->flags & 1,
		 a2u16(NET_TCP_BUF(buf)->wnd));
	NET_INFO("|(Checksum)    0x%04x |(Urgent)           %5u |",
		 ntohs(NET_TCP_BUF(buf)->chksum),
		 a2u16(NET_TCP_BUF(buf)->urg));
}
#else
#define net_tcp_trace(...)
#endif

static inline uint32_t init_isn(void)
{
	/* Randomise initial seq number */
	return sys_rand32_get();
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

	tcp_context[i].state = NET_TCP_CLOSED;
	tcp_context[i].context = context;

	tcp_context[i].send_seq = init_isn();
	tcp_context[i].recv_max_ack = tcp_context[i].send_seq + 1u;

	return &tcp_context[i];
}

int net_tcp_release(struct net_tcp *tcp)
{
	int key;

	if (tcp >= &tcp_context[0] ||
	    tcp <= &tcp_context[NET_MAX_TCP_CONTEXT]) {
		return -EINVAL;
	}

	tcp->state = NET_TCP_CLOSED;
	tcp->context = NULL;

	if (tcp->send) {
		net_nbuf_unref(tcp->send);
		tcp->send = NULL;
	}

	if (tcp->recv) {
		net_nbuf_unref(tcp->recv);
		tcp->recv = NULL;
	}

	key = irq_lock();
	tcp->flags &= ~NET_TCP_IN_USE;
	irq_unlock(key);

	return 0;
}

static inline int net_tcp_add_options(struct net_buf *header, size_t len,
				      void *data)
{
	uint8_t optlen;

	memcpy(net_buf_add(header, len), data, len);

	/* Set the length (this value is saved in 4-byte words format) */
	if ((len & 0x3u) != 0u) {
		optlen = (len & 0xfffCu) + 4u;
	} else {
		optlen = len;
	}

	return 0;
}

static struct net_buf *prepare_segment(struct net_tcp *tcp,
				       struct tcp_segment *segment)
{
	struct net_buf *buf, *header;
	struct net_tcp_hdr *tcphdr;
	struct net_context *context = tcp->context;
	uint16_t dst_port, src_port;

	NET_ASSERT(context);

	buf = net_nbuf_get_tx(context);

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		net_ipv4_create(context, buf,
				&(net_sin(segment->dst_addr)->sin_addr));
		dst_port = net_sin(segment->dst_addr)->sin_port;
		src_port = ((struct sockaddr_in_ptr *)&context->local)->
								sin_port;
		NET_IPV4_BUF(buf)->proto = IPPROTO_TCP;
	} else
#endif
#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		net_ipv6_create(tcp->context, buf,
				&(net_sin6(segment->dst_addr)->sin6_addr));
		dst_port = net_sin6(segment->dst_addr)->sin6_port;
		src_port = ((struct sockaddr_in6_ptr *)&context->local)->
								sin6_port;
		NET_IPV6_BUF(buf)->nexthdr = IPPROTO_TCP;
	} else
#endif
	{
		goto proto_err;
	}

	header = buf->frags;

	tcphdr = (struct net_tcp_hdr *)net_buf_add(header, NET_TCPH_LEN);

	if (segment->options && segment->optlen) {
		net_tcp_add_options(header, segment->optlen, segment->options);
	} else {
		tcphdr->offset = NET_TCPH_LEN << 2;
	}

	tcphdr->src_port = src_port;
	tcphdr->dst_port = dst_port;
	tcphdr->seq[0] = segment->seq >> 24;
	tcphdr->seq[1] = segment->seq >> 16;
	tcphdr->seq[2] = segment->seq >> 8;
	tcphdr->seq[3] = segment->seq;
	tcphdr->ack[0] = segment->ack >> 24;
	tcphdr->ack[1] = segment->ack >> 16;
	tcphdr->ack[2] = segment->ack >> 8;
	tcphdr->ack[3] = segment->ack;
	tcphdr->flags = segment->flags;
	tcphdr->wnd[0] = segment->wnd >> 8;
	tcphdr->wnd[1] = segment->wnd;

	if (segment->data) {
		net_buf_frag_add(header, segment->data);
	}

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		net_ipv4_finalize(context, buf);
	} else
#endif
#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		net_ipv6_finalize(context, buf);
	} else
#endif
	{
		/* Set the data to NULL that we avoid double free when
		 * called from net_tcp_prepare_data_segment()
		 */
		segment->data = NULL;

	proto_err:
		NET_DBG("Protocol family %d not supported",
			net_nbuf_family(buf));
		net_nbuf_unref(buf);
		return NULL;
	}

	buf = net_nbuf_compact(buf);

	net_tcp_trace("", buf);

	return buf;
}

static inline uint32_t get_recv_wnd(struct net_tcp *tcp)
{
	int32_t wnd;
	uint32_t recv_wnd;

	recv_wnd = tcp->recv_wnd;

	wnd = (int32_t)(NET_TCP_BUF_MAX_LEN - tcp->recv->len);
	if (wnd < 0) {
		wnd = 0;
	}

	if (recv_wnd < (uint32_t)wnd) {
		if (((uint32_t)wnd - recv_wnd >= tcp->recv_mss) ||
		    ((uint32_t)wnd - recv_wnd >= NET_TCP_BUF_MAX_LEN >> 1) ||
		    !recv_wnd) {
			recv_wnd = (uint32_t)wnd;
		}
	} else {
		recv_wnd = (uint32_t)wnd;
	}

	return recv_wnd;
}

/* True if the (signed!) difference "seq1 - seq2" is positive and less
 * than 2^29.  That is, seq1 is "after" seq2.
 */
static inline bool seq_greater(uint32_t seq1, uint32_t seq2)
{
	int d = (int)(seq1 - seq2);
	return d > 0 && d < 0x20000000;
}

int net_tcp_prepare_segment(struct net_tcp *tcp, uint8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr *remote,
			    struct net_buf **send_buf)
{
	uint32_t seq;
	uint16_t wnd;
	uint32_t ack = 0;
	struct tcp_segment segment = { 0 };

	seq = tcp->send_seq;
	tcp->recv_wnd = get_recv_wnd(tcp);

	if (flags & NET_TCP_ACK) {
		ack = tcp->send_ack;
	}

	if (flags & NET_TCP_FIN) {
		tcp->flags |= NET_TCP_FINAL_SENT;
		seq++;

		if (tcp->state == NET_TCP_ESTABLISHED) {
			net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_1);
		} else if (tcp->state == NET_TCP_CLOSE_WAIT) {
			net_tcp_change_state(tcp, NET_TCP_LAST_ACK);
		}
	}

	if (flags & NET_TCP_SYN) {
		seq++;

		if (tcp->recv_wnd > NET_TCP_MAX_WIN) {
			wnd = NET_TCP_MAX_WIN;
		} else {
			wnd = (uint16_t)tcp->recv_wnd;
		}
	} else {
		wnd = (uint16_t)tcp->recv_wnd;
	}

	segment.src_addr = &tcp->context->local;
	segment.dst_addr = remote;
	segment.seq = tcp->send_seq;
	segment.ack = ack;
	segment.flags = flags;
	segment.wnd = wnd;
	segment.options = options;
	segment.optlen = optlen;
	segment.data = NULL;

	*send_buf = prepare_segment(tcp, &segment);

	tcp->send_seq = seq;

	if (seq_greater(tcp->send_seq, tcp->recv_max_ack)) {
		tcp->recv_max_ack = tcp->send_seq;
	}

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

int net_tcp_prepare_data_segment(struct net_tcp *tcp,
				 struct net_buf *buf,
				 void *options, size_t optlen,
				 const struct sockaddr *remote,
				 struct net_buf **send_buf)
{
	struct tcp_segment segment;
	size_t new_size;
	uint32_t seq;
	size_t data_size = net_buf_frags_len(buf);
	struct net_buf *data = NULL;
	uint8_t flags = 0;
	uint32_t tmp = 0;
	int ret = 0;

	NET_ASSERT_INFO(tcp, "TCP control block NULL");
	NET_ASSERT_INFO(buf, "No data to send");

	seq = tcp->send_seq;

	/* How much data can we send? */
	if (tcp->send) {
		new_size = net_buf_frags_len(tcp->send) -
			get_size(tcp->recv_ack, tcp->send_seq);
	} else {
		new_size = get_size(tcp->recv_ack, tcp->send_seq);
	}

	if (data_size > new_size) {
		/* Now we will only use part of the data in net_buf's */
		data_size = new_size;
	}

	if (net_sin(&tcp->context->remote)->sin_family == AF_INET) {
		tmp = ip_max_packet_len(&net_sin(&tcp->context->remote)->
					sin_addr);
	}

	/* TCP header needs to fit the MTU */
	if (data_size + NET_TCPH_LEN > tmp) {
		data_size = tmp - NET_TCPH_LEN;
	}

	flags |= NET_TCP_ACK;

	if (data_size > 0 && new_size == data_size) {
		flags |= NET_TCP_PSH;
	}

	if (tcp->flags & NET_TCP_IS_SHUTDOWN) {
		if (new_size == data_size) {
			/* End of the data sending. */
			flags |= NET_TCP_FIN;
			seq++;

			if (tcp->state == NET_TCP_ESTABLISHED) {
				net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_1);
			} else if (tcp->state == NET_TCP_CLOSE_WAIT) {
				net_tcp_change_state(tcp, NET_TCP_LAST_ACK);
			}

			tcp->flags |= NET_TCP_FINAL_SENT;
		}
	}

	tcp->recv_wnd = get_recv_wnd(tcp);

	if (data_size) {
		/* The data will not contain the TX user data buf as a first
		 * element after the copy.
		 */
		if (buf->user_data_size) {
			if (!buf->frags) {
				NET_ERR("Wrong TX buf when sending TCP data");
				return -EINVAL;
			}

			data = net_nbuf_copy(buf->frags, data_size, 0);
		} else {
			data = net_nbuf_copy(buf, data_size, 0);
		}

		/* Remove stuff from the buf so that it only contains
		 * stuff that we have not been sent yet.
		 */
		net_nbuf_pull(buf, data_size);

		/* If there is already pending data, append new data after
		 * the old one.
		 */
		if (tcp->send) {
			net_buf_frag_add(tcp->send, data);
		} else {
			tcp->send = data;
		}

		if (unlikely(!data)) {
			tcp->send_seq = seq + data_size;
			return -ENOMEM;
		}
	}

	/* Send the segment. */
	segment.src_addr = &tcp->context->local;
	segment.dst_addr = remote;
	segment.seq = tcp->send_seq;
	segment.ack = tcp->send_ack;
	segment.flags = flags;
	segment.wnd = (uint16_t)tcp->recv_wnd;
	segment.options = options;
	segment.optlen = optlen;
	segment.data = tcp->send;

	*send_buf = prepare_segment(tcp, &segment);
	if (!*send_buf) {
		if (segment.data) {
			/* tcp->send is not yet freed if we get here */
			net_nbuf_unref(tcp->send);
		}

		tcp->send = NULL;

		ret = -EINVAL;
	}

	tcp->send_seq = seq + data_size;

	if (seq_greater(tcp->send_seq, tcp->recv_max_ack)) {
		tcp->recv_max_ack = tcp->send_seq;
	}

	return ret;
}

static void net_tcp_set_syn_opt(struct net_tcp *tcp, uint8_t *options,
				uint8_t *optionlen)
{
	*optionlen = 0;

	/* If 0, detect MSS based on interface MTU minus "TCP,IP header size"
	 */
	if (tcp->recv_mss == 0) {
		sa_family_t family = net_context_get_family(tcp->context);

		if (family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			struct net_if *iface =
				net_context_get_iface(tcp->context);

			if (iface) {
				/* MTU - [TCP,IP header size]. */
				tcp->recv_mss = iface->mtu - 40;
			}
#else
			tcp->recv_mss = 0;
#endif /* CONFIG_NET_IPV4 */
		}
#if defined(CONFIG_NET_IPV6)
		else if (family == AF_INET6) {
			tcp->recv_mss = 1280;
		}
#endif /* CONFIG_NET_IPV6 */
		else {
			tcp->recv_mss = 0;
		}
	}

	*((uint32_t *)(options + *optionlen)) =
		htonl((uint32_t)(tcp->recv_mss | NET_TCP_MSS_HEADER));
	*optionlen += NET_TCP_MSS_SIZE;

	return;
}

int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_buf **buf)
{
	uint8_t options[NET_TCP_MAX_OPT_SIZE];
	uint8_t optionlen;

	switch (tcp->state) {
	case NET_TCP_SYN_RCVD:
		/* In the SYN_RCVD state acknowledgment must be with the
		 * SYN flag.
		 */
		tcp->send_seq--;

		net_tcp_set_syn_opt(tcp, options, &optionlen);

		net_tcp_prepare_segment(tcp, NET_TCP_SYN | NET_TCP_ACK,
					options, optionlen, remote, buf);
		break;

	case NET_TCP_FIN_WAIT_1:
	case NET_TCP_LAST_ACK:
		/* In the FIN_WAIT_1 and LAST_ACK states acknowledgment must
		 * be with the FIN flag.
		 */
		tcp->send_seq--;

		net_tcp_prepare_segment(tcp, NET_TCP_FIN | NET_TCP_ACK,
					0, 0, remote, buf);
		break;

	default:
		net_tcp_prepare_segment(tcp, NET_TCP_ACK, 0, 0, remote, buf);
		break;
	}

	return 0;
}

int net_tcp_prepare_reset(struct net_tcp *tcp,
			  const struct sockaddr *remote,
			  struct net_buf **buf)
{
	struct tcp_segment segment = { 0 };

	if ((net_context_get_state(tcp->context) != NET_CONTEXT_UNCONNECTED) &&
	    (tcp->state != NET_TCP_SYN_SENT) &&
	    (tcp->state != NET_TCP_TIME_WAIT)) {
		if (tcp->state == NET_TCP_SYN_RCVD) {
			/* Send the reset segment with acknowledgment. */
			segment.seq = 0;
			segment.ack = tcp->send_ack;
			segment.flags = NET_TCP_RST | NET_TCP_ACK;
		} else {
			/* Send the reset segment without acknowledgment. */
			segment.seq = tcp->recv_ack;
			segment.ack = 0;
			segment.flags = NET_TCP_RST;
		}

		segment.src_addr = &tcp->context->local;
		segment.dst_addr = remote;
		segment.wnd = 0;
		segment.options = NULL;
		segment.optlen = 0;
		segment.data = NULL;

		*buf = prepare_segment(tcp, &segment);
	}

	return 0;
}

const char const *net_tcp_state_str(enum net_tcp_state state)
{
#if NET_DEBUG
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
#endif

	return "";
}

void net_tcp_init(void)
{
	nano_sem_init(&tcp_lock);
	nano_sem_give(&tcp_lock);
}
