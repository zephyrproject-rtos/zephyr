/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* libc headers */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Zephyr headers */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_addr, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/internal/syscall_handler.h>

#include "../../ip/route_ipv4.h"
#include "../../ip/route_ipv6.h"

#if defined(CONFIG_DNS_RESOLVER) || defined(CONFIG_NET_IP)
#define ANY_RESOLVER

#if defined(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES)
#define AI_ARR_MAX CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES
#else
#define AI_ARR_MAX 1
#endif /* defined(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES) */

/* Initialize static fields of addrinfo structure. A macro to let it work
 * with any net_sockaddr_* type.
 */
#define INIT_ADDRINFO(addrinfo, sockaddr) { \
		(addrinfo)->ai_addr = net_sad(&(addrinfo)->_ai_addr); \
		(addrinfo)->ai_addrlen = sizeof(*(sockaddr)); \
		(addrinfo)->ai_canonname = (addrinfo)->_ai_canonname; \
		(addrinfo)->_ai_canonname[0] = '\0'; \
		(addrinfo)->ai_next = NULL; \
	}

#endif

#if defined(CONFIG_DNS_RESOLVER)

struct getaddrinfo_state {
	const struct zsock_addrinfo *hints;
	struct k_sem sem;
	int status;
	uint16_t idx;
	uint16_t port;
	uint16_t dns_id;
	/* Entries of the first queried family */
	uint16_t first_count;
	/* Slots the second family may take back from the first one */
	uint16_t reserve;
	struct zsock_addrinfo *ai_arr;
};

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info, void *user_data)
{
	struct getaddrinfo_state *state = user_data;
	struct zsock_addrinfo *ai;
	int socktype = NET_SOCK_STREAM;

	NET_DBG("dns status: %d", status);

	if (info == NULL) {
		if (status == DNS_EAI_ALLDONE) {
			status = 0;
		}
		state->status = status;
		k_sem_give(&state->sem);
		return;
	}

	if (state->idx >= AI_ARR_MAX) {
		if (state->first_count <= AI_ARR_MAX - state->reserve) {
			NET_DBG("getaddrinfo entries full, dropping address");
			return;
		}

		/* Take back the last slot of the first family: move the entries
		 * of the second family one slot towards the start of the array.
		 * sort_results() fixes the self-pointers of the moved entries.
		 */
		memmove(&state->ai_arr[state->first_count - 1U],
			&state->ai_arr[state->first_count],
			(state->idx - state->first_count) * sizeof(state->ai_arr[0]));
		state->first_count--;
		state->idx--;
	}

	ai = &state->ai_arr[state->idx];
	if (state->idx > 0) {
		state->ai_arr[state->idx - 1].ai_next = ai;
	}

	memcpy(&ai->_ai_addr, &info->ai_addr_storage, info->ai_addrlen);
	net_sin(net_sad(&ai->_ai_addr))->sin_port = state->port;
	ai->ai_addr = net_sad(&ai->_ai_addr);
	ai->ai_addrlen = info->ai_addrlen;
	memcpy(&ai->_ai_canonname, &info->ai_canonname,
	       sizeof(ai->_ai_canonname));
	ai->ai_canonname = ai->_ai_canonname;
	ai->ai_family = info->ai_family;

	if (state->hints) {
		if (state->hints->ai_socktype) {
			socktype = state->hints->ai_socktype;
		}
	}

	ai->ai_socktype = socktype;
	ai->ai_protocol = (socktype == NET_SOCK_DGRAM) ? NET_IPPROTO_UDP : NET_IPPROTO_TCP;

	state->idx++;
}

static k_timeout_t recalc_timeout(k_timepoint_t end, k_timeout_t timeout)
{
	k_timepoint_t new_timepoint;

	timeout.ticks <<= 1;

	new_timepoint = sys_timepoint_calc(timeout);

	if (sys_timepoint_cmp(end, new_timepoint) < 0) {
		timeout = sys_timepoint_timeout(end);
	}

	return timeout;
}

static int exec_query(const char *host, int family,
		      struct getaddrinfo_state *ai_state)
{
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(CONFIG_NET_SOCKETS_DNS_TIMEOUT));
	k_timeout_t timeout = K_MSEC(MIN(CONFIG_NET_SOCKETS_DNS_TIMEOUT,
					 CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL));
	int timeout_ms;
	int st, ret;

	if (family == NET_AF_INET6) {
		qtype = DNS_QUERY_TYPE_AAAA;
	}

again:
	timeout_ms = k_ticks_to_ms_ceil32(timeout.ticks);

	NET_DBG("Timeout %d", timeout_ms);

	ret = dns_get_addr_info(host, qtype, &ai_state->dns_id,
				dns_resolve_cb, ai_state, timeout_ms);
	if (ret == 0) {
		/* If the resolver callback is not called for any reason, let the
		 * semaphore timeout so getaddrinfo() does not hang forever.
		 * Keep sem timeout slightly longer than the DNS timeout.
		 */
		ret = k_sem_take(&ai_state->sem, K_MSEC(timeout_ms + 100));
		if (ret == -EAGAIN) {
			/* Explicitly cancel timed out query before retrying. This
			 * prevents delayed callbacks from using stack-based state
			 * after getaddrinfo() returns.
			 *
			 * Use name-qualified cancellation so valid resolver-assigned
			 * id 0 queries (e.g. mDNS) are also canceled without risking
			 * canceling an unrelated query that happens to use id 0.
			 */
			(void) dns_cancel_addr_info_with_name(host, qtype, ai_state->dns_id);

			if (!sys_timepoint_expired(end)) {
				k_sem_reset(&ai_state->sem);
				timeout = recalc_timeout(end, timeout);
				goto again;
			}
			st = DNS_EAI_AGAIN;
		} else {
			if (ai_state->status == DNS_EAI_CANCELED) {
				if (!sys_timepoint_expired(end)) {
					timeout = recalc_timeout(end, timeout);
					goto again;
				}
			}

			st = ai_state->status;
		}
	} else if (ret == -EPFNOSUPPORT) {
		/* If we are returned -EPFNOSUPPORT then that will indicate
		 * wrong address family type queried. Check that and return
		 * DNS_EAI_ADDRFAMILY.
		 */
		st = DNS_EAI_ADDRFAMILY;
	} else if (ret == -EAGAIN) {
		/* Temporary resolver-side condition (e.g. no available query slot). */
		st = DNS_EAI_AGAIN;
	} else {
		errno = -ret;
		st = DNS_EAI_SYSTEM;
	}

	return st;
}

static bool ipv6_dst_is_reachable(const struct net_in6_addr *dst)
{
	const struct net_in6_addr *src;
	struct net_if *iface = NULL;

	/* The loopback interface is always there */
	if (net_ipv6_is_addr_loopback(dst)) {
		return true;
	}

	/* We need a source address other than loopback */
	src = net_if_ipv6_select_src_addr(NULL, dst);
	if (src == NULL || net_ipv6_is_addr_unspecified(src) ||
	    net_ipv6_is_addr_loopback(src)) {
		return false;
	}

	/* Link-local destinations are on-link by definition */
	if (net_ipv6_is_ll_addr(dst)) {
		return true;
	}

	/* A link-local source cannot reach a global destination */
	if (net_ipv6_is_ll_addr(src)) {
		return false;
	}

	/* The destination matches an on-link prefix */
	if (net_if_ipv6_addr_onlink(NULL, dst)) {
		return true;
	}

	/* Links without ND or point-to-point links */
	if (net_if_ipv6_addr_lookup(src, &iface) != NULL &&
	    (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND) ||
	     net_if_flag_is_set(iface, NET_IF_POINTOPOINT))) {
		return true;
	}

	/* Check for a route to the destination */
	if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) &&
	    net_route_ipv6_lookup(NULL, dst) != NULL) {
		return true;
	}

	/* Otherwise, we need a default router on any interface */
	return net_if_ipv6_router_find_default(NULL, dst) != NULL;
}

/* Only interfaces that resolve the next hop with ARP need a gateway or a
 * default router to send off-link: other L2s such as PPP hand off-link
 * packets to the link as is.
 */
static bool ipv4_iface_uses_arp(struct net_if *iface)
{
	if (!IS_ENABLED(CONFIG_NET_ARP)) {
		return false;
	}

	return (IS_ENABLED(CONFIG_NET_L2_ETHERNET) &&
		net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) ||
	       net_eth_is_vlan_interface(iface);
}

static bool ipv4_dst_is_reachable(const struct net_in_addr *dst)
{
	const struct net_in_addr *src;
	struct net_if *iface = NULL;
	struct net_in_addr gw;

	/* The loopback interface is always there */
	if (net_ipv4_is_addr_loopback(dst)) {
		return true;
	}

	/* We need a source address other than loopback */
	src = net_if_ipv4_select_src_addr(NULL, dst);
	if (src == NULL || net_ipv4_is_addr_unspecified(src) ||
	    net_ipv4_is_addr_loopback(src)) {
		return false;
	}

	/* Link-local destinations are on-link by definition */
	if (net_ipv4_is_ll_addr(dst)) {
		return true;
	}

	/* A link-local source cannot reach a global destination */
	if (net_ipv4_is_ll_addr(src)) {
		return false;
	}

	if (net_if_ipv4_addr_lookup(src, &iface) == NULL) {
		return false;
	}

	/* The destination is in the subnet of the source interface */
	if (net_if_ipv4_addr_onlink(&iface, dst)) {
		return true;
	}

	if (!ipv4_iface_uses_arp(iface)) {
		return true;
	}

	/* Check for a route to the destination */
	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) &&
	    net_route_ipv4_lookup(NULL, dst) != NULL) {
		return true;
	}

	/* Otherwise, we need a gateway on the source interface or a default
	 * router on any interface
	 */
	gw = net_if_ipv4_get_gw(iface);

	return !net_ipv4_is_addr_unspecified(&gw) ||
	       net_if_ipv4_router_find_default(NULL, dst) != NULL;
}

/* A destination is reachable if we have a source address for it, and it is
 * either on-link or there is a default router for it.
 */
static bool dst_is_reachable(const struct zsock_addrinfo *ai)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && ai->ai_family == NET_AF_INET6) {
		return ipv6_dst_is_reachable(&net_sin6(ai->ai_addr)->sin6_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && ai->ai_family == NET_AF_INET) {
		return ipv4_dst_is_reachable(&net_sin(ai->ai_addr)->sin_addr);
	}

	return false;
}

struct dest_info {
	int family;
	bool reachable;
};

/* Compare two destinations as per a subset of RFC 6724 section 6. Returns
 * a negative value if a is preferred, a positive value if b is preferred,
 * 0 otherwise.
 */
static int compare_dests(const struct dest_info *a, const struct dest_info *b,
			 int preferred_family)
{
	/* Rule 1: avoid unusable destinations */
	if (a->reachable != b->reachable) {
		return a->reachable ? -1 : 1;
	}

	/* Rule 6: prefer higher precedence */
	if (a->family != b->family) {
		return a->family == preferred_family ? -1 : 1;
	}

	return 0;
}

/* Order results as per a subset of RFC 6724 section 6. Rule #1: avoid
 * unusable destinations. From rule #6: prioritize IPv6 over IPv4.
 */
static void sort_results(struct zsock_addrinfo *ai_arr, uint16_t count)
{
	const int preferred_family =
		IS_ENABLED(CONFIG_NET_SOCKETS_DNS_PREFER_IPV4) ? NET_AF_INET : NET_AF_INET6;
	struct dest_info info[AI_ARR_MAX];
	struct zsock_addrinfo tmp;
	struct dest_info tmp_info;

	if (!IS_ENABLED(CONFIG_NET_IPV4) || !IS_ENABLED(CONFIG_NET_IPV6) || count < 2U) {
		return;
	}

	__ASSERT_NO_MSG(count <= AI_ARR_MAX);

	/* Prepare data for sorting */
	for (uint16_t idx = 0U; idx < count; idx++) {
		info[idx].family = ai_arr[idx].ai_family;
		info[idx].reachable = dst_is_reachable(&ai_arr[idx]);
	}

	/* Stable insertion sort */
	for (uint16_t idx = 1U; idx < count; idx++) {
		for (uint16_t pos = idx;
		     pos > 0U && compare_dests(&info[pos - 1U], &info[pos], preferred_family) > 0;
		     pos--) {
			tmp_info = info[pos];
			info[pos] = info[pos - 1U];
			info[pos - 1U] = tmp_info;

			tmp = ai_arr[pos];
			ai_arr[pos] = ai_arr[pos - 1U];
			ai_arr[pos - 1U] = tmp;
		}
	}

	/* Entries have self-pointers. Fix them after we moved them */
	for (uint16_t idx = 0U; idx < count; idx++) {
		ai_arr[idx].ai_addr = net_sad(&ai_arr[idx]._ai_addr);
		ai_arr[idx].ai_canonname = ai_arr[idx]._ai_canonname;
		ai_arr[idx].ai_next = &ai_arr[idx + 1U];
	}
}

static int getaddrinfo_null_host(int port, const struct zsock_addrinfo *hints,
				struct zsock_addrinfo *res)
{
	if (!hints || !(hints->ai_flags & ZSOCK_AI_PASSIVE)) {
		return DNS_EAI_FAIL;
	}

	/* For NET_AF_UNSPEC, should we default to IPv6 or IPv4? */
	if (hints->ai_family == NET_AF_INET || hints->ai_family == NET_AF_UNSPEC) {
		struct net_sockaddr_in *addr = net_sin(net_sad(&res->_ai_addr));

		addr->sin_addr.s_addr = NET_INADDR_ANY;
		addr->sin_port = net_htons(port);
		addr->sin_family = NET_AF_INET;
		INIT_ADDRINFO(res, addr);
		res->ai_family = NET_AF_INET;
	} else if (hints->ai_family == NET_AF_INET6) {
		struct net_sockaddr_in6 *addr6 = net_sin6(net_sad(&res->_ai_addr));

		addr6->sin6_addr = net_in6addr_any;
		addr6->sin6_port = net_htons(port);
		addr6->sin6_family = NET_AF_INET6;
		INIT_ADDRINFO(res, addr6);
		res->ai_family = NET_AF_INET6;
	} else {
		return DNS_EAI_FAIL;
	}

	if (hints->ai_socktype == NET_SOCK_DGRAM) {
		res->ai_socktype = NET_SOCK_DGRAM;
		res->ai_protocol = NET_IPPROTO_UDP;
	} else {
		res->ai_socktype = NET_SOCK_STREAM;
		res->ai_protocol = NET_IPPROTO_TCP;
	}
	return 0;
}

/* Query one family, unless it is disabled or excluded by the hints */
static int query_family(const char *host, int hints_family, int family,
			struct getaddrinfo_state *state)
{
	if ((family == NET_AF_INET && !IS_ENABLED(CONFIG_NET_IPV4)) ||
	    (family == NET_AF_INET6 && !IS_ENABLED(CONFIG_NET_IPV6)) ||
	    (hints_family != NET_AF_UNSPEC && hints_family != family)) {
		return DNS_EAI_ADDRFAMILY;
	}

	return exec_query(host, family, state);
}

int z_impl_z_zsock_getaddrinfo_internal(const char *host, const char *service,
				       const struct zsock_addrinfo *hints,
				       struct zsock_addrinfo *res)
{
	int family = NET_AF_UNSPEC;
	int ai_flags = 0;
	long int port = 0;
	const int first = IS_ENABLED(CONFIG_NET_SOCKETS_DNS_PREFER_IPV4) ?
			  NET_AF_INET : NET_AF_INET6;
	const int second = first == NET_AF_INET6 ? NET_AF_INET : NET_AF_INET6;
	int st1;
	int st2;
	struct net_sockaddr *ai_addr;
	struct getaddrinfo_state ai_state;

	if (hints) {
		family = hints->ai_family;
		ai_flags = hints->ai_flags;

		if ((family != NET_AF_UNSPEC) && (family != NET_AF_INET) &&
		    (family != NET_AF_INET6)) {
			return DNS_EAI_ADDRFAMILY;
		}
	}

	if (ai_flags & ZSOCK_AI_NUMERICHOST) {
		/* Asked to resolve host as numeric, but it wasn't possible
		 * to do that.
		 */
		return DNS_EAI_FAIL;
	}

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > 65535) {
			return DNS_EAI_NONAME;
		}
	}

	if (host == NULL) {
		/* Per POSIX, both can't be NULL. */
		if (service == NULL) {
			errno = EINVAL;
			return DNS_EAI_SYSTEM;
		}

		return getaddrinfo_null_host(port, hints, res);
	}

	ai_state.hints = hints;
	ai_state.idx = 0U;
	ai_state.port = net_htons(port);
	ai_state.ai_arr = res;
	ai_state.dns_id = 0;
	ai_state.first_count = 0U;
	ai_state.reserve = 0U;
	k_sem_init(&ai_state.sem, 0, K_SEM_MAX_LIMIT);

	/* The preferred family is queried first and may fill all slots.
	 * Sorting is done after fetching both families.
	 */
	st1 = query_family(host, family, first, &ai_state);
	if (st1 == DNS_EAI_AGAIN) {
		return st1;
	}

	/* The second family takes back up to half of the slots */
	ai_state.first_count = ai_state.idx;
	ai_state.reserve = AI_ARR_MAX / 2U;
	st2 = query_family(host, family, second, &ai_state);
	if (st2 == DNS_EAI_AGAIN) {
		return st2;
	}

	for (uint16_t idx = 0; idx < ai_state.idx; idx++) {
		ai_addr = net_sad(&ai_state.ai_arr[idx]._ai_addr);
		net_sin(ai_addr)->sin_port = net_htons(port);
	}

	/* If both attempts failed, it's error */
	if (st1 != 0 && st2 != 0) {
		return st1 != DNS_EAI_ADDRFAMILY ? st1 : st2;
	}

	if (family == NET_AF_UNSPEC) {
		sort_results(ai_state.ai_arr, ai_state.idx);
	}

	/* Mark entry as last */
	ai_state.ai_arr[ai_state.idx - 1].ai_next = NULL;

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_z_zsock_getaddrinfo_internal(const char *host,
					const char *service,
				       const struct zsock_addrinfo *hints,
				       struct zsock_addrinfo *res)
{
	struct zsock_addrinfo hints_copy;
	char *host_copy = NULL, *service_copy = NULL;
	uint32_t ret;

	if (hints) {
		K_OOPS(k_usermode_from_copy(&hints_copy, (void *)hints,
					sizeof(hints_copy)));
	}
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(res, AI_ARR_MAX, sizeof(struct zsock_addrinfo)));

	if (service) {
		service_copy = k_usermode_string_alloc_copy((char *)service, 64);
		if (!service_copy) {
			ret = DNS_EAI_MEMORY;
			goto out;
		}
	}

	if (host) {
		host_copy = k_usermode_string_alloc_copy((char *)host, 64);
		if (!host_copy) {
			ret = DNS_EAI_MEMORY;
			goto out;
		}
	}

	ret = z_impl_z_zsock_getaddrinfo_internal(host_copy, service_copy,
						 hints ? &hints_copy : NULL,
						 (struct zsock_addrinfo *)res);
out:
	k_free(service_copy);
	k_free(host_copy);

	return ret;
}
#include <zephyr/syscalls/z_zsock_getaddrinfo_internal_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* defined(CONFIG_DNS_RESOLVER) */

#if defined(CONFIG_NET_IP)
static int try_resolve_literal_addr(const char *host, const char *service,
				    const struct zsock_addrinfo *hints,
				    struct zsock_addrinfo *res)
{
	int family = NET_AF_UNSPEC;
	int resolved_family = NET_AF_UNSPEC;
	long port = 0;
	bool result;
	int socktype = NET_SOCK_STREAM;
	int protocol = NET_IPPROTO_TCP;

	if (!host) {
		return DNS_EAI_NONAME;
	}

	if (hints) {
		family = hints->ai_family;
		if (hints->ai_socktype == NET_SOCK_DGRAM) {
			socktype = NET_SOCK_DGRAM;
			protocol = NET_IPPROTO_UDP;
		}
	}

	result = net_ipaddr_parse(host, strlen(host), net_sad(&res->_ai_addr));

	if (!result) {
		return DNS_EAI_NONAME;
	}

	resolved_family = res->_ai_addr.ss_family;

	if ((family != NET_AF_UNSPEC) && (resolved_family != family)) {
		return DNS_EAI_NONAME;
	}

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > 65535) {
			return DNS_EAI_NONAME;
		}
	}

	res->ai_family = resolved_family;
	res->ai_socktype = socktype;
	res->ai_protocol = protocol;

	switch (resolved_family) {
	case NET_AF_INET:
	{
		struct net_sockaddr_in *addr = net_sin(net_sad(&res->_ai_addr));

		INIT_ADDRINFO(res, addr);
		addr->sin_port = net_htons(port);
		addr->sin_family = NET_AF_INET;
		break;
	}

	case NET_AF_INET6:
	{
		struct net_sockaddr_in6 *addr = net_sin6(net_sad(&res->_ai_addr));

		INIT_ADDRINFO(res, addr);
		addr->sin6_port = net_htons(port);
		addr->sin6_family = NET_AF_INET6;
		break;
	}

	default:
		return DNS_EAI_NONAME;
	}

	return 0;
}
#endif /* CONFIG_NET_IP */

int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	if (socket_offload_dns_is_enabled()) {
		return socket_offload_getaddrinfo(host, service, hints, res);
	}

	int ret = DNS_EAI_FAIL;

#if defined(ANY_RESOLVER)
	*res = k_calloc(AI_ARR_MAX, sizeof(struct zsock_addrinfo));
	if (!(*res)) {
		return DNS_EAI_MEMORY;
	}
#endif

#if defined(CONFIG_NET_IP)
	/* Resolve literal address even if DNS is not available */
	if (ret) {
		ret = try_resolve_literal_addr(host, service, hints, *res);
	}
#endif

#if defined(CONFIG_DNS_RESOLVER)
	if (ret) {
		ret = z_zsock_getaddrinfo_internal(host, service, hints, *res);
	}
#endif

#if defined(ANY_RESOLVER)
	if (ret) {
		k_free(*res);
		*res = NULL;
	}
#endif

	return ret;
}

void zsock_freeaddrinfo(struct zsock_addrinfo *ai)
{
	if (socket_offload_dns_is_enabled()) {
		socket_offload_freeaddrinfo(ai);
		return;
	}

	k_free(ai);
}

#define ERR(e) case DNS_ ## e: return #e
const char *zsock_gai_strerror(int errcode)
{
	switch (errcode) {
	ERR(EAI_BADFLAGS);
	ERR(EAI_NONAME);
	ERR(EAI_AGAIN);
	ERR(EAI_FAIL);
	ERR(EAI_NODATA);
	ERR(EAI_FAMILY);
	ERR(EAI_SOCKTYPE);
	ERR(EAI_SERVICE);
	ERR(EAI_ADDRFAMILY);
	ERR(EAI_MEMORY);
	ERR(EAI_SYSTEM);
	ERR(EAI_OVERFLOW);
	ERR(EAI_INPROGRESS);
	ERR(EAI_CANCELED);
	ERR(EAI_NOTCANCELED);
	ERR(EAI_ALLDONE);
	ERR(EAI_IDN_ENCODE);

	default:
		return "EAI_UNKNOWN";
	}
}
#undef ERR
