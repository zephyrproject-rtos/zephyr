/** @file
 @brief TCP data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#ifndef __TCP_H
#define __TCP_H

#include <stdint.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Is this TCP context/socket used or not */
#define NET_TCP_IN_USE BIT(0)

/** Is the final segment sent */
#define NET_TCP_FINAL_SENT BIT(1)

/** Is the final segment received */
#define NET_TCP_FINAL_RECV BIT(2)

/** Is the socket shutdown for read/write */
#define NET_TCP_IS_SHUTDOWN BIT(3)

/*
 * TCP connection states
 */
enum net_tcp_state {
	NET_TCP_CLOSED = 0,
	NET_TCP_LISTEN,
	NET_TCP_SYN_SENT,
	NET_TCP_SYN_RCVD,
	NET_TCP_ESTABLISHED,
	NET_TCP_CLOSE_WAIT,
	NET_TCP_LAST_ACK,
	NET_TCP_FIN_WAIT_1,
	NET_TCP_FIN_WAIT_2,
	NET_TCP_TIME_WAIT,
	NET_TCP_CLOSING,
};

/* TCP packet types */
#define NET_TCP_FIN 0x01
#define NET_TCP_SYN 0x02
#define NET_TCP_RST 0x04
#define NET_TCP_PSH 0x08
#define NET_TCP_ACK 0x10
#define NET_TCP_URG 0x20
#define NET_TCP_CTL 0x3f

/* TCP max window size */
#define NET_TCP_MAX_WIN   (4 * 1024)

/* Maximal value of the sequence number */
#define NET_TCP_MAX_SEQ   0xffffffff

#define NET_TCP_MAX_OPT_SIZE  8

#define NET_TCP_MSS_HEADER    0x02040000 /* MSS option */
#define NET_TCP_WINDOW_HEADER 0x30300    /* Window scale option */

#define NET_TCP_MSS_SIZE      4          /* MSS option size */
#define NET_TCP_WINDOW_SIZE   3          /* Window scale option size */

struct net_context;

struct net_tcp {
	/** Network context back pointer. */
	struct net_context *context;

	/** TCP state. */
	enum net_tcp_state state;

	/** Previous TCP state. */
	enum net_tcp_state prev_state;

	/** Retransmission timer. */
	struct nano_delayed_work retransmit_timer;

	/** ACK message timer */
	struct nano_delayed_work ack_timer;

	/** Temp buf for received data. The first element of the fragment list
	 * does not contain user data part.
	 */
	struct net_buf *recv;

	/** Temp buf for data to be sent. The first element of the fragment
	 * list does not contain user data part.
	 */
	struct net_buf *send;

	/** Max buffer length */
	uint32_t buf_max_len;

	/** Highest acknowledged number of sent segments. */
	uint32_t recv_ack;

	/** Max acknowledgment. */
	uint32_t recv_max_ack;

	/** Receive window. */
	uint32_t recv_wnd;

	/** Current sequence number. */
	uint32_t send_seq;

	/** Acknowledgment number. */
	uint32_t send_ack;

	/** Send window. */
	uint32_t send_wnd;

	/** Max window of another side. */
	uint32_t send_max_wnd;

	/** Congestion window. */
	uint32_t send_cwnd;

	/** Counter of the send_cwnd parts. */
	uint32_t send_pcount;

	/** Slow start threshold. */
	uint32_t send_ss_threshold;

	/** Max RX segment size (MSS). */
	uint16_t recv_mss;

	/** Max TX segment size (MSS). */
	uint16_t send_mss;

	/** Scale of the send window. */
	uint8_t send_scale;

	/** Scale of the receive window. */
	uint8_t recv_scale;

	/** Flags for the TCP. */
	uint8_t flags;
};

static inline bool net_tcp_is_used(struct net_tcp *tcp)
{
	NET_ASSERT(tcp);

	return tcp->flags & NET_TCP_IN_USE;
}

/**
 * @brief Register a callback to be called when TCP packet
 * is received corresponding to received packet.
 *
 * @param remote_addr Remote address of the connection end point.
 * @param local_addr Local address of the connection end point.
 * @param remote_port Remote port of the connection end point.
 * @param local_port Local port of the connection end point.
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param handle TCP handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
static inline int net_tcp_register(const struct sockaddr *remote_addr,
				   const struct sockaddr *local_addr,
				   uint16_t remote_port,
				   uint16_t local_port,
				   net_conn_cb_t cb,
				   void *user_data,
				   void **handle)
{
	return net_conn_register(IPPROTO_TCP, remote_addr, local_addr,
				 remote_port, local_port, cb, user_data,
				 handle);
}

/**
 * @brief Unregister TCP handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
static inline int net_tcp_unregister(void *handle)
{
	return net_conn_unregister(handle);
}

const char const *net_tcp_state_str(enum net_tcp_state state);

static inline void net_tcp_change_state(struct net_tcp *tcp,
					enum net_tcp_state new_state)
{
#if defined(CONFIG_NET_TCP)
	NET_ASSERT(tcp);

	if (tcp->state == new_state) {
		return;
	}

	NET_ASSERT(new_state >= NET_TCP_CLOSED &&
		   new_state <= NET_TCP_CLOSING);

	NET_DBG("%s (%d) => %s (%d)",
		net_tcp_state_str(tcp->state), tcp->state,
		net_tcp_state_str(new_state), new_state);

	tcp->prev_state = tcp->state;
	tcp->state = new_state;

	if (tcp->state != NET_TCP_CLOSED) {
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

	if (tcp->context->accept_cb) {
		tcp->context->accept_cb(tcp->context,
					&tcp->context->remote,
					sizeof(struct sockaddr),
					-ENETRESET,
					tcp->context->user_data);
	}
#endif /* CONFIG_NET_TCP */
}

/**
 * @brief Allocate TCP connection context.
 *
 * @param context Pointer to net_context that is tied to this TCP
 * context.
 *
 * @return Pointer TCP connection context. NULL if no available
 * context can be found.
 */
struct net_tcp *net_tcp_alloc(struct net_context *context);

/**
 * @brief Release TCP connection context.
 *
 * @param tcp Pointer to net_tcp context.
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_release(struct net_tcp *tcp);

/**
 * @brief Send a TCP segment without any data. The returned buffer
 * is a ready made packet that can be sent via net_send_data()
 * function.
 *
 * @param tcp TCP context
 * @param flags TCP flags
 * @param options Pointer TCP options, NULL if no options.
 * @param optlen Length of the options.
 * @param remote Peer address
 * @param send_buf Full IP + TCP header that is to be sent.
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_segment(struct net_tcp *tcp, uint8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr *remote,
			    struct net_buf **send_buf);

/**
 * @brief Prepare a TCP segment with data. The returned buffer
 * is a ready made packet that can be sent via net_send_data()
 * function.
 *
 * @param tcp TCP context
 * @param buf Data fragments to send. This should have been allocated
 * by net_nbuf_get_tx() or net_nbuf_get_reserve_tx() function.
 * @param options Pointer TCP options, NULL if no options.
 * @param optlen Length of the options.
 * @param remote Peer address
 * @param send_buf Full IP + TCP header + data that is to be sent.
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_data_segment(struct net_tcp *tcp, struct net_buf *buf,
				 void *options, size_t optlen,
				 const struct sockaddr *remote,
				 struct net_buf **send_buf);

/**
 * @brief Prepare a TCP ACK message that can be send to peer.
 *
 * @param tcp TCP context
 * @param remote Peer address
 * @param buf Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_buf **buf);

/**
 * @brief Prepare a TCP RST message that can be send to peer.
 *
 * @param tcp TCP context
 * @param remote Peer address
 * @param buf Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_reset(struct net_tcp *tcp, const struct sockaddr *remote,
			  struct net_buf **buf);

#if defined(CONFIG_NET_TCP)
void net_tcp_init(void);
#else
#define net_tcp_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TCP_H */
