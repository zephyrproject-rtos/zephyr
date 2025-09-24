/** @file
 * @brief DNS resolve API
 *
 * An API for applications to do DNS query.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dns_resolve, CONFIG_DNS_RESOLVER_LOG_LEVEL);

#include <zephyr/types.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include <zephyr/sys/crc.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_service.h>
#include "../../ip/net_private.h"
#include "dns_pack.h"
#include "dns_internal.h"
#include "dns_cache.h"
#include "../../ip/net_stats.h"

#define DNS_SERVER_COUNT CONFIG_DNS_RESOLVER_MAX_SERVERS
#define SERVER_COUNT     (DNS_SERVER_COUNT + DNS_MAX_MCAST_SERVERS)

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(resolve_svc, dns_dispatcher_svc_handler,
				      DNS_RESOLVER_MAX_POLL);

#define MDNS_IPV4_ADDR "224.0.0.251:5353"
#define MDNS_IPV6_ADDR "[ff02::fb]:5353"

#define LLMNR_IPV4_ADDR "224.0.0.252:5355"
#define LLMNR_IPV6_ADDR "[ff02::1:3]:5355"

#define DNS_QUERY_MAX_SIZE	(DNS_MSG_HEADER_SIZE + CONFIG_DNS_RESOLVER_MAX_QUERY_LEN + \
				 DNS_QTYPE_LEN + DNS_QCLASS_LEN)

/* Compressed RR uses a pointer to another RR. So, min size is 12 bytes without
 * considering RR payload.
 * See https://tools.ietf.org/html/rfc1035#section-4.1.4
 */
#define DNS_ANSWER_PTR_LEN	12

/* See dns_unpack_answer, and also see:
 * https://tools.ietf.org/html/rfc1035#section-4.1.2
 */
#define DNS_QUERY_POS		0x0c

#define DNS_IPV4_LEN		sizeof(struct in_addr)
#define DNS_IPV6_LEN		sizeof(struct in6_addr)

#define DNS_RESOLVER_MIN_BUF	1
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_DNS_RESOLVER_ADDITIONAL_BUF_CTR)

NET_BUF_POOL_DEFINE(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

NET_BUF_POOL_DEFINE(dns_qname_pool, DNS_RESOLVER_BUF_CTR,
		    CONFIG_DNS_RESOLVER_MAX_QUERY_LEN,
		    0, NULL);

#ifdef CONFIG_DNS_RESOLVER_CACHE
DNS_CACHE_DEFINE(dns_cache, CONFIG_DNS_RESOLVER_CACHE_MAX_ENTRIES);
#endif /* CONFIG_DNS_RESOLVER_CACHE */

static K_MUTEX_DEFINE(lock);
static int init_called;
static struct dns_resolve_context dns_default_ctx;

/* Must be invoked with context lock held */
static int dns_write(struct dns_resolve_context *ctx,
		     int server_idx,
		     int query_idx,
		     uint8_t *buf, size_t buf_len, size_t max_len,
		     struct net_buf *dns_qname,
		     int hop_limit);
static int dns_read(struct dns_resolve_context *ctx,
		    struct net_buf *dns_data, size_t buf_len,
		    uint16_t *dns_id,
		    struct net_buf *dns_cname,
		    uint16_t *query_hash);
static inline int get_slot_by_id(struct dns_resolve_context *ctx,
				 uint16_t dns_id,
				 uint16_t query_hash);
static inline void invoke_query_callback(int status,
					 struct dns_addrinfo *info,
					 struct dns_pending_query *pending_query);
static void release_query(struct dns_pending_query *pending_query);

static bool server_is_mdns(sa_family_t family, struct sockaddr *addr)
{
	if (family == AF_INET) {
		if (net_ipv4_is_addr_mcast(&net_sin(addr)->sin_addr) &&
		    net_sin(addr)->sin_addr.s4_addr[3] == 251U) {
			return true;
		}

		return false;
	}

	if (family == AF_INET6) {
		if (net_ipv6_is_addr_mcast(&net_sin6(addr)->sin6_addr) &&
		    net_sin6(addr)->sin6_addr.s6_addr[15] == 0xfb) {
			return true;
		}

		return false;
	}

	return false;
}

static bool server_is_llmnr(sa_family_t family, struct sockaddr *addr)
{
	if (family == AF_INET) {
		if (net_ipv4_is_addr_mcast(&net_sin(addr)->sin_addr) &&
		    net_sin(addr)->sin_addr.s4_addr[3] == 252U) {
			return true;
		}

		return false;
	}

	if (family == AF_INET6) {
		if (net_ipv6_is_addr_mcast(&net_sin6(addr)->sin6_addr) &&
		    net_sin6(addr)->sin6_addr.s6_addr[15] == 0x03) {
			return true;
		}

		return false;
	}

	return false;
}

static void join_ipv4_mcast_group(struct net_if *iface, void *user_data)
{
	struct sockaddr *addr = user_data;
	int ret;

	ret = net_ipv4_igmp_join(iface, &net_sin(addr)->sin_addr, NULL);
	if (ret < 0 && ret != -EALREADY) {
		NET_DBG("Cannot join %s mDNS group (%d)", "IPv4", ret);
	} else {
		NET_DBG("Joined %s mDNS group %s", "IPv4",
			net_sprint_ipv4_addr(&net_sin(addr)->sin_addr));
	}
}

static void join_ipv6_mcast_group(struct net_if *iface, void *user_data)
{
	struct sockaddr *addr = user_data;
	int ret;

	ret = net_ipv6_mld_join(iface, &net_sin6(addr)->sin6_addr);
	if (ret < 0 && ret != -EALREADY) {
		NET_DBG("Cannot join %s mDNS group (%d)", "IPv6", ret);
	} else {
		NET_DBG("Joined %s mDNS group %s", "IPv6",
			net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr));
	}
}

static void dns_postprocess_server(struct dns_resolve_context *ctx, int idx)
{
	struct sockaddr *addr = &ctx->servers[idx].dns_server;

	if (addr->sa_family == AF_INET) {
		ctx->servers[idx].is_mdns = server_is_mdns(AF_INET, addr);
		if (!ctx->servers[idx].is_mdns) {
			ctx->servers[idx].is_llmnr =
				server_is_llmnr(AF_INET, addr);
		}

		if (net_sin(addr)->sin_port == 0U) {
			if (IS_ENABLED(CONFIG_MDNS_RESOLVER) &&
			    ctx->servers[idx].is_mdns) {
				/* We only use 5353 as a default port
				 * if mDNS support is enabled. User can
				 * override this by defining the port
				 * in config file.
				 */
				net_sin(addr)->sin_port = htons(5353);
			} else if (IS_ENABLED(CONFIG_LLMNR_RESOLVER) &&
				   ctx->servers[idx].is_llmnr) {
				/* We only use 5355 as a default port
				 * if LLMNR support is enabled. User can
				 * override this by defining the port
				 * in config file.
				 */
				net_sin(addr)->sin_port = htons(5355);
			} else {
				net_sin(addr)->sin_port = htons(53);
			}
		}

		/* Join the mDNS multicast group if responder is not enabled,
		 * because it will join the group itself.
		 */
		if (!IS_ENABLED(CONFIG_MDNS_RESPONDER) && ctx->servers[idx].is_mdns) {
			struct in_addr mcast_addr = { { { 224, 0, 0, 251 } } };

			if (net_sin(addr)->sin_addr.s_addr == mcast_addr.s_addr) {
				struct net_if *iface;

				iface = net_if_get_by_index(ctx->servers[idx].if_index);
				if (iface == NULL) {
					/* Join all interfaces */
					net_if_foreach(join_ipv4_mcast_group, addr);
				} else {
					/* Join specific interface */
					join_ipv4_mcast_group(iface, addr);
				}
			}
		}
	} else {
		ctx->servers[idx].is_mdns = server_is_mdns(AF_INET6, addr);
		if (!ctx->servers[idx].is_mdns) {
			ctx->servers[idx].is_llmnr =
				server_is_llmnr(AF_INET6, addr);
		}

		if (net_sin6(addr)->sin6_port == 0U) {
			if (IS_ENABLED(CONFIG_MDNS_RESOLVER) &&
			    ctx->servers[idx].is_mdns) {
				net_sin6(addr)->sin6_port = htons(5353);
			} else if (IS_ENABLED(CONFIG_LLMNR_RESOLVER) &&
				   ctx->servers[idx].is_llmnr) {
				net_sin6(addr)->sin6_port = htons(5355);
			} else {
				net_sin6(addr)->sin6_port = htons(53);
			}
		}

		if (!IS_ENABLED(CONFIG_MDNS_RESPONDER) && ctx->servers[idx].is_mdns) {
			struct in6_addr mcast_addr = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0xfb } } };

			if (memcmp(&net_sin6(addr)->sin6_addr, &mcast_addr,
				   sizeof(struct in6_addr)) == 0) {
				struct net_if *iface;

				iface = net_if_get_by_index(ctx->servers[idx].if_index);
				if (iface == NULL) {
					/* Join all interfaces */
					net_if_foreach(join_ipv6_mcast_group, addr);
				} else {
					/* Join specific interface */
					join_ipv6_mcast_group(iface, addr);
				}
			}
		}
	}
}

static int dispatcher_cb(struct dns_socket_dispatcher *my_ctx, int sock,
			 struct sockaddr *addr, size_t addrlen,
			 struct net_buf *dns_data, size_t len)
{
	struct dns_resolve_context *ctx = my_ctx->resolve_ctx;
	struct net_buf *dns_cname = NULL;
	uint16_t query_hash = 0U;
	uint16_t dns_id = 0U;
	int ret = 0, i;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
		goto unlock;
	}

	dns_cname = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
	if (!dns_cname) {
		ret = DNS_EAI_MEMORY;
		goto free_buf;
	}

	ret = dns_read(ctx, dns_data, len, &dns_id, dns_cname, &query_hash);
	if (!ret) {
		/* We called the callback already in dns_read() if there
		 * were no errors.
		 */
		goto free_buf;
	}

	/* Query again if we got CNAME */
	if (ret == DNS_EAI_AGAIN) {
		int ntry = 0, nfail = 0;
		int j;

		i = get_slot_by_id(ctx, dns_id, query_hash);
		if (i < 0) {
			goto free_buf;
		}

		for (j = 0; j < SERVER_COUNT; j++) {
			if (ctx->servers[j].sock < 0) {
				continue;
			}

			ntry++;
			ret = dns_write(ctx, j, i, dns_data->data, len,
					net_buf_max_len(dns_data),
					dns_cname, 0);
			if (ret < 0) {
				nfail++;
			}
		}

		if (nfail > 0) {
			NET_DBG("DNS cname query %d fails on %d attempts",
				nfail, ntry);
		}
		if ((ntry == 0) || (ntry == nfail)) {
			ret = DNS_EAI_SYSTEM;
			goto quit;
		}

		goto free_buf;
	}

quit:
	i = get_slot_by_id(ctx, dns_id, query_hash);
	if (i < 0) {
		goto free_buf;
	}

	invoke_query_callback(ret, NULL, &ctx->queries[i]);

	/* Marks the end of the results */
	release_query(&ctx->queries[i]);

free_buf:
	if (dns_cname) {
		net_buf_unref(dns_cname);
	}

unlock:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

static int register_dispatcher(struct dns_resolve_context *ctx,
			       const struct net_socket_service_desc *svc,
			       struct dns_server *server,
			       struct sockaddr *local,
			       const struct in6_addr *addr6,
			       const struct in_addr *addr4)
{
	server->dispatcher.type = DNS_SOCKET_RESOLVER;
	server->dispatcher.cb = dispatcher_cb;
	server->dispatcher.fds = ctx->fds;
	server->dispatcher.fds_len = ARRAY_SIZE(ctx->fds);
	server->dispatcher.sock = server->sock;
	server->dispatcher.svc = svc;
	server->dispatcher.resolve_ctx = ctx;

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    server->dns_server.sa_family == AF_INET6) {
		memcpy(&server->dispatcher.local_addr,
		       local,
		       sizeof(struct sockaddr_in6));
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   server->dns_server.sa_family == AF_INET) {
		memcpy(&server->dispatcher.local_addr,
		       local,
		       sizeof(struct sockaddr_in));
	} else {
		return -ENOTSUP;
	}

	return dns_dispatcher_register(&server->dispatcher);
}

static int bind_to_iface(int sock, const struct sockaddr *addr, int if_index)
{
	struct ifreq ifreq = { 0 };
	int ret;

	ret = net_if_get_name(net_if_get_by_index(if_index), ifreq.ifr_name,
			      sizeof(ifreq.ifr_name));
	if (ret < 0) {
		LOG_DBG("Cannot get interface name for %d (%d)", if_index, ret);
		return ret;
	}

	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	if (ret < 0) {
		ret = -errno;

		NET_DBG("Cannot bind %s to %d (%d)",
			net_sprint_addr(addr->sa_family, &net_sin(addr)->sin_addr),
			if_index, ret);
	}

	return ret;
}

static bool is_server_name_found(struct dns_resolve_context *ctx,
				 const char *server, size_t server_len,
				 const char *iface_str)
{
	ARRAY_FOR_EACH(ctx->servers, i) {
		if (ctx->servers[i].dns_server.sa_family == AF_INET ||
		    ctx->servers[i].dns_server.sa_family == AF_INET6) {
			char addr_str[INET6_ADDRSTRLEN];
			size_t addr_len;

			if (net_addr_ntop(ctx->servers[i].dns_server.sa_family,
					  &net_sin(&ctx->servers[i].dns_server)->sin_addr,
					  addr_str, sizeof(addr_str)) == NULL) {
				continue;
			}

			addr_len = strlen(addr_str);
			if (addr_len != server_len ||
			    strncmp(addr_str, server, server_len) != 0) {
				continue;
			}

			if (iface_str != NULL && ctx->servers[i].if_index > 0) {
				char iface_name[IFNAMSIZ];

				net_if_get_name(net_if_get_by_index(
						ctx->servers[i].if_index),
						iface_name, sizeof(iface_name));

				if (strcmp(iface_name, iface_str) != 0) {
					continue;
				}
			}

			return true;
		}
	}

	return false;
}

static bool is_server_addr_found(struct dns_resolve_context *ctx,
				 const struct sockaddr *addr,
				 int if_index)
{
	ARRAY_FOR_EACH(ctx->servers, i) {
		if (ctx->servers[i].dns_server.sa_family == addr->sa_family &&
		    memcmp(&ctx->servers[i].dns_server, addr,
			   sizeof(ctx->servers[i].dns_server)) == 0) {
			if (if_index == 0 ||
			    (if_index > 0 &&
			     ctx->servers[i].if_index != if_index)) {
				continue;
			}

			return true;
		}
	}

	return false;
}

static int get_free_slot(struct dns_resolve_context *ctx)
{
	ARRAY_FOR_EACH(ctx->servers, i) {
		if (ctx->servers[i].dns_server.sa_family == 0) {
			return i;
		}
	}

	return -ENOENT;
}

const char *dns_get_source_str(enum dns_server_source source)
{
	switch (source) {
	case DNS_SOURCE_UNKNOWN:
		return "unknown";
	case DNS_SOURCE_MANUAL:
		return "manual";
	case DNS_SOURCE_DHCPV4:
		__fallthrough;
	case DNS_SOURCE_DHCPV6:
		return "DHCP";
	case DNS_SOURCE_IPV6_RA:
		return "IPv6 RA";
	case DNS_SOURCE_PPP:
		return "PPP";
	}

	return "";
}

/* Must be invoked with context lock held */
static int dns_resolve_init_locked(struct dns_resolve_context *ctx,
				   const char *servers[],
				   const struct sockaddr *servers_sa[],
				   const struct net_socket_service_desc *svc,
				   uint16_t port, int interfaces[],
				   bool do_cleanup,
				   enum dns_server_source source)
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 local_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(port),
	};
#endif
#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in local_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
	};
#endif
	struct sockaddr *local_addr = NULL;
	socklen_t addr_len = 0;
	int i = 0, idx = 0;
	const struct in6_addr *addr6 = NULL;
	const struct in_addr *addr4 = NULL;
	struct net_if *iface;
	int ret, count;

	if (!ctx) {
		return -ENOENT;
	}

	if (do_cleanup) {
		if (ctx->state != DNS_RESOLVE_CONTEXT_INACTIVE) {
			ret = -ENOTEMPTY;
			NET_DBG("DNS resolver context is not inactive (%d)", ctx->state);
			goto fail;
		}

		ARRAY_FOR_EACH(ctx->servers, j) {
			ctx->servers[j].sock = -1;
		}

		ARRAY_FOR_EACH(ctx->fds, j) {
			ctx->fds[j].fd = -1;
		}
	}

	/* If user has provided a list of servers in string format, then
	 * figure out the network interface from that list. If user used
	 * list of sockaddr servers, then use the interfaces parameter.
	 * The interfaces parameter should point to an array that is the
	 * the same length as the servers_sa parameter array.
	 */
	for (i = 0; servers != NULL && idx < SERVER_COUNT && servers[i] != NULL; i++) {
		const char *iface_str;
		size_t server_len;
		struct sockaddr *addr;
		bool found;

		iface_str = strstr(servers[i], "%");
		if (iface_str != NULL) {
			server_len = iface_str - servers[i];
			iface_str++;

			if (server_len == 0) {
				NET_DBG("Empty server name");
				continue;
			}

			found = is_server_name_found(ctx, servers[i],
						     server_len, iface_str);
			if (found) {
				NET_DBG("Server %.*s already exists",
					(int)server_len, servers[i]);
				continue;
			}

			/* Figure out if there are free slots where to add
			 * the server.
			 */
			idx = get_free_slot(ctx);
			if (idx < 0) {
				NET_DBG("No free slots for server %.*s",
					(int)server_len, servers[i]);
				break;
			}

			/* Skip empty interface name */
			if (iface_str[0] == '\0') {
				ctx->servers[idx].if_index = 0;
				iface_str = NULL;
			} else {
				ctx->servers[idx].if_index =
					net_if_get_by_name(iface_str);
			}

		} else {
			server_len = strlen(servers[i]);
			if (server_len == 0) {
				NET_DBG("Empty server name");
				continue;
			}

			found = is_server_name_found(ctx, servers[i],
						     server_len, NULL);
			if (found) {
				NET_DBG("Server %.*s already exists",
					(int)server_len, servers[i]);
				continue;
			}

			idx = get_free_slot(ctx);
			if (idx < 0) {
				NET_DBG("No free slots for server %.*s",
					(int)server_len, servers[i]);
				break;
			}
		}

		ctx->servers[idx].source = source;

		addr = &ctx->servers[idx].dns_server;

		(void)memset(addr, 0, sizeof(*addr));

		ret = net_ipaddr_parse(servers[i], server_len, addr);
		if (!ret) {
			if (servers[i][0] != '\0') {
				NET_DBG("Invalid server address %.*s",
					(int)server_len, servers[i]);
			}

			continue;
		}

		dns_postprocess_server(ctx, idx);

		NET_DBG("[%d] %.*s%s%s%s%s%s%s%s", i, (int)server_len, servers[i],
			IS_ENABLED(CONFIG_MDNS_RESOLVER) ?
			(ctx->servers[i].is_mdns ? " mDNS" : "") : "",
			IS_ENABLED(CONFIG_LLMNR_RESOLVER) ?
			(ctx->servers[i].is_llmnr ? " LLMNR" : "") : "",
			iface_str != NULL ? " via " : "",
			iface_str != NULL ? iface_str : "",
			source != DNS_SOURCE_UNKNOWN ? " (" : "",
			source != DNS_SOURCE_UNKNOWN ? dns_get_source_str(source) : "",
			source != DNS_SOURCE_UNKNOWN ? ")" : "");
		idx++;
	}

	for (i = 0; servers_sa != NULL && idx < SERVER_COUNT && servers_sa[i] != NULL; i++) {
		char iface_str[IFNAMSIZ] = { 0 };
		bool found;

		found = is_server_addr_found(ctx, servers_sa[i],
					     interfaces == NULL ? 0 : interfaces[i]);
		if (found) {
			NET_DBG("Server %s already exists",
				net_sprint_addr(ctx->servers[i].dns_server.sa_family,
						&net_sin(&ctx->servers[i].dns_server)->sin_addr));
			continue;
		}

		/* Figure out if there are free slots where to add the server.
		 */
		idx = get_free_slot(ctx);
		if (idx < 0) {
			NET_DBG("No free slots for server %s",
				net_sprint_addr(ctx->servers[i].dns_server.sa_family,
						&net_sin(&ctx->servers[i].dns_server)->sin_addr));
			break;
		}

		ctx->servers[idx].source = source;

		memcpy(&ctx->servers[idx].dns_server, servers_sa[i],
		       sizeof(ctx->servers[idx].dns_server));

		if (interfaces != NULL) {
			ctx->servers[idx].if_index = interfaces[i];

			net_if_get_name(net_if_get_by_index(ctx->servers[idx].if_index),
					iface_str, sizeof(iface_str));
		}

		dns_postprocess_server(ctx, idx);

		NET_DBG("[%d] %s%s%s%s%s%s%s%s", i,
			net_sprint_addr(servers_sa[i]->sa_family,
					&net_sin(servers_sa[i])->sin_addr),
			IS_ENABLED(CONFIG_MDNS_RESOLVER) ?
			(ctx->servers[i].is_mdns ? " mDNS" : "") : "",
			IS_ENABLED(CONFIG_LLMNR_RESOLVER) ?
			(ctx->servers[i].is_llmnr ? " LLMNR" : "") : "",
			interfaces != NULL ? " via " : "",
			interfaces != NULL ? iface_str : "",
			source != DNS_SOURCE_UNKNOWN ? " (" : "",
			source != DNS_SOURCE_UNKNOWN ? dns_get_source_str(source) : "",
			source != DNS_SOURCE_UNKNOWN ? ")" : "");
		idx++;
	}

	for (i = 0, count = 0;
	     i < SERVER_COUNT && ctx->servers[i].dns_server.sa_family != 0; i++) {

		if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			local_addr = (struct sockaddr *)&local_addr6;
			addr_len = sizeof(struct sockaddr_in6);

			if (IS_ENABLED(CONFIG_MDNS_RESOLVER) &&
			    ctx->servers[i].is_mdns && port == 0) {
				local_addr6.sin6_port = htons(5353);
			}
#else
			continue;
#endif
		}

		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			local_addr = (struct sockaddr *)&local_addr4;
			addr_len = sizeof(struct sockaddr_in);

			if (IS_ENABLED(CONFIG_MDNS_RESOLVER) &&
			    ctx->servers[i].is_mdns && port == 0) {
				local_addr4.sin_port = htons(5353);
			}
#else
			continue;
#endif
		}

		if (!local_addr) {
			NET_DBG("Local address not set");
			ret = -EAFNOSUPPORT;
			goto fail;
		}

		if (ctx->servers[i].sock >= 0) {
			/* Socket already exists, so skip it */
			NET_DBG("Socket %d already exists for %s",
				ctx->servers[i].sock,
				net_sprint_addr(ctx->servers[i].dns_server.sa_family,
						&net_sin(&ctx->servers[i].dns_server)->sin_addr));
			count++;
			continue;
		}

		ret = zsock_socket(ctx->servers[i].dns_server.sa_family,
				   SOCK_DGRAM, IPPROTO_UDP);
		if (ret < 0) {
			ret = -errno;
			NET_ERR("Cannot get socket (%d)", ret);
			goto fail;
		}

		ctx->servers[i].sock = ret;

		/* Try to bind to the interface if it is set */
		if (ctx->servers[i].if_index > 0) {
			ret = bind_to_iface(ctx->servers[i].sock,
					    &ctx->servers[i].dns_server,
					    ctx->servers[i].if_index);
			if (ret < 0) {
				zsock_close(ctx->servers[i].sock);
				ctx->servers[i].sock = -1;
				ctx->servers[i].dns_server.sa_family = 0;
				continue;
			}

			iface = net_if_get_by_index(ctx->servers[i].if_index);
			NET_DBG("Binding %s to %d",
				net_sprint_addr(ctx->servers[i].dns_server.sa_family,
						&net_sin(&ctx->servers[i].dns_server)->sin_addr),
				ctx->servers[i].if_index);
		} else {
			iface = NULL;
		}

		if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			if (iface == NULL) {
				iface = net_if_ipv6_select_src_iface(
					&net_sin6(&ctx->servers[i].dns_server)->sin6_addr);
			}

			addr6 = net_if_ipv6_select_src_addr(iface,
					&net_sin6(&ctx->servers[i].dns_server)->sin6_addr);
		} else {
			if (iface == NULL) {
				iface = net_if_ipv4_select_src_iface(
					&net_sin(&ctx->servers[i].dns_server)->sin_addr);
			}

			addr4 = net_if_ipv4_select_src_addr(iface,
					&net_sin(&ctx->servers[i].dns_server)->sin_addr);
		}

		ARRAY_FOR_EACH(ctx->fds, j) {
			if (ctx->fds[j].fd == ctx->servers[i].sock) {
				/* There was query to this server already */
				ret = 0;
				break;
			}

			if (ctx->fds[j].fd < 0) {
				ctx->fds[j].fd = ctx->servers[i].sock;
				ctx->fds[j].events = ZSOCK_POLLIN;
				ret = 0;
				break;
			}
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
			zsock_close(ctx->servers[i].sock);
			ctx->servers[i].sock = -1;
			ctx->servers[i].dns_server.sa_family = 0;
			continue;
		}

		ret = register_dispatcher(ctx, svc, &ctx->servers[i], local_addr,
					  addr6, addr4);
		if (ret < 0) {
			if (ret == -EALREADY) {
				goto skip_event;
			}

			NET_DBG("Cannot register dispatcher for %s (%d)",
				ctx->servers[i].is_mdns ? "mDNS" : "DNS", ret);
			goto fail;
		}

		if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
			net_mgmt_event_notify_with_info(
				NET_EVENT_DNS_SERVER_ADD,
				iface, (void *)&ctx->servers[i].dns_server,
				sizeof(struct sockaddr));
		} else {
			net_mgmt_event_notify(NET_EVENT_DNS_SERVER_ADD, iface);
		}

skip_event:

#if defined(CONFIG_NET_IPV6)
		local_addr6.sin6_port = htons(port);
#endif

#if defined(CONFIG_NET_IPV4)
		local_addr4.sin_port = htons(port);
#endif

		count++;
	}

	if (count == 0) {
		/* No servers defined */
		NET_DBG("No DNS servers defined.");
		ret = -EINVAL;
		goto fail;
	}

	init_called++;
	ctx->state = DNS_RESOLVE_CONTEXT_ACTIVE;
	ctx->buf_timeout = DNS_BUF_TIMEOUT;
	ret = 0;

fail:
	return ret;
}

int dns_resolve_init_with_svc(struct dns_resolve_context *ctx, const char *servers[],
			      const struct sockaddr *servers_sa[],
			      const struct net_socket_service_desc *svc,
			      uint16_t port, int interfaces[])
{
	int ret;

	if (!ctx) {
		return -ENOENT;
	}

	k_mutex_lock(&lock, K_FOREVER);

	/* Do cleanup only if we are starting the context for the first time */
	if (init_called == 0) {
		(void)memset(ctx, 0, sizeof(*ctx));

		(void)k_mutex_init(&ctx->lock);
		ctx->state = DNS_RESOLVE_CONTEXT_INACTIVE;
	}

	ret = dns_resolve_init_locked(ctx, servers, servers_sa, svc, port,
				      interfaces, true, DNS_SOURCE_UNKNOWN);

	k_mutex_unlock(&lock);

	return ret;
}

int dns_resolve_init(struct dns_resolve_context *ctx, const char *servers[],
		     const struct sockaddr *servers_sa[])
{
	return dns_resolve_init_with_svc(ctx, servers, servers_sa,
					 &resolve_svc, 0, NULL);
}

/* Check whether a slot is available for use, or optionally whether it can be
 * reclaimed.
 *
 * @param pending_query the query slot in question
 *
 * @param reclaim_if_available if the slot is marked in use, but the query has
 * been completed and the work item is no longer pending, complete the release
 * of the slot.
 *
 * @return true if and only if the slot can be used for a new query.
 */
static inline bool check_query_active(struct dns_pending_query *pending_query,
				      bool reclaim_if_available)
{
	int ret = false;

	if (pending_query->cb != NULL) {
		ret = true;
		if (reclaim_if_available
		    && pending_query->query == NULL
		    && k_work_delayable_busy_get(&pending_query->timer) == 0) {
			pending_query->cb = NULL;
			ret = false;
		}
	}

	return ret;
}

/* Must be invoked with context lock held */
static inline int get_cb_slot(struct dns_resolve_context *ctx)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (!check_query_active(&ctx->queries[i], true)) {
			return i;
		}
	}

	return -ENOENT;
}

/* Invoke the callback associated with a query slot, if still relevant.
 *
 * Must be invoked with context lock held.
 *
 * @param status the query status value
 * @param info the query result structure
 * @param pending_query the query slot that will provide the callback
 **/
static inline void invoke_query_callback(int status,
					 struct dns_addrinfo *info,
					 struct dns_pending_query *pending_query)
{
	/* Only notify if the slot is neither released nor in the process of
	 * being released.
	 */
	if (pending_query->query != NULL && pending_query->cb != NULL)  {
		pending_query->cb(status, info, pending_query->user_data);
	}
}

/* Release a query slot reserved by get_cb_slot().
 *
 * Must be invoked with context lock held.
 *
 * @param pending_query the query slot to be released
 */
static void release_query(struct dns_pending_query *pending_query)
{
	int busy = k_work_cancel_delayable(&pending_query->timer);

	/* If the work item is no longer pending we're done. */
	if (busy == 0) {
		/* All done. */
		pending_query->cb = NULL;
	} else {
		/* Work item is still pending.  Set a secondary condition that
		 * can be checked by get_cb_slot() to complete release of the
		 * slot once the work item has been confirmed to be completed.
		 */
		pending_query->query = NULL;
	}
}

/* Must be invoked with context lock held */
static inline int get_slot_by_id(struct dns_resolve_context *ctx,
				 uint16_t dns_id,
				 uint16_t query_hash)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (check_query_active(&ctx->queries[i], false) &&
		    ctx->queries[i].id == dns_id &&
		    (query_hash == 0 ||
		     ctx->queries[i].query_hash == query_hash)) {
			return i;
		}
	}

	return -ENOENT;
}

static int update_query_idx(struct dns_resolve_context *ctx,
			    struct dns_msg_t *dns_msg,
			    uint16_t *dns_id,
			    int *query_idx,
			    uint16_t *query_hash)
{
	int ret;
	char *query_name;
	int query_name_len;

	query_name = dns_msg->msg + dns_msg->query_offset;
	query_name_len = strlen(query_name);

	/* Convert the query name to small case so that our
	 * hash checker can find it.
	 */
	for (size_t i = 0, n = query_name_len; i < n; i++) {
		query_name[i] = tolower(query_name[i]);
	}

	/* Hash the name with \0 and query type */
	*query_hash = crc16_ansi(query_name,
				 query_name_len + 1 + 2);

	*query_idx = get_slot_by_id(ctx, *dns_id, *query_hash);
	if (*query_idx < 0) {
		/* Re-check if this was a mDNS probe query */
		if (IS_ENABLED(CONFIG_MDNS_RESPONDER_PROBE) && *dns_id == 0) {
			uint16_t orig_qtype;

			orig_qtype = sys_get_be16(&query_name[query_name_len + 1]);

			/* Replace the query type with ANY as that was used
			 * when creating the hash.
			 */
			sys_put_be16(DNS_RR_TYPE_ANY,
				     &query_name[query_name_len + 1]);

			*query_hash = crc16_ansi(query_name,
						 query_name_len + 1 + 2);

			sys_put_be16(orig_qtype, &query_name[query_name_len + 1]);

			*query_idx = get_slot_by_id(ctx, *dns_id, *query_hash);
			if (*query_idx < 0) {
				ret = -ENOENT;
				goto quit;
			}
		} else {
			ret = -ENOENT;
			goto quit;
		}
	}

	ret = 0;
quit:
	return ret;
}

/* Unit test needs to be able to call this function */
#if !defined(CONFIG_NET_TEST)
static
#endif
int dns_validate_msg(struct dns_resolve_context *ctx,
		     struct dns_msg_t *dns_msg,
		     uint16_t *dns_id,
		     int *query_idx,
		     struct net_buf *dns_cname,
		     uint16_t *query_hash)
{
	struct dns_addrinfo info = { 0 };
	uint32_t ttl; /* RR ttl, so far it is not passed to caller */
	uint8_t *src, *addr;
	int address_size;
	/* index that points to the current answer being analyzed */
	int answer_ptr;
	int items;
	int server_idx;
	int ret = 0;

	/* Make sure that we can read DNS id, flags and rcode */
	if (dns_msg->msg_size < (sizeof(*dns_id) + sizeof(uint16_t))) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	/* The dns_unpack_response_header() has design flaw as it expects
	 * dns id to be given instead of returning the id to the caller.
	 * In our case we would like to get it returned instead so that we
	 * can match the DNS query that we sent. When dns_read() is called,
	 * we do not know what the DNS id is yet.
	 */
	*dns_id = dns_unpack_header_id(dns_msg->msg);

	if (dns_header_rcode(dns_msg->msg) == DNS_HEADER_REFUSED) {
		ret = DNS_EAI_FAIL;
		goto quit;
	}

	/* We might receive a query while we are waiting for a response, in that
	 * case we just ignore the query instead of making the resolving fail.
	 */
	if (dns_header_qr(dns_msg->msg) == DNS_QUERY) {
		ret = 0;
		goto quit;
	}

	ret = dns_unpack_response_header(dns_msg, *dns_id);
	if (ret < 0) {
		errno = -ret;
		ret = DNS_EAI_SYSTEM;
		goto quit;
	}

	if (dns_header_qdcount(dns_msg->msg) != 1) {
		/* For mDNS (when dns_id == 0) the query count is 0 */
		if (*dns_id > 0) {
			ret = DNS_EAI_FAIL;
			goto quit;
		}
	}

	ret = dns_unpack_response_query(dns_msg);
	if (ret < 0) {
		if (ret == -ENOMEM) {
			errno = -ret;
			ret = DNS_EAI_SYSTEM;
			goto quit;
		}

		/* Check mDNS like above */
		if (*dns_id > 0) {
			ret = DNS_EAI_FAIL;
			goto quit;
		}
	}

	if (dns_header_qdcount(dns_msg->msg) < 1 && *dns_id == 0) {
		/* mDNS responses to do not have the query part so the
		 * answer starts immediately after the header.
		 */
		dns_msg->answer_offset = dns_msg->query_offset;
	}

	/* Because in mDNS the DNS id is set to 0 and must be ignored
	 * on reply, we need to figure out the answer in order to find
	 * the proper query. To simplify things, the normal DNS responses
	 * are handled the same way.
	 */

	answer_ptr = DNS_QUERY_POS;
	items = 0;
	server_idx = 0;
	enum dns_rr_type answer_type = DNS_RR_TYPE_INVALID;

	while (server_idx < dns_header_ancount(dns_msg->msg)) {
		ret = dns_unpack_answer(dns_msg, answer_ptr, &ttl,
					&answer_type);
		if (ret < 0) {
			errno = -ret;
			ret = DNS_EAI_SYSTEM;
			goto quit;
		}

		switch (dns_msg->response_type) {
		case DNS_RESPONSE_DATA:
		case DNS_RESPONSE_IP:
			if (*query_idx < 0) {
				ret = update_query_idx(ctx, dns_msg, dns_id,
							query_idx, query_hash);
				if (ret < 0) {
					errno = -ret;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}
			}

			if (ctx->queries[*query_idx].query_type ==
							DNS_QUERY_TYPE_A) {
				if (answer_type != DNS_RR_TYPE_A) {
					ret = DNS_EAI_ADDRFAMILY;
					goto quit;
				}

rr_qtype_a:
				address_size = DNS_IPV4_LEN;
				addr = (uint8_t *)&net_sin(&info.ai_addr)->
								sin_addr;
				info.ai_family = AF_INET;
				info.ai_addr.sa_family = AF_INET;
				info.ai_addrlen = sizeof(struct sockaddr_in);

			} else if (ctx->queries[*query_idx].query_type ==
							DNS_QUERY_TYPE_AAAA) {
				if (answer_type != DNS_RR_TYPE_AAAA) {
					ret = DNS_EAI_ADDRFAMILY;
					goto quit;
				}

rr_qtype_aaaa:
				/* We cannot resolve IPv6 address if IPv6 is
				 * disabled. The reason being that
				 * "struct sockaddr" does not have enough space
				 * for IPv6 address in that case.
				 */
#if defined(CONFIG_NET_IPV6)
				address_size = DNS_IPV6_LEN;
				addr = (uint8_t *)&net_sin6(&info.ai_addr)->
								sin6_addr;
				info.ai_family = AF_INET6;
				info.ai_addr.sa_family = AF_INET6;
				info.ai_addrlen = sizeof(struct sockaddr_in6);
#else
				ret = DNS_EAI_FAMILY;
				goto quit;
#endif
			} else if (ctx->queries[*query_idx].query_type ==
				   (enum dns_query_type)DNS_RR_TYPE_ANY) {
				/* If we did ANY query, we need to check what
				 * type of answer we got. Currently only A or AAAA
				 * are supported.
				 */
				if (answer_type == DNS_RR_TYPE_A) {
					goto rr_qtype_a;
				} else if (answer_type == DNS_RR_TYPE_AAAA) {
					goto rr_qtype_aaaa;
				} else {
					ret = DNS_EAI_ADDRFAMILY;
					goto quit;
				}

			} else if (ctx->queries[*query_idx].query_type ==
							DNS_QUERY_TYPE_PTR) {
				/* Synthesize a reply and place the returned info
				 * in to the ai_canonname field. Use AF_LOCAL address
				 * family as this is not a real address.
				 */
				struct net_buf *result;
				uint8_t *pos;

				address_size = MIN(dns_msg->response_length,
						   DNS_MAX_NAME_SIZE);

				/* Temporary buffer that is needed by dns_unpack_name()
				 * to unpack the resolved name.
				 */
				result = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
				if (result == NULL) {
					NET_DBG("Cannot allocate buffer for DNS query result");
					ret = DNS_EAI_MEMORY;
					goto quit;
				}

				pos = dns_msg->msg + dns_msg->response_position;
				ret = dns_unpack_name(dns_msg->msg, dns_msg->msg_size, pos,
						      result, NULL);
				if (ret < 0) {
					errno = -ret;
					ret = DNS_EAI_SYSTEM;
					net_buf_unref(result);
					goto quit;
				}

				info.ai_family = AF_LOCAL;
				info.ai_addrlen = MIN(result->len, DNS_MAX_NAME_SIZE);
				memcpy(info.ai_canonname, result->data, info.ai_addrlen);
				info.ai_canonname[info.ai_addrlen] = '\0';

				net_buf_unref(result);

			} else {
				ret = DNS_EAI_FAMILY;
				goto quit;
			}

			if (ctx->queries[*query_idx].query_type ==
							DNS_QUERY_TYPE_A ||
			    ctx->queries[*query_idx].query_type ==
							DNS_QUERY_TYPE_AAAA) {
				if (dns_msg->response_length < address_size) {
					/* it seems this is a malformed message */
					errno = EMSGSIZE;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}

				if ((dns_msg->response_position + address_size) >
				    dns_msg->msg_size) {
					/* Too short message */
					errno = EMSGSIZE;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}

				src = dns_msg->msg + dns_msg->response_position;
				memcpy(addr, src, address_size);
			}

			invoke_query_callback(DNS_EAI_INPROGRESS, &info,
					      &ctx->queries[*query_idx]);
#ifdef CONFIG_DNS_RESOLVER_CACHE
			dns_cache_add(&dns_cache,
				ctx->queries[*query_idx].query, &info, ttl);
#endif /* CONFIG_DNS_RESOLVER_CACHE */
			items++;
			break;

		case DNS_RESPONSE_TXT: {
			uint8_t *pos;

			if (*query_idx < 0) {
				ret = update_query_idx(ctx, dns_msg, dns_id,
							query_idx, query_hash);
				if (ret < 0) {
					errno = -ret;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}
			}

			pos = dns_msg->msg + dns_msg->response_position;

			info.ai_family = AF_UNSPEC;
			info.ai_extension = DNS_RESOLVE_TXT;
			info.ai_txt.textlen = MIN(dns_msg->response_length,
						  DNS_MAX_TEXT_SIZE);
			memcpy(info.ai_txt.text, pos, info.ai_txt.textlen);
			info.ai_txt.text[info.ai_txt.textlen] = '\0';

			invoke_query_callback(DNS_EAI_INPROGRESS, &info,
					      &ctx->queries[*query_idx]);
			break;
		}
		case DNS_RESPONSE_SRV: {
			int priority;
			int weight;
			int port;
			struct net_buf *target;
			uint8_t *pos;

			if (*query_idx < 0) {
				ret = update_query_idx(ctx, dns_msg, dns_id,
							query_idx, query_hash);
				if (ret < 0) {
					errno = -ret;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}
			}

			address_size = MIN(dns_msg->response_length,
					   6 + DNS_MAX_NAME_SIZE);
			if (address_size < 6) {
				/* 3 tuples of be16 - priority, weight, port */
				errno = EMSGSIZE;
				ret = DNS_EAI_SYSTEM;
				goto quit;
			}

			/* Temporary buffer that is needed by dns_unpack_name()
			 * to unpack the target.
			 */
			target = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
			if (target == NULL) {
				NET_DBG("Cannot allocate buffer for DNS query target");
				ret = DNS_EAI_MEMORY;
				goto quit;
			}

			pos = dns_msg->msg + dns_msg->response_position;

			priority = dns_unpack_srv_priority(pos);
			weight = dns_unpack_srv_weight(pos);
			port = dns_unpack_srv_port(pos);

			ret = dns_unpack_name(dns_msg->msg, dns_msg->msg_size, pos + 6,
					      target, NULL);
			if (ret < 0) {
				errno = -ret;
				ret = DNS_EAI_SYSTEM;
				net_buf_unref(target);
				goto quit;
			}

			info.ai_family = AF_UNSPEC;
			info.ai_extension = DNS_RESOLVE_SRV;
			info.ai_srv.priority = priority;
			info.ai_srv.weight = weight;
			info.ai_srv.port = port;
			info.ai_srv.targetlen = MIN(target->len, DNS_MAX_NAME_SIZE);
			memcpy(info.ai_srv.target, target->data, info.ai_srv.targetlen);
			info.ai_srv.target[info.ai_srv.targetlen] = '\0';

			net_buf_unref(target);

			invoke_query_callback(DNS_EAI_INPROGRESS, &info,
					      &ctx->queries[*query_idx]);
#ifdef CONFIG_DNS_RESOLVER_CACHE
			dns_cache_add(&dns_cache,
				ctx->queries[*query_idx].query, &info, ttl);
#endif /* CONFIG_DNS_RESOLVER_CACHE */
			items++;
			break;
		}
		case DNS_RESPONSE_CNAME_NO_IP:
			/* Instead of using the QNAME at DNS_QUERY_POS,
			 * we will use this CNAME
			 */
			answer_ptr = dns_msg->response_position;
			break;

		default:
			ret = DNS_EAI_FAIL;
			goto quit;
		}

		/* Update the answer offset to point to the next RR (answer) */
		dns_msg->answer_offset += dns_msg->response_position -
							dns_msg->answer_offset;
		dns_msg->answer_offset += dns_msg->response_length;

		server_idx++;
	}

	if (*query_idx < 0) {
		/* If the query_idx is still unknown, try to get it here
		 * and hope it is found.
		 */
		ret = update_query_idx(ctx, dns_msg, dns_id,
					query_idx, query_hash);
		if (ret < 0) {
			errno = -ret;
			ret = DNS_EAI_SYSTEM;
			goto quit;
		}
	}

	/* No IP addresses were found, so we take the last CNAME to generate
	 * another query. Number of additional queries is controlled via Kconfig
	 */
	if (items == 0) {
		if (dns_msg->response_type == DNS_RESPONSE_CNAME_NO_IP) {
			uint16_t pos = dns_msg->response_position;

			/* The dns_cname should always be set. As a special
			 * case, it might not be set for unit tests that call
			 * this function directly.
			 */
			if (dns_cname) {
				ret = dns_copy_qname(dns_cname->data,
						     &dns_cname->len,
						     net_buf_max_len(dns_cname),
						     dns_msg, pos);
				if (ret < 0) {
					errno = -ret;
					ret = DNS_EAI_SYSTEM;
					goto quit;
				}
			}

			ret = DNS_EAI_AGAIN;
			goto quit;
		}
	}

	if (items == 0) {
		ret = DNS_EAI_NODATA;
	} else {
		ret = DNS_EAI_ALLDONE;
	}

quit:
	return ret;
}

/* Must be invoked with context lock held */
static int dns_read(struct dns_resolve_context *ctx,
		    struct net_buf *dns_data, size_t buf_len,
		    uint16_t *dns_id,
		    struct net_buf *dns_cname,
		    uint16_t *query_hash)
{
	/* Helper struct to track the dns msg received from the server */
	struct dns_msg_t dns_msg;
	int data_len;
	int ret;
	int query_idx = -1;

	data_len = MIN(buf_len, DNS_RESOLVER_MAX_BUF_SIZE);

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	ret = dns_validate_msg(ctx, &dns_msg, dns_id, &query_idx,
			       dns_cname, query_hash);
	if (ret == DNS_EAI_AGAIN) {
		goto finished;
	}

	if ((ret < 0 && ret != DNS_EAI_ALLDONE) || query_idx < 0 ||
	    query_idx > CONFIG_DNS_NUM_CONCUR_QUERIES) {
		goto quit;
	}

#if defined(CONFIG_DNS_RESOLVER_PACKET_FORWARDING)
	if (ctx->pkt_fw_cb != NULL) {
		ctx->pkt_fw_cb(dns_data, data_len, ctx->queries[query_idx].user_data);
	}
#endif /* CONFIG_DNS_RESOLVER_PACKET_FORWARDING */

	invoke_query_callback(ret, NULL, &ctx->queries[query_idx]);

	/* Marks the end of the results */
	release_query(&ctx->queries[query_idx]);

	return 0;

finished:
	dns_resolve_cancel_with_name(ctx, *dns_id,
				     ctx->queries[query_idx].query,
				     ctx->queries[query_idx].query_type);
quit:
	return ret;
}

static int set_ttl_hop_limit(int sock, int level, int option, int new_limit)
{
	return zsock_setsockopt(sock, level, option, &new_limit, sizeof(new_limit));
}

/* Must be invoked with context lock held */
static int dns_write(struct dns_resolve_context *ctx,
		     int server_idx,
		     int query_idx,
		     uint8_t *buf, size_t buf_len, size_t max_len,
		     struct net_buf *dns_qname,
		     int hop_limit)
{
	enum dns_query_type query_type;
	struct sockaddr *server;
	int server_addr_len;
	uint16_t dns_id, len;
	int ret, sock, family;
	char *query_name;

	sock = ctx->servers[server_idx].sock;
	family = ctx->servers[server_idx].dns_server.sa_family;
	server = &ctx->servers[server_idx].dns_server;
	dns_id = ctx->queries[query_idx].id;
	query_type = ctx->queries[query_idx].query_type;

	len = buf_len;

	ret = dns_msg_pack_query(buf, &len, (uint16_t)max_len,
				 dns_qname->data, dns_qname->len, dns_id,
				 (enum dns_rr_type)query_type);
	if (ret < 0) {
		return -EINVAL;
	}

	query_name = buf + DNS_MSG_HEADER_SIZE;

	/* Convert the query name to small case so that our
	 * hash checker can find it later when we get the answer.
	 */
	for (int i = 0; i < dns_qname->len; i++) {
		query_name[i] = tolower(query_name[i]);
	}

	/* Add \0 and query type (A or AAAA) to the hash. Note that
	 * the dns_qname->len contains the length of \0
	 */
	ctx->queries[query_idx].query_hash =
		crc16_ansi(query_name, dns_qname->len + 2);

	if (hop_limit > 0) {
		if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
			ret = set_ttl_hop_limit(sock, IPPROTO_IPV6,
						IPV6_UNICAST_HOPS,
						hop_limit);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
			ret = set_ttl_hop_limit(sock, IPPROTO_IP, IP_TTL,
						hop_limit);
		} else {
			ret = -ENOTSUP;
		}

		if (ret < 0) {
			NET_DBG("Cannot set %s to socket (%d)",
				family == AF_INET6 ? "hop limit" :
				(family == AF_INET ? "TTL" : "<unknown>"),
				ret);
			return ret;
		}
	}

	ret = -ENOENT;

	ARRAY_FOR_EACH(ctx->fds, i) {
		if (ctx->fds[i].fd == sock) {
			/* There was query to this server already */
			ret = 0;
			break;
		}

		if (ctx->fds[i].fd < 0) {
			ctx->fds[i].fd = sock;
			ctx->fds[i].events = ZSOCK_POLLIN;
			ret = 0;
			break;
		}
	}

	if (ret < 0) {
		NET_DBG("Cannot set %s to socket (%d)", "polling", ret);
		return ret;
	}

	if (family == AF_INET) {
		server_addr_len = sizeof(struct sockaddr_in);
	} else {
		server_addr_len = sizeof(struct sockaddr_in6);
	}

	ret = k_work_reschedule(&ctx->queries[query_idx].timer,
				ctx->queries[query_idx].timeout);
	if (ret < 0) {
		NET_DBG("[%u] cannot submit work to server idx %d for id %u "
			"ret %d", query_idx, server_idx, dns_id, ret);
		return ret;
	}

	NET_DBG("[%u] submitting work to server idx %d for id %u "
		"hash %u", query_idx, server_idx, dns_id,
		ctx->queries[query_idx].query_hash);

	ret = zsock_sendto(sock, buf, len, 0, server, server_addr_len);
	if (ret < 0) {
		NET_DBG("Cannot send query (%d)", -errno);
		return ret;
	} else {
		if (IS_ENABLED(CONFIG_NET_STATISTICS_DNS)) {
			struct net_if *iface = NULL;

			if (IS_ENABLED(CONFIG_NET_IPV6) && server->sa_family == AF_INET6) {
				iface = net_if_ipv6_select_src_iface(&net_sin6(server)->sin6_addr);
			} else if (IS_ENABLED(CONFIG_NET_IPV4) && server->sa_family == AF_INET) {
				iface = net_if_ipv4_select_src_iface(&net_sin(server)->sin_addr);
			}

			if (iface != NULL) {
				net_stats_update_dns_sent(iface);
			}
		}
	}

	return 0;
}

/* Must be invoked with context lock held */
static void dns_resolve_cancel_slot(struct dns_resolve_context *ctx, int slot)
{
	invoke_query_callback(DNS_EAI_CANCELED, NULL, &ctx->queries[slot]);

	release_query(&ctx->queries[slot]);
}

/* Must be invoked with context lock held */
static void dns_resolve_cancel_all(struct dns_resolve_context *ctx)
{
	int i;

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (ctx->queries[i].cb && ctx->queries[i].query) {
			dns_resolve_cancel_slot(ctx, i);
		}
	}
}

static int dns_resolve_cancel_with_hash(struct dns_resolve_context *ctx,
					uint16_t dns_id,
					uint16_t query_hash,
					const char *query_name)
{
	int ret = 0;
	int i;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (ctx->state == DNS_RESOLVE_CONTEXT_DEACTIVATING) {
		/*
		 * Cancel is part of context "deactivating" process, so no need
		 * to do anything more.
		 */
		goto unlock;
	}

	i = get_slot_by_id(ctx, dns_id, query_hash);
	if (i < 0) {
		ret = -ENOENT;
		goto unlock;
	}

	NET_DBG("Cancelling DNS req %u (name %s type %d hash %u)", dns_id,
		query_name == NULL ? "<unknown>" : query_name,
		ctx->queries[i].query_type, query_hash);

	dns_resolve_cancel_slot(ctx, i);

unlock:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int dns_resolve_cancel_with_name(struct dns_resolve_context *ctx,
				 uint16_t dns_id,
				 const char *query_name,
				 enum dns_query_type query_type)
{
	uint16_t query_hash = 0;

	if (query_name) {
		struct net_buf *buf;
		uint16_t len;
		int ret;

		/* Use net_buf as a temporary buffer to store the packed
		 * DNS name.
		 */
		buf = net_buf_alloc(&dns_msg_pool, ctx->buf_timeout);
		if (!buf) {
			return -ENOMEM;
		}

		ret = dns_msg_pack_qname(&len, buf->data,
					 net_buf_max_len(buf),
					 query_name);
		if (ret >= 0) {
			/* If the query string + \0 + query type (A or AAAA)
			 * does not fit the tmp buf, then bail out
			 */
			if ((len + 2) > net_buf_max_len(buf)) {
				net_buf_unref(buf);
				return -ENOMEM;
			}

			net_buf_add(buf, len);
			net_buf_add_be16(buf, query_type);

			query_hash = crc16_ansi(buf->data, len + 2);
		}

		net_buf_unref(buf);

		if (ret < 0) {
			return ret;
		}
	}

	return dns_resolve_cancel_with_hash(ctx, dns_id, query_hash,
					    query_name);
}

int dns_resolve_cancel(struct dns_resolve_context *ctx, uint16_t dns_id)
{
	return dns_resolve_cancel_with_name(ctx, dns_id, NULL, 0);
}

static void query_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dns_pending_query *pending_query =
		CONTAINER_OF(dwork, struct dns_pending_query, timer);
	int ret;

	/* We have to take the lock as we're inspecting protected content
	 * associated with the query.  But don't block the system work queue:
	 * if the lock can't be taken immediately, reschedule the work item to
	 * be run again after everything else has had a chance.
	 *
	 * Note that it's OK to use the k_work API on the delayable work
	 * without holding the lock: it's only the associated state in the
	 * containing structure that must be protected.
	 */
	ret = k_mutex_lock(&pending_query->ctx->lock, K_NO_WAIT);
	if (ret != 0) {
		struct k_work_delayable *dwork2 = k_work_delayable_from_work(work);

		/*
		 * Reschedule query timeout handler with some delay, so that all
		 * threads (including those with lower priorities) have a chance
		 * to move forward and release DNS context lock.
		 *
		 * Timeout value was arbitrarily chosen and can be updated in
		 * future if needed.
		 */
		k_work_reschedule(dwork2, K_MSEC(10));
		return;
	}

	NET_DBG("Query timeout DNS req %u type %d hash %u", pending_query->id,
		pending_query->query_type, pending_query->query_hash);

	/* The resolve cancel will invoke release_query(), but release will
	 * not be completed because the work item is still pending.  Instead
	 * the release will be completed when check_query_active() confirms
	 * the work item is no longer active.
	 */
	(void)dns_resolve_cancel_with_hash(pending_query->ctx,
					   pending_query->id,
					   pending_query->query_hash,
					   pending_query->query);

	k_mutex_unlock(&pending_query->ctx->lock);
}

int dns_resolve_name_internal(struct dns_resolve_context *ctx,
			      const char *query,
			      enum dns_query_type type,
			      uint16_t *dns_id,
			      dns_resolve_cb_t cb,
			      void *user_data,
			      int32_t timeout,
			      bool use_cache)
{
	k_timeout_t tout;
	struct net_buf *dns_data = NULL;
	struct net_buf *dns_qname = NULL;
	struct sockaddr addr;
	int ret, i = -1, j = 0;
	int ntry = 0, nfail = 0;
	bool mdns_query = false;
	uint8_t hop_limit;
#ifdef CONFIG_DNS_RESOLVER_CACHE
	struct dns_addrinfo cached_info[CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES] = {0};
#endif /* CONFIG_DNS_RESOLVER_CACHE */

	if (!ctx || !query || !cb) {
		return -EINVAL;
	}

	tout = SYS_TIMEOUT_MS(timeout);

	/* Timeout cannot be 0 as we cannot resolve name that fast.
	 */
	if (K_TIMEOUT_EQ(tout, K_NO_WAIT)) {
		return -EINVAL;
	}

	ret = net_ipaddr_parse(query, strlen(query), &addr);
	if (ret) {
		/* The query name was already in numeric form, no
		 * need to continue further.
		 */
		struct dns_addrinfo info = { 0 };

		if (type == DNS_QUERY_TYPE_A) {
			if (net_sin(&addr)->sin_family == AF_INET6) {
				return -EPFNOSUPPORT;
			}

			memcpy(net_sin(&info.ai_addr), net_sin(&addr),
			       sizeof(struct sockaddr_in));
			info.ai_family = AF_INET;
			info.ai_addr.sa_family = AF_INET;
			info.ai_addrlen = sizeof(struct sockaddr_in);
		} else if (type == DNS_QUERY_TYPE_AAAA) {
			/* We do not support AI_V4MAPPED atm, so if the user
			 * asks an IPv6 address but it is an IPv4 one, then
			 * return an error. Note that getaddrinfo() will swap
			 * the error to EINVAL, the EPFNOSUPPORT is returned
			 * here so that we can find it easily.
			 */
			if (net_sin(&addr)->sin_family == AF_INET) {
				return -EPFNOSUPPORT;
			}

#if defined(CONFIG_NET_IPV6)
			memcpy(net_sin6(&info.ai_addr), net_sin6(&addr),
			       sizeof(struct sockaddr_in6));
			info.ai_family = AF_INET6;
			info.ai_addr.sa_family = AF_INET6;
			info.ai_addrlen = sizeof(struct sockaddr_in6);
#else
			return -EAFNOSUPPORT;
#endif
		} else {
			goto try_resolve;
		}

		cb(DNS_EAI_INPROGRESS, &info, user_data);
		cb(DNS_EAI_ALLDONE, NULL, user_data);

		return 0;
	}

try_resolve:
#ifdef CONFIG_DNS_RESOLVER_CACHE
	if (use_cache) {
		ret = dns_cache_find(&dns_cache, query, type, cached_info,
				     ARRAY_SIZE(cached_info));
		if (ret > 0) {
			/* The query was cached, no
			 * need to continue further.
			 */
			for (size_t cache_index = 0; cache_index < ret; cache_index++) {
				cb(DNS_EAI_INPROGRESS, &cached_info[cache_index], user_data);
			}

			cb(DNS_EAI_ALLDONE, NULL, user_data);

			return 0;
		}
	}
#else
	ARG_UNUSED(use_cache);
#endif /* CONFIG_DNS_RESOLVER_CACHE */

	/* If we get a query to localhost, then short circuit early */
	if ((IS_ENABLED(CONFIG_NET_LOOPBACK) || IS_ENABLED(CONFIG_NET_TEST)) &&
	    strcmp(query, "localhost") == 0) {
		struct dns_addrinfo info = { 0 };

		if (type == DNS_QUERY_TYPE_A) {
			if (!IS_ENABLED(CONFIG_NET_IPV4)) {
				return -EAFNOSUPPORT;
			}

			struct in_addr addr4 = INADDR_LOOPBACK_INIT;

			memcpy(&net_sin(&info.ai_addr)->sin_addr, &addr4,
			       sizeof(struct in_addr));

			info.ai_family = AF_INET;
			info.ai_addr.sa_family = AF_INET;
			info.ai_addrlen = sizeof(struct sockaddr_in);

		} else if (type == DNS_QUERY_TYPE_AAAA) {
			if (!IS_ENABLED(CONFIG_NET_IPV6)) {
				return -EAFNOSUPPORT;
			}

			struct in6_addr addr6 = IN6ADDR_LOOPBACK_INIT;

			memcpy(&net_sin6(&info.ai_addr)->sin6_addr, &addr6,
			       sizeof(struct in6_addr));

			info.ai_family = AF_INET6;
			info.ai_addr.sa_family = AF_INET6;
			info.ai_addrlen = sizeof(struct sockaddr_in6);
		} else {
			return -EINVAL;
		}

		cb(DNS_EAI_INPROGRESS, &info, user_data);
		cb(DNS_EAI_ALLDONE, NULL, user_data);

		return 0;
	}

	if (IS_ENABLED(CONFIG_NET_HOSTNAME_ENABLE)) {
		const char *hostname = net_hostname_get();
		struct dns_addrinfo info = { 0 };

		/* If the hostname is the same as the query, then
		 * return a local address.
		 */
		if (strcmp(hostname, query) == 0) {

			if (type == DNS_QUERY_TYPE_A) {
				if (!IS_ENABLED(CONFIG_NET_IPV4)) {
					return -EAFNOSUPPORT;
				}

				struct in_addr addr4 = INADDR_LOOPBACK_INIT;
				const struct in_addr *paddr;

				paddr = net_if_ipv4_select_src_addr(NULL, &addr4);
				if (paddr == NULL) {
					return -ENOENT;
				}

				memcpy(&net_sin(&info.ai_addr)->sin_addr, paddr,
				       sizeof(struct in_addr));

				info.ai_family = AF_INET;
				info.ai_addr.sa_family = AF_INET;
				info.ai_addrlen = sizeof(struct sockaddr_in);

			} else if (type == DNS_QUERY_TYPE_AAAA) {
				if (!IS_ENABLED(CONFIG_NET_IPV6)) {
					return -EAFNOSUPPORT;
				}

				struct in6_addr addr6 = IN6ADDR_LOOPBACK_INIT;
				const struct in6_addr *paddr;

				paddr = net_if_ipv6_select_src_addr(NULL, &addr6);
				if (paddr == NULL) {
					return -ENOENT;
				}

				memcpy(&net_sin6(&info.ai_addr)->sin6_addr, paddr,
				       sizeof(struct in6_addr));

				info.ai_family = AF_INET6;
				info.ai_addr.sa_family = AF_INET6;
				info.ai_addrlen = sizeof(struct sockaddr_in6);
			} else {
				return -EINVAL;
			}

			cb(DNS_EAI_INPROGRESS, &info, user_data);
			cb(DNS_EAI_ALLDONE, NULL, user_data);

			return 0;
		}
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
		ret = -EINVAL;
		goto fail;
	}

	i = get_cb_slot(ctx);
	if (i < 0) {
		ret = -EAGAIN;
		goto fail;
	}

	ctx->queries[i].cb = cb;
	ctx->queries[i].timeout = tout;
	ctx->queries[i].query = query;
	ctx->queries[i].query_type = type;
	ctx->queries[i].user_data = user_data;
	ctx->queries[i].ctx = ctx;
	ctx->queries[i].query_hash = 0;

	k_work_init_delayable(&ctx->queries[i].timer, query_timeout);

	dns_data = net_buf_alloc(&dns_msg_pool, ctx->buf_timeout);
	if (!dns_data) {
		ret = -ENOMEM;
		goto quit;
	}

	dns_qname = net_buf_alloc(&dns_qname_pool, ctx->buf_timeout);
	if (!dns_qname) {
		ret = -ENOMEM;
		goto quit;
	}

	ret = dns_msg_pack_qname(&dns_qname->len, dns_qname->data,
				CONFIG_DNS_RESOLVER_MAX_QUERY_LEN, ctx->queries[i].query);
	if (ret < 0) {
		goto quit;
	}

	ctx->queries[i].id = sys_rand16_get();

	/* If mDNS is enabled, then send .local queries only to multicast
	 * address. For mDNS the id should be set to 0, see RFC 6762 ch. 18.1
	 * for details.
	 */
	if (IS_ENABLED(CONFIG_MDNS_RESOLVER)) {
		const char *ptr = strrchr(query, '.');

		/* Note that we memcmp() the \0 here too */
		if (ptr && !memcmp(ptr, (const void *){ ".local" }, 7)) {
			mdns_query = true;

			ctx->queries[i].id = 0;
		}
	}

	/* Do this immediately after calculating the Id so that the unit
	 * test will work properly.
	 */
	if (dns_id) {
		*dns_id = ctx->queries[i].id;

		NET_DBG("DNS id will be %u", *dns_id);
	}

	for (j = 0; j < SERVER_COUNT; j++) {
		hop_limit = 0U;

		if (ctx->servers[j].sock < 0) {
			continue;
		}

		/* If mDNS is enabled, then send .local queries only to
		 * a well known multicast mDNS server address.
		 */
		if (IS_ENABLED(CONFIG_MDNS_RESOLVER) && mdns_query &&
		    !ctx->servers[j].is_mdns) {
			continue;
		}

		/* If llmnr is enabled, then all the queries are sent to
		 * LLMNR multicast address unless it is a mDNS query.
		 */
		if (!mdns_query && IS_ENABLED(CONFIG_LLMNR_RESOLVER)) {
			if (!ctx->servers[j].is_llmnr) {
				continue;
			}

			hop_limit = 1U;
		}

		ntry++;
		ret = dns_write(ctx, j, i, dns_data->data,
				net_buf_max_len(dns_data),
				net_buf_max_len(dns_data),
				dns_qname, hop_limit);
		if (ret < 0) {
			nfail++;
			continue;
		}

		/* Do one concurrent query only for each name resolve.
		 * TODO: Change the i (query index) to do multiple concurrent
		 *       to each server.
		 */
		break;
	}

	if (nfail > 0) {
		NET_DBG("DNS query %d fails on %d attempts", nfail, ntry);
	}
	if ((ntry == 0) || (ntry == nfail)) {
		ret = -ENOENT;
		goto quit;
	}

	ret = 0;

quit:
	if (ret < 0) {
		if (i >= 0) {
			release_query(&ctx->queries[i]);
		}

		if (dns_id) {
			*dns_id = 0U;
		}
	}

	if (dns_data) {
		net_buf_unref(dns_data);
	}

	if (dns_qname) {
		net_buf_unref(dns_qname);
	}

fail:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int dns_resolve_name(struct dns_resolve_context *ctx,
		     const char *query,
		     enum dns_query_type type,
		     uint16_t *dns_id,
		     dns_resolve_cb_t cb,
		     void *user_data,
		     int32_t timeout)
{
	return dns_resolve_name_internal(ctx, query, type, dns_id, cb,
					 user_data, timeout, true);
}

static int dns_server_close(struct dns_resolve_context *ctx,
			    int server_idx)
{
	struct net_if *iface;

	if (ctx->servers[server_idx].sock < 0) {
		return -ENOENT;
	}

	(void)dns_dispatcher_unregister(&ctx->servers[server_idx].dispatcher);

	if (ctx->servers[server_idx].dns_server.sa_family == AF_INET6) {
		iface = net_if_ipv6_select_src_iface(
			&net_sin6(&ctx->servers[server_idx].dns_server)->sin6_addr);
	} else {
		iface = net_if_ipv4_select_src_iface(
			&net_sin(&ctx->servers[server_idx].dns_server)->sin_addr);
	}

	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
		net_mgmt_event_notify_with_info(
			NET_EVENT_DNS_SERVER_DEL,
			iface,
			(void *)&ctx->servers[server_idx].dns_server,
			sizeof(struct sockaddr));
	} else {
		net_mgmt_event_notify(NET_EVENT_DNS_SERVER_DEL, iface);
	}

	zsock_close(ctx->servers[server_idx].sock);

	ctx->servers[server_idx].sock = -1;
	ctx->servers[server_idx].dns_server.sa_family = 0;

	ARRAY_FOR_EACH(ctx->fds, j) {
		ctx->fds[j].fd = -1;
	}

	return 0;
}

/* Must be invoked with context lock held */
static int dns_resolve_close_locked(struct dns_resolve_context *ctx)
{
	int i, ret;

	if (ctx->state != DNS_RESOLVE_CONTEXT_ACTIVE) {
		return -ENOENT;
	}

	ctx->state = DNS_RESOLVE_CONTEXT_DEACTIVATING;

	/* ctx->net_ctx is never used in "deactivating" state. Additionally
	 * following code is guaranteed to be executed only by one thread at a
	 * time, due to required "active" -> "deactivating" state change. This
	 * means that it is safe to put net_ctx with mutex released.
	 *
	 * Released mutex will prevent lower networking layers from deadlock
	 * when calling cb_recv() (which acquires ctx->lock) just before closing
	 * network context.
	 */
	k_mutex_unlock(&ctx->lock);

	for (i = 0; i < SERVER_COUNT; i++) {
		ret = dns_server_close(ctx, i);
		if (ret < 0) {
			NET_DBG("Cannot close DNS server %d (%d)", i, ret);
		}
	}

	if (--init_called <= 0) {
		init_called = 0;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->state = DNS_RESOLVE_CONTEXT_INACTIVE;

	return 0;
}

int dns_resolve_close(struct dns_resolve_context *ctx)
{
	int ret;

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = dns_resolve_close_locked(ctx);
	k_mutex_unlock(&ctx->lock);

	return ret;
}

static bool dns_server_exists(struct dns_resolve_context *ctx,
			      const struct sockaddr *addr)
{
	for (int i = 0; i < SERVER_COUNT; i++) {
		if (IS_ENABLED(CONFIG_NET_IPV4) && (addr->sa_family == AF_INET) &&
		    (ctx->servers[i].dns_server.sa_family == AF_INET)) {
			if (net_ipv4_addr_cmp(&net_sin(addr)->sin_addr,
					      &net_sin(&ctx->servers[i].dns_server)->sin_addr)) {
				return true;
			}
		}

		if (IS_ENABLED(CONFIG_NET_IPV6) && (addr->sa_family == AF_INET6) &&
		    (ctx->servers[i].dns_server.sa_family == AF_INET6)) {
			if (net_ipv6_addr_cmp(&net_sin6(addr)->sin6_addr,
					      &net_sin6(&ctx->servers[i].dns_server)->sin6_addr)) {
				return true;
			}
		}
	}

	return false;
}

static bool dns_servers_exists(struct dns_resolve_context *ctx,
			       const char *servers[],
			       const struct sockaddr *servers_sa[])
{
	if (servers) {
		for (int i = 0; i < SERVER_COUNT && servers[i]; i++) {
			struct sockaddr addr;

			if (!net_ipaddr_parse(servers[i], strlen(servers[i]), &addr)) {
				continue;
			}

			if (!dns_server_exists(ctx, &addr)) {
				return false;
			}
		}
	}

	if (servers_sa) {
		for (int i = 0; i < SERVER_COUNT && servers_sa[i]; i++) {
			if (!dns_server_exists(ctx, servers_sa[i])) {
				return false;
			}
		}
	}

	return true;
}

static int do_dns_resolve_reconfigure(struct dns_resolve_context *ctx,
				      const char *servers[],
				      const struct sockaddr *servers_sa[],
				      int interfaces[],
				      bool do_close,
				      enum dns_server_source source)
{
	int err;

	if (!ctx) {
		return -ENOENT;
	}

	k_mutex_lock(&lock, K_FOREVER);
	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (dns_servers_exists(ctx, servers, servers_sa)) {
		/* DNS servers did not change. */
		err = 0;
		goto unlock;
	}

	if (ctx->state == DNS_RESOLVE_CONTEXT_DEACTIVATING) {
		err = -EBUSY;
		goto unlock;
	}

	if (ctx->state == DNS_RESOLVE_CONTEXT_ACTIVE &&
	    (do_close || init_called == 0)) {
		dns_resolve_cancel_all(ctx);

		err = dns_resolve_close_locked(ctx);
		if (err) {
			goto unlock;
		}

		/* Make sure we do fresh start once */
		do_close = true;
	}

	err = dns_resolve_init_locked(ctx, servers, servers_sa,
				      &resolve_svc, 0, interfaces,
				      do_close,
				      source);

unlock:
	k_mutex_unlock(&ctx->lock);
	k_mutex_unlock(&lock);

	return err;
}

int dns_resolve_reconfigure_with_interfaces(struct dns_resolve_context *ctx,
					    const char *servers[],
					    const struct sockaddr *servers_sa[],
					    int interfaces[],
					    enum dns_server_source source)
{
	return do_dns_resolve_reconfigure(ctx,
					  servers,
					  servers_sa,
					  interfaces,
					  IS_ENABLED(CONFIG_DNS_RECONFIGURE_CLEANUP) ?
					  true : false,
					  source);
}

int dns_resolve_reconfigure(struct dns_resolve_context *ctx,
			    const char *servers[],
			    const struct sockaddr *servers_sa[],
			    enum dns_server_source source)
{
	return do_dns_resolve_reconfigure(ctx,
					  servers,
					  servers_sa,
					  NULL,
					  IS_ENABLED(CONFIG_DNS_RECONFIGURE_CLEANUP) ?
					  true : false,
					  source);
}

static int dns_resolve_remove_and_check_source(struct dns_resolve_context *ctx, int if_index,
					       bool check_source,
					       enum dns_server_source source)
{
	int i;
	int ret = -ENOENT;
	int st = 0;

	if (!ctx) {
		return -ENOENT;
	}

	if (if_index <= 0) {
		/* If network interface index is 0, then do nothing.
		 * If your want to remove all the servers, then just call
		 * dns_resolve_close() directly.
		 */
		return -EINVAL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	for (i = 0; i < SERVER_COUNT; i++) {
		if (ctx->servers[i].if_index != if_index) {
			continue;
		}

		if (check_source && ctx->servers[i].source != source) {
			continue;
		}

		ctx->servers[i].if_index = 0;

		/* See comment in dns_resolve_close_locked() about
		 * releasing the lock before closing the server socket.
		 */
		k_mutex_unlock(&ctx->lock);

		ret = dns_server_close(ctx, i);

		k_mutex_lock(&ctx->lock, K_FOREVER);

		if (ret < 0) {
			st = ret;
		}
	}

	k_mutex_unlock(&ctx->lock);

	return st;
}

int dns_resolve_remove(struct dns_resolve_context *ctx, int if_index)
{
	return dns_resolve_remove_and_check_source(ctx, if_index, false,
						   DNS_SOURCE_UNKNOWN);
}

int dns_resolve_remove_source(struct dns_resolve_context *ctx, int if_index,
			      enum dns_server_source source)
{
	return dns_resolve_remove_and_check_source(ctx, if_index, true, source);
}

struct dns_resolve_context *dns_resolve_get_default(void)
{
	return &dns_default_ctx;
}

int dns_resolve_init_default(struct dns_resolve_context *ctx)
{
	int ret = 0;
#if defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	static const char *dns_servers[SERVER_COUNT + 1];
	int count = DNS_SERVER_COUNT;

	if (count > 5) {
		count = 5;
	}

	switch (count) {
#if DNS_SERVER_COUNT > 4
	case 5:
		dns_servers[4] = CONFIG_DNS_SERVER5;
		__fallthrough;
#endif
#if DNS_SERVER_COUNT > 3
	case 4:
		dns_servers[3] = CONFIG_DNS_SERVER4;
		__fallthrough;
#endif
#if DNS_SERVER_COUNT > 2
	case 3:
		dns_servers[2] = CONFIG_DNS_SERVER3;
		__fallthrough;
#endif
#if DNS_SERVER_COUNT > 1
	case 2:
		dns_servers[1] = CONFIG_DNS_SERVER2;
		__fallthrough;
#endif
#if DNS_SERVER_COUNT > 0
	case 1:
		dns_servers[0] = CONFIG_DNS_SERVER1;
		__fallthrough;
#endif
	case 0:
		break;
	}

#if defined(CONFIG_MDNS_RESOLVER) && (MDNS_SERVER_COUNT > 0)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	dns_servers[DNS_SERVER_COUNT + 1] = MDNS_IPV6_ADDR;
	dns_servers[DNS_SERVER_COUNT] = MDNS_IPV4_ADDR;
#else /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#if defined(CONFIG_NET_IPV6)
	dns_servers[DNS_SERVER_COUNT] = MDNS_IPV6_ADDR;
#endif
#if defined(CONFIG_NET_IPV4)
	dns_servers[DNS_SERVER_COUNT] = MDNS_IPV4_ADDR;
#endif
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#endif /* MDNS_RESOLVER && MDNS_SERVER_COUNT > 0 */

#if defined(CONFIG_LLMNR_RESOLVER) && (LLMNR_SERVER_COUNT > 0)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
	dns_servers[DNS_SERVER_COUNT + MDNS_SERVER_COUNT + 1] =
							LLMNR_IPV6_ADDR;
	dns_servers[DNS_SERVER_COUNT + MDNS_SERVER_COUNT] = LLMNR_IPV4_ADDR;
#else /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#if defined(CONFIG_NET_IPV6)
	dns_servers[DNS_SERVER_COUNT + MDNS_SERVER_COUNT] = LLMNR_IPV6_ADDR;
#endif
#if defined(CONFIG_NET_IPV4)
	dns_servers[DNS_SERVER_COUNT + MDNS_SERVER_COUNT] = LLMNR_IPV4_ADDR;
#endif
#endif /* CONFIG_NET_IPV6 && CONFIG_NET_IPV4 */
#endif /* LLMNR_RESOLVER && LLMNR_SERVER_COUNT > 0 */

	dns_servers[SERVER_COUNT] = NULL;

	ret = dns_resolve_init(ctx, dns_servers, NULL);
	if (ret < 0) {
		NET_WARN("Cannot initialize DNS resolver (%d)", ret);
	}
#else
	/* We must always call init even if there are no servers configured so
	 * that DNS mutex gets initialized properly.
	 */
	(void)dns_resolve_init(dns_resolve_get_default(), NULL, NULL);
#endif
	return ret;
}

#ifdef CONFIG_DNS_RESOLVER_AUTO_INIT
void dns_init_resolver(void)
{
	dns_resolve_init_default(dns_resolve_get_default());
}
#endif /* CONFIG_DNS_RESOLVER_AUTO_INIT */
