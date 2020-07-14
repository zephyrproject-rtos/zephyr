/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* libc headers */
#include <stdlib.h>
#include <ctype.h>

/* Zephyr headers */
#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock_addr, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <kernel.h>
#include <net/socket.h>
#include <net/socket_offload.h>
#include <syscall_handler.h>

#define AI_ARR_MAX	2

#if defined(CONFIG_DNS_RESOLVER)

/* Helper macros which take into account the fact that ai_family as passed
 * into getaddrinfo() may take values AF_INET, AF_INET6, or AF_UNSPEC, where
 * AF_UNSPEC means resolve both AF_INET and AF_INET6.
 */
#define RESOLVE_IPV4(ai_family) \
	(IS_ENABLED(CONFIG_NET_IPV4) && (ai_family) != AF_INET6)
#define RESOLVE_IPV6(ai_family) \
	(IS_ENABLED(CONFIG_NET_IPV6) && (ai_family) != AF_INET)

struct getaddrinfo_state {
	const struct zsock_addrinfo *hints;
	struct k_sem sem;
	int status;
	uint16_t idx;
	uint16_t port;
	struct zsock_addrinfo *ai_arr;
};

/* Initialize static fields of addrinfo structure. A macro to let it work
 * with any sockaddr_* type.
 */
#define INIT_ADDRINFO(addrinfo, sockaddr) { \
		(addrinfo)->ai_addr = &(addrinfo)->_ai_addr; \
		(addrinfo)->ai_addrlen = sizeof(*sockaddr); \
		(addrinfo)->ai_canonname = (addrinfo)->_ai_canonname; \
		(addrinfo)->_ai_canonname[0] = '\0'; \
		(addrinfo)->ai_next = NULL; \
	}

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info, void *user_data)
{
	struct getaddrinfo_state *state = user_data;
	struct zsock_addrinfo *ai;
	int socktype = SOCK_STREAM;
	int proto;

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

	proto = IPPROTO_TCP;
	if (socktype == SOCK_DGRAM) {
		proto = IPPROTO_UDP;
	}

	ai->ai_socktype = socktype;
	ai->ai_protocol = proto;

	state->idx++;
}

static int exec_query(const char *host, int family,
		      struct getaddrinfo_state *ai_state)
{
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		qtype = DNS_QUERY_TYPE_AAAA;
	}

	return dns_get_addr_info(host, qtype, NULL,
				 dns_resolve_cb, ai_state,
				 CONFIG_NET_SOCKETS_DNS_TIMEOUT);
}

static int getaddrinfo_null_host(int port, const struct zsock_addrinfo *hints,
				struct zsock_addrinfo *res)
{
	if (hints && (hints->ai_flags & AI_PASSIVE)) {
		struct sockaddr_in *addr =
		    (struct sockaddr_in *)&res->_ai_addr;
		addr->sin_addr.s_addr = INADDR_ANY;
		addr->sin_port = htons(port);
		addr->sin_family = AF_INET;
		INIT_ADDRINFO(res, addr);
		res->ai_family = AF_INET;
		res->ai_socktype = SOCK_STREAM;
		res->ai_protocol = IPPROTO_TCP;
		return 0;
	}

	return DNS_EAI_FAIL;
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
	int ret;
	struct getaddrinfo_state ai_state;

	if (hints) {
		family = hints->ai_family;
		ai_flags = hints->ai_flags;
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

#define SIN_ADDR(ptr) (net_sin(ptr)->sin_addr)

	/* Check for IPv4 numeric address. Start with a quick heuristic check,
	 * of first char of the address, then do long validating inet_pton()
	 * call if needed.
	 */
	if (RESOLVE_IPV4(family) &&
	    isdigit((int)*host) &&
	    zsock_inet_pton(AF_INET, host,
			    &SIN_ADDR(&res->_ai_addr)) == 1) {
		struct sockaddr_in *addr =
			(struct sockaddr_in *)&res->_ai_addr;

		addr->sin_port = htons(port);
		addr->sin_family = AF_INET;
		INIT_ADDRINFO(res, addr);
		res->ai_family = AF_INET;
		res->ai_socktype = SOCK_STREAM;
		res->ai_protocol = IPPROTO_TCP;
		return 0;
	}

	if (ai_flags & AI_NUMERICHOST) {
		/* Asked to resolve host as numeric, but it wasn't possible
		 * to do that.
		 */
		return DNS_EAI_FAIL;
	}

	ai_state.hints = hints;
	ai_state.idx = 0U;
	ai_state.port = htons(port);
	ai_state.ai_arr = res;
	k_sem_init(&ai_state.sem, 0, UINT_MAX);

	/* Link entries in advance */
	ai_state.ai_arr[0].ai_next = &ai_state.ai_arr[1];

	/* If the family is AF_UNSPEC, then we query IPv4 address first */
	ret = exec_query(host, family, &ai_state);
	if (ret == 0) {
		/* If the DNS query for reason fails so that the
		 * dns_resolve_cb() would not be called, then we want the
		 * semaphore to timeout so that we will not hang forever.
		 * So make the sem timeout longer than the DNS timeout so that
		 * we do not need to start to cancel any pending DNS queries.
		 */
		int ret = k_sem_take(&ai_state.sem,
				     K_MSEC(CONFIG_NET_SOCKETS_DNS_TIMEOUT +
					    100));
		if (ret == -EAGAIN) {
			return DNS_EAI_AGAIN;
		}

		st1 = ai_state.status;
	} else {
		/* If we are returned -EPFNOSUPPORT then that will indicate
		 * wrong address family type queried. Check that and return
		 * DNS_EAI_ADDRFAMILY and set errno to EINVAL.
		 */
		if (ret == -EPFNOSUPPORT) {
			errno = EINVAL;
			st1 = DNS_EAI_ADDRFAMILY;
		} else {
			errno = -ret;
			st1 = DNS_EAI_SYSTEM;
		}
	}

	/* If family is AF_UNSPEC, the IPv4 query has been already done
	 * so we can do IPv6 query next if IPv6 is enabled in the config.
	 */
	if (family == AF_UNSPEC && IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = exec_query(host, AF_INET6, &ai_state);
		if (ret == 0) {
			int ret = k_sem_take(
				&ai_state.sem,
				K_MSEC(CONFIG_NET_SOCKETS_DNS_TIMEOUT + 100));
			if (ret == -EAGAIN) {
				return DNS_EAI_AGAIN;
			}

			st2 = ai_state.status;
		} else {
			errno = -ret;
			st2 = DNS_EAI_SYSTEM;
		}
	}

	if (ai_state.idx > 0) {
		ai_addr = &ai_state.ai_arr[ai_state.idx - 1]._ai_addr;
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
		Z_OOPS(z_user_from_copy(&hints_copy, (void *)hints,
					sizeof(hints_copy)));
	}
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_WRITE(res, AI_ARR_MAX,
					    sizeof(struct zsock_addrinfo)));

	if (service) {
		service_copy = z_user_string_alloc_copy((char *)service, 64);
		if (!service_copy) {
			ret = DNS_EAI_MEMORY;
			goto out;
		}
	}

	if (host) {
		host_copy = z_user_string_alloc_copy((char *)host, 64);
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
#include <syscalls/z_zsock_getaddrinfo_internal_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* defined(CONFIG_DNS_RESOLVER) */

int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD)) {
		return socket_offload_getaddrinfo(host, service, hints, res);
	}

#if defined(CONFIG_DNS_RESOLVER)
	int ret;

	*res = calloc(AI_ARR_MAX, sizeof(struct zsock_addrinfo));
	if (!(*res)) {
		return DNS_EAI_MEMORY;
	}
	ret = z_zsock_getaddrinfo_internal(host, service, hints, *res);
	if (ret) {
		free(*res);
		*res = NULL;
	}
	return ret;
#endif

	return DNS_EAI_FAIL;
}

void zsock_freeaddrinfo(struct zsock_addrinfo *ai)
{
	if (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD)) {
		return socket_offload_freeaddrinfo(ai);
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
	ERR(EAI_MEMORY);
	ERR(EAI_SYSTEM);
	ERR(EAI_SERVICE);

	default:
		return "EAI_UNKNOWN";
	}
}
#undef ERR
