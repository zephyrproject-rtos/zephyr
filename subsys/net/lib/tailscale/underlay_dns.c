/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/net/socket_service.h>

LOG_MODULE_REGISTER(tailscale_underlay_dns, LOG_LEVEL_INF);

#include "../../ip/net_private.h"
#include "underlay_dns.h"

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *event);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(underlay_dns_service, dns_dispatcher_svc_handler,
				      DNS_RESOLVER_MAX_POLL);
static K_MUTEX_DEFINE(underlay_dns_lock);
static struct dns_resolve_context underlay_dns;

struct underlay_query {
	struct net_sockaddr_storage *addresses;
	size_t capacity;
	size_t count;
	enum dns_resolve_status status;
	struct k_sem done;
};

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
static int resolve_offloaded(const char *host, net_sa_family_t family, uint16_t port,
			     struct net_sockaddr_storage *addresses, size_t capacity)
{
	struct zsock_addrinfo hints = {.ai_family = family, .ai_socktype = NET_SOCK_STREAM};
	struct zsock_addrinfo *result;
	net_socklen_t len = family == NET_AF_INET ? sizeof(struct net_sockaddr_in)
						 : sizeof(struct net_sockaddr_in6);
	char service[sizeof("65535")];
	size_t count = 0;

	snprintf(service, sizeof(service), "%u", port);
	int rc = socket_offload_getaddrinfo(host, service, &hints, &result);

	if (rc != 0) {
		return rc == DNS_EAI_AGAIN ? -ETIMEDOUT : -EHOSTUNREACH;
	}
	for (struct zsock_addrinfo *entry = result; entry != NULL && count < capacity;
	     entry = entry->ai_next) {
		if (entry->ai_family != family || entry->ai_addr == NULL ||
		    entry->ai_addrlen != len) {
			continue;
		}
		addresses[count] = (struct net_sockaddr_storage){0};
		memcpy(&addresses[count++], entry->ai_addr, len);
	}
	socket_offload_freeaddrinfo(result);
	return count > 0U ? (int)count : -EHOSTUNREACH;
}
#endif

static void resolved(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data)
{
	struct underlay_query *query = user_data;

	if (status == DNS_EAI_INPROGRESS) {
		if (query->count < query->capacity) {
			query->addresses[query->count++] = info->ai_addr_storage;
		}
		return;
	}
	query->status = status;
	k_sem_give(&query->done);
}

static int open_resolver(int if_index)
{
	struct dns_resolve_context *configured = dns_resolve_get_default();
	struct net_sockaddr_storage storage[DNS_RESOLVER_MAX_SERVERS];
	const struct net_sockaddr *servers[ARRAY_SIZE(storage) + 1];
	int interfaces[ARRAY_SIZE(storage)];
	size_t count = 0;
	int rc;

	if (configured->state == DNS_RESOLVE_CONTEXT_UNINITIALIZED) {
		return -ENETDOWN;
	}
	k_mutex_lock(&configured->lock, K_FOREVER);
	if (configured->state == DNS_RESOLVE_CONTEXT_ACTIVE) {
		ARRAY_FOR_EACH(configured->servers, i) {
			const struct dns_server_info *server = &configured->servers[i];
			net_sa_family_t family = server->dns_server_addr.ss_family;

			if (server->sock < 0 || server->is_mdns || server->is_llmnr ||
			    (family != NET_AF_INET && family != NET_AF_INET6) ||
			    (server->if_index > 0 && server->if_index != if_index)) {
				continue;
			}
			storage[count] = server->dns_server_addr;
			servers[count] = net_sad(&storage[count]);
			interfaces[count++] = if_index;
			if (count == ARRAY_SIZE(storage)) {
				break;
			}
		}
	}
	k_mutex_unlock(&configured->lock);
	if (count == 0U) {
		return -ENETDOWN;
	}
	servers[count] = NULL;
	rc = dns_resolve_init_with_svc(&underlay_dns, NULL, servers, &underlay_dns_service, 0,
				      interfaces);
	if (rc < 0) {
		/* Initialization can fail after opening some of the scoped sockets. */
		(void)dns_resolve_remove(&underlay_dns, if_index);
	}
	return rc;
}

static int resolve_native(int if_index, const char *host, enum dns_query_type type,
			  struct underlay_query *query)
{
	const k_timepoint_t deadline =
		sys_timepoint_calc(K_MSEC(CONFIG_NET_SOCKETS_DNS_TIMEOUT));
	uint64_t backoff_ms = CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL;
	int rc;

	if (k_mutex_lock(&underlay_dns_lock, sys_timepoint_timeout(deadline)) != 0) {
		return -ETIMEDOUT;
	}
	rc = open_resolver(if_index);
	if (rc < 0) {
		goto unlock;
	}

	k_sem_init(&query->done, 0, 1);
	do {
		uint16_t id;
		k_timeout_t remaining = sys_timepoint_timeout(deadline);
		int32_t timeout_ms = MIN(backoff_ms, k_ticks_to_ms_ceil64(remaining.ticks));

		if (timeout_ms == 0) {
			rc = -ETIMEDOUT;
			break;
		}
		query->count = 0;
		rc = dns_resolve_name_internal(&underlay_dns, host, type, &id, resolved, query,
					       timeout_ms, false);
		if (rc != 0) {
			rc = rc == -EAGAIN ? -ETIMEDOUT : rc;
			break;
		}
		if (k_sem_take(&query->done, sys_timepoint_timeout(deadline)) != 0) {
			(void)dns_resolve_cancel_with_name(&underlay_dns, id, host, type);
			k_sem_reset(&query->done);
			query->status = DNS_EAI_CANCELED;
		}
		backoff_ms = MIN(backoff_ms * 2, CONFIG_NET_SOCKETS_DNS_TIMEOUT);
	} while (query->status == DNS_EAI_CANCELED && !sys_timepoint_expired(deadline));

	(void)dns_resolve_close(&underlay_dns);
	if (rc == 0) {
		if (query->status == DNS_EAI_CANCELED || query->status == DNS_EAI_AGAIN) {
			rc = -ETIMEDOUT;
		} else if (query->status != DNS_EAI_ALLDONE || query->count == 0U) {
			rc = -EHOSTUNREACH;
		}
	}
unlock:
	k_mutex_unlock(&underlay_dns_lock);
	return rc;
}

int tailscale_underlay_resolve(struct net_if *iface, const char *host, net_sa_family_t family,
			      uint16_t port,
			      struct net_sockaddr_storage *addresses, size_t capacity)
{
	struct underlay_query query = {.addresses = addresses, .capacity = capacity};
	enum dns_query_type type = family == NET_AF_INET6 ? DNS_QUERY_TYPE_AAAA : DNS_QUERY_TYPE_A;
	int if_index;

	if (iface == NULL || host == NULL || addresses == NULL || capacity == 0U) {
		return -EINVAL;
	}
	if (family != NET_AF_INET && family != NET_AF_INET6) {
		return -EAFNOSUPPORT;
	}
	if_index = net_if_get_by_iface(iface);
	if (if_index <= 0) {
		return -ENODEV;
	}
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	if (socket_offload_dns_is_enabled()) {
		return resolve_offloaded(host, family, port, addresses, capacity);
	}
#endif
	addresses[0] = (struct net_sockaddr_storage){0};
	if (net_ipaddr_parse(host, strlen(host), net_sad(&addresses[0]))) {
		if (addresses[0].ss_family != family) {
			return -EAFNOSUPPORT;
		}
		query.count = 1;
	} else {
		int rc = resolve_native(if_index, host, type, &query);

		if (rc < 0) {
			return rc;
		}
	}

	for (size_t i = 0; i < query.count; i++) {
		if (family == NET_AF_INET6) {
			net_sin6(net_sad(&addresses[i]))->sin6_port = net_htons(port);
		} else {
			net_sin(net_sad(&addresses[i]))->sin_port = net_htons(port);
		}
	}
	return (int)query.count;
}
