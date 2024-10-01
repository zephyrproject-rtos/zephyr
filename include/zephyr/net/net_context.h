/** @file
 * @brief Network context definitions
 *
 * An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CONTEXT_H_
#define ZEPHYR_INCLUDE_NET_NET_CONTEXT_H_

/**
 * @brief Application network context
 * @defgroup net_context Application network context
 * @since 1.0
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_stats.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Is this context used or not */
#define NET_CONTEXT_IN_USE BIT(0)

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

/**
 * The address family, connection type and IP protocol are
 * stored into a bit field to save space.
 */
/** Protocol family of this connection */
#define NET_CONTEXT_FAMILY (BIT(3) | BIT(4) | BIT(5))

/** Type of the connection (datagram / stream / raw) */
#define NET_CONTEXT_TYPE   (BIT(6) | BIT(7))

/** Remote address set */
#define NET_CONTEXT_REMOTE_ADDR_SET  BIT(8)

/** Is the socket accepting connections */
#define NET_CONTEXT_ACCEPTING_SOCK  BIT(9)

/** Is the socket closing / closed */
#define NET_CONTEXT_CLOSING_SOCK  BIT(10)

/** Context is bound to a specific interface */
#define NET_CONTEXT_BOUND_TO_IFACE BIT(11)

struct net_context;

/**
 * @typedef net_context_recv_cb_t
 * @brief Network data receive callback.
 *
 * @details The recv callback is called after a network data packet is
 * received. This callback is called by RX thread so its stack and execution
 * context is used here. Keep processing in the callback minimal to reduce the
 * time spent blocked while handling packets.
 *
 * @param context The context to use.
 * @param pkt Network buffer that is received. If the pkt is not NULL,
 * then the callback will own the buffer and it needs to unref the pkt
 * as soon as it has finished working with it.  On EOF, pkt will be NULL.
 * @param ip_hdr a pointer to relevant IP (v4 or v6) header.
 * @param proto_hdr a pointer to relevant protocol (udp or tcp) header.
 * @param status Value is set to 0 if some data or the connection is
 * at EOF, <0 if there was an error receiving data, in this case the
 * pkt parameter is set to NULL.
 * @param user_data The user data given in net_recv() call.
 */
typedef void (*net_context_recv_cb_t)(struct net_context *context,
				      struct net_pkt *pkt,
				      union net_ip_header *ip_hdr,
				      union net_proto_header *proto_hdr,
				      int status,
				      void *user_data);

/**
 * @typedef net_context_send_cb_t
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data packet is sent.
 * This callback is called by TX thread so its stack and execution context is
 * used here. Keep processing in the callback minimal to reduce the time spent
 * blocked while handling packets.
 *
 * @param context The context to use.
 * @param status Value is set to >= 0: amount of data that was sent,
 * < 0 there was an error sending data.
 * @param user_data The user data given in net_send() call.
 */
typedef void (*net_context_send_cb_t)(struct net_context *context,
				      int status,
				      void *user_data);

/**
 * @typedef net_tcp_accept_cb_t
 * @brief Accept callback
 *
 * @details The accept callback is called after a successful connection was
 * established or if there was an error while we were waiting for a connection
 * attempt. This callback is called by RX thread so its stack and execution
 * context is used here. Keep processing in the callback minimal to reduce the
 * time spent blocked while handling packets.
 *
 * @param new_context The context to use.
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
 * For TCP connections, this callback is called by RX thread so its stack and
 * execution context is used here. The callback is called after the TCP
 * connection was established or if the connection failed. Keep processing in
 * the callback minimal to reduce the time spent blocked while handling
 * packets.
 * For UDP connections, this callback is called immediately by
 * net_context_connect() function. UDP is a connectionless protocol so the
 * connection can be thought of being established immediately.
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
__net_socket struct net_context {
	/** First member of the structure to allow to put contexts into a FIFO.
	 */
	void *fifo_reserved;

	/** User data associated with a context.
	 */
	void *user_data;

	/** Reference count
	 */
	atomic_t refcount;

	/** Internal lock for protecting this context from multiple access.
	 */
	struct k_mutex lock;

	/** Local endpoint address. Note that the values are in network byte
	 * order.
	 */
	struct sockaddr_ptr local;

	/** Remote endpoint address. Note that the values are in network byte
	 * order.
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

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	/** Get TX net_buf pool for this context.
	 */
	net_pkt_get_slab_func_t tx_slab;

	/** Get DATA net_buf pool for this context.
	 */
	net_pkt_get_pool_func_t data_pool;
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_TCP)
	/** TCP connection information */
	void *tcp;
#endif /* CONFIG_NET_TCP */

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	/**
	 * Semaphore to signal synchronous recv call completion.
	 */
	struct k_sem recv_data_wait;
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

#if defined(CONFIG_NET_SOCKETS)
	/** BSD socket private data */
	void *socket_data;

	/** Per-socket packet or connection queues */
	union {
		struct k_fifo recv_q;
		struct k_fifo accept_q;
	};

	struct {
		/** Condition variable used when receiving data */
		struct k_condvar recv;

		/** Mutex used by condition variable */
		struct k_mutex *lock;
	} cond;
#endif /* CONFIG_NET_SOCKETS */

#if defined(CONFIG_NET_OFFLOAD)
	/** context for use by offload drivers */
	void *offload_context;
#endif /* CONFIG_NET_OFFLOAD */

#if defined(CONFIG_NET_SOCKETS_CAN)
	int can_filter_id;
#endif /* CONFIG_NET_SOCKETS_CAN */

	/** Option values */
	struct {
#if defined(CONFIG_NET_CONTEXT_PRIORITY)
		/** Priority of the network data sent via this net_context */
		uint8_t priority;
#endif
#if defined(CONFIG_NET_CONTEXT_TXTIME)
		/** When to send the packet out */
		bool txtime;
#endif
#if defined(CONFIG_SOCKS)
		/** Socks proxy address */
		struct {
			struct sockaddr addr;
			socklen_t addrlen;
		} proxy;
#endif
#if defined(CONFIG_NET_CONTEXT_RCVTIMEO)
		/** Receive timeout */
		k_timeout_t rcvtimeo;
#endif
#if defined(CONFIG_NET_CONTEXT_SNDTIMEO)
		/** Send timeout */
		k_timeout_t sndtimeo;
#endif
#if defined(CONFIG_NET_CONTEXT_RCVBUF)
		/** Receive buffer maximum size */
		uint16_t rcvbuf;
#endif
#if defined(CONFIG_NET_CONTEXT_SNDBUF)
		/** Send buffer maximum size */
		uint16_t sndbuf;
#endif
#if defined(CONFIG_NET_CONTEXT_DSCP_ECN)
		/**
		 * DSCP (Differentiated Services Code point) and
		 * ECN (Explicit Congestion Notification) values.
		 */
		uint8_t dscp_ecn;
#endif
#if defined(CONFIG_NET_CONTEXT_REUSEADDR)
		/** Re-use address (SO_REUSEADDR) flag on a socket. */
		bool reuseaddr;
#endif
#if defined(CONFIG_NET_CONTEXT_REUSEPORT)
		/** Re-use port (SO_REUSEPORT) flag on a socket. */
		bool reuseport;
#endif
#if defined(CONFIG_NET_IPV4_MAPPING_TO_IPV6)
		/** Support v4-mapped-on-v6 addresses */
		bool ipv6_v6only;
#endif
#if defined(CONFIG_NET_CONTEXT_RECV_PKTINFO)
		/** Receive network packet information in recvmsg() call */
		bool recv_pktinfo;
#endif
#if defined(CONFIG_NET_IPV6)
		/**
		 * Source address selection preferences. Currently used only for IPv6,
		 * see RFC 5014 for details.
		 */
		uint16_t addr_preferences;
#endif
#if defined(CONFIG_NET_CONTEXT_TIMESTAMPING)
		/** Enable RX, TX or both timestamps of packets send through sockets. */
		uint8_t timestamping;
#endif
	} options;

	/** Protocol (UDP, TCP or IEEE 802.3 protocol value) */
	uint16_t proto;

	/** Flags for the context */
	uint16_t flags;

	/** Network interface assigned to this context */
	int8_t iface;

	/** IPv6 hop limit or IPv4 ttl for packets sent via this context. */
	union {
		struct {
			uint8_t ipv6_hop_limit;       /**< IPv6 hop limit */
			uint8_t ipv6_mcast_hop_limit; /**< IPv6 multicast hop limit */
		};
		struct {
			uint8_t ipv4_ttl;       /**< IPv4 TTL */
			uint8_t ipv4_mcast_ttl; /**< IPv4 multicast TTL */
		};
	};

#if defined(CONFIG_SOCKS)
	/** Is socks proxy enabled */
	bool proxy_enabled;
#endif

};

/**
 * @brief Is this context used or not.
 *
 * @param context Network context.
 *
 * @return True if the context is currently in use, False otherwise.
 */
static inline bool net_context_is_used(struct net_context *context)
{
	NET_ASSERT(context);

	return context->flags & NET_CONTEXT_IN_USE;
}

/**
 * @brief Is this context bound to a network interface.
 *
 * @param context Network context.
 *
 * @return True if the context is bound to network interface, False otherwise.
 */
static inline bool net_context_is_bound_to_iface(struct net_context *context)
{
	NET_ASSERT(context);

	return context->flags & NET_CONTEXT_BOUND_TO_IFACE;
}

/**
 * @brief Is this context is accepting data now.
 *
 * @param context Network context.
 *
 * @return True if the context is accepting connections, False otherwise.
 */
static inline bool net_context_is_accepting(struct net_context *context)
{
	NET_ASSERT(context);

	return context->flags & NET_CONTEXT_ACCEPTING_SOCK;
}

/**
 * @brief Set this context to accept data now.
 *
 * @param context Network context.
 * @param accepting True if accepting, False if not
 */
static inline void net_context_set_accepting(struct net_context *context,
					     bool accepting)
{
	NET_ASSERT(context);

	if (accepting) {
		context->flags |= NET_CONTEXT_ACCEPTING_SOCK;
	} else {
		context->flags &= (uint16_t)~NET_CONTEXT_ACCEPTING_SOCK;
	}
}

/**
 * @brief Is this context closing.
 *
 * @param context Network context.
 *
 * @return True if the context is closing, False otherwise.
 */
static inline bool net_context_is_closing(struct net_context *context)
{
	NET_ASSERT(context);

	return context->flags & NET_CONTEXT_CLOSING_SOCK;
}

/**
 * @brief Set this context to closing.
 *
 * @param context Network context.
 * @param closing True if closing, False if not
 */
static inline void net_context_set_closing(struct net_context *context,
					   bool closing)
{
	NET_ASSERT(context);

	if (closing) {
		context->flags |= NET_CONTEXT_CLOSING_SOCK;
	} else {
		context->flags &= (uint16_t)~NET_CONTEXT_CLOSING_SOCK;
	}
}

/** @cond INTERNAL_HIDDEN */

#define NET_CONTEXT_STATE_SHIFT 1
#define NET_CONTEXT_STATE_MASK 0x03

/** @endcond */

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

	return (enum net_context_state)
		((context->flags >> NET_CONTEXT_STATE_SHIFT) &
		NET_CONTEXT_STATE_MASK);
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

	return ((context->flags & NET_CONTEXT_FAMILY) >> 3);
}

/**
 * @brief Set address family for this network context.
 *
 * @details This function sets the address family (IPv4, IPv6 or AF_PACKET)
 * of the context.
 *
 * @param context Network context.
 * @param family Address family (AF_INET, AF_INET6, AF_PACKET, AF_CAN)
 */
static inline void net_context_set_family(struct net_context *context,
					  sa_family_t family)
{
	uint8_t flag = 0U;

	NET_ASSERT(context);

	if (family == AF_UNSPEC || family == AF_INET || family == AF_INET6 ||
	    family == AF_PACKET || family == AF_CAN) {
		/* Family is in BIT(4), BIT(5) and BIT(6) */
		flag = (uint8_t)(family << 3);
	}

	context->flags |= flag;
}

/**
 * @brief Get context type for this network context.
 *
 * @details This function returns the context type (stream, datagram or raw)
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

	return (enum net_sock_type)((context->flags & NET_CONTEXT_TYPE) >> 6);
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
	uint16_t flag = 0U;

	NET_ASSERT(context);

	if (type == SOCK_DGRAM || type == SOCK_STREAM || type == SOCK_RAW) {
		/* Type is in BIT(6) and BIT(7)*/
		flag = (uint16_t)(type << 6);
	}

	context->flags |= flag;
}

/**
 * @brief Set CAN filter id for this network context.
 *
 * @details This function sets the CAN filter id of the context.
 *
 * @param context Network context.
 * @param filter_id CAN filter id
 */
#if defined(CONFIG_NET_SOCKETS_CAN)
static inline void net_context_set_can_filter_id(struct net_context *context,
					     int filter_id)
{
	NET_ASSERT(context);

	context->can_filter_id = filter_id;
}
#else
static inline void net_context_set_can_filter_id(struct net_context *context,
					     int filter_id)
{
	ARG_UNUSED(context);
	ARG_UNUSED(filter_id);
}
#endif

/**
 * @brief Get CAN filter id for this network context.
 *
 * @details This function gets the CAN filter id of the context.
 *
 * @param context Network context.
 *
 * @return Filter id of this network context
 */
#if defined(CONFIG_NET_SOCKETS_CAN)
static inline int net_context_get_can_filter_id(struct net_context *context)
{
	NET_ASSERT(context);

	return context->can_filter_id;
}
#else
static inline int net_context_get_can_filter_id(struct net_context *context)
{
	ARG_UNUSED(context);

	return -1;
}
#endif

/**
 * @brief Get context IP protocol for this network context.
 *
 * @details This function returns the IP protocol (UDP / TCP /
 * IEEE 802.3 protocol value) of the context.
 *
 * @param context Network context.
 *
 * @return Network context IP protocol.
 */
static inline uint16_t net_context_get_proto(struct net_context *context)
{
	return context->proto;
}

/**
 * @brief Set context IP protocol for this network context.
 *
 * @details This function sets the context IP protocol (UDP / TCP)
 * of the context.
 *
 * @param context Network context.
 * @param proto Context IP protocol (IPPROTO_UDP, IPPROTO_TCP or IEEE 802.3
 * protocol value)
 */
static inline void net_context_set_proto(struct net_context *context,
					 uint16_t proto)
{
	context->proto = proto;
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

	context->iface = (uint8_t)net_if_get_by_iface(iface);
}

/**
 * @brief Bind network interface to this context.
 *
 * @details This function binds network interface to this context.
 *
 * @param context Network context.
 * @param iface Network interface.
 */
static inline void net_context_bind_iface(struct net_context *context,
					  struct net_if *iface)
{
	NET_ASSERT(iface);

	context->flags |= NET_CONTEXT_BOUND_TO_IFACE;
	net_context_set_iface(context, iface);
}

/**
 * @brief Get IPv4 TTL (time-to-live) value for this context.
 *
 * @details This function returns the IPv4 TTL (time-to-live) value that is
 *          set to this context.
 *
 * @param context Network context.
 *
 * @return IPv4 TTL value
 */
static inline uint8_t net_context_get_ipv4_ttl(struct net_context *context)
{
	return context->ipv4_ttl;
}

/**
 * @brief Set IPv4 TTL (time-to-live) value for this context.
 *
 * @details This function sets the IPv4 TTL (time-to-live) value for
 *          this context.
 *
 * @param context Network context.
 * @param ttl IPv4 time-to-live value.
 */
static inline void net_context_set_ipv4_ttl(struct net_context *context,
					    uint8_t ttl)
{
	context->ipv4_ttl = ttl;
}

/**
 * @brief Get IPv4 multicast TTL (time-to-live) value for this context.
 *
 * @details This function returns the IPv4 multicast TTL (time-to-live) value
 *          that is set to this context.
 *
 * @param context Network context.
 *
 * @return IPv4 multicast TTL value
 */
static inline uint8_t net_context_get_ipv4_mcast_ttl(struct net_context *context)
{
	return context->ipv4_mcast_ttl;
}

/**
 * @brief Set IPv4 multicast TTL (time-to-live) value for this context.
 *
 * @details This function sets the IPv4 multicast TTL (time-to-live) value for
 *          this context.
 *
 * @param context Network context.
 * @param ttl IPv4 multicast time-to-live value.
 */
static inline void net_context_set_ipv4_mcast_ttl(struct net_context *context,
						  uint8_t ttl)
{
	context->ipv4_mcast_ttl = ttl;
}

/**
 * @brief Get IPv6 hop limit value for this context.
 *
 * @details This function returns the IPv6 hop limit value that is set to this
 *          context.
 *
 * @param context Network context.
 *
 * @return IPv6 hop limit value
 */
static inline uint8_t net_context_get_ipv6_hop_limit(struct net_context *context)
{
	return context->ipv6_hop_limit;
}

/**
 * @brief Set IPv6 hop limit value for this context.
 *
 * @details This function sets the IPv6 hop limit value for this context.
 *
 * @param context Network context.
 * @param hop_limit IPv6 hop limit value.
 */
static inline void net_context_set_ipv6_hop_limit(struct net_context *context,
						  uint8_t hop_limit)
{
	context->ipv6_hop_limit = hop_limit;
}

/**
 * @brief Get IPv6 multicast hop limit value for this context.
 *
 * @details This function returns the IPv6 multicast hop limit value
 *          that is set to this context.
 *
 * @param context Network context.
 *
 * @return IPv6 multicast hop limit value
 */
static inline uint8_t net_context_get_ipv6_mcast_hop_limit(struct net_context *context)
{
	return context->ipv6_mcast_hop_limit;
}

/**
 * @brief Set IPv6 multicast hop limit value for this context.
 *
 * @details This function sets the IPv6 multicast hop limit value for
 *          this context.
 *
 * @param context Network context.
 * @param hop_limit IPv6 multicast hop limit value.
 */
static inline void net_context_set_ipv6_mcast_hop_limit(struct net_context *context,
							uint8_t hop_limit)
{
	context->ipv6_mcast_hop_limit = hop_limit;
}

/**
 * @brief Enable or disable socks proxy support for this context.
 *
 * @details This function either enables or disables socks proxy support for
 *          this context.
 *
 * @param context Network context.
 * @param enable Enable socks proxy or disable it.
 */
#if defined(CONFIG_SOCKS)
static inline void net_context_set_proxy_enabled(struct net_context *context,
						 bool enable)
{
	context->proxy_enabled = enable;
}
#else
static inline void net_context_set_proxy_enabled(struct net_context *context,
						 bool enable)
{
	ARG_UNUSED(context);
	ARG_UNUSED(enable);
}
#endif

/**
 * @brief Is socks proxy support enabled or disabled for this context.
 *
 * @details This function returns current socks proxy status for
 *          this context.
 *
 * @param context Network context.
 *
 * @return True if socks proxy is enabled for this context, False otherwise
 */
#if defined(CONFIG_SOCKS)
static inline bool net_context_is_proxy_enabled(struct net_context *context)
{
	return context->proxy_enabled;
}
#else
static inline bool net_context_is_proxy_enabled(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}
#endif

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
 * @param ip_proto IP protocol, IPPROTO_UDP or IPPROTO_TCP. For raw socket
 * access, the value is the L2 protocol value from IEEE 802.3 (see ethernet.h)
 * @param context The allocated context is returned to the caller.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_get(sa_family_t family,
		    enum net_sock_type type,
		    uint16_t ip_proto,
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
 * @brief Create IPv4 packet in provided net_pkt from context
 *
 * @param context Network context for a connection
 * @param pkt Network packet
 * @param src Source address, or NULL to choose a default
 * @param dst Destination IPv4 address
 *
 * @return Return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_IPV4)
int net_context_create_ipv4_new(struct net_context *context,
				struct net_pkt *pkt,
				const struct in_addr *src,
				const struct in_addr *dst);
#else
static inline int net_context_create_ipv4_new(struct net_context *context,
					      struct net_pkt *pkt,
					      const struct in_addr *src,
					      const struct in_addr *dst)
{
	return -1;
}
#endif /* CONFIG_NET_IPV4 */

/**
 * @brief Create IPv6 packet in provided net_pkt from context
 *
 * @param context Network context for a connection
 * @param pkt Network packet
 * @param src Source address, or NULL to choose a default from context
 * @param dst Destination IPv6 address
 *
 * @return Return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_IPV6)
int net_context_create_ipv6_new(struct net_context *context,
				struct net_pkt *pkt,
				const struct in6_addr *src,
				const struct in6_addr *dst);
#else
static inline int net_context_create_ipv6_new(struct net_context *context,
					      struct net_pkt *pkt,
					      const struct in6_addr *src,
					      const struct in6_addr *dst)
{
	ARG_UNUSED(context);
	ARG_UNUSED(pkt);
	ARG_UNUSED(src);
	ARG_UNUSED(dst);
	return -1;
}
#endif /* CONFIG_NET_IPV6 */

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
 *                   connection is established, the user-supplied callback (cb)
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
			k_timeout_t timeout,
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
 * After the connection is established a caller-supplied callback is called.
 * The callback is called even if timeout was set to K_FOREVER, the
 * callback is called before this function will return in this case.
 * The callback is not called if the timeout expires.
 * This is similar as BSD accept() function.
 *
 * @param context The context to use.
 * @param cb Caller-supplied callback function.
 * @param timeout Timeout for the connection. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_accept(struct net_context *context,
		       net_tcp_accept_cb_t cb,
		       k_timeout_t timeout,
		       void *user_data);

/**
 * @brief Send data to a peer.
 *
 * @details This function can be used to send network data to a peer
 * connection. After the network buffer is sent, a caller-supplied
 * callback is called. Note that the callback might be called after this
 * function has returned. For context of type SOCK_DGRAM, the destination
 * address must have been set by the call to net_context_connect().
 * This is similar as BSD send() function.
 *
 * @param context The network context to use.
 * @param buf The data buffer to send
 * @param len Length of the buffer
 * @param cb Caller-supplied callback function.
 * @param timeout Currently this value is not used.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_send(struct net_context *context,
		     const void *buf,
		     size_t len,
		     net_context_send_cb_t cb,
		     k_timeout_t timeout,
		     void *user_data);

/**
 * @brief Send data to a peer specified by address.
 *
 * @details This function can be used to send network data to a peer
 * specified by address. This variant can only be used for datagram
 * connections of type SOCK_DGRAM. After the network buffer is sent,
 * a caller-supplied callback is called. Note that the callback might be
 * called after this function has returned.
 * This is similar as BSD sendto() function.
 *
 * @param context The network context to use.
 * @param buf The data buffer to send
 * @param len Length of the buffer
 * @param dst_addr Destination address.
 * @param addrlen Length of the address.
 * @param cb Caller-supplied callback function.
 * @param timeout Currently this value is not used.
 * @param user_data Caller-supplied user data.
 *
 * @return numbers of bytes sent on success, a negative errno otherwise
 */
int net_context_sendto(struct net_context *context,
		       const void *buf,
		       size_t len,
		       const struct sockaddr *dst_addr,
		       socklen_t addrlen,
		       net_context_send_cb_t cb,
		       k_timeout_t timeout,
		       void *user_data);

/**
 * @brief Send data in iovec to a peer specified in msghdr struct.
 *
 * @details This function has similar semantics as Posix sendmsg() call.
 * For unconnected socket, the msg_name field in msghdr must be set. For
 * connected socket the msg_name should be set to NULL, and msg_namelen to 0.
 * After the network buffer is sent, a caller-supplied callback is called.
 * Note that the callback might be called after this function has returned.
 *
 * @param context The network context to use.
 * @param msghdr The data to send
 * @param flags Flags for the sending.
 * @param cb Caller-supplied callback function.
 * @param timeout Currently this value is not used.
 * @param user_data Caller-supplied user data.
 *
 * @return numbers of bytes sent on success, a negative errno otherwise
 */
int net_context_sendmsg(struct net_context *context,
			const struct msghdr *msghdr,
			int flags,
			net_context_send_cb_t cb,
			k_timeout_t timeout,
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
 * After the network buffer is received, a caller-supplied callback is
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
 * update connection information from context. If recv() is called before
 * bind() call, it may refuse to bind to a context which already has
 * a connection associated.
 *
 * @param context The network context to use.
 * @param cb Caller-supplied callback function.
 * @param timeout Caller-supplied timeout. Possible values
 * are K_FOREVER, K_NO_WAIT, >0.
 * @param user_data Caller-supplied user data.
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     k_timeout_t timeout,
		     void *user_data);

/**
 * @brief Update TCP receive window for context.
 *
 * @details This function should be used by an application which
 * doesn't fully process incoming data in its receive callback,
 * but for example, queues it. In this case, receive callback
 * should decrease the window (call this function with a negative
 * value) by the size of queued data, and function(s) which dequeue
 * data - with positive value corresponding to the dequeued size.
 * For example, if receive callback gets a packet with the data
 * size of 256 and queues it, it should call this function with
 * delta of -256. If a function extracts 10 bytes of the queued
 * data, it should call it with delta of 10.
 *
 * @param context The TCP network context to use.
 * @param delta Size, in bytes, by which to increase TCP receive
 * window (negative value to decrease).
 *
 * @return 0 if ok, < 0 if error
 */
int net_context_update_recv_wnd(struct net_context *context,
				int32_t delta);

/** @brief Network context options. These map to BSD socket option values. */
enum net_context_option {
	NET_OPT_PRIORITY          = 1,  /**< Context priority */
	NET_OPT_TXTIME            = 2,  /**< TX time */
	NET_OPT_SOCKS5            = 3,  /**< SOCKS5 */
	NET_OPT_RCVTIMEO          = 4,  /**< Receive timeout */
	NET_OPT_SNDTIMEO          = 5,  /**< Send timeout */
	NET_OPT_RCVBUF            = 6,  /**< Receive buffer */
	NET_OPT_SNDBUF            = 7,  /**< Send buffer */
	NET_OPT_DSCP_ECN          = 8,  /**< DSCP ECN */
	NET_OPT_REUSEADDR         = 9,  /**< Re-use address */
	NET_OPT_REUSEPORT         = 10, /**< Re-use port */
	NET_OPT_IPV6_V6ONLY       = 11, /**< Share IPv4 and IPv6 port space */
	NET_OPT_RECV_PKTINFO      = 12, /**< Receive packet information */
	NET_OPT_MCAST_TTL         = 13, /**< IPv4 multicast TTL */
	NET_OPT_MCAST_HOP_LIMIT   = 14, /**< IPv6 multicast hop limit */
	NET_OPT_UNICAST_HOP_LIMIT = 15, /**< IPv6 unicast hop limit */
	NET_OPT_TTL               = 16, /**< IPv4 unicast TTL */
	NET_OPT_ADDR_PREFERENCES  = 17, /**< IPv6 address preference */
	NET_OPT_TIMESTAMPING      = 18, /**< Packet timestamping */
};

/**
 * @brief Set an connection option for this context.
 *
 * @param context The network context to use.
 * @param option Option to set
 * @param value Option value
 * @param len Option length
 *
 * @return 0 if ok, <0 if error
 */
int net_context_set_option(struct net_context *context,
			   enum net_context_option option,
			   const void *value, size_t len);

/**
 * @brief Get connection option value for this context.
 *
 * @param context The network context to use.
 * @param option Option to set
 * @param value Option value
 * @param len Option length (returned to caller)
 *
 * @return 0 if ok, <0 if error
 */
int net_context_get_option(struct net_context *context,
			   enum net_context_option option,
			   void *value, size_t *len);

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
 * @param cb User-supplied callback function to call.
 * @param user_data User specified data.
 */
void net_context_foreach(net_context_cb_t cb, void *user_data);

/**
 * @brief Set custom network buffer pools for context send operations
 *
 * Set custom network buffer pools used by the IP stack to allocate
 * network buffers used by the context when sending data to the
 * network. Using dedicated buffers may help make send operations on
 * a given context more reliable, e.g. not be subject to buffer
 * starvation due to operations on other network contexts. Buffer pools
 * are set per context, but several contexts may share the same buffers.
 * Note that there's no support for per-context custom receive packet
 * pools.
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

	context->tx_slab = tx_slab;
	context->data_pool = data_pool;
}
#else
#define net_context_setup_pools(context, tx_pool, data_pool)
#endif

/**
 * @brief Check if a port is in use (bound)
 *
 * This function checks if a port is bound with respect to the specified
 * @p ip_proto and @p local_addr.
 *
 * @param ip_proto the IP protocol
 * @param local_port the port to check
 * @param local_addr the network address
 *
 * @return true if the port is bound
 * @return false if the port is not bound
 */
bool net_context_port_in_use(enum net_ip_protocol ip_proto,
	uint16_t local_port, const struct sockaddr *local_addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_CONTEXT_H_ */
