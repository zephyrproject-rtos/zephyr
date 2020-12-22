/*
 * Copyright (c) 2018-2019 Intel Corporation
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

#define TCP_PKT_ALLOC_TIMEOUT K_MSEC(100)

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

#define conn_mss(_conn)					\
	((_conn)->recv_options.mss_found ?		\
	 (_conn)->recv_options.mss : (uint16_t)NET_IPV6_MTU)

#define conn_state(_conn, _s)						\
({									\
	NET_DBG("%s->%s",						\
		tcp_state_to_str((_conn)->state, false),		\
		tcp_state_to_str((_s), false));				\
	(_conn)->state = _s;						\
})

#define conn_send_data_dump(_conn)					\
({									\
	NET_DBG("conn: %p total=%zd, unacked_len=%d, "			\
		"send_win=%hu, mss=%hu",				\
		(_conn), net_pkt_get_len((_conn)->send_data),		\
		conn->unacked_len, conn->send_win,			\
		(uint16_t)conn_mss((_conn)));				\
	NET_DBG("conn: %p send_data_timer=%hu, send_data_retries=%hu",	\
		(_conn),						\
		(bool)k_delayed_work_remaining_get(&(_conn)->send_data_timer),\
		(_conn)->send_data_retries);				\
})

#define TCPOPT_END	0
#define TCPOPT_NOP	1
#define TCPOPT_MAXSEG	2
#define TCPOPT_WINDOW	3

enum pkt_addr {
	TCP_EP_SRC = 1,
	TCP_EP_DST = 0
};

struct tcphdr {
	uint16_t th_sport;
	uint16_t th_dport;
	uint32_t th_seq;
	uint32_t th_ack;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t th_x2:4;	/* unused */
	uint8_t th_off:4;	/* data offset */
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t th_off:4;
	uint8_t th_x2:4;
#endif
	uint8_t th_flags;
	uint16_t th_win;
	uint16_t th_sum;
	uint16_t th_urp;
} __packed;

enum th_flags {
	FIN = 1,
	SYN = 1 << 1,
	RST = 1 << 2,
	PSH = 1 << 3,
	ACK = 1 << 4,
	URG = 1 << 5,
	ECN = 1 << 6,
	CWR = 1 << 7,
};

enum tcp_state {
	TCP_LISTEN = 1,
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

struct tcp_options {
	uint16_t mss;
	uint16_t window;
	bool mss_found : 1;
	bool wnd_found : 1;
};

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
	struct k_mutex lock;
	struct k_sem connect_sem; /* semaphore for blocking connect */
	struct k_fifo recv_data;  /* temp queue before passing data to app */
	struct tcp_options recv_options;
	struct k_delayed_work send_timer;
	struct k_delayed_work recv_queue_timer;
	struct k_delayed_work send_data_timer;
	struct k_delayed_work timewait_timer;
	struct k_delayed_work fin_timer;
	union tcp_endpoint src;
	union tcp_endpoint dst;
	size_t send_data_total;
	size_t send_retries;
	int unacked_len;
	atomic_t ref_count;
	enum tcp_state state;
	enum tcp_data_mode data_mode;
	uint32_t seq;
	uint32_t ack;
	uint16_t recv_win;
	uint16_t send_win;
	uint8_t send_data_retries;
	bool in_retransmission : 1;
	bool in_connect : 1;
	bool in_close : 1;
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
	_flags(_fl, _op, _mask, strlen("" #_args) ? _args : true)

typedef void (*net_tcp_cb_t)(struct tcp *conn, void *user_data);
