/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Transmission Control Protocol (TCP)
 *
 * - net_tcp_get() is called by net_context_get(AF_INET, SOCK_STREAM,
     IPPROTO_TCP, ...) and creates struct tcp for the net_context
 * - net_tcp_listen()/net_tcp_accept() listen/accept
 * - At the reception of SYN on the listening net_context, a new pair
 *   of net_context/struct tcp registers a new net_conn handle
 *   with the tcp_recv() as a callback
 * - net_tcp_queue() queues the data for the transmission
 * - The incoming data is delivered up through the context->recv_cb
 * - net_tcp_put() closes the connection
 *
 * NOTE: The present API is provided in order to make the integration
 *       into the ip stack and the socket layer less intrusive.
 *
 *       Semantically cleaner use is possible (and might be exposed),
 *       look into the unit test tests/net/tcp for insights.
 */

#ifndef TCP_H
#define TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <sys/types.h>

/**
 * @brief Allocate a TCP connection for the net_context
 *        and mutually link the net_context and TCP connection.
 *
 * @param context Network context
 *
 * @return 0 on success, < 0 on error
 */
int net_tcp_get(struct net_context *context);

/**
 * @brief Close and delete the TCP connection for the net_context
 *
 * @param context Network context
 *
 * @return 0 on success, < 0 on error
 */
int net_tcp_put(struct net_context *context);

/**
 * @brief Listen for an incoming TCP connection
 *
 * @param context Network context
 *
 * @return 0 if successful, < 0 on error
 */
int net_tcp_listen(struct net_context *context);

/**
 * @brief Register an accept callback
 *
 * @param context	Network context
 * @param cb		net_tcp_accept_cb_t callback
 * @param user_data	User data passed as an argument in the callback
 *
 * @return 0 if successful, < 0 on error
 */
int net_tcp_accept(struct net_context *context, net_tcp_accept_cb_t cb,
			void *user_data);

/* TODO: split into 2 functions, conn -> context, queue -> send? */

/* The following functions are provided solely for the compatibility
 * with the old TCP
 */

/**
 * @brief Return struct net_tcp_hdr pointer
 *
 * @param pkt Network packet
 * @param tcp_access Helper variable for accessing TCP header
 *
 * @return Pointer to the TCP header on success, NULL on error
 */
struct net_tcp_hdr *net_tcp_input(struct net_pkt *pkt,
					struct net_pkt_data_access *tcp_access);
/* TODO: net_tcp_input() isn't used by TCP and might be dropped with little
 *       re-factoring
 */

/* No ops, provided for compatibility with the old TCP */

#if defined(CONFIG_NET_NATIVE_TCP)
void net_tcp_init(void);
#else
#define net_tcp_init(...)
#endif
int net_tcp_update_recv_wnd(struct net_context *context, int32_t delta);
int net_tcp_finalize(struct net_pkt *pkt, bool force_chksum);

#if defined(CONFIG_NET_TEST_PROTOCOL)
/**
 * @brief Handle an incoming TCP packet
 *
 * This function is provided for the TCP sanity check and will be eventually
 * dropped.
 *
 * @param pkt Network packet
 */
void tcp_input(struct net_pkt *pkt);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TCP_H */
