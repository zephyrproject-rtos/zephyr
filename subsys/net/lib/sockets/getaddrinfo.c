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
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/internal/syscall_handler.h>

#if defined(CONFIG_DNS_RESOLVER) || defined(CONFIG_NET_IP)
#define ANY_RESOLVER

#if defined(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES)
#define AI_ARR_MAX CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES
#else
#define AI_ARR_MAX 1
#endif /* defined(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES) */

/* Initialize static fields of addrinfo structure. A macro to let it work
 * with any sockaddr_* type.
 */
#define INIT_ADDRINFO(addrinfo, sockaddr) { \
		(addrinfo)->ai_addr = &(addrinfo)->_ai_addr; \
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
	struct zsock_addrinfo *ai_arr;
};

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info, void *user_data)
{
	struct getaddrinfo_state *state = user_data;
	struct zsock_addrinfo *ai;
	int socktype = SOCK_STREAM;

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
		NET_DBG("getaddrinfo entries overflow");
		return;
	}

	ai = &state->ai_arr[state->idx];
	if (state->idx > 0) {
		state->ai_arr[state->idx - 1].ai_next = ai;
	}

	memcpy(&ai->_ai_addr, &info->ai_addr, info->ai_addrlen);
	net_sin(&ai->_ai_addr)->sin_port = state->port;
	ai->ai_addr = &ai->_ai_addr;
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
	ai->ai_protocol = (socktype == SOCK_DGRAM) ? IPPROTO_UDP : IPPROTO_TCP;

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

	if (family == AF_INET6) {
		qtype = DNS_QUERY_TYPE_AAAA;
	}

again:
	timeout_ms = k_ticks_to_ms_ceil32(timeout.ticks);

	NET_DBG("Timeout %d", timeout_ms);

	ret = dns_get_addr_info(host, qtype, &ai_state->dns_id,
				dns_resolve_cb, ai_state, timeout_ms);
	if (ret == 0) {
		/* If the DNS query for reason fails so that the
		 * dns_resolve_cb() would not be called, then we want the
		 * semaphore to timeout so that we will not hang forever.
		 * So make the sem timeout longer than the DNS timeout so that
		 * we do not need to start to cancel any pending DNS queries.
		 */
		ret = k_sem_take(&ai_state->sem, K_MSEC(timeout_ms + 100));
		if (ret == -EAGAIN) {
			if (!sys_timepoint_expired(end)) {
				timeout = recalc_timeout(end, timeout);
				goto again;
			}

			(void)dns_cancel_addr_info(ai_state->dns_id);
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
	} else {
		errno = -ret;
		st = DNS_EAI_SYSTEM;
	}

	return st;
}

static int getaddrinfo_null_host(int port, const struct zsock_addrinfo *hints,
				struct zsock_addrinfo *res)
{
	if (!hints || !(hints->ai_flags & AI_PASSIVE)) {
		return DNS_EAI_FAIL;
	}

	/* For AF_UNSPEC, should we default to IPv6 or IPv4? */
	if (hints->ai_family == AF_INET || hints->ai_family == AF_UNSPEC) {
		struct sockaddr_in *addr = net_sin(&res->_ai_addr);
		addr->sin_addr.s_addr = INADDR_ANY;
		addr->sin_port = htons(port);
		addr->sin_family = AF_INET;
		INIT_ADDRINFO(res, addr);
		res->ai_family = AF_INET;
	} else if (hints->ai_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = net_sin6(&res->_ai_addr);
		addr6->sin6_addr = in6addr_any;
		addr6->sin6_port = htons(port);
		addr6->sin6_family = AF_INET6;
		INIT_ADDRINFO(res, addr6);
		res->ai_family = AF_INET6;
	} else {
		return DNS_EAI_FAIL;
	}

	if (hints->ai_socktype == SOCK_DGRAM) {
		res->ai_socktype = SOCK_DGRAM;
		res->ai_protocol = IPPROTO_UDP;
	} else {
		res->ai_socktype = SOCK_STREAM;
		res->ai_protocol = IPPROTO_TCP;
	}
	return 0;
}

int z_impl_z_zsock_getaddrinfo_internal(const char *host, const char *service,
				       const struct zsock_addrinfo *hints,
				       struct zsock_addrinfo *res)
{
	int family = AF_UNSPEC;
	int ai_flags = 0;
	long int port = 0;
	int st1 = DNS_EAI_ADDRFAMILY, st2 = DNS_EAI_ADDRFAMILY;
	struct sockaddr *ai_addr;
	struct getaddrinfo_state ai_state;

	if (hints) {
		family = hints->ai_family;
		ai_flags = hints->ai_flags;

		if ((family != AF_UNSPEC) && (family != AF_INET) && (family != AF_INET6)) {
			return DNS_EAI_ADDRFAMILY;
		}
	}

	if (ai_flags & AI_NUMERICHOST) {
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
	ai_state.port = htons(port);
	ai_state.ai_arr = res;
	ai_state.dns_id = 0;
	k_sem_init(&ai_state.sem, 0, K_SEM_MAX_LIMIT);

	/* If family is AF_UNSPEC, then we query IPv4 address first
	 * if IPv4 is enabled in the config.
	 */
	if ((family != AF_INET6) && IS_ENABLED(CONFIG_NET_IPV4)) {
		st1 = exec_query(host, AF_INET, &ai_state);
		if (st1 == DNS_EAI_AGAIN) {
			return st1;
		}
	}

	/* If family is AF_UNSPEC, the IPv4 query has been already done
	 * so we can do IPv6 query next if IPv6 is enabled in the config.
	 */
	if ((family != AF_INET) && IS_ENABLED(CONFIG_NET_IPV6)) {
		st2 = exec_query(host, AF_INET6, &ai_state);
		if (st2 == DNS_EAI_AGAIN) {
			return st2;
		}
	}

	for (uint16_t idx = 0; idx < ai_state.idx; idx++) {
		ai_addr = &ai_state.ai_arr[idx]._ai_addr;
		net_sin(ai_addr)->sin_port = htons(port);
	}

	/* If both attempts failed, it's error */
	if (st1 && st2) {
		if (st1 != DNS_EAI_ADDRFAMILY) {
			return st1;
		}
		return st2;
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
	int family = AF_UNSPEC;
	int resolved_family = AF_UNSPEC;
	long port = 0;
	bool result;
	int socktype = SOCK_STREAM;
	int protocol = IPPROTO_TCP;

	if (!host) {
		return DNS_EAI_NONAME;
	}

	if (hints) {
		family = hints->ai_family;
		if (hints->ai_socktype == SOCK_DGRAM) {
			socktype = SOCK_DGRAM;
			protocol = IPPROTO_UDP;
		}
	}

	result = net_ipaddr_parse(host, strlen(host), &res->_ai_addr);

	if (!result) {
		return DNS_EAI_NONAME;
	}

	resolved_family = res->_ai_addr.sa_family;

	if ((family != AF_UNSPEC) && (resolved_family != family)) {
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
	case AF_INET:
	{
		struct sockaddr_in *addr =
			(struct sockaddr_in *)&res->_ai_addr;

		INIT_ADDRINFO(res, addr);
		addr->sin_port = htons(port);
		addr->sin_family = AF_INET;
		break;
	}

	case AF_INET6:
	{
		struct sockaddr_in6 *addr =
			(struct sockaddr_in6 *)&res->_ai_addr;

		INIT_ADDRINFO(res, addr);
		addr->sin6_port = htons(port);
		addr->sin6_family = AF_INET6;
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
	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD)) {
		return socket_offload_getaddrinfo(host, service, hints, res);
	}

	int ret = DNS_EAI_FAIL;

#if defined(ANY_RESOLVER)
	*res = calloc(AI_ARR_MAX, sizeof(struct zsock_addrinfo));
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
		free(*res);
		*res = NULL;
	}
#endif

	return ret;
}

void zsock_freeaddrinfo(struct zsock_addrinfo *ai)
{
	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD)) {
		socket_offload_freeaddrinfo(ai);
		return;
	}

	free(ai);
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
