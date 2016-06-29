/** @file
 * @brief Network context definitions
 *
 * An API for applications to define a network connection.
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

#ifndef __NET_CONTEXT_H
#define __NET_CONTEXT_H

#include <nanokernel.h>

#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_stats.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Is this context used or not */
#define NET_CONTEXT_IN_USE BIT(0)

/** State of the context (bits 1 & 2 in the flags) */
enum net_context_state {
	NET_CONTEXT_IDLE = 0,
	NET_CONTEXT_CONFIGURING = 1,
	NET_CONTEXT_READY = 2,
	NET_CONTEXT_CLOSING = 3,
};

#define NET_CONTEXT_LISTEN BIT(3)

/**
 * The address family, connection type and IP protocol are
 * stored into a bit field to save space.
 */
/** Protocol family of this connection */
#define NET_CONTEXT_FAMILY BIT(4)

/** Type of the connection (datagram / stream) */
#define NET_CONTEXT_TYPE   BIT(5)

/** IP protocol (like UDP or TCP) */
#define NET_CONTEXT_PROTO  BIT(6)

/** Remote address set */
#define NET_CONTEXT_REMOTE_ADDR_SET  BIT(7)

struct net_context;

/**
 * @brief Network data receive callback.
 *
 * @details The recv callback is called after a network data is
 * received. The callback is called in fiber context.
 *
 * @param context The context to use.
 * @param buf Network buffer that is received. If the buf is not NULL,
 * then the callback will own the buffer and it needs to to unref the buf
 * as soon as it has finished working with it.
 * @param status Value is set to 0 if some data is received, <0 if
 * there was an error receiving data, in this case the buf parameter is
 * set to NULL.
 * @param user_data The user data given in net_recv() call.
 */
typedef void (*net_context_recv_cb_t)(struct net_context *context,
				      struct net_buf *buf,
				      int status,
				      void *user_data);

/**
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data is
 * sent. The callback is called in fiber context.
 *
 * @param context The context to use.
 * @param status Value is set to 0 if all data was sent ok, <0 if
 * there was an error sending data. >0 amount of data that was
 * sent when not all data was sent ok.
 * @param token User specified value specified in net_send() call.
 * @param user_data The user data given in net_send() call.
 */
typedef void (*net_context_send_cb_t)(struct net_context *context,
				      int status,
				      void *token,
				      void *user_data);

/**
 * Note that we do not store the actual source IP address in the context
 * because the address is already be set in the network interface struct.
 * If there is no such source address there, the packet cannot be sent
 * anyway. This saves 12 bytes / context in IPv6.
 */
struct net_context {
	/** Local IP address. Note that the values are in network byte order.
	 */
	struct sockaddr_ptr local;

	/** Remote IP address. Note that the values are in network byte order.
	 */
	struct sockaddr remote;

	/** Connection handle */
	void *conn_handler;

	/** Receive callback to be called when desired packet
	 * has been received.
	 */
	net_context_recv_cb_t recv_cb;

	/** Send callback to be called when the packet has been sent
	 * successfully.
	 */
	net_context_send_cb_t send_cb;

	/** User data.
	 */
	void *user_data;

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	/**
	 * Mutex for synchronous recv API call.
	 */
	struct nano_sem recv_data_wait;
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	/** Network interface assigned to this context */
	uint8_t iface;

	/** Flags for the context */
	uint8_t flags;
};

static inline bool net_context_is_used(struct net_context *context)
{
	NET_ASSERT(context);

	return context->flags & NET_CONTEXT_IN_USE;
}

#define NET_CONTEXT_STATE_SHIFT 1
#define NET_CONTEXT_STATE_MASK 0x03

/**
 * @brief Get state for this network context.
 *
 * @details This function returns the state of the context.
 *
 * @param context Network context.
 *
 * @return Network state.
 */
static inline
enum net_context_state net_context_get_state(struct net_context *context)
{
	NET_ASSERT(context);

	return (context->flags >> NET_CONTEXT_STATE_SHIFT) &
		NET_CONTEXT_STATE_MASK;
}

/**
 * @brief Set state for this network context.
 *
 * @details This function sets the state of the context.
 *
 * @param context Network context.
 * @param state New network context state.
 */
static inline void net_context_set_state(struct net_context *context,
					 enum net_context_state state)
{
	NET_ASSERT(context);

	context->flags |= ((state & NET_CONTEXT_STATE_MASK) <<
			   NET_CONTEXT_STATE_SHIFT);
}

/**
 * @brief Get address family for this network context.
 *
 * @details This function returns the address family (IPv4 or IPv6)
 * of the context.
 *
 * @param context Network context.
 *
 * @return Network state.
 */
static inline sa_family_t net_context_get_family(struct net_context *context)
{
	NET_ASSERT(context);

	if (context->flags & NET_CONTEXT_FAMILY) {
		return AF_INET6;
	}

	return AF_INET;
}

/**
 * @brief Set address family for this network context.
 *
 * @details This function sets the address family (IPv4 or IPv6)
 * of the context.
 *
 * @param context Network context.
 * @param family Address family (AF_INET or AF_INET6)
 */
static inline void net_context_set_family(struct net_context *context,
					  sa_family_t family)
{
	NET_ASSERT(context);

	if (family == AF_INET6) {
		context->flags |= NET_CONTEXT_FAMILY;
		return;
	}

	context->flags &= ~NET_CONTEXT_FAMILY;
}

/**
 * @brief Get context type for this network context.
 *
 * @details This function returns the context type (stream or datagram)
 * of the context.
 *
 * @param context Network context.
 *
 * @return Network context type.
 */
static inline
enum net_sock_type net_context_get_type(struct net_context *context)
{
	NET_ASSERT(context);

	if (context->flags & NET_CONTEXT_TYPE) {
		return SOCK_STREAM;
	}

	return SOCK_DGRAM;
}

/**
 * @brief Set context type for this network context.
 *
 * @details This function sets the context type (stream or datagram)
 * of the context.
 *
 * @param context Network context.
 * @param type Context type (SOCK_STREAM or SOCK_DGRAM)
 */
static inline void net_context_set_type(struct net_context *context,
					enum net_sock_type type)
{
	NET_ASSERT(context);

	if (type == SOCK_STREAM) {
		context->flags |= NET_CONTEXT_TYPE;
		return;
	}

	context->flags &= ~NET_CONTEXT_TYPE;
}

/**
 * @brief Get context IP protocol for this network context.
 *
 * @details This function returns the context IP protocol (UDP / TCP)
 * of the context.
 *
 * @param context Network context.
 *
 * @return Network context IP protocol.
 */
static inline
enum net_ip_protocol net_context_get_ip_proto(struct net_context *context)
{
	NET_ASSERT(context);

	if (context->flags & NET_CONTEXT_TYPE) {
		return IPPROTO_TCP;
	}

	return IPPROTO_UDP;
}

/**
 * @brief Set context IP protocol for this network context.
 *
 * @details This function sets the context IP protocol (UDP / TCP)
 * of the context.
 *
 * @param context Network context.
 * @param ip_proto Context IP protocol (IPPROTO_UDP or IPPROTO_TCP)
 */
static inline void net_context_set_ip_proto(struct net_context *context,
					    enum net_ip_protocol ip_proto)
{
	NET_ASSERT(context);

	if (ip_proto == IPPROTO_TCP) {
		context->flags |= NET_CONTEXT_PROTO;
		return;
	}

	context->flags &= ~NET_CONTEXT_PROTO;
}

/**
 * @brief Get network interface for this context.
 *
 * @details This function returns the used network interface.
 *
 * @param context Network context.
 *
 * @return Context network interface if context is bind to interface,
 * NULL otherwise.
 */
static inline
struct net_if *net_context_get_iface(struct net_context *context)
{
	NET_ASSERT(context);

	return net_if_get_by_index(context->iface);
}

/**
 * @brief Set network interface for this context.
 *
 * @details This function binds network interface to this context.
 *
 * @param context Network context.
 * @param iface Network interface.
 */
static inline void net_context_set_iface(struct net_context *context,
					 struct net_if *iface)
{
	NET_ASSERT(iface);

	context->iface = net_if_get_by_iface(iface);
}

/**
 * @brief Get network context.
 *
 * @details Network context is used to define the connection
 * 5-tuple (protocol, remote address, remote port, source
 * address and source port). This is similar as BSD socket()
 * function.
 *
 * @param family IP address family (AF_INET or AF_INET6)
 * @param type Type of the socket, SOCK_STREAM or SOCK_DGRAM
 * @param ip_proto IP protocol, IPPROTO_UDP or IPPROTO_TCP
 * @param context The allocated context is returned to the caller.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_get(sa_family_t family,
		    enum net_sock_type type,
		    enum net_ip_protocol ip_proto,
		    struct net_context **context);

/**
 * @brief Free/close a network context.
 *
 * @details This releases the context. It is not possible to
 * send or receive data via this context after this call.
 * This is similar as BSD shutdown() function.
 *
 * @param context The context to be closed.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_put(struct net_context *context);

/**
 * @brief Assign a socket a local address.
 *
 * @details This is similar as BSD bind() function.
 *
 * @param context The context to be assigned.
 * @param addr Address to assigned.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_bind(struct net_context *context,
		     const struct sockaddr *addr);

/**
 * @brief Mark the context as a listening one.
 *
 * @details This is similar as BSD listen() function.
 *
 * @param context The context to use.
 * @param backlog The size of the pending connections backlog.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_listen(struct net_context *context,
		       int backlog);

/**
 * @brief Connection callback.
 *
 * @details The connect callback is called after a connection is being
 * established. The callback is called in fiber context.
 *
 * @param context The context to use.
 * @param user_data The user data given in net_context_connect() call.
 */
typedef void (*net_context_connect_cb_t)(struct net_context *context,
					 void *user_data);

/**
 * @brief Create a network connection.
 *
 * @details Initiate a connection to be created. This function
 * will return immediately if the timeout is set to 0. If the timeout
 * is set to TICKS_UNLIMITED, the function will wait until the
 * connection is established. Timeout value > 0, will wait as
 * many system ticks. After the connection is established
 * a caller supplied callback is called. The callback is called even
 * if timeout was set to TICKS_UNLIMITED, the callback is called
 * before this function will return in this case. The callback is not
 * called if the timeout expires.
 * For context with type SOCK_DGRAM, this function marks the default
 * address of the destination but it does not try to actually connect.
 * The callback is called also for both SOCK_DGRAM and SOCK_STREAM
 * type connections.
 * This is similar as BSD connect() function.
 *
 * @param context The context to use.
 * @param addr The peer address to connect to.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are TICKS_UNLIMITED, 0, >0.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_connect(struct net_context *context,
			const struct sockaddr *addr,
			net_context_connect_cb_t cb,
			int32_t timeout,
			void *user_data);

/**
 * @brief Accept callback
 *
 * @details The accept callback is called after a successful
 * connection is being established while we are waiting a connection
 * attempt. The callback is called in fiber context.
 *
 * @param context The context to use.
 * @param user_data The user data given in net_context_accept() call.
 */
typedef void (*net_context_accept_cb_t)(struct net_context *new_context,
					void *user_data);

/**
 * @brief Accept a network connection attempt.
 *
 * @details Accept a connection being established. This function
 * will return immediately if the timeout is set to 0. In this case
 * the context will call the supplied callback when ever there is
 * a connection established to this context. This is "a register
 * handler and forget" type of call (async).
 * If the timeout is set to TICKS_UNLIMITED, the function will wait
 * until the connection is established. Timeout value > 0, will wait as
 * many system ticks.
 * After the connection is established a caller supplied callback is called.
 * The callback is called even if timeout was set to TICKS_UNLIMITED, the
 * callback is called before this function will return in this case.
 * The callback is not called if the timeout expires.
 * This is similar as BSD accept() function.
 *
 * @param context The context to use.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are TICKS_UNLIMITED, 0, >0.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_accept(struct net_context *context,
		       net_context_accept_cb_t cb,
		       int32_t timeout,
		       void *user_data);

/**
 * @brief Send a network buffer to a peer.
 *
 * @details This function can be used to send network data to a peer
 * connection. This function will return immediately if the timeout
 * is set to 0. If the timeout is set to TICKS_UNLIMITED, the function
 * will wait until the network buffer is sent. Timeout value > 0 will
 * wait as many system ticks. After the network buffer is sent,
 * a caller supplied callback is called. The callback is called even
 * if timeout was set to TICKS_UNLIMITED, the callback is called
 * before this function will return in this case. The callback is not
 * called if the timeout expires. For context of type SOCK_DGRAM,
 * the destination address must have been set by the call to
 * net_context_connect().
 * This is similar as BSD send() function.
 *
 * @param buf The network buffer to send.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are TICKS_UNLIMITED, 0, >0.
 * @param token Caller specified value that is passed as is to callback.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_send(struct net_buf *buf,
		     net_context_send_cb_t cb,
		     int32_t timeout,
		     void *token,
		     void *user_data);

/**
 * @brief Send a network buffer to a peer specified by address.
 *
 * @details This function can be used to send network data to a peer
 * specified by address. This variant can only be used for datagram
 * connections of type SOCK_DGRAM. This function will return immediately
 * if the timeout is set to 0. If the timeout is set to TICKS_UNLIMITED,
 * the function will wait until the network buffer is sent. Timeout
 * value > 0 will wait as many system ticks. After the network buffer
 * is sent, a caller supplied callback is called. The callback is called
 * even if timeout was set to TICKS_UNLIMITED, the callback is called
 * before this function will return. The callback is not called if the
 * timeout expires.
 * This is similar as BSD sendto() function.
 *
 * @param buf The network buffer to send.
 * @param dst_addr Destination address. This will override the address
 * already set in network buffer.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are TICKS_UNLIMITED, 0, >0.
 * @param token Caller specified value that is passed as is to callback.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_sendto(struct net_buf *buf,
		       const struct sockaddr *dst_addr,
		       net_context_send_cb_t cb,
		       int32_t timeout,
		       void *token,
		       void *user_data);

/**
 * @brief Receive network data from a peer specified by context.
 *
 * @details This function can be used to register a callback function
 * that is called by the network stack when network data has been received
 * for this context. As this function registers a callback, then there
 * is no need to call this function multiple times if timeout is set to 0.
 * If callback function or user data changes, then the function can be called
 * multiple times to register new values.
 * This function will return immediately if the timeout is set to 0. If the
 * timeout is set to TICKS_UNLIMITED, the function will wait until the
 * network buffer is received. Timeout value > 0 will wait as many system
 * ticks. After the network buffer is received, a caller supplied callback is
 * called. The callback is called even if timeout was set to TICKS_UNLIMITED,
 * the callback is called before this function will return in this case.
 * The callback is not called if the timeout expires. The timeout functionality
 * can be compiled out if synchronous behaviour is not needed. The sync call
 * logic requires some memory that can be saved if only async way of call is
 * used. If CONFIG_NET_CONTEXT_SYNC_RECV is not set, then the timeout parameter
 * value is ignored.
 * This is similar as BSD recv() function.
 *
 * @param context The network context to use.
 * @param cb Caller supplied callback function.
 * @param timeout Caller supplied timeout
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     int32_t timeout,
		     void *user_data);

/**
 * @brief Internal function that is called when network packet is sent
 * successfully.
 *
 * @param context The network context to use.
 * @param token User supplied token tied to the net_buf that was sent.
 * @param err_code Error code
 */
static inline void net_context_send_cb(struct net_context *context,
				       void *token,
				       int err_code)
{
	if (context->send_cb) {
		context->send_cb(context, err_code, token, context->user_data);
	}

	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		NET_STATS(++net_stats.udp.sent);
	}
}

#ifdef __cplusplus
}
#endif

#endif /* __NET_CONTEXT_H */
