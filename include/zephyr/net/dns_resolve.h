/** @file
 * @brief DNS resolving library
 *
 * An API for applications to resolve a DNS name.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DNS_RESOLVE_H_
#define ZEPHYR_INCLUDE_NET_DNS_RESOLVE_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/net/net_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DNS resolving library
 * @defgroup dns_resolve DNS Resolve Library
 * @ingroup networking
 * @{
 */

/**
 * DNS query type enum
 */
enum dns_query_type {
	/** IPv4 query */
	DNS_QUERY_TYPE_A = 1,
	/** IPv6 query */
	DNS_QUERY_TYPE_AAAA = 28
};

/** Max size of the resolved name. */
#ifndef DNS_MAX_NAME_SIZE
#define DNS_MAX_NAME_SIZE 20
#endif

/** @cond INTERNAL_HIDDEN */

#define DNS_BUF_TIMEOUT K_MSEC(500) /* ms */

/* This value is recommended by RFC 1035 */
#define DNS_RESOLVER_MAX_BUF_SIZE	512

/* Make sure that we can compile things even if CONFIG_DNS_RESOLVER
 * is not enabled.
 */
#if defined(CONFIG_DNS_RESOLVER_MAX_SERVERS)
#define DNS_RESOLVER_MAX_SERVERS CONFIG_DNS_RESOLVER_MAX_SERVERS
#else
#define DNS_RESOLVER_MAX_SERVERS 0
#endif

#if defined(CONFIG_DNS_NUM_CONCUR_QUERIES)
#define DNS_NUM_CONCUR_QUERIES CONFIG_DNS_NUM_CONCUR_QUERIES
#else
#define DNS_NUM_CONCUR_QUERIES 1
#endif

#if defined(CONFIG_NET_IF_MAX_IPV6_COUNT)
#define MAX_IPV6_IFACE_COUNT CONFIG_NET_IF_MAX_IPV6_COUNT
#else
#define MAX_IPV6_IFACE_COUNT 1
#endif

#if defined(CONFIG_NET_IF_MAX_IPV4_COUNT)
#define MAX_IPV4_IFACE_COUNT CONFIG_NET_IF_MAX_IPV4_COUNT
#else
#define MAX_IPV4_IFACE_COUNT 1
#endif

/* If mDNS is enabled, then add some extra well known multicast servers to the
 * server list.
 */
#if defined(CONFIG_MDNS_RESOLVER)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define MDNS_SERVER_COUNT 2
#else
#define MDNS_SERVER_COUNT 1
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#else
#define MDNS_SERVER_COUNT 0
#endif /* CONFIG_MDNS_RESOLVER */

/* If LLMNR is enabled, then add some extra well known multicast servers to the
 * server list.
 */
#if defined(CONFIG_LLMNR_RESOLVER)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define LLMNR_SERVER_COUNT 2
#else
#define LLMNR_SERVER_COUNT 1
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#else
#define LLMNR_SERVER_COUNT 0
#endif /* CONFIG_MDNS_RESOLVER */

#define DNS_MAX_MCAST_SERVERS (MDNS_SERVER_COUNT + LLMNR_SERVER_COUNT)

#if defined(CONFIG_MDNS_RESPONDER)
#if defined(CONFIG_NET_IPV6)
#define MDNS_MAX_IPV6_IFACE_COUNT CONFIG_NET_IF_MAX_IPV6_COUNT
#else
#define MDNS_MAX_IPV6_IFACE_COUNT 0
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
#define MDNS_MAX_IPV4_IFACE_COUNT CONFIG_NET_IF_MAX_IPV4_COUNT
#else
#define MDNS_MAX_IPV4_IFACE_COUNT 0
#endif /* CONFIG_NET_IPV4 */

#define MDNS_MAX_POLL (MDNS_MAX_IPV4_IFACE_COUNT + MDNS_MAX_IPV6_IFACE_COUNT)
#else
#define MDNS_MAX_POLL 0
#endif /* CONFIG_MDNS_RESPONDER */

#if defined(CONFIG_LLMNR_RESPONDER)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define LLMNR_MAX_POLL 2
#else
#define LLMNR_MAX_POLL 1
#endif
#else
#define LLMNR_MAX_POLL 0
#endif /* CONFIG_LLMNR_RESPONDER */

#define DNS_RESOLVER_MAX_POLL (DNS_RESOLVER_MAX_SERVERS + DNS_MAX_MCAST_SERVERS)

/** How many sockets the dispatcher is able to poll. */
#define DNS_DISPATCHER_MAX_POLL (DNS_RESOLVER_MAX_POLL + MDNS_MAX_POLL + LLMNR_MAX_POLL)

#if defined(CONFIG_NET_SOCKETS_POLL_MAX)
BUILD_ASSERT(CONFIG_NET_SOCKETS_POLL_MAX >= DNS_DISPATCHER_MAX_POLL,
	     "CONFIG_NET_SOCKETS_POLL_MAX must be larger than " STRINGIFY(DNS_DISPATCHER_MAX_POLL));
#endif

/** @brief What is the type of the socket given to DNS socket dispatcher,
 * resolver or responder.
 */
enum dns_socket_type {
	DNS_SOCKET_RESOLVER = 1,  /**< Socket is used for resolving (client type) */
	DNS_SOCKET_RESPONDER = 2  /**< Socket is used for responding (server type) */
};

struct dns_resolve_context;
struct mdns_responder_context;

/**
 * @typedef dns_socket_dispatcher_cb
 * @brief Callback used when the DNS socket dispatcher has found a handler for
 * this type of socket.
 *
 * @param ctx DNS resolve or mDNS responder context.
 * @param sock Socket which is seeing traffic.
 * @param addr Socket address of the peer that sent the DNS packet.
 * @param addrlen Length of the socket address.
 * @param buf Pointer to data buffer containing the DNS message.
 * @param data_len Length of the data in buffer chain.
 *
 * @return 0 if ok, <0 if error
 */
typedef int (*dns_socket_dispatcher_cb)(void *ctx, int sock,
					struct sockaddr *addr, size_t addrlen,
					struct net_buf *buf, size_t data_len);

/** @brief DNS socket dispatcher context. */
struct dns_socket_dispatcher {
	/** slist node for the different dispatcher contexts */
	sys_snode_t node;
	/** Socket service for this dispatcher instance */
	const struct net_socket_service_desc *svc;
	/** DNS resolver context that contains information needed by the
	 * resolver/responder handler, or mDNS responder context.
	 */
	union {
		void *ctx;
		struct dns_resolve_context *resolve_ctx;
		struct mdns_responder_context *mdns_ctx;
	};

	/** Type of the socket (resolver / responder) */
	enum dns_socket_type type;
	/** Local endpoint address (used when binding the socket) */
	struct sockaddr local_addr;
	/** DNS socket dispatcher callback is called for incoming traffic */
	dns_socket_dispatcher_cb cb;
	/** Socket descriptors to poll */
	struct zsock_pollfd *fds;
	/** Length of the poll array */
	int fds_len;
	/** Local socket to dispatch */
	int sock;
	/** There can be two contexts to dispatch. This points to the other
	 * context if sharing the socket between resolver / responder.
	 */
	struct dns_socket_dispatcher *pair;
	/** Mutex lock protecting access to this dispatcher context */
	struct k_mutex lock;
	/** Buffer allocation timeout */
	k_timeout_t buf_timeout;
};

struct mdns_responder_context {
	struct sockaddr server_addr;
	struct dns_socket_dispatcher dispatcher;
	struct zsock_pollfd fds[1];
	int sock;
};

/**
 * @brief Register a DNS dispatcher socket. Each code wanting to use
 * the dispatcher needs to create the dispatcher context and call
 * this function.
 *
 * @param ctx DNS socket dispatcher context.
 *
 * @return 0 if ok, <1 if error
 */
int dns_dispatcher_register(struct dns_socket_dispatcher *ctx);

/**
 * @brief Unregister a DNS dispatcher socket. Called when the
 * resolver/responder no longer needs to receive traffic for the
 * socket.
 *
 * @param ctx DNS socket dispatcher context.
 *
 * @return 0 if ok, <1 if error
 */
int dns_dispatcher_unregister(struct dns_socket_dispatcher *ctx);

/** @endcond */

/**
 * Address info struct is passed to callback that gets all the results.
 */
struct dns_addrinfo {
	/** IP address information */
	struct sockaddr ai_addr;
	/** Length of the ai_addr field */
	socklen_t       ai_addrlen;
	/** Address family of the address information */
	uint8_t         ai_family;
	/** Canonical name of the address */
	char            ai_canonname[DNS_MAX_NAME_SIZE + 1];
};

/**
 * Status values for the callback.
 */
enum dns_resolve_status {
	/** Invalid value for `ai_flags' field */
	DNS_EAI_BADFLAGS    = -1,
	/** NAME or SERVICE is unknown */
	DNS_EAI_NONAME      = -2,
	/** Temporary failure in name resolution */
	DNS_EAI_AGAIN       = -3,
	/** Non-recoverable failure in name res */
	DNS_EAI_FAIL        = -4,
	/** No address associated with NAME */
	DNS_EAI_NODATA      = -5,
	/** `ai_family' not supported */
	DNS_EAI_FAMILY      = -6,
	/** `ai_socktype' not supported */
	DNS_EAI_SOCKTYPE    = -7,
	/** SRV not supported for `ai_socktype' */
	DNS_EAI_SERVICE     = -8,
	/** Address family for NAME not supported */
	DNS_EAI_ADDRFAMILY  = -9,
	/** Memory allocation failure */
	DNS_EAI_MEMORY      = -10,
	/** System error returned in `errno' */
	DNS_EAI_SYSTEM      = -11,
	/** Argument buffer overflow */
	DNS_EAI_OVERFLOW    = -12,
	/** Processing request in progress */
	DNS_EAI_INPROGRESS  = -100,
	/** Request canceled */
	DNS_EAI_CANCELED    = -101,
	/** Request not canceled */
	DNS_EAI_NOTCANCELED = -102,
	/** All requests done */
	DNS_EAI_ALLDONE     = -103,
	/** IDN encoding failed */
	DNS_EAI_IDN_ENCODE  = -105,
};

/**
 * @typedef dns_resolve_cb_t
 * @brief DNS resolve callback
 *
 * @details The DNS resolve callback is called after a successful
 * DNS resolving. The resolver can call this callback multiple times, one
 * for each resolved address.
 *
 * @param status The status of the query:
 *  DNS_EAI_INPROGRESS returned for each resolved address
 *  DNS_EAI_ALLDONE    mark end of the resolving, info is set to NULL in
 *                     this case
 *  DNS_EAI_CANCELED   if the query was canceled manually or timeout happened
 *  DNS_EAI_FAIL       if the name cannot be resolved by the server
 *  DNS_EAI_NODATA     if there is no such name
 *  other values means that an error happened.
 * @param info Query results are stored here.
 * @param user_data The user data given in dns_resolve_name() call.
 */
typedef void (*dns_resolve_cb_t)(enum dns_resolve_status status,
				 struct dns_addrinfo *info,
				 void *user_data);

/** @cond INTERNAL_HIDDEN */

enum dns_resolve_context_state {
	DNS_RESOLVE_CONTEXT_ACTIVE,
	DNS_RESOLVE_CONTEXT_DEACTIVATING,
	DNS_RESOLVE_CONTEXT_INACTIVE,
};

/** @endcond */

/**
 * DNS resolve context structure.
 */
struct dns_resolve_context {
	/** List of configured DNS servers */
	struct dns_server {
		/** DNS server information */
		struct sockaddr dns_server;

		/** Connection to the DNS server */
		int sock;

		/** Is this server mDNS one */
		uint8_t is_mdns : 1;

		/** Is this server LLMNR one */
		uint8_t is_llmnr : 1;

/** @cond INTERNAL_HIDDEN */
		/** Dispatch DNS data between resolver and responder */
		struct dns_socket_dispatcher dispatcher;
/** @endcond */
	} servers[DNS_RESOLVER_MAX_POLL];

/** @cond INTERNAL_HIDDEN */
	/** Socket polling for each server connection */
	struct zsock_pollfd fds[DNS_RESOLVER_MAX_POLL];
/** @endcond */

	/** Prevent concurrent access */
	struct k_mutex lock;

	/** This timeout is also used when a buffer is required from the
	 * buffer pools.
	 */
	k_timeout_t buf_timeout;

	/** Result callbacks. We have multiple callbacks here so that it is
	 * possible to do multiple queries at the same time.
	 *
	 * Contents of this structure can be inspected and changed only when
	 * the lock is held.
	 */
	struct dns_pending_query {
		/** Timeout timer */
		struct k_work_delayable timer;

		/** Back pointer to ctx, needed in timeout handler */
		struct dns_resolve_context *ctx;

		/** Result callback.
		 *
		 * A null value indicates the slot is not in use.
		 */
		dns_resolve_cb_t cb;

		/** User data */
		void *user_data;

		/** TX timeout */
		k_timeout_t timeout;

		/** String containing the thing to resolve like www.example.com
		 *
		 * This is set to a non-null value when the query is started,
		 * and is not used thereafter.
		 *
		 * If the query completed at a point where the work item was
		 * still pending the pointer is cleared to indicate that the
		 * query is complete, but release of the query slot will be
		 * deferred until a request for a slot determines that the
		 * work item has been released.
		 */
		const char *query;

		/** Query type */
		enum dns_query_type query_type;

		/** DNS id of this query */
		uint16_t id;

		/** Hash of the DNS name + query type we are querying.
		 * This hash is calculated so we can match the response that
		 * we are receiving. This is needed mainly for mDNS which is
		 * setting the DNS id to 0, which means that the id alone
		 * cannot be used to find correct pending query.
		 */
		uint16_t query_hash;
	} queries[DNS_NUM_CONCUR_QUERIES];

	/** Is this context in use */
	enum dns_resolve_context_state state;
};

/**
 * @brief Init DNS resolving context.
 *
 * @details This function sets the DNS server address and initializes the
 * DNS context that is used by the actual resolver. DNS server addresses
 * can be specified either in textual form, or as struct sockaddr (or both).
 * Note that the recommended way to resolve DNS names is to use
 * the dns_get_addr_info() API. In that case user does not need to
 * call dns_resolve_init() as the DNS servers are already setup by the system.
 *
 * @param ctx DNS context. If the context variable is allocated from
 * the stack, then the variable needs to be valid for the whole duration of
 * the resolving. Caller does not need to fill the variable beforehand or
 * edit the context afterwards.
 * @param dns_servers_str DNS server addresses using textual strings. The
 * array is NULL terminated. The port number can be given in the string.
 * Syntax for the server addresses with or without port numbers:
 *    IPv4        : 10.0.9.1
 *    IPv4 + port : 10.0.9.1:5353
 *    IPv6        : 2001:db8::22:42
 *    IPv6 + port : [2001:db8::22:42]:5353
 * @param dns_servers_sa DNS server addresses as struct sockaddr. The array
 * is NULL terminated. Port numbers are optional in struct sockaddr, the
 * default will be used if set to 0.
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_init(struct dns_resolve_context *ctx,
		     const char *dns_servers_str[],
		     const struct sockaddr *dns_servers_sa[]);

/**
 * @brief Init DNS resolving context with default Kconfig options.
 *
 * @param ctx DNS context.
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_init_default(struct dns_resolve_context *ctx);

/**
 * @brief Close DNS resolving context.
 *
 * @details This releases DNS resolving context and marks the context unusable.
 * Caller must call the dns_resolve_init() again to make context usable.
 *
 * @param ctx DNS context
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_close(struct dns_resolve_context *ctx);

/**
 * @brief Reconfigure DNS resolving context.
 *
 * @details Reconfigures DNS context with new server list.
 *
 * @param ctx DNS context
 * @param servers_str DNS server addresses using textual strings. The
 * array is NULL terminated. The port number can be given in the string.
 * Syntax for the server addresses with or without port numbers:
 *    IPv4        : 10.0.9.1
 *    IPv4 + port : 10.0.9.1:5353
 *    IPv6        : 2001:db8::22:42
 *    IPv6 + port : [2001:db8::22:42]:5353
 * @param servers_sa DNS server addresses as struct sockaddr. The array
 * is NULL terminated. Port numbers are optional in struct sockaddr, the
 * default will be used if set to 0.
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_reconfigure(struct dns_resolve_context *ctx,
			    const char *servers_str[],
			    const struct sockaddr *servers_sa[]);

/**
 * @brief Cancel a pending DNS query.
 *
 * @details This releases DNS resources used by a pending query.
 *
 * @param ctx DNS context
 * @param dns_id DNS id of the pending query
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_cancel(struct dns_resolve_context *ctx,
		       uint16_t dns_id);

/**
 * @brief Cancel a pending DNS query using id, name and type.
 *
 * @details This releases DNS resources used by a pending query.
 *
 * @param ctx DNS context
 * @param dns_id DNS id of the pending query
 * @param query_name Name of the resource we are trying to query (hostname)
 * @param query_type Type of the query (A or AAAA)
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_cancel_with_name(struct dns_resolve_context *ctx,
				 uint16_t dns_id,
				 const char *query_name,
				 enum dns_query_type query_type);

/**
 * @brief Resolve DNS name.
 *
 * @details This function can be used to resolve e.g., IPv4 or IPv6 address.
 * Note that this is asynchronous call, the function will return immediately
 * and system will call the callback after resolving has finished or timeout
 * has occurred.
 * We might send the query to multiple servers (if there are more than one
 * server configured), but we only use the result of the first received
 * response.
 *
 * @param ctx DNS context
 * @param query What the caller wants to resolve.
 * @param type What kind of data the caller wants to get.
 * @param dns_id DNS id is returned to the caller. This is needed if one
 * wishes to cancel the query. This can be set to NULL if there is no need
 * to cancel the query.
 * @param cb Callback to call after the resolving has finished or timeout
 * has happened.
 * @param user_data The user data.
 * @param timeout The timeout value for the query. Possible values:
 * SYS_FOREVER_MS: the query is tried forever, user needs to cancel it
 *            manually if it takes too long time to finish
 * >0: start the query and let the system timeout it after specified ms
 *
 * @return 0 if resolving was started ok, < 0 otherwise
 */
int dns_resolve_name(struct dns_resolve_context *ctx,
		     const char *query,
		     enum dns_query_type type,
		     uint16_t *dns_id,
		     dns_resolve_cb_t cb,
		     void *user_data,
		     int32_t timeout);

/**
 * @brief Get default DNS context.
 *
 * @details The system level DNS context uses DNS servers that are
 * defined in project config file. If no DNS servers are defined by the
 * user, then resolving DNS names using default DNS context will do nothing.
 * The configuration options are described in subsys/net/lib/dns/Kconfig file.
 *
 * @return Default DNS context.
 */
struct dns_resolve_context *dns_resolve_get_default(void);

/**
 * @brief Get IP address info from DNS.
 *
 * @details This function can be used to resolve e.g., IPv4 or IPv6 address.
 * Note that this is asynchronous call, the function will return immediately
 * and system will call the callback after resolving has finished or timeout
 * has occurred.
 * We might send the query to multiple servers (if there are more than one
 * server configured), but we only use the result of the first received
 * response.
 * This variant uses system wide DNS servers.
 *
 * @param query What the caller wants to resolve.
 * @param type What kind of data the caller wants to get.
 * @param dns_id DNS id is returned to the caller. This is needed if one
 * wishes to cancel the query. This can be set to NULL if there is no need
 * to cancel the query.
 * @param cb Callback to call after the resolving has finished or timeout
 * has happened.
 * @param user_data The user data.
 * @param timeout The timeout value for the connection. Possible values:
 * SYS_FOREVER_MS: the query is tried forever, user needs to cancel it
 *            manually if it takes too long time to finish
 * >0: start the query and let the system timeout it after specified ms
 *
 * @return 0 if resolving was started ok, < 0 otherwise
 */
static inline int dns_get_addr_info(const char *query,
				    enum dns_query_type type,
				    uint16_t *dns_id,
				    dns_resolve_cb_t cb,
				    void *user_data,
				    int32_t timeout)
{
	return dns_resolve_name(dns_resolve_get_default(),
				query,
				type,
				dns_id,
				cb,
				user_data,
				timeout);
}

/**
 * @brief Cancel a pending DNS query.
 *
 * @details This releases DNS resources used by a pending query.
 *
 * @param dns_id DNS id of the pending query
 *
 * @return 0 if ok, <0 if error.
 */
static inline int dns_cancel_addr_info(uint16_t dns_id)
{
	return dns_resolve_cancel(dns_resolve_get_default(), dns_id);
}

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initialize DNS subsystem.
 */
#if defined(CONFIG_DNS_RESOLVER_AUTO_INIT)
void dns_init_resolver(void);

#else
#define dns_init_resolver(...)
#endif /* CONFIG_DNS_RESOLVER_AUTO_INIT */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DNS_RESOLVE_H_ */
