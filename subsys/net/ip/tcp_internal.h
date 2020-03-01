/** @file
 @brief TCP data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TCP_INTERNAL_H
#define __TCP_INTERNAL_H

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

/* BIT(1), BIT(2) are unused and available */

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

#define NET_TCP_FLAGS(hdr) (hdr->flags & NET_TCP_CTL)

/* Length of TCP header, including options */
/* "offset": 4-bit field in high nibble, units of dwords */
#define NET_TCP_HDR_LEN(hdr) (4 * ((hdr)->offset >> 4))

/* RFC 1122 4.2.2.6 "If an MSS option is not received at connection
 * setup, TCP MUST assume a default send MSS of 536"
 */
#define NET_TCP_DEFAULT_MSS   536

/* TCP max window size */
#define NET_TCP_MAX_WIN   (4 * 1024)

/* Maximal value of the sequence number */
#define NET_TCP_MAX_SEQ   0xffffffff

#define NET_TCP_MAX_OPT_SIZE  8

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

/** Parsed TCP option values for net_tcp_parse_opts()  */
struct net_tcp_options {
	u16_t mss;
};

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

	/** Timer for doing active close in case the peer FIN is lost. */
	struct k_delayed_work fin_timer;

	/** Retransmit timer */
	struct k_delayed_work retry_timer;

	/** TIME_WAIT timer */
	struct k_delayed_work timewait_timer;

	/** List pointer used for TCP retransmit buffering */
	sys_slist_t sent_list;

	/** Current sequence number. */
	u32_t send_seq;

	/** Acknowledgment number to send in next packet. */
	u32_t send_ack;

	/** Last ACK value sent */
	u32_t sent_ack;

	/** Accept callback to be called when the connection has been
	 * established.
	 */
	net_tcp_accept_cb_t accept_cb;

	/**
	 * Semaphore to signal TCP connection completion
	 */
	struct k_sem connect_wait;

	/**
	 * Current TCP receive window for our side
	 */
	u16_t recv_wnd;

	/**
	 * Send MSS for the peer
	 */
	u16_t send_mss;

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
	/** Remaining bits in this u32_t */
	u32_t _padding : 13;
};

typedef void (*net_tcp_cb_t)(struct net_tcp *tcp, void *user_data);

static inline bool net_tcp_is_used(struct net_tcp *tcp)
{
	NET_ASSERT(tcp);

	return tcp->flags & NET_TCP_IN_USE;
}

/**
 * @brief Register a callback to be called when TCP packet
 * is received corresponding to received packet.
 *
 * @param family Protocol family
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
static inline int net_tcp_register(u8_t family,
				   const struct sockaddr *remote_addr,
				   const struct sockaddr *local_addr,
				   u16_t remote_port,
				   u16_t local_port,
				   net_conn_cb_t cb,
				   void *user_data,
				   struct net_conn_handle **handle)
{
	return net_conn_register(IPPROTO_TCP, family, remote_addr, local_addr,
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

/*
 * @brief Generate initial TCP sequence number
 *
 * @return Return a random TCP sequence number
 */
static inline u32_t tcp_init_isn(void)
{
	/* Randomise initial seq number */
	return sys_rand32_get();
}

const char *net_tcp_state_str(enum net_tcp_state state);

#if defined(CONFIG_NET_NATIVE_TCP)
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
#if defined(CONFIG_NET_NATIVE_TCP)
struct net_tcp *net_tcp_alloc(struct net_context *context);
#else
static inline struct net_tcp *net_tcp_alloc(struct net_context *context)
{
	ARG_UNUSED(context);
	return NULL;
}
#endif

/**
 * @brief Release TCP connection context.
 *
 * @param tcp Pointer to net_tcp context.
 *
 * @return 0 if ok, < 0 if error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_release(struct net_tcp *tcp);
#else
static inline int net_tcp_release(struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);
	return 0;
}
#endif

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
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_prepare_segment(struct net_tcp *tcp, u8_t flags,
			    void *options, size_t optlen,
			    const struct sockaddr_ptr *local,
			    const struct sockaddr *remote,
			    struct net_pkt **send_pkt);
#else
static inline int net_tcp_prepare_segment(struct net_tcp *tcp, u8_t flags,
					  void *options, size_t optlen,
					  const struct sockaddr_ptr *local,
					  const struct sockaddr *remote,
					  struct net_pkt **send_pkt)
{
	ARG_UNUSED(tcp);
	ARG_UNUSED(flags);
	ARG_UNUSED(options);
	ARG_UNUSED(optlen);
	ARG_UNUSED(local);
	ARG_UNUSED(remote);
	ARG_UNUSED(send_pkt);
	return 0;
}
#endif

/**
 * @brief Prepare a TCP ACK message that can be send to peer.
 *
 * @param tcp TCP context
 * @param remote Peer address
 * @param pkt Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_prepare_ack(struct net_tcp *tcp, const struct sockaddr *remote,
			struct net_pkt **pkt);
#else
static inline int net_tcp_prepare_ack(struct net_tcp *tcp,
				      const struct sockaddr *remote,
				      struct net_pkt **pkt)
{
	ARG_UNUSED(tcp);
	ARG_UNUSED(remote);
	ARG_UNUSED(pkt);
	return 0;
}
#endif

/**
 * @brief Prepare a TCP RST message that can be send to peer.
 *
 * @param tcp TCP context
 * @param local Source address
 * @param remote Peer address
 * @param pkt Network buffer
 *
 * @return 0 if ok, < 0 if error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_prepare_reset(struct net_tcp *tcp,
			  const struct sockaddr *local,
			  const struct sockaddr *remote,
			  struct net_pkt **pkt);
#else
static inline int net_tcp_prepare_reset(struct net_tcp *tcp,
					const struct sockaddr *remote,
					struct net_pkt **pkt)
{
	ARG_UNUSED(tcp);
	ARG_UNUSED(remote);
	ARG_UNUSED(pkt);
	return 0;
}
#endif

/**
 * @brief Go through all the TCP connections and call callback
 * for each of them.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
#if defined(CONFIG_NET_NATIVE_TCP)
void net_tcp_foreach(net_tcp_cb_t cb, void *user_data);
#else
static inline void net_tcp_foreach(net_tcp_cb_t cb, void *user_data)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
}
#endif

/**
 * @brief Send available queued data over TCP connection
 *
 * @param context TCP context
 * @param cb TCP callback function
 * @param user_data User specified data
 *
 * @return 0 if ok, < 0 if error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_send_data(struct net_context *context, net_context_send_cb_t cb,
		      void *user_data);
#else
static inline int net_tcp_send_data(struct net_context *context,
				    net_context_send_cb_t cb,
				    void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return 0;
}
#endif

/**
 * @brief Enqueue a single packet for transmission
 *
 * @param context TCP context
 * @param pkt Packet
 *
 * @return 0 if ok, < 0 if error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_queue_data(struct net_context *context, struct net_pkt *pkt);
#else
static inline int net_tcp_queue_data(struct net_context *context,
				     struct net_pkt *pkt)
{
	ARG_UNUSED(context);
	ARG_UNUSED(pkt);
	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Sends one TCP packet initialized with the _prepare_*()
 *        family of functions.
 *
 * @param pkt Packet
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_send_pkt(struct net_pkt *pkt);
#else
static inline int net_tcp_send_pkt(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);
	return 0;
}
#endif

/**
 * @brief Handle a received TCP ACK
 *
 * @param cts Context
 * @param seq Received ACK sequence number
 * @return False if ACK sequence number is invalid, true otherwise
 */
#if defined(CONFIG_NET_NATIVE_TCP)
bool net_tcp_ack_received(struct net_context *ctx, u32_t ack);
#else
static inline bool net_tcp_ack_received(struct net_context *ctx, u32_t ack)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(ack);
	return false;
}
#endif

/**
 * @brief Calculates and returns the MSS for a given TCP context
 *
 * @param tcp TCP context
 *
 * @return Maximum Segment Size
 */
#if defined(CONFIG_NET_NATIVE_TCP)
u16_t net_tcp_get_recv_mss(const struct net_tcp *tcp);
#else
static inline u16_t net_tcp_get_recv_mss(const struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);
	return 0;
}
#endif

/**
 * @brief Returns the receive window for a given TCP context
 *
 * @param tcp TCP context
 *
 * @return Current TCP receive window
 */
#if defined(CONFIG_NET_NATIVE_TCP)
u32_t net_tcp_get_recv_wnd(const struct net_tcp *tcp);
#else
static inline u32_t net_tcp_get_recv_wnd(const struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);
	return 0;
}
#endif

/**
 * @brief Obtains the state for a TCP context
 *
 * @param tcp TCP context
 */
#if defined(CONFIG_NET_NATIVE_TCP)
static inline enum net_tcp_state net_tcp_get_state(const struct net_tcp *tcp)
{
	return (enum net_tcp_state)tcp->state;
}
#else
static inline enum net_tcp_state net_tcp_get_state(const struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);
	return NET_TCP_CLOSED;
}
#endif

/**
 * @brief Check if the sequence number is valid i.e., it is inside the window.
 *
 * @param tcp TCP context
 * @param tcp_hdr TCP header pointer
 *
 * @return true if network packet sequence number is valid, false otherwise
 */
#if defined(CONFIG_NET_NATIVE_TCP)
bool net_tcp_validate_seq(struct net_tcp *tcp, struct net_tcp_hdr *tcp_hdr);
#else
static inline bool net_tcp_validate_seq(struct net_tcp *tcp,
					struct net_tcp_hdr *tcp_hdr)
{
	ARG_UNUSED(tcp);
	ARG_UNUSED(tcp_hdr);
	return false;
}
#endif

/**
 * @brief Finalize TCP packet
 *
 * @param pkt Network packet
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_finalize(struct net_pkt *pkt);
#else
static inline int net_tcp_finalize(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);
	return 0;
}
#endif

/**
 * @brief Parse TCP options from network packet.
 *
 * Parse TCP options, returning MSS value (as that the only one we
 * handle so far).
 *
 * @param pkt Network packet
 * @param opt_totlen Total length of options to parse
 * @param opts Pointer to TCP options structure. (Each option is updated
 * only if present, so the structure must be initialized with the default
 * values.)
 *
 * @return 0 if no error, <0 in case of error
 */
int net_tcp_parse_opts(struct net_pkt *pkt, int opt_totlen,
		       struct net_tcp_options *opts);

/**
 * @brief TCP receive function
 *
 * @param context Network context
 * @param cb TCP receive callback function
 * @param user_data TCP receive callback user data
 *
 * @return 0 if no erro, < 0 in case of error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data);
#else
static inline int net_tcp_recv(struct net_context *context,
			       net_context_recv_cb_t cb, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -EPROTOTYPE;
}
#endif

/**
 * @brief Queue a TCP FIN packet if needed to close the socket
 *
 * @param context Network context
 *
 * @return 0 on success where a TCP FIN packet has been queueed, -ENOTCONN
 *         in case the socket was not connected or listening, -EOPNOTSUPP
 *         in case it was not a TCP socket or -EPROTONOSUPPORT if TCP is not
 *         supported
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_put(struct net_context *context);
#else
static inline int net_tcp_put(struct net_context *context)
{
	ARG_UNUSED(context);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Set TCP socket into listening state
 *
 * @param context Network context
 *
 * @return 0 if successful, -EOPNOTSUPP if the context was not for TCP,
 *         -EPROTONOSUPPORT if TCP is not supported
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_listen(struct net_context *context);
#else
static inline int net_tcp_listen(struct net_context *context)
{
	ARG_UNUSED(context);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Update TCP receive window
 *
 * @param context Network context
 * @param delta Receive window delta
 *
 * @return 0 on success, -EPROTOTYPE if there is no TCP context, -EINVAL
 *         if the receive window delta is out of bounds, -EPROTONOSUPPORT
 *         if TCP is not supported
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_update_recv_wnd(struct net_context *context, s32_t delta);
#else
static inline int net_tcp_update_recv_wnd(struct net_context *context,
					  s32_t delta)
{
	ARG_UNUSED(context);
	ARG_UNUSED(delta);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Initialize TCP parts of a context
 *
 * @param context Network context
 *
 * @return 0 if successful, < 0 on error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_get(struct net_context *context);
#else
static inline int net_tcp_get(struct net_context *context)
{
	ARG_UNUSED(context);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Unref TCP parts of a context
 *
 * @param context Network context
 *
 * @return 0 if successful, < 0 on error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_unref(struct net_context *context);
#else
static inline int net_tcp_unref(struct net_context *context)
{
	ARG_UNUSED(context);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Accept TCP connection
 *
 * @param context Network context
 * @param cb Accept callback
 * @param user_data Accept callback user data
 *
 * @return 0 on success, < 0 on error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_accept(struct net_context *context, net_tcp_accept_cb_t cb,
		   void *user_data);
#else
static inline int net_tcp_accept(struct net_context *context,
				 net_tcp_accept_cb_t cb, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Connect TCP connection
 *
 * @param context Network context
 * @param addr Remote address
 * @param laddr Local address
 * @param rport Remote port
 * @param lport Local port
 * @param timeout Connect timeout
 * @param cb Connect callback
 * @param user_data Connect callback user data
 *
 * @return 0 on success, < 0 on error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_connect(struct net_context *context,
		    const struct sockaddr *addr,
		    struct sockaddr *laddr,
		    u16_t rport,
		    u16_t lport,
		    s32_t timeout,
		    net_context_connect_cb_t cb,
		    void *user_data);
#else
static inline int net_tcp_connect(struct net_context *context,
				  const struct sockaddr *addr,
				  struct sockaddr *laddr,
				  u16_t rport, u16_t lport, s32_t timeout,
				  net_context_connect_cb_t cb, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(addr);
	ARG_UNUSED(laddr);
	ARG_UNUSED(rport);
	ARG_UNUSED(lport);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Get pointer to TCP header in net_pkt
 *
 * @param pkt Network packet
 * @param tcp_access Helper variable for accessing TCP header
 *
 * @return TCP header on success, NULL on error
 */
#if defined(CONFIG_NET_NATIVE_TCP)
struct net_tcp_hdr *net_tcp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *tcp_access);
#else
static inline
struct net_tcp_hdr *net_tcp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *tcp_access)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(tcp_access);

	return NULL;
}
#endif

#if defined(CONFIG_NET_NATIVE_TCP)
void net_tcp_init(void);
#else
#define net_tcp_init(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TCP_INTERNAL_H */
