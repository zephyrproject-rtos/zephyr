/** @file
 * @brief Network context definitions
 *
 * An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_CONTEXT_H
#define __NET_CONTEXT_H

/**
 * @brief Application network context
 * @defgroup net_context Application network context
 * @{
 */

#include <kernel.h>
#include <atomic.h>

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
	NET_CONTEXT_UNCONNECTED = 0,
	NET_CONTEXT_CONFIGURING = 1,
	NET_CONTEXT_CONNECTING = 1,
	NET_CONTEXT_READY = 2,
	NET_CONTEXT_CONNECTED = 2,
	NET_CONTEXT_LISTENING = 3,
};

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
 * @typedef net_context_recv_cb_t
 * @brief Network data receive callback.
 *
 * @details The recv callback is called after a network data is
 * received.
 *
 * @param context The context to use.
 * @param pkt Network buffer that is received. If the pkt is not NULL,
 * then the callback will own the buffer and it needs to to unref the pkt
 * as soon as it has finished working with it.  On EOF, pkt will be NULL.
 * @param status Value is set to 0 if some data or the connection is
 * at EOF, <0 if there was an error receiving data, in this case the
 * pkt parameter is set to NULL.
 * @param user_data The user data given in net_recv() call.
 */
typedef void (*net_context_recv_cb_t)(struct net_context *context,
				      struct net_pkt *pkt,
				      int status,
				      void *user_data);

/**
 * @typedef net_context_send_cb_t
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data is
 * sent.
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
 * @typedef net_tcp_accept_cb_t
 * @brief Accept callback
 *
 * @details The accept callback is called after a successful
 * connection is being established or if there was an error
 * while we were waiting for a connection attempt.
 *
 * @param context The context to use.
 * @param addr The peer address.
 * @param addrlen Length of the peer address.
 * @param status The status code, 0 on success, < 0 otherwise
 * @param user_data The user data given in net_context_accept() call.
 */
typedef void (*net_tcp_accept_cb_t)(struct net_context *new_context,
				    struct sockaddr *addr,
				    socklen_t addrlen,
				    int status,
				    void *user_data);

/**
 * @typedef net_context_connect_cb_t
 * @brief Connection callback.
 *
 * @details The connect callback is called after a connection is being
 * established.
 *
 * @param context The context to use.
 * @param status Status of the connection establishment. This is 0
 * if the connection was established successfully, <0 if there was an
 * error.
 * @param user_data The user data given in net_context_connect() call.
 */
typedef void (*net_context_connect_cb_t)(struct net_context *context,
					 int status,
					 void *user_data);

/* The net_pkt_get_slab_func_t is here in order to avoid circular
 * dependency between net_pkt.h and net_context.h
 */
/**
 * @typedef net_pkt_get_slab_func_t
 *
 * @brief Function that is called to get the slab that is used
 * for net_pkt allocations.
 *
 * @return Pointer to valid struct k_mem_slab instance.
 */
typedef struct k_mem_slab *(*net_pkt_get_slab_func_t)(void);

/* The net_pkt_get_pool_func_t is here in order to avoid circular
 * dependency between net_pkt.h and net_context.h
 */
/**
 * @typedef net_pkt_get_pool_func_t
 *
 * @brief Function that is called to get the pool that is used
 * for net_buf allocations.
 *
 * @return Pointer to valid struct net_buf_pool instance.
 */
typedef struct net_buf_pool *(*net_pkt_get_pool_func_t)(void);

struct net_tcp;

struct net_conn_handle;

/**
 * Note that we do not store the actual source IP address in the context
 * because the address is already be set in the network interface struct.
 * If there is no such source address there, the packet cannot be sent
 * anyway. This saves 12 bytes / context in IPv6.
 */
struct net_context {
	/** Reference count
	 */
	atomic_t refcount;

	/** Local IP address. Note that the values are in network byte order.
	 */
	struct sockaddr_ptr local;

	/** Remote IP address. Note that the values are in network byte order.
	 */
	struct sockaddr remote;

	/** Connection handle */
	struct net_conn_handle *conn_handler;

	/** Receive callback to be called when desired packet
	 * has been received.
	 */
	net_context_recv_cb_t recv_cb;

	/** Send callback to be called when the packet has been sent
	 * successfully.
	 */
	net_context_send_cb_t send_cb;

	/** Connect callback to be called when a connection has been
	 *  established.
	 */
	net_context_connect_cb_t connect_cb;

	/** User data.
	 */
	void *user_data;

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	/** Get TX net_buf pool for this context.
	 */
	net_pkt_get_slab_func_t tx_slab;

	/** Get DATA net_buf pool for this context.
	 */
	net_pkt_get_pool_func_t data_pool;
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	/**
	 * Semaphore to signal synchronous recv call completion.
	 */
	struct k_sem recv_data_wait;
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	/** Network interface assigned to this context */
	u8_t iface;

	/** Flags for the context */
	u8_t flags;

#if defined(CONFIG_NET_TCP)
	/** TCP connection information */
	struct net_tcp *tcp;
#endif /* CONFIG_NET_TCP */
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

	context->flags &= ~(NET_CONTEXT_STATE_MASK << NET_CONTEXT_STATE_SHIFT);
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

	if (context->flags & NET_CONTEXT_PROTO) {
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
 * @details Network context is used to define the connection 5-tuple
 * (protocol, remote address, remote port, source address and source
 * port). Random free port number will be assigned to source port when
 * context is created. This is similar as BSD socket() function.
 * The context will be created with a reference count of 1.
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
 * @brief Close and unref a network context.
 *
 * @details This releases the context. It is not possible to send or
 * receive data via this context after this call.  This is similar as
 * BSD shutdown() function.  For legacy compatibility, this function
 * will implicitly decrement the reference count and possibly destroy
 * the context either now or when it reaches a final state.
 *
 * @param context The context to be closed.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_put(struct net_context *context);

/**
 * @brief Take a reference count to a net_context, preventing destruction
 *
 * @details Network contexts are not recycled until their reference
 * count reaches zero.  Note that this does not prevent any "close"
 * behavior that results from errors or net_context_put.  It simply
 * prevents the context from being recycled for further use.
 *
 * @param context The context on which to increment the reference count
 *
 * @return The new reference count
 */
int net_context_ref(struct net_context *context);

/**
 * @brief Decrement the reference count to a network context
 *
 * @details Decrements the refcount.  If it reaches zero, the context
 * will be recycled.  Note that this does not cause any
 * network-visible "close" behavior (i.e. future packets to this
 * connection may see TCP RST or ICMP port unreachable responses).  See
 * net_context_put() for that.
 *
 * @param context The context on which to decrement the reference count
 *
 * @return The new reference count, zero if the context was destroyed
 */
int net_context_unref(struct net_context *context);

/**
 * @brief Assign a socket a local address.
 *
 * @details This is similar as BSD bind() function.
 *
 * @param context The context to be assigned.
 * @param addr Address to assigned.
 * @param addrlen Length of the address.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_bind(struct net_context *context,
		     const struct sockaddr *addr,
		     socklen_t addrlen);

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
 * @brief            Create a network connection.
 *
 * @details          The net_context_connect function creates a network
 *                   connection to the host specified by addr. After the
 *                   connection is established, the user supplied callback (cb)
 *                   is executed. cb is called even if the timeout was set to
 *                   K_FOREVER. cb is not called if the timeout expires.
 *                   For datagram sockets (SOCK_DGRAM), this function only sets
 *                   the peer address.
 *                   This function is similar to the BSD connect() function.
 *
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
 * @return           -ETIMEDOUT if the connect operation times out.
 */
int net_context_connect(struct net_context *context,
			const struct sockaddr *addr,
			socklen_t addrlen,
			net_context_connect_cb_t cb,
			s32_t timeout,
			void *user_data);

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
 * After the connection is established a caller supplied callback is called.
 * The callback is called even if timeout was set to K_FOREVER, the
 * callback is called before this function will return in this case.
 * The callback is not called if the timeout expires.
 * This is similar as BSD accept() function.
 *
 * @param context The context to use.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_accept(struct net_context *context,
		       net_tcp_accept_cb_t cb,
		       s32_t timeout,
		       void *user_data);

/**
 * @brief Send a network buffer to a peer.
 *
 * @details This function can be used to send network data to a peer
 * connection. This function will return immediately if the timeout
 * is set to K_NO_WAIT. If the timeout is set to K_FOREVER, the function
 * will wait until the network buffer is sent. Timeout value > 0 will
 * wait as many ms. After the network buffer is sent,
 * a caller supplied callback is called. The callback is called even
 * if timeout was set to K_FOREVER, the callback is called
 * before this function will return in this case. The callback is not
 * called if the timeout expires. For context of type SOCK_DGRAM,
 * the destination address must have been set by the call to
 * net_context_connect().
 * This is similar as BSD send() function.
 *
 * @param pkt The network buffer to send.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param token Caller specified value that is passed as is to callback.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_send(struct net_pkt *pkt,
		     net_context_send_cb_t cb,
		     s32_t timeout,
		     void *token,
		     void *user_data);

/**
 * @brief Send a network buffer to a peer specified by address.
 *
 * @details This function can be used to send network data to a peer
 * specified by address. This variant can only be used for datagram
 * connections of type SOCK_DGRAM. This function will return immediately
 * if the timeout is set to K_NO_WAIT. If the timeout is set to K_FOREVER,
 * the function will wait until the network buffer is sent. Timeout
 * value > 0 will wait as many ms. After the network buffer
 * is sent, a caller supplied callback is called. The callback is called
 * even if timeout was set to K_FOREVER, the callback is called
 * before this function will return. The callback is not called if the
 * timeout expires.
 * This is similar as BSD sendto() function.
 *
 * @param pkt The network buffer to send.
 * @param dst_addr Destination address. This will override the address
 * already set in network buffer.
 * @param addrlen Length of the address.
 * @param cb Caller supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param token Caller specified value that is passed as is to callback.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_sendto(struct net_pkt *pkt,
		       const struct sockaddr *dst_addr,
		       socklen_t addrlen,
		       net_context_send_cb_t cb,
		       s32_t timeout,
		       void *token,
		       void *user_data);

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
 * network buffer is received. Timeout value > 0 will wait as many ms.
 * After the network buffer is received, a caller supplied callback is
 * called. The callback is called even if timeout was set to K_FOREVER,
 * the callback is called before this function will return in this case.
 * The callback is not called if the timeout expires. The timeout functionality
 * can be compiled out if synchronous behavior is not needed. The sync call
 * logic requires some memory that can be saved if only async way of call is
 * used. If CONFIG_NET_CONTEXT_SYNC_RECV is not set, then the timeout parameter
 * value is ignored.
 * This is similar as BSD recv() function.
 * Note that net_context_bind() should be called before net_context_recv().
 * Default random port number is assigned to local port. Only bind() will
 * updates connection information from context. If recv() is called before
 * bind() call, it may refuse to bind to a context which already has
 * a connection associated.
 *
 * @param context The network context to use.
 * @param cb Caller supplied callback function.
 * @param timeout Caller supplied timeout. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     s32_t timeout,
		     void *user_data);

/**
 * @typedef net_context_cb_t
 * @brief Callback used while iterating over network contexts
 *
 * @param context A valid pointer on current network context
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_context_cb_t)(struct net_context *context, void *user_data);

/**
 * @brief Go through all the network connections and call callback
 * for each network context.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 */
void net_context_foreach(net_context_cb_t cb, void *user_data);

/**
 * @brief Create network buffer pool that is used by the IP stack
 * to allocate network buffers that are used by the context when
 * sending data to network.
 *
 * @param context Context that will use the given net_buf pools.
 * @param tx_pool Pointer to the function that will return TX pool
 * to the caller. The TX pool is used when sending data to network.
 * There is one TX net_pkt for each network packet that is sent.
 * @param data_pool Pointer to the function that will return DATA pool
 * to the caller. The DATA pool is used to store data that is sent to
 * the network.
 */
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static inline void net_context_setup_pools(struct net_context *context,
					   net_pkt_get_slab_func_t tx_slab,
					   net_pkt_get_pool_func_t data_pool)
{
	NET_ASSERT(context);
	NET_ASSERT(tx_slab);
	NET_ASSERT(data_pool);

	context->tx_slab = tx_slab;
	context->data_pool = data_pool;
}
#else
#define net_context_setup_pools(context, tx_pool, data_pool)
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __NET_CONTEXT_H */
