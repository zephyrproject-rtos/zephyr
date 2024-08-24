/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for offloading IP stack
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_OFFLOAD_H_
#define ZEPHYR_INCLUDE_NET_NET_OFFLOAD_H_

/**
 * @brief Network offloading interface
 * @defgroup net_offload Network Offloading Interface
 * @since 1.7
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_OFFLOAD)

/** @cond INTERNAL_HIDDEN */

static inline int32_t timeout_to_int32(k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return 0;
	} else if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return -1;
	} else {
		return k_ticks_to_ms_floor32(timeout.ticks);
	}
}

/** @endcond */

/** For return parameters and return values of the elements in this
 * struct, see similarly named functions in net_context.h
 */
struct net_offload {
	/**
	 * This function is called when the socket is to be opened.
	 */
	int (*get)(sa_family_t family,
		   enum net_sock_type type,
		   enum net_ip_protocol ip_proto,
		   struct net_context **context);

	/**
	 * This function is called when user wants to bind to local IP address.
	 */
	int (*bind)(struct net_context *context,
		    const struct sockaddr *addr,
		    socklen_t addrlen);

	/**
	 * This function is called when user wants to mark the socket
	 * to be a listening one.
	 */
	int (*listen)(struct net_context *context, int backlog);

	/**
	 * This function is called when user wants to create a connection
	 * to a peer host.
	 */
	int (*connect)(struct net_context *context,
		       const struct sockaddr *addr,
		       socklen_t addrlen,
		       net_context_connect_cb_t cb,
		       int32_t timeout,
		       void *user_data);

	/**
	 * This function is called when user wants to accept a connection
	 * being established.
	 */
	int (*accept)(struct net_context *context,
		      net_tcp_accept_cb_t cb,
		      int32_t timeout,
		      void *user_data);

	/**
	 * This function is called when user wants to send data to peer host.
	 */
	int (*send)(struct net_pkt *pkt,
		    net_context_send_cb_t cb,
		    int32_t timeout,
		    void *user_data);

	/**
	 * This function is called when user wants to send data to peer host.
	 */
	int (*sendto)(struct net_pkt *pkt,
		      const struct sockaddr *dst_addr,
		      socklen_t addrlen,
		      net_context_send_cb_t cb,
		      int32_t timeout,
		      void *user_data);

	/**
	 * This function is called when user wants to receive data from peer
	 * host.
	 */
	int (*recv)(struct net_context *context,
		    net_context_recv_cb_t cb,
		    int32_t timeout,
		    void *user_data);

	/**
	 * This function is called when user wants to close the socket.
	 */
	int (*put)(struct net_context *context);
};

/**
 * @brief Get a network socket/context from the offloaded IP stack.
 *
 * @details Network socket is used to define the connection
 * 5-tuple (protocol, remote address, remote port, source
 * address and source port). This is similar as BSD socket()
 * function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param family IP address family (AF_INET or AF_INET6)
 * @param type Type of the socket, SOCK_STREAM or SOCK_DGRAM
 * @param ip_proto IP protocol, IPPROTO_UDP or IPPROTO_TCP
 * @param context The allocated context is returned to the caller.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_get(struct net_if *iface,
				  sa_family_t family,
				  enum net_sock_type type,
				  enum net_ip_protocol ip_proto,
				  struct net_context **context)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->get);

	return net_if_offload(iface)->get(family, type, ip_proto, context);
}

/**
 * @brief Assign a socket a local address.
 *
 * @details This is similar as BSD bind() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param context The context to be assigned.
 * @param addr Address to assigned.
 * @param addrlen Length of the address.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_bind(struct net_if *iface,
				   struct net_context *context,
				   const struct sockaddr *addr,
				   socklen_t addrlen)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->bind);

	return net_if_offload(iface)->bind(context, addr, addrlen);
}

/**
 * @brief Mark the context as a listening one.
 *
 * @details This is similar as BSD listen() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param context The context to use.
 * @param backlog The size of the pending connections backlog.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_listen(struct net_if *iface,
				     struct net_context *context,
				     int backlog)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->listen);

	return net_if_offload(iface)->listen(context, backlog);
}

/**
 * @brief            Create a network connection.
 *
 * @details          The net_context_connect function creates a network
 *                   connection to the host specified by addr. After the
 *                   connection is established, the user-supplied callback (cb)
 *                   is executed. cb is called even if the timeout was set to
 *                   K_FOREVER. cb is not called if the timeout expires.
 *                   For datagram sockets (SOCK_DGRAM), this function only sets
 *                   the peer address.
 *                   This function is similar to the BSD connect() function.
 *
 * @param iface      Network interface where the offloaded IP stack can be
 *                   reached.
 * @param context    The network context.
 * @param addr       The peer address to connect to.
 * @param addrlen    Peer address length.
 * @param cb         Callback function. Set to NULL if not required.
 * @param timeout    The timeout value for the connection. Possible values:
 *                   * K_NO_WAIT: this function will return immediately,
 *                   * K_FOREVER: this function will block until the
 *                                      connection is established,
 *                   * >0: this function will wait the specified ms.
 * @param user_data  Data passed to the callback function.
 *
 * @return           0 on success.
 * @return           -EINVAL if an invalid parameter is passed as an argument.
 * @return           -ENOTSUP if the operation is not supported or implemented.
 */
static inline int net_offload_connect(struct net_if *iface,
				      struct net_context *context,
				      const struct sockaddr *addr,
				      socklen_t addrlen,
				      net_context_connect_cb_t cb,
				      k_timeout_t timeout,
				      void *user_data)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->connect);

	return net_if_offload(iface)->connect(
		context, addr, addrlen, cb,
		timeout_to_int32(timeout),
		user_data);
}

/**
 * @brief Accept a network connection attempt.
 *
 * @details Accept a connection being established. This function
 * will return immediately if the timeout is set to K_NO_WAIT.
 * In this case the context will call the supplied callback when ever
 * there is a connection established to this context. This is "a register
 * handler and forget" type of call (async).
 * If the timeout is set to K_FOREVER, the function will wait
 * until the connection is established. Timeout value > 0, will wait as
 * many ms.
 * After the connection is established a caller-supplied callback is called.
 * The callback is called even if timeout was set to K_FOREVER, the
 * callback is called before this function will return in this case.
 * The callback is not called if the timeout expires.
 * This is similar as BSD accept() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param context The context to use.
 * @param cb Caller-supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_accept(struct net_if *iface,
				     struct net_context *context,
				     net_tcp_accept_cb_t cb,
				     k_timeout_t timeout,
				     void *user_data)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->accept);

	return net_if_offload(iface)->accept(
		context, cb,
		timeout_to_int32(timeout),
		user_data);
}

/**
 * @brief Send a network packet to a peer.
 *
 * @details This function can be used to send network data to a peer
 * connection. This function will return immediately if the timeout
 * is set to K_NO_WAIT. If the timeout is set to K_FOREVER, the function
 * will wait until the network packet is sent. Timeout value > 0 will
 * wait as many ms. After the network packet is sent,
 * a caller-supplied callback is called. The callback is called even
 * if timeout was set to K_FOREVER, the callback is called
 * before this function will return in this case. The callback is not
 * called if the timeout expires. For context of type SOCK_DGRAM,
 * the destination address must have been set by the call to
 * net_context_connect().
 * This is similar as BSD send() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param pkt The network packet to send.
 * @param cb Caller-supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_send(struct net_if *iface,
				   struct net_pkt *pkt,
				   net_context_send_cb_t cb,
				   k_timeout_t timeout,
				   void *user_data)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->send);

	return net_if_offload(iface)->send(
		pkt, cb,
		timeout_to_int32(timeout),
		user_data);
}

/**
 * @brief Send a network packet to a peer specified by address.
 *
 * @details This function can be used to send network data to a peer
 * specified by address. This variant can only be used for datagram
 * connections of type SOCK_DGRAM. This function will return immediately
 * if the timeout is set to K_NO_WAIT. If the timeout is set to K_FOREVER,
 * the function will wait until the network packet is sent. Timeout
 * value > 0 will wait as many ms. After the network packet
 * is sent, a caller-supplied callback is called. The callback is called
 * even if timeout was set to K_FOREVER, the callback is called
 * before this function will return. The callback is not called if the
 * timeout expires.
 * This is similar as BSD sendto() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param pkt The network packet to send.
 * @param dst_addr Destination address. This will override the address
 * already set in network packet.
 * @param addrlen Length of the address.
 * @param cb Caller-supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_sendto(struct net_if *iface,
				     struct net_pkt *pkt,
				     const struct sockaddr *dst_addr,
				     socklen_t addrlen,
				     net_context_send_cb_t cb,
				     k_timeout_t timeout,
				     void *user_data)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->sendto);

	return net_if_offload(iface)->sendto(
		pkt, dst_addr, addrlen, cb,
		timeout_to_int32(timeout),
		user_data);
}

/**
 * @brief Receive network data from a peer specified by context.
 *
 * @details This function can be used to register a callback function
 * that is called by the network stack when network data has been received
 * for this context. As this function registers a callback, then there
 * is no need to call this function multiple times if timeout is set to
 * K_NO_WAIT.
 * If callback function or user data changes, then the function can be called
 * multiple times to register new values.
 * This function will return immediately if the timeout is set to K_NO_WAIT.
 * If the timeout is set to K_FOREVER, the function will wait until the
 * network packet is received. Timeout value > 0 will wait as many ms.
 * After the network packet is received, a caller-supplied callback is
 * called. The callback is called even if timeout was set to K_FOREVER,
 * the callback is called before this function will return in this case.
 * The callback is not called if the timeout expires. The timeout functionality
 * can be compiled out if synchronous behavior is not needed. The sync call
 * logic requires some memory that can be saved if only async way of call is
 * used. If CONFIG_NET_CONTEXT_SYNC_RECV is not set, then the timeout parameter
 * value is ignored.
 * This is similar as BSD recv() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param context The network context to use.
 * @param cb Caller-supplied callback function.
 * @param timeout Caller-supplied timeout. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_recv(struct net_if *iface,
				   struct net_context *context,
				   net_context_recv_cb_t cb,
				   k_timeout_t timeout,
				   void *user_data)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->recv);

	return net_if_offload(iface)->recv(
		context, cb,
		timeout_to_int32(timeout),
		user_data);
}

/**
 * @brief Free/close a network context.
 *
 * @details This releases the context. It is not possible to
 * send or receive data via this context after this call.
 * This is similar as BSD shutdown() function.
 *
 * @param iface Network interface where the offloaded IP stack can be
 * reached.
 * @param context The context to be closed.
 *
 * @return 0 if ok, < 0 if error
 */
static inline int net_offload_put(struct net_if *iface,
				  struct net_context *context)
{
	NET_ASSERT(iface);
	NET_ASSERT(net_if_offload(iface));
	NET_ASSERT(net_if_offload(iface)->put);

	return net_if_offload(iface)->put(context);
}

#else

/** @cond INTERNAL_HIDDEN */

static inline int net_offload_get(struct net_if *iface,
				  sa_family_t family,
				  enum net_sock_type type,
				  enum net_ip_protocol ip_proto,
				  struct net_context **context)
{
	return 0;
}

static inline int net_offload_bind(struct net_if *iface,
				   struct net_context *context,
				   const struct sockaddr *addr,
				   socklen_t addrlen)
{
	return 0;
}

static inline int net_offload_listen(struct net_if *iface,
				     struct net_context *context,
				     int backlog)
{
	return 0;
}

static inline int net_offload_connect(struct net_if *iface,
				      struct net_context *context,
				      const struct sockaddr *addr,
				      socklen_t addrlen,
				      net_context_connect_cb_t cb,
				      k_timeout_t timeout,
				      void *user_data)
{
	return 0;
}

static inline int net_offload_accept(struct net_if *iface,
				     struct net_context *context,
				     net_tcp_accept_cb_t cb,
				     k_timeout_t timeout,
				     void *user_data)
{
	return 0;
}

static inline int net_offload_send(struct net_if *iface,
				   struct net_pkt *pkt,
				   net_context_send_cb_t cb,
				   k_timeout_t timeout,
				   void *user_data)
{
	return 0;
}

static inline int net_offload_sendto(struct net_if *iface,
				     struct net_pkt *pkt,
				     const struct sockaddr *dst_addr,
				     socklen_t addrlen,
				     net_context_send_cb_t cb,
				     k_timeout_t timeout,
				     void *user_data)
{
	return 0;
}

static inline int net_offload_recv(struct net_if *iface,
				   struct net_context *context,
				   net_context_recv_cb_t cb,
				   k_timeout_t timeout,
				   void *user_data)
{
	return 0;
}

static inline int net_offload_put(struct net_if *iface,
				  struct net_context *context)
{
	return 0;
}

/** @endcond */

#endif /* CONFIG_NET_OFFLOAD */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_OFFLOAD_H_ */
