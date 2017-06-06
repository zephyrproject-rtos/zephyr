/** @file
 @brief TCP data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TCP_H
#define __TCP_H

#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
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

/** A retransmitted packet has been sent and not yet ack'd */
#define NET_TCP_RETRYING BIT(4)

/** MSS option has been set already */
#define NET_TCP_RECV_MSS_SET BIT(5)

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

#define NET_TCP_FLAGS(net_pkt) (NET_TCP_HDR(net_pkt)->flags & NET_TCP_CTL)

/* TCP max window size */
#define NET_TCP_MAX_WIN   (4 * 1024)

/* Maximal value of the sequence number */
#define NET_TCP_MAX_SEQ   0xffffffff

#define NET_TCP_MAX_OPT_SIZE  8

#define NET_TCP_MSS_HEADER    0x02040000 /* MSS option */
#define NET_TCP_WINDOW_HEADER 0x30300    /* Window scale option */

#define NET_TCP_MSS_SIZE      4          /* MSS option size */
#define NET_TCP_WINDOW_SIZE   3          /* Window scale option size */

/* Max received bytes to buffer internally */
#define NET_TCP_BUF_MAX_LEN 1280

/* Max segment lifetime, in seconds */
#define NET_TCP_MAX_SEG_LIFETIME 60

struct net_context;

struct net_tcp {
	/** Network context back pointer. */
	struct net_context *context;

	/** Cookie pointer passed to net_context_recv() */
	void *recv_user_data;

	/** ACK message timer */
	struct k_delayed_work ack_timer;

	/** Retransmit timer */
	struct k_timer retry_timer;

	/** List pointer used for TCP retransmit buffering */
	sys_slist_t sent_list;

	/** Max acknowledgment. */
	u32_t recv_max_ack;

	/** Current sequence number. */
	u32_t send_seq;

	/** Acknowledgment number to send in next packet. */
	u32_t send_ack;

	/** Last ACK value sent */
	u32_t sent_ack;

	/** Current retransmit period */
	u32_t retry_timeout_shift : 5;
	/** Flags for the TCP */
	u32_t flags : 8;
	/** Current TCP state */
	u32_t state : 4;
	/* An outbound FIN packet has been sent */
	u32_t fin_sent : 1;
	/* An inbound FIN packet has been received */
	u32_t fin_rcvd : 1;
	/* Tells if ack timer has been already cancelled. It might happen
	 * that the timer is executed even if it is cancelled, this is because
	 * of various timing issues when timer is scheduled to run.
	 */
	u32_t ack_timer_cancelled : 1;
	/** Remaining bits in this u32_t */
	u32_t _padding : 12;

	/** Accept callback to be called when the connection has been
	 * established.
	 */
	net_tcp_accept_cb_t accept_cb;

	/**
	 * Semaphore to signal TCP connection completion
	 */
	struct k_sem connect_wait;
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
				   u16_t remote_port,
				   u16_t local_port,
				   net_conn_cb_t cb,
				   void *user_data,
				   struct net_conn_handle **handle)
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
static inline int net_tcp_unregister(struct net_conn_handle *handle)
{
	return net_conn_unregister(handle);
}

const char * const net_tcp_state_str(enum net_tcp_state state);

#if defined(CONFIG_NET_TCP)
void net_tcp_change_state(struct net_tcp *tcp, enum net_tcp_state new_state);
#else
#define net_tcp_change_state(...)
#endif

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
 * @param local Source address, or NULL to use the local address of
 *        the TCP context
 * @param remote Peer address
 * @param send_pkt Full IP + TCP header that is to be sent.
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_segment(struct net_tcp *tcp, u8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr_ptr *local,
			    const struct sockaddr *remote,
			    struct net_pkt **send_pkt);

/**
 * @brief Prepare a TCP ACK message that can be send to peer.
 *
 * @param tcp TCP context
 * @param remote Peer address
 * @param pkt Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_pkt **pkt);

/**
 * @brief Prepare a TCP RST message that can be send to peer.
 *
 * @param tcp TCP context
 * @param remote Peer address
 * @param pkt Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_prepare_reset(struct net_tcp *tcp, const struct sockaddr *remote,
			  struct net_pkt **pkt);

typedef void (*net_tcp_cb_t)(struct net_tcp *tcp, void *user_data);

/**
 * @brief Go through all the TCP connections and call callback
 * for each of them.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_tcp_foreach(net_tcp_cb_t cb, void *user_data);

/**
 * @brief Send available queued data over TCP connection
 *
 * @param context TCP context
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_send_data(struct net_context *context);

/**
 * @brief Enqueue a single packet for transmission
 *
 * @param context TCP context
 * @param pkt Packet
 *
 * @return 0 if ok, < 0 if error
 */
int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt);

/**
 * @brief Sends one TCP packet initialized with the _prepare_*()
 *        family of functions.
 *
 * @param pkt Packet
 */
int net_tcp_send_pkt(struct net_pkt *pkt);

/**
 * @brief Handle a received TCP ACK
 *
 * @param cts Context
 * @param seq Received ACK sequence number
 */
void net_tcp_ack_received(struct net_context *ctx, u32_t ack);

/**
 * @brief Calculates and returns the MSS for a given TCP context
 *
 * @param tcp TCP context
 *
 * @return Maximum Segment Size
 */
u16_t net_tcp_get_recv_mss(const struct net_tcp *tcp);

/**
 * @brief Obtains the state for a TCP context
 *
 * @param tcp TCP context
 */
static inline enum net_tcp_state net_tcp_get_state(const struct net_tcp *tcp)
{
	return (enum net_tcp_state)tcp->state;
}

/**
 * @brief Check if the sequence number is valid i.e., it is inside the window.
 *
 * @param tcp TCP context
 * @param pkt Network packet
 *
 * @return true if network packet sequence number is valid, false otherwise
 */
bool net_tcp_validate_seq(struct net_tcp *tcp, struct net_pkt *pkt);

#if defined(CONFIG_NET_TCP)
void net_tcp_init(void);
#else
#define net_tcp_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TCP_H */
