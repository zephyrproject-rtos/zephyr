/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tp.h"

#define is(_a, _b) (strcmp((_a), (_b)) == 0)
#define is_timer_subscribed(_t) (k_timer_remaining_get(_t))

#define th_seq(_x) ntohl((_x)->th_seq)
#define th_ack(_x) ntohl((_x)->th_ack)
#define ip_get(_x) ((struct net_ipv4_hdr *) net_pkt_ip_data((_x)))
#define ip6_get(_x) ((struct net_ipv6_hdr *) net_pkt_ip_data((_x)))

#define tcp_slist(_slist, _op, _type, _link)				\
({									\
	sys_snode_t *_node = sys_slist_##_op(_slist);			\
									\
	_type * _x = _node ? CONTAINER_OF(_node, _type, _link) : NULL;	\
									\
	_x;								\
})

#if IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)
#define tcp_malloc(_size) \
	tp_malloc(_size, tp_basename(__FILE__), __LINE__, __func__)
#define tcp_calloc(_nmemb, _size) \
	tp_calloc(_nmemb, _size, tp_basename(__FILE__), __LINE__, __func__)
#define tcp_free(_ptr) tp_free(_ptr, tp_basename(__FILE__), __LINE__, __func__)
#else
#define tcp_malloc(_size) k_malloc(_size)
#define tcp_calloc(_nmemb, _size) k_calloc(_nmemb, _size)
#define tcp_free(_ptr) k_free(_ptr)
#endif

#if IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)
#define tcp_nbuf_alloc(_pool, _len) \
	tp_nbuf_alloc(_pool, _len, tp_basename(__FILE__), __LINE__, __func__)
#define tcp_nbuf_unref(_nbuf) \
	tp_nbuf_unref(_nbuf, tp_basename(__FILE__), __LINE__, __func__)
#else
#define tcp_nbuf_alloc(_pool, _len) net_buf_alloc_len(_pool, _len, K_NO_WAIT)
#define tcp_nbuf_unref(_nbuf) net_buf_unref(_nbuf)
#endif

#if IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)
#define tcp_pkt_alloc(_len) tp_pkt_alloc(_len, tp_basename(__FILE__), __LINE__)
#define tcp_pkt_clone(_pkt) tp_pkt_clone(_pkt, tp_basename(__FILE__), __LINE__)
#define tcp_pkt_unref(_pkt) tp_pkt_unref(_pkt, tp_basename(__FILE__), __LINE__)
#else
static struct net_pkt *tcp_pkt_alloc(size_t len)
{
	struct net_pkt *pkt = net_pkt_alloc(K_NO_WAIT);

	pkt->family = AF_INET;

	NET_ASSERT(pkt);

	if (len) {
		struct net_buf *buf = net_pkt_get_frag(pkt, K_NO_WAIT);

		net_buf_add(buf, len);
		net_pkt_frag_insert(pkt, buf);
		NET_ASSERT(buf);
	}

	return pkt;
}
#define tcp_pkt_clone(_pkt) net_pkt_clone(_pkt, K_NO_WAIT)
#define tcp_pkt_unref(_pkt) net_pkt_unref(_pkt)
#endif
#define tcp_pkt_ref(_pkt) net_pkt_ref(_pkt)

#if IS_ENABLED(CONFIG_NET_TEST_PROTOCOL)
#define conn_seq(_conn, _req) \
	tp_seq_track(TP_SEQ, &(_conn)->seq, (_req), tp_basename(__FILE__), \
			__LINE__, __func__)
#define conn_ack(_conn, _req) \
	tp_seq_track(TP_ACK, &(_conn)->ack, (_req), tp_basename(__FILE__), \
			__LINE__, __func__)
#else
#define conn_seq(_conn, _req) (_conn)->seq += (_req)
#define conn_ack(_conn, _req) (_conn)->ack += (_req)
#endif

#define conn_state(_conn, _s)						\
({									\
	NET_DBG("%s->%s",						\
		tcp_state_to_str((_conn)->state, false),		\
		tcp_state_to_str((_s), false));				\
	(_conn)->state = _s;						\
})

#define TCPOPT_PAD	0
#define TCPOPT_NOP	1
#define TCPOPT_MAXSEG	2
#define TCPOPT_WINDOW	3

enum pkt_addr {
	SRC = 1,
	DST = 0
};

struct tcphdr {
	u16_t th_sport;
	u16_t th_dport;
	u32_t th_seq;
	u32_t th_ack;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	u8_t th_x2:4;	/* unused */
	u8_t th_off:4;	/* data offset */
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u8_t th_off:4;
	u8_t th_x2:4;
#endif
	u8_t th_flags;
	u16_t th_win;
	u16_t th_sum;
	u16_t th_urp;
};

enum th_flags {
	FIN = 1,
	SYN = 1 << 1,
	RST = 1 << 2,
	PSH = 1 << 3,
	ACK = 1 << 4,
	URG = 1 << 5,
};

enum tcp_state {
	TCP_LISTEN = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECEIVED,
	TCP_ESTABLISHED,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_CLOSE_WAIT,
	TCP_CLOSING,
	TCP_LAST_ACK,
	TCP_TIME_WAIT,
	TCP_CLOSED
};

struct tcp_win { /* TCP window */
	size_t len;
	sys_slist_t bufs;
};

union tcp_endpoint {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

struct tcp { /* TCP connection */
	sys_snode_t next;
	struct net_context *context;
	void *recv_user_data;
	enum tcp_state state;
	u32_t seq;
	u32_t ack;
	union tcp_endpoint *src;
	union tcp_endpoint *dst;
	u16_t win;
	struct tcp_win *rcv;
	struct tcp_win *snd;
	struct k_timer send_timer;
	sys_slist_t send_queue;
	bool in_retransmission;
	size_t send_retries;
	struct net_if *iface;
	net_tcp_accept_cb_t accept_cb;
	atomic_t ref_count;
};

#define _flags(_fl, _op, _mask, _cond)					\
({									\
	bool result = false;						\
									\
	if (*(_fl) && (_cond) && (*(_fl) _op (_mask))) {		\
		*(_fl) &= ~(_mask);					\
		result = true;						\
	}								\
									\
	result;								\
})

#define FL(_fl, _op, _mask, _args...)					\
	_flags(_fl, _op, _mask, strlen("" #_args) ? _args : true)
