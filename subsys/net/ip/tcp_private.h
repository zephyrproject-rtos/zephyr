/*
 * Copyright (c) 2018-2019 Intel Corporation
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tp.h"

#define is(_a, _b) (strcmp((_a), (_b)) == 0)

#ifndef MIN3
#define MIN3(_a, _b, _c) MIN((_a), MIN((_b), (_c)))
#endif

#define th_sport(_x) UNALIGNED_GET(&(_x)->th_sport)
#define th_dport(_x) UNALIGNED_GET(&(_x)->th_dport)
#define th_seq(_x) ntohl(UNALIGNED_GET(&(_x)->th_seq))
#define th_ack(_x) ntohl(UNALIGNED_GET(&(_x)->th_ack))
#define th_off(_x) ((_x)->th_off)
#define th_flags(_x) UNALIGNED_GET(&(_x)->th_flags)
#define th_win(_x) UNALIGNED_GET(&(_x)->th_win)

#define tcp_slist(_conn, _slist, _op, _type, _link)			\
({									\
	k_mutex_lock(&_conn->lock, K_FOREVER);				\
									\
	sys_snode_t *_node = sys_slist_##_op(_slist);			\
									\
	_type * _x = _node ? CONTAINER_OF(_node, _type, _link) : NULL;	\
									\
	k_mutex_unlock(&_conn->lock);					\
									\
	_x;								\
})

#if defined(CONFIG_NET_TEST_PROTOCOL)
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

#define TCP_PKT_ALLOC_TIMEOUT K_MSEC(CONFIG_NET_TCP_PKT_ALLOC_TIMEOUT)

#if defined(CONFIG_NET_TEST_PROTOCOL)
#define tcp_pkt_clone(_pkt) tp_pkt_clone(_pkt, tp_basename(__FILE__), __LINE__)
#define tcp_pkt_unref(_pkt) tp_pkt_unref(_pkt, tp_basename(__FILE__), __LINE__)
#else
#define tcp_pkt_clone(_pkt) net_pkt_clone(_pkt, TCP_PKT_ALLOC_TIMEOUT)
#define tcp_pkt_unref(_pkt) net_pkt_unref(_pkt)
#define tp_pkt_alloc(args...)
#endif

#define tcp_pkt_ref(_pkt) net_pkt_ref(_pkt)
#define tcp_pkt_alloc(_conn, _len)					\
({									\
	struct net_pkt *_pkt;						\
									\
	if ((_len) > 0) {						\
		_pkt = net_pkt_alloc_with_buffer(			\
			(_conn)->iface,					\
			(_len),						\
			net_context_get_family((_conn)->context),	\
			IPPROTO_TCP,					\
			TCP_PKT_ALLOC_TIMEOUT);				\
	} else {							\
		_pkt = net_pkt_alloc(TCP_PKT_ALLOC_TIMEOUT);		\
	}								\
									\
	tp_pkt_alloc(_pkt, tp_basename(__FILE__), __LINE__);		\
									\
	_pkt;								\
})

#define tcp_rx_pkt_alloc(_conn, _len)					\
({									\
	struct net_pkt *_pkt;						\
									\
	if ((_len) > 0) {						\
		_pkt = net_pkt_rx_alloc_with_buffer(			\
			(_conn)->iface,					\
			(_len),						\
			net_context_get_family((_conn)->context),	\
			IPPROTO_TCP,					\
			TCP_PKT_ALLOC_TIMEOUT);				\
	} else {							\
		_pkt = net_pkt_rx_alloc(TCP_PKT_ALLOC_TIMEOUT);		\
	}								\
									\
	tp_pkt_alloc(_pkt, tp_basename(__FILE__), __LINE__);		\
									\
	_pkt;								\
})

#define tcp_pkt_alloc_no_conn(_iface, _family, _len)			\
({									\
	struct net_pkt *_pkt;						\
									\
	if ((_len) > 0) {						\
		_pkt = net_pkt_alloc_with_buffer(			\
			(_iface), (_len), (_family),			\
			IPPROTO_TCP,					\
			TCP_PKT_ALLOC_TIMEOUT);				\
	} else {							\
		_pkt = net_pkt_alloc(TCP_PKT_ALLOC_TIMEOUT);		\
	}								\
									\
	tp_pkt_alloc(_pkt, tp_basename(__FILE__), __LINE__);		\
									\
	_pkt;								\
})

#if defined(CONFIG_NET_TEST_PROTOCOL)
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

#define NET_TCP_DEFAULT_MSS 536

#define conn_mss(_conn)							\
	MIN((_conn)->recv_options.mss_found ? (_conn)->recv_options.mss	\
					    : NET_TCP_DEFAULT_MSS,	\
	    net_tcp_get_supported_mss(_conn))

#define conn_state(_conn, _s)						\
({									\
	NET_DBG("%s->%s",						\
		tcp_state_to_str((_conn)->state, false),		\
		tcp_state_to_str((_s), false));				\
	(_conn)->state = _s;						\
})

#define conn_send_data_dump(_conn)                                             \
	({                                                                     \
		NET_DBG("conn: %p total=%zd, unacked_len=%d, "                 \
			"send_win=%hu, mss=%hu",                               \
			(_conn), net_pkt_get_len((_conn)->send_data),          \
			_conn->unacked_len, _conn->send_win,                   \
			(uint16_t)conn_mss((_conn)));                          \
		NET_DBG("conn: %p send_data_timer=%hu, send_data_retries=%hu", \
			(_conn),                                               \
			(bool)k_ticks_to_ms_ceil32(                            \
				k_work_delayable_remaining_get(                \
					&(_conn)->send_data_timer)),           \
			(_conn)->send_data_retries);                           \
	})

enum pkt_addr {
	TCP_EP_SRC = 1,
	TCP_EP_DST = 0
};

struct tcphdr {
	uint16_t th_sport;
	uint16_t th_dport;
	uint32_t th_seq;
	uint32_t th_ack;
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t th_x2:4;	/* unused */
	uint8_t th_off:4;	/* data offset, in units of 32-bit words */
#else
	uint8_t th_off:4;
	uint8_t th_x2:4;
#endif
	uint8_t th_flags;
	uint16_t th_win;
	uint16_t th_sum;
	uint16_t th_urp;
} __packed;

enum th_flags {
	FIN = BIT(0),
	SYN = BIT(1),
	RST = BIT(2),
	PSH = BIT(3),
	ACK = BIT(4),
	URG = BIT(5),
	ECN = BIT(6),
	CWR = BIT(7),
};

struct tcp_mss_option {
	uint32_t option;
};

enum tcp_state {
	TCP_UNUSED = 0,
	TCP_LISTEN,
	TCP_SYN_SENT,
	TCP_SYN_RECEIVED,
	TCP_ESTABLISHED,
	TCP_FIN_WAIT_1,
	TCP_FIN_WAIT_2,
	TCP_CLOSE_WAIT,
	TCP_CLOSING,
	TCP_LAST_ACK,
	TCP_TIME_WAIT,
	TCP_CLOSED
};

enum tcp_data_mode {
	TCP_DATA_MODE_SEND = 0,
	TCP_DATA_MODE_RESEND = 1
};

union tcp_endpoint {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

/* TCP Option codes */
#define NET_TCP_END_OPT          0
#define NET_TCP_NOP_OPT          1
#define NET_TCP_MSS_OPT          2
#define NET_TCP_WINDOW_SCALE_OPT 3

/* TCP Option sizes */
#define NET_TCP_END_SIZE          1
#define NET_TCP_NOP_SIZE          1
#define NET_TCP_MSS_SIZE          4
#define NET_TCP_WINDOW_SCALE_SIZE 3

struct tcp_options {
	uint16_t mss;
	uint16_t window;
	bool mss_found : 1;
	bool wnd_found : 1;
};

#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE

struct tcp_collision_avoidance_reno {
	uint16_t cwnd;
	uint16_t ssthresh;
	uint16_t pending_fast_retransmit_bytes;
};
#endif

struct tcp;
typedef void (*net_tcp_closed_cb_t)(struct tcp *conn, void *user_data);

struct tcp { /* TCP connection */
	sys_snode_t next;
	struct net_context *context;
	struct net_pkt *send_data;
	struct net_pkt *queue_recv_data;
	struct net_if *iface;
	void *recv_user_data;
	sys_slist_t send_queue;
	union {
		net_tcp_accept_cb_t accept_cb;
		struct tcp *accepted_conn;
	};
	net_context_connect_cb_t connect_cb;
#if defined(CONFIG_NET_TEST)
	net_tcp_closed_cb_t test_closed_cb;
	void *test_user_data;
#endif
	struct k_mutex lock;
	struct k_sem connect_sem; /* semaphore for blocking connect */
	struct k_sem tx_sem; /* Semaphore indicating if transfers are blocked . */
	struct k_fifo recv_data;  /* temp queue before passing data to app */
	struct tcp_options recv_options;
	struct tcp_options send_options;
	struct k_work_delayable send_timer;
	struct k_work_delayable recv_queue_timer;
	struct k_work_delayable send_data_timer;
	struct k_work_delayable timewait_timer;
	struct k_work_delayable persist_timer;
	struct k_work_delayable ack_timer;
#if defined(CONFIG_NET_TCP_KEEPALIVE)
	struct k_work_delayable keepalive_timer;
#endif /* CONFIG_NET_TCP_KEEPALIVE */
	struct k_work conn_release;

	union {
		/* Because FIN and establish timers are never happening
		 * at the same time, share the timer between them to
		 * save memory.
		 */
		struct k_work_delayable fin_timer;
		struct k_work_delayable establish_timer;
	};
	union tcp_endpoint src;
	union tcp_endpoint dst;
#if defined(CONFIG_NET_TCP_IPV6_ND_REACHABILITY_HINT)
	int64_t last_nd_hint_time;
#endif
	size_t send_data_total;
	size_t send_retries;
	int unacked_len;
	atomic_t ref_count;
	enum tcp_state state;
	enum tcp_data_mode data_mode;
	uint32_t seq;
	uint32_t ack;
#if defined(CONFIG_NET_TCP_KEEPALIVE)
	uint32_t keep_idle;
	uint32_t keep_intvl;
	uint32_t keep_cnt;
	uint32_t keep_cur;
#endif /* CONFIG_NET_TCP_KEEPALIVE */
	uint16_t recv_win_sent;
	uint16_t recv_win_max;
	uint16_t recv_win;
	uint16_t send_win_max;
	uint16_t send_win;
#ifdef CONFIG_NET_TCP_RANDOMIZED_RTO
	uint16_t rto;
#endif
#ifdef CONFIG_NET_TCP_CONGESTION_AVOIDANCE
	struct tcp_collision_avoidance_reno ca;
#endif
	uint8_t send_data_retries;
#ifdef CONFIG_NET_TCP_FAST_RETRANSMIT
	uint8_t dup_ack_cnt;
#endif
	uint8_t zwp_retries;
	bool in_retransmission : 1;
	bool in_connect : 1;
	bool in_close : 1;
#if defined(CONFIG_NET_TCP_KEEPALIVE)
	bool keep_alive : 1;
#endif /* CONFIG_NET_TCP_KEEPALIVE */
	bool tcp_nodelay : 1;
	bool addr_ref_done : 1;
	bool rst_received : 1;
};

#define _flags(_fl, _op, _mask, _cond)					\
({									\
	bool result = false;						\
									\
	if (UNALIGNED_GET(_fl) && (_cond) &&				\
	    (UNALIGNED_GET(_fl) _op(_mask))) {				\
		UNALIGNED_PUT(UNALIGNED_GET(_fl) & ~(_mask), _fl);	\
		result = true;						\
	}								\
									\
	result;								\
})

#define FL(_fl, _op, _mask, _args...)					\
	_flags(_fl, _op, _mask, sizeof(#_args) > 1 ? _args : true)

typedef void (*net_tcp_cb_t)(struct tcp *conn, void *user_data);

#if defined(CONFIG_NET_TEST)
void tcp_install_close_cb(struct net_context *ctx,
			  net_tcp_closed_cb_t cb,
			  void *user_data);
#endif
