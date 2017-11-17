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

#ifndef _DNS_RESOLVE_H
#define _DNS_RESOLVE_H

#include <net/net_ip.h>
#include <net/net_context.h>

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
	DNS_QUERY_TYPE_A = 1,	 /* IPv4 */
	DNS_QUERY_TYPE_AAAA = 28 /* IPv6 */
};

#ifndef DNS_MAX_NAME_SIZE
#define DNS_MAX_NAME_SIZE 20
#endif

/* Make sure that we can compile things even if CONFIG_DNS_RESOLVER
 * is not enabled.
 */
#if !defined(CONFIG_DNS_RESOLVER_MAX_SERVERS)
#define CONFIG_DNS_RESOLVER_MAX_SERVERS 1
#endif
#if !defined(CONFIG_DNS_NUM_CONCUR_QUERIES)
#define CONFIG_DNS_NUM_CONCUR_QUERIES 1
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

/**
 * Address info struct is passed to callback that gets all the results.
 */
struct dns_addrinfo {
	struct sockaddr ai_addr;
	socklen_t       ai_addrlen;
	u8_t            ai_family;
	char            ai_canonname[DNS_MAX_NAME_SIZE + 1];
};

/**
 * Status values for the callback.
 */
enum dns_resolve_status {
	DNS_EAI_BADFLAGS    = -1,   /* Invalid value for `ai_flags' field */
	DNS_EAI_NONAME      = -2,   /* NAME or SERVICE is unknown */
	DNS_EAI_AGAIN       = -3,   /* Temporary failure in name resolution */
	DNS_EAI_FAIL        = -4,   /* Non-recoverable failure in name res */
	DNS_EAI_NODATA      = -5,   /* No address associated with NAME */
	DNS_EAI_FAMILY      = -6,   /* `ai_family' not supported */
	DNS_EAI_SOCKTYPE    = -7,   /* `ai_socktype' not supported */
	DNS_EAI_SERVICE     = -8,   /* SRV not supported for `ai_socktype' */
	DNS_EAI_ADDRFAMILY  = -9,   /* Address family for NAME not supported */
	DNS_EAI_MEMORY      = -10,  /* Memory allocation failure */
	DNS_EAI_SYSTEM      = -11,  /* System error returned in `errno' */
	DNS_EAI_OVERFLOW    = -12,  /* Argument buffer overflow */
	DNS_EAI_INPROGRESS  = -100, /* Processing request in progress */
	DNS_EAI_CANCELED    = -101, /* Request canceled */
	DNS_EAI_NOTCANCELED = -102, /* Request not canceled */
	DNS_EAI_ALLDONE     = -103, /* All requests done */
	DNS_EAI_IDN_ENCODE  = -105, /* IDN encoding failed */
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

/**
 * DNS resolve context structure.
 */
struct dns_resolve_context {
	struct {
		/** DNS server information */
		struct sockaddr dns_server;

		/** Connection to the DNS server */
		struct net_context *net_ctx;

		/** Is this server mDNS one */
		bool is_mdns;
	} servers[CONFIG_DNS_RESOLVER_MAX_SERVERS + MDNS_SERVER_COUNT];

	/** This timeout is also used when a buffer is required from the
	 * buffer pools.
	 */
	s32_t buf_timeout;

	/** Result callbacks. We have multiple callbacks here so that it is
	 * possible to do multiple queries at the same time.
	 */
	struct dns_pending_query {
		/** Timeout timer */
		struct k_delayed_work timer;

		/** Back pointer to ctx, needed in timeout handler */
		struct dns_resolve_context *ctx;

		/** Result callback */
		dns_resolve_cb_t cb;

		/** User data */
		void *user_data;

		/** TX timeout */
		s32_t timeout;

		/** String containing the thing to resolve like www.example.com
		 */
		const char *query;

		/** Query type */
		enum dns_query_type query_type;

		/** DNS id of this query */
		u16_t id;
	} queries[CONFIG_DNS_NUM_CONCUR_QUERIES];

	/** Is this context in use */
	bool is_used;
};

/**
 * @brief Init DNS resolving context.
 *
 * @details This function sets the DNS server address and initializes the
 * DNS context that is used by the actual resolver.
 * Note that the recommended way to resolve DNS names is to use
 * the dns_get_addr_info() API. In that case user does not need to
 * call dns_resolve_init() as the DNS servers are already setup by the system.
 *
 * @param ctx DNS context. If the context variable is allocated from
 * the stack, then the variable needs to be valid for the whole duration of
 * the resolving. Caller does not need to fill the variable beforehand or
 * edit the context afterwards.
 * @param dns_servers DNS server addresses. The array is null terminated.
 * The port number can be given in the string.
 * Syntax for the server addresses with or without port numbers:
 *    IPv4        : 10.0.9.1
 *    IPv4 + port : 10.0.9.1:5353
 *    IPv6        : 2001:db8::22:42
 *    IPv6 + port : [2001:db8::22:42]:5353
 *
 * @return 0 if ok, <0 if error.
 */
int dns_resolve_init(struct dns_resolve_context *ctx,
		     const char *dns_servers[]);

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
		       u16_t dns_id);

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
 * K_FOREVER: the query is tried forever, user needs to cancel it manually
 *            if it takes too long time to finish
 * >0: start the query and let the system timeout it after specified ms
 *
 * @return 0 if resolving was started ok, < 0 otherwise
 */
int dns_resolve_name(struct dns_resolve_context *ctx,
		     const char *query,
		     enum dns_query_type type,
		     u16_t *dns_id,
		     dns_resolve_cb_t cb,
		     void *user_data,
		     s32_t timeout);

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
 * K_FOREVER: the query is tried forever, user needs to cancel it manually
 *            if it takes too long time to finish
 * >0: start the query and let the system timeout it after specified ms
 *
 * @return 0 if resolving was started ok, < 0 otherwise
 */
static inline int dns_get_addr_info(const char *query,
				    enum dns_query_type type,
				    u16_t *dns_id,
				    dns_resolve_cb_t cb,
				    void *user_data,
				    s32_t timeout)
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
static inline int dns_cancel_addr_info(u16_t dns_id)
{
	return dns_resolve_cancel(dns_resolve_get_default(), dns_id);
}

/**
 * @}
 */

#if defined(CONFIG_DNS_RESOLVER)
/**
 * @brief Initialize DNS subsystem.
 */
void dns_init_resolver(void);

#else
#define dns_init_resolver(...)
#endif /* CONFIG_DNS_RESOLVER */

#ifdef __cplusplus
}
#endif

#endif /* _DNS_RESOLVE_H */
