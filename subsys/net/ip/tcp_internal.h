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
#include <zephyr/random/rand32.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif


#include "tcp_private.h"

enum tcp_conn_option {
	TCP_OPT_NODELAY	= 1,
};

/**
 * @brief Calculates and returns the MSS for a given TCP context
 *
 * @param tcp TCP context
 *
 * @return Maximum Segment Size
 */
#if defined(CONFIG_NET_NATIVE_TCP)
uint16_t net_tcp_get_supported_mss(const struct tcp *conn);
#else
static inline uint16_t net_tcp_get_supported_mss(const struct tcp *conn)
{
	ARG_UNUSED(conn);
	return 0;
}
#endif

const char *net_tcp_state_str(enum tcp_state state);

/**
 * @brief Obtains the state for a TCP context
 *
 * @param tcp TCP context
 */
#if defined(CONFIG_NET_NATIVE_TCP)
static inline enum tcp_state net_tcp_get_state(const struct tcp *conn)
{
	return conn->state;
}
#else
static inline enum tcp_state net_tcp_get_state(const struct tcp *conn)
{
	ARG_UNUSED(conn);
	return TCP_CLOSED;
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
		    uint16_t rport,
		    uint16_t lport,
		    k_timeout_t timeout,
		    net_context_connect_cb_t cb,
		    void *user_data);
#else
static inline int net_tcp_connect(struct net_context *context,
				  const struct sockaddr *addr,
				  struct sockaddr *laddr,
				  uint16_t rport, uint16_t lport,
				  k_timeout_t timeout,
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
 * @brief TCP receive function
 *
 * @param context Network context
 * @param cb TCP receive callback function
 * @param user_data TCP receive callback user data
 *
 * @return 0 if no error, < 0 in case of error
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
int net_tcp_update_recv_wnd(struct net_context *context, int32_t delta);
#else
static inline int net_tcp_update_recv_wnd(struct net_context *context,
					  int32_t delta)
{
	ARG_UNUSED(context);
	ARG_UNUSED(delta);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Queue a TCP FIN packet if needed to close the socket
 *
 * @param context Network context
 *
 * @return 0 on success where a TCP FIN packet has been queued, -ENOTCONN
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

#define NET_TCP_MAX_OPT_SIZE  8

#if defined(CONFIG_NET_NATIVE_TCP)
void net_tcp_init(void);
#else
#define net_tcp_init(...)
#endif

/**
 * @brief Set tcp specific options of a socket
 *
 * @param context Network context
 *
 * @return 0 on success, -EINVAL if the value is not allowed
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_set_option(struct net_context *context,
		       enum tcp_conn_option option,
		       const void *value, size_t len);
#else
static inline int net_tcp_set_option(struct net_context *context,
				     enum tcp_conn_option option,
				     const void *value, size_t len)
{
	ARG_UNUSED(context);
	ARG_UNUSED(option);
	ARG_UNUSED(value);
	ARG_UNUSED(len);

	return -EPROTONOSUPPORT;
}
#endif


/**
 * @brief Obtain tcp specific options of a socket
 *
 * @param context Network context
 *
 * @return 0 on success
 */
#if defined(CONFIG_NET_NATIVE_TCP)
int net_tcp_get_option(struct net_context *context,
		       enum tcp_conn_option option,
		       void *value, size_t *len);
#else
static inline int net_tcp_get_option(struct net_context *context,
				     enum tcp_conn_option option,
				     void *value, size_t *len)
{
	ARG_UNUSED(context);
	ARG_UNUSED(option);
	ARG_UNUSED(value);
	ARG_UNUSED(len);

	return -EPROTONOSUPPORT;
}
#endif

/**
 * @brief Obtain a semaphore indicating if transfers are blocked (either due to
 *        filling TX window or entering retransmission mode).
 *
 * @param context Network context
 *
 * @return semaphore indicating if transfers are blocked
 */
struct k_sem *net_tcp_tx_sem_get(struct net_context *context);

#ifdef __cplusplus
}
#endif

#endif /* __TCP_INTERNAL_H */
